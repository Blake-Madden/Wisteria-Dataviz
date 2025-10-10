#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../src/data/dataset.h"

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
                CHECK(ds.GetCategoricalColumn(L"REVIEW")->GetStringTable().size() == 0);

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
