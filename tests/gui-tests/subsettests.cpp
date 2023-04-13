#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../src/data/dataset.h"
#include "../../src/data/subset.h"

using namespace Wisteria;
using namespace Wisteria::Data;

TEST_CASE("Subset Simple Categorical", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Spelling Grades.csv",
        ImportInfo().
        ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
        CategoricalColumns({
            { L"Gender" },
            { L"WEEK_NAME" }
            }));

    Subset dsSubset;

    SECTION("Simple ==")
        {
        // dataset with only female observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 5);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID, femaleID, femaleID, femaleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5 });
        }

    SECTION("Simple <>")
        {
        // dataset with only male observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::NotEquals, { L"Female" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"MaLE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 5);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ maleID, maleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 90, 82, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5 });
        }

    SECTION("Simple <=")
        {
        // same as original dataset
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::LessThanOrEqualTo, { L"MALE" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"MaLE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEMALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 10);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value(),
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 90, 82, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 });
        }

    SECTION("Simple <")
        {
        // dataset with only female observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::LessThan, { L"MALE" } });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 5);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID, femaleID, femaleID, femaleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5 });
        }

    SECTION("Simple >=")
        {
        // same as original dataset
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::GreaterThanOrEqualTo, { L"feMALE" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"MaLE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEMALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 10);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value(),
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 90, 82, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 });
        }

    SECTION("Simple >")
        {
        // dataset with only male observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::GreaterThan, { L"Female" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"MaLE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 5);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ maleID, maleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"Week 1").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 90, 82, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5 });
        }

    SECTION("Simple Categorical No Matches")
        {
        std::shared_ptr<Data::Dataset> subset{ nullptr };
        // dataset with failed criterion will be null and exception is thrown
        REQUIRE_THROWS(subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"UNKNOWN" } }));
        CHECK(subset == nullptr);

        // will return empty dataset
        subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::GreaterThan, { L"UNKNOWN" } });
        CHECK(subset->GetRowCount() == 0);

        subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Gender", Comparison::GreaterThanOrEqualTo, { L"UNKNOWN" } });
        CHECK(subset->GetRowCount() == 0);
        }
    }

TEST_CASE("Subset Simple ID", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Spelling Grades.csv",
        ImportInfo().IdColumn(L"WEEK_NAME").
        ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
        CategoricalColumns({
            { L"Gender" }
            }));

    Subset dsSubset;

    SECTION("Simple ==")
        {
        // dataset with only Week 3 observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::Equals, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>{ L"Week 3", L"Week 3" });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3, 3 });
        }

    SECTION("Simple <>")
        {
        // missing week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::NotEquals, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 8);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 4", L"Week 5",
            L"Week 1", L"Week 2", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 86, 90, 90, 82, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 4, 5, 1, 2, 4, 5 });
        }

    SECTION("Simple <")
        {
        // less than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::LessThan, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 4);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 1", L"Week 2"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 90, 82 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 1, 2 });
        }

    SECTION("Simple <")
        {
        // <= week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::LessThanOrEqualTo, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 6);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 3", L"Week 1", L"Week 2", L"Week 3"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 90, 82, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 1, 2, 3 });
        }

    SECTION("Simple >")
        {
        // > than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::GreaterThan, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 4);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 4", L"Week 5", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 86, 90, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 4, 5, 4, 5 });
        }

    SECTION("Simple >=")
        {
        // >= than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week_name", Comparison::GreaterThanOrEqualTo, { L"weeK 3" } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 6);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 3", L"Week 4", L"Week 5", L"Week 3", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84, 86, 90, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3, 4, 5, 3, 4, 5 });
        }
    }

TEST_CASE("Subset Simple Continuous", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Spelling Grades.csv",
        ImportInfo().IdColumn(L"WEEK_NAME").
        ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
        CategoricalColumns({
            { L"Gender" }
            }));

    Subset dsSubset;

    SECTION("Simple ==")
        {
        // dataset with only Week 3 observations
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::Equals, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>{ L"Week 3", L"Week 3" });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3, 3 });
        }

    SECTION("Simple <>")
        {
        // missing week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::NotEquals, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 8);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 4", L"Week 5",
            L"Week 1", L"Week 2", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 86, 90, 90, 82, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 4, 5, 1, 2, 4, 5 });
        }

    SECTION("Simple <")
        {
        // less than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::LessThan, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 4);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 1", L"Week 2"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 90, 82 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 1, 2 });
        }

    SECTION("Simple <")
        {
        // <= week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::LessThanOrEqualTo, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 6);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 1", L"Week 2", L"Week 3", L"Week 1", L"Week 2", L"Week 3"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 90, 82, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 1, 2, 3 });
        }

    SECTION("Simple >")
        {
        // > than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::GreaterThan, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 4);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 4", L"Week 5", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 86, 90, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 4, 5, 4, 5 });
        }

    SECTION("Simple >=")
        {
        // >= than week 3
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"week", Comparison::GreaterThanOrEqualTo, { 3.0 } });
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"mALE").value();
        const auto femaleID = subset->GetCategoricalColumn(L"gender")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetIdColumn();

        CHECK(subset->GetRowCount() == 6);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(weekNameVar.GetValues() == std::vector<wxString>
            {
            L"Week 3", L"Week 4", L"Week 5", L"Week 3", L"Week 4", L"Week 5"
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84, 86, 90, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3, 4, 5, 3, 4, 5 });
        }
    }

TEST_CASE("Subset Simple Datetime", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Economics/Silver Futures.csv",
        ImportInfo().ContinuousColumns({ L"Close/Last" }).
        DateColumns({ { L"Date", DateImportMethod::Automatic, wxString{} } }));

    Subset dsSubset;

    SECTION("Simple ==")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::Equals, { L"12/07/2021" }});

        CHECK(subset->GetRowCount() == 1);
        CHECK(subset->GetContinuousColumn(L"Close/Last")->GetValues() == std::vector<double>{ 22.523 });
        }

    SECTION("Simple <>")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::NotEquals, { L"12/07/2021" }});

        CHECK(subset->GetRowCount() == 251);
        }

    SECTION("Simple <")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::LessThan, { L"01/06/2021" }});

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetContinuousColumn(L"Close/Last")->GetValues() == std::vector<double>{ 27.64, 27.364 });
        }

    SECTION("Simple <=")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::LessThanOrEqualTo, { L"01/06/2021" }});

        CHECK(subset->GetRowCount() == 3);
        CHECK(subset->GetContinuousColumn(L"Close/Last")->GetValues() == std::vector<double>{ 27.042, 27.64, 27.364 });
        }

    SECTION("Simple >")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::GreaterThan, { L"12/29/2021" }});

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetContinuousColumn(L"Close/Last")->GetValues() == std::vector<double>{ 23.352, 23.06 });
        }

    SECTION("Simple >=")
        {
        const auto subset =
            dsSubset.SubsetSimple(ds,
                ColumnFilterInfo{ L"Date", Comparison::GreaterThanOrEqualTo, { L"12/29/2021" }});

        CHECK(subset->GetRowCount() == 3);
        CHECK(subset->GetContinuousColumn(L"Close/Last")->GetValues() == std::vector<double>{ 23.352, 23.06, 22.858 });
        }
    }

TEST_CASE("Subset AND", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Spelling Grades.csv",
        ImportInfo().
        ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
        CategoricalColumns({
            { L"Gender" },
            { L"WEEK_NAME" }
            }));

    Subset dsSubset;

    SECTION("Subset ==")
        {
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::Equals, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 1);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3 });
        }

    SECTION("Subset >=")
        {
        // dataset with only female observations starting from Week 3 or later
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::GreaterThanOrEqualTo, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 3);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID, femaleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 84, 86, 90 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 3, 4, 5 });
        }

    SECTION("Subset >")
        {
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::GreaterThan, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WEEK 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 86, 90 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 4, 5 });
        }

    SECTION("Subset <")
        {
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::LessThan, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 2);
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WEEK 1").value(),
            weekNameVar->GetIDFromLabel(L"Week 2").value()
            });
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2 });
        }

    SECTION("Subset <=")
        {
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::LessThanOrEqualTo, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 3);
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WEEK 1").value(),
            weekNameVar->GetIDFromLabel(L"Week 2").value(),
            weekNameVar->GetIDFromLabel(L"Week 3").value()
            });
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID, femaleID });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3 });
        }

    SECTION("Subset <>")
        {
        const auto subset =
            dsSubset.SubsetAnd(ds,
                {
                ColumnFilterInfo{ L"GENDER", Comparison::Equals, { L"FEMALE" } },
                ColumnFilterInfo{ L"week_name", Comparison::NotEquals, { L"WEEK 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 4);
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WEEK 1").value(),
            weekNameVar->GetIDFromLabel(L"Week 2").value(),
            weekNameVar->GetIDFromLabel(L"Week 4").value(),
            weekNameVar->GetIDFromLabel(L"Week 5").value()
            });
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>{ femaleID, femaleID, femaleID, femaleID });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 86, 90 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 4, 5 });
        }
	}

TEST_CASE("Subset OR", "[data][subset]")
	{
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    auto ds = std::make_shared<Data::Dataset>();
    ds->ImportCSV(appDir + L"/datasets/Spelling Grades.csv",
        ImportInfo().
        ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
        CategoricalColumns({
            { L"Gender" },
            { L"WEEK_NAME" }
            }));

    Subset dsSubset;

    SECTION("Subset ==")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::Equals, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 6);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value()
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 3 });
        }

    SECTION("Subset <>")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::NotEquals, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 9);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 90, 82, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 1, 2, 4, 5 });
        }

    SECTION("Subset <=")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::LessThanOrEqualTo, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 8);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 90, 82, 83 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 1, 2, 3 });
        }

    SECTION("Subset <")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::LessThan, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 7);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 90, 82 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 1, 2 });
        }

    SECTION("Subset >=")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::GreaterThanOrEqualTo, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 8);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 83, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 3, 4, 5 });
        }

    SECTION("Subset >")
        {
        const auto subset =
            dsSubset.SubsetOr(ds,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, { L"Female" } },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::GreaterThan, { L"Week 3" } }
                });
        const auto femaleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"FEmALE").value();
        const auto maleID = subset->GetCategoricalColumn(L"GENDER")->GetIDFromLabel(L"male").value();
        const auto weekNameVar = subset->GetCategoricalColumn(L"week_name");

        CHECK(subset->GetRowCount() == 7);
        CHECK(subset->GetCategoricalColumn(L"GENDER")->GetValues() == std::vector<GroupIdType>
            { femaleID, femaleID, femaleID, femaleID, femaleID, maleID, maleID });
        CHECK(subset->GetCategoricalColumn(L"WEEK_NAME")->GetValues() == std::vector<GroupIdType>
            {
            weekNameVar->GetIDFromLabel(L"WeEk 1").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 2").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 3").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 4").value(),
            weekNameVar->GetIDFromLabel(L"WeEk 5").value(),
            });
        CHECK(subset->GetContinuousColumn(L"AVG_GRADE")->GetValues() == std::vector<double>{ 88, 85, 84, 86, 90, 59, 91 });
        CHECK(subset->GetContinuousColumn(L"WEEK")->GetValues() == std::vector<double>{ 1, 2, 3, 4, 5, 4, 5 });
        }
    }
