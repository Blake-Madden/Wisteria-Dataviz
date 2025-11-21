#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../src/data/dataset.h"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

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
