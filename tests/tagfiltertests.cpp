// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/tag_filter.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Tag filter", "[tag filter]")
	{
    SECTION("Has Ignore Tags")
        {
        const wchar_t* blah = L"Some text [[ignore this]] is written[[ignore]] here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("Tag At Beginning")
        {
        const wchar_t* blah = L"[[ignore this]]Some text  is written[[ignore]] here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("Tag At End")
        {
        const wchar_t* blah = L"Some text  is written[[ignore]] here.[[ignore this]]";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("No Tags")
        {
        const wchar_t* blah = L"Some text  is written here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("Empty Tags")
        {
        const wchar_t* blah = L"Some text  is written[[]] here.[[]]";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("All Tags")
        {
        const wchar_t* blah = L"[[ignore]][[ignore this]]";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"") == 0);
        }
    SECTION("Missing End Tag")
        {
        // should ignore the rest of the text
        const wchar_t* blah = L"Some text  is written[[ignore here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written") == 0);
        }
    SECTION("Missing End Tag2")
        {
        // should ignore the rest of the text
        const wchar_t* blah = L"Some text  ]]is written[[ignore here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  ]]is written") == 0);
        }
    SECTION("Has Overlapping Ignore Tags")
        {
        const wchar_t* blah = L"Some text [[ignore [[ignore]] this]] is written here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
    SECTION("Has Overlapping Ignore Tags With Missing End Tag")
        {
        // should ignore the rest of the text
        const wchar_t* blah = L"Some text [[ignore [[ignore]] this is written here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text ") == 0);
        }
    SECTION("Multiple Tags")
        {
        const wchar_t* blah = L"Some{IGNORE} text [[ignore this]] is written**ignore** here.";
        lily_of_the_valley::tag_filter tfilter;
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"**", L"**"));
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"[[", L"]]"));
        tfilter.add_filter_tag(lily_of_the_valley::text_filter_tag(L"{", L"}"));
        const wchar_t* result = tfilter(blah, std::wcslen(blah));
        CHECK(std::wcscmp(result, L"Some text  is written here.") == 0);
        }
	}

// NOLINTEND
// clang-format on
