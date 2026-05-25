Wisteria Dataviz
=============================

<img src="wisteria-dv.svg" width="200" />

| Platforms     | Result        |
| ------------- | ------------- |
| Linux  | [![Linux Build & Unit Tests](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/unix%20build.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/unix%20build.yml) |
| macOS  | [![macOS Build & Unit Tests](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/macos-build.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/macos-build.yml) |
| Windows  | [![Windows Build & Unit Tests](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/msbuild.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/msbuild.yml ) |

| Code Analyses | Result        |
| ------------- | ------------- |
| cppcheck | [![cppcheck](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/cppcheck.yml) |
| clang-tidy | [![clang-tidy](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/clang-tidy.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/clang-tidy.yml) |
| MS PREfast | [![Microsoft C++ Code Analysis](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/msvc.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/msvc.yml) |
| Quneiform | [![Quneiform](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/i18n-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/i18n-check.yml) |

| Documentation & Formatting | Result        |
| --------------------  | ------------- |
| Doxygen | [![doxygen](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/doxygen.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/doxygen.yml) |
| Spellcheck | [![Spell Check](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/spell-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/spell-check.yml) |
| Profanity Check | [![Profanity Check](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/profanity-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/profanity-check.yml) |
| clang-format | [![clang-format](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/clang-format-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/clang-format-check.yml) |
| UNIX Newlines | [![Check LF line endings](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/newlines-check.yml/badge.svg)](https://github.com/Blake-Madden/Wisteria-Dataviz/actions/workflows/newlines-check.yml) |

About
=============================

Wisteria Dataviz is a data visualization toolkit based on [wxWidgets](https://github.com/wxWidgets/wxWidgets), with a focus on statistics and social sciences.
It ships in two forms:

- A stable C++ **library** that can be embedded into any wxWidgets application
- A stand-alone **application** (currently in alpha release) for building graphical reports interactively, with no coding required

Library features include:

- Numerous built-in graphs (refer to the `Wisteria::Graphs` namespace for a full list)
- Interface for displaying tabular data
- Built-in [printing](docs/printing.md), copying, and exporting support
- Data [importing](docs/importing-data.md) (Excel, OpenDocument Spreadsheet, CSV, tab-delimited, or user-defined delimited files) support
- Data [transformations](docs/transforming-data.md), such as filtering, pivoting (both longer or wider), subsetting, and recoding
- Data exporting (CSV, tab-delimited, or user-defined delimited files) support
- [Image](docs/images.md) support, including the ability to use images for plot and bar backgrounds, logos, and point markers
- Image effects, such as oil-painting and Sepia tone
- Effects for boxes and bars, including transparency, a watercolor look, a glassy look, stipple brushes, and color fades
- Reference lines and areas
- Pre-defined and extensible color schemes
- Multi-plot support
  - Graphs can be embedded side-by-side on the same (scrollable) canvas
  - Includes support for setting a common axis for all graphs across a row or down a column

See more in the [features overview](docs/features.md).

Stand-Alone Application (Alpha)
=============================

In addition to the library, Wisteria Dataviz is bundled as a stand-alone desktop application (built as the `WisteriaDV` target). The application exposes the library's full graphing and data-handling capabilities through a ribbon-based interface, with no programming required.

> **Note:** The application is currently an **alpha release**. Project file formats, dialogs, and behaviors may change between versions.

Report authoring
-----------------------------

- **Page layout manager** for assembling multi-page graphical reports — each project can contain any number of pages, with insert/edit/delete/reorder support
- Each page is a `Wisteria::Canvas` that can hold multiple graphs, tables, labels, images, shapes, and common axes side-by-side or in a grid layout
- **PDF export** of the entire report (single- or multi-page), preserving vector graphics and using embedded fonts
- **SVG export** of individual canvases for high-fidelity scalable output
- Raster image export (PNG, JPEG, BMP, TIFF, etc.) with configurable size, DPI, and background
- Full **print and print preview** support, with customizable headers/footers and page setup
- Copy graphs and tables to the clipboard for pasting into other applications
- Project save/save-as to a self-contained project file (graphs, datasets, settings, and embedded resources)

Graph and object insertion
-----------------------------

- Insertion dialogs for every graph type in the library — basic, business, statistical, survey, education, social-sciences, and sports categories
- Add **multi-series line plots** that combine several series onto a single chart
- Per-item editing dialogs for customizing colors, axes, titles, captions, accessibility attributes, and more

Data management
-----------------------------

- Manage **multiple datasets** per project, each independently editable and reusable across graphs
- **Import** from Excel (`.xlsx`), OpenDocument Spreadsheet (`.ods`), CSV, tab-delimited, and user-defined delimited text files, with a preview dialog for spreadsheet sources
- Built-in dataset **viewer/editor** for inspecting rows and columns before plotting
- Interactive **data transformations**:
  - Subset (filter rows by criteria)
  - Pivot longer / pivot wider (reshape between long and wide formats)
  - Join (left and inner joins against another dataset)
  - Recode and define named **constants** for reuse across the project
- A log/warning report window that surfaces import issues, data quality warnings, and runtime messages

Library Workflow
=============================

- Setup the [library's settings](docs/setup.md) (optionally) when your application starts
- Construct a `Wisteria::Canvas` object (which is a `wxScrolledWindow`-derived window), and embed it into
a `wxFrame` or `wxDialog`
- [Import data](docs/importing-data.md) into a `Wisteria::Data::Dataset` (or [build](docs/building-data.md) a dataset),
specifying which columns to include and how to classify them
- Construct a plot object (e.g., `Wisteria::Graphs::LinePlot`) and pass your dataset to it
- Customize the plot, as needed
  - Change the colors and styles of the bars, lines, etc.
  - Change the plot background color, or use an image as the background
  - Add titles and captions
  - Adjust the [axes](docs/axes.md), add tick marks, add custom [labels](docs/labels.md)
  - etc.
- Add the plot (and its legend [if applicable]) to the canvas

Graph Types
=============================

Some of the graphs included are:

Basic
-----------------------------

| Bar Chart (Wisteria::Graphs::BarChart) | Line Plot (Wisteria::Graphs::LinePlot) |
| :-------------- | :-------------- |
| ![](docs/images/BarChart.svg) | ![](docs/images/LinePlot.svg) |

| Pie Chart (Wisteria::Graphs::PieChart) | Donut Chart (Wisteria::Graphs::PieChart) |
| :-------------------------------- | :-------------------------------- |
| ![](docs/images/PieChart.svg) | ![](docs/images/DonutChart.svg) |

| Styled Pie Charts | |
| :-------------- | :-------------------------------- |
| ![](docs/images/clock.svg) | ![](docs/images/pizza.svg) |
| ![](docs/images/coffee.svg) |  |

| Table (Wisteria::Graphs::Table) |
| :-------------- |
| ![](docs/images/TableMajors.svg) |

| Sankey Diagram (Wisteria::Graphs::SankeyDiagram) |
| :-------------- |
| ![](docs/images/grouped-sankey.png) |

| Waffle Chart (Wisteria::Graphs::WaffleChart) |
| :-------------- |
| ![](docs/images/WaffleChart.png) |

| Word Cloud (Wisteria::Graphs::WordCloud) |
| :-------------- |
| ![](docs/images/wordcloud.png) |

Business
-----------------------------

| Gantt Chart (Wisteria::Graphs::GanttChart) |
| :-------------------------------- |
| ![](docs/images/GanttChart.png) |

| Candlestick Plot (Wisteria::Graphs::CandlestickPlot) |
| :-------------------------------- |
| ![](docs/images/CandlestickPlot.svg) |

Statistical
-----------------------------

| Histogram (Wisteria::Graphs::Histogram) | Box Plot (Wisteria::Graphs::BoxPlot) |
| :-------------- | :-------------- |
| ![](docs/images/Histogram.svg) | ![](docs/images/BoxPlot.svg) |

| Discrete Heat Map (Wisteria::Graphs::HeatMap) | Grouped Discrete Heat Map (Wisteria::Graphs::HeatMap) |
| :-------------- | :-------------------------------- |
| ![](docs/images/Heatmap.svg) | ![](docs/images/HeatmapGrouped.svg) |

| Scatter Plot (Wisteria::Graphs::ScatterPlot) | Bubble Plot (Wisteria::Graphs::BubblePlot) |
| :-------------- | :-------------------------------- |
| ![](docs/images/scatterplot.svg) | ![](docs/images/bubbleplot.svg) |

| Chernoff Faces (Female) (Wisteria::Graphs::ChernoffFacesPlot) | Chernoff Faces (Male) (Wisteria::Graphs::ChernoffFacesPlot) |
| :-------------- | :-------------------------------- |
| ![](docs/images/chernoff-female.svg) | ![](docs/images/chernoff-male.svg) |

Fully customizable with options for skin tone, hair color, and hair style!

| Stem & Leaf Plot (Wisteria::Graphs::StemAndLeafPlot) | Stem & Leaf Plot (Back-to-back) (Wisteria::Graphs::StemAndLeafPlot) |
| :-------------- | :-------------------------------- |
| ![](docs/images/stem-leaf.svg) | ![](docs/images/stem-leaf-grouped.svg) |

Survey Data
-----------------------------

| 3-Point Likert Chart (Wisteria::Graphs::LikertChart) |
| :-------------- |
| ![](docs/images/Likert7Point.png) |

| Pro & Con Roadmap (Wisteria::Graphs::ProConRoadmap) |
| :-------------- |
| ![](docs/images/SWOTRoadmap.svg) |

Social Sciences
-----------------------------

| W-Curve Plot (Wisteria::Graphs::WCurvePlot) |
| :-------------- |
| ![](docs/images/WCurve.svg) |

| Linear Regression Roadmap (Wisteria::Graphs::LRRoadmap) |
| :-------------- |
| ![](docs/images/LRRoadmapFirstYear.svg) |

Sports
-----------------------------

| Win/Loss Sparkline (Wisteria::Graphs::WinLossSparkline) |
| :-------------- |
| ![](docs/images/WinLossSparkline.svg) |

See more in the [graphs gallery](docs/gallery.md).

Release Notes
=============================
Release information is available [here](release.md).

Building
=============================

Windows
-----------------------------

Install the following:

- *Visual Studio*
- *Doxygen* (if wanting the API documentation)

Download [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.3.3 or higher:

- Open *Visual Studio* and select *Clone a Repository*
  - Enter [https://github.com/wxWidgets/wxWidgets.git](https://github.com/wxWidgets/wxWidgets.git) and clone it

Next, download and build *Wisteria*:

- Open *Visual Studio* and select *Clone a Repository*
  - Enter [https://github.com/Blake-Madden/Wisteria-Dataviz.git](https://github.com/Blake-Madden/Wisteria-Dataviz.git) and clone it.
    Note that this project's folder should be at the same level as the *wxWidgets* folder.
- Open this project's *CMake* file in *Visual Studio*:
  - Open **Project** > **CMake Settings for Wisteria**
    - Set the configuration type to "Release" (or create a new release configuration)
    - Save your changes
- Select **View** > **CMake Targets**
- Build the *demo*, and *wisteria*, and/or *doxygen-docs* targets

Linux
-----------------------------

Install the following from your repository manager (or build from source):

- *GTK3* development files (version 3.3 or higher)
- *WebKitGTK* development files (*libwebkit2gtk-4.1-dev* or *webkit2gtk3-devel*)
- *OpenMP* (*libomp*) and its development files
- *Threading Building Blocks* (*libtbb*) and its development files
- *Doxygen* (if wanting the API documentation)

Download [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.3.3 or higher at the
same folder level as this project:

```
git clone https://github.com/wxWidgets/wxWidgets.git --recurse-submodules
```

Next, download *Wisteria* and build the library, demo, and documentation:

```
git clone https://github.com/Blake-Madden/Wisteria-Dataviz.git --recurse-submodules
cd Wisteria-Dataviz
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build -j4 --config Release
```

If using *CLion*, *KDevelop* or *VS Code*, you can also open the *CMake* file and build from there.

macOS
-----------------------------

Install the following:

- *XCode* (will include the *AppleClang* compiler)
- *Doxygen* (if wanting the API documentation)

Download [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.3.3 or higher at the
same folder level as this project:

```
git clone https://github.com/wxWidgets/wxWidgets.git --recurse-submodules
```

Next, download *Wisteria* and build the library, demo, and documentation:

```
git clone https://github.com/Blake-Madden/Wisteria-Dataviz.git --recurse-submodules
cd Wisteria-Dataviz
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build -j4 --config Release
```

Documentation
=============================

To build the API documentation, open "docs/doxygen/Doxyfile" in Doxygen and run it.

Dependencies
=============================

- [wxWidgets](https://github.com/wxWidgets/wxWidgets) 3.3.3 or higher
- A C++20 compatible compiler (*AppleClang* on macOS)
- *CMake* 3.25 or higher
- *Doxygen* (if wanting the API documentation)
- *GTK 3* (Linux)
- *Threading Building Blocks*: *libtbb* (Linux)
- *OpenMP* (Linux)
