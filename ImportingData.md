Importing Data
=============================

A `Wisteria::Data::Dataset` object is available for importing data, as well as connecting the data to
various graph types. When you import data, you can select which columns to use and specify
which ones are which. For example, you would specify which column would be the Y data, another
the X data, another a grouping column, etc. Then you would pass the dataset to a graph's
`SetData()` method and it will use the columns that it expects.

Importing
=============================
Datasets can be imported from tab- or comma-delimited text files via `Wisteria::Data::Dataset::ImportCsv()` or
`Wisteria::Data::Dataset::ImportTsv()`. During this import, you indicate which columns to include and how
they should be used by your graphs. This variable selection is done through the `Wisteria::Data::ImportInfo` class,
which include chainable calls which classify the columns. For example:

```cpp
auto BelongingData = std::make_shared<Data::Dataset>();
BelongingData->ImportCSV(L"Sense of Belonging.csv",
    ImportInfo().
    // double value columns
    ContinuousColumns({ L"Belong", L"Year" }).
    // categorical columns
    CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
```

In the above example, a file named "Sense of Belonging.csv" will be imported. The importer will search for columns named
"Year" and "Belong" (case insensitively) and will load them as continuous data. Next, the column "Name" will be
imported as the categorical (i.e., grouping) column.

Continuous Data
=============================

Columns imported as continuous will be read as double values using `wxString::ToCDouble()`. This means that the data
should be in C locale (i.e., US format), where '.' is the radix separator.

Although data is imported and stored as floating point values, discrete/integer values can also be read into these columns.

Missing data in a continuous column will be imported as NaN (@c std::numeric_limits<double>::quiet_NaN(), so @c std::isnan()
should be used when working with imported data.

Categorical and Grouping Data
=============================

For the grouping/categorical columns, you can control how these are read. They can either
be read as integer codes (which should start at 1 for most graphs), or read as strings. This allows
for importing a grouping column in either of these formats:

| GENDER |
| --:    |
| 1      |
| &nbsp; |
| 2      |
| 2      |
| 2      |
| 1      |
| 2      |
| 1      |

or

| GENDER |
| --:    |
| M      |
| &nbsp; |
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

Missing data in a column will be coded as '0'. If using integer codes in your data, ensure that '0' is not used or
only used to represent missing data. Also, if setting a string table to your column, be sure that the string connected
'0' relates to missing data (e.g., "Unknown", "No response", etc.).

You can import multiple categorical columns, where you can specify each one's name and how to import it.

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

Date Data
=============================

Date column importing is highly configurable. The default is to use a combination of `wxString::ParseDateTime()`
and `wxString::ParseDate()`, which will make an "educated guess" to determine the date string's format.
You can, however, have more granularity over the date input formats. The following demonstrates this:

```cpp
auto surveyData = std::make_shared<Data::Dataset>();
companyAcquisitionData->ImportCSV(L"datasets/Company Acquisition.csv",
    ImportInfo().
    ContinuousColumns({ L"Completion" }).
    // The default DateImportMethod::Automatic would work,
    // but the following demonstrates how you can customize
    // date importing.
    DateColumns({
        // import dates like "1979-10-31"
        { L"Start", DateImportMethod::IsoDate },
        // import dates like "10/31/1979"
        { L"End", DateImportMethod::StrptimeFormatString, L"%m/%d/%Y" }
        });
```

Missing data in a date column are imported as wxInvalidDateTime, so `wxDateTime::IsValid()` should be called when
working with imported values. Also, any parsing errors (from malformed input) while imporing dates are logged
(via `wxLogWarning()`).

Using the Data
=============================

After importing a dataset, you then pass it to a graph's `SetData()` function and specify which columns to use.
For example, a `Wisteria::Graphs::BoxPlot` will use a dataset's continuous column (and optionally a group column).
A `Wisteria::Graphs::WCurvePlot` will, on the other hand, requires two continuous columns and a group column.

Building a Dataset
=============================

To build a dataset manually, please refer to the [dataset building overview](BuildingData.md).