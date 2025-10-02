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

    // Helper to add a row: no string ID column used; we rely on two categorical IDs + one value
    static void AddRow(Dataset& ds, const std::vector<GroupIdType>& cats,
                       const std::vector<double>& vals)
        {
        RowInfo r;
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

TEST_CASE("PivotWider: ID collision when concatenating labels without a separator",
          "[Pivot][Wider]")
    {
    // Build a dataset with TWO categorical ID columns whose labels collide when concatenated:
    //
    //   Row A: K1="AB", K2="C"   → "AB" + "C"  → "ABC"
    //   Row B: K1="A",  K2="BC"  → "A"  + "BC" → "ABC"
    //
    // These represent DISTINCT IDs and must remain separate rows.
    // If the implementation fuses IDs by naive concatenation, they will MERGE into one row.

    Dataset src;

    // Define categorical ID columns K1, K2
    ColumnWithStringTable::StringTableType stK1, stK2, stGroup;
    stK1.insert({ 0, "AB" });
    stK1.insert({ 1, "A" });
    stK2.insert({ 0, "C" });
    stK2.insert({ 1, "BC" });

    // namesFrom column "Group" with a single level "G"
    stGroup.insert({ 0, "G" });

    // Order matters: add K1, K2, Group in this order
    src.AddCategoricalColumn("K1", stK1);
    src.AddCategoricalColumn("K2", stK2);
    src.AddCategoricalColumn("Group", stGroup);

    // One continuous value column
    src.AddContinuousColumn("Val");

    // Two rows that should be distinct identifiers:
    // Row A: K1=AB (0), K2=C (0),  Group=G (0)  → Val=1
    // Row B: K1=A  (1), K2=BC(1),  Group=G (0)  → Val=2
    AddRow(src, /*cats*/ { 0, 0, 0 }, /*vals*/ { 1.0 });
    AddRow(src, /*cats*/ { 1, 1, 0 }, /*vals*/ { 2.0 });

    // Pivot wider using the TWO ID columns; names come from "Group"; values from "Val".
    auto wide = Pivot::PivotWider(std::make_shared<const Dataset>(src),
                                  /*IdColumns*/ { "K1", "K2" },
                                  /*namesFrom*/ "Group",
                                  /*valuesFrom*/ { "Val" },
                                  /*namesSep*/ "_",
                                  /*namesPrefix*/ wxEmptyString,
                                  /*fillValue*/ 0.0);

    // Expected behavior: TWO distinct rows (AB,C) and (A,BC).
    CHECK(wide->GetRowCount() == 2);

    // Column must exist
    const auto col = wide->GetContinuousColumn("G");
    REQUIRE(col != wide->GetContinuousColumns().cend());

    // Values should NOT be summed together in a single row.
    if (wide->GetRowCount() == 1)
        {
        CHECK_THAT(col->GetValue(0), WithinAbs(1.0, 1e-12)); // would fail (likely 3.0)
        }

    // Optional: verify IDs remain distinct (labels preserved per row)
    // NOTE: depends on how your wider output represents ID columns.
    // If K1 and K2 are preserved as categoricals, you can re-check them:
    const auto k1Col = wide->GetCategoricalColumn("K1");
    const auto k2Col = wide->GetCategoricalColumn("K2");
    if (k1Col != wide->GetCategoricalColumns().cend() &&
        k2Col != wide->GetCategoricalColumns().cend() && wide->GetRowCount() == 2)
        {
        // Row order may be implementation-defined; check as a set
        wxString r0 = k1Col->GetValueAsLabel(0) + "/" + k2Col->GetValueAsLabel(0);
        wxString r1 = k1Col->GetValueAsLabel(1) + "/" + k2Col->GetValueAsLabel(1);

        const bool seenAB_C = (r0 == "AB/C") || (r1 == "AB/C");
        const bool seenA_BC = (r0 == "A/BC") || (r1 == "A/BC");
        CHECK(seenAB_C);
        CHECK(seenA_BC);
        }
    }

TEST_CASE("PivotWider: two valuesFrom columns expand with <valueName>_<label>", "[Pivot][Wider]")
    {
    using namespace Wisteria::Data;
    using Catch::Matchers::WithinAbs;

    Dataset src;

    // ID column (string)
    src.GetIdColumn().SetName("ID");

    // namesFrom column with two categories: X, Y
    ColumnWithStringTable::StringTableType stGroup;
    stGroup.insert({ 0, "X" });
    stGroup.insert({ 1, "Y" });
    src.AddCategoricalColumn("Group", stGroup);

    // two continuous value columns
    src.AddContinuousColumn("ValA");
    src.AddContinuousColumn("ValB");

        // Row 1: ID=row1, Group=X  → ValA=10,  ValB=100
        {
        RowInfo r;
        r.Id("row1");
        r.Categoricals({ 0 }); // Group: X
        r.Continuous({ 10.0, 100.0 });
        src.AddRow(r);
        }
        // Row 2: ID=row2, Group=Y  → ValA=20,  ValB=200
        {
        RowInfo r;
        r.Id("row2");
        r.Categoricals({ 1 }); // Group: Y
        r.Continuous({ 20.0, 200.0 });
        src.AddRow(r);
        }

    // Pivot wider using Group as namesFrom, both value cols
    auto wide = Pivot::PivotWider(std::make_shared<const Dataset>(src),
                                  /*IdColumns*/ { "ID" },
                                  /*namesFrom*/ "Group",
                                  /*valuesFrom*/ { "ValA", "ValB" },
                                  /*namesSep*/ "_",
                                  /*namesPrefix*/ wxEmptyString,
                                  /*fillValue*/ 0.0);

    CHECK(wide->GetRowCount() == 2);

    // Expect four expanded columns: ValA_X, ValA_Y, ValB_X, ValB_Y
    const auto colA_X = wide->GetContinuousColumn("ValA_X");
    const auto colA_Y = wide->GetContinuousColumn("ValA_Y");
    const auto colB_X = wide->GetContinuousColumn("ValB_X");
    const auto colB_Y = wide->GetContinuousColumn("ValB_Y");

    REQUIRE(colA_X != wide->GetContinuousColumns().cend());
    REQUIRE(colA_Y != wide->GetContinuousColumns().cend());
    REQUIRE(colB_X != wide->GetContinuousColumns().cend());
    REQUIRE(colB_Y != wide->GetContinuousColumns().cend());

    // row 0 is "row1" (Group=X)
    CHECK_THAT(colA_X->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(colB_X->GetValue(0), WithinAbs(100.0, 1e-12));
    CHECK_THAT(colA_Y->GetValue(0), WithinAbs(0.0, 1e-12));
    CHECK_THAT(colB_Y->GetValue(0), WithinAbs(0.0, 1e-12));

    // row 1 is "row2" (Group=Y)
    CHECK_THAT(colA_Y->GetValue(1), WithinAbs(20.0, 1e-12));
    CHECK_THAT(colB_Y->GetValue(1), WithinAbs(200.0, 1e-12));
    CHECK_THAT(colA_X->GetValue(1), WithinAbs(0.0, 1e-12));
    CHECK_THAT(colB_X->GetValue(1), WithinAbs(0.0, 1e-12));
    }
