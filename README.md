Wisteria Dataviz
=============================

<img src="wisteria-dv.svg" width="200" />

[![cppcheck](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/cppcheck.yml)
[![i18n-check](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/i18n-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/i18n-check.yml)
[![doxygen](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/doxygen.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/doxygen.yml)
[![unix build](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/unix%20build.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/unix%20build.yml)

About
=============================

Wisteria Dataviz is data visualization library based on [wxWidgets](https://github.com/wxWidgets/wxWidgets). It includes features
such as:

- Numerous built-in graphs (refer to the `Wisteria::Graphs` namespace for a full list)
- Interface for displaying tabular data
- Built-in [printing](Printing.md), copying, and exporting support
- Data [importing](ImportingData.md) (Excel, CSV, tab-delimited, or user-defined delimited files) support
- Data exporting (CSV, tab-delimited, or user-defined delimited files) support
- [Image](Images.md) support, including the ability to use images for plot and bar backgrounds, logos, and point markers
- Image effects, such as oil-painting and Sepia tone
- Effects for boxes and bars, including transparency, a watercolor look, a glassy look, stipple brushes, and color fades
- Reference lines and areas
- Pre-defined and extensible color schemes
- Multi-plot support
  - Graphs can be embedded side-by-side on the same (scrollable) canvas
  - Includes support for setting a common axis for all graphs across a row or down a column

See more in the [features overview](Features.md).
  
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

Install the following from your repository manager (or build from source):

- wxWidgets 3.2
- wxGTK3 development files (version 3.2 or higher)
- Threading Building Blocks (libtbb) and its development files

Go into the project folder and run the following to
build the library and demo:

```
cmake ./
make -j4
```

If using KDevelop or VS Code, you can also open the CMake file and build from there.

macOS
-----------------------------

Install the following using brew (or build from source):

- wxWidgets 3.2
- Threading Building Blocks (tbb)
- OpenMP (libomp)

Go into the project folder and run the following to
build the library and demo:

```
cmake ./
make -j4
```

Documentation
=============================

To build the API documentation, open "docs/doxygen/Doxyfile" in Doxygen and run it.

Dependencies
=============================

- [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.2 or higher
- GTK 3 (Linux)
- Threading Building Blocks: libtbb (Linux), tbb (macOS)
- A C++17 compatible compiler
- OpenMP (optional): libomp (macOS)

If using a version of CMake older than 3.24, please refer to
[wxWidgets's CMake overview](https://docs.wxwidgets.org/trunk/overview_cmake.html) for instructions
on how to set up CMake to work with wxWidgets 3.2.
