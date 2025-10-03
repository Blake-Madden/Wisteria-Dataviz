#include "../../src/data/clone.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace
    {
    // Build a simple string table {0:"...", 1:"...", ...}
    static Wisteria::Data::ColumnWithStringTable::StringTableType
    MakeST(std::initializer_list<wxString> labels)
        {
        Wisteria::Data::ColumnWithStringTable::StringTableType st;
        Wisteria::Data::GroupIdType id{ 0 };
        for (const auto& s : labels)
            {
            st.insert({ id++, s });
            }
        return st;
        }

    static wxDateTime DMY(int y, wxDateTime::Month m, int d)
        {
        wxDateTime dt;
        dt.Set(d, m, y);
        return dt;
        }

    // Find columns by name via public containers (no private getters)
    static auto FindCat(const Wisteria::Data::Dataset& ds, const wxString& name)
        {
        const auto& cols = ds.GetCategoricalColumns();
        return std::find_if(cols.cbegin(), cols.cend(),
                            [&](const auto& c) { return c.GetName().CmpNoCase(name) == 0; });
        }

    static auto FindCont(const Wisteria::Data::Dataset& ds, const wxString& name)
        {
        const auto& cols = ds.GetContinuousColumns();
        return std::find_if(cols.cbegin(), cols.cend(),
                            [&](const auto& c) { return c.GetName().CmpNoCase(name) == 0; });
        }

    static auto FindDate(const Wisteria::Data::Dataset& ds, const wxString& name)
        {
        const auto& cols = ds.GetDateColumns();
        return std::find_if(cols.cbegin(), cols.cend(),
                            [&](const auto& c) { return c.GetName().CmpNoCase(name) == 0; });
        }

    // A tiny helper that uses the protected DatasetClone API to filter rows.
    class EvenRowCloner : public Wisteria::Data::DatasetClone
        {
      public:
        std::shared_ptr<Wisteria::Data::Dataset>
        CloneEvenRowsOnly(const std::shared_ptr<const Wisteria::Data::Dataset>& src)
            {
            SetSourceData(src);
            while (HasMoreRows())
                {
                const auto pos = GetNextRowPosition();
                if (pos && ((*pos) % 2 == 0))
                    {
                    CopyNextRow();
                    }
                else
                    {
                    SkipNextRow();
                    }
                }
            return GetClone();
            }
        };
    } // namespace

// -----------------------------------------------------------------------------
// 1) Clone() without SetSourceData() returns nullptr
// -----------------------------------------------------------------------------
TEST_CASE("DatasetClone: Clone without SetSourceData returns null", "[Clone][Basics]")
    {
    Wisteria::Data::DatasetClone cloner;
    auto out = cloner.Clone();
    CHECK_FALSE(out);
    }

// -----------------------------------------------------------------------------
// 2) Full-fidelity clone: schema (names, order, string tables) and data preserved
// -----------------------------------------------------------------------------
TEST_CASE("DatasetClone: full clone preserves schema and data", "[Clone][Schema][Data]")
    {
    using namespace Wisteria::Data;

    // Build source dataset
    auto src = std::make_shared<Dataset>();

    // ID + columns
    src->GetIdColumn().SetName("ID");

    auto stCat = MakeST({ "Red", "Green", "Blue" });
    src->AddCategoricalColumn("Color", stCat);
    src->AddContinuousColumn("Score");
    src->AddDateColumn("When");

        // rows
        {
        RowInfo r;
        r.Id("a");
        r.Categoricals({ 0 });
        r.Continuous({ 1.5 });
        r.Dates({ DMY(2020, wxDateTime::Jan, 1) });
        src->AddRow(r);
        }
        {
        RowInfo r;
        r.Id("b");
        r.Categoricals({ 1 });
        r.Continuous({ 2.5 });
        r.Dates({ DMY(2021, wxDateTime::Feb, 2) });
        src->AddRow(r);
        }
        {
        RowInfo r;
        r.Id("c");
        r.Categoricals({ 2 });
        r.Continuous({ 3.5 });
        r.Dates({ DMY(2022, wxDateTime::Mar, 3) });
        src->AddRow(r);
        }

    // Clone
    DatasetClone cloner;
    cloner.SetSourceData(src);
    auto out = cloner.Clone();

    REQUIRE(out);
    REQUIRE(out->GetRowCount() == src->GetRowCount());

    // Column names and order preserved
    CHECK(out->GetIdColumn().GetName() == "ID");
    REQUIRE(out->GetCategoricalColumns().size() == 1);
    REQUIRE(out->GetContinuousColumns().size() == 1);
    REQUIRE(out->GetDateColumns().size() == 1);
    CHECK(out->GetCategoricalColumns()[0].GetName() == "Color");
    CHECK(out->GetContinuousColumns()[0].GetName() == "Score");
    CHECK(out->GetDateColumns()[0].GetName() == "When");

    // String table equality (by labels)
    const auto srcCat = FindCat(*src, "Color");
    const auto outCat = FindCat(*out, "Color");
    REQUIRE(srcCat != src->GetCategoricalColumns().cend());
    REQUIRE(outCat != out->GetCategoricalColumns().cend());
    const auto& srcST = srcCat->GetStringTable();
    const auto& outST = outCat->GetStringTable();
    CHECK(srcST.size() == outST.size());
    for (const auto& kv : srcST)
        {
        auto it = outST.find(kv.first);
        REQUIRE(it != outST.end());
        CHECK(it->second == kv.second);
        }

    // Data equality row-by-row
    const auto outCont = FindCont(*out, "Score");
    const auto outDate = FindDate(*out, "When");
    REQUIRE(outCont != out->GetContinuousColumns().cend());
    REQUIRE(outDate != out->GetDateColumns().cend());

    const auto srcCont = FindCont(*src, "Score");
    const auto srcDate = FindDate(*src, "When");
    REQUIRE(srcCont != src->GetContinuousColumns().cend());
    REQUIRE(srcDate != src->GetDateColumns().cend());

    for (size_t i = 0; i < src->GetRowCount(); ++i)
        {
        CHECK(out->GetIdColumn().GetValue(i) == src->GetIdColumn().GetValue(i));
        CHECK(outCat->GetValue(i) == srcCat->GetValue(i));
        CHECK(outCont->GetValue(i) == srcCont->GetValue(i));
        CHECK(outDate->GetValue(i) == srcDate->GetValue(i));
        }
    }

// -----------------------------------------------------------------------------
// 3) Skip path: use protected API to clone only even rows
// -----------------------------------------------------------------------------
TEST_CASE("DatasetClone: custom cloning routine can SkipNextRow()", "[Clone][Skip]")
    {
    using namespace Wisteria::Data;

    // Source with 5 rows
    auto src = std::make_shared<Dataset>();
    src->GetIdColumn().SetName("ID");
    src->AddContinuousColumn("V");

    for (int i = 0; i < 5; ++i)
        {
        RowInfo r;
        r.Id(wxString::Format("row%d", i));
        r.Continuous({ static_cast<double>(i) });
        src->AddRow(r);
        }

    EvenRowCloner custom;
    auto out = custom.CloneEvenRowsOnly(src);

    REQUIRE(out);
    CHECK(out->GetRowCount() == 3); // rows 0,2,4

    const auto v = FindCont(*out, "V");
    REQUIRE(v != out->GetContinuousColumns().cend());

    // Verify the kept rows are the even-indexed ones with matching values
    CHECK(out->GetIdColumn().GetValue(0) == "row0");
    CHECK(v->GetValue(0) == 0.0);

    CHECK(out->GetIdColumn().GetValue(1) == "row2");
    CHECK(v->GetValue(1) == 2.0);

    CHECK(out->GetIdColumn().GetValue(2) == "row4");
    CHECK(v->GetValue(2) == 4.0);
    }

// -----------------------------------------------------------------------------
// 4) Empty dataset: schema only, zero rows
// -----------------------------------------------------------------------------
TEST_CASE("DatasetClone: empty dataset clones schema, no rows", "[Clone][Empty]")
    {
    using namespace Wisteria::Data;

    auto src = std::make_shared<Dataset>();
    src->GetIdColumn().SetName("ID");
    src->AddCategoricalColumn("Cat", MakeST({ "A", "B" }));
    src->AddContinuousColumn("X");
    src->AddDateColumn("D");

    DatasetClone cloner;
    cloner.SetSourceData(src);
    auto out = cloner.Clone();

    REQUIRE(out);
    CHECK(out->GetRowCount() == 0);

    // Schema preserved
    CHECK(out->GetIdColumn().GetName() == "ID");
    REQUIRE(out->GetCategoricalColumns().size() == 1);
    REQUIRE(out->GetContinuousColumns().size() == 1);
    REQUIRE(out->GetDateColumns().size() == 1);
    CHECK(out->GetCategoricalColumns()[0].GetName() == "Cat");
    CHECK(out->GetContinuousColumns()[0].GetName() == "X");
    CHECK(out->GetDateColumns()[0].GetName() == "D");

    // String table carried over
    const auto cat = out->GetCategoricalColumns()[0];
    const auto& st = cat.GetStringTable();
    REQUIRE(st.size() == 2);
    CHECK(st.at(0) == "A");
    CHECK(st.at(1) == "B");
    }

// -----------------------------------------------------------------------------
// 5) Preserves row order and sizes
// -----------------------------------------------------------------------------
TEST_CASE("DatasetClone: preserves row order and sizes", "[Clone][Order]")
    {
    using namespace Wisteria::Data;

    auto src = std::make_shared<Dataset>();
    src->GetIdColumn().SetName("ID");
    src->AddContinuousColumn("Y");

    for (int i = 0; i < 10; ++i)
        {
        RowInfo r;
        r.Id(wxString::Format("k%02d", i));
        r.Continuous({ i * 0.5 });
        src->AddRow(r);
        }

    DatasetClone cloner;
    cloner.SetSourceData(src);
    auto out = cloner.Clone();

    REQUIRE(out);
    CHECK(out->GetRowCount() == src->GetRowCount());

    const auto yOut = FindCont(*out, "Y");
    const auto ySrc = FindCont(*src, "Y");
    REQUIRE(yOut != out->GetContinuousColumns().cend());
    REQUIRE(ySrc != src->GetContinuousColumns().cend());

    for (size_t i = 0; i < src->GetRowCount(); ++i)
        {
        CHECK(out->GetIdColumn().GetValue(i) == src->GetIdColumn().GetValue(i));
        CHECK(yOut->GetValue(i) == ySrc->GetValue(i));
        }
    }
