// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/hhc_hhk_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("HHC Import", "[hhc import]")
	{
    SECTION("Null")
        {
        hhc_hhk_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext(L"<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"Introductory Overview\">\n<param name=\"Local\" value=\"Overviews/IntroductoryOverview.htm\">\n</object>\n<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"&quot;True&quot; Mean and Confidence Interval\">\n<param name=\"Local\" value=\"Overview/TrueMeanandConfidenceInterval.htm\">\n</object>", 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Simple")
        {
        const wchar_t* text = L"<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"Introductory Overview\">\n<param name=\"Local\" value=\"Overviews/IntroductoryOverview.htm\">\n</object>\n<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"&quot;True&quot; Mean and Confidence Interval\">\n<param name=\"Local\" value=\"Overview/TrueMeanandConfidenceInterval.htm\">\n</object>";
        hhc_hhk_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Introductory Overview\n\n\"True\" Mean and Confidence Interval\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 60);
        }
    }

// NOLINTEND
// clang-format on
