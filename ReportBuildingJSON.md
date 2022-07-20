Report Building from a Configuration File
=============================
[TOC]

A report can be loaded directly from a JSON configuration file using the `ReportBuilder` class.
These reports can consist of a single page (i.e., a canvas), or a series of multiple pages.
On each page, items such as tables, plots, notes, etc. are embedded, and the configuration file
specifies how these items are laid out.

Along with the objects on the pages, a configuration file can also define the datasources and
user-defined values used by report's objects, printer settings, etc.

The following details the available options for JSON configuration files.

# Root-level Items
- @c "name": contains a string value, representing the name of the report.

## Datasources {#datasources-properties}
Properties for the @c "datasources" node:
- @c "datasources": contains an array of datasources, which are referenced by other items in the report.
  - @c "name": the name of the datasource.\n
  This name is referenced by items (e.g., plots) elsewhere in the configuration file and must be unique.
  - @c "path": the full file path of the datasource.
  - @c "importer": how to import the datasource.\n
    This is optional and the default is to import the file based on its file extension.\n
    Available options are:
    - @c "tsv": a tab-delimited text file.
    - @c "csv": a comma-delimited text file.
  - @c "id-column": the ID column.\n
    This property is optional.
  - @c "continuous-columns": an array of continuous column names to import.\n
    This property is optional.
  - @c "categorical-columns": an array of categorical column specifications.\n
    This property is optional.\n
  Each specification consists of the following:
    - @c "name": the name of the column.
    - @c "parser": how to read the column.\n
      Available options are:
      - @c "as-integers": imports the column as discrete integers.
      - @c "as-strings" imports the column as text. (If not specified, this will be the default).
  - @c "date-columns": an array of date column specifications.\n
    This property is optional.\n
    Each specification consists of the following:
    - @c "name": the name of the column.
    - @c "parser": how to parse the column's date strings.\n
      Available options are:
      - @c "iso-date"
      - @c "iso-combined"
      - @c "rfc822"
      - @c "strptime-format"
      - @c "automatic" (if not specified, this will be the default).
    - @c "format": if @c "parser" is set to @c "strptime-format", then this is the user-defined format to parse with.

## Values {#values-properties}
Properties for the @c "values" node:
- @c "values": contains an array of key and value pairs, which are referenced by other items in the reports.\n
  Items reference key/value pairs via text labels using a special syntax. For example, @c label objects or graph
  titles can embed a reference to a run-time value, which will be expanded when the report is rendered.
  - @c "name": the key used for the item. Other items reference this using the syntax `{{name}}`, where @c name is the look-up key.
  - @c "value": either a string or numeric value to associate with the key.\n
    If a string, then it can be a literal string or a formula. The following formulas are available:\n
    - `min(dataset, column)`:
      Returns the minimum value of the given column from the dataset.
      - @c dataset is the name of the dataset (loaded from the ["datasources"](#datasources-properties) section).
      - @c column is the column name from the dataset.\n
    - `max(dataset, column)`:
      Returns the maximum value of the given column from the dataset.
      - @c dataset is the name of the dataset (loaded from the ["datasources"](#datasources-properties) section).
      - @c column is the column name from the dataset.\n
    - `n(dataset, column)`:
      Returns the valid number of observations in the given column from the dataset.
      - @c dataset is the name of the dataset (loaded from the ["datasources"](#datasources-properties) section).
      - @c column is the column name from the dataset.\n
    - `n(dataset, column, groupColum, groupId)`:
      Returns the valid number of observations in the given column from the dataset, using group filtering.
      - @c dataset is the name of the dataset (loaded from the ["datasources"](#datasources-properties) section).
      - @c column is the column name from the dataset.
      - @c groupColum is a group column to filter on.
      - @c groupId is the group ID to filter on.\n
        Note that @c groupId can either be a string or an embedded formula (which must be wrapped in a set of `{{` and `}}`).\n
        For example, the group ID can be a formula getting the highest label from the grouping column:\n
        `n(Awards, Degree, Academic Year, {{max(Awards, Academic Year)}})`\n

## Pages {#pages-properties}
A page is a grid-based container, where items (e.g., plots, labels) are layed out row-wise.\n
The @c "pages" node will contain an array of definitions for all pages, each containing the following:
- @c "name": contains a string value, representing the name of the page.
- @c "print": properties related to printer settings.\n
  - @c "orientation": string specifying the print orientation.
    Available options are:
    - @c "landscape" or @c "horizontal"
    - @c "portrait" or @c "vertical"
- @c "rows": an array of rows, each containing items to draw on the page (layed out horizontally).\n
  - @c "items": An array of items in the current row.
    - @c "type": the type of object that the item is (e.g., @c Wisteria::Graphs::Table, @c Wisteria::Graphs::LinePlot, etc.)\n
      Available options are:
      - ["line-plot"](#line-plot-properties)
      - ["categorical-bar-chart"](#categorical-bar-chart-properties)
      - ["common-axis"](#common-axis-properties)
      - ["image"](#image-properties)
      - ["label"](#label-properties)
      - ["pie-chart"](#pie-chart-properties)
      - ["table"](#table-properties)
      - @c null: a null value will add a placeholder on the page.\n
        If an entire row contains nulls, then the previous row will consume that row
        (meaning it will grow that much in height).

# Canvas Items

## Axis {#axis-properties}
Properties for @c "axes" nodes:
- @c "axis-type": the type of axis.\n
  Available options are:
  - @c "bottom-x"
  - @c "right-y"
  - @c "left-y"
  - @c "top-y"
- @c "title": the title of the axis, which contains ["label"](#label-properties) properties.
- @c "tickmarks": tickmarks settings, which contains the following properties:
  - @c "display": a string, indicating how to display the tickmarks.\n
    Available options are:
    - @c "inner"
    - @c "outer"
    - @c "crossed"
    - @c "no-display"
- @c "label-display": what to display for the labels along the axis.\n
  Available options are:
  - @c "custom-labels-or-values"
  - @c "only-custom-labels"
  - @c "custom-labels-and-values"
  - @c "no-display"

## Categorical Bar Chart {#categorical-bar-chart-properties}
Properties for @c "categorical-bar-chart" nodes:
- @c "variables": an item containing the following properties:
  - @c "aggregate": The (optional) aggregate count column.\n
       These are the values accumulated into the respective labels from the group column(s).\n
       If this column is not provided, then frequency counts of the labels from the group column(s) are used.
  - @c "category": the categorical column containing the items to group and count.
  - @c "group": a (optional) grouping column to further divide the main categorical data into.
- @c "bar-orientation": string specifying the orientation of the bars.\n
  Available options are:
  - @c "vertical"
  - @c "horizontal"
- Some base properties available to [graphs](#graph-properties).

## Common Axis {#common-axis-properties}
Properties for @c "common-axis" nodes:
- All properties available to ["common-axis"](#axis-properties) are included.\n
  Note that for @c "axis-type", common axes only support @c "bottom-x" and @c "right-y".
- @c "child-ids": a numeric array of IDs of the graphs that this common axis will manage.
- @c "common-perpendicular-axis": if @c "axis-type" is @c "bottom-x" and this is @c true,
     then the min and max of the graphs' left X axes will be used for the ranges of all the graphs.

## Image {#image-properties}
Properties for @c "image" nodes:
- @c "path": the file path of the image to load.

## Label {#label-properties}
Properties for @c "label" nodes:
- @c "text": the title's text.\n
  Note that this property supports embedded formulas that can reference user-defined values loaded from the
  ["values"](#values-properties) section.
- @c "orientation": string specifying the orientation of the text.\n
  Available options are:
  - @c "vertical"
  - @c "horizontal"
- @c "background": the background color. This can be either a color name or hex-encoded value.
- @c "color": the font color. This can be either a color name or hex-encoded value.
- @c "bold": @c true to make the text bold.
- @c "text-alignment": how to align the label's text.\n
  The available options are:
  - @c "flush-left" or @c "ragged-right"
  - @c "flush-right" or @c "ragged-left"
  - @c "centered"
  - @c "justified"
- @c "header": attributes to apply to the first row of the label.\n
     The following sub-properties are available:
  - @c "bold": @c true to make the header bold.
  - @c "color": the font color for the header. This can be either a color name or hex-encoded value.
  - @c "scaling": numeric value of how much to scale the header's font size. For example, @c 2.0 will double the
       header's default font size.\n
       Note that this will only affect the header scaling. To alter the label's scaling, use the label's root-level
       @c "scaling" property.

## Line Plot {#line-plot-properties}
Properties for @c "line-plot" nodes:
- @c "variables": an item containing the following properties:
  - @c "x": the X column.
  - @c "y": the X column.
  - @c "group": the grouping column (this is optional).
- Some base properties available to [graphs](#graph-properties).

## Pie Chart {#pie-chart-properties}
Properties for @c "pie-chart" nodes:
- @c "variables": an item containing the following properties:
  - @c "aggregate": The (optional) aggregate count column.\n
       These are the values accumulated into the respective labels from the group column(s).\n
       If this column is not provided, then frequency counts of the labels from the group column(s) are used.
  - @c "group-1": the inner-ring grouping column (or only ring, if @c "group-2" isn't used).
  - @c "group-2": the outer-ring grouping column (this is optional).
- @c "label-placement": a string specifying where to align the outer labels.\n
  Available options are:
  - @c "flush"
  - @c "next-to-parent"
- @c "inner-pie-midpoint-label-display": a string specifying what to display on the labels in the middle
     of the slices (within the inner pie).\n
  Available options are:
  - @c "value"
  - @c "percentage"
  - @c "value-and-percentage"
  - @c "bin-name"
  - @c "no-display"
- @c "outer-pie-midpoint-label-display": a string specifying what to display on the labels in the middle
     of the slices (within the outer pie, or pie if only one grouping variable is in use).\n
  Available options are:
  - @c "value"
  - @c "percentage"
  - @c "value-and-percentage"
  - @c "bin-name"
  - @c "no-display"
- @c "showcase-slices": draw attention to a slice (or series of slices).\n
  Available options are:
  - @c "pie": which pie to showcase.\n
    Available options are:
    - @c "inner"
    - @c "outer"
  - @c "category": which type of slices to showcase.\n
    Available options are:
    - @c "smallest"
    - @c "largest"
  - @c "by-group": if showcasing the inner pie, @c true will show the smallerst/largest slice for within each group.
  - @c "show-outer-pie-labels": if showcasing the inner pie, @c true keep the outer pie labels shown.
- @c "color-labels": @c true to apply the slice colors to their respective outer labels.
- @c "include-outer-pie-labels": @c true to show the outer labels for the outer pie (or main pie, if only using one grouping variable).
- @c "include-inner-pie-labels": @c true to show the outer labels for the inner pie (if a second grouping variable is in use).
- @c "donut-hole": donut hole properties, which include:
  - @c "proportion": a value between @c 0.0 and @c 0.95, specifying how much of the pie the hole should consume.
  - @c "label": the [label](#label-properties) shown in the middle of the hole.\n
        Note that this label will be implicitly justified and centered within the hole.
  - @c "color": the color of the donut hole.
- Some base properties available to [graphs](#graph-properties).

## Table {#table-properties}
Properties for @c "table" nodes:
- @c "transpose": @c true to transpose the data at the time of import. This means that the columns will become
     the rows and vice versa.\n
- @c "min-width-proportion": the minimum percent of the drawing area's width that the table should consume
     (between @c 0.0 to @c 1.0, representing 0% to 100%).
- @c "min-height-proportion": the minimum percent of the drawing area's height that the table should consume
     (between @c 0.0 to @c 1.0, representing 0% to 100%).
- @c "highlight-pen": the pen used for highlighting cells. Set to @c null to turn off the pen, or use the following properties:
  - @c "color": the pen color. This can be either a color name or hex-encoded value.
  - @c "width": the width of the pen's line.
- Some base properties available to [graphs](#graph-properties).

The remaining properties are executed in the following order:
- @c "rows-add": commands a series of rows to be added, which is an array of row properties containing the following:
  - @c "position": where to insert the row.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "values": an array of strings to fill the row with (left-to-right).
  - @c "background": the background color of the row.
- @c "rows-group": a numeric array representing which rows to apply label grouping to.\n
     Across each provided row, this will combine consecutive cells with the same label into one cell.
- @c "rows-color": an array of row and color pairs, which contain the following properties:
  - @c "position": which row to apply a background color.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "background": the background color of the row.
- @c "rows-content-align": an array of commands to align the content inside of the cells across a row.
  - @c "position": which row to change the content alignment in.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "horizontal-page-alignment": how to horizontally align the cells' content.\n
    Available options are:
    - @c "left-aligned"
    - @c "right-aligned"
    - @c "centered"
- @c "columns-add-aggregates": an array of column aggregate definitions that will be added to the table.\n
     Each column aggregate node contains the following properties:
  - @c "type": the type of aggregate column to add.\n
    Available options are:
    - @c "percent-change"
  - @c "name": the name for the column.
  - @c "start": the first column that the aggregate column should use.\n
       This property is optional, and if not included the first numeric column encountered will be used.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "end": the last column that the aggregate column should use.\n
       This property is optional, and if not included the last numeric column encountered will be used.
       Like @c "start", this is optional and has the same properties.
- @c "cells-update": an array of cell updating commands, which contain the following properties:
  - @c "column": the column position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "row": the row position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "column-count": the number of columns that this cell should consume.\n
       This can either be a number, or the string @c "all" (meaning all columns).
  - @c "row-count": the number of rows that this cell should consume.\n
       This can either be a number, or the string @c "all" (meaning all rows).
  - @c "value": a numeric, string, or null value to assign to the cell.
  - @c "background": the background color. This can be either a color name or hex-encoded value.
  - @c "bold": @c true to make the cell bold.
  - @c "highlight": @c true to highlight the cell.
  - @c "show-borders": an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - @c "horizontal-page-alignment": how to horizontally align the item within its area.\n
    Available options are:
    - @c "left-aligned"
    - @c "right-aligned"
    - @c "centered"

# Base-level Properties

## Positions {#position-properties}
Properties for row or column positions:
- @c "origin": this is either the zero-based index of the row/column, or a string.\n
     The string values available are @c "last-row" or @c "last-column",
     which will be interpreted as the last row or column in the data, respectively.
- @c "offset": a numeric value combined with the value for @c "origin".\n
     This is optional and is useful for when @c "origin" is interpreted at run-time.\n
     For example, if @c origin is @c "last-row" and @c offset is @c -1, then this will
     result in the second-to-last row.

## Graphs {#graph-properties}
Properties common to all graph items:
- @c "datasource": if the object requires a datasource (most graphs do), then this is the name of the datasource.\n
     Note that this is the unique name of the datasource loaded from the report's ["datasources"](#datasources-properties) section,
     not a filepath.\n
     Also, this is optional if the item type doesn't require a datasource (e.g., a @c Wisteria::GraphItems::Label).
- @c "title": the title of the graph, which contains ["label"](#label-properties) properties.
- @c "sub-title": the subtitle of the graph, which contains ["label"](#label-properties) properties.
- @c "caption": the caption of the graph, which contains ["label"](#label-properties) properties.
- @c "axes": an array of [axis](#axis-properties) objects.
- @c "color-scheme": for graphs that support color schemes only.\n
     This can either be an array of color strings or the name of the color scheme.\n
     Refer to Wisteria::Colors::Schemes for a list of color schemes.
- @c "icon-scheme": for graphs that support icon/marker schemes only.\n
  This is an array of icon strings (which can be recycled):
  - @c "blank-icon"
  - @c "horizontal-line-icon"
  - @c "arrow-right-icon"
  - @c "circle-icon"
  - @c "image-icon"
  - @c "horizontal-separator"
  - @c "horizontal-arrow-right-separator"
  - @c "image-whole-legend"
  - @c "color-gradient-icon"
  - @c "square-icon"
  - @c "triangle-upward-icon"
  - @c "triangle-downward-icon"
  - @c "triangle-right-icon"
  - @c "triangle-left-icon"
  - @c "diamond-icon"
  - @c "cross-icon"
  - @c "asterisk-icon"
  - @c "hexagon-icon"
  - @c "box-plot-icon"
  - @c "location-marker"
  - @c "go-road-sign"
  - @c "warning-road-sign"
- @c "legend": an item containing the following properties:
  - @c "placement": where to place the legend.\n
    Available options are:
    - @c "left"
    - @c "right"
    - @c "top"
    - @c "bottom"

## All Items {#item-properties}
Properties common to all items:
- @c "id": a (unique) numeric identifier for the item. This can be referenced by other items in the report.\n
     An example can be adding an ID to a graph and then inserted a ["common-axis"](#common-axis-properties) to the
     report that accepts this ID.
- @c "canvas-margins": a numeric array (one to four numbers), representing the item's canvas margin going clockwise,
     starting at 12 o'clock.
- @c "padding": a numeric array (one to four numbers), representing the item's padding going clockwise,
     starting at 12 o'clock.
- @c "pen": the item's pen. Set to @c null to turn off the pen, or use the following properties:
  - @c "color": the pen color. This can be either a color name or hex-encoded value.
  - @c "width": the width of the pen's line.
- @c "scaling": numeric value of how much to scale the object's size. For example, @c 2.0 will double the
     its default size.
- @c "relative-alignment": how an object is aligned with its parent (e.g., an axis title relative to its axis).\n
  Available options are:
  - @c "flush-left"
  - @c "flush-right"
  - @c "flush-top"
  - @c "flush-bottom"
  - @c "centered"
- @c "horizontal-page-alignment": how to horizontally align the item within its area.\n
  Available options are:
  - @c "left-aligned"
  - @c "right-aligned"
  - @c "centered"
- @c "vertical-page-alignment": how to vertically align the item within its area.\n
  Available options are:
  - @c "top-aligned"
  - @c "bottom-aligned"
  - @c "centered"
- @c "fit-row-to-content": a boolean value specifying whether the item's calculated height should
     control how tall its canvas row is.
- @c "fit-to-content-width": a boolean value specifying whether the item should be constrained
     to its calculated width within its row.

# Notes
Color values can either be hex encoded (e.g., "#FF0000" for red) or a named value ("pumpkin"). For a full list
of color names available, refer to the @c Wisteria::Colors::Color enumeration.