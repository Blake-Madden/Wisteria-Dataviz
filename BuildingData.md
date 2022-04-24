Building a Dataset
=============================

A `Wisteria::Data::Dataset` object can be built programmatically, as well as [imported](ImportingData.md).
To build a dataset, you first construct it as a `shared_ptr<Data::Dataset>`, then you set its data via
`AddRow()`. After that, if you are using a categorical (e.g., grouping) variable, you can set its string labels.

The following demostrates this:

```cpp
auto yData = std::make_shared<Data::Dataset>();

// specify the columns
yData->AddContinuousColumn(L"Score");
yData->AddCategoricalColumn(L"Group");
// fill the continuous column with values
// and set the group ID to 0
yData->AddRow(RowInfo().Continuous({ 13.2 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 11.1 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 9.9 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 8.75 }).Categoricals({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Categoricals({ 0 }));

// continue filling the continuous column,
// but set these observations' group IDs to 1
yData->AddRow(RowInfo().Continuous({ 7.1 }).Categoricals({ 1 }));
yData->AddRow(RowInfo().Continuous({ 25 }).Categoricals({ 1 }));
yData->AddRow(RowInfo().Continuous({ 13 }).Categoricals({ 1 }));
yData->AddRow(RowInfo().Continuous({ 10 }).Categoricals({ 1 }));
yData->AddRow(RowInfo().Continuous({ 11 }).Categoricals({ 1 }));
yData->AddRow(RowInfo().Continuous({ 13 }).Categoricals({ 1 }));

// set the string labels for the codes in the grouping column
yData->GetCategoricalColumn(L"Group")->GetStringTable() =
    { { 0, L"MALE" }, { 1, L"FEMALE" } };
```

After the dataset is built, you can pass it to most graph types to plot it.