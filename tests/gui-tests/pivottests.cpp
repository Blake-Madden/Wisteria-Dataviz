#include "../../src/data/dataset.h"
#include "../../src/data/pivot.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using namespace Wisteria;
using namespace Wisteria::Data;

namespace
    {
    // Small helper to add a row: ID + categoricals + continuous values
    static void AddRow(Dataset& ds, const wxString& id, const std::vector<GroupIdType>& cats,
                       const std::vector<double>& vals)
        {
        RowInfo r;
        r.Id(id);
        if (!cats.empty())
            {
            r.Categoricals(cats);
            }
        if (!vals.empty())
            {
            r.Continuous(vals);
            }
        ds.AddRow(r);
        }

    // UTF-8 helpers to accept both char and char8_t (C++20) literals
    static std::vector<unsigned char> MakeUtf8(const char* s)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(s);
        return std::vector<unsigned char>(p, p + std::strlen(s));
        }

    static std::vector<unsigned char> MakeUtf8(std::string_view sv)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(sv.data());
        return std::vector<unsigned char>(p, p + sv.size());
        }

    static std::vector<unsigned char> MakeUtf8(const char8_t* s)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(s);
        const auto n = std::char_traits<char8_t>::length(s);
        return std::vector<unsigned char>(p, p + n);
        }

    static std::vector<unsigned char> MakeUtf8(std::u8string_view sv)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(sv.data());
        return std::vector<unsigned char>(p, p + sv.size());
        }
    } // namespace

// --------------------------- PivotWider ---------------------------

TEST_CASE("PivotWider: multi-value columns with fill", "[Pivot][Wider]")
    {
    // Source dataset:
    //  ID | Group | Val1 | Val2
    //  r1 |   A   |  10  |  1
    //  r1 |   B   |  20  |  2
    //  r2 |   A   |  30  |  3   (missing B → fill)
    Dataset src;

    // Define columns
    src.GetIdColumn().SetName("ID");

    ColumnWithStringTable::StringTableType groupST;
    groupST.insert({ 0, "A" });
    groupST.insert({ 1, "B" });
    src.AddCategoricalColumn("Group", groupST);

    src.AddContinuousColumn("Val1");
    src.AddContinuousColumn("Val2");

    // Add rows (Group IDs correspond to groupST keys)
    AddRow(src, "r1", { 0 }, { 10, 1 });
    AddRow(src, "r1", { 1 }, { 20, 2 });
    AddRow(src, "r2", { 0 }, { 30, 3 });

    // Pivot wider: ID is the identifier, names from "Group",
    // values from {Val1, Val2}, separator "_", no prefix, fill = 0.0
    auto wide = Pivot::PivotWider(std::make_shared<const Dataset>(src),
                                  /*IdColumns*/ { "ID" },
                                  /*namesFrom*/ "Group",
                                  /*valuesFrom*/ { "Val1", "Val2" },
                                  /*namesSep*/ "_",
                                  /*namesPrefix*/ wxEmptyString,
                                  /*fillValue*/ 0.0);

    // Expect 2 rows (r1, r2) and 4 pivot columns: Val1_A, Val1_B, Val2_A, Val2_B
    CHECK(wide->GetRowCount() == 2);

    const auto cVal1A = wide->GetContinuousColumn("Val1_A");
    const auto cVal1B = wide->GetContinuousColumn("Val1_B");
    const auto cVal2A = wide->GetContinuousColumn("Val2_A");
    const auto cVal2B = wide->GetContinuousColumn("Val2_B");
    CHECK(cVal1A != wide->GetContinuousColumns().cend());
    CHECK(cVal1B != wide->GetContinuousColumns().cend());
    CHECK(cVal2A != wide->GetContinuousColumns().cend());
    CHECK(cVal2B != wide->GetContinuousColumns().cend());

    // r1 row:
    CHECK(wide->GetIdColumn().GetValue(0) == "r1");
    if (cVal1A != wide->GetContinuousColumns().cend())
        {
        CHECK_THAT(cVal1A->GetValue(0), WithinAbs(10.0, 1e-9));
        CHECK_THAT(cVal1B->GetValue(0), WithinAbs(20.0, 1e-9));
        CHECK_THAT(cVal2A->GetValue(0), WithinAbs(1.0, 1e-9));
        CHECK_THAT(cVal2B->GetValue(0), WithinAbs(2.0, 1e-9));
        }
    // r2 row (missing B gets fill 0.0)
    CHECK(wide->GetIdColumn().GetValue(1) == "r2");
    if (cVal1A != wide->GetContinuousColumns().cend())
        {
        CHECK_THAT(cVal1A->GetValue(1), WithinAbs(30.0, 1e-9));
        CHECK_THAT(cVal1B->GetValue(1), WithinAbs(0.0, 1e-9));
        CHECK_THAT(cVal2A->GetValue(1), WithinAbs(3.0, 1e-9));
        CHECK_THAT(cVal2B->GetValue(1), WithinAbs(0.0, 1e-9));
        }
    }

TEST_CASE("PivotWider: frequency mode with no valuesFrom", "[Pivot][Wider]")
    {
    // Expect counts per Group when valuesFrom is empty
    Dataset src;
    src.GetIdColumn().SetName("ID");

    ColumnWithStringTable::StringTableType groupST;
    groupST.insert({ 0, "A" });
    groupST.insert({ 1, "B" });
    src.AddCategoricalColumn("Group", groupST);

    // Two A's and one B for r1, one A for r2
    AddRow(src, "r1", { 0 }, {}); // A
    AddRow(src, "r1", { 0 }, {}); // A
    AddRow(src, "r1", { 1 }, {}); // B
    AddRow(src, "r2", { 0 }, {}); // A

    auto wide = Pivot::PivotWider(std::make_shared<const Dataset>(src),
                                  /*IdColumns*/ { "ID" },
                                  /*namesFrom*/ "Group",
                                  /*valuesFrom*/ { /* none */ },
                                  /*namesSep*/ "_",
                                  /*namesPrefix*/ "N_",
                                  /*fillValue*/ 0.0);

    const auto cNA = wide->GetContinuousColumn("N_A");
    const auto cNB = wide->GetContinuousColumn("N_B");
    CHECK(cNA != wide->GetContinuousColumns().cend());
    CHECK(cNB != wide->GetContinuousColumns().cend());

    CHECK(wide->GetRowCount() == 2);

    // r1: A=2, B=1
    CHECK(wide->GetIdColumn().GetValue(0) == "r1");
    CHECK_THAT(cNA->GetValue(0), WithinAbs(2.0, 1e-12));
    CHECK_THAT(cNB->GetValue(0), WithinAbs(1.0, 1e-12));

    // r2: A=1, B=0
    CHECK(wide->GetIdColumn().GetValue(1) == "r2");
    CHECK_THAT(cNA->GetValue(1), WithinAbs(1.0, 1e-12));
    CHECK_THAT(cNB->GetValue(1), WithinAbs(0.0, 1e-12));
    }

// --------------------------- PivotLonger ---------------------------

TEST_CASE("PivotLonger: single namesTo with stacked values", "[Pivot][Longer]")
    {
    // Source:
    //  ID | Q1 | Q2
    //  r1 | 10 | 20
    //  r2 | 30 | 40
    Dataset src;
    src.GetIdColumn().SetName("ID");
    src.AddContinuousColumn("Q1");
    src.AddContinuousColumn("Q2");

    AddRow(src, "r1", {}, { 10, 20 });
    AddRow(src, "r2", {}, { 30, 40 });

    // Keep ID; pivot Q1,Q2 into (Quarter, Value)
    auto longDS = Pivot::PivotLonger(std::make_shared<const Dataset>(src),
                                     /*columnsToKeep*/ { "ID" },
                                     /*fromColumns*/ { "Q1", "Q2" },
                                     /*namesTo*/ { "Quarter" },
                                     /*valuesTo*/ "Value");

    // Expect 4 rows (2x each input row)
    CHECK(longDS->GetRowCount() == 4);

    const auto quarterCol = longDS->GetCategoricalColumn("Quarter");
    const auto valueCol = longDS->GetContinuousColumn("Value");
    CHECK(quarterCol != longDS->GetCategoricalColumns().cend());
    CHECK(valueCol != longDS->GetContinuousColumns().cend());

    // r1-Q1, r1-Q2, r2-Q1, r2-Q2
    CHECK(longDS->GetIdColumn().GetValue(0) == "r1");
    CHECK(quarterCol->GetValueAsLabel(0) == "Q1");
    CHECK_THAT(valueCol->GetValue(0), WithinAbs(10.0, 1e-9));

    CHECK(longDS->GetIdColumn().GetValue(1) == "r1");
    CHECK(quarterCol->GetValueAsLabel(1) == "Q2");
    CHECK_THAT(valueCol->GetValue(1), WithinAbs(20.0, 1e-9));

    CHECK(longDS->GetIdColumn().GetValue(2) == "r2");
    CHECK(quarterCol->GetValueAsLabel(2) == "Q1");
    CHECK_THAT(valueCol->GetValue(2), WithinAbs(30.0, 1e-9));

    CHECK(longDS->GetIdColumn().GetValue(3) == "r2");
    CHECK(quarterCol->GetValueAsLabel(3) == "Q2");
    CHECK_THAT(valueCol->GetValue(3), WithinAbs(40.0, 1e-9));
    }

TEST_CASE("PivotLonger: multiple namesTo via regex split", "[Pivot][Longer]")
    {
    // Source:
    //  ID | M1 | M2
    //  a  |  5 |  7
    //  b  |  9 | 11
    Dataset src;
    src.GetIdColumn().SetName("ID");
    src.AddContinuousColumn("M1");
    src.AddContinuousColumn("M2");
    AddRow(src, "a", {}, { 5, 7 });
    AddRow(src, "b", {}, { 9, 11 });

    // Keep ID; pivot M1,M2 into (Metric, Index, Value), splitting names by regex:
    //  "M1" -> Metric="M", Index="1"; "M2" -> Metric="M", Index="2"
    auto longDS = Pivot::PivotLonger(std::make_shared<const Dataset>(src),
                                     /*columnsToKeep*/ { "ID" },
                                     /*fromColumns*/ { "M1", "M2" },
                                     /*namesTo*/ { "Metric", "Index" },
                                     /*valuesTo*/ "Value",
                                     /*namesPattern*/ L"([A-Za-z]+)([0-9]+)");

    CHECK(longDS->GetRowCount() == 4);

    const auto metricCol = longDS->GetCategoricalColumn("Metric");
    const auto indexCol = longDS->GetCategoricalColumn("Index");
    const auto valueCol = longDS->GetContinuousColumn("Value");

    CHECK(metricCol != longDS->GetCategoricalColumns().cend());
    CHECK(indexCol != longDS->GetCategoricalColumns().cend());
    CHECK(valueCol != longDS->GetContinuousColumns().cend());

    // Expected ordering: by ID, then M1, M2
    // a-M1
    CHECK(longDS->GetIdColumn().GetValue(0) == "a");
    CHECK(metricCol->GetValueAsLabel(0) == "M");
    CHECK(indexCol->GetValueAsLabel(0) == "1");
    CHECK_THAT(valueCol->GetValue(0), WithinAbs(5.0, 1e-9));
    // a-M2
    CHECK(longDS->GetIdColumn().GetValue(1) == "a");
    CHECK(metricCol->GetValueAsLabel(1) == "M");
    CHECK(indexCol->GetValueAsLabel(1) == "2");
    CHECK_THAT(valueCol->GetValue(1), WithinAbs(7.0, 1e-9));
    // b-M1
    CHECK(longDS->GetIdColumn().GetValue(2) == "b");
    CHECK(metricCol->GetValueAsLabel(2) == "M");
    CHECK(indexCol->GetValueAsLabel(2) == "1");
    CHECK_THAT(valueCol->GetValue(2), WithinAbs(9.0, 1e-9));
    // b-M2
    CHECK(longDS->GetIdColumn().GetValue(3) == "b");
    CHECK(metricCol->GetValueAsLabel(3) == "M");
    CHECK(indexCol->GetValueAsLabel(3) == "2");
    CHECK_THAT(valueCol->GetValue(3), WithinAbs(11.0, 1e-9));
    }
