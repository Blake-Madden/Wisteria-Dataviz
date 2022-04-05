Importing Data {#importing-data}
=============================
[TOC]

A `Wisteria::Data::Dataset` object is available for importing data, as well as connecting the data to
various graph types. When you import data, you can select which columns to use and specify
which ones are which. For example, you would specify which column would be the Y data, another
the X data, another a grouping column, etc. Then you would pass the dataset to a graph's
`SetData()` method and it will use the columns that it expects.

Importing
=============================
Datasets can be imported from tab- or comma-delimited text files via `Wisteria::Data::Dataset::ImportCSV()` or
`Wisteria::Data::Dataset::ImportTSV()`. During this import, you indicate which columns to include and how
they should be used by your graphs. This variable selection is done through the `Wisteria::Data::ImportInfo` class,
which include chainable calls which classify the columns. For example:

```cpp
auto BelongingData = std::make_shared<Data::Dataset>();
BelongingData->ImportCSV(L"Sense of Belonging.csv",
    ImportInfo().
    // Y column, then X column
    ContinuousColumns({ L"Belong", L"Year" }).
    CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
```

In the above example, a file names "Sense of Belonging.csv" is imported. The importer will search for
a column named "Year" (case insensitively) and specify that column as the X data. The column named
"Belong" will be imported as the Y data, and "Name" will be imported as the grouping column.

Categorical Columns
=============================

A special note about categorical columns is that you can import multiple ones. When importing categoricals,
you specify each one's name and how to import it.

In the following example, we import seven categorical columns that are read as integer codes.
We also read in one categorical (`Gender`) as strings (e.g., "Male", "Female").

```cpp
auto surveyData = std::make_shared<Data::Dataset>();
surveyData->ImportCSV(L"Graph Library Survey.csv",
    Data::ImportInfo().
    CategoricalColumns(
        {
        { L"GENDER" },
        { L"I am happy with my current graphics library",
            CategoricalImportMethod::ReadAsIntegers },
        { L"Customization is important to me",
            CategoricalImportMethod::ReadAsIntegers },
        { L"A simple API is important to me",
            CategoricalImportMethod::ReadAsIntegers },
        { L"Support for obscure graphs is important to me",
            CategoricalImportMethod::ReadAsIntegers },
        { L"Extensibility is important to me",
            CategoricalImportMethod::ReadAsIntegers },
        { LR"(Standard, "out-of-the-box" graph support is important to me)",
            CategoricalImportMethod::ReadAsIntegers },
        { L"Data importing features are important to me",
            CategoricalImportMethod::ReadAsIntegers }
        }) );
```

Categorical and Grouping Data
=============================

The X and Y columns in a dataset are imported as double values, and the label column as strings.
For the grouping and categorical columns, however, you can control how these are read. They can either
be read as integer codes (which should start at 0 or 1 for most graphs), or read as strings. This allows
for importing a grouping column in either of these formats:

| GENDER |
| --:    |
| 0      |
| 1      |
| 1      |
| 1      |
| 1      |
| 0      |
| 1      |
| 0      |

or

| GENDER |
| --:    |
| M      |
| F      |
| F      |
| F      |
| F      |
| M      |
| F      |
| M      |

Rather than storing a large amount of (possibly redundant) strings, these columns store their data as integers
and use a lookup table to retrieve the strings that a graph may need. Doing this provides optimization and a
smaller memory footprint.

In the dataset's import functions, a `Wisteria::Data::ImportInfo()` is used to specify which columns to import. Here
you can indicate which columns to import as a grouping/categorical (via `CategoricalColumns()`).
It's in these functions that you control how to import these columns with a `CategoricalImportMethod` value.
(The above code example demonstrates this.)

If reading the columns as strings, then the importer will internally assign integral codes to the strings
in the order that they appear in the file. Otherwise, if the file contains integers, then those codes will
be imported and used. If you are importing as integers and want strings to be assigned to your codes,
this can done by accessing the column's string table, like so:

```cpp
yData->GetCategoricalColumn(0).GetStringTable() =
    { { 0, L"MALE" }, { 1, L"FEMALE" } };
```

Note that this step is optional, as some graphs only require integer codes for these columns (e.g., Likert charts).

Using the Data
=============================

After importing a dataset, you pass it to a graph to use. Graphs are pre-configured to use specific
columns from a dataset, so you only need to call a graph's `SetData()` function and it will handle the rest.
For example, a `Wisteria::Graphs::BoxPlot` will use a dataset's first continuous column (and optionally its group column); all other
columns in the dataset are ignored. A `Wisteria::Graphs::WCurvePlot` will, on the other hand, requires two
continuous columns and a group column.

Building a Dataset
=============================

To build a dataset manually, please refer to the [dataset building overview](BuildingData.md).