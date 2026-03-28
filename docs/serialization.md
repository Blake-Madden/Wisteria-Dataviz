# Serialization Architecture

This document describes how graph objects, datasets, and project settings are serialized (saved) and deserialized (loaded) in *Wisteria-Dataviz*.

## Overview

Projects are stored as JSON files.
The serialization system has four main participants:

1. **ReportBuilder** (`src/base/reportbuilder.h/.cpp`) — reads JSON into live objects
2. **WisteriaDoc** (`src/app/wisteriadoc.cpp`) — writes live objects back to JSON
3. **Graph classes** (`src/graphs/`) — the live graph objects that hold runtime state
4. **Insert/Edit dialogs** (`src/ui/dialogs/`) — UI for creating and editing graphs

The JSON format is the single source of truth for project persistence.
A project file contains top-level sections for `name`, `watermark`, `print`, `datasets`, `constants`, and `pages`.

## JSON Project Structure

```json
{
  "name": "My Report",
  "watermark": { "label": "DRAFT", "color": "red" },
  "print": { "orientation": "landscape", "paper-size": "letter" },
  "constants": [
    { "name": "MaxSemester", "value": "Fall 2025" }
  ],
  "datasets": [
    {
      "name": "Students",
      "path": "data/students.csv",
      "categorical-columns": [...],
      "subsets": [...],
      "pivots": [...],
      "merges": [...]
    }
  ],
  "pages": [
    {
      "name": "Page 1",
      "rows": [
        {
          "items": [
            {
              "type": "scatter-plot",
              "dataset": "Students",
              "variables": { "x": "GPA", "y": "SAT", "group": "{{GroupVar}}" },
              "show-regression-lines": true,
              "title": { "text": "SAT vs GPA — {{MaxSemester}}" }
            }
          ]
        }
      ]
    }
  ]
}
```

## Loading (JSON to Live Objects)

Entry point: `ReportBuilder::LoadConfigurationFile()`.

### Step 1: Load constants and datasets

Constants are loaded first into an internal `m_values` map (name -> string or double).
Datasets are loaded next, including nested subsets, pivots, and merges.
Each dataset is stored in `m_datasets` by name, and its import options are preserved in `m_datasetImportOptions` for later round-trip serialization.

### Step 2: Load pages and items

Pages are iterated row-by-row.
Each item in a row is dispatched by its `"type"` field to a type-specific loader:

- `"scatter-plot"` -> `LoadScatterPlot()`
- `"line-plot"` -> `LoadLinePlot()`
- `"histogram"` -> `LoadHistogram()`
- `"label"` -> `LoadLabel()`
- etc.

### Step 3: Type-specific loader (e.g., `LoadScatterPlot`)

A type-specific loader does the following:

1. Looks up the dataset by name from `m_datasets`
2. Reads variable names from the `"variables"` node
3. Expands any placeholder values (double-brace syntax) via `ExpandConstants()`
4. Caches the raw (unexpanded) template strings on the graph via `SetPropertyTemplate()`
5. Calls `SetData()` on the graph with the expanded values
6. Reads type-specific options (e.g., `show-regression-lines`, `confidence-level`)
7. Delegates to `LoadGraph()` for common graph settings

Example from `LoadScatterPlot`:

```cpp
const auto groupVarNameRaw = variablesNode->GetProperty("group")->AsString();
const auto groupVarName = ExpandConstants(groupVarNameRaw);

auto scatterPlot = std::make_shared<Graphs::ScatterPlot>(canvas, ...);
scatterPlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
scatterPlot->SetData(dataset, yExpanded, xExpanded, groupVarName);
```

### Step 4: Common graph loading (`LoadGraph`)

`LoadGraph()` handles properties shared by all graph types:

- Caches the dataset name as a property template
- Loads title, subtitle, caption (as `Label` objects)
- Background color and image
- Axis configuration (titles, ranges, pens, brackets, custom labels)
- Legend creation and placement on the canvas
- Common item properties (scaling, margins, padding, alignment)

## Saving (Live Objects to JSON)

Entry point: `WisteriaDoc::SaveProject()`.

### Step 1: Build root JSON skeleton

Creates the root object with `name`, `watermark`, `print`, `datasets`, `constants`, and `pages` sections.

### Step 2: Save datasets

Iterates datasets in insertion order.
For each top-level dataset (those with a file path), writes:

- `name` (only if it differs from the file stem)
- `path` (made relative to the project directory)
- Import options (worksheet, importer override, column types)
- Transform options (renames, recodes, mutations, formulas)
- Nested subsets, pivots, and merges

### Step 3: Save constants

Writes name/value pairs from `m_constants`.

### Step 4: Save pages

For each canvas (page):

1. Iterates the fixed-object grid row by row.
2. Each item is dispatched through `SavePageItem()`, which uses `IsKindOf()` to route to the correct saver:
   - `Graph2D` -> `SaveGraphByType()`
   - `FillableShape` -> `SaveFillableShape()`
   - `Label` -> `SaveLabel()`
   - `Image` -> `SaveImage()`
   - `Axis` -> `SaveCommonAxis()` (for common/shared axes)
3. Legend labels are skipped (they are serialized as part of their parent graph).

### Step 5: `SaveGraphByType()` and `SaveGraph()`

`SaveGraphByType()` does the following:

1. Determines the graph's type string (e.g., `"scatter-plot"`)
2. Creates a JSON node with the type and placeholder arrays for margins/padding/outline
3. Calls `SaveGraph()` for common graph properties
4. Writes type-specific properties based on `IsKindOf()` checks

`SaveGraph()` handles common properties:

- **Dataset name**: Retrieved from `GetPropertyTemplate("dataset")` — this preserves any double-brace placeholder syntax
- **Variables**: Iterates all property templates with `"variables."` prefix
  Indexed templates like `"variables.y[0]"`, `"variables.y[1]"` are collapsed into JSON arrays
- **Title/subtitle/caption**: Serialized via `SaveLabelPropertiesToStr()`, which prefers the property template over the display text to preserve constant placeholders
- **Axes**: All four axes serialized with titles, pens, gridlines, brackets, tick marks
- **Background**: Color and image properties (with template-cached paths)
- **Legend**: Placement hint and options

### Type-specific save example (`ScatterPlot`)

```cpp
const auto* scatterPlot = dynamic_cast<const Graphs::ScatterPlot*>(graph);
if (scatterPlot->IsShowingRegressionLines())
    node->Add(L"show-regression-lines", true);
if (scatterPlot->IsShowingConfidenceBands())
    node->Add(L"show-confidence-bands", true);
if (!compare_doubles(scatterPlot->GetConfidenceLevel(), 0.95))
    node->Add(L"confidence-level", scatterPlot->GetConfidenceLevel());
// regression-line-scheme (custom line styles)...
```

## Property Templates

Property templates are the mechanism that enables round-trip serialization of double-brace placeholder values.
Without them, expanded runtime values would be saved instead of the original template strings, breaking the dynamic nature of constants.

### Storage

Each `GraphItemBase` has a `std::map<wxString, wxString> m_propertyTemplates` (in `graphitems.h`).
Keys are dot-separated property paths like:

- `"dataset"` — the dataset reference
- `"variables.x"` — the X variable name
- `"variables.y[0]"` — first Y variable (for multi-series)
- `"text"` — a label's text content
- `"color"` — a label's font color
- `"pen.color"` — a pen's color
- `"image-import.path"` — a background image path

### ExpandAndCache pattern

During loading, `ExpandAndCache()` (in `reportbuilder.h`) performs both steps in one call:

```cpp
wxString ExpandAndCache(GraphItemBase* item, const wxString& property,
                        const wxString& rawValue) const
    {
    const wxString expanded = ExpandConstants(rawValue);
    if (item != nullptr)
        item->SetPropertyTemplate(property, rawValue);
    return expanded;
    }
```

This stores the raw template string (e.g., the unexpanded constant name) while returning `"Fall 2025"` for runtime use.

### During saving

`SaveGraph()` retrieves templates via `GetPropertyTemplate()`.
If a template exists, it is written to JSON instead of the runtime value.
Example:

```cpp
// saves "{{MaxSemester}}" not "Fall 2025"
const auto textTmpl = label.GetPropertyTemplate(L"text");
const auto& text = textTmpl.empty() ? label.GetText() : textTmpl;
```

## Constants and ExpandConstants

Constants are user-defined name/value pairs from the `"constants"` JSON section.
They can also come from dataset formulas (e.g., ``"MAX(`GPA`)"``) which are evaluated at load time and stored in the `m_values` map.

`ExpandConstants()` (in `reportbuilder.cpp`) uses regex to find all double-brace patterns in a string and replaces them with their values from `m_values`.
(Numeric values are formatted with thousands separators.)
The function also supports calculated expressions (e.g., `Now()` wrapped in double braces).

Constants enable a single JSON template to produce different outputs when the underlying data changes — variable names, filter values, titles, and colors can all reference constants.

## Insert/Edit Dialogs

### Inserting a new graph

1. The view creates and shows the dialog (e.g., `InsertScatterPlotDlg`).
2. On `OK`, the view constructs a new graph object.
3. Dialog settings are applied to the graph (`ApplyGraphOptions`, `ApplyPageOptions`, plus type-specific setters).
4. `SetData()` is called with the selected dataset and variables.
5. Property templates are set explicitly for round-tripping:

```cpp
plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
plot->SetPropertyTemplate(L"variables.y", dlg.GetYVariable());
plot->SetPropertyTemplate(L"variables.x", dlg.GetXVariable());
```

6. The graph (and optionally its legend) is placed on the canvas.

### Editing an existing graph

1. The dialog is opened in `EditMode::Edit`.
2. `LoadFromGraph()` reads the existing graph's state back into the dialog controls.
   It reads both the graph's live properties and its property templates.
3. On `OK`, a new graph object is created (not mutated in place).
4. A `carryForward` lambda preserves double-brace placeholder templates when the user hasn't changed the corresponding value:

```cpp
const auto carryForward = [&](const wxString& prop, const wxString& newVal,
                              const wxString& oldExpanded) {
    if (newVal != oldExpanded || newVal.empty())
        plot->SetPropertyTemplate(prop, newVal);       // user changed it
    else
        {
        const auto oldTemplate = graph.GetPropertyTemplate(prop);
        plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
        }
};
```

This ensures that if a user edits a graph loaded from a template file but doesn't change the X variable, the original placeholder is preserved rather than being replaced with the expanded column name.

## Color Serialization

`WisteriaDoc::ColorToStr()` converts colors back to strings using a priority order:

1. **Constants table** — if the hex value matches a constant, returns the double-brace placeholder.
2. **Named Wisteria colors** — checks the built-in color map for a named match.
3. **Hex fallback** — returns the HTML hex string (e.g., `"#FF6600"`).

This preserves constant references and readable color names over raw hex values.

## Enum Conversion

`ReportEnumConvert` (`src/base/reportenumconvert.h`) provides bidirectional conversion between C++ enums and their JSON string representations.
This covers pen styles, brush styles, axis types, icon shapes, text alignment, page alignment, and many other enumerated settings.
Both the load side (`ReportBuilder`) and the save side (`WisteriaDoc`) use these converters to translate between JSON strings and C++ enums.

## Adding Serialization for a New Graph Type

When adding a new graph type (e.g., `FooPlot`), the following files need changes to support full round-trip serialization:

### 1. Graph class (`src/graphs/fooplot.h/.cpp`)

- Implement the graph with its `SetData()`, `RecalcSizes()`, and `CreateLegend()` methods as usual
- Expose getters for any type-specific options that need to be serialized (e.g., `IsShowingSomething()`, `GetSomeLevel()`)

### 2. Loading: `ReportBuilder` (`src/base/reportbuilder.h/.cpp`)

- **Declare** `LoadFooPlot()` in `reportbuilder.h` following the existing pattern (takes a `graphNode`, `canvas`, `currentRow`, `currentColumn`; returns `std::shared_ptr<Graphs::Graph2D>`)
- **Implement** `LoadFooPlot()` in `reportbuilder.cpp`:
  1. Look up the dataset from `m_datasets` using the `"dataset"` node
  2. Read variables from the `"variables"` node. Use `ExpandAndCache()` for each variable so the raw template is cached and the expanded value is returned
  3. Construct the graph, call `SetData()` with expanded values
  4. Read type-specific options from the JSON node
  5. Call `LoadGraph()` at the end to handle common properties (title, axes, legend, etc.)
- **Add a dispatch branch** in the item-loading section of `LoadConfigurationFile()` where type strings are compared. Add a case for your type string (e.g., `"foo-plot"`).

### 3. Saving: `WisteriaDoc` (`src/app/wisteriadoc.h/.cpp`)

- **Add a type-string mapping** in `GetGraphTypeString()`.
  Use `IsKindOf()` to check for your class and return the type string (e.g., `"foo-plot"`).
  If your class inherits from another graph class that is already in the list, place your check *before* the parent class check (derived classes must be checked first).
- **Add a type-specific branch** in `SaveGraphByType()`.
  Use `IsKindOf()` to detect your type, `dynamic_cast` to access it, and write any non-default type-specific options to the JSON node.
  Common properties (dataset, variables, title, axes, legend) are already handled by `SaveGraph()`.

### 4. Insert dialog (`src/ui/dialogs/insertfooplotdlg.h/.cpp`)

- Create the dialog inheriting from `InsertGraphDlg`
- Provide controls for dataset selection, variable selection, and type-specific options
- Implement `Validate()` to check required fields
- Implement `LoadFromGraph()` for the edit flow — read the graph's live properties and property templates back into the dialog controls

### 5. View integration (`src/app/wisteriaview.h/.cpp`)

- **Insert handler** (`OnInsertFooPlot`):
  1. Show the dialog
  2. Construct the graph and apply settings from the dialog
  3. Call `SetData()`
  4. **Cache property templates** for dataset and each variable via `SetPropertyTemplate()`
  5. Place the graph (and legend) on the canvas
- **Edit handler** (`EditFooPlot`):
  1. Show the dialog in `EditMode::Edit`, calling `LoadFromGraph()`
  2. On OK, construct a new graph (do not mutate in place)
  3. Use the `carryForward` pattern to preserve double-brace placeholders for any values the user did not change
  4. Replace the old graph on the canvas
- **Add dispatch branches** in the edit-graph routing (where `IsKindOf()` checks dispatch to `EditScatterPlot`, `EditLinePlot`, etc.)

### Inheritance ordering note

Both `GetGraphTypeString()` and `SaveGraphByType()` use `IsKindOf()` chains.
Since `IsKindOf()` returns true for parent classes, derived types must appear before their base types.
For example, `BubblePlot` (which inherits from `ScatterPlot`) is checked before `ScatterPlot`, and `WCurvePlot` (which inherits from `LinePlot`) is checked before `LinePlot`.

## Data Flow Summary

```
JSON File
  |
  | LoadConfigurationFile()
  v
ReportBuilder
  |-- LoadConstants() -> m_values map
  |-- LoadDatasets()  -> m_datasets map + m_datasetImportOptions
  |-- LoadScatterPlot() / LoadLinePlot() / ...
  |     |-- ExpandAndCache() stores raw templates, returns expanded values
  |     |-- Graph::SetData() with expanded values
  |     |-- LoadGraph() for common properties
  |     '-- Canvas placement
  v
Live Canvas + Graph Objects (with cached property templates)
  |
  | SaveProject()
  v
WisteriaDoc
  |-- SavePageItem() dispatches by type
  |     |-- SaveGraphByType() -> SaveGraph() + type-specific
  |     |     |-- GetPropertyTemplate() retrieves raw templates
  |     |     '-- Writes placeholders back to JSON
  |     |-- SaveLabel() / SaveImage() / SaveShape() / ...
  |-- Datasets, constants, print settings
  v
JSON File (round-tripped)
```
