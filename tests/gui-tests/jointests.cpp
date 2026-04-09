#include "../../src/data/dataset.h"
#include "../../src/data/join_inner.h"
#include "../../src/data/join_left.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using namespace Wisteria::Data;

namespace
    {
    // Build a simple categorical string table with given labels in order 0..N-1
    static ColumnWithStringTable::StringTableType MakeST(std::initializer_list<wxString> labs)
        {
        ColumnWithStringTable::StringTableType st;
        GroupIdType i{ 0 };
        for (const auto& s : labs)
            {
            st.insert({ i++, s });
            }
        return st;
        }

    // Convenience to add a row with optional ID, categoricals, continuous
    static void AddRow(Dataset& ds, const std::optional<wxString>& id,
                       const std::vector<GroupIdType>& cats, const std::vector<double>& conts)
        {
        RowInfo r;
        if (id)
            {
            r.Id(*id);
            }
        if (!cats.empty())
            {
            r.Categoricals(cats);
            }
        if (!conts.empty())
            {
            r.Continuous(conts);
            }
        ds.AddRow(r);
        }
    } // namespace

// -----------------------------------------------------------------------------
// 1) Basic join by ID: Right contributes a continuous column, Left row order kept
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: basic join on ID", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });
    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    // left row count preserved
    REQUIRE(out->GetRowCount() == 2);

    // new column exists
    auto r = out->GetContinuousColumn("RVal");
    REQUIRE(r != out->GetContinuousColumns().cend());

    // values aligned by ID
    CHECK(out->GetIdColumn().GetValue(0) == "A");
    CHECK_THAT(r->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK(out->GetIdColumn().GetValue(1) == "B");
    CHECK_THAT(r->GetValue(1), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 2) Duplicate keys in Right: "last wins" semantics
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: right duplicates last-wins", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "X" }, {}, { 0.0 });
    AddRow(right, wxString{ "X" }, {}, { 100.0 }); // first
    AddRow(right, wxString{ "X" }, {}, { 999.0 }); // duplicate, should override

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    auto r = out->GetContinuousColumn("RVal");
    REQUIRE(r != out->GetContinuousColumns().cend());
    CHECK_THAT(r->GetValue(0), WithinAbs(999.0, 1e-12)); // last wins
    }

// -----------------------------------------------------------------------------
// 3) Name collision: Right column collides with Left → suffix applied
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: name collision applies suffix", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    // both have a column named "Score"
    left.AddContinuousColumn("Score");
    right.AddContinuousColumn("Score");

    AddRow(left, wxString{ "K1" }, {}, { 1.0 });
    AddRow(right, wxString{ "K1" }, {}, { 2.0 });

    auto out = DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                           std::make_shared<const Dataset>(right),
                                           { { "ID", "ID" } }, L".x");

    // Left's Score still there
    auto l = out->GetContinuousColumn("Score");
    REQUIRE(l != out->GetContinuousColumns().cend());
    CHECK_THAT(l->GetValue(0), WithinAbs(1.0, 1e-12));

    // Right's Score should be suffixed
    auto r = out->GetContinuousColumn("Score.x");
    REQUIRE(r != out->GetContinuousColumns().cend());
    CHECK_THAT(r->GetValue(0), WithinAbs(2.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 4) Join by categorical columns (not ID)
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: join by categorical keys", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;

    // add categorical "Group" to both
    auto st = MakeST({ "A", "B" });
    left.AddCategoricalColumn("Group", st);
    right.AddCategoricalColumn("Group", st);

    right.AddContinuousColumn("Val");

    // Left rows
    AddRow(left, std::nullopt, { 0 }, {}); // A
    AddRow(left, std::nullopt, { 1 }, {}); // B
    // Right rows
    AddRow(right, std::nullopt, { 0 }, { 10.0 }); // A
    AddRow(right, std::nullopt, { 1 }, { 20.0 }); // B

    auto out = DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                           std::make_shared<const Dataset>(right),
                                           { { "Group", "Group" } });

    auto v = out->GetContinuousColumn("Val");
    REQUIRE(v != out->GetContinuousColumns().cend());

    // row 0: A → 10; row 1: B → 20
    CHECK_THAT(v->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(v->GetValue(1), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 5) Right-only keys are ignored; unmatched Left rows remain missing in new cols
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: ignore right-only keys; unmatched remain missing",
          "[Join][LeftJoinUnique]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    right.AddContinuousColumn("R");

    AddRow(left, wxString{ "L1" }, {}, {});
    AddRow(left, wxString{ "L2" }, {}, {});
    AddRow(right, wxString{ "NOPE" }, {}, { 42.0 }); // no match

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    auto r = out->GetContinuousColumn("R");
    REQUIRE(r != out->GetContinuousColumns().cend());

    // both rows should be missing in R
    CHECK(r->IsMissingData(0));
    CHECK(r->IsMissingData(1));
    }

// -----------------------------------------------------------------------------
// 6) Right has valid ID, Left does not; not joining by ID → copy Right's ID
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: copy right ID when left has none", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;

    // Left: no ID assigned (leave name empty), join by categorical "Key"
    auto stKey = MakeST({ "K1", "K2" });
    left.AddCategoricalColumn("Key", stKey);

    // Right: has ID "RID" and categorical "Key"
    right.GetIdColumn().SetName("RID");
    right.AddCategoricalColumn("Key", stKey);
    right.AddContinuousColumn("Val");

    // Left rows (no ID)
    AddRow(left, std::nullopt, { 0 }, {});
    AddRow(left, std::nullopt, { 1 }, {});
    // Right rows with RID set
    AddRow(right, wxString{ "r1" }, { 0 }, { 1.0 });
    AddRow(right, wxString{ "r2" }, { 1 }, { 2.0 });

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "Key", "Key" } });

    // ID column name should be copied from Right
    CHECK(out->GetIdColumn().GetName() == "RID");
    CHECK(out->GetIdColumn().GetValue(0) == "r1");
    CHECK(out->GetIdColumn().GetValue(1) == "r2");
    }

// -----------------------------------------------------------------------------
// 7) Both have IDs but not joined by them → Left ID kept; Right ID not copied
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: both have IDs; not joining by ID", "[Join][LeftJoinUnique]")
    {
    Dataset left, right;

    left.GetIdColumn().SetName("LID");
    right.GetIdColumn().SetName("RID");

    auto stKey = MakeST({ "K1", "K2" });
    left.AddCategoricalColumn("Key", stKey);
    right.AddCategoricalColumn("Key", stKey);
    right.AddContinuousColumn("Val");

    AddRow(left, wxString{ "L_A" }, { 0 }, {});
    AddRow(left, wxString{ "L_B" }, { 1 }, {});
    AddRow(right, wxString{ "R_A" }, { 0 }, { 11.0 });
    AddRow(right, wxString{ "R_B" }, { 1 }, { 22.0 });

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "Key", "Key" } });

    // Left ID should remain; Right ID should not overwrite ID column
    CHECK(out->GetIdColumn().GetName() == "LID");
    CHECK(out->GetIdColumn().GetValue(0) == "L_A");
    CHECK(out->GetIdColumn().GetValue(1) == "L_B");

    // Right's values still copied
    auto v = out->GetContinuousColumn("Val");
    REQUIRE(v != out->GetContinuousColumns().cend());
    CHECK_THAT(v->GetValue(0), WithinAbs(11.0, 1e-12));
    CHECK_THAT(v->GetValue(1), WithinAbs(22.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 8) Exceptions: nullptr datasets, empty by, empty suffix
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: throws on invalid arguments", "[Join][LeftJoinUnique][Throws]")
    {
    auto ds = std::make_shared<Dataset>();
    ds->GetIdColumn().SetName("ID");

    // nullptr left
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueLast(nullptr, ds, { { "ID", "ID" } }));

    // nullptr right
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueLast(ds, nullptr, { { "ID", "ID" } }));

    // empty 'by'
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueLast(ds, ds, {}));

    // empty suffix
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueLast(ds, ds, { { "ID", "ID" } }, wxString{}));
    }

namespace
    {
    // quick helper to build a wxDateTime(Y, M, D)
    static wxDateTime DMY(int y, wxDateTime::Month m, int d)
        {
        wxDateTime dt;
        dt.Set(d, m, y);
        return dt;
        }

    static void AddRow(Dataset& ds, const std::optional<wxString>& id,
                       const std::vector<wxDateTime>& dates, const std::vector<double>& conts = {})
        {
        RowInfo r;
        if (id)
            {
            r.Id(*id);
            }
        if (!dates.empty())
            {
            r.Dates(dates);
            }
        if (!conts.empty())
            {
            r.Continuous(conts);
            }
        ds.AddRow(r);
        }
    } // namespace

// -----------------------------------------------------------------------------
// 1) Join by ID: Right contributes a Date column; unmatched stays missing
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: date join by ID (unmatched stays missing)", "[Join][Date]")
    {
    Dataset left, right;

    // define schemas
    left.GetIdColumn().SetName("ID");
    left.AddDateColumn("LeftWhen");   // left already has its own date
    left.AddContinuousColumn("LVal"); // some payload

    right.GetIdColumn().SetName("ID");
    right.AddDateColumn("When"); // right date to copy over

    // rows
    AddRow(left, wxString{ "A" }, { DMY(2024, wxDateTime::Jan, 10) }, { 1.0 });
    AddRow(left, wxString{ "B" }, { DMY(2024, wxDateTime::Feb, 20) }, { 2.0 });
    AddRow(left, wxString{ "C" }, { DMY(2024, wxDateTime::Mar, 30) }, { 3.0 }); // will be unmatched

    AddRow(right, wxString{ "A" }, { DMY(2025, wxDateTime::Apr, 5) });
    AddRow(right, wxString{ "B" }, { DMY(2025, wxDateTime::May, 15) });
    AddRow(right, wxString{ "Z" }, { DMY(2025, wxDateTime::Jun, 25) }); // right-only, ignored

    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    // shape checks
    REQUIRE(out->GetRowCount() == 3);

    // new right date column is present (name doesn't collide here)
    const auto dateIt = out->GetDateColumn("When");
    REQUIRE(dateIt != out->GetDateColumns().cend());

    // matched rows copy the right date
    CHECK(dateIt->GetValue(0) == DMY(2025, wxDateTime::Apr, 5));  // A
    CHECK(dateIt->GetValue(1) == DMY(2025, wxDateTime::May, 15)); // B

    // unmatched left row remains missing in new date column
    CHECK(dateIt->IsMissingData(2)); // C had no match
    }

// -----------------------------------------------------------------------------
// 2) Name collision on Date column: suffix applied (e.g., ".x")
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: date name collision applies suffix", "[Join][Date][Suffix]")
    {
    Dataset left, right;

    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    // both sides have a date column called "When"
    left.AddDateColumn("When");
    right.AddDateColumn("When");

    // left rows
    AddRow(left, wxString{ "K1" }, { DMY(2020, wxDateTime::Sep, 9) });
    AddRow(left, wxString{ "K2" }, { DMY(2020, wxDateTime::Oct, 10) });

    // right rows (same IDs)
    AddRow(right, wxString{ "K1" }, { DMY(2030, wxDateTime::Nov, 11) });
    AddRow(right, wxString{ "K2" }, { DMY(2030, wxDateTime::Dec, 12) });

    // use default suffix ".x"
    auto out =
        DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    // left's "When" must remain unchanged
    const auto leftWhen = out->GetDateColumn("When");
    REQUIRE(leftWhen != out->GetDateColumns().cend());
    CHECK(leftWhen->GetValue(0) == DMY(2020, wxDateTime::Sep, 9));
    CHECK(leftWhen->GetValue(1) == DMY(2020, wxDateTime::Oct, 10));

    // right's "When" is added with suffix
    const auto rightWhen = out->GetDateColumn("When.x");
    REQUIRE(rightWhen != out->GetDateColumns().cend());
    CHECK(rightWhen->GetValue(0) == DMY(2030, wxDateTime::Nov, 11));
    CHECK(rightWhen->GetValue(1) == DMY(2030, wxDateTime::Dec, 12));
    }

namespace
    {
    static void AddRow(Dataset& ds, const wxString& id, const std::vector<GroupIdType>& cats = {},
                       const std::vector<double>& conts = {})
        {
        RowInfo r;
        r.Id(id);
        if (!cats.empty())
            {
            r.Categoricals(cats);
            }
        if (!conts.empty())
            {
            r.Continuous(conts);
            }
        ds.AddRow(r);
        }
    } // namespace

// -----------------------------------------------------------------------------
// Categorical: Left has "Group" and "Group.x"; Right also has "Group"
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: chained suffix collision (categorical)", "[Join][Suffix][Categorical]")
    {
    Dataset left, right;

    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    // Left already has both base and first-suffixed categorical columns.
    auto st = MakeST({ "A", "B" });
    left.AddCategoricalColumn("Group", st);
    left.AddCategoricalColumn("Group.x", st);

    // Give them some data so they’re legit
    AddRow(left, "K1", std::vector<GroupIdType>{ 0, 1 });
    AddRow(left, "K2", std::vector<GroupIdType>{ 1, 0 });

    // Right comes in with the original name "Group"
    right.AddCategoricalColumn("Group", st);
    AddRow(right, "K1", std::vector<GroupIdType>{ 0 });
    AddRow(right, "K2", std::vector<GroupIdType>{ 1 });

    // One-step suffixer will try to append "Group.x" again
    auto joined = DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                              std::make_shared<const Dataset>(right),
                                              { { "ID", "ID" } }, L".x");
    CHECK(joined->ContainsColumn(L"Group.x2"));
    }

// -----------------------------------------------------------------------------
// Continuous: Left has "Score" and "Score.x"; Right also has "Score"
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: chained suffix collision (continuous)", "[Join][Suffix][Continuous]")
    {
    Dataset left, right;

    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    // Left already holds base + first-suffixed continuous columns
    left.AddContinuousColumn("Score");
    left.AddContinuousColumn("Score.x");

        {
        RowInfo r;
        r.Id("A");
        r.Continuous({ 1.0, 10.0 });
        left.AddRow(r);
        }

        {
        RowInfo r;
        r.Id("B");
        r.Continuous({ 2.0, 20.0 });
        left.AddRow(r);
        }

    // Right brings another "Score"
    right.AddContinuousColumn("Score");
        {
        RowInfo r;
        r.Id("A");
        r.Continuous({ 100.0 });
        right.AddRow(r);
        }
        {
        RowInfo r;
        r.Id("B");
        r.Continuous({ 200.0 });
        right.AddRow(r);
        }

    // Attempting to add "Score" from Right → suffix to "Score.x"
    auto joined = DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                              std::make_shared<const Dataset>(right),
                                              { { "ID", "ID" } }, L".x");
    CHECK(joined->ContainsColumn(L"Score.x2"));
    }

// -----------------------------------------------------------------------------
// Date: Left has "When" and "When.x"; Right also has "When"
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUnique: chained suffix collision (date)", "[Join][Suffix][Date]")
    {
    Dataset left, right;

    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddDateColumn("When");
    left.AddDateColumn("When.x");
        // Fill a couple rows with any dates; values don’t matter for this test
        {
        RowInfo r;
        r.Id("X");
        r.Dates({ wxDateTime::Now(), wxDateTime::Now() });
        left.AddRow(r);
        RowInfo s;
        s.Id("Y");
        s.Dates({ wxDateTime::Now(), wxDateTime::Now() });
        left.AddRow(s);
        }

    right.AddDateColumn("When");
        {
        RowInfo r;
        r.Id("X");
        r.Dates({ wxDateTime::Now() });
        right.AddRow(r);
        RowInfo s;
        s.Id("Y");
        s.Dates({ wxDateTime::Now() });
        right.AddRow(s);
        }

    // Right's "When" → wants to become "When.x" by suffix
    auto joined = DatasetLeftJoin::LeftJoinUniqueLast(std::make_shared<const Dataset>(left),
                                              std::make_shared<const Dataset>(right),
                                              { { "ID", "ID" } }, L".x");
    CHECK(joined->ContainsColumn(L"When.x2"));
    }

// =============================================================================
// LeftJoinUniqueFirst tests
// =============================================================================

// -----------------------------------------------------------------------------
// 1) Basic join by ID: Right contributes a continuous column, Left row order kept
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: basic join on ID", "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });
    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });

    auto out =
        DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 2);

    auto r = out->GetContinuousColumn("RVal");
    REQUIRE(r != out->GetContinuousColumns().cend());

    CHECK(out->GetIdColumn().GetValue(0) == "A");
    CHECK_THAT(r->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK(out->GetIdColumn().GetValue(1) == "B");
    CHECK_THAT(r->GetValue(1), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 2) Duplicate keys in Right: "first wins" semantics
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: right duplicates first-wins", "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "X" }, {}, { 0.0 });
    AddRow(right, wxString{ "X" }, {}, { 100.0 }); // first — should be kept
    AddRow(right, wxString{ "X" }, {}, { 999.0 }); // duplicate, should be ignored

    auto out =
        DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    auto r = out->GetContinuousColumn("RVal");
    REQUIRE(r != out->GetContinuousColumns().cend());
    CHECK_THAT(r->GetValue(0), WithinAbs(100.0, 1e-12)); // first wins
    }

// -----------------------------------------------------------------------------
// 3) Name collision: Right column collides with Left -> suffix applied
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: name collision applies suffix",
          "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("Score");
    right.AddContinuousColumn("Score");

    AddRow(left, wxString{ "K1" }, {}, { 1.0 });
    AddRow(right, wxString{ "K1" }, {}, { 2.0 });

    auto out = DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                           std::make_shared<const Dataset>(right),
                                           { { "ID", "ID" } }, L".x");

    auto l = out->GetContinuousColumn("Score");
    REQUIRE(l != out->GetContinuousColumns().cend());
    CHECK_THAT(l->GetValue(0), WithinAbs(1.0, 1e-12));

    auto r = out->GetContinuousColumn("Score.x");
    REQUIRE(r != out->GetContinuousColumns().cend());
    CHECK_THAT(r->GetValue(0), WithinAbs(2.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 4) Join by categorical columns (not ID)
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: join by categorical keys",
          "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;

    auto st = MakeST({ "A", "B" });
    left.AddCategoricalColumn("Group", st);
    right.AddCategoricalColumn("Group", st);

    right.AddContinuousColumn("Val");

    AddRow(left, std::nullopt, { 0 }, {}); // A
    AddRow(left, std::nullopt, { 1 }, {}); // B
    AddRow(right, std::nullopt, { 0 }, { 10.0 }); // A
    AddRow(right, std::nullopt, { 1 }, { 20.0 }); // B

    auto out = DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                           std::make_shared<const Dataset>(right),
                                           { { "Group", "Group" } });

    auto v = out->GetContinuousColumn("Val");
    REQUIRE(v != out->GetContinuousColumns().cend());

    CHECK_THAT(v->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(v->GetValue(1), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 5) Right-only keys are ignored; unmatched Left rows remain missing
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: ignore right-only keys; unmatched remain missing",
          "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    right.AddContinuousColumn("R");

    AddRow(left, wxString{ "L1" }, {}, {});
    AddRow(left, wxString{ "L2" }, {}, {});
    AddRow(right, wxString{ "NOPE" }, {}, { 42.0 });

    auto out =
        DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    auto r = out->GetContinuousColumn("R");
    REQUIRE(r != out->GetContinuousColumns().cend());

    CHECK(r->IsMissingData(0));
    CHECK(r->IsMissingData(1));
    }

// -----------------------------------------------------------------------------
// 6) Exceptions: nullptr datasets, empty by, empty suffix
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: throws on invalid arguments",
          "[Join][LeftJoinUniqueFirst][Throws]")
    {
    auto ds = std::make_shared<Dataset>();
    ds->GetIdColumn().SetName("ID");

    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueFirst(nullptr, ds, { { "ID", "ID" } }));
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueFirst(ds, nullptr, { { "ID", "ID" } }));
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoinUniqueFirst(ds, ds, {}));
    REQUIRE_THROWS(
        DatasetLeftJoin::LeftJoinUniqueFirst(ds, ds, { { "ID", "ID" } }, wxString{}));
    }

// -----------------------------------------------------------------------------
// 7) Multiple duplicates: only first is kept, rest ignored
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst: multiple duplicates first-wins",
          "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "X" }, {}, { 0.0 });
    AddRow(right, wxString{ "X" }, {}, { 1.0 });   // first — kept
    AddRow(right, wxString{ "X" }, {}, { 2.0 });   // duplicate — ignored
    AddRow(right, wxString{ "X" }, {}, { 3.0 });   // duplicate — ignored

    auto out =
        DatasetLeftJoin::LeftJoinUniqueFirst(std::make_shared<const Dataset>(left),
                                    std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    auto r = out->GetContinuousColumn("RVal");
    REQUIRE(r != out->GetContinuousColumns().cend());
    CHECK_THAT(r->GetValue(0), WithinAbs(1.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 8) First vs Last: verify opposite behavior from LeftJoinUniqueLast
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoinUniqueFirst vs LeftJoinUniqueLast: opposite duplicate behavior",
          "[Join][LeftJoinUniqueFirst]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    right.AddContinuousColumn("Val");

    AddRow(left, wxString{ "K" }, {}, {});
    AddRow(right, wxString{ "K" }, {}, { 111.0 }); // first
    AddRow(right, wxString{ "K" }, {}, { 222.0 }); // last

    auto leftDs = std::make_shared<const Dataset>(left);
    auto rightDs = std::make_shared<const Dataset>(right);

    auto outFirst =
        DatasetLeftJoin::LeftJoinUniqueFirst(leftDs, rightDs, { { "ID", "ID" } });
    auto outLast =
        DatasetLeftJoin::LeftJoinUniqueLast(leftDs, rightDs, { { "ID", "ID" } });

    auto rFirst = outFirst->GetContinuousColumn("Val");
    auto rLast = outLast->GetContinuousColumn("Val");

    CHECK_THAT(rFirst->GetValue(0), WithinAbs(111.0, 1e-12)); // first wins
    CHECK_THAT(rLast->GetValue(0), WithinAbs(222.0, 1e-12));  // last wins
    }

// =============================================================================
// LeftJoin tests (standard left join with row duplication)
// =============================================================================

// -----------------------------------------------------------------------------
// 1) Basic join by ID: one-to-one, no row duplication
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: basic one-to-one join on ID", "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });
    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });

    auto out =
        DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                 std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 2);

    auto rCol = out->GetContinuousColumn("RVal");
    REQUIRE(rCol != out->GetContinuousColumns().cend());

    CHECK(out->GetIdColumn().GetValue(0) == "A");
    CHECK_THAT(rCol->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK(out->GetIdColumn().GetValue(1) == "B");
    CHECK_THAT(rCol->GetValue(1), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 2) One-to-many: left row duplicated for each matching right row
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: one-to-many duplicates left rows", "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "X" }, {}, { 5.0 });
    AddRow(right, wxString{ "X" }, {}, { 100.0 });
    AddRow(right, wxString{ "X" }, {}, { 200.0 });
    AddRow(right, wxString{ "X" }, {}, { 300.0 });

    auto out =
        DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                 std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    // one left row x three right rows = three output rows
    REQUIRE(out->GetRowCount() == 3);

    auto lCol = out->GetContinuousColumn("LVal");
    auto rCol = out->GetContinuousColumn("RVal");
    REQUIRE(lCol != out->GetContinuousColumns().cend());
    REQUIRE(rCol != out->GetContinuousColumns().cend());

    // left value duplicated across all three rows
    CHECK_THAT(lCol->GetValue(0), WithinAbs(5.0, 1e-12));
    CHECK_THAT(lCol->GetValue(1), WithinAbs(5.0, 1e-12));
    CHECK_THAT(lCol->GetValue(2), WithinAbs(5.0, 1e-12));

    // each right value present
    CHECK_THAT(rCol->GetValue(0), WithinAbs(100.0, 1e-12));
    CHECK_THAT(rCol->GetValue(1), WithinAbs(200.0, 1e-12));
    CHECK_THAT(rCol->GetValue(2), WithinAbs(300.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 3) Mixed: some left rows match many, some match one, some match none
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: mixed match counts", "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("LVal");
    right.AddContinuousColumn("RVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 }); // matches 2 right rows
    AddRow(left, wxString{ "B" }, {}, { 2.0 }); // matches 1 right row
    AddRow(left, wxString{ "C" }, {}, { 3.0 }); // no match

    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "A" }, {}, { 11.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });

    auto out =
        DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                 std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    // 2 + 1 + 1 = 4 output rows
    REQUIRE(out->GetRowCount() == 4);

    auto lCol = out->GetContinuousColumn("LVal");
    auto rCol = out->GetContinuousColumn("RVal");
    REQUIRE(lCol != out->GetContinuousColumns().cend());
    REQUIRE(rCol != out->GetContinuousColumns().cend());

    // A expanded to 2 rows
    CHECK(out->GetIdColumn().GetValue(0) == "A");
    CHECK_THAT(lCol->GetValue(0), WithinAbs(1.0, 1e-12));
    CHECK_THAT(rCol->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK(out->GetIdColumn().GetValue(1) == "A");
    CHECK_THAT(lCol->GetValue(1), WithinAbs(1.0, 1e-12));
    CHECK_THAT(rCol->GetValue(1), WithinAbs(11.0, 1e-12));

    // B: single match
    CHECK(out->GetIdColumn().GetValue(2) == "B");
    CHECK_THAT(lCol->GetValue(2), WithinAbs(2.0, 1e-12));
    CHECK_THAT(rCol->GetValue(2), WithinAbs(20.0, 1e-12));

    // C: no match, right column is missing
    CHECK(out->GetIdColumn().GetValue(3) == "C");
    CHECK_THAT(lCol->GetValue(3), WithinAbs(3.0, 1e-12));
    CHECK(rCol->IsMissingData(3));
    }

// -----------------------------------------------------------------------------
// 4) Join by categorical columns with one-to-many
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: one-to-many by categorical keys", "[Join][LeftJoin]")
    {
    Dataset left, right;

    auto st = MakeST({ "A", "B" });
    left.AddCategoricalColumn("Group", st);
    right.AddCategoricalColumn("Group", st);

    right.AddContinuousColumn("Val");

    AddRow(left, std::nullopt, { 0 }, {}); // A
    AddRow(left, std::nullopt, { 1 }, {}); // B

    AddRow(right, std::nullopt, { 0 }, { 10.0 }); // A
    AddRow(right, std::nullopt, { 0 }, { 11.0 }); // A again
    AddRow(right, std::nullopt, { 1 }, { 20.0 }); // B

    auto out = DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                         std::make_shared<const Dataset>(right),
                                         { { "Group", "Group" } });

    // A expands to 2 rows, B stays 1 = 3 total
    REQUIRE(out->GetRowCount() == 3);

    auto vCol = out->GetContinuousColumn("Val");
    REQUIRE(vCol != out->GetContinuousColumns().cend());

    CHECK_THAT(vCol->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(vCol->GetValue(1), WithinAbs(11.0, 1e-12));
    CHECK_THAT(vCol->GetValue(2), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 5) Name collision: suffix applied
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: name collision applies suffix", "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddContinuousColumn("Score");
    right.AddContinuousColumn("Score");

    AddRow(left, wxString{ "K1" }, {}, { 1.0 });
    AddRow(right, wxString{ "K1" }, {}, { 2.0 });

    auto out = DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                         std::make_shared<const Dataset>(right),
                                         { { "ID", "ID" } }, L".x");

    auto lCol = out->GetContinuousColumn("Score");
    REQUIRE(lCol != out->GetContinuousColumns().cend());
    CHECK_THAT(lCol->GetValue(0), WithinAbs(1.0, 1e-12));

    auto rCol = out->GetContinuousColumn("Score.x");
    REQUIRE(rCol != out->GetContinuousColumns().cend());
    CHECK_THAT(rCol->GetValue(0), WithinAbs(2.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// 6) Right-only keys ignored; unmatched left rows remain missing
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: ignore right-only keys; unmatched remain missing",
          "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    right.AddContinuousColumn("R");

    AddRow(left, wxString{ "L1" }, {}, {});
    AddRow(left, wxString{ "L2" }, {}, {});
    AddRow(right, wxString{ "NOPE" }, {}, { 42.0 });

    auto out =
        DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                 std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 2);

    auto rCol = out->GetContinuousColumn("R");
    REQUIRE(rCol != out->GetContinuousColumns().cend());

    CHECK(rCol->IsMissingData(0));
    CHECK(rCol->IsMissingData(1));
    }

// -----------------------------------------------------------------------------
// 7) Exceptions: nullptr datasets, empty by, empty suffix
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: throws on invalid arguments", "[Join][LeftJoin][Throws]")
    {
    auto ds = std::make_shared<Dataset>();
    ds->GetIdColumn().SetName("ID");

    REQUIRE_THROWS(DatasetLeftJoin::LeftJoin(nullptr, ds, { { "ID", "ID" } }));
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoin(ds, nullptr, { { "ID", "ID" } }));
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoin(ds, ds, {}));
    REQUIRE_THROWS(DatasetLeftJoin::LeftJoin(ds, ds, { { "ID", "ID" } }, wxString{}));
    }

// -----------------------------------------------------------------------------
// 8) Date columns carried over with one-to-many expansion
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin: date columns with one-to-many", "[Join][LeftJoin][Date]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    left.AddDateColumn("LeftWhen");
    right.AddDateColumn("RightWhen");

    AddRow(left, wxString{ "A" }, { DMY(2024, wxDateTime::Jan, 10) });
    AddRow(right, wxString{ "A" }, { DMY(2025, wxDateTime::Mar, 1) });
    AddRow(right, wxString{ "A" }, { DMY(2025, wxDateTime::Apr, 2) });

    auto out =
        DatasetLeftJoin::LeftJoin(std::make_shared<const Dataset>(left),
                                 std::make_shared<const Dataset>(right), { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 2);

    const auto leftWhen = out->GetDateColumn("LeftWhen");
    const auto rightWhen = out->GetDateColumn("RightWhen");
    REQUIRE(leftWhen != out->GetDateColumns().cend());
    REQUIRE(rightWhen != out->GetDateColumns().cend());

    // left date duplicated
    CHECK(leftWhen->GetValue(0) == DMY(2024, wxDateTime::Jan, 10));
    CHECK(leftWhen->GetValue(1) == DMY(2024, wxDateTime::Jan, 10));

    // each right date present
    CHECK(rightWhen->GetValue(0) == DMY(2025, wxDateTime::Mar, 1));
    CHECK(rightWhen->GetValue(1) == DMY(2025, wxDateTime::Apr, 2));
    }

// -----------------------------------------------------------------------------
// 9) Comparison with Unique variants: LeftJoin keeps all matches
// -----------------------------------------------------------------------------
TEST_CASE("LeftJoin vs Unique: all matches kept", "[Join][LeftJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");

    right.AddContinuousColumn("Val");

    AddRow(left, wxString{ "K" }, {}, {});
    AddRow(right, wxString{ "K" }, {}, { 111.0 });
    AddRow(right, wxString{ "K" }, {}, { 222.0 });

    auto leftDs = std::make_shared<const Dataset>(left);
    auto rightDs = std::make_shared<const Dataset>(right);

    auto outJoin =
        DatasetLeftJoin::LeftJoin(leftDs, rightDs, { { "ID", "ID" } });
    auto outFirst =
        DatasetLeftJoin::LeftJoinUniqueFirst(leftDs, rightDs, { { "ID", "ID" } });
    auto outLast =
        DatasetLeftJoin::LeftJoinUniqueLast(leftDs, rightDs, { { "ID", "ID" } });

    // unique variants collapse to 1 row; standard join keeps both
    CHECK(outFirst->GetRowCount() == 1);
    CHECK(outLast->GetRowCount() == 1);
    CHECK(outJoin->GetRowCount() == 2);

    auto rJoin = outJoin->GetContinuousColumn("Val");
    CHECK_THAT(rJoin->GetValue(0), WithinAbs(111.0, 1e-12));
    CHECK_THAT(rJoin->GetValue(1), WithinAbs(222.0, 1e-12));
    }

// =============================================================================
// Inner Join Tests
// =============================================================================

// -----------------------------------------------------------------------------
// Basic one-to-one inner join on ID
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: basic one-to-one join on ID", "[Join][InnerJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    left.AddContinuousColumn("LeftVal");
    right.GetIdColumn().SetName("ID");
    right.AddContinuousColumn("RightVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });
    AddRow(left, wxString{ "C" }, {}, { 3.0 });

    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });
    AddRow(right, wxString{ "C" }, {}, { 30.0 });

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 3);
    auto rv = out->GetContinuousColumn("RightVal");
    CHECK_THAT(rv->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(rv->GetValue(1), WithinAbs(20.0, 1e-12));
    CHECK_THAT(rv->GetValue(2), WithinAbs(30.0, 1e-12));
    auto lv = out->GetContinuousColumn("LeftVal");
    CHECK_THAT(lv->GetValue(0), WithinAbs(1.0, 1e-12));
    CHECK_THAT(lv->GetValue(1), WithinAbs(2.0, 1e-12));
    CHECK_THAT(lv->GetValue(2), WithinAbs(3.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// Unmatched rows from both sides are excluded
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: excludes unmatched rows from both sides", "[Join][InnerJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    left.AddContinuousColumn("LeftVal");
    right.GetIdColumn().SetName("ID");
    right.AddContinuousColumn("RightVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });
    AddRow(left, wxString{ "C" }, {}, { 3.0 });

    AddRow(right, wxString{ "B" }, {}, { 20.0 });
    AddRow(right, wxString{ "D" }, {}, { 40.0 });

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    // only "B" matches
    REQUIRE(out->GetRowCount() == 1);
    CHECK(out->GetIdColumn().GetValue(0) == "B");
    auto lv = out->GetContinuousColumn("LeftVal");
    CHECK_THAT(lv->GetValue(0), WithinAbs(2.0, 1e-12));
    auto rv = out->GetContinuousColumn("RightVal");
    CHECK_THAT(rv->GetValue(0), WithinAbs(20.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// One-to-many: left rows duplicated for each matching right row
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: one-to-many duplicates rows", "[Join][InnerJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    left.AddContinuousColumn("LeftVal");
    right.GetIdColumn().SetName("ID");
    right.AddContinuousColumn("RightVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(left, wxString{ "B" }, {}, { 2.0 });

    AddRow(right, wxString{ "A" }, {}, { 10.0 });
    AddRow(right, wxString{ "A" }, {}, { 11.0 });
    AddRow(right, wxString{ "A" }, {}, { 12.0 });
    AddRow(right, wxString{ "B" }, {}, { 20.0 });

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    // A matches 3 right rows, B matches 1 => 4 output rows
    REQUIRE(out->GetRowCount() == 4);
    auto rv = out->GetContinuousColumn("RightVal");
    CHECK_THAT(rv->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(rv->GetValue(1), WithinAbs(11.0, 1e-12));
    CHECK_THAT(rv->GetValue(2), WithinAbs(12.0, 1e-12));
    CHECK_THAT(rv->GetValue(3), WithinAbs(20.0, 1e-12));
    // left data duplicated
    auto lv = out->GetContinuousColumn("LeftVal");
    CHECK_THAT(lv->GetValue(0), WithinAbs(1.0, 1e-12));
    CHECK_THAT(lv->GetValue(1), WithinAbs(1.0, 1e-12));
    CHECK_THAT(lv->GetValue(2), WithinAbs(1.0, 1e-12));
    CHECK_THAT(lv->GetValue(3), WithinAbs(2.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// No matches yields empty dataset
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: no matches yields empty result", "[Join][InnerJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    left.AddContinuousColumn("LeftVal");
    right.GetIdColumn().SetName("ID");
    right.AddContinuousColumn("RightVal");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(right, wxString{ "Z" }, {}, { 99.0 });

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    CHECK(out->GetRowCount() == 0);
    }

// -----------------------------------------------------------------------------
// Join by categorical keys
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: join by categorical keys", "[Join][InnerJoin]")
    {
    auto st = MakeST({ L"X", L"Y", L"Z" });
    Dataset left, right;
    left.AddCategoricalColumn("Key", st);
    left.AddContinuousColumn("LeftVal");
    right.AddCategoricalColumn("Key", st);
    right.AddContinuousColumn("RightVal");

    AddRow(left, std::nullopt, { 0 }, { 1.0 }); // X
    AddRow(left, std::nullopt, { 1 }, { 2.0 }); // Y
    AddRow(left, std::nullopt, { 2 }, { 3.0 }); // Z

    AddRow(right, std::nullopt, { 0 }, { 10.0 }); // X
    AddRow(right, std::nullopt, { 2 }, { 30.0 }); // Z

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out =
        DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "Key", "Key" } });

    // Y has no match, so only X and Z
    REQUIRE(out->GetRowCount() == 2);
    auto rv = out->GetContinuousColumn("RightVal");
    CHECK_THAT(rv->GetValue(0), WithinAbs(10.0, 1e-12));
    CHECK_THAT(rv->GetValue(1), WithinAbs(30.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// Name collision applies suffix
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: name collision applies suffix", "[Join][InnerJoin]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    left.AddContinuousColumn("Score");
    right.GetIdColumn().SetName("ID");
    right.AddContinuousColumn("Score");

    AddRow(left, wxString{ "A" }, {}, { 1.0 });
    AddRow(right, wxString{ "A" }, {}, { 99.0 });

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    REQUIRE(out->GetRowCount() == 1);
    auto leftScore = out->GetContinuousColumn("Score");
    auto rightScore = out->GetContinuousColumn("Score.x");
    CHECK_THAT(leftScore->GetValue(0), WithinAbs(1.0, 1e-12));
    CHECK_THAT(rightScore->GetValue(0), WithinAbs(99.0, 1e-12));
    }

// -----------------------------------------------------------------------------
// Throws on invalid arguments
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: throws on invalid arguments", "[Join][InnerJoin][Throws]")
    {
    auto ds = std::make_shared<Dataset>();
    CHECK_THROWS(DatasetInnerJoin::InnerJoin(nullptr, ds, { { "ID", "ID" } }));
    CHECK_THROWS(DatasetInnerJoin::InnerJoin(ds, nullptr, { { "ID", "ID" } }));
    CHECK_THROWS(DatasetInnerJoin::InnerJoin(ds, ds, {}));
    CHECK_THROWS(DatasetInnerJoin::InnerJoin(ds, ds, { { "ID", "ID" } }, L""));
    }

// -----------------------------------------------------------------------------
// Date columns preserved in inner join
// -----------------------------------------------------------------------------
TEST_CASE("InnerJoin: date columns", "[Join][InnerJoin][Date]")
    {
    Dataset left, right;
    left.GetIdColumn().SetName("ID");
    right.GetIdColumn().SetName("ID");
    right.AddDateColumn("EventDate");

    AddRow(left, wxString{ "A" }, {}, {});
    AddRow(left, wxString{ "B" }, {}, {});

    RowInfo rr;
    rr.Id("A");
    rr.Dates({ wxDateTime(1, wxDateTime::Jan, 2025) });
    right.AddRow(rr);

    auto leftDs = std::make_shared<Dataset>(std::move(left));
    auto rightDs = std::make_shared<Dataset>(std::move(right));
    auto out = DatasetInnerJoin::InnerJoin(leftDs, rightDs, { { "ID", "ID" } });

    // only A matches
    REQUIRE(out->GetRowCount() == 1);
    CHECK(out->GetIdColumn().GetValue(0) == "A");
    auto dc = out->GetDateColumn("EventDate");
    CHECK(dc->GetValue(0) == wxDateTime(1, wxDateTime::Jan, 2025));
    }
