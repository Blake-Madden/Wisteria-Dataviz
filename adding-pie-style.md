Adding a New Pie Style
=============================

A `PieStyle` applies a visual theme to an entire pie chart (e.g., pizza, coffee ring, cookie).
The following is an overview of the files involved and the changes needed when adding a new style.
The `ChocolateChipCookie` style is used as an example throughout.

Enum Declaration
=============================

In `src/base/enums.h`, add the new value to the `PieStyle` enum:

```cpp
/// @brief A chocolate chip cookie with a slightly irregular edge, toasted spots,
///     chocolate chips, and a few crumbs off to the side.
///     Slice colors will be overridden with cookie colors.
ChocolateChipCookie
```

Header Declarations
=============================

In `src/graphs/piechart.h`, two changes are needed in the private section of the `PieChart` class.

First, add a color getter for the slice fill color:

```cpp
/// @returns A warm golden tan for cookie slice fill.
[[nodiscard]]
static wxColour GetCookieFillColor()
    {
    return wxColour{ 210, 170, 110 };
    }
```

Then declare the drawing helper functions:

```cpp
void AddCookieEdge(const DrawAreas& drawAreas);
void AddCookieToastedSpots(const DrawAreas& drawAreas);
void AddChocolateChips(const DrawAreas& drawAreas);
void AddCookieCrumbs(const DrawAreas& drawAreas);
```

Slice Color Overrides
=============================

In `src/graphs/piechart.cpp`, the slice brush and outline pen need to be overridden for the new style.
This appears in two places: once for the inner pie and once for the outer pie.

Search for the existing `GetPieStyle() == PieStyle::Mars` conditions (there are two) and add the new style after each:

```cpp
else if (GetPieStyle() == PieStyle::ChocolateChipCookie)
    {
    sliceBrush.SetColour(GetCookieFillColor());
    sliceOutlinePen.SetColour(wxColour{ 180, 140, 80 });
    }
```

Style Dispatch
=============================

In the `RecalcSizes()` method of `src/graphs/piechart.cpp`, there is a chain of `if/else if` blocks that call the drawing helpers for each style.
Add the new style at the end of the chain:

```cpp
else if (GetPieStyle() == PieStyle::ChocolateChipCookie)
    {
    AddCookieEdge(drawAreas);
    AddCookieToastedSpots(drawAreas);
    AddChocolateChips(drawAreas);
    AddCookieCrumbs(drawAreas);
    }
```

Style Rendering Implementation
=============================

Implement the drawing functions in `src/graphs/piechart.cpp`. A few conventions to follow:

- Use `ScaleToScreenAndCanvas()` for DPI-aware pixel values.
- Use proportional sizing (based on `drawAreas.m_pieDrawArea` dimensions) so the style scales properly with zoom.
- Use `HashToUnitInterval()` and `RingIrregularity()` for deterministic pseudo-random variation.
- Use `GraphItems::Polygon` for complex shapes, adding them via `AddObject()`.
- Use unique seed constants (e.g., `0xC00C1E`) for reproducible randomness.

For edge effects (like a wobbly cookie edge), adapt the pattern from `AddCrustRing()`:
sample points around the perimeter, apply noise via `RingIrregularity()`, and build a polygon.

For scattered elements (like chocolate chips or crumbs), adapt the pattern from `AddToastedCheeseSpots()` or `AddPepperoni()`:
distribute points using angle steps and radial distance, check for spacing collisions, then draw each element as a polygon.

JSON Report Support
=============================

In `src/base/reportenumconvert.h`, add a string mapping in the `ConvertPieStyle` function's `pieStyleEnums` map so that the style can be used from JSON report files:

```cpp
{ L"chocolate-chip-cookie", PieStyle::ChocolateChipCookie }
```

Documentation
=============================

In `docs/syntax-manual/graphs.qmd`, add the style to the `"pie-style"` property list under the Pie Chart section:

```markdown
  - `"chocolate-chip-cookie"`: A chocolate chip cookie with a slightly wobbly edge,
    toasted spots, chocolate chips, and a few crumbs off to the side.
    Slice colors will be overridden with cookie colors.
```

Summary of Files to Modify
=============================

1. `src/base/enums.h` - Add enum value to `PieStyle`
2. `src/graphs/piechart.h` - Add color getter and drawing function declarations
3. `src/graphs/piechart.cpp` - Add slice color overrides, style dispatch, and drawing implementations
4. `src/base/reportenumconvert.h` - Add string mapping in `ConvertPieStyle`
5. `docs/syntax-manual/graphs.qmd` - Add to pie-style list in documentation
