Report Building from a Configuration File
=============================
[TOC]

A report can be loaded directly from a JSON configuration file using the `ReportBuilder` class.
These reports can consist of a single page (i.e., a canvas), or a series of multiple pages.
On each page, items such as tables, plots, notes, etc. are embedded, and the configuration file
specifies how these items are laid out.

Along with the objects on the pages, a configuration file can also define the datasources used
by report's objects, per-page printer settings, etc.

The following details the available options for JSON configuration files.

At the root level:

- @c "name": contains a string value, representing the name of the report.
- @c "datasources": contains an array of datasources, which are referenced by other items in the report.
  - @c "name": the name of the datasource.\n
  This name is referenced by items (e.g., plots) elsewhere in the configuration file and must be unique.
  - @c "path": the full file path of the datasource.
  - @c "parser": how to parse the datasource.\n The options are:
    - @c "tsv"
    - @c "csv"
  - @c "id-column": the ID column.\n
       This property is optional.
  - @c "continuous-columns": an array of column names from the data representing continuous variables.\n
       This property is optional.
  - @c "categorical-columns": an array of column name/parser pairs from the data representing categorical variables.\n
       This property is optional.\n
  Each of the properties in this array consist of the following:
    - @c "name": the name of the column.
    - @c "parser": how to read the column.\n The options are:\n
      - @c "as-integers"
      - @c "as-strings" (if not specified, this will be the default)
  - @c "date-columns": an array of column name/parser/format structures from the data representing date variables.\n
       This property is optional.\n
       Each of the properties in this array consist of the following:
    - @c "name": the name of the column.
    - @c "parser": how to parse the column's date strings.\n The options are:
      - @c "iso-date"
      - @c "iso-combined"
      - @c "rfc822"
      - @c "strptime-format"
      - @c "automatic" (if not specified, this will be the default)
    - @c "format": if @c "parser" is set to @c "strptime-format", then this is the user-defined format to parse with.
- @c "pages": contains an array of pages.
  - @c "name": contains a string value, representing the name of the page.
  - @c "rows": an array of rows, containing items to draw on the canvas.
    - @c "items": An array of items in the current row.
      - @c "type": the type of object that the item is (e.g., @c Wisteria::Graphs::Table, @c Wisteria::Graphs::LinePlot, etc.)\n
        The options are:
        - ["line-plot"](#line-plot-properties)
        - ["common-axis"](#common-axis-properties)
        - ["label"](#label-properties)
        - ["table"](#table-properties)
        - @c null: a null value will add a placeholder on the canvas.\n
          If an entire row contains nulls, then the previous row will consume that row
          (meaning it will grow that much in height).
      - The remaining item properties are object specific.\n
        Refer to the following sections about the properties available for different object types.

# Canvas Items

## Axis {#axis-properties}
Properties for @c "axis" nodes:
- @c "axis-type": the type of axis.\n
  The options are:\n
  - @c "bottom-x"
  - @c "right-y"
  - @c "left-y"
  - @c "top-y"
- @c "title": the title of the axis, which contains ["label"](#label-properties) properties.

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

## Line Plot {#line-plot-properties}
Properties for @c "line-plot" nodes:
- @c "variables": an item containing the following properties:
  - @c "x": the X column.
  - @c "y": the X column.
  - @c "group": the grouping column (this is optional).

## Label {#label-properties}
Properties for @c "label" nodes:
- @c "text": the title's text.
- @c "background": the background color. This can be a either a color name or hex-encoded value.
- @c "color": the font color. This can be a either a color name or hex-encoded value.
- @c "bold": @c true to make the text bold.

## Table {#table-properties}
Properties for @c "table" nodes:
- @c "transpose": @c true to transpose the data at the time of import. This means that the columns will become
     the rows and vice versa.\n
     The following properties are executed in the following order:
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
- @c "columns-add-aggregates": an array of column aggregate definitions that will be added to the table.\n
     Each column aggregate node contains the following properties:
  - @c "type": the type of aggegate column to add. The options are:
    - @c "percent-change"
  - @c "name": the name for the column.
  - @c "start": the first column that the aggregate column should use.\n
       This property is optional, and if not included the first numeric column encountered will be used.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "end": the last column that the aggregate column should use.\n
       This property is optional, and if not included the last numeric column encountered will be used.
       Like @c "start", this is optional an has the same properties.
- @c "cells-update": an array of cell updating commands, which contain the following properties:
  - @c "column": the column position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "row": the row position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "column-count": the number of columns that this cell should consume.\n
       The can either be a number, or the string @c "all" (meaning all columns).

# Base-level Properties

## Positions {#position-properties}
Properties for row or column positions:
- @c "origin": this is either the zero-based index of row/column, or a string.\n
     The string values available are @c "last-row" or @c "last-column",
     which will be interpretted as the last row or column in the data, respectively.
- @c "offset": a numeric value combined with the value for @c "origin".\n
     This is optional and is useful for when @c "origin" is interpretted at runtime
     (for something like @c "last-row").

## Graphs {#graph-properties}
Properties common to all graph items:
- @c "datasource": if the object requires a datasource (most graphs do), then this is the name of the datasource.\n
      Note that this is the unique name of the datasource loaded from the report's @c "datasources" section,
      not a filepath.\n
      Also, this is optional if the item type doesn't require a datasource (e.g., a Wisteria::GraphItems::Label).
- @c "title": the title of the graph, which contains ["label"](#label-properties) properties.
- @c "sub-title": the subtitle of the graph, which contains ["label"](#label-properties) properties.
- @c "caption": the caption of the graph, which contains ["label"](#label-properties) properties.
- @c "axes": an array of ["axis"](#axis-properties) objects.
- @c "legend": an item containing the following properties:
  - @c "placement": where to place the legend.\n
    The options are:\n
    - @c "left"
    - @c "right"
    - @c "top"
    - @c "bottom"

## All Items {#item-properties}
Properties common to all items:
- @c "id": a (unique) numeric identifier for the item. This can be referenced by other items in the report.\n
  An example can be adding an ID to a graph and then inserted a ["common-axis](#common-axis-properties) to the
  report that accepts this ID.
- @c "canvas-margin": a numeric array (one to four numbers), representing the item's canvas margin going clockwise,
  starting at 12 o'clock.
- @c "padding": a numeric array (one to four numbers), representing the item's padding going clockwise,
  starting at 12 o'clock.
- @c "relative-alignment": how an object is aligned with its parent (e.g., an axis title relative to its axis).\n
  The options are:\n
  - @c "flush-left"
  - @c "flush-right"
  - @c "flush-top"
  - @c "flush-bottom"
  - @c "centered"
- @c "horizontal-page-alignment": how to horizontally align the item within its area.\n
  The options are:\n
  - @c "left-aligned"
  - @c "right-aligned"
  - @c "centered"
- @c "vertical-page-alignment": how to vertically align the item within its area.\n
  The options are:\n
  - @c "top-aligned"
  - @c "bottom-aligned"
  - @c "centered"
- @c "fit-row-to-content": a boolean value specifying whether the item's calculated height should
  control how tall its canvas row is.
- @c "fit-to-content-width": a boolean value specifying whether the item should be constrained
  to its calculated width within its row.