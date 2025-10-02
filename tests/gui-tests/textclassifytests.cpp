#include "../../src/data/textclassifier.h"
#include <catch2/catch_test_macros.hpp>

using namespace Wisteria::Data;

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

    // Convenience: find categorical column by name (const iterator)
    template<typename DS>
    static auto Cat(const DS& ds, const wxString& name)
        {
        return ds.GetCategoricalColumn(name);
        }

    // Normalize “missing” subcategory to empty string for stable compares
    static std::tuple<wxString, wxString, wxString> Row3(const Wisteria::Data::Dataset& ds,
                                                         size_t row, const wxString& c0,
                                                         const wxString& c1, const wxString& c2)
        {
        const auto a = Cat(ds, c0);
        const auto b = Cat(ds, c1);
        const auto c = Cat(ds, c2);

        wxString sub{};
        if (c != ds.GetCategoricalColumns().cend())
            {
            sub = c->IsMissingData(row) ? wxString{} : c->GetValueAsLabel(row);
            }

        return { a->GetValueAsLabel(row), b->GetValueAsLabel(row), sub };
        }

    // Collect rows into an ordered vector of tuples for order-independent checks
    static std::vector<std::tuple<wxString, wxString, wxString>>
    CollectTriples(const std::shared_ptr<Wisteria::Data::Dataset>& ds, const wxString& a,
                   const wxString& b, const wxString& c)
        {
        std::vector<std::tuple<wxString, wxString, wxString>> out;
        out.reserve(ds->GetRowCount());
        for (size_t i = 0; i < ds->GetRowCount(); ++i)
            {
            out.push_back(Row3(*ds, i, a, b, c));
            }
        std::sort(out.begin(), out.end());
        return out;
        }
    } // namespace

// -----------------------------------------------------------------------------
// 1) Full classifier: categories, subcategories, negation; multi-match per row
// -----------------------------------------------------------------------------
TEST_CASE("TextClassifier: classify with categories, subcategories, and negation",
          "[TextClassifier][Regex][Subcategory][Negation]")
    {
    // ---- Build classifier dataset (with explicit missing IDs) ---------------
    Dataset classifier;

    auto stCat = MakeST({ "Facilities", "Athletics", "Food & Beverage", "Programs" });
    auto stSub = MakeST({ "Parking Lot", "CompSci", "Engineering" });
    auto stPat = MakeST({
        "(?i)stadium",                     // 0
        "(?i)\\bparking",                  // 1
        "(?i)(foot|basket|base|soft)ball", // 2
        "(?i)stadium",                     // 3 (again, for Athletics)
        "(?i)\\bfood\\b",                  // 4
        "(?i)pretzel",                     // 5
        "(?i)software",                    // 6
        "(?i)engineer"                     // 7
    });
    auto stNeg = MakeST({ "(?i)software" });

    // add explicit missing entries (empty label) to SUBCATEGORY and NEGATE
    const auto subMD = ColumnWithStringTable::GetNextKey(stSub);
    stSub.insert({ subMD, wxString{} });
    const auto negMD = ColumnWithStringTable::GetNextKey(stNeg);
    stNeg.insert({ negMD, wxString{} });

    classifier.AddCategoricalColumn("CATEGORY", stCat);
    classifier.AddCategoricalColumn("SUBCATEGORY", stSub);
    classifier.AddCategoricalColumn("PATTERN", stPat);
    classifier.AddCategoricalColumn("NEGATE_PATTERN", stNeg);

    // rows: use subMD / negMD for “no subcategory/negation”
    classifier.AddRow(
        RowInfo().Categoricals({ /*Facilities*/ 0, subMD, /*stadium*/ 0, /*neg*/ negMD }));
    classifier.AddRow(RowInfo().Categoricals(
        { /*Facilities*/ 0, /*Parking Lot*/ 0, /*parking*/ 1, /*neg*/ negMD }));
    classifier.AddRow(
        RowInfo().Categoricals({ /*Athletics*/ 1, subMD, /*ball*/ 2, /*neg*/ negMD }));
    classifier.AddRow(
        RowInfo().Categoricals({ /*Athletics*/ 1, subMD, /*stadium*/ 3, /*neg*/ negMD }));
    classifier.AddRow(RowInfo().Categoricals({ /*Food*/ 2, subMD, /*food*/ 4, /*neg*/ negMD }));
    classifier.AddRow(RowInfo().Categoricals({ /*Food*/ 2, subMD, /*pretzel*/ 5, /*neg*/ negMD }));
    classifier.AddRow(
        RowInfo().Categoricals({ /*Programs*/ 3, /*CompSci*/ 1, /*software*/ 6, /*neg*/ negMD }));
    classifier.AddRow(RowInfo().Categoricals(
        { /*Programs*/ 3, /*Engineering*/ 2, /*engineer*/ 7, /*neg*/ 0 })); // negated by software

    // ---- Build content dataset ---------------------------------------------
    Dataset content;
    auto stComments = MakeST({
        L"I love the football games. The stadium needs more seats though.",    // 0
        L"The parking lot is hard to find.",                                   // 1
        L"Wish they had hot pretzels at the softball games.",                  // 2
        L"The printer in the library was out of paper :(",                     // 3
        L"More classes for engineering (software development) would be nice.", // 4
        L"The chemical engineering classes are too tough."                     // 5
    });
    content.AddCategoricalColumn("COMMENTS", stComments);
    for (GroupIdType gid = 0; gid < static_cast<GroupIdType>(stComments.size()); ++gid)
        {
        content.AddRow(RowInfo().Categoricals({ gid }));
        }

    // ---- Run classifier -----------------------------------------------------
    TextClassifier tc;
    tc.SetClassifierData(std::make_shared<const Dataset>(classifier), "CATEGORY", "SUBCATEGORY",
                         "PATTERN", "NEGATE_PATTERN");

    auto [classified, unclassified] =
        tc.ClassifyData(std::make_shared<const Dataset>(content), "COMMENTS");

    REQUIRE(classified);
    REQUIRE(unclassified);

    // ---- Validate results ---------------------------------------------------
    auto got = CollectTriples(classified, "COMMENTS", "CATEGORY", "SUBCATEGORY");

    std::vector<std::tuple<wxString, wxString, wxString>> expected = {
        { stComments.at(0), "Athletics", wxString{} },
        { stComments.at(0), "Facilities", wxString{} },
        { stComments.at(1), "Facilities", "Parking Lot" },
        { stComments.at(2), "Athletics", wxString{} },
        { stComments.at(2), "Food & Beverage", wxString{} },
        { stComments.at(4), "Programs", "CompSci" },
        { stComments.at(5), "Programs", "Engineering" }
    };
    std::sort(expected.begin(), expected.end());

    if (got.size() != expected.size())
        {
        INFO("got.size()=" << got.size() << " expected.size()=" << expected.size());
        for (const auto& t : got)
            {
            INFO("got: [" << std::get<0>(t) << "] / [" << std::get<1>(t) << "] / ["
                          << std::get<2>(t) << "]");
            }
        for (const auto& t : expected)
            {
            INFO("exp: [" << std::get<0>(t) << "] / [" << std::get<1>(t) << "] / ["
                          << std::get<2>(t) << "]");
            }
        }
    CHECK(got == expected);

    // Unclassified should be just the printer comment (row 3)
    REQUIRE(unclassified->GetRowCount() == 1);
    const auto uc = Cat(*unclassified, "COMMENTS");
    REQUIRE(uc != unclassified->GetCategoricalColumns().cend());
    CHECK(uc->GetValueAsLabel(0) == stComments.at(3));
    }

// -----------------------------------------------------------------------------
// 2) No subcategory / no negation: single category column output
// -----------------------------------------------------------------------------
TEST_CASE("TextClassifier: classify without subcategories or negation",
          "[TextClassifier][Regex][NoSubcategory]")
    {
    using namespace Wisteria::Data;

    // Classifier with just CATEGORY + PATTERN
    Dataset classifier;
    auto stCat = MakeST({ "Fruit", "Veg" });
    auto stPat = MakeST({
        "(?i)\\bapple\\b",
        "(?i)\\bcarrots?\\b" // allow singular/plural
    });

    classifier.AddCategoricalColumn("CATEGORY", stCat);
    classifier.AddCategoricalColumn("PATTERN", stPat);

    // Fruit→apple
    classifier.AddRow(RowInfo().Categoricals({ 0, 0 }));
    // Veg→carrot
    classifier.AddRow(RowInfo().Categoricals({ 1, 1 }));

    // Content
    Dataset content;
    auto stC = MakeST({ L"apple pie", L"baby carrots", L"no produce here" });
    content.AddCategoricalColumn("COMMENTS", stC);
    content.AddRow(RowInfo().Categoricals({ 0 }));
    content.AddRow(RowInfo().Categoricals({ 1 }));
    content.AddRow(RowInfo().Categoricals({ 2 }));

    TextClassifier tc;
    tc.SetClassifierData(std::make_shared<const Dataset>(classifier), "CATEGORY",
                         /*sub*/ std::nullopt, "PATTERN", /*neg*/ std::nullopt);

    auto [classified, unclassified] =
        tc.ClassifyData(std::make_shared<const Dataset>(content), "COMMENTS");

    REQUIRE(classified);
    REQUIRE(unclassified);

    // Should only have 2 columns in classified: COMMENTS + CATEGORY
    CHECK(classified->GetCategoricalColumns().size() == 2);

    auto got = CollectTriples(classified, "COMMENTS", "CATEGORY", /*no sub*/ wxString{});
    std::vector<std::tuple<wxString, wxString, wxString>> expected = {
        { stC.at(0), "Fruit", wxString{} }, { stC.at(1), "Veg", wxString{} }
    };
    std::sort(expected.begin(), expected.end());
    CHECK(got == expected);

    // Unclassified should contain the third comment
    REQUIRE(unclassified->GetRowCount() == 1);
    const auto uc = Cat(*unclassified, "COMMENTS");
    CHECK(uc->GetValueAsLabel(0) == stC.at(2));
    }

// -----------------------------------------------------------------------------
// 3) Missing columns → throws
// -----------------------------------------------------------------------------
TEST_CASE("TextClassifier: throws when named columns are missing", "[TextClassifier][Throws]")
    {
    using namespace Wisteria::Data;

    Dataset classifier, content;

    TextClassifier tc;

    // SetClassifierData: category column missing
    REQUIRE_THROWS(tc.SetClassifierData(std::make_shared<const Dataset>(classifier), "CATEGORY",
                                        std::nullopt, "PATTERN", std::nullopt));

    // Build a minimal classifier correctly to get past SetClassifierData
    auto stCat = MakeST({ "X" });
    auto stPat = MakeST({ ".*" });
    classifier.AddCategoricalColumn("CATEGORY", stCat);
    classifier.AddCategoricalColumn("PATTERN", stPat);
    classifier.AddRow(RowInfo().Categoricals({ 0, 0 }));
    REQUIRE_NOTHROW(tc.SetClassifierData(std::make_shared<const Dataset>(classifier), "CATEGORY",
                                         std::nullopt, "PATTERN", std::nullopt));

    // ClassifyData: content column missing
    REQUIRE_THROWS(tc.ClassifyData(std::make_shared<const Dataset>(content), "COMMENTS"));
    }

// -----------------------------------------------------------------------------
// 4) No classifier loaded (or all invalid patterns) → returns {nullptr, nullptr}
// -----------------------------------------------------------------------------
TEST_CASE("TextClassifier: returns null outputs when no patterns available",
          "[TextClassifier][Empty]")
    {
    using namespace Wisteria::Data;

    TextClassifier tc;

    // Content has a COMMENTS column, but we will not load any classifier
    Dataset content;
    auto stC = MakeST({ L"anything" });
    content.AddCategoricalColumn("COMMENTS", stC);
    content.AddRow(RowInfo().Categoricals({ 0 }));

    auto [classified, unclassified] =
        tc.ClassifyData(std::make_shared<const Dataset>(content), "COMMENTS");

    CHECK_FALSE(classified);
    CHECK_FALSE(unclassified);
    }

// -----------------------------------------------------------------------------
// 5) All invalid regex entries are ignored → behaves like “no classifier”
// -----------------------------------------------------------------------------
TEST_CASE("TextClassifier: invalid regex rows ignored (no effective patterns)",
          "[TextClassifier][InvalidRegex]")
    {
    using namespace Wisteria::Data;

    Dataset classifier, content;

    // CATEGORY and PATTERN exist, but pattern is invalid
    auto stCat = MakeST({ "Broken" });
    auto stPat = MakeST({ "(" }); // invalid regex
    classifier.AddCategoricalColumn("CATEGORY", stCat);
    classifier.AddCategoricalColumn("PATTERN", stPat);
    classifier.AddRow(RowInfo().Categoricals({ 0, 0 }));

    TextClassifier tc;
    // Should not throw; invalid regex rows are logged & skipped
    REQUIRE_NOTHROW(tc.SetClassifierData(std::make_shared<const Dataset>(classifier), "CATEGORY",
                                         std::nullopt, "PATTERN", std::nullopt));

    // Content
    auto stC = MakeST({ L"text" });
    content.AddCategoricalColumn("COMMENTS", stC);
    content.AddRow(RowInfo().Categoricals({ 0 }));

    // Since the only row was invalid, patterns map is empty -> null outputs
    auto [classified, unclassified] =
        tc.ClassifyData(std::make_shared<const Dataset>(content), "COMMENTS");

    CHECK_FALSE(classified);
    CHECK_FALSE(unclassified);
    }
