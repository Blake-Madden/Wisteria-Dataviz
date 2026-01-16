# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Important: User Preferences

- **No Git**: Do NOT run any git commands (commit, add, push, status, diff, etc.). The user handles all version control.
- **One edit at a time**: Make edits one by one and wait for user approval before proceeding to the next edit.
- **Ignore submodules**: Always exclude these directories from any review, search, or modification:
  - `src/easyexif`
  - `src/wxSimpleJSON`
  - `src/CRCpp`
  - `src/utfcpp`
  - `src/wxStartPage`

## Build Commands

**Windows (Visual Studio):**
```bash
# Open CMakeLists.txt in Visual Studio, configure Release build, then build targets
# Build targets: demo, wisteria (library), doxygen-docs
```

**Linux/macOS:**
```bash
cmake . -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4 --config Release
```

**Build targets:**
- `wisteria` - Static library
- `demo` - Demo application
- `doxygen-docs` - API documentation (requires Doxygen)
- `quarto-docs` - JSON syntax documentation (requires Quarto)

## Running Tests

There are two test suites using Catch2:

**Non-GUI tests** (tests/):
```bash
cd tests
cmake . -DCMAKE_BUILD_TYPE=Debug
cmake --build .
./bin/WisteriaTestRunner
# Or run specific test: ./bin/WisteriaTestRunner "[tagname]"
```

**GUI tests** (tests/gui-tests/):
```bash
cd tests/gui-tests
cmake . -DCMAKE_BUILD_TYPE=Debug
cmake --build .
./bin/WisteriaGuiTestRunner
```

## Code Analysis

The project uses these static analysis tools (run via GitHub Actions):
- cppcheck
- clang-tidy (enable with `-DUSE_CLANG_TIDY=ON`)
- MS PREfast (Windows)
- clang-format for formatting

## Architecture Overview

### Source Structure

- `src/base/` - Core graphics infrastructure (Canvas, Axis, Label, GraphItemBase)
- `src/graphs/` - Graph implementations (all chart types)
- `src/data/` - Dataset class and data transformations (pivot, subset, join)
- `src/import/` - File parsers (Excel, HTML, DOCX, CSV, etc.)
- `src/math/` - Statistics and mathematical utilities
- `src/util/` - File I/O, string operations, logging
- `src/ui/` - Dialogs and UI components

### Key Class Hierarchy

```
GraphItemBase (wxObject)
    └── Graph2D (abstract base for all 2D plots)
            ├── GroupGraph2D (for grouped/categorical data)
            │       ├── BarChart, CategoricalBarChart
            │       ├── LinePlot, WCurvePlot
            │       ├── Histogram
            │       └── BoxPlot, HeatMap
            └── Direct subclasses
                    ├── PieChart
                    ├── Table
                    ├── SankeyDiagram
                    ├── GanttChart
                    └── WordCloud, WaffleChart, Roadmap variants
```

### Core Components

**Canvas** (`src/base/canvas.h`): wxScrolledWindow-derived rendering surface. Manages a grid of fixed objects (graphs, legends, labels). Handles export to SVG, PNG, PDF.

**Dataset** (`src/data/dataset.h`): Column-based data storage supporting continuous (double), categorical (with string tables), date, and ID columns. Used by all graph types via `SetData()`.

**Graph2D** (`src/graphs/graph2d.h`): Abstract base providing axes (top/bottom/left/right), titles, captions, reference lines, and annotations. All chart types inherit from this.

### Typical Workflow

1. Create a `Canvas` and embed it in a wxFrame/wxDialog
2. Import data into a `Dataset` using `ImportCSV()`, `ImportXLSX()`, etc.
3. Create a graph (e.g., `LinePlot`, `BarChart`) and call `SetData()` with the dataset
4. Customize axes, colors, titles via graph methods
5. Add graph to canvas via `SetFixedObject(row, col, graph)`

### JSON-based Reports

`ReportBuilder` (`src/base/reportbuilder.h`) can generate complete multi-page reports from JSON configuration files, creating datasets, graphs, and layouts declaratively.

## Dependencies

- wxWidgets 3.3.1+ (sibling directory expected)
- C++20 compiler
- CMake 3.25+
- Catch2 (for tests, sibling directory or system install)
- Linux: GTK3, libtbb, OpenMP

## Submodules

Located in `src/`: easyexif, wxSimpleJSON, CRCpp, utfcpp, wxStartPage - exclude these from code searches/modifications.
