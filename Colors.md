Color Management
=============================

Creating Colors
=============================

wxWidgets provides a list of standard colors, such as `wxRED` and `wxWHITE`. Wisteria adds hundreds of more
colors that can be accessed by name and returned as compatible `wxColour` objects.
These colors can be retrieved as such:

```cpp
wxColour barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);
```

Refer to the `Wisteria::Colors::Color` enumeration for a list of colors.

Color Schemes
=============================

Most graphs accept color schemes that are used for various purposes. For example, some use them to apply different
colors to groups. A number of pre-defined color schemes are available that can be passed to graphs'
constructor; from there, the graph will handle applying the scheme to the groups. In the following example,
the color scheme `Decade1960s` is used for the groups in a `Wisteria::Graphs::LinePlot`:

```cpp
auto linePlot = std::make_shared<LinePlot>(canvas,
    std::make_shared<Colors::Schemes::Decade1960s>());
```

Likewise, a custom color scheme can also be used:

```cpp
auto linePlot = std::make_shared<LinePlot>(canvas,
    std::make_shared<Colors::Schemes::ColorScheme>
                 (Colors::Schemes::ColorScheme{
                     ColorBrewer::GetColor(Colors::Color::Auburn),
                     ColorBrewer::GetColor(Colors::Color::OctoberMist) }));
```

Refer to `Wisteria::Colors::Schemes` for a list of color schemes.

Contrasting
=============================

Functions are available for tinting (i.e., whitening) and shading (i.e., darkening) colors, as well as intelligent
methods for changing a color depending on its luminance. For example, the following will return a dark gray:

```cpp
auto newColor = ColorContrast::ShadeOrTint(*wxBLACK);
```

The `Wisteria::Colors::ColorContrast` object is also capable of taking a base color and then returning versions of other colors
that contrast well against the base color. This is useful when selecting a font color when overlaying the text
on top of another object.

Color Brewing
=============================

The `Wisteria::Colors::ColorBrewer` class enables you to construct a color scale, which is useful for graphs with a wide range
of values (e.g., `Wisteria::Graphs::HeatMap`). This object will take a start and end color, and when given a series of values,
it will return the respective colors for those values along the color scale.

```cpp
using namespace Wisteria::Colors;

ColorBrewer cb;
cb.SetColorScale(
    {
    // the color for the min value
    *wxBLUE,
    // the color for the max value (because it's the last color added)
    ColorBrewer::GetColor(Color::Red)
    });

std::vector<double> data =
    {
    50,   // max value (will be red)
    1,    // min value (will be blue)
    25.5  // in between value (will be purple)
    };

auto res = cb.BrewColors(data.begin(), data.end());

// an initializer list could also be used:
// auto res = cb.BrewColors({ 50, 1, 25.5 });

// res[0] will be red, res[1] will be blue, and res[2] will be purple
```

Color Management Functions
=============================

All color-related utilities in Wisteria are organized under the `Wisteria::Colors` namespace.
These classes can be mixed and matched when customizing graphs, labels, or other canvas elements.

| Class | Purpose | Example Use |
|--------|----------|-------------|
| `ColorBrewer` | Creates color scales and retrieves named colors | `ColorBrewer::GetColor(Color::CornflowerBlue)` |
| `ColorContrast` | Adjusts colors for visibility and contrast | `ColorContrast::ShadeOrTint(baseColor)` |
| `Schemes::ColorScheme` | Defines a reusable set of colors for groups | `auto scheme = std::make_shared<Schemes::Decade1960s>();` |

Color scheme objects can be shared across multiple graphs to maintain a consistent look throughout a report.
All color objects returned from these functions are fully compatible with `wxColour` and `wxBrush`, making them directly usable
with `wxGraphicsContext` drawing routines.

For a full list of predefined color names and palettes, refer to the `Wisteria::Colors::Color` and `Wisteria::Colors::Schemes`
documentation.
