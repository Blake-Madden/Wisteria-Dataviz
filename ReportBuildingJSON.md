Report Building from a Project File
=============================
[TOC]

A report can be loaded directly from a JSON project file using the `ReportBuilder` class.
These reports can consist of a single page (i.e., a canvas), or a series of multiple pages.
On each page, items such as tables, plots, notes, etc. are embedded, and the project file
specifies how these items are laid out.

Along with the objects on the pages, a project file can also define the datasets and
user-defined values used by report's objects, printer settings, etc.

The following details the available options for JSON project files.

# Root-level Items
- @c "name": contains a string value, representing the name of the report.
- @c "print": properties related to printer settings.\n
  Available options are:
  - @c "orientation": string specifying the print orientation.
    Available options are:
    - @c "landscape" or @c "horizontal"
    - @c "portrait" or @c "vertical"

## Constants {#constants-properties}
Properties for the @c "constants" node:
- @c "constants": contains an array of key and value pairs, which are referenced by other items in the project.\n
  Items reference key/value pairs via text labels using a special syntax. For example, @c label objects or graph
  titles can embed a reference to a runtime value, which will be expanded when the report is rendered.
  - @c "name": the key used for the item. Other items reference this using the syntax `{{name}}`, where @c name is the look-up key.
  - @c "value": either a string or numeric value to associate with the key.\n
    If a number, then it will be formatted to the current locale when displayed in the report.\n

Note that datasets have a similar @c "formulas" section which can create constants from self-referencing formulas.

## Datasets {#datasets-properties}
Properties for the @c "datasets" node:
- @c "datasets": contains an array of datasets, which are referenced by other items in the report.
  - @c "name": the name of the dataset.\n
  This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - @c "path": the full file path of the dataset.
  - @c "importer": how to import the dataset.\n
    This is optional and the default is to import the file based on its file extension.\n
    Available options are:
    - @c "tsv": a tab-delimited text file.
    - @c "csv": a comma-delimited text file.
  - @c "continuous-md-recode-value": numeric value used to substitue missing data in continuous columns.\n
    This property is optional.
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
    - @c "format": if @c "parser" is set to @c "strptime-format",
         then this is the user-defined format to parse with.

  Next, any transformation commands for a dataset node are executed. These are performed in the following order:
  - @c "columns-rename": an array of column rename commands, which contain the following:
    - @c "name": the column to rename.
    - @c "new-name": the new name for the column.
  - @c "recode-re": an array of categorical column recode commands. This will apply a regular expression
       text replace for each label in the provided column(s). Each set of commands contains the following properties:
    - @c "column": the categorical column to recode.
    - @c "pattern": the regular expression pattern to search for.
    - @c "replacement": the replacement text. Note that capture groups are supported.

  Next, a @c "formulas" section can also be loaded from within the dataset's node.
  In this context, this is a formula string which references the dataset.\n
  This is an array of specifications which include the following properties:
  - @c "name": the key used for the item. Other items in the project reference this using the syntax `{{name}}`, where @c name is the look-up key.
  - @c "value": a string containing one of the following formulas:
    - ``Min(`column`)``:
      Returns the minimum value of the given column from the dataset.
      - @c column: the column name from the dataset.
    - ``Max(`column`)``:
      Returns the maximum value of the given column from the dataset.
      - @c column: the column name from the dataset.
    - ``Total(`column`)``:
      Returns the total of the given continuous column from the dataset.
      - @c column: the column name from the dataset.
    - ``GrandTotal()``:
      Returns the total of all continuous columns from the dataset.
    - ``N(`column`)``:
      Returns the valid number of observations in the given column from the dataset.
      - @c column: the column name from the dataset.
    - ``N(`column`, `groupColum`, `groupId`)``:
      Returns the valid number of observations in the given column from the dataset, using group filtering.\n
      - @c column: the column name from the dataset.
      - @c groupColum: a group column to filter on.
      - @c groupId: the group ID to filter on.
    - ``GroupCount(`groupColum`, `groupId`)``:
      Returns the number of occurences of @c groupId in the categorical column @c groupColum.\n
      - @c groupColum: the group column.
      - @c groupId: the group ID to count.
    - ``GroupPercentDecimal(`groupColum`, `groupId`)``:
      Returns the decimal percent (i.e., @c 0.0 to @c 1.0) of the categorical column @c groupColum
      that has the group ID @c groupId.\n
      - @c groupColum: the group column.
      - @c groupId: the group ID to count.
    - ``GroupPercent(`groupColum`, `groupId`)``:
      Returns the percent (as a string, such as @c "75%") of the categorical column @c groupColum
      that has the group ID @c groupId.\n
      - @c groupColum: the group column.
      - @c groupId: the group ID to count.
  
  Note that formula arguments can either be a string (wrapped in a pair of \`) or an embedded formula
  (which must be wrapped in a set of `{{` and `}}`).\n
  For example, the group ID can be a formula getting the highest label from the grouping column:\n

  ```
  N(`Degree`, `Academic Year`, {{max(`Academic Year`)}})
  ```

  Also, instead of referencing columns by name, functions are also available for referencing columns by index.
  - ``ContinuousColumn(`index`)``: returns the name of the continuous column at the given index.

  The following example would return the total of the first continuous column:

  ```
  Total({{ContinuousColumn(0)}})
  ```

  Next, the @c "subsets" section of the dataset's node is parsed. This is an array of subset specifications which
  contain the following properties:
  - @c "name": the name of the subset. (This should be different from the dataset that it is subsetting;
    otherwise, it will overwrite it.)\n
    This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - @c "filter": the subset filtering definition, which will contain the following:
    - @c "column": the column from the dataset to filter on.
    - @c "operator": how to compare the values from the column with the filter's value.
      Available options are:
      - @c "=" or "==": equals (the default)
      - @c "!=" or "<>": not equals
      - @c "<": less than
      - @c "<=": less than or equal to
      - @c ">": greater than
      - @c ">=": great than or equal to
    - @c "value": the value to filter the column on. This can be a number, string, or date
         (depending on the column's data type).\n
         Note that string values can reference constants loaded from the ["constants"](#constants-properties) section
         or @c "formulas" section of the parent dataset.

  Finally, the @c "pivots" section of the dataset's node is parsed. This is an array of pivot specifications which
  contain the following properties:
  - @c "name": the name of the pivoted dataset. (This should be different from the dataset that it is pivoting;
    otherwise, it will overwrite it.)\n
    This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - @c "type": a string specifying which type of pivot to use.
    Available options are:
    - "wider": pivot wider (the default).
    - "longer" pivot longer.

  If pivoting wider, the following options are available:
  - @c "id-columns": an array of strings representing the ID columns.
  - @c "names-from-column": a strings representing the 'names from'.
  - @c "values-from-columns": an array of strings representing the 'values from' columns.
  - @c "names-separator": If multiple value columns are provided, then this separator will
       join the label from @c namesFromColumn and the value column name.
  - @c "names-prefix": string to prepend to newly created pivot columns.
  - @c "fill-value": numeric value to assign to missing data. The default it so leave it as missing data.

  If pivoting longer, the following options are available:
  - @c "columns-to-keep": an array of strings specifying the columns to not pivot. These will be copied to the new dataset
    and will have their values filled in all new rows created from their observation.
    These would usually the ID columns.\n
    These columns can be of any type, including the ID column.
  - @c "from-columns": an array of strings specifying the continuous column(s) to pivot into longer format.
  - @c "names-to": an array of strings specifying the target column(s) to move the names from the @c "from-columns" into.
  - @c "values-to": a string specifying the column to move the values from the @c "from-columns" into.
  - @c "names-pattern": an optional string specifying a regular expression to split the @c "from-columns" names into.

  Note that subset and pivot nodes can contain their own transformation and formula sections,
  same as a dataset node.

## Pages {#pages-properties}
A page is a grid-based container, where items (e.g., plots, labels) are layed out row-wise.\n
The @c "pages" node will contain an array of definitions for all pages, each containing the following:
- @c "name": contains a string value, representing the name of the page.
- @c "rows": an array of rows, each containing items to draw on the page (layed out horizontally).\n
  - @c "items": An array of items in the current row.
    - @c "type": the type of object that the item is (e.g., @c Wisteria::Graphs::Table, @c Wisteria::Graphs::LinePlot, etc.)\n
      Available options are:
      - ["line-plot"](#line-plot-properties)
      - ["categorical-bar-chart"](#categorical-bar-chart-properties)
      - ["common-axis"](#common-axis-properties)
      - ["histogram"](#histogram-properties)
      - ["image"](#image-properties)
      - ["label"](#label-properties)
      - ["pie-chart"](#pie-chart-properties)
      - ["table"](#table-properties)
      - @c null: a @c null value will act as a placeholder for the previous column.\n
        If an entire row contains nulls, then the previous row will consume that row
        (meaning it will grow that much in height).

# Canvas Items

## Axis {#axis-properties}
@c "axes" nodes can either be part of a graph node definition or a ["common-axis"](#common-axis-properties) node.\n
  For the former, this can define properties for the various axes in the graph.\n
  @c "axes" nodes contain an array of axis objects, each with the following properties:
- @c "axis-type": the type of axis.\n
  Available options are:
  - @c "bottom-x"
  - @c "right-y"
  - @c "left-y"
  - @c "top-y"
- @c "title": the title of the axis, which contains ["label"](#label-properties) properties.
- @c "axis-pen": the pen for the axis line, which includes [pen](#pen-properties) properties.
- @c "gridline-pen": the pen for the gridlines, which includes [pen](#pen-properties) properties.
- @c "double-sided-labels": boolean value specifying whether axis labels should be shown on both sides of the axis.
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
- @c "brackets": adds brackets to the axis, which included the following properties:
  - @c "pen": the bracket line, which includes [pen](#pen-properties) properties.
  - @c "simplify": a boolean value indicating whether to simplify the bracket's labels.
  - @c "style": the style of the braces.\n
    Available options are:
    - @c "arrow"
    - @c "reverse-arrow"
    - @c "lines"
    - @c "curly-braces"
    - @c "no-connection-lines"

  If building brackets from a dataset, use the following properties:
  - @c "dataset": the name of the dataset to read the columns from.\n
       Note that this dataset can be different from the dataset used for the first
       child graph if you are wanting to use different labels. For this situation, the
       @c "value" variable should have the same scale as the child graph.
  - @c "variables": which include the following properties:
    - @c "label": the labels column.
    - @c "value": the values column. (This can be either a continous, categorical, or date column.)
  
  If building brackets individually, use the following property:
  - @c "bracket-items": an array of the items with the following properties:
    - @c "position-1": the starting axis position for the area.\n
         This can either be a numeric position, a date (as a string), or an axis label.
    - @c "position-2": the ending axis position for the area.\n
         This can either be a numeric position, a date (as a string), or an axis label.
    - @c "label": the text to display on the legend referring to area.
    - @c "pen": the bracket line, which includes [pen](#pen-properties) properties.

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
- Properties available to [bar charts](#bar-chart-properties).
- Some base properties available to [graphs](#graph-properties).

## Common Axis {#common-axis-properties}
Properties for @c "common-axis" nodes:
- All properties available to [axis](#axis-properties) nodes are included.\n
  Note that @c "axis-type" will specify where to place the common axis.
- @c "child-ids": a numeric array of IDs of the graphs that this common axis will manage.\n
     IDs are assigned to graphs using the @c "id" property, which should be numeric.
- @c "common-perpendicular-axis": if @c "axis-type" is @c "bottom-x" and this is @c true,
     then the min and max of the graphs' left X axes will be used for the ranges of all the graphs.

## Fillable Shape {#fillable-shape-properties}
Properties for @c "fillable-shape" nodes:
- @c "icon": a string specifying the icon to draw the shape as.
- @c "pen": the pen to draw with, which includes [pen](#pen-properties) properties.
- @c "brush": the pen to brush with, which includes [brush](#brush-properties) properties.
- @c "size": the size of the shape, which contains the following properties:
  - @c "width": the width.
  - @c "height": the height.
- @c "fill-percent": the percent to fill the shape, which is a number between @c 0.0 and @c 1.0.\n
  This can either be a numeric or a formula reference.

## Histogram {#histogram-properties}
Properties for @c "histogram" nodes:
- @c "variables": an item containing the following properties:
  - @c "aggregate": the continuous data column to aggregate.
  - @c "group": the grouping column (this is optional).
- @c "binning-method": string specifying how to sort the data.
  Available options are:
  - @c "bin-by-integer-range"
  - @c "bin-by-range"
  - @c "bin-unique-values"
- @c "interval-display": string specifying how to sort the data.
  Available options are:
  - @c "cutpoints"
  - @c "midpoints"
- @c "rounding": string specifying how to round the data.
  Available options are:
  - @c "no-rounding"
  - @c "round"
  - @c "round-down"
  - @c "round-up"
- @c "bins-start": number specifying where on the X axis to start the bins.
- @c "suggested-bin-count": number specifying the suggested number of bins.
- @c "max-bin-count": number specifying the maximumn number of bins.
- Properties available to [bar charts](#bar-chart-properties).
- Some base properties available to [graphs](#graph-properties).

## Image {#image-properties}
Properties for @c "image" nodes:
- @c "path": the file path of the image to load.
- @c "resize-method": How the image's size is adjusted when its boudning box is changed.\n
  Available options are:
  - @c "downscale-only"
  - @c "downscale-or-upscale"
  - @c "upscale-only"
  - @c "no-resize"

## Label {#label-properties}
Properties for @c "label" nodes:
- @c "text": the title's text.\n
  Note that this property supports embedded formulas that can reference user-defined values loaded from the
  ["constants"](#constants-properties) section.
- @c "orientation": string specifying the orientation of the text.\n
  Available options are:
  - @c "vertical"
  - @c "horizontal"
- @c "background": the background color. This can be either a color name or hex-encoded value.
- @c "color": the font color. This can be either a color name or hex-encoded value.
- @c "bold": @c true to make the text bold.
- @c "line-spacing": a number representing the spaces between lines (if text is multiline).
- @c "text-alignment": how to align the label's text.\n
  The available options are:
  - @c "flush-left" or @c "ragged-right"
  - @c "flush-right" or @c "ragged-left"
  - @c "centered"
  - @c "justified"
- @c "left-side-image": properties specifying an image to display on the left side of the label.
  The available options are:
  - @c "path": string specifying the image path to load.
- @c "header": attributes to apply to the first row of the label.\n
     The following sub-properties are available:
  - @c "bold": @c true to make the header bold.
  - @c "color": the font color for the header. This can be either a color name or hex-encoded value.
  - @c "relative-scaling": numeric value of how much to scale the header's font size relative to the label's scaling.
       For example, @c 2.0 will double the header's default font size compared to the rest of the label.\n
       Note that this will only affect the header scaling. To alter the label's scaling, use the label's root-level
       @c "scaling" property.

Note that it is recommended to set @c "fit-row-to-content" to @c true if the label is a separate
object on the canvas (i.e., not a label on another object).

## Line Plot {#line-plot-properties}
Properties for @c "line-plot" nodes:
- @c "variables": an item containing the following properties:
  - @c "x": the X column.
  - @c "y": the Y column.
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
- @c "showcase-slices-groups": an array of strings specifying which inners slices to showcase,
  based on the parent group(s). All outer labelling will be turned off, and the showcased inner
  slices' labels will be shown. The parent slices' colors will be solid, like their children inner slices.
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
  - @c "by-group": if showcasing the inner pie, @c true will show the smallest/largest slice for within each group.
  - @c "show-outer-pie-labels": if showcasing the inner pie, @c true keep the outer pie labels shown.
- @c "color-labels": @c true to apply the slice colors to their respective outer labels.
- @c "include-outer-pie-labels": @c true to show the outer labels for the outer pie
     (or main pie, if only using one grouping variable).
- @c "include-inner-pie-labels": @c true to show the outer labels for the inner pie
     (if a second grouping variable is in use).
- @c "donut-hole": donut hole properties, which include:
  - @c "proportion": a value between @c 0.0 and @c 0.95, specifying how much of the pie the hole should consume.
  - @c "label": the [label](#label-properties) shown in the middle of the hole.\n
        Note that this label will be implicitly justified and centered within the hole.
  - @c "color": the color of the donut hole.
- Some base properties available to [graphs](#graph-properties).

## Shape {#shape-properties}
Properties for @c "shape" nodes:
- @c "icon": a string specifying the icon to draw the shape as.
- @c "pen": the pen to draw with, which includes [pen](#pen-properties) properties.
- @c "brush": the pen to brush with, which includes [brush](#brush-properties) properties.
- @c "size": the size of the shape, which contains the following properties:
  - @c "width": the width.
  - @c "height": the height.

## Table {#table-properties}
Properties for @c "table" nodes:
- @c "variables": an array of column names to use from the dataset.\n
  These values can also be variable selection formulas, including the following:
  - ``{{matches(`value`)}}``: where @c value will be the regular expression search pattern, and any column
      in the dataset that contains this text will be included.
  - ``{{everything()}}``: will return all columns from the dataset. The column order will be the ID column (if in use),
      the categoricals, the continuous columns, and finally the date columns.
- @c "transpose": @c true to transpose the data at the time of import. This means that the columns will become
     the rows and vice versa.\n
- @c "min-width-proportion": the minimum percent of the drawing area's width that the table should consume
     (between @c 0.0 to @c 1.0, representing 0% to 100%).
- @c "min-height-proportion": the minimum percent of the drawing area's height that the table should consume
     (between @c 0.0 to @c 1.0, representing 0% to 100%).
- @c "highlight-pen": the pen used for highlighting cells, which includes [pen](#pen-properties) properties.
- Some base properties available to [graphs](#graph-properties).

Note that it is recommended to set @c "fit-row-to-content" to @c true for shorter tables to better fit them to the page.
Likewise, set @c "fit-row-to-content" to @c false for taller tables that are meant to consume most of a page's height.

The remaining properties are executed in the following order:
- @c "row-add": commands a series of rows to be added, which is an array of row properties containing the following:
  - @c "position": where to insert the row.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "constants": an array of strings to fill the row with (left-to-right).
  - @c "background": the background color of the row.
- @c "row-group": a numeric array representing which rows to apply label grouping to.\n
     Across each provided row, this will combine consecutive cells with the same label into one cell.
- @c "row-color": an array of row and color pairs, which contain the following properties:
  - @c "position": which row to apply a background color.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "background": the background color of the row.
  - @c "stops": an array of which columns to skip over when changing the row's color.\n
       This is an array of @c "position" items.
- @c "row-content-align": an array of commands to align the content inside of the cells across a row.
  - @c "position": which row to change the content alignment in.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "horizontal-page-alignment": how to horizontally align the cells' content.\n
    Available options are:
    - @c "left-aligned"
    - @c "right-aligned"
    - @c "centered"
  - @c "stops": an array of which columns to skip over when changing the column's content alignment.\n
       This is an array of @c "position" items.
- @c "column-group": a numeric array representing which columns to apply label grouping to.\n
     Down each provided column, this will combine consecutive cells with the same label into one cell.
- @c "column-color": an array of column and color pairs, which contain the following properties:
  - @c "position": which column to apply a background color.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "background": the background color of the column.
  - @c "stops": an array of which rows to skip over when changing the column's color.\n
       This is an array of @c "position" items.
- @c "column-borders": an array of column border specifications, which contain the following:
  - @c "position": which column to edit.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "borders": an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - @c "stops": an array of which rows to skip over when changing the column's borders.\n
       This is an array of @c "position" items.
- @c "column-highlight": an array of column highlight specifications, which contain the following:
  - @c "position": which column to edit.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "borders": an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - @c "stops": an array of which rows to skip over when changing the column's cell highlighting.\n
       This is an array of @c "position" items.
- @c "aggregates": an array of aggregate definitions that will be added to the table.\n
  Each aggregate node contains the following properties:
  - @c "type": where to add the aggregate.\n
    Available options are:
    - @c "row"
    - @c "column"
  - @c "aggregate-type": the type of aggregate to add.\n
    Available options are:
    - @c "percent-change"
    - @c "total"
  - @c "background": the background color for the column/row. This can be either a color name or hex-encoded value.
  - @c "name": the name for the newly added aggregate column/row.
  - @c "start": the first column/row that the aggregate column/row should use.\n
       This property is optional, and if not included the first numeric column/row encountered will be used.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "end": the last column/row that the aggregate column/row should use.\n
       This property is optional, and if not included the last numeric column/row encountered will be used.
       Like @c "start", this is optional and has the same properties.
- @c "row-totals": adds a grand total row to the bottom of the table.\n
  If the first column is grouped and the second colum is text, then this will also insert subtotal rows.\n
  Available options are:
  - @c "background": the background color for the row(s).
- @c "cell-update": an array of cell updating commands, which contain the following properties:
  - @c "column": the column position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "row": the row position of the cell to update.\n
       Refer to the [position](#position-properties) properties that are available.
  - @c "value-to-find": if @c "row"/"column" are not provided, this is a string to search for in
       the cells' content. The first cell that matches this text will be returned and that is the
       cell that will be updated.
  - @c "column-count": the number of columns that this cell should consume.\n
       This can either be a number, or the string @c "all" (meaning all columns).
  - @c "row-count": the number of rows that this cell should consume.\n
       This can either be a number, or the string @c "all" (meaning all rows).
  - @c "value": a numeric, string, or date value to assign to the cell.
  - @c "background": the background color. This can be either a color name or hex-encoded value.
  - @c "bold": @c true to make the cell bold.
  - @c "highlight": @c true to highlight the cell.
  - @c "prefix": a character to display on the left side of the cell.
  - @c "show-borders": an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - @c "horizontal-page-alignment": how to horizontally align the item within its area.\n
    Available options are:
    - @c "left-aligned"
    - @c "right-aligned"
    - @c "centered"
  - @c "text-alignment": how to align the cell's text (if multi-line).\n
    The available options are:
    - @c "flush-left" or @c "ragged-right"
    - @c "flush-right" or @c "ragged-left"
    - @c "centered"
    - @c "justified"
- @c "cell-annotations": an array of cell annotation specifications, which each contain the following properties:
  - @c "value":
  - @c "side":
  - @c "pen":
  - @c "background":
  - @c "cells":
    The available options are:
    - @c "column-outliers"
    - @c "column-top-n"
      The available options are:
      - @c "n"
- @c "footnotes": an array of footnote specifications, which each contain the following properties:
  - @c "value": the cell value (as a string) to add a footnote number to. Can include ["constants"](#constants-properties).
  - @c "footnote": the footnote to add to the caption. Can include ["constants"](#constants-properties).

# Base-level Properties

## Positions {#position-properties}
Properties for row or column positions:
- @c "origin": this is either the zero-based index of the row/column, or a string.\n
     The string values available are @c "last-row" or @c "last-column",
     which will be interpreted as the last row or column in the data, respectively.\n
     Column names from the dataset can also be used if looking for a column position.
- @c "offset": a numeric value combined with the value for @c "origin".\n
     This is optional and is useful for when @c "origin" is interpreted at runtime.\n
     For example, if @c origin is @c "last-row" and @c offset is @c -1, then this will
     result in the second-to-last row.

## Pen {#pen-properties}
Properties for pens:
- @c "color": the pen color. This can be either a color name or hex-encoded value.
- @c "width": the width of the pen's line.
- @c "style": the pen's style, which includes the following:
  - @c "dot"
  - @c "dot-dash"
  - @c "long-dot"
  - @c "short-dot"
  - @c "solid" (the default)
  - @c "cross-hatch"
  - @c "horizontal-hatch"
  - @c "vertical-hatch"

Note that a pen can be set to @c null to turn it off.

## Brush {#brush-properties}
Properties for brushes:
- @c "color": the brush color. This can be either a color name or hex-encoded value.
- @c "style": the brush's pattern, which includes the following:
  - @c "backwards-diagonal-hatch"
  - @c "forward-diagonal-hatch"
  - @c "cross-diagonal-hatch"
  - @c "solid" (the default)
  - @c "cross-hatch"
  - @c "horizontal-hatch"
  - @c "vertical-hatch"

Note that brushes can be set to @c null to turn it off.

## Bar Chart {#bar-chart-properties}
Properties common to all bar-chart derived graphs:
- @c "box-effect": string specifying the effects for the bars.
  Available options are:
  - @c "common-image"
  - @c "fade-from-bottom-to-top"
  - @c "fade-from-left-to-right"
  - @c "fade-from-right-to-left"
  - @c "fade-from-top-to-bottom"
  - @c "glassy"
  - @c "solid"
  - @c "stipple"
- @c "bar-label-display": string specifying how to display labels on the bars.
  Available options are:
  - @c "percentage"
  - @c "value"
  - @c "value-and-percentage"
  - @c "no-display"
  - @c "bin-name"
- @c "bar-sort": properties specifying how to sort the bars (after loading the data).
  - @c "direction"
    Available options are:
    - @c "ascending"
    - @c "descending"
  - @c "by": how to sort the bars, by label or length. This will override any @c "labels" option.
    Available options are:
    - @c "length"
    - @c "label"
  - @c "labels": the bar labels, specified in the order they should be sorted.\n
       If @c "direction" is @c "ascending", then the bars will be sorted top-to-bottom or
       left-to-right (based on the order of the provided labels). Otherwise, they will be
       sorted bottom-to-top or right-to-left.\n
       Note that this option is ignored if "by" is specified.

## Graphs {#graph-properties}
Properties common to all graph items:
- @c "dataset": if the object requires a dataset (most graphs do), then this is the name of the dataset.\n
     Note that this is the unique name of the dataset loaded from the report's
     ["datasets"](#datasets-properties) section, not a filepath.\n
     Also, this is optional if the item type doesn't require a dataset (e.g., a @c Wisteria::GraphItems::Label).
- @c "title": the title of the graph, which contains ["label"](#label-properties) properties.
- @c "sub-title": the subtitle of the graph, which contains ["label"](#label-properties) properties.
- @c "caption": the caption of the graph, which contains ["label"](#label-properties) properties.
- @c "axes": an array of [axis](#axis-properties) objects.
- @c "annotations": text messages that can be added on top of the graph, optionally pointing to positions
     on the graph. This is an array of annotation definitions, which contain the following properties:
  - @c "label": the label for the note, , which contains ["label"](#label-properties) properties.
  - @c "interest-points": The points that the note will point at.\n
    This is an array of items which contain the following properties:
    - @c "x": the X axis position of the point.
    - @c "y": the Y axis position of the point.
  - @c "anchor": the point to anchor the label.\n
     Note that the `label`'s @ "anchoring" property will control which part of the label that the
     anchoring point refers to.\n
     If not specified, the graph will attempt to place it at the best location.\n
     This contains the following properties:
     - @c "x": the X axis position of the point.
     - @c "y": the Y axis position of the point.
- @c "reference-lines": an array of reference line definitions, which contain the following properties:
  - @c "axis-type": the axis to add the reference line to.\n
    Available options are:
    - @c "bottom-x"
    - @c "right-y"
    - @c "left-y"
    - @c "top-y"
  - @c "position": the axis position for the line.\n
       This can either be a numeric position, a date (as a string), or an axis label.
  - @c "label": the text to display on the legend referring to line.
  - @c "pen": the definition for the [pen](#pen-properties) properties.
- @c "reference-areas": an array of reference area definitions, which contain the following properties:
  - @c "axis-type": the axis to add the reference area to.\n
    Available options are:
    - @c "bottom-x"
    - @c "right-y"
    - @c "left-y"
    - @c "top-y"
  - @c "position-1": the starting axis position for the area.\n
       This can either be a numeric position, a date (as a string), or an axis label.
  - @c "position-2": the ending axis position for the area.\n
       This can either be a numeric position, a date (as a string), or an axis label.
  - @c "label": the text to display on the legend referring to area.
  - @c "pen": the definition for the [pen](#pen-properties) properties.\n
        The color is used for the surrounding reference lines, and a tinted version
        is used the area.
  - @c "style": the style to apply to the reference area.\n
    Available options are:
    - @c "fade-from-left-to-right"
    - @c "fade-from-right-to-left"
    - @c "solid"
- @c "brush-scheme": for graphs that support brush schemes only.\n
     Contains the following properties:
     @c "brush-styles": an array of strings specifying the [brush styles](#brush-properties).
     @c "color-scheme": a color scheme that is mapped to the brush styles.
- @c "color-scheme": for graphs that support color schemes only.\n
     This can either be an array of color strings or the name of the color scheme.\n
     Refer to @c Wisteria::Colors::Schemes for a list of color schemes.
- @c "icon-scheme": for graphs that support icon/marker schemes only.\n
  This an either be an icon scheme name or an array of icon strings (which can be recycled)\n
  Refer to @c Wisteria::Icons::Schemes for a list of icon schemes.\n
  The following individual icons are available:
  - @c "blank"
  - @c "horizontal-line"
  - @c "arrow-right"
  - @c "circle"
  - @c "image"
  - @c "horizontal-separator"
  - @c "horizontal-arrow-right-separator"
  - @c "color-gradient"
  - @c "square"
  - @c "triangle-upward"
  - @c "triangle-downward"
  - @c "triangle-right"
  - @c "triangle-left"
  - @c "diamond"
  - @c "plus"
  - @c "asterisk"
  - @c "hexagon"
  - @c "box-plot"
  - @c "location-marker"
  - @c "go-road-sign"
  - @c "warning-road-sign"
  - @c "sun"
  - @c "flower"
  - @c "fall-leaf"
  - @c "top-curly-brace"
  - @c "right-curly-brace"
  - @c "bottom-curly-brace"
  - @c "left-curly-brace"
  - @c "male"
  - @c "female"
  - @c "female-business"
- @c "line-scheme": for graphs that support line schemes only.\n
  This is an array of line specifications (which can be recycled)\n
  Each specification contains the following:
  - @c "line-style": a string specifying the line style.\n
    Available options are:
    - @c "lines"
    - @c "arrows"
    - @c "spline"
  - @c "pen-style": a [pen](#pen-properties) specificaiton (although only the pen style is used)
- @c "legend": an item containing the following properties:
  - @c "placement": where to place the legend.\n
    Available options are:
    - @c "left"
    - @c "right" (the default)
    - @c "top"
    - @c "bottom"
  - @c "include-header": @c true to include a header (the default), @c false to not.
  - @c "title": if @c "include-header" is @c true, a string specifying the title of the legend.
  - @c "ring": for pie charts, which ring to use for the legend.\n
    Available options are:
    - @c "outer": only the outer (or only) ring is included. (This is the default.)
    - @c "inner" : the inner and outer rings are included in the legend.

## All Items {#item-properties}
Properties common to all items:
- @c "id": a (unique) numeric identifier for the item. This can be referenced by other items in the report.\n
     An example can be adding an ID to a graph and then inserted a ["common-axis"](#common-axis-properties) to the
     report that accepts this ID.
- @c "canvas-margins": a numeric array (one to four numbers), representing the item's canvas margin going clockwise,
     starting at 12 o'clock.
- @c "padding": a numeric array (one to four numbers), representing the item's padding going clockwise,
     starting at 12 o'clock.
- @c "pen": the item's pen, which includes [pen](#pen-properties) properties
- @c "scaling": numeric value of how much to scale the object's size. For example, @c 2.0 will double the
     its default size.
- @c "anchoring": controls the starting point of where the object is drawn.\n
  This is usually only relative for objects being embedded on a plot (e.g., annotations).\n
  Available options are:
  - @c "bottom-left-corner"
  - @c "bottom-right-corner"
  - @c "center" (the default)
  - @c "top-left-corner"
  - @c "top-right-corner"
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
- @c "fixed-width": a boolean value specifying whether the item should be constrained
     to its calculated width within its row.

# Notes {#notes}
Color values can either be hex encoded (e.g., @c "#FF0000" for red) or a named value (@c "pumpkin"). For a full list
of color names available, refer to the @c Wisteria::Colors::Color enumeration.

Constants defined in the ["constants"](#constants-properties) can also be referenced, which likewise can
be named colors or hex encoded values.