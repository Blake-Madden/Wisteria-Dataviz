Transforming Data
=============================

Recoding Data
=============================

Text columns can have regular expression replacements applied to them during import. The `ImportInfo` parameter
to the various `ImportXXX()` methods accepts a map of regex pattern and their respective replacements, which
is performed on each categorical column as the data is read.

This can be useful for recoding values to missing data (e.g., "N/A") or correcting misspellings.
The following demonstrates this feature:

```cpp
auto commentsData = std::make_shared<Data::Dataset>();
commentsData->ImportCSV(L"/home/rdoyle/data/Comments.csv",
    Data::ImportInfo().CategoricalColumns({
        { L"Comments", CategoricalImportMethod::ReadAsStrings }
        }).
    ReplacementStrings({
        // replace cells that contain only something like
        // 'NA' or 'n/a'
        { std::make_shared<wxRegEx>(L"^[nN][/]?[aA]$"), L"" },
        // replace 'foot ball' with 'football'
        { std::make_shared<wxRegEx>(L"(?i)foot ball"), L"football" }
        }));
```

These regex replacements can also be loaded from another file:

```cpp
// file that contains the regex patterns to replace
// and what to replace them with
auto replacementsData = std::make_shared<Data::Dataset>();
replacementsData->ImportCSV(L"/home/dmoon/data/replacements.csv",
    Data::ImportInfo().CategoricalColumns({
        { L"pattern", CategoricalImportMethod::ReadAsStrings },
        { L"replacement", CategoricalImportMethod::ReadAsStrings }
        }));

// load a file with a column of text and transform it
auto commentsData = std::make_shared<Data::Dataset>();
commentsData->ImportCSV(L"/home/dmoon/data/Comments.csv",
    Data::ImportInfo().CategoricalColumns({
        { L"Comments", CategoricalImportMethod::ReadAsStrings }
        }).
    ReplacementStrings(
        ImportInfo::DatasetToRegExMap(replacementsData, L"pattern", L"replacement")
        ));
```

Subsetting
=============================

A dataset can be subsetted using the `Wisteria::Data::Subset` class. This class's `Subset()`
method accepts a filter criterion based on a column, the value to filter with, and how to compare with
that value. For example, the following will create a subset where it filtered a dataset on the
column **Gender** equaling **Female**.

```cpp
auto theData = std::make_shared<Data::Dataset>();
theData->ImportCSV(L"/home/emma/datasets/Spelling Grades.csv",
    ImportInfo().
    ContinuousColumns({ L"AVG_GRADE"}).
    CategoricalColumns({
        { L"Gender" },
        { L"WEEK_NAME" }
        }));
Subset dsSubset;
// dataset with only female observations
const auto subset =
    dsSubset.SubsetSimple(theData,
        ColumnFilterInfo{ L"Gender", Comparison::Equals, L"Female" });
// "subset" can now be exported or plotted
```

Pivoting Longer
=============================

For situations where multiple continuous columns need to be combined into one, you can pivot (i.e., stack or cast) the dataset.
For example, if you have data like this:

| X   |  Y1   |  Y2 |
| --: | --:   | --: |
| 1   | 7     | 9   |
| 2   | 7.5   | 11  |

and you wish to plot **Y1** and **Y2** as separate lines along the X values, then
you will need to pivot the dataset longer. To do this, call `Pivot::PivotLonger()` and
pass in the dataset to create a "long" version of it that you can use with the line plot.
This new version of the dataset will stack the **Y1** and **Y2** columns on top of each other,
while mapping each value's respective value from **X** into the **X** column. Also, it will create
a new grouping column to indicate which column each value in the new **Y** originated from.

For example:

```cpp          
Pivot pv;
auto newDataset =
    pv.PivotLonger(myDataset, { L"x" }, { L"y1", L"y2" }, { L"GROUP" }, L"YValues");
```

This will result in a new dataset that appears as such:

| X   | YValues | GROUP |
| --: | --:     | :--   |
| 1   | 7       | y1    |
| 2   | 7.5     | y1    |
| 1   | 9       | y2    |
| 2   | 11      | y2    |

At this point, you can pass `newDataset` to the line plot and specify **X** and **YValues** as the X and Y
and **GROUP** as the grouping column. From there, this will create a line for the original **Y1** values and
another line for the original **Y2** values.
