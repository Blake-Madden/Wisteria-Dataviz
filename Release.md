# Releases

## 1.0.2 Release

- New graph type: Multi-series Line Plot\
  This allows for plotting multiple data series on a line plot
- Add sword and Immaculate Heart with sword shapes
- Add Chapel Blue as predefined color
- Likert Charts
  - Now uses guillemets instead of Unicode arrows\
    This fixes display issues where some fonts don't support those characters
  - Arrow icons are now customizable for Likert Charts
  - Fix bar labels erroneously being shown
- Add showcasing option to Lix Gauge
- Fix axis brackets in Lix Gauge to be a consistent color
- Add showcasing option to German Lix Gauge
- Improve auto-contrasting graphs with black backgrounds
- All shapes now support non-opaque brushes
- Add currency formatting support to Unix platforms
- Add `Tint()` function
- Optimizations for bar sorting (bar charts)

## 1.0.1 Release

**NOTE**: wxWidgets 3.3.1 is required now

- New graph type: Scale Chart
- New graph type: Inflesz Chart
- Add WebP image support
- DPI resolution for saving images is now customizable
- Add support for adding PNG description when exporting images
- Add showcasing to DB2 plot
- Expanded JSON syntax
- Shapes:
  - Add dollar bill and monitor shapes
  - Force car shape to always be dark blue
  - Fix appearance of some shapes for horizontal bars
  - Improve appearance of some shapes
- Bar charts:
  - Apply opacity to showcased bars' stipple shapes
  - Add option to bar chart showcasing to keep the labels on ghosted bars
  - Bar chart bin labels will now use the precision that the parallel axis uses
  - Fix sorting bars with a set of defined labels
  - Fix decals in vertical bar charts
  - Fix bar chart blocks getting rounded down to integers
  - Improve bar labels to not overlap so often
  - Improve making space for larger bin labels
  - Don't return suffix label for bin labels if not displayed
  - Vertically center bar block decals in vertical bar charts
  - Allow finding bar blocks when scale is reversed
  - Add reverse arrow bar shape (bar charts)
- Fix W-Curve Plot axis labels not being readable
- Fix `AdjustRangeToLabels()` to work with reversed scale
- Don't automatically add margins to common axes
- Ghost middle labels on pie slice if slice is ghosted
- Add ghosting support to:
  - Bar blocks (in bar charts)
  - Axis points
  - Axis brackets
- Add showcasing to:
  - Line plots
  - Axis points
  - Axis brackets
- Use reverse arrow on Gantt chart if axis is reversed
- Add currency formatting support to Axes (Windows only)
- Add `Wisteria::GetLibraryVersionInfo()`
- Fix doxygen not being included in `ALL` *CMake* build

## 1.0 Release

- Added JSON syntax for building multi-page reports
- Added numerous vector-based shapes to use as stipples and icons
- Various improvements

## 0.9.1 Release

- WARNING: icon classes and schemes have been moved into @c Wisteria::Icons and @c Wisteria::Icons::Schemes
  namespaces, respectively.
- WARNING: the common axis functions have been renamed to @c CommonAxisBuilder::BuildYAxis and
  @c CommonAxisBuilder::BuildXAxis
- Added Linear Regression Roadmap
- Added Pros \& Cons Roadmap
- Added Table (i.e., presentation of tabular data, with aggregation and annotation support)
- Added ability to stretch content to fit the entire page when printing
- Simplified layout of canvas rows when they are fitting their content (see Canvas::CalcRowDimensions())
- Added common axis builder helper class
- Line plots can now use categorical or continuous columns for X axis values
- Added support for aligning pie chart labels flush left or right against the plotting area
- Fixed pie charts being drawn outside of their plot area under certain circumstances
- Setting background color of canvas titles now stretches them across the canvas,
  resulting in a banner appearance
- Improved layout of legends appearing above or below a graph

## 0.9 Release

Initial public release.
