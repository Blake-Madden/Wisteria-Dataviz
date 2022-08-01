Wisteria Dataviz 0.9.1 Release
=============================

- WARNING: icon classes and schemes have been moved into @c Wisteria::Icons and @c Wisteria::Icons::Schemes
  namespaces, respectively.
- WARNING: the common axis functions have been renamed to @c CommonAxisBuilder::BuildYAxis and
  @c CommonAxisBuilder::BuildXAxis
- Added Linear Regression Roadmap
- Added Pros & Cons Roadmap
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

Wisteria Dataviz 0.9 Release
=============================

Initial public release.