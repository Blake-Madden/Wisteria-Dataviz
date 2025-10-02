#include "../../src/data/dataset.h"
#include "../../src/data/join.h"
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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

    auto out = DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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

    auto out = DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
    REQUIRE_THROWS(DatasetJoin::LeftJoinUnique(nullptr, ds, { { "ID", "ID" } }));

    // nullptr right
    REQUIRE_THROWS(DatasetJoin::LeftJoinUnique(ds, nullptr, { { "ID", "ID" } }));

    // empty 'by'
    REQUIRE_THROWS(DatasetJoin::LeftJoinUnique(ds, ds, {}));

    // empty suffix
    REQUIRE_THROWS(DatasetJoin::LeftJoinUnique(ds, ds, { { "ID", "ID" } }, wxString{}));
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
        DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
    AddRow(left, "K1", { 0, 1 });
    AddRow(left, "K2", { 1, 0 });

    // Right comes in with the original name "Group"
    right.AddCategoricalColumn("Group", st);
    AddRow(right, "K1", { 0 });
    AddRow(right, "K2", { 1 });

    // One-step suffixer will try to append "Group.x" again
    auto joined = DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
    auto joined = DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
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
    auto joined = DatasetJoin::LeftJoinUnique(std::make_shared<const Dataset>(left),
                                               std::make_shared<const Dataset>(right),
                                               { { "ID", "ID" } }, L".x");
    CHECK(joined->ContainsColumn(L"When.x2"));
    }
