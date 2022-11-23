Wisteria Dataviz
=============================

<img src="wisteria.svg" width="200" />

About
=============================

Wisteria Dataviz is data visualization library based on [wxWidgets](https://github.com/wxWidgets/wxWidgets). It includes features
such as:

- Numerous built-in graphs (refer to the `Wisteria::Graphs` namespace for a full list)
- Interface for displaying tabular data, including support for aggregate columns/rows, cell highlighting,
  outlier detection, and annotations
- Built-in [printing](Printing.md) support
- Built-in copy support
- [Exporting](Exporting.md) (SVG, PNG, JPEG, GIF, BMP, TGA, and TIFF are supported)
- Mouse- and keyboard-enabled zooming
- Data [importing](ImportingData.md) (Excel, CSV, tab-delimited, or user-defined delimited files) support
- Data exporting (CSV, tab-delimited, or user-defined delimited files) support
- A dataset interface, which allows for easily connecting data to most graphs
  - Includes support for continuous, categorical, date, and ID columns
  - Graphs are designed to handle missing data
- Uses high-quality `wxGraphicsContext` for rendering (also uses Direct2D on Windows, if available)
- [Image](Images.md) support, including the ability to use images for plot and bar backgrounds, logos, and point markers
- Effects for boxes and bars, including transparency, a glassy look, stipple brushes, and color fades
- HiDPI display support (Windows)
- Customizable point markers and line styles
  - Pre-defined and extensible shape schemes
- Customizable [axes](Axes.md)
  - Ranges can be adjusted
  - Supports date ranges
  - Supports logarithmic scales
  - Labels can be stacked, hidden, replaced with custom text, etc.
  - Header and footers can be added
  - Tickmarks can be customized
  - Axis lines can be drawn as a regular line or an arrow
- Custom axes can be added onto the plot area (inside of the main axes)
- Reference lines
- Reference areas (e.g., recession areas on a financial chart)
- Legends with shape, image, and color scale support
- Extensive list of pre-built colors that can be referenced via an enumeration (`Wisteria::Colors::Color`)
- A [color brewing](Colors.md) interface, which is helpful for:
  - Building color scales
  - Shading or tinting colors
  - Selecting black or white (e.g., for a font color), depending on which better contrasts against the background color
- Pre-defined and extensible color schemes
- Multi-plot support
  - Graphs can be embedded side-by-side on the same (scrollable) canvas
  - Includes support for setting a common axis for all graphs across a row or down a column
  - The size proportions of the rows and columns within the canvas can be customized
- Annotations support; story-telling notes can be added to a plot with an arrow pointing to specific data points
- Text boxes that can be drawn vertically or horizontally
- Selectable objects; for example, selecting a bar on a bar chart can show a custom label
- i18n support
  - Fully Unicode
  - Locale-sensitive number formatting (based on `wxUILocale`'s settings)
  - All UI-facing text is available for translation (via the `gettext` library)
- An extensible framework for designing custom graphs
  - New graph types can be designed from the ground up or be derived from existing graph types (e.g., [BarChart](BarChart.md))
  - Uses an object-based API for positioning points, text boxes, polygons, lines, and images
  - Supports custom axes
  - Draw bounding boxes and extended information in debug mode
  - Profiling macros (e.g., `PROFILE()`) to help with reviewing performance
  - i18n functions to help prevent accidental translations (e.g., `DONTTRANSLATE()` and `_DT()`)
  - `LogFile` class for routing logging information to a formatted file
  
General Workflow
=============================

- Setup the [library's settings](Setup.md) (optionally) when your application starts
- Construct a `Wisteria::Canvas` object (which is a `wxScrolledWindow`-derived window), and embed it into
a `wxFrame` or `wxDialog`
- [Import data](ImportingData.md) into a `Wisteria::Data::Dataset` (or [build](BuildingData.md) a dataset),
specifying which columns to include and how to classify them
- Construct a plot object (e.g., `Wisteria::Graphs::LinePlot`) and pass your dataset to it
- Customize the plot, as needed
  - Change the colors and styles of the bars, lines, etc.
  - Change the plot background color, or use an image as the background
  - Add titles and captions
  - Adjust the [axes](Axes.md), add tickmarks, add custom [labels](Labels.md)
  - etc.
- Add the plot (and its legend [if applicable]) to the canvas

Graph Types
=============================

Some of the graphs included are:

Basic
-----------------------------

| Bar Chart (Wisteria::Graphs::BarChart) | Line Plot (Wisteria::Graphs::LinePlot) |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/BarChart.svg) | ![](docs/doxygen/images/LinePlot.svg) |

| Pie Chart (Wisteria::Graphs::PieChart) | Donut Chart (Wisteria::Graphs::PieChart) |
| :-------------------------------- | :-------------------------------- |
| ![](docs/doxygen/images/PieChart.svg) | ![](docs/doxygen/images/DonutChart.svg) |

| Table (Wisteria::Graphs::Table) |
| :-------------- |
| ![](docs/doxygen/images/TableMajors.svg) |

Business
-----------------------------

| Gantt Chart (Wisteria::Graphs::GanttChart) |
| :-------------------------------- |
| ![](docs/doxygen/images/GanttChart.png) |

| Candlestick Plot (Wisteria::Graphs::CandlestickPlot) |
| :-------------------------------- |
| ![](docs/doxygen/images/CandlestickPlot.svg) |

Statistical
-----------------------------

| Histogram (Wisteria::Graphs::Histogram) | Box Plot (Wisteria::Graphs::BoxPlot) |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/Histogram.svg) | ![](docs/doxygen/images/BoxPlot.svg) |

| Discrete Heat Map (Wisteria::Graphs::HeatMap) | Grouped Discrete Heat Map (Wisteria::Graphs::HeatMap) |
| :-------------- | :-------------------------------- |
| ![](docs/doxygen/images/Heatmap.svg) | ![](docs/doxygen/images/HeatmapGrouped.svg) |

Survey Data
-----------------------------

| 3-Point Likert Chart (Wisteria::Graphs::LikertChart) |
| :-------------- |
| ![](docs/doxygen/images/Likert7Point.png) |

| Pro & Con Roadmap (Wisteria::Graphs::ProConRoadmap) |
| :-------------- |
| ![](docs/doxygen/images/SWOTRoadmap.svg) |

Social Sciences
-----------------------------

| W-Curve Plot (Wisteria::Graphs::WCurvePlot) |
| :-------------- |
| ![](docs/doxygen/images/WCurve.svg) |

| Linear Regression Roadmap (Wisteria::Graphs::LRRoadmap) |
| :-------------- |
| ![](docs/doxygen/images/LRRoadmapFirstYear.svg) |

See more in the [graphs gallery](Gallery.md).

Release Notes
=============================
Release information is available [here](Release.md).

Building
=============================

First, download Wisteria:

```
git clone https://github.com/Blake-Madden/Wisteria-Dataviz.git --recurse-submodules
```

Windows
-----------------------------

Get and build [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.2 or higher:

```
git clone https://github.com/wxWidgets/wxWidgets.git --recurse-submodules
```

Refer [here](https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md) for how to build wxWidgets.

Next, build Wisteria:

If using CMake GUI, open "CMakeLists.txt" and set `wxWidgets_ROOT_DIR` to the
root folder of wxWidgets. Next, configure and generate a project file for your compiler.

If using Visual Studio, open the Wisteria folder to load the project.
Then go to `Project -> CMake Settings for Wisteria...`. Add an entry for `wxWidgets_ROOT_DIR`
and specify the path to your wxWidgets folder. Save and then build the project.

Linux
-----------------------------

Install the following from your repository manager:

- wxWidgets 3.2
- wxGTK3 development files (version 3.2 or higher)
- Threading Building Blocks (libtbb) and its development files

Open "CMakeLists.txt" in CMake GUI, configure, and then generate a make script. Finally, go into
the build folder that you specified and run `make` to build the library and demo.

If using KDevelop, you can also open the CMake file and build from there.

Documentation
=============================

To build the API documentation, open "docs/doxygen/Doxyfile" in Doxygen and run it.

Dependencies
=============================

- [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.2 or higher
- GTK 3 (Linux)
- libtbb (Linux)
- A C++17 compatible compiler

If using a version of CMake older than 3.24, please refer to
[wxWidgets's CMake overview](https://docs.wxwidgets.org/trunk/overview_cmake.html) for instructions
on how to set up CMake to work with wxWidgets 3.2.