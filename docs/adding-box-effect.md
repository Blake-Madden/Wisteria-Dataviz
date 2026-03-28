Adding a New Box Effect
=============================

A `BoxEffect` controls how bars and boxes are rendered (e.g., solid, glassy, watercolor).
The following is an overview of the files involved and the changes needed when adding a new effect.
The `Marker` effect is used as an example throughout.

Enum & Type Declarations
=============================

The effect needs entries in three enums across the framework.

In `src/base/enums.h`, add the new value to the `BoxEffect` enum using the next available integer,
and increment `EFFECTS_COUNT`:

```cpp
/// @brief A felt-marker-like effect, where the outline is rough and slightly uneven
///     and the interior is filled with diagonal hatching lines.
Marker = 10,
/// @private
EFFECTS_COUNT = 11
```

In `src/base/icons.h`, add a corresponding `IconShape` entry near the other rectangle shapes
(`WaterColorRectangle`, `ThickWaterColorRectangle`):

```cpp
MarkerRectangle,  /*!< A rectangle drawn with a marker-like effect, with rough
                       outline and diagonal hatching. */
```

In `src/base/polygon.h`, add a matching `PolygonShape` entry to `Polygon::PolygonShape`:

```cpp
/// @brief A marker-like rectangle, with rough outline and diagonal hatching.
MarkerRectangle,
```

Shape Rendering
=============================

The actual drawing is handled by `ShapeRenderer` in `src/base/shapes.h` and `src/base/shapes.cpp`.

First, declare the draw function in the `ShapeRenderer` class in `src/base/shapes.h`:

```cpp
void DrawMarkerRectangle(wxRect rect, wxDC& dc) const;
```

Then in `src/base/shapes.cpp`, register the function in the `shapeMap` inside the `ShapeRenderer` constructor:

```cpp
{ Icons::IconShape::MarkerRectangle, &ShapeRenderer::DrawMarkerRectangle },
```

The implementation itself goes in the same file. A few conventions to follow:

- Use `GraphicsContextFallback` to acquire a `wxGraphicsContext`
  (refer to `DrawWaterColorRectangle` for the pattern).
- Use `GetGraphItemInfo().GetBrush()` for the fill color.
- Use `ScaleToScreenAndCanvas()` for DPI-aware pixel values.
- Use proportional sizing (based on `rect` dimensions) so the effect scales properly with zoom.
- Use `m_mt` (the class's `mutable std::mt19937`) for randomness if needed.

Polygon Dispatch
=============================

The `Polygon` class in `src/base/polygon.cpp` delegates rendering to the `Shape`/`ShapeRenderer` classes
for special effects. Three changes are needed here.

Add a brush assertion alongside the existing assertions:

```cpp
wxASSERT_MSG(!(GetShape() == PolygonShape::MarkerRectangle && !GetBrush().IsOk()),
             L"Brush must be set when using marker-filled rectangle!");
```

Exclude the new shape from the gradient background fill path (since the effect handles its own fill):

```cpp
if (GetBackgroundFill().IsOk() &&
    (GetShape() != PolygonShape::WaterColorRectangle) &&
    (GetShape() != PolygonShape::ThickWaterColorRectangle) &&
    (GetShape() != PolygonShape::MarkerRectangle))
```

Add an `else if` branch in the shape drawing section that delegates to the `Shape` renderer:

```cpp
else if (GetShape() == PolygonShape::MarkerRectangle)
    {
    const GraphItems::Shape sh(GetGraphItemInfo(), Icons::IconShape::MarkerRectangle,
                               boundingBox.GetSize());
    sh.Draw(boundingBox, dc);
    }
```

Box Plot Integration
=============================

In `src/graphs/boxplot.cpp`, the `RecalcSizes()` method contains a `drawBox` lambda with a
`SetShape()` ternary chain that maps `BoxEffect` values to `PolygonShape` values. Add the new
mapping alongside the existing watercolor entries:

```cpp
(box.GetBoxEffect() == BoxEffect::Marker) ?
    GraphItems::Polygon::PolygonShape::MarkerRectangle :
```

Bar Chart Integration
=============================

In `src/graphs/barchart.cpp`, there are two nearly identical bar-drawing sections (one for each
orientation). Both need the same changes. They can be found by searching for `SetShape` in the file;
the first uses `Outline(true, false, true, false)` and the second uses `Outline(false, true, false, true)`.

In each section, add the new effect to the `SetShape()` ternary chain:

```cpp
(bar.GetEffect() == BoxEffect::Marker) ?
    GraphItems::Polygon::PolygonShape::MarkerRectangle :
```

Effects that draw outside their bounds (like watercolor and marker) should not be clipped and should
use a hard outline. Add the new effect to the existing condition:

```cpp
if (bar.GetEffect() == BoxEffect::WaterColor ||
    bar.GetEffect() == BoxEffect::ThickWaterColor ||
    bar.GetEffect() == BoxEffect::Marker)
```

Inside that same block, set the default legend shape:

```cpp
SetDefaultLegendShape(
    (bar.GetEffect() == BoxEffect::Marker) ?
        Icons::IconShape::MarkerRectangle :
        Icons::IconShape::WaterColorRectangle);
```

JSON Report Support
=============================

In `src/base/reportenumconvert.h`, add string mappings so that the effect can be used from
JSON report files.

In the `ConvertIconShape` function's map:

```cpp
{ L"marker-rectangle", Icons::IconShape::MarkerRectangle },
```

In the `ConvertBoxEffect` function's map:

```cpp
{ L"marker", BoxEffect::Marker },
```

Note that `src/base/reportbuilder.cpp` does not need changes. It reads the `"box-effect"` JSON
property and passes it through `ReportEnumConvert::ConvertBoxEffect()`, so new effects are
picked up automatically from the string map.

Documentation
=============================

The syntax manual in `docs/syntax-manual` has three files that list available effects:

- `graphs-properties.qmd`: add the icon shape string (e.g., `"marker-rectangle"`) to the shape list.
- `graphs-base-level.qmd`: add the effect name (e.g., `"marker"`) to the bar chart `box-effect` options.
- `graphs.qmd`: add the effect name to the box plot `box-effect` options.
