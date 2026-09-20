Adding a New Shape
=============================

A shape (or icon) is a drawable element that can be used canvases, plots, and legends.
The following is an overview of the files involved and the changes needed when adding a new shape.
The `HawaiianPizza` shape is used as an example throughout.

Enum Declaration
=============================

In `src/base/icons.h`, add the new value(s) to the `IconShape` enum:

```cpp
HawaiianPizza             /*!< A Hawaiian pizza (pepperoni and pineapple).*/
```

Shape Rendering Declaration
=============================

In `src/base/shapes.h`, declare the draw function(s) in the `ShapeRenderer` class, within the "Shape Rendering Functions" section:

```cpp
/// @brief Draws a Hawaiian pizza (pepperoni and pineapple).
/// @param rect The area to draw the image within.
/// @param dc The DC to draw to.
void DrawHawaiianPizza(wxRect rect, wxDC& dc) const;
```

Shape Rendering Implementation
=============================

Two changes are needed, in two different places.

First, in `src/base/shapes.cpp`, register the function in the `shapeMap` inside the `Shape` constructor:

```cpp
{ Icons::IconShape::HawaiianPizza, &ShapeRenderer::DrawHawaiianPizza }
```

Then implement the draw function.
The draw functions are split by category into `src/base/shapes_*.cpp` files.
Add the function to the file that fits the new shape (or to the closest match):

- `shapes_art.cpp`
- `shapes_buildings.cpp`
- `shapes_business.cpp`
- `shapes_education.cpp`
- `shapes_food.cpp` (e.g., `DrawHawaiianPizza`)
- `shapes_geometric.cpp`
- `shapes_medical.cpp`
- `shapes_nature.cpp`
- `shapes_people.cpp`
- `shapes_religion.cpp`
- `shapes_stats.cpp`
- `shapes_vehicles.cpp`

A few conventions to follow:

- Use `GraphicsContextFallback` to acquire a `wxGraphicsContext` for advanced rendering.
- Use `GetGraphItemInfo().GetBrush()` for the fill color when applicable.
- Use `ScaleToScreenAndCanvas()` for DPI-aware pixel values.
- Use proportional sizing (based on `rect` dimensions) so the shape scales properly with zoom.
- Use `GetRadius(rect)` for circular shapes.
- Use `GetMidPoint(rect)` to find the center of the drawing area.
- Shapes can call other shape renderers for composition (e.g., `DrawPepperoniPizza` calls
  `DrawCheesePizza` first, then adds pepperoni on top).

Example implementation pattern:

```cpp
void ShapeRenderer::DrawCheesePizza(const wxRect rect, wxDC& dc) const
    {
    const wxDCPenChanger pc{ dc, *wxTRANSPARENT_PEN };
    const wxDCBrushChanger bc{ dc, *wxTRANSPARENT_BRUSH };

    const GraphicsContextFallback gcf{ &dc, rect };
    auto* gc = gcf.GetGraphicsContext();
    if (gc == nullptr)
        {
        return;
        }

    // Drawing code here using gc->DrawEllipse(), gc->FillPath(), etc.
    }
```

JSON Report Support
=============================

In `src/reporting/reportenumconvert.h`, add the string mapping in both directions so that the
shape can be read from (and written to) JSON report files.

First, add the string-to-enum mapping to the `m_iconEnums` map (used by `ConvertIcon()`):

```cpp
{ L"hawaiian-pizza", Icons::IconShape::HawaiianPizza }
```

Then add the enum-to-string mapping to the map inside `ConvertIconToString()`:

```cpp
{ Icons::IconShape::HawaiianPizza, L"hawaiian-pizza" }
```

Accessibility Name
=============================

In `src/base/shapes.cpp`, add a `case` to `ShapeInfo::GetReadableShapeName()` so that screen
readers and other accessibility descriptions can refer to the shape by a natural-language label:

```cpp
case Icons::IconShape::HawaiianPizza:
    return _(L"Hawaiian pizza");
```

UI Support
=============================

In `src/ui/dialogs/editors/insertshapedlg.cpp`, add the shape to the `shapes` list in `PopulateShapeChoice()` so that it can be selected from the "Insert Shape" dialog (the list is alphabetical by label, so insert it in the appropriate place):

```cpp
{ _(L"Hawaiian pizza"), Icons::IconShape::HawaiianPizza }
```

Documentation
=============================

In `docs/syntax-manual/graphs-properties.qmd`, add the icon shape string to the `"icon-scheme"` list:

```markdown
  - `"hawaiian-pizza"`
```

Summary of Files to Modify
=============================

1. `src/base/icons.h` - Add enum value(s) to `IconShape`
2. `src/base/shapes.h` - Declare draw function in `ShapeRenderer`
3. `src/base/shapes.cpp` - Register in `shapeMap` and add a `case` to
   `ShapeInfo::GetReadableShapeName()`
4. `src/base/shapes_*.cpp` - Implement the draw function in the file matching the shape's category (e.g., `shapes_food.cpp`)
5. `src/reporting/reportenumconvert.h` - Add string mappings to `m_iconEnums` and to `ConvertIconToString()`
6. `src/ui/dialogs/editors/insertshapedlg.cpp` - Add to the shape selection dialog
7. `docs/syntax-manual/graphs-properties.qmd` - Add to icon list in documentation
