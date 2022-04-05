Building a Dataset {#building-data}
=============================

A `Wisteria::Data::Dataset` object can be built programmatically, as well as [imported](ImportingData.md).
To build a dataset, you first construct it as a `shared_ptr<Data::Dataset>`, then you set its data via
`AddRow()`. After that, if you are using a grouping variable, you can set its string labels.

The following demostrates this:

```cpp
auto yData = std::make_shared<Data::Dataset>();
// fill the first continuous column with values
// and set the group ID to 0
yData->AddRow(RowInfo().Continuous({ 13.2 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 11.1 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 9.9 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 8.75 }).Group({ 0 }));
yData->AddRow(RowInfo().Continuous({ 12 }).Group({ 0 }));

// continue filling the first continuous column,
// but set these observations' group IDs to 1
yData->AddRow(RowInfo().Continuous({ 7.1 }).Group({ 1 }));
yData->AddRow(RowInfo().Continuous({ 25 }).Group({ 1 }));
yData->AddRow(RowInfo().Continuous({ 13 }).Group({ 1 }));
yData->AddRow(RowInfo().Continuous({ 10 }).Group({ 1 }));
yData->AddRow(RowInfo().Continuous({ 11 }).Group({ 1 }));
yData->AddRow(RowInfo().Continuous({ 13 }).Group({ 1 }));

// set the string labels for the codes in the first grouping column
yData->GetCategoricalColumn(0).GetStringTable() = { { 0, L"MALE" }, { 1, L"FEMALE" } };
```

After the dataset is built, you can pass it to most graph types to plot it.