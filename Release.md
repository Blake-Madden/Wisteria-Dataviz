# Releases

## 1.0.2 Release

- New graph type: Multi-series Line Plot
  (this allows for plotting multiple data series on a line plot)
- Added sword and Immaculate Heart with sword shapes
- Added Chapel Blue as predefined color
- Likert Charts
  - Now uses guillemets instead of Unicode arrows
    (this fixes display issues where some fonts don't support those characters)
  - Arrow icons are now customizable for Likert Charts
  - Fixed bar labels erroneously being shown
- Added showcasing option to Lix Gauge
- Fixed axis brackets in Lix Gauge to be a consistent color
- Added showcasing option to German Lix Gauge
- Improved auto-contrasting graphs with black backgrounds
- All shapes now support non-opaque brushes
- Added currency formatting support to Unix platforms
- Added `Tint()` function
- Optimizations for bar sorting (bar charts)

## 1.0.1 Release

**NOTE**: wxWidgets 3.3.1 is required now

- New graph type: Scale Chart
- New graph type: Inflesz Chart
- Added WebP image support
- DPI resolution for saving images is now customizable
- Added support for adding PNG description when exporting images
- Added showcasing to DB2 plot
- Expanded JSON syntax
- Shapes:
  - Added dollar bill and monitor shapes
  - Force car shape to always be dark blue
  - Fixed appearance of some shapes for horizontal bars
  - Improve appearance of some shapes
- Bar charts:
  - Apply opacity to showcased bars' stipple shapes
  - Added option to bar chart showcasing to keep the labels on ghosted bars
  - Bar chart bin labels will now use the precision that the parallel axis uses
  - Fixed sorting bars with a set of defined labels
  - Fixed decals in vertical bar charts
  - Fixed bar chart blocks getting rounded down to integers
  - Improve bar labels to not overlap so often
  - Improve making space for larger bin labels
  - Don't return suffix label for bin labels if not displayed
  - Vertically center bar block decals in vertical bar charts
  - Allow finding bar blocks when scale is reversed
  - Added reverse arrow bar shape (bar charts)
- Fixed W-Curve Plot axis labels not being readable
- Fixed `AdjustRangeToLabels()` to work with reversed scale
- Don't automatically add margins to common axes
- Ghost middle labels on pie slice if slice is ghosted
- Added ghosting support to:
  - Bar blocks (in bar charts)
  - Axis points
  - Axis brackets
- Added showcasing to:
  - Line plots
  - Axis points
  - Axis brackets
- Use reverse arrow on Gantt chart if axis is reversed
- Added currency formatting support to Axes (Windows only)
- Added `Wisteria::GetLibraryVersionInfo()`
- Fixed doxygen not being included in `ALL` *CMake* build

## 1.0 Release

- Added JSON syntax for building multi-page reports
- Added numerous vector-based shapes to use as stipples and icons
- Various improvements

## 0.9.1 Release

**WARNING**: icon classes and schemes have been moved into @c Wisteria::Icons and @c Wisteria::Icons::Schemes
  namespaces, respectively

**WARNING**: the common axis functions have been renamed to @c Wisteria::CommonAxisBuilder::BuildYAxis and
  @c Wisteria::CommonAxisBuilder::BuildXAxis

- New graph type: Linear Regression Roadmap
- New graph type: Pros \& Cons Roadmap
- New graph type: Table (i.e., presentation of tabular data, with aggregation and annotation support)
- Added ability to stretch content to fit the entire page when printing
- Simplified layout of canvas rows when they are fitting their content (see @c Wisteria::Canvas::CalcRowDimensions())
- Added common axis builder helper class
- Line plots can now use categorical or continuous columns for X axis values
- Added support for aligning pie chart labels flush left or right against the plotting area
- Fixed pie charts being drawn outside of their plot area under certain circumstances
- Setting background color of canvas titles now stretches them across the canvas,
  resulting in a banner appearance
- Improved layout of legends appearing above or below a graph

## 0.9 Release

Initial public release.
