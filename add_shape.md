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

In `src/base/shapes.cpp`, two changes are needed.

First, register the function in the `shapeMap` inside the `Shape` constructor:

```cpp
{ Icons::IconShape::HawaiianPizza, &ShapeRenderer::DrawHawaiianPizza }
```

Then implement the draw function. A few conventions to follow:

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

In `src/base/reportenumconvert.h`, add string mappings in the `ConvertIcon` function's `iconEnums` map so that the shape can be used from JSON report files:

```cpp
{ L"hawaiian-pizza", Icons::IconShape::HawaiianPizza }
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
3. `src/base/shapes.cpp` - Register in `shapeMap` and implement draw function
4. `src/base/reportenumconvert.h` - Add string mapping in `ConvertIcon`
5. `docs/syntax-manual/graphs-properties.qmd` - Add to icon list in documentation
