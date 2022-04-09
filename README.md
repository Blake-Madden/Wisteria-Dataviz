Wisteria Dataviz
=============================

Introduction
=============================

Wisteria Dataviz is data visualization library based on [wxWidgets](https://github.com/wxWidgets/wxWidgets). It includes features
such as:

- Built-in [printing](Printing.md) support
- Built-in copy support
- [Exporting](Exporting.md) (SVG, PNG, JPEG, GIF, BMP, TGA, and TIFF are supported)
- Mouse- and keyboard-enabled zooming
- Data [importing](ImportingData.md) (CSV or tab-delimited files) support
- A dataset interface, which allows for easily connecting data to most graphs
  - Includes support for continuous, categorical, date, and ID columns
- Can handle [HiDPI](HiDPI.md) displays
- Uses high-quality `wxGraphicsContext` for rendering (also uses Direct2D on Microsoft Windows, if available)
- [Image](Images.md) support, including the ability to use images for plot and bar backgrounds
- Effects for boxes and bars, including transparency, a glossy look, stipple brushes, and color fades
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
- Annotations support; story-telling notes can be added to a plot with an arrow pointing to a specific data point
- Text boxes that can be drawn vertically or horizontally
- Selectable objects; for example, selecting a bar on a bar chart can show a custom label
- Locale-sensitive number formatting (based on `wxUILocale`'s settings)
- An extensible framework for designing custom graphs
  - New graph types can be designed from the ground up or be derived from existing graph types (e.g., [BarChart](BarChart.md))
  - Uses an object-based API for positioning points, text boxes, polygons, lines, and images
  - Supports custom axes
  - Draws bounding boxes and extended information in debug mode
  - Profiling macros (e.g., `PROFILE()`) to help with reviewing performance
  
General Workflow
=============================

- [Setup the library's settings](Setup.md) (optionally) when your application starts
- Construct a `Wisteria::Canvas` object (which is a `wxScrolledWindow`-derived window), and embed it into
a `wxFrame` or `wxDialog`
- [Import data](ImportingData.md) into a `Wisteria::Data::Dataset` object (or [build](BuildingData.md) a dataset),
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

The following graph are included:

Basic
-----------------------------

| Bar Chart (Wisteria::Graphs::BarChart) | Stylized Bar Chart (Wisteria::Graphs::BarChart) |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/BarChart.svg) | ![](docs/doxygen/images/BarChartStylized.svg) |
| ![](docs/doxygen/images/BarChartImage.svg) |

| Line Plot (Wisteria::Graphs::LinePlot) | Stylized Line Plot (Wisteria::Graphs::LinePlot) |
| :-------------------------------- | :-------------------------------- |
| ![](docs/doxygen/images/LinePlot.svg) | ![](docs/doxygen/images/LinePlotCustomized.svg) |

| Pie Chart (Wisteria::Graphs::PieChart) | Pie Chart with Subgroups (Wisteria::Graphs::PieChart) |
| :-------------------------------- | :-------------------------------- |
| ![](docs/doxygen/images/PieChart.svg) | ![](docs/doxygen/images/PieChartSubgrouped.svg) |

| Donut Chart (Wisteria::Graphs::PieChart) | Donut Chart with Subgroups (Wisteria::Graphs::PieChart) |
| :-------------------------------- | :-------------------------------- |
| ![](docs/doxygen/images/DonutChart.svg) | ![](docs/doxygen/images/DonutChartSubgrouped.svg) |

Business
-----------------------------

| Gantt Chart (Wisteria::Graphs::GanttChart) |
| :-------------------------------- |
| ![](docs/doxygen/images/GanttChart.svg) |

| Candlestick Plot (Wisteria::Graphs::CandlestickPlot) |
| :-------------------------------- |
| ![](docs/doxygen/images/CandlestickPlot.svg) |

Statistical
-----------------------------

| Histogram (Wisteria::Graphs::Histogram) | Grouped Histogram (Wisteria::Graphs::Histogram) |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/Histogram.svg) | ![](docs/doxygen/images/GroupedHistogram.svg) |

| Box Plot (Wisteria::Graphs::BoxPlot) | Grouped Box Plot (Wisteria::Graphs::BoxPlot) |
| :-------------- | :-------------------------------- |
| ![](docs/doxygen/images/BoxPlot.svg) | ![](docs/doxygen/images/GroupedBoxPlot.svg) |

| Heat Map (Wisteria::Graphs::HeatMap) | Grouped Heat Map (Wisteria::Graphs::HeatMap) |
| :-------------- | :-------------------------------- |
| ![](docs/doxygen/images/Heatmap.svg) | ![](docs/doxygen/images/HeatmapGrouped.svg) |

Survey Data
-----------------------------

| 3-Point Likert Chart (Wisteria::Graphs::LikertChart) | 7-Point Likert Chart (Wisteria::Graphs::LikertChart) |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/Likert3Point.svg) | ![](docs/doxygen/images/Likert7Point.svg) |

Social Sciences
-----------------------------

| W-Curve Plot (Wisteria::Graphs::WCurvePlot) |
| :-------------- |
| ![](docs/doxygen/images/WCurve.svg) |

Multi-plot Layouts
-----------------------------

| Multiple Plots | Multiple Plots with a Common Axis |
| :-------------- | :-------------- |
| ![](docs/doxygen/images/MultiPlot.svg) | ![](docs/doxygen/images/MultiPlotCommonAxis.svg) |

Building
=============================

NOTE: A C++17 compatible compiler is required.

First, you will need to get and build [wxWidgets](https://github.com/wxWidgets/wxWidgets) (3.1.6 or higher is required):

```
git clone https://github.com/wxWidgets/wxWidgets.git --recurse-submodules
```

Refer [here](https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md) for how to build wxWidgets.

Next, get and build %Wisteria:

```
git clone https://github.com/Blake-Madden/Wisteria.git --recurse-submodules
```

To build the library and demo, you can use CMake (either directly or by using an IDE).

To build the API documentation, open "docs/doxygen/Doxyfile" in Doxygen and run it.

Windows
-----------------------------

On Windows, you will need to set `wxWidgets_ROOT_DIR` to the root folder where you have wxWidgets.

Assuming that you had built wxWidgets in "C:/SRC/wxWidgets," you can pass this command line CMake:

```
-DwxWidgets_ROOT_DIR=C:/SRC/wxWidgets
```

Alternatively, if using Visual Studio, add this to your "CMakeSettings.json" file:

```
"cmakeCommandArgs": "-DwxWidgets_ROOT_DIR=C:/SRC/wxWidgets",
```

A note about using Visual Studio is that the output for the library is controlled in the "CMakeSettings.json"
file by these two items: `buildRoot` and `installRoot`. Adding these lines will specify to write the library
and demo into the project's "build" folder:

```
"buildRoot": "${workspaceRoot}\\build\\${name}",
"installRoot": "${workspaceRoot}\\build\\${name}",
```

Linux
-----------------------------

On Linux, you will need to set `wxWidgets_CONFIG_EXECUTABLE` to where wx-config is located.
Assuming that you had built wxWidgets in "/home/myname/wxWidgets/gtk-build," you can pass this command line
option to CMake:

```
-DwxWidgets_CONFIG_EXECUTABLE=/home/myname/wxWidgets/gtk-build/wx-config
```

If using KDevelop, go to `Project -> Open Configuration... -> Cmake` and you can edit this
variable there.

Dependencies
=============================

[wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.1.6
