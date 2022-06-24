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
  - @c "parser": how to parse the datasource.\n
  The options are:
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
    - @c "parser": how to read the column.\n
    The options are:\n
      - @c "as-integers"
      - @c "as-strings" (if not specified, this will be the default)
  - @c "date-columns": an array of column name/parser/format structures from the data representing date variables.\n
  This property is optional.\n
  Each of the properties in this array consist of the following:
    - @c "name": the name of the column.
    - @c "parser": how to parse the column's date strings.\n
    The options are:
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
        - @c "line-plot"
        - @c "common-axis"
      - @c "datasource": if the object requires a datasource (most graphs do), then this is the name of the datasource.\n
      Note that this is the unique name of the datasource loaded from the report's @c "datasources" section,
      not a filepath.\n
      Also, this is optional if the item type doesn't require a datasource (e.g., a Wisteria::GraphItems::Label).
      - The remaining item properties are object specific. Refer to the following sections
      about the properties available for different object types.

@c "common-axis" properties:
- @c "axis-type": the type of axis.\n
  The options are:\n
  - @c "BottomXAxis"
  - @c "RightYAxis"
- @c "child-ids": a numeric array of IDs of the graphs that this common axis will manage.
- @c "common-perpendicular-axis": if "axis-type" is @c "BottomXAxis" and this is @c true,
  then the min and max of the graphs' left X axes will be used for the ranges of all the graphs.

@c "line-plot" properties:
- @c "variables": an item containing the following properties:
  - @c "x": the X column.
  - @c "y": the X column.
  - @c "group": the grouping column (this is optional).

@c "label" properties:
- @c "text": the title's text.

Properties common to all graph items:
- @c "title": the title of the graph, which contains @c label properties.
- @c "legend": an item containing the following properties:
  - @c "placement": where to place the legend.\n
    The options are:\n
    - @c "left"
    - @c "right"
    - @c "top"
    - @c "bottom"

Properties common to all items:
- @c "id": a (unique) numeric identifier for the item. This can be referenced by other items in the report.\n
  An example can be adding an ID to a graph and then inserted a @c "common-axis" to the report that accepts this ID.
- @c "canvas-padding": a numeric array (one to four numbers), representing the padding item's going clockwise,
  starting at 12 o'clock.