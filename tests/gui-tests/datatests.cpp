#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../src/data/dataset.h"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include <wx/file.h>
#include <wx/filename.h>

using namespace Wisteria;
using namespace Wisteria::Data;

TEST_CASE("Data range", "[data]")
    {
    Data::Dataset ds;

    ds.AddRow(Data::RowInfo().Continuous({ 5, 6 }).Id(L"label1"));
    ds.AddRow(Data::RowInfo().Continuous({ 5, 6 }).Id(L"label2"));
    CHECK(ds.GetIdColumn().GetValue(0) == L"label1");
    CHECK(ds.GetIdColumn().GetValue(1) == L"label2");
    }

TEST_CASE("GetContinuousMedian", "[data]")
    {
    Dataset ds;
    ds.AddContinuousColumn(L"score");
    ds.AddCategoricalColumn(L"group");

    // Insert rows with both continuous and categorical data
    ds.AddRow(RowInfo().Continuous({ 10 }).Categoricals({ 0 }).Id(L"id1"));
    ds.AddRow(RowInfo().Continuous({ 20 }).Categoricals({ 0 }).Id(L"id2"));
    ds.AddRow(RowInfo().Continuous({ 30 }).Categoricals({ 1 }).Id(L"id3"));
    ds.AddRow(RowInfo().Continuous({ 40 }).Categoricals({ 1 }).Id(L"id4"));
    ds.AddRow(RowInfo().Continuous({ 50 }).Categoricals({ 1 }).Id(L"id5"));

    SECTION("Median of odd number of items")
        {
        CHECK_THAT(ds.GetContinuousMedian(L"score"),
                   Catch::Matchers::WithinAbs(30.0, 0.00001));
        }

    SECTION("Median of even number of items")
        {
        ds.AddRow(RowInfo().Continuous({ 60 }).Categoricals({ 1 }).Id(L"id6"));
        CHECK_THAT(ds.GetContinuousMedian(L"score"),
                   Catch::Matchers::WithinAbs((30.0 + 40.0) / 2.0, 0.00001));
        }

    SECTION("Median when some values are NaN")
        {
        ds.AddRow(RowInfo().Continuous(
                      { std::numeric_limits<double>::quiet_NaN() })
                      .Categoricals({ 0 })
                      .Id(L"id7"));

        CHECK_THAT(ds.GetContinuousMedian(L"score"),
                   Catch::Matchers::WithinAbs(30.0, 0.00001));
        }

    SECTION("Grouped median")
        {
        // Group 0 contains: 10,20  → median = 15
        CHECK_THAT(ds.GetContinuousMedian(L"score", L"group", 0),
                   Catch::Matchers::WithinAbs(15.0, 0.00001));

        // Group 1 contains: 30,40,50 → median = 40
        CHECK_THAT(ds.GetContinuousMedian(L"score", L"group", 1),
                   Catch::Matchers::WithinAbs(40.0, 0.00001));
        }

    SECTION("Empty dataset returns NaN")
        {
        Dataset empty;
        empty.AddContinuousColumn(L"score");
        CHECK(std::isnan(empty.GetContinuousMedian(L"score")));
        }
    }

TEST_CASE("GetContinuousMinMax", "[data]")
    {
    Dataset ds;
    ds.AddContinuousColumn(L"score");

    ds.AddRow(RowInfo().Continuous({ 10 }).Id(L"id1"));
    ds.AddRow(RowInfo().Continuous({ 20 }).Id(L"id2"));
    ds.AddRow(RowInfo().Continuous({ 30 }).Id(L"id3"));
    ds.AddRow(RowInfo().Continuous({ 40 }).Id(L"id4"));
    ds.AddRow(RowInfo().Continuous({ 50 }).Id(L"id5"));

    SECTION("Basic min/max")
        {
        auto [minVal, maxVal] = ds.GetContinuousMinMax(L"score");

        CHECK_THAT(minVal, Catch::Matchers::WithinAbs(10.0, 0.00001));
        CHECK_THAT(maxVal, Catch::Matchers::WithinAbs(50.0, 0.00001));
        }

    SECTION("Min/max ignoring NaN")
        {
        ds.AddRow(RowInfo().Continuous(
                    { std::numeric_limits<double>::quiet_NaN() })
                    .Id(L"id6"));

        auto [minVal, maxVal] = ds.GetContinuousMinMax(L"score");

        CHECK_THAT(minVal, Catch::Matchers::WithinAbs(10.0, 0.00001));
        CHECK_THAT(maxVal, Catch::Matchers::WithinAbs(50.0, 0.00001));
        }

    SECTION("Grouped min/max")
    {
        Dataset ds2;
        ds2.AddContinuousColumn(L"score");
        ds2.AddCategoricalColumn(L"group");

        ds2.AddRow(RowInfo().Continuous({ 10 }).Categoricals({ 0 }).Id(L"id1"));
        ds2.AddRow(RowInfo().Continuous({ 20 }).Categoricals({ 0 }).Id(L"id2"));
        ds2.AddRow(RowInfo().Continuous({ 30 }).Categoricals({ 1 }).Id(L"id3"));
        ds2.AddRow(RowInfo().Continuous({ 40 }).Categoricals({ 1 }).Id(L"id4"));
        ds2.AddRow(RowInfo().Continuous({ 50 }).Categoricals({ 1 }).Id(L"id5"));
        ds2.AddRow(RowInfo().Continuous({ 15 }).Categoricals({ 0 }).Id(L"id6"));
        ds2.AddRow(RowInfo().Continuous({ 45 }).Categoricals({ 1 }).Id(L"id7"));

        auto [gMinA, gMaxA] = ds2.GetContinuousMinMax(L"score", L"group", 0);
        CHECK_THAT(gMinA, Catch::Matchers::WithinAbs(10.0, 0.00001));
        CHECK_THAT(gMaxA, Catch::Matchers::WithinAbs(20.0, 0.00001));

        auto [gMinB, gMaxB] = ds2.GetContinuousMinMax(L"score", L"group", 1);
        CHECK_THAT(gMinB, Catch::Matchers::WithinAbs(30.0, 0.00001));
        CHECK_THAT(gMaxB, Catch::Matchers::WithinAbs(50.0, 0.00001));
    }

    SECTION("Empty dataset returns NaNs")
        {
        Dataset empty;
        empty.AddContinuousColumn(L"score");
        auto [minVal, maxVal] = empty.GetContinuousMinMax(L"score");

        CHECK(std::isnan(minVal));
        CHECK(std::isnan(maxVal));
        }
    }

TEST_CASE("GetContinuousTotal", "[data]")
    {
    Dataset ds;
    ds.AddContinuousColumn(L"score");

    ds.AddRow(RowInfo().Continuous({ 10 }).Id(L"id1"));
    ds.AddRow(RowInfo().Continuous({ 20 }).Id(L"id2"));
    ds.AddRow(RowInfo().Continuous({ 30 }).Id(L"id3"));
    ds.AddRow(RowInfo().Continuous({ 40 }).Id(L"id4"));
    ds.AddRow(RowInfo().Continuous({ 50 }).Id(L"id5"));

    SECTION("Total of continuous column")
        {
        double total = ds.GetContinuousTotal(L"score");
        CHECK_THAT(total, Catch::Matchers::WithinAbs(150.0, 0.00001));
        }

    SECTION("Total ignoring NaN")
        {
        ds.AddRow(RowInfo().Continuous(
                    { std::numeric_limits<double>::quiet_NaN() })
                    .Id(L"id6"));

        double total = ds.GetContinuousTotal(L"score");
        CHECK_THAT(total, Catch::Matchers::WithinAbs(150.0, 0.00001));
        }

    SECTION("Grouped total")
    {
        Dataset ds2;
        ds2.AddContinuousColumn(L"score");
        ds2.AddCategoricalColumn(L"group");

        ds2.AddRow(RowInfo().Continuous({ 10 }).Categoricals({ 0 }).Id(L"id1"));
        ds2.AddRow(RowInfo().Continuous({ 20 }).Categoricals({ 0 }).Id(L"id2"));
        ds2.AddRow(RowInfo().Continuous({ 30 }).Categoricals({ 1 }).Id(L"id3"));
        ds2.AddRow(RowInfo().Continuous({ 40 }).Categoricals({ 1 }).Id(L"id4"));
        ds2.AddRow(RowInfo().Continuous({ 50 }).Categoricals({ 1 }).Id(L"id5"));
        ds2.AddRow(RowInfo().Continuous({ 15 }).Categoricals({ 0 }).Id(L"id6"));
        ds2.AddRow(RowInfo().Continuous({ 45 }).Categoricals({ 1 }).Id(L"id7"));

        double totalA = ds2.GetContinuousTotal(L"score", L"group", 0);
        double totalB = ds2.GetContinuousTotal(L"score", L"group", 1);

        CHECK_THAT(totalA, Catch::Matchers::WithinAbs(45.0, 0.00001));  // 10+20+15
        CHECK_THAT(totalB, Catch::Matchers::WithinAbs(165.0, 0.00001)); // 30+40+50+45
    }

    SECTION("Empty dataset returns NaN")
        {
        Dataset empty;
        empty.AddContinuousColumn(L"score");

        double total = empty.GetContinuousTotal(L"score");
        CHECK(std::isnan(total));
        }
    }

SCENARIO("Add continuous variables", "[data]")
    {
    Data::Dataset ds;
    ds.AddContinuousColumn(L"score");

    GIVEN("continuous 'score' column is already present")
        {
        WHEN("inserting new column with the same name")
            {
            THEN("will be no effect.")
                {
                ds.AddContinuousColumn(L"ScoRe");
                ds.AddCategoricalColumn(L"SCORE");
                CHECK(ds.GetContinuousColumns().size() == 1);
                }
            }
        }
    }

SCENARIO("Add date variables", "[data]")
    {
    Data::Dataset ds;
    ds.AddDateColumn(L"when");

    GIVEN("continuous 'when' column is already present")
        {
        WHEN("inserting new column with the same name")
            {
            THEN("will be no effect.")
                {
                ds.AddDateColumn(L"WHEN");
                ds.AddDateColumn(L"WhEn");
                CHECK(ds.GetDateColumns().size() == 1);
                }
            }
        }
    }

TEST_CASE("MISSING_DATA_CODE constant", "[data][md]")
    {
    // MISSING_DATA_CODE should be the max value of GroupIdType
    CHECK(ColumnWithStringTable::MISSING_DATA_CODE ==
          std::numeric_limits<GroupIdType>::max());
    // it should not be zero (the old sentinel)
    CHECK(ColumnWithStringTable::MISSING_DATA_CODE != 0);
    }

TEST_CASE("AddCategoricalColumn fills with MISSING_DATA_CODE", "[data][md]")
    {
    Dataset ds;
    ds.AddContinuousColumn(L"score");
    ds.AddRow(RowInfo().Continuous({ 1 }).Id(L"r1"));
    ds.AddRow(RowInfo().Continuous({ 2 }).Id(L"r2"));
    ds.AddRow(RowInfo().Continuous({ 3 }).Id(L"r3"));

    SECTION("No string table — new column backfilled with MD")
        {
        ds.AddCategoricalColumn(L"cat");
        const auto col = ds.GetCategoricalColumn(L"cat");
        REQUIRE(col != ds.GetCategoricalColumns().cend());
        // all 3 existing rows should be MISSING_DATA_CODE, not 0
        for (size_t i = 0; i < ds.GetRowCount(); ++i)
            {
            CHECK(col->GetValue(i) == ColumnWithStringTable::MISSING_DATA_CODE);
            CHECK(col->GetValue(i) != 0);
            }
        // string table should have the MD entry
        const auto mdCode = col->FindMissingDataCode();
        CHECK(mdCode.has_value());
        CHECK(mdCode.value() == ColumnWithStringTable::MISSING_DATA_CODE);
        }

    SECTION("Empty string table — new column backfilled with MD")
        {
        ColumnWithStringTable::StringTableType emptySt;
        ds.AddCategoricalColumn(L"cat", emptySt);
        const auto col = ds.GetCategoricalColumn(L"cat");
        REQUIRE(col != ds.GetCategoricalColumns().cend());
        for (size_t i = 0; i < ds.GetRowCount(); ++i)
            {
            CHECK(col->GetValue(i) == ColumnWithStringTable::MISSING_DATA_CODE);
            }
        const auto mdCode = col->FindMissingDataCode();
        CHECK(mdCode.has_value());
        CHECK(mdCode.value() == ColumnWithStringTable::MISSING_DATA_CODE);
        }

    SECTION("Non-empty string table — rows use that table's values")
        {
        ColumnWithStringTable::StringTableType st{
            { 0, L"No" }, { 1, L"Yes" } };
        ds.AddCategoricalColumn(L"cat", st);
        const auto col = ds.GetCategoricalColumn(L"cat");
        REQUIRE(col != ds.GetCategoricalColumns().cend());
        // string table should contain our entries plus the MD backfill entry
        CHECK(col->GetStringTable().size() == 3);
        CHECK(col->GetIDFromLabel(L"No").value() == 0);
        CHECK(col->GetIDFromLabel(L"Yes").value() == 1);
        CHECK(col->FindMissingDataCode().has_value());
        }
    }

TEST_CASE("Zero is a valid categorical group ID", "[data][md]")
    {
    Dataset ds;
    ds.AddContinuousColumn(L"val");
    ColumnWithStringTable::StringTableType st{
        { 0, L"GroupA" }, { 1, L"GroupB" } };
    ds.AddCategoricalColumn(L"grp", st);

    // insert rows using group ID 0 — should be treated as real data, not MD
    ds.AddRow(RowInfo().Continuous({ 10 }).Categoricals({ 0 }).Id(L"r1"));
    ds.AddRow(RowInfo().Continuous({ 20 }).Categoricals({ 0 }).Id(L"r2"));
    ds.AddRow(RowInfo().Continuous({ 30 }).Categoricals({ 1 }).Id(L"r3"));

    const auto col = ds.GetCategoricalColumn(L"grp");
    REQUIRE(col != ds.GetCategoricalColumns().cend());

    // group 0 rows should NOT be flagged as missing data
    CHECK_FALSE(col->IsMissingData(0));
    CHECK_FALSE(col->IsMissingData(1));
    CHECK(col->GetValue(0) == 0);
    CHECK(col->GetValue(1) == 0);
    CHECK(col->GetValueAsLabel(0) == L"GroupA");
    CHECK(col->GetValueAsLabel(1) == L"GroupA"); // row 1 also has group 0
    CHECK(col->GetValueAsLabel(2) == L"GroupB"); // row 2 has group 1

    // grouped operations should see group 0 rows
    CHECK_THAT(ds.GetContinuousMedian(L"val", L"grp", 0),
               Catch::Matchers::WithinAbs(15.0, 0.00001));
    CHECK_THAT(ds.GetContinuousMedian(L"val", L"grp", 1),
               Catch::Matchers::WithinAbs(30.0, 0.00001));
    }

TEST_CASE("FillWithMissingData uses MISSING_DATA_CODE", "[data][md]")
    {
    Dataset ds;
    ColumnWithStringTable::StringTableType st{
        { 0, L"A" }, { 1, L"B" } };
    ds.AddCategoricalColumn(L"cat", st);
    ds.AddRow(RowInfo().Categoricals({ 0 }).Id(L"r1"));
    ds.AddRow(RowInfo().Categoricals({ 1 }).Id(L"r2"));

    auto col = ds.GetCategoricalColumn(L"cat");
    REQUIRE(col != ds.GetCategoricalColumns().end());
    col->FillWithMissingData();

    // after fill, all values should be the MD code
    const auto mdCode = col->FindMissingDataCode();
    REQUIRE(mdCode.has_value());
    CHECK(mdCode.value() == ColumnWithStringTable::MISSING_DATA_CODE);
    for (size_t i = 0; i < ds.GetRowCount(); ++i)
        {
        CHECK(col->GetValue(i) == mdCode.value());
        CHECK(col->IsMissingData(i));
        }
    // group 0 data should be gone
    CHECK(col->GetValue(0) != 0);
    CHECK(col->GetValue(1) != 1);
    }

TEST_CASE("FindMissingDataCode with various string tables", "[data][md]")
    {
    SECTION("Empty string table returns nullopt")
        {
        const ColumnWithStringTable::StringTableType st;
        CHECK_FALSE(
            ColumnWithStringTable::FindMissingDataCode(st).has_value());
        }

    SECTION("Table with only non-empty strings returns nullopt")
        {
        const ColumnWithStringTable::StringTableType st{
            { 0, L"A" }, { 1, L"B" } };
        CHECK_FALSE(
            ColumnWithStringTable::FindMissingDataCode(st).has_value());
        }

    SECTION("Table with empty string at MISSING_DATA_CODE key")
        {
        const ColumnWithStringTable::StringTableType st{
            { 0, L"A" },
            { ColumnWithStringTable::MISSING_DATA_CODE, wxString{} } };
        const auto md =
            ColumnWithStringTable::FindMissingDataCode(st);
        REQUIRE(md.has_value());
        CHECK(md.value() == ColumnWithStringTable::MISSING_DATA_CODE);
        }

    SECTION("Table with empty string at key 0 finds it at 0")
        {
        // if someone manually puts empty at 0, FindMissingDataCode finds it
        const ColumnWithStringTable::StringTableType st{
            { 0, wxString{} }, { 1, L"B" } };
        const auto md =
            ColumnWithStringTable::FindMissingDataCode(st);
        REQUIRE(md.has_value());
        CHECK(md.value() == 0);
        }
    }

TEST_CASE("GetNextKey starts at 0 for empty tables", "[data][md]")
    {
    const ColumnWithStringTable::StringTableType empty;
    CHECK(ColumnWithStringTable::GetNextKey(empty) == 0);

    // with entries, next key is max key + 1
    const ColumnWithStringTable::StringTableType st{
        { 0, L"A" }, { 1, L"B" } };
    CHECK(ColumnWithStringTable::GetNextKey(st) == 2);

    // even with MISSING_DATA_CODE in the table
    const ColumnWithStringTable::StringTableType stWithMd{
        { 0, L"A" },
        { ColumnWithStringTable::MISSING_DATA_CODE, wxString{} } };
    // next key would overflow, but GetNextKey just returns max + 1
    CHECK(ColumnWithStringTable::GetNextKey(stWithMd) ==
          ColumnWithStringTable::MISSING_DATA_CODE + 1);
    }

TEST_CASE("Categorical column with 0 and 1 data values", "[data][md]")
    {
    // this is the original bug scenario: a column with only 0 and 1
    // should work as valid categorical data, not be treated as continuous
    Dataset ds;
    ColumnWithStringTable::StringTableType st{
        { 0, L"No" }, { 1, L"Yes" } };
    ds.AddCategoricalColumn(L"flag", st);
    ds.AddContinuousColumn(L"amount");

    ds.AddRow(RowInfo().Categoricals({ 0 }).Continuous({ 100 }).Id(L"r1"));
    ds.AddRow(RowInfo().Categoricals({ 1 }).Continuous({ 200 }).Id(L"r2"));
    ds.AddRow(RowInfo().Categoricals({ 0 }).Continuous({ 150 }).Id(L"r3"));
    ds.AddRow(RowInfo().Categoricals({ 1 }).Continuous({ 250 }).Id(L"r4"));

    const auto col = ds.GetCategoricalColumn(L"flag");
    REQUIRE(col != ds.GetCategoricalColumns().cend());

    // 0 should be valid data, not missing
    CHECK_FALSE(col->IsMissingData(0));
    CHECK_FALSE(col->IsMissingData(2));
    CHECK(col->GetValueAsLabel(0) == L"No");
    CHECK(col->GetValueAsLabel(1) == L"Yes");

    // grouped stats should work with group 0
    CHECK_THAT(ds.GetContinuousTotal(L"amount", L"flag", 0),
               Catch::Matchers::WithinAbs(250.0, 0.00001));
    CHECK_THAT(ds.GetContinuousTotal(L"amount", L"flag", 1),
               Catch::Matchers::WithinAbs(450.0, 0.00001));
    }

TEST_CASE("ContainsMissingData with MISSING_DATA_CODE", "[data][md]")
    {
    Dataset ds;
    ColumnWithStringTable::StringTableType st{
        { 0, L"A" }, { 1, L"B" },
        { ColumnWithStringTable::MISSING_DATA_CODE, wxString{} } };
    ds.AddCategoricalColumn(L"cat", st);

    SECTION("No missing data rows")
        {
        ds.AddRow(RowInfo().Categoricals({ 0 }).Id(L"r1"));
        ds.AddRow(RowInfo().Categoricals({ 1 }).Id(L"r2"));
        const auto col = ds.GetCategoricalColumn(L"cat");
        CHECK_FALSE(col->ContainsMissingData());
        }

    SECTION("Has missing data rows")
        {
        ds.AddRow(RowInfo().Categoricals({ 0 }).Id(L"r1"));
        ds.AddRow(RowInfo()
                      .Categoricals({ ColumnWithStringTable::MISSING_DATA_CODE })
                      .Id(L"r2"));
        const auto col = ds.GetCategoricalColumn(L"cat");
        CHECK(col->ContainsMissingData());
        CHECK_FALSE(col->IsMissingData(0));
        CHECK(col->IsMissingData(1));
        }
    }

TEST_CASE("String table with ID 0 as real data alongside MD", "[data][md]")
    {
    // the whole point: 0 is real data, UINT64_MAX is MD
    Dataset ds;
    ColumnWithStringTable::StringTableType st{
        { 0, L"Zero" },
        { 1, L"One" },
        { 2, L"Two" },
        { ColumnWithStringTable::MISSING_DATA_CODE, wxString{} } };
    ds.AddCategoricalColumn(L"cat", st);

    ds.AddRow(RowInfo().Categoricals({ 0 }).Id(L"r1"));
    ds.AddRow(RowInfo().Categoricals({ 1 }).Id(L"r2"));
    ds.AddRow(RowInfo().Categoricals({ 2 }).Id(L"r3"));
    ds.AddRow(RowInfo()
                  .Categoricals({ ColumnWithStringTable::MISSING_DATA_CODE })
                  .Id(L"r4"));

    const auto col = ds.GetCategoricalColumn(L"cat");
    REQUIRE(col != ds.GetCategoricalColumns().cend());

    // 0, 1, 2 are valid data
    CHECK(col->GetValueAsLabel(0) == L"Zero");
    CHECK(col->GetValueAsLabel(1) == L"One");
    CHECK(col->GetValueAsLabel(2) == L"Two");
    CHECK_FALSE(col->IsMissingData(0));
    CHECK_FALSE(col->IsMissingData(1));
    CHECK_FALSE(col->IsMissingData(2));

    // row 3 is missing data
    CHECK(col->IsMissingData(3));
    CHECK(col->GetValue(3) == ColumnWithStringTable::MISSING_DATA_CODE);
    CHECK(col->ContainsMissingData());
    }

TEST_CASE("Import CSV with empty categorical cells as MD", "[data][md]")
    {
    // write a temp CSV: a string column with some empty cells
    const wxString tempPath =
        wxFileName::CreateTempFileName(L"md_import_") + L".csv";
    {
    wxFile f(tempPath, wxFile::write);
    REQUIRE(f.IsOpened());
    f.Write(L"Name,Score\n"
            L"Alice,10\n"
            L",20\n"
            L"Charlie,30\n"
            L",40\n");
    }

    SECTION("ReadAsStrings — empty cells become MD")
        {
        Dataset ds;
        ds.ImportCSV(tempPath,
                     ImportInfo()
                         .ContinuousColumns({ L"Score" })
                         .CategoricalColumns(
                             { { L"Name",
                                 CategoricalImportMethod::ReadAsStrings } }));

        const auto col = ds.GetCategoricalColumn(L"Name");
        REQUIRE(col != ds.GetCategoricalColumns().cend());

        // rows 0 and 2 should have real data
        CHECK_FALSE(col->IsMissingData(0));
        CHECK_FALSE(col->IsMissingData(2));
        CHECK(col->GetValueAsLabel(0) == L"Alice");
        CHECK(col->GetValueAsLabel(2) == L"Charlie");

        // rows 1 and 3 should be missing data (empty string in CSV)
        CHECK(col->IsMissingData(1));
        CHECK(col->IsMissingData(3));

        // the MD code should map to empty string in the string table
        const auto mdCode = col->FindMissingDataCode();
        REQUIRE(mdCode.has_value());
        CHECK(col->GetLabelFromID(mdCode.value()).empty());
        }

    SECTION("ReadAsIntegers — empty cells become MISSING_DATA_CODE")
        {
        // rewrite CSV with integer codes and empty cells
        {
        wxFile f(tempPath, wxFile::write);
        f.Write(L"Code,Score\n"
                L"0,10\n"
                L",20\n"
                L"1,30\n"
                L",40\n");
        }

        Dataset ds;
        ds.ImportCSV(tempPath,
                     ImportInfo()
                         .ContinuousColumns({ L"Score" })
                         .CategoricalColumns(
                             { { L"Code",
                                 CategoricalImportMethod::ReadAsIntegers } }));

        const auto col = ds.GetCategoricalColumn(L"Code");
        REQUIRE(col != ds.GetCategoricalColumns().cend());

        // row 0 has code 0 — should be valid data, not MD
        CHECK(col->GetValue(0) == 0);

        // rows 1 and 3 should have the MD sentinel
        CHECK(col->GetValue(1) == ColumnWithStringTable::MISSING_DATA_CODE);
        CHECK(col->GetValue(3) == ColumnWithStringTable::MISSING_DATA_CODE);

        // row 2 has code 1
        CHECK(col->GetValue(2) == 1);
        }

    wxRemoveFile(tempPath);
    }

SCENARIO("Add categorical variables", "[data]")
    {
    Data::Dataset ds;
    ds.AddCategoricalColumn(L"review");
    ColumnWithStringTable::StringTableType st{ { 0, L"Bad" } };

    GIVEN("categorical 'Review' column is already present")
        {
        WHEN("inserting new column with the same name")
            {
            THEN("will be no effect.")
                {
                ds.AddCategoricalColumn(L"RevIeW");
                ds.AddCategoricalColumn(L"REVIEW");
                CHECK(ds.GetCategoricalColumns().size() == 1);
                CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetStringTable().empty());

                AND_WHEN("the same named column is inserted with a string table")
                    {
                    THEN("the existing column will use that string table.")
                        {
                        ds.AddCategoricalColumn(L"review", st);
                        CHECK(ds.GetCategoricalColumns().size() == 1);
                        CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetStringTable().size() == 1);
                        CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetIDFromLabel(L"bad") == 0);

                        AND_WHEN("the column name (with no string table) is added again")
                            {
                            THEN("there is no effect, and the string table is the same.")
                                {
                                ds.AddCategoricalColumn(L"review");
                                CHECK(ds.GetCategoricalColumns().size() == 1);
                                CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetStringTable().size() == 1);
                                CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetIDFromLabel(L"bad") == 0);

                                AND_WHEN("the string table is changed and the column name is added again")
                                    {
                                    st.insert({ 1, L"Good" });
                                    THEN("the existing column's string table is updated.")
                                        {
                                        ds.AddCategoricalColumn(L"review", st);
                                        CHECK(ds.GetCategoricalColumns().size() == 1);
                                        CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetStringTable().size() == 2);
                                        CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetIDFromLabel(L"bad") == 0);
                                        CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetIDFromLabel(L"good") == 1);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
