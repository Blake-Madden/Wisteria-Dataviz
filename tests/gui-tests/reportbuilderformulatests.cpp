#include "../../src/base/reportbuilder.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <wx/uilocale.h>

using namespace Wisteria;

TEST_CASE("ReportBuilder::FormatFormulaFromUS / FormatFormulaToUS",
          "[reportbuilder][formula][locale]")
    {
    wxUILocale::UseLocaleName(L"C");

    SECTION("US locale is a no-op")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"en-US"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Add(5.2, 3.1)") == L"Add(5.2, 3.1)");
        CHECK(ReportBuilder::FormatFormulaToUS(L"Add(5.2, 3.1)") == L"Add(5.2, 3.1)");
        }

    SECTION("German locale swaps decimal and parameter separators")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Add(5.2, 3.1)") == L"Add(5,2; 3,1)");
        CHECK(ReportBuilder::FormatFormulaToUS(L"Add(5,2; 3,1)") == L"Add(5.2, 3.1)");
        }

    SECTION("Nested formulas are converted too")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Add(`{{Total(`col`)}}`, 2.5)") ==
              L"Add(`{{Total(`col`)}}`; 2,5)");
        }

    SECTION("Backtick-quoted values with a comma are left untouched")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Add(`Sales, Q1`, 5.5)") ==
              L"Add(`Sales, Q1`; 5,5)");
        CHECK(ReportBuilder::FormatFormulaToUS(L"Add(`Sales, Q1`; 5,5)") ==
              L"Add(`Sales, Q1`, 5.5)");
        }

    SECTION("Backtick-quoted values with a period are left untouched")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Total(`cost.center`)") ==
              L"Total(`cost.center`)");
        }

    SECTION("Ellipses are not mistaken for decimal separators")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Sum(value, value2, ...)") ==
              L"Sum(value; value2; ...)");
        }

    SECTION("Round trip through a non-U.S. locale is lossless")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        const wxString original{ L"Add(`Sales, Q1`, Add(1.5, 2.25))" };
        const wxString localized = ReportBuilder::FormatFormulaFromUS(original);
        CHECK(ReportBuilder::FormatFormulaToUS(localized) == original);
        }

    SECTION("French locale uses a semicolon as the parameter separator")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"fr-FR"));
        CHECK(ReportBuilder::FormatFormulaFromUS(L"GroupCount(`Gender`, `Female`)") ==
              L"GroupCount(`Gender`; `Female`)");
        }

    SECTION("Unmatched backtick still converts the rest of the string")
        {
        REQUIRE(wxUILocale::UseLocaleName(L"de-DE"));
        // once inside an (unterminated) backtick span, everything after it
        // is left alone rather than risk corrupting quoted text
        CHECK(ReportBuilder::FormatFormulaFromUS(L"Add(5.2, `oops") == L"Add(5,2; `oops");
        }

    wxUILocale::UseLocaleName(L"C");
    }
