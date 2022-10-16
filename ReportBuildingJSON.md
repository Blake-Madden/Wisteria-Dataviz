Report Building from a Project File
=============================

A report can be loaded directly from a JSON project file using the `ReportBuilder` class.
These reports can consist of a single page (i.e., a canvas), or a series of multiple pages.
On each page, items such as tables, plots, notes, etc. are embedded, and the project file
specifies how these items are laid out.

Along with the objects on the pages, a project file can also define the datasets and
user-defined values used by report's objects, printer settings, etc.

The following details the available options for JSON project files.

# Project Properties

- `"name"` contains a string value, representing the name of the report.
- `"print"` properties related to printer settings.<br />
  Available options are:
  - `"orientation"` string specifying the print orientation.<br />
    Available options are:
    - `"landscape"` or `"horizontal"`
    - `"portrait"` or `"vertical"`
  - `"paper-size"`: the paper size to print to.<br />
    Available options are:
    - `"paper-letter"`: Letter, 8 1/2 by 11 inches.
    - `"paper-legal"`: Legal, 8 1/2 by 14 inches.
    - `"paper-a4"`: A4 Sheet, 210 by 297 millimeters.
    - `"paper-csheet"`: C Sheet, 17 by 22 inches.
    - `"paper-dsheet"`: D Sheet, 22 by 34 inches.
    - `"paper-esheet"`: E Sheet, 34 by 44 inches.
    - `"paper-lettersmall"`: Letter Small, 8 1/2 by 11 inches.
    - `"paper-tabloid"`: Tabloid, 11 by 17 inches.
    - `"paper-ledger"`: Ledger, 17 by 11 inches.
    - `"paper-statement"`: Statement, 5 1/2 by 8 1.
    - `"paper-executive"`: Executive, 7 1/4 by 10 1.
    - `"paper-a3"`: A3 sheet, 297 by 420 millimeters.
    - `"paper-a4small"`: A4 small sheet, 210 by 297 millimeters.
    - `"paper-a5"`: A5 sheet, 148 by 210 millimeters.
    - `"paper-b4"`: B4 sheet, 250 by 354 millimeters.
    - `"paper-b5"`: B5 sheet, 182-by-257-millimeter paper.
    - `"paper-folio"`: Folio, 8-1/2-by-13-inch paper.
    - `"paper-quarto"`: Quarto, 215-by-275-millimeter paper.
    - `"paper-10x14"`: 10-by-14-inch sheet.
    - `"paper-11x17"`: 11-by-17-inch sheet.
    - `"paper-note"`: Note, 8 1/2 by 11 inches.
    - `"paper-env-9"`: #9 Envelope, 3 78 by 8 7.
    - `"paper-env-10"`: #10 Envelope, 4 1/8 by 9 1.
    - `"paper-env-11"`: #11 Envelope, 4 1/2 by 10 3.
    - `"paper-env-12"`: #12 Envelope, 4 3/4 by 11 inches.
    - `"paper-env-14"`: #14 Envelope, 5 by 11 1/2 inches.
    - `"paper-env-dl"`: DL Envelope, 110 by 220 millimeters.
    - `"paper-env-c5"`: C5 Envelope, 162 by 229 millimeters.
    - `"paper-env-c3"`: C3 Envelope, 324 by 458 millimeters.
    - `"paper-env-c4"`: C4 Envelope, 229 by 324 millimeters.
    - `"paper-env-c6"`: C6 Envelope, 114 by 162 millimeters.
    - `"paper-env-c65"`: C65 Envelope, 114 by 229 millimeters.
    - `"paper-env-b4"`: B4 Envelope, 250 by 353 millimeters.
    - `"paper-env-b5"`: B5 Envelope, 176 by 250 millimeters.
    - `"paper-env-b6"`: B6 Envelope, 176 by 125 millimeters.
    - `"paper-env-italy"`: Italy Envelope, 110 by 230 millimeters.
    - `"paper-env-monarch"`: Monarch Envelope, 3 78 by 7 1.
    - `"paper-env-personal"`: 6 3/4 Envelope, 3 5.
    - `"paper-fanfold-us"`: US Std Fanfold, 14 78 by 11 inches.
    - `"paper-fanfold-std-german"`: German Std Fanfold, 8 1/2 by 1/2 inches.
    - `"paper-fanfold-lgl-german"`: German Legal Fanfold, 8 1/2 by 13 inches.
    - `"paper-iso-b4"`: B4 (ISO) 250 x 353 mm.
    - `"paper-japanese-postcard"`: Japanese Postcard 100 x 148 mm.
    - `"paper-9x11"`: 9 x 11 in.
    - `"paper-10x11"`: 10 x 11 in.
    - `"paper-15x11"`: 15 x 11 in.
    - `"paper-env-invite"`: Envelope Invite 220 x 220 mm.
    - `"paper-letter-extra"`: Letter Extra 9 \275 x 12 in.
    - `"paper-legal-extra"`: Legal Extra 9 \275 x 15 in.
    - `"paper-tabloid-extra"`: Tabloid Extra 11.69 x 18 in.
    - `"paper-a4-extra"`: A4 Extra 9.27 x 12.69 in.
    - `"paper-letter-transverse"`: Letter Transverse 8 \275 x 11 in.
    - `"paper-a4-transverse"`: A4 Transverse 210 x 297 mm.
    - `"paper-letter-extra-transverse"`: Letter Extra Transverse 9\275 x 12 in.
    - `"paper-a-plus"`: SuperA.
    - `"paper-b-plus"`: SuperB.
    - `"paper-letter-plus"`: Letter Plus 8.5 x 12.69 in.
    - `"paper-a4-plus"`: A4 Plus 210 x 330 mm.
    - `"paper-a5-transverse"`: A5 Transverse 148 x 210 mm.
    - `"paper-b5-transverse"`: B5 (JIS) Transverse 182 x 257 mm.
    - `"paper-a3-extra"`: A3 Extra 322 x 445 mm.
    - `"paper-a5-extra"`: A5 Extra 174 x 235 mm.
    - `"paper-b5-extra"`: B5 (ISO) Extra 201 x 276 mm.
    - `"paper-a2"`: A2 420 x 594 mm.
    - `"paper-a3-transverse"`: A3 Transverse 297 x 420 mm.
    - `"paper-a3-extra-transverse"`: A3 Extra Transverse 322 x 445 mm.
    - `"paper-dbl-japanese-postcard"`: Japanese Double Postcard 200 x 148 mm.
    - `"paper-a6"`: A6 105 x 148 mm.
    - `"paper-jenv-kaku2"`: Japanese Envelope Kaku #2.
    - `"paper-jenv-kaku3"`: Japanese Envelope Kaku #3.
    - `"paper-jenv-chou3"`: Japanese Envelope Chou #3.
    - `"paper-jenv-chou4"`: Japanese Envelope Chou #4.
    - `"paper-letter-rotated"`: Letter Rotated 11 x 8 1/2 in.
    - `"paper-a3-rotated"`: A3 Rotated 420 x 297 mm.
    - `"paper-a4-rotated"`: A4 Rotated 297 x 210 mm.
    - `"paper-a5-rotated"`: A5 Rotated 210 x 148 mm.
    - `"paper-b4-jis-rotated"`: B4 (JIS) Rotated 364 x 257 mm.
    - `"paper-b5-jis-rotated"`: B5 (JIS) Rotated 257 x 182 mm.
    - `"paper-japanese-postcard-rotated"`: Japanese Postcard Rotated 148 x 100 mm.
    - `"paper-dbl-japanese-postcard-rotated"`: Double Japanese Postcard Rotated 148 x 200 mm.
    - `"paper-a6-rotated"`: A6 Rotated 148 x 105 mm.
    - `"paper-jenv-kaku2-rotated"`: Japanese Envelope Kaku #2 Rotated.
    - `"paper-jenv-kaku3-rotated"`: Japanese Envelope Kaku #3 Rotated.
    - `"paper-jenv-chou3-rotated"`: Japanese Envelope Chou #3 Rotated.
    - `"paper-jenv-chou4-rotated"`: Japanese Envelope Chou #4 Rotated.
    - `"paper-b6-jis"`: B6 (JIS) 128 x 182 mm.
    - `"paper-b6-jis-rotated"`: B6 (JIS) Rotated 182 x 128 mm.
    - `"paper-12x11"`: 12 x 11 in.
    - `"paper-jenv-you4"`: Japanese Envelope You #4.
    - `"paper-jenv-you4-rotated"`: Japanese Envelope You #4 Rotated.
    - `"paper-p16k"`: PRC 16K 146 x 215 mm.
    - `"paper-p32k"`: PRC 32K 97 x 151 mm.
    - `"paper-p32kbig"`: PRC 32K(Big) 97 x 151 mm.
    - `"paper-penv-1"`: PRC Envelope #1 102 x 165 mm.
    - `"paper-penv-2"`: PRC Envelope #2 102 x 176 mm.
    - `"paper-penv-3"`: PRC Envelope #3 125 x 176 mm.
    - `"paper-penv-4"`: PRC Envelope #4 110 x 208 mm.
    - `"paper-penv-5"`: PRC Envelope #5 110 x 220 mm.
    - `"paper-penv-6"`: PRC Envelope #6 120 x 230 mm.
    - `"paper-penv-7"`: PRC Envelope #7 160 x 230 mm.
    - `"paper-penv-8"`: PRC Envelope #8 120 x 309 mm.
    - `"paper-penv-9"`: PRC Envelope #9 229 x 324 mm.
    - `"paper-penv-10"`: PRC Envelope #10 324 x 458 mm.
    - `"paper-p16k-rotated"`: PRC 16K Rotated.
    - `"paper-p32k-rotated"`: PRC 32K Rotated.
    - `"paper-p32kbig-rotated"`: PRC 32K(Big) Rotated.
    - `"paper-penv-1-rotated"`: PRC Envelope #1 Rotated 165 x 102 mm.
    - `"paper-penv-2-rotated"`: PRC Envelope #2 Rotated 176 x 102 mm.
    - `"paper-penv-3-rotated"`: PRC Envelope #3 Rotated 176 x 125 mm.
    - `"paper-penv-4-rotated"`: PRC Envelope #4 Rotated 208 x 110 mm.
    - `"paper-penv-5-rotated"`: PRC Envelope #5 Rotated 220 x 110 mm.
    - `"paper-penv-6-rotated"`: PRC Envelope #6 Rotated 230 x 120 mm.
    - `"paper-penv-7-rotated"`: PRC Envelope #7 Rotated 230 x 160 mm.
    - `"paper-penv-8-rotated"`: PRC Envelope #8 Rotated 309 x 120 mm.
    - `"paper-penv-9-rotated"`: PRC Envelope #9 Rotated 324 x 229 mm.
    - `"paper-penv-10-rotated"`: PRC Envelope #10 Rotated 458 x 324 m.
    - `"paper-a0"`: A0 Sheet 841 x 1189 mm.
    - `"paper-a1"`: A1 Sheet 594 x 841 mm.

# Constants {#constants-properties}

Properties for the `"constants"` node:

- `"constants"`: contains an array of key and value pairs, which are referenced by other items in the project.<br />
  Items reference key/value pairs via text labels using a special syntax. For example, `label` objects or graph
  titles can embed a reference to a runtime value, which will be expanded when the report is rendered.
  - `"name"`: the key used for the item. Other items reference this using the syntax `{{name}}`, where `name` is the look-up key.
  - `"value"` either a string or numeric value to associate with the key.<br />
    If a number, then it will be formatted to the current locale when displayed in the report.<br />

Note that datasets have a similar `"formulas"` section which can create constants from self-referencing formulas.

# Datasets {#datasets-properties}

Properties for the `"datasets"` node:

- `"datasets"`: contains an array of datasets, which are referenced by other items in the report.
  - `"name"`: the name of the dataset.<br />
  This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - `"path"`: the full file path of the dataset.
  - `"importer"` how to import the dataset.<br />
    This is optional and the default is to import the file based on its file extension.<br />
    Available options are:
    - `"tsv"`: a tab-delimited text file.
    - `"csv"`: a comma-delimited text file.
  - `"continuous-md-recode-value"` numeric value used to substitute missing data in continuous columns.<br />
    This property is optional.
  - `"id-column"`: the ID column.<br />
    This property is optional.
  - `"continuous-columns"`: an array of continuous column names to import.<br />
    This property is optional.
  - `"categorical-columns"`: an array of categorical column specifications.<br />
    This property is optional.<br />
    Each specification consists of the following:
    - `"name"`: the name of the column.
    - `"parser"` how to read the column.<br />
      Available options are:
      - `"as-integers"` imports the column as discrete integers.
      - `"as-strings"` imports the column as text. (If not specified, this will be the default).
  - `"date-columns"`: an array of date column specifications.<br />
    This property is optional.<br />
    Each specification consists of the following:
    - `"name"`: the name of the column.
    - `"parser"` how to parse the column's date strings.<br />
      Available options are:
      - `"iso-date"`
      - `"iso-combined"`
      - `"rfc822"`
      - `"strptime-format"`
      - `"automatic"` (if not specified, this will be the default).
    - `"format"` if `"parser"` is set to `"strptime-format"`,
         then this is the user-defined format to parse with.

  Next, any transformation commands for a dataset node are executed. These are performed in the following order:

  - `"columns-rename"`: an array of column rename commands, which contain the following:
    - `"name"`: the column to rename.
    - `"new-name"`: the new name for the column.
  - `"recode-re"`: an array of categorical column recode commands. This will apply a regular expression
       text replace for each label in the provided column(s). Each set of commands contains the following properties:
    - `"column"`: the categorical column to recode.
    - `"pattern"`: the regular expression pattern to search for.
    - `"replacement"`: the replacement text. Note that capture groups are supported.
  - `"collapse-min"` collapses strings which appear fewer than a minimum number of times. This is an array of
    specifications which contain the following:
    - `"column"`: the categorical column to collapse.
    - `"min"`: the minimum number of times a string must appear in the column to remain in the string table.
    - `"other-label"`: the label to use for the new category where low-frequency values are lumped into. Default is `"Other"`.
  - `"collapse-except"` collapses strings into an "Other" category, except for a list of provided labels.
    This is an array of specifications which contain the following:
    - `"column"`: the categorical column to collapse.
    - `"labels-to-keep"`: an array of strings to preserve; all others will be lumped into a new `"Other"` category.
    - `"other-label"`: the label to use for the new category where other labels are lumped into. Default is `"Other"`.

  Next, a `"formulas"` section can also be loaded from within the dataset's node.
  In this context, this is a formula string which references the dataset.<br />
  This is an array of specifications which include the following properties:

  - `"name"`: the key used for the item. Other items in the project reference this using the syntax `{{name}}`, where `name` is the look-up key.
  - `"value"`: a string containing one of the following formulas:
    - ``Min(`column`)``:
      Returns the minimum value of the given column from the dataset.
      - `column`: the column name from the dataset.
    - ``Max(`column`)``:
      Returns the maximum value of the given column from the dataset.
      - `column`: the column name from the dataset.
    - ``Total(`column`)``:
      Returns the total of the given continuous column from the dataset.
      - `column`: the column name from the dataset.
    - ``GrandTotal()``:
      Returns the total of all continuous columns from the dataset.
    - ``N(`column`)``:
      Returns the valid number of observations in the given column from the dataset.
      - `column`: the column name from the dataset.
    - ``N(`column`, `groupColum`, `groupId`)``:
      Returns the valid number of observations in the given column from the dataset, using group filtering.<br />
      - `column`: the column name from the dataset.
      - `groupColum`: a group column to filter on.
      - `groupId`: the group ID to filter on.
    - ``GroupCount(`groupColum`, `groupId`)``:
      Returns the number of occurrences of `groupId` in the categorical column `groupColum`.<br />
      - `groupColum`: the group column.
      - `groupId`: the group ID to count.
    - ``GroupPercentDecimal(`groupColum`, `groupId`)``:
      Returns the decimal percent (i.e., `0.0` to `1.0`) of the categorical column `groupColum`
      that has the group ID `groupId`.<br />
      - `groupColum`: the group column.
      - `groupId`: the group ID to count.
    - ``GroupPercent(`groupColum`, `groupId`)``:
      Returns the percent (as a string, such as `"75%"`) of the categorical column `groupColum`
      that has the group ID `groupId`.<br />
      - `groupColum`: the group column.
      - `groupId`: the group ID to count.
  
  Note that formula arguments can either be a string (wrapped in a pair of \`) or an embedded formula
  (which must be wrapped in a set of `{{`: and `}}`).<br />
  For example, the group ID can be a formula getting the highest label from the grouping column:<br />

  ```
  N(`Degree`, `Academic Year`, {{max(`Academic Year`)}})
  ```

  Also, instead of referencing columns by name, functions are also available for referencing columns by index.
  - ``ContinuousColumn(index)``: returns the name of the continuous column at the given index. Index can either
    be a number or `last` (returns the last continuous column).

  The following example would return the total of the first continuous column:

  ```
  Total({{ContinuousColumn(0)}})
  ```

  The following example would return the total of the last continuous column:

  ```
  Total({{ContinuousColumn(`last`)}})
  ```

  Along with the dataset-related functions, [additional functions](#additional-functions) are also available.

  Next, the `"subsets"` section of the dataset's node is parsed. This is an array of subset specifications which
  contain the following properties:
  - `"name"`: the name of the subset. (This should be different from the dataset that it is subsetting;
    otherwise, it will overwrite it.)<br />
    This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - `"filter"`: the subset filtering definition, which will contain the following:
    - `"column"`: the column from the dataset to filter on.
    - `"operator"` how to compare the values from the column with the filter's value.
      Available options are:
      - `"="` or `"=="` equals (the default)
      - `"!="` or `"<>"` not equals
      - `"<"` less than
      - `"<="` less than or equal to
      - `">"` greater than
      - `">="` great than or equal to
    - `"values"`: an array of values to filter the column on. These can be numbers, strings, or dates
         (depending on the column's data type).<br />
         Multiple values will be an OR operation, where if a value in the data matches *any* of these
         values, then it's a match.
         Note that string values can reference constants loaded from the ["constants"](#constants-properties) section
         or `"formulas"` section of the parent dataset.
  - `"filter-or"` same as `"filter"`, except that it takes an array of subset filtering definitions. These criteria
    are ORed together, meaning that if any condition for a row is true, then it will be included in the subset.
  - `"filter-and"` same as `"filter"`, except that it takes an array of subset filtering definitions. These criteria
    are ANDed together, meaning that all conditions for a row must be met for it to be included in the subset.

  Next, the `"merges"` section of the dataset's node is parsed. This is an array of pivot specifications which
  contain the following properties:
  - `"name"`: the name of the merged dataset. (This should be different from the datasets being merged;
    otherwise, it will overwrite them.)<br />
    This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - `"type"`: a string specifying which type of merge to use.
    Available options are:
    - "left-join-unique"` (unique) left join the dataset with another dataset (the default).
  - `"other-dataset"`: the other dataset to merge the current dataset with. This should be a name referencing
    a previously loaded dataset.
  - `"by"`: an array of properties specifying which columns to join by. Each item should contain the following:
    - `"left-column"`: the column from the current dataset.
    - `"right-column"`: the column from the other dataset.
  - `"suffix"` if columns in the right dataset already appear in the left dataset, then append this suffix to
    to column to make it unique.

  Finally, the `"pivots"` section of the dataset's node is parsed. This is an array of pivot specifications which
  contain the following properties:
  - `"name"`: the name of the pivoted dataset. (This should be different from the dataset that it is pivoting;
    otherwise, it will overwrite it.)<br />
    This name is referenced by items (e.g., plots) elsewhere in the project file and must be unique.
  - `"type"`: a string specifying which type of pivot to use.
    Available options are:
    - `"wider"` pivot wider (the default).
    - `"longer"` pivot longer.

  If pivoting wider, the following options are available:
  - `"id-columns"`: an array of strings representing the ID columns.
  - `"names-from-column"`: a strings representing the 'names from'.
  - `"values-from-columns"`: an array of strings representing the 'values from' columns.
  - `"names-separator"` If multiple value columns are provided, then this separator will
       join the label from `namesFromColumn` and the value column name.
  - `"names-prefix"` string to prepend to newly created pivot columns.
  - `"fill-value"` numeric value to assign to missing data. The default it so leave it as missing data.

  If pivoting longer, the following options are available:
  - `"columns-to-keep"`: an array of strings specifying the columns to not pivot. These will be copied to the new dataset
    and will have their values filled in all new rows created from their observation.
    These would usually the ID columns.<br />
    These columns can be of any type, including the ID column.
  - `"from-columns"`: an array of strings specifying the continuous column(s) to pivot into longer format.
  - `"names-to"`: an array of strings specifying the target column(s) to move the names from the `"from-columns"` into.
  - `"values-to"`: a string specifying the column to move the values from the `"from-columns"` into.
  - `"names-pattern"`: an optional string specifying a regular expression to split the `"from-columns"` names into.

  Note that subset and pivot nodes can contain their own transformation and formula sections,
  same as a dataset node.

# Pages {#pages-properties}

A page is a grid-based container, where items (e.g., plots, labels) are layed out row-wise.<br />
The `"pages"` node will contain an array of definitions for all pages, each containing the following:

- `"name"` contains a string value, representing the name of the page.
- `"background-color"`: a string specifying the page's background color.
- `"background-image"`: a string specifying the path to an image to use for the page's background.
- `"page-numbering"` string specifying the numbering style that `PageNumber()` will return.
  Available options are:
  - `"arabic"`
  Note that this will also reset the current page number to `1`.
- `"rows"`: an array of rows, each containing items to draw on the page (layed out horizontally).<br />
  - `"items"`: an array of items in the current row.
    - `"type"`: the type of object that the item is (e.g., `Wisteria::Graphs::Table`, `Wisteria::Graphs::LinePlot`, etc.)<br />
      Available options are:
      - ["line-plot"](#line-plot-properties)
      - ["box-chart"](#box-plot-properties)
      - ["categorical-bar-chart"](#categorical-bar-chart-properties)
      - ["common-axis"](#common-axis-properties)
      - ["histogram"](#histogram-properties)
      - ["image"](#image-properties)
      - ["label"](#label-properties)
      - ["pie-chart"](#pie-chart-properties)
      - ["table"](#table-properties)
      - ["w-curve-plot"](#w-curve-plot-properties)
      - `null`: a `null` value will act as a placeholder for the previous column.<br />
        If an entire row contains nulls, then the previous row will consume that row
        (meaning it will grow that much in height).

# Canvas Items

## Axis {#axis-properties}

`"axes"` nodes can either be part of a graph node definition or a ["common-axis"](#common-axis-properties) node.<br />
  For the former, this can define properties for the various axes in the graph.<br />
  `"axes"` nodes contain an array of axis objects, each with the following properties:

- `"axis-type"`: the type of axis.<br />
  Available options are:
  - `"bottom-x"`
  - `"right-y"`
  - `"left-y"`
  - `"top-y"`
- `"title"`: the title of the axis, which contains ["label"](#label-properties) properties.
- `"axis-pen"`: the pen for the axis line, which includes [pen](#pen-properties) properties.
- `"gridline-pen"`: the pen for the gridlines, which includes [pen](#pen-properties) properties.
- `"double-sided-labels"`: boolean value specifying whether axis labels should be shown on both sides of the axis.
- `"tickmarks"`: tickmarks settings, which contains the following properties:
  - `"display"`: a string, indicating how to display the tickmarks.<br />
    Available options are:
    - `"inner"`
    - `"outer"`
    - `"crossed"`
    - `"no-display"`
- `"label-display"`: what to display for the labels along the axis.<br />
  Available options are:
  - `"custom-labels-or-values"`
  - `"only-custom-labels"`
  - `"custom-labels-and-values"`
  - `"no-display"`
- `"show-outer-labels"`: `true` to show the outer labels on the axis.
- `"label-length"`: a number specifying the suggested maximum length for the axis labels. (Default is 100.)
- `"label-length-auto"` if `true`, an attempt will be made to split longer axis labels based on
     various separators in them (e.g., parentheses, commas, conjunctions).

- `"brackets"`: adds brackets to the axis,which can be an array of bracket definitions or single set of properties
  using a dataset.<br />
  Available options for either method are:
  - `"simplify"`: a boolean value indicating whether to simplify the bracket's labels.
 
  If building brackets from a dataset, use the following properties:
  - `"dataset"`: the name of the dataset to read the columns from.<br />
       Note that this dataset can be different from the dataset used for the first
       child graph if you are wanting to use different labels. For this situation, the
       `"value"` variable should have the same scale as the child graph.
  - `"variables"` which include the following properties:
    - `"label"`: the labels column.
    - `"value"`: the values column. (This can be either a continous, categorical, or date column.)
  - `"pen"`: the bracket line, which includes [pen](#pen-properties) properties.
  - `"style"`: the style of the braces.<br />
    Available options are:
    - `"arrow"`
    - `"reverse-arrow"`
    - `"lines"`
    - `"curly-braces"`
    - `"no-connection-lines"`
  
  If building brackets individually, specify an area of items, each with the following properties:
  - `"start"`: the starting axis position for the area.<br />
       This can either be a numeric position, a date (as a string), or an axis label.
  - `"end"`: the ending axis position for the area.<br />
       This can either be a numeric position, a date (as a string), or an axis label.
  - `"label"`: the text to display on next to the brackets.
  - `"pen"`: the bracket line, which includes [pen](#pen-properties) properties.
  - `"style"`: the style of the braces.<br />
    Available options are:
    - `"arrow"`
    - `"reverse-arrow"`
    - `"lines"`
    - `"curly-braces"` (the default)
    - `"no-connection-lines"`

## Box Plot {#box-plot-properties}

Properties for `"box-plot"` nodes:

- `"variables"`: an item containing the following properties:
  - `"aggregate"`: the column containing the data.
  - `"group-1"`: a (optional) grouping column to divide the boxes into (along the X axis).
- `"box-effect"` string specifying the effects for the boxes.
  Available options are:
  - `"common-image"` (This will require an `"image-scheme"` to be defined.)
  - `"image" `(This will require an `"image-scheme"` to be defined.)
  - `"fade-from-bottom-to-top"`
  - `"fade-from-left-to-right"`
  - `"fade-from-right-to-left"`
  - `"fade-from-top-to-bottom"`
  - `"glassy"`
  - `"solid"` (the default)
  - `"stipple"`
- `"show-all-points"`: `true` to show all points. By default, only outliers are shown.
- `"show-labels"`: `true` to show slabels on the hinges, midpoint, and outliers.

## Categorical Bar Chart {#categorical-bar-chart-properties}

Properties for `"categorical-bar-chart"` nodes:

- `"variables"`: an item containing the following properties:
  - `"aggregate"`: the (optional) aggregate count column.<br />
       These are the values accumulated into the respective labels from the group column(s).<br />
       If this column is not provided, then frequency counts of the labels from the group column(s) are used.
  - `"category"`: the categorical (or ID) column containing the items to group and count.
  - `"group"`: a (optional) grouping column to further divide the main categorical data into.
- `"bar-orientation"` string specifying the orientation of the bars.<br />
  Available options are:
  - `"vertical"`
  - `"horizontal"`
- Properties available to [bar charts](#bar-chart-properties).
- Some base properties available to [graphs](#graph-properties).

## Common Axis {#common-axis-properties}

Properties for `"common-axis"` nodes:

- All properties available to [axis](#axis-properties) nodes are included.<br />
  Note that `"axis-type"` will specify where to place the common axis.
- `"child-ids"`: a numeric array of IDs of the graphs that this common axis will manage.<br />
     IDs are assigned to graphs using the `"id"` property, which should be numeric.
- `"common-perpendicular-axis"` if `"axis-type"` is `"bottom-x"`: and this is `true`,
     then the min and max of the graphs' left X axes will be used for the ranges of all the graphs.

## Fillable Shape {#fillable-shape-properties}

Properties for `"fillable-shape"` nodes:

- `"icon"`: a string specifying the icon to draw the shape as.
- `"pen"`: the pen to draw with, which includes [pen](#pen-properties) properties.
- `"brush"`: the brush to paint with, which includes [brush](#brush-properties) properties.
- `"size"`: the size of the shape, which contains the following properties:
  - `"width"`: the width.
  - `"height"`: the height.
- `"fill-percent"`: the percent to fill the shape, which is a number between `0.0` and `1.0`.<br />
  This can either be a numeric or a formula reference.

## Histogram {#histogram-properties}

Properties for `"histogram"` nodes:

- `"variables"`: an item containing the following properties:
  - `"aggregate"`: the continuous data column to aggregate.
  - `"group"`: the grouping column (this is optional).
- `"binning-method"`: string specifying how to sort the data.
  Available options are:
  - `"bin-by-integer-range"`
  - `"bin-by-range"`
  - `"bin-unique-values"`
- `"interval-display"` string specifying how to sort the data.
  Available options are:
  - `"cutpoints"`
  - `"midpoints"`
- `"rounding"`: string specifying how to round the data.
  Available options are:
  - `"no-rounding"`
  - `"round"`
  - `"round-down"`
  - `"round-up"`
- `"bins-start"`: number specifying where on the X axis to start the bins.
- `"suggested-bin-count"`: number specifying the suggested number of bins.
- `"max-bin-count"`: number specifying the maximumn number of bins.
- Properties available to [bar charts](#bar-chart-properties).
- Some base properties available to [graphs](#graph-properties).

## Image {#image-properties}

Properties for `"image"` nodes:

- `"path"`: the file path of the image to load.
- `"resize-method"`: How the image's size is adjusted when its boudning box is changed.<br />
  Available options are:
  - `"downscale-only"`
  - `"downscale-or-upscale"`
  - `"upscale-only"`
  - `"no-resize"`

## Label {#label-properties}

Properties for `"label"` nodes:

- `"text"`: the title's text.<br />
  Note that this property supports embedded formulas that can reference user-defined values loaded from the
  ["constants"](#constants-properties) section.
- `"orientation"` string specifying the orientation of the text.<br />
  Available options are:
  - `"vertical"`
  - `"horizontal"`
- `"background"`: the background color. This can be either a color name or hex-encoded value.
- `"color"`: the font color. This can be either a color name or hex-encoded value.
- `"bold"`: `true` to make the text bold.
- `"line-spacing"`: a number representing the spaces between lines (if text is multiline).
- `"text-alignment"`: how to align the label's text.<br />
  The available options are:
  - `"flush-left"` or `"ragged-right"`
  - `"flush-right"` or `"ragged-left"`
  - `"centered"`
  - `"justified"` or `"justified-at-character"`
  - `"justified-at-word"`
- `"left-image"`: properties specifying an image to display on the left side of the label.<br />
  The available options are:
  - `"path"`: string specifying the image path to load.
- `"top-image"`: properties specifying an image to display above the label.
  The available options are:
  - `"path"`: string specifying the image path to load.
  - `"offset"`: number specifying how far from the top of the label's box to draw the image. (Default is `0`.)
- `"header"`: attributes to apply to the first row of the label.<br />
     The following sub-properties are available:
  - `"bold"`: `true` to make the header bold.
  - `"color"`: the font color for the header. This can be either a color name or hex-encoded value.
  - `"relative-scaling:"` numeric value of how much to scale the header's font size relative to the label's scaling.
       For example, `2.0` will double the header's default font size compared to the rest of the label.<br />
       Note that this will only affect the header scaling. To alter the label's scaling, use the label's root-level
       `"scaling"` property.

Note that it is recommended to set `"fit-row-to-content"` to `true` if the label is a separate
object on the canvas (i.e., not a label on another object).

## Line Plot {#line-plot-properties}

Properties for `"line-plot"` nodes:

- `"variables"`: an item containing the following properties:
  - `"x"`: the X column.
  - `"y"`: the Y column.
  - `"group"`: the grouping column (this is optional).
- Some base properties available to [graphs](#graph-properties).

## Pie Chart {#pie-chart-properties}

Properties for `"pie-chart"` nodes:

- `"variables"`: an item containing the following properties:
  - `"aggregate"`: the (optional) aggregate count column.<br />
       These are the values accumulated into the respective labels from the group column(s).<br />
       If this column is not provided, then frequency counts of the labels from the group column(s) are used.
  - `"group-1"`: the inner-ring grouping column (or only ring, if `"group-2"` isn't used).
  - `"group-2"`: the outer-ring grouping column (this is optional).
- `"label-placement"`: a string specifying where to align the outer labels.<br />
  Available options are:
  - `"flush"` (the default)
  - `"next-to-parent"`
- `"inner-pie-midpoint-label-display"`: a string specifying what to display on the labels in the middle
  of the slices (within the inner pie).<br />
  Available options are:
  - `"value"`
  - `"percentage"` (the default)
  - `"value-and-percentage"`
  - `"bin-name"`
  - `"no-display"`
- `"outer-pie-midpoint-label-display"`: a string specifying what to display on the labels in the middle
  of the slices (within the outer pie, or pie if only one grouping variable is in use).<br />
  Available options are:
  - `"value"`
  - `"percentage"` (the default)
  - `"value-and-percentage"`
  - `"bin-name"`
  - `"no-display"`
- `"showcase-slices-groups"`: an array of strings specifying which inners slices to showcase,
  based on the parent group(s). All outer labelling will be turned off, and the showcased inner
  slices' labels will be shown. The parent slices' colors will be solid, like their children inner slices.
- `"showcase-slices"` draw attention to a slice (or series of slices).<br />
  Available options are:
  - `"pie"` which pie to showcase.<br />
    Available options are:
    - `"inner"`
    - `"outer"`
  - `"category"` which type of slices to showcase.<br />
    Available options are:
    - `"smallest"`
    - `"largest"`
  - `"by-group"`: if showcasing the inner pie, `true` will show the smallest/largest slice for within each group.
  - `"show-outer-pie-labels"` if showcasing the inner pie, `true` keep the outer pie labels shown.
- `"color-labels"`: `true` to apply the slice colors to their respective outer labels.
- `"ghost-opacity"`: number specifying the opacity for ghosted labels.
- `"include-outer-pie-labels"`: `true` to show the outer labels for the outer pie
     (or main pie, if only using one grouping variable).
- `"include-inner-pie-labels"`: `true` to show the outer labels for the inner pie
     (if a second grouping variable is in use).
- `"donut-hole"`: donut hole properties, which include:
  - `"proportion"`: a value between `0.0` and `0.95`, specifying how much of the pie the hole should consume.
  - `"label"`: the [label](#label-properties) shown in the middle of the hole.<br />
        Note that this label will be implicitly justified and centered within the hole.
  - `"color"`: the color of the donut hole.
- Some base properties available to [graphs](#graph-properties).

## Shape {#shape-properties}

Properties for `"shape"` nodes:

- `"icon"`: a string specifying the icon to draw the shape as.
- `"pen"`: the pen to draw with, which includes [pen](#pen-properties) properties.
- `"brush"`: the brush to paint with, which includes [brush](#brush-properties) properties.
- `"size"`: the size of the shape, which contains the following properties:
  - `"width"`: the width.
  - `"height"`: the height.

## Table {#table-properties}

Properties for `"table"` nodes:

- `"variables"`: an array of column names to use from the dataset.<br />
  These values can also be variable selection formulas, including the following:
  - ``{{matches(`value`)}}``: where `value` will be the regular expression search pattern, and any column
      in the dataset that contains this text will be included.
  - ``{{everything()}}``: will return all columns from the dataset. The column order will be the ID column (if in use),
      the categoricals, the continuous columns, and finally the date columns.
- `"transpose"`: `true` to transpose the data at the time of import. This means that the columns will become
     the rows and vice versa.<br />
- `"min-width-proportion"`: the minimum percent of the drawing area's width that the table should consume
     (between `0.0` to `1.0`, representing 0% to 100%).
- `"min-height-proportion"`: the minimum percent of the drawing area's height that the table should consume
     (between `0.0` to `1.0`, representing 0% to 100%).
- `"highlight-pen"`: the pen used for highlighting cells, which includes [pen](#pen-properties) properties.
- `"default-borders"`: an array of boolean values, representing whether the default borders of the
      cell should be drawn. These values go clockwise, starting at 12 o'clock.
- Some base properties available to [graphs](#graph-properties).

Note that it is recommended to set `"fit-row-to-content"` to `true` for shorter tables to better fit them to the page.
Likewise, set `"fit-row-to-content"` to `false` for taller tables that are meant to consume most of a page's height.

The remaining properties are executed in the following order:
- `"row-group"`: a numeric array representing which rows to apply label grouping to.<br />
     Across each provided row, this will combine consecutive cells with the same label into one cell.
- `"column-group"`: a numeric array representing which columns to apply label grouping to.<br />
     Down each provided column, this will combine consecutive cells with the same label into one cell.
- `"alternate-row-color"`: applies "zebra stripes" down the rows.
  Available options are:
  - `"color"`: the base row color for the starting row; a shaded/tinted version of this color will be applied
       to every other row.
  - `"start"`: the row to start the alternating from. (Default is `0`.)
  - `"stops"`: an array of which columns to skip over when applying the row alternating color.<br />
       This is an array of `"position"` items.
- `"row-add"`: commands a series of rows to be added, which is an array of row properties containing the following:
  - `"position"`: where to insert the row.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"constants"`: an array of strings to fill the row with (left-to-right).
  - `"background"`: the background color of the row.
- `"row-color"`: an array of row and color pairs, which contain the following properties:
  - `"position"`: which row to apply a background color.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"background"`: the background color of the row.
  - `"stops"`: an array of which columns to skip over when changing the row's color.<br />
       This is an array of `"position"` items.
- `"row-bold"`: an array of rows, which contain the following properties:
  - `"position"` which row to make bold.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"stops"`: an array of which columns to skip over when bolding the row.<br />
       This is an array of `"position"` items.
- `"row-borders"`: an array of row border specifications, which contain the following:
  - `"position"` which row to edit.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"borders"`: an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - `"right-border"` boolean specifying whether to show the cell's right border.
  - `"top-border"` boolean specifying whether to show the cell's top border.
  - `"bottom-border"` boolean specifying whether to show the cell's bottom border.
  - `"left-border"` boolean specifying whether to show the cell's left border.
  - `"stops"`: an array of which rows to skip over when changing the row's borders.<br />
       This is an array of `"position"` items.
- `"row-content-align"`: an array of commands to align the content inside of the cells across a row.
  - `"position"` which row to change the content alignment in.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"horizontal-page-alignment"` how to horizontally align the cells' content.<br />
    Available options are:
    - `"left-aligned"`
    - `"right-aligned"`
    - `"centered"`
  - `"stops"`: an array of which columns to skip over when changing the column's content alignment.<br />
       This is an array of `"position"` items.
- `"column-color"`: an array of column and color pairs, which contain the following properties:
  - `"position"` which column to apply a background color.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"background"`: the background color of the column.
  - `"stops"`: an array of which rows to skip over when changing the column's color.<br />
       This is an array of `"position"` items.
- `"column-bold"`: an array of columns, which contain the following properties:
  - `"position"` which column to make bold.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"stops"`: an array of which rows to skip over when making the column bold.<br />
       This is an array of `"position"` items.
- `"column-borders"`: an array of column border specifications, which contain the following:
  - `"position"` which column to edit.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"borders"`: an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - `"right-border"` boolean specifying whether to show the cell's right border.
  - `"top-border"` boolean specifying whether to show the cell's top border.
  - `"bottom-border"` boolean specifying whether to show the cell's bottom border.
  - `"left-border"` boolean specifying whether to show the cell's left border.
  - `"stops"`: an array of which rows to skip over when changing the column's borders.<br />
       This is an array of `"position"` items.
- `"column-highlight"`: an array of column highlight specifications, which contain the following:
  - `"position"` which column to edit.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"stops"`: an array of which rows to skip over when changing the column's cell highlighting.<br />
       This is an array of `"position"` items.
- `"aggregates"`: an array of aggregate definitions that will be added to the table.<br />
  Each aggregate node contains the following properties:
  - `"type"` where to add the aggregate.<br />
    Available options are:
    - `"row"`
    - `"column"`
  - `"aggregate-type"`: the type of aggregate to add.<br />
    Available options are:
    - `"percent-change"`
    - `"total"`
  - `"background"`: the background color for the column/row. This can be either a color name or hex-encoded value.
  - `"use-adjacent-color"`: `true` to use the color of the cell adjacent to this column.
       `false` will apply a light gray to the column.
  - `"name"`: the name for the newly added aggregate column/row.
  - `"start"`: the first column/row that the aggregate column/row should use.<br />
       This property is optional, and if not included the first numeric column/row encountered will be used.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"end"`: the last column/row that the aggregate column/row should use.<br />
       This property is optional, and if not included the last numeric column/row encountered will be used.
       Like `"start"`, this is optional and has the same properties.
  - `"borders"`: an array of boolean values, representing whether the borders of the
      cell should be drawn. These values go clockwise, starting at 12 o'clock.<br />
      This is optional and overrides the default cell borders.
- `"row-totals"`: adds a grand total row to the bottom of the table.<br />
  If the first column is grouped and the second colum is text, then this will also insert subtotal rows.<br />
  Available options are:
  - `"background"`: the background color for the row(s).
- `"cell-update"`: an array of cell updating commands, which contain the following properties:
  - `"column"`: the column position of the cell to update.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"row"`: the row position of the cell to update.<br />
       Refer to the [position](#position-properties) properties that are available.
  - `"value-to-find"`: if `"row"/"column"`: are not provided, this is a cell value to search for.
       The first cell that matches this text will be returned and that is the cell that will be updated.
  - `"column-count"`: the number of columns that this cell should consume.<br />
       This can either be a number, or the string `"all"` (meaning all columns).
  - `"row-count"`: the number of rows that this cell should consume.<br />
       This can either be a number, or the string `"all"` (meaning all rows).
  - `"value"`: a numeric, string, or date value to assign to the cell.
  - `"background"`: the background color. This can be either a color name or hex-encoded value.
  - `"bold"`: `true` to make the cell bold.
  - `"highlight"`: `true` to highlight the cell.
  - `"prefix"`: a character to display on the left side of the cell.
  - `"left-image"`: properties specifying an image to display on the left side of the label.
    The available options are:
    - `"path"`: string specifying the image path to load.
  - `"show-borders"`: an array of boolean values, representing whether the borders of the
       cell should be drawn. These values go clockwise, starting at 12 o'clock.
  - `"horizontal-page-alignment"`: how to horizontally align the item within its area.<br />
    Available options are:
    - `"left-aligned"`
    - `"right-aligned"`
    - `"centered"`
  - `"text-alignment"` how to align the cell's text (if multi-line).<br />
    The available options are:
    - `"flush-left"`or `"ragged-right"`
    - `"flush-right"`or `"ragged-left"`
    - `"centered"`
    - `"justified"`
- `"cell-annotations"`: an array of cell annotation specifications, which each contain the following properties:
  - `"value"`
  - `"side"`
  - `"pen"`
  - `"background"`
  - `"cells"`
    The available options are:
    - `"column-outliers"`
    - `"column-top-n"`
      The available options are:
      - `"n"`
- `"footnotes"`: an array of footnote specifications, which each contain the following properties:
  - `"value"`: the cell value (as a string) to add a footnote number to. Can include ["constants"](#constants-properties).
  - `"footnote"`: the footnote to add to the caption. Can include ["constants"](#constants-properties).

## W-Curve Plot {#w-curve-plot-properties}

Properties for `"w-curve-plot"` nodes:

- `"variables"`: an item containing the following properties:
  - `"x"`: the X column.
  - `"y"`: the Y column.
  - `"group"`: the grouping column (this is optional).
- `"time-interval-label"` string the label for the major time intervals used in the data collection (e.g., `"semester"` or `"year"`).
- Some base properties available to [graphs](#graph-properties).

# Base-level Properties

## Positions {#position-properties}

Properties for row or column positions:

- `"origin"`: this is either the zero-based index of the row/column, or a string.<br />
     The string values available are `"last-row"` or `"last-column"`,
     which will be interpreted as the last row or column in the data, respectively.<br />
     Column names from the dataset can also be used if looking for a column position.
- `"offset"`: a numeric value combined with the value for `"origin"`.<br />
     This is optional and is useful for when `"origin"` is interpreted at runtime.<br />
     For example, if `origin` is `"last-row"`: and `offset` is `-1`, then this will
     result in the second-to-last row.

A position can also be a number if not needing to use the `"origin"`/`"offset"` features.

## Pen {#pen-properties}

Properties for pens:

- `"color"`: the pen color. This can be either a color name or hex-encoded value.
- `"width"`: the width of the pen's line.
- `"style"`: the pen's style, which includes the following:
  - `"dot"`
  - `"dot-dash"`
  - `"long-dot"`
  - `"short-dot"`
  - `"solid"` (the default)
  - `"cross-hatch"`
  - `"horizontal-hatch"`
  - `"vertical-hatch"`

Note that a pen can be set to `null` to turn it off.

## Brush {#brush-properties}

Properties for brushes:

- `"color"`: the brush color. This can be either a color name or hex-encoded value.
- `"style"`: the brush's pattern, which includes the following:
  - `"backwards-diagonal-hatch"`
  - `"forward-diagonal-hatch"`
  - `"cross-diagonal-hatch"`
  - `"solid"` (the default)
  - `"cross-hatch"`
  - `"horizontal-hatch"`
  - `"vertical-hatch"`

Note that brushes can be set to `null` to turn it off.

## Bar Chart {#bar-chart-properties}

Properties common to all bar-chart derived graphs:

- `"box-effect"`: string specifying the effects for the bars.
  Available options are:
  - `"common-image"` (This will require an `"image-scheme"` to be defined.)
  - `"fade-from-bottom-to-top"`
  - `"fade-from-left-to-right"`
  - `"fade-from-right-to-left"`
  - `"fade-from-top-to-bottom"`
  - `"glassy"`
  - `"solid"` (the default)
  - `"stipple"`
- `"bar-label-display"`: string specifying how to display labels on the bars.
  Available options are:
  - `"percentage"`
  - `"value"`
  - `"value-and-percentage"`
  - `"no-display"`
  - `"bin-name"`
- `"include-spaces-between-bars"`: `true` to include spaces between bars.
- `"ghost-opacity"`: number specifying the opacity level of non-showcased bars.
- `"showcase-bars"`: an array of strings specifying which bars to showcase.
- `"bar-sort"`: properties specifying how to sort the bars (after loading the data).
  Available options are:
  - `"direction"`
    Available options are:
    - `"ascending"`
    - `"descending"`
  - `"by"` how to sort the bars, by label or length. This will override any `"labels"` option.
    Available options are:
    - `"length"`
    - `"label"`
  - `"labels"`: the bar labels, specified in the order they should be sorted.<br />
       If `"direction"` is `"ascending"`, then the bars will be sorted top-to-bottom or
       left-to-right (based on the order of the provided labels). Otherwise, they will be
       sorted bottom-to-top or right-to-left.<br />
       Note that this option is ignored if `"by"` is specified.
- `"bar-groups"`: an array of bar groups, each containing the following properties:
  - `"start"`: the axis label or numeric position to start the group.
  - `"end"`: the axis label or numeric position to end the group.
  - `"decal"`: a label to display on the group bar.
  - `"brush"`: the brush to paint with, which includes [brush](#brush-properties) properties.
  - `"color"`: the base color to paint under the brush. This is useful if the brush is using a hatch pattern.
- `"decals"`: an array of decals to add to the bars. This will be an array of nodes containing the following properties:
  - `"bar"`: the axis label of the bar.
  - `"block"`: number specifying which block in the bar to add the decal to. If not specified, defaults to `0`.
  - `"decal"`: the decal to add to the bar, which will contain ["label"](#label-properties) properties.
- `"first-bar-brackets"`: properties adding axis brackets that reference the first bar.
  Available options are:
  - `"label"`: string for the bracket's label.
  - `"start-block-re"`: a regular expression used to find the starting block for the bracket.
  - `"end-block-re"`: a regular expression used to find the ending block for the bracket.

  If the `"re"` variations of the starting and ending labels are available, then these properties are used:
  - `"start-block"`: string specifying the tag of the starting block of the bar.
  - `"end-block"`: string specifying the tag of the ending block of the bar.
- `"constrain-scaling-axis-to-bars"`: `true` to force the scaling axis to the longest bar.

Note that bar sorting is performed prior to adding bar groups. When specifying the start and end of your bar groups,
any bar sorting that you specified will be in effect. Likewise, axis brackets are also added after the bars are sorted;
if specifying start and end points for your brackets, be aware that the bars will already be sorted.

## Graphs {#graph-properties}

Properties common to all graph items:

- `"dataset"`: if the object requires a dataset (most graphs do), then this is the name of the dataset.<br />
     Note that this is the unique name of the dataset loaded from the report's
     ["datasets"](#datasets-properties) section, not a filepath.<br />
     Also, this is optional if the item type doesn't require a dataset (e.g., a `Wisteria::GraphItems::Label`).
- `"title"`: the title of the graph, which contains ["label"](#label-properties) properties.
- `"sub-title"`: the subtitle of the graph, which contains ["label"](#label-properties) properties.
- `"caption"`: the caption of the graph, which contains ["label"](#label-properties) properties.
- `"axes"`: an array of [axis](#axis-properties) objects.
- `"background-color"`: a string specifying the plotting area's background color.
- `"image-scheme"`: an array of image filepaths to use for the image scheme.
- `"common-box-image-outline"`: string specifying the color for the outline around the bars/boxes
     if using a common box image.
- `"stiple-image"`: a string specifying the path to an image to use a stipple brush.
- `"annotations"`: text messages that can be added on top of the graph, optionally pointing to positions
  on the graph. This is an array of annotation definitions, which contain the following properties:
  - `"label"`: the label for the note, , which contains ["label"](#label-properties) properties.
  - `"interest-points"`: The points that the note will point at.<br />
    This is an array of items which contain the following properties:
    - `"x"`: the X axis position of the point.
    - `"y"`: the Y axis position of the point.
  - `"anchor"`: the point to anchor the label.<br />
     Note that the `label`'s `"anchoring"` property will control which part of the label that the
     anchoring point refers to.<br />
     If not specified, the graph will attempt to place it at the best location.<br />
     This contains the following properties:
     - `"x"`: the X axis position of the point.
     - `"y"`: the Y axis position of the point.
- `"reference-lines"`: an array of reference line definitions, which contain the following properties:
  - `"axis-type"`: the axis to add the reference line to.<br />
    Available options are:
    - `"bottom-x"`
    - `"right-y"`
    - `"left-y"`
    - `"top-y"`
  - `"position"`: the axis position for the line.<br />
       This can either be a numeric position, a date (as a string), or an axis label.
  - `"label"`: the text to display on the legend referring to line.
  - `"pen"`: the definition for the [pen](#pen-properties) properties.
- `"reference-areas"`: an array of reference area definitions, which contain the following properties:
  - `"axis-type"`: the axis to add the reference area to.<br />
    Available options are:
    - `"bottom-x"`
    - `"right-y"`
    - `"left-y"`
    - `"top-y"`
  - `"start"`: the starting axis position for the area.<br />
       This can either be a numeric position, a date (as a string), or an axis label.
  - `"end"`: the ending axis position for the area.<br />
       This can either be a numeric position, a date (as a string), or an axis label.
  - `"label"`: the text to display on the legend referring to area.
  - `"pen"`: the definition for the [pen](#pen-properties) properties.<br />
        The color is used for the surrounding reference lines, and a tinted version
        is used the area.
  - `"style"`: the style to apply to the reference area.<br />
    Available options are:
    - `"fade-from-left-to-right"`
    - `"fade-from-right-to-left"`
    - `"solid"`
- `"brush-scheme"` for graphs that support brush schemes only.<br />
     Contains the following properties:
     `"brush-styles"`: an array of strings specifying the [brush styles](#brush-properties).
     `"color-scheme"`: a color scheme that is mapped to the brush styles.
- `"color-scheme"` for graphs that support color schemes only.<br />
     This can either be an array of color strings or the name of the color scheme.<br />
     Refer to `Wisteria::Colors::Schemes` for a list of color schemes.
- `"icon-scheme"` for graphs that support icon/marker schemes only.<br />
  This an either be an icon scheme name or an array of icon strings (which can be recycled)<br />
  Refer to `Wisteria::Icons::Schemes` for a list of icon schemes.<br />
  The following individual icons are available:
  - `"blank"`
  - `"horizontal-line"`
  - `"arrow-right"`
  - `"circle"`
  - `"image"`
  - `"horizontal-separator"`
  - `"horizontal-arrow-right-separator"`
  - `"color-gradient"`
  - `"square"`
  - `"triangle-upward"`
  - `"triangle-downward"`
  - `"triangle-right"`
  - `"triangle-left"`
  - `"diamond"`
  - `"plus"`
  - `"asterisk"`
  - `"hexagon"`
  - `"box-plot"`
  - `"location-marker"`
  - `"go-road-sign"`
  - `"warning-road-sign"`
  - `"sun"`
  - `"flower"`
  - `"fall-leaf"`
  - `"top-curly-brace"`
  - `"right-curly-brace"`
  - `"bottom-curly-brace"`
  - `"left-curly-brace"`
  - `"male"`
  - `"female"`
  - `"female-business"`
- `"line-scheme"` for graphs that support line schemes only.<br />
  This is an array of line specifications (which can be recycled)<br />
  Each specification contains the following:
  - `"line-style"`: a string specifying the line style.<br />
    Available options are:
    - `"lines"`
    - `"arrows"`
    - `"spline"`
  - `"pen-style"`: a [pen](#pen-properties) specificaiton (although only the pen style is used)
- `"legend"`: an item containing the following properties:
  - `"placement"` where to place the legend.<br />
    Available options are:
    - `"left"`
    - `"right"` (the default)
    - `"top"`
    - `"bottom"`
  - `"include-header"` `true` to include a header (the default), `false` to not.
  - `"title"` if `"include-header"` is `true`, a string specifying the title of the legend.
  - `"ring"` for pie charts, which ring to use for the legend.<br />
    Available options are:
    - `"outer"` only the outer (or only) ring is included. (This is the default.)
    - `"inner"`: the inner and outer rings are included in the legend.

## All Items {#item-properties}

Properties common to all items:

- `"id"`: a (unique) numeric identifier for the item. This can be referenced by other items in the report.<br />
     An example can be adding an ID to a graph and then inserted a ["common-axis"](#common-axis-properties) to the
     report that accepts this ID.
- `"canvas-margins"`: a numeric array (one to four numbers), representing the item's canvas margin going clockwise,
     starting at 12 o'clock.
- `"padding"`: a numeric array (one to four numbers), representing the item's padding going clockwise,
     starting at 12 o'clock.
- `"pen"`: the item's pen, which includes [pen](#pen-properties) properties.
- `"outline"`: a boolean array, specifying the item's border outlines to draw going clockwise,
     starting at 12 o'clock. Note that `"pen"` must be valid and that this is not supported by all objects.
- `"scaling"` numeric value of how much to scale the object's size. For example, `2.0` will double the
     its default size.
- `"show"` boolean value specifying whether to show the item.
- `"anchoring"` controls the starting point of where the object is drawn.<br />
  This is usually only relative for objects being embedded on a plot (e.g., annotations).<br />
  Available options are:
  - `"bottom-left-corner"`
  - `"bottom-right-corner"`
  - `"center"` (the default)
  - `"top-left-corner"`
  - `"top-right-corner"`
- `"relative-alignment"` how an object is aligned with its parent (e.g., an axis title relative to its axis).<br />
  Available options are:
  - `"flush-left"`
  - `"flush-right"`
  - `"flush-top"`
  - `"flush-bottom"`
  - `"centered"`
- `"horizontal-page-alignment"` how to horizontally align the item within its area.<br />
  Available options are:
  - `"left-aligned"`
  - `"right-aligned"`
  - `"centered"`
- `"vertical-page-alignment"` how to vertically align the item within its area.<br />
  Available options are:
  - `"top-aligned"`
  - `"bottom-aligned"`
  - `"centered"`
- `"fit-row-to-content"`: a boolean value specifying whether the item's calculated height should
     control how tall its canvas row is.
- `"fixed-width"`: a boolean value specifying whether the item should be constrained
     to its calculated width within its row.

# Additional Functions {#additional-functions}

Along with the dataset-related functions that can be used for dataset formulas, other functions are also available.
These functions can be used in a dataset's formulas section, the `"constants"` section, and even in-place (e.g., in a label's text value).

- ``ReportName()``: the report's name.
- ``PageNumber()``: the current page number.
- ``Now(`value`)``:
    Returns the a string representing the current date and time (or date component).
    - `value`: the part of today's date to return (if not provided, then the full date is returned).
      The following options are available:
      - `"Year"`: the year.
      - `"Day"`: the day of the month.
      - `"DayName"`: the day of the month as a name (e.g., Tuesday).
      - `"Month"`: the numeric value of the month.
      - `"MonthName"`: the name of the month.
      - `"MonthShortName"`: the abbreviated name of the month.
      - `"Fancy"` returns the date formatted as full month name, day, and four-digit year.

# Notes {#notes}

Color values can either be hex encoded (e.g., `"#FF0000"` for red) or a named value (`"pumpkin"`). For a full list
of color names available, refer to the `Wisteria::Colors::Color` enumeration. Specifying `null` for a color
will result in a transparent color.

Constants defined in the ["constants"](#constants-properties) can also be referenced, which likewise can
be named colors or hex encoded values.