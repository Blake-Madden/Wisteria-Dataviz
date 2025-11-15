// NOLINTBEGIN

#include "../src/util/char_traits.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>

using namespace string_util;

TEST_CASE("case_insensitive_character_traits :: character-level eq", "[ciwstr][traits]")
    {
    using traits = case_insensitive_character_traits;

    SECTION("eq returns true for letters differing only by case")
        {
        CHECK(traits::eq(L'A', L'a'));
        CHECK(traits::eq(L'z', L'Z'));
        }

    SECTION("eq returns false for different letters")
        {
        CHECK_FALSE(traits::eq(L'A', L'B'));
        CHECK_FALSE(traits::eq(L'x', L'Y'));
        }
    }

TEST_CASE("case_insensitive_character_traits :: character-level lt", "[ciwstr][traits]")
    {
    using traits = case_insensitive_character_traits;
    SECTION("lt compares letters case-insensitively")
        {
        CHECK(traits::lt(L'a', L'B')); // 'a' < 'b'
        CHECK(traits::lt(L'A', L'c'));
        }

    SECTION("lt respects lexicographical ordering")
        {
        CHECK_FALSE(traits::lt(L'C', L'a')); // 'c' < 'a' is false
        }
    }

TEST_CASE("case_insensitive_character_traits :: compare on wchar_t*", "[ciwstr][traits][ptr]")
    {
    using traits = case_insensitive_character_traits;

    const wchar_t* value1 = L"HeLlO";
    const wchar_t* value2 = L"hello";
    const wchar_t* value3 = L"hellp";

    SECTION("compare returns 0 for same letters ignoring case")
        {
        CHECK(traits::compare(value1, value2, 5) == 0);
        }

    SECTION("compare returns < 0 or > 0 correctly")
        {
        CHECK(traits::compare(value2, value3, 5) < 0);
        CHECK(traits::compare(value3, value2, 5) > 0);
        }
    }

TEST_CASE("case_insensitive_wstring basic equality", "[ciwstr]")
    {
    const case_insensitive_wstring hello_upper = L"HELLO";
    const case_insensitive_wstring hello_lower = L"hello";
    const case_insensitive_wstring mixed_case = L"HeLlO";

    SECTION("Equality ignores case")
        {
        CHECK(hello_upper == hello_lower);
        CHECK(hello_upper == mixed_case);
        CHECK(hello_lower == mixed_case);
        }

    SECTION("Inequality detects differing content")
        {
        const case_insensitive_wstring different = L"hella";
        CHECK(hello_upper != different);
        CHECK(hello_lower != different);
        }
    }

TEST_CASE("case_insensitive_wstring compares against std::wstring", "[ciwstr]")
    {
    const case_insensitive_wstring ci_hello = L"HeLLo";
    const std::wstring wide_upper = L"HELLO";
    const std::wstring wide_lower = L"hello";
    const std::wstring wide_mixed = L"hElLo";

    SECTION("Equality with std::wstring")
        {
        CHECK(ci_hello == wide_upper);
        CHECK(ci_hello == wide_lower);
        CHECK(ci_hello == wide_mixed);
        }

    SECTION("Reverse equality (wstring == ci_wstring)")
        {
        CHECK(wide_upper == ci_hello);
        CHECK(wide_lower == ci_hello);
        CHECK(wide_mixed == ci_hello);
        }

    SECTION("Inequality with std::wstring")
        {
        const std::wstring different = L"HELLo?";
        CHECK(ci_hello != different);
        CHECK(different != ci_hello);
        }
    }

TEST_CASE("case_insensitive_wstring length differences", "[ciwstr]")
    {
    const case_insensitive_wstring short_value = L"abc";
    const case_insensitive_wstring long_value = L"abcd";

    SECTION("Different lengths never compare equal")
        {
        CHECK(short_value != long_value);
        CHECK_FALSE(short_value == long_value);
        }

    SECTION("Equality only when sizes match")
        {
        CHECK(short_value == case_insensitive_wstring(L"ABC"));
        }
    }

TEST_CASE("case_insensitive_wstring ordering (if compare implements it)", "[ciwstr][ordering]")
    {
    // Only valid if your traits define case-insensitive ordering
    const case_insensitive_wstring apple = L"APPLE";
    const case_insensitive_wstring banana = L"banana";

    SECTION("Lexicographical compare is case-insensitive")
        {
        // Compare via traits directly to avoid depending on < being overloaded
        const int cmp_result = case_insensitive_character_traits::compare(
            apple.data(), banana.data(), std::min(apple.size(), banana.size()));

        CHECK(cmp_result < 0); // "apple" < "banana"
        }
    }

TEST_CASE("case_insensitive_wstring copies and assigns", "[ciwstr]")
    {
    const case_insensitive_wstring original = L"TestValue";

    SECTION("Copy constructor preserves behavior")
        {
        case_insensitive_wstring copy = original;
        CHECK(copy == original);
        CHECK(copy == L"testvalue");
        }

    SECTION("Assignment preserves behavior")
        {
        case_insensitive_wstring assigned;
        assigned = original;
        CHECK(assigned == original);
        CHECK(assigned == L"TESTVALUE");
        }
    }

TEST_CASE("case_insensitive_wstring works in containers", "[ciwstr][container]")
    {
    std::vector<case_insensitive_wstring> values = { L"alpha", L"BRAVO", L"Charlie" };

    SECTION("Sorting does not crash (ordering must be defined)")
        {
        std::sort(values.begin(), values.end(),
                  [](const auto& lhs, const auto& rhs)
                  {
                      return case_insensitive_character_traits::compare(
                                 lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size())) < 0;
                  });

        // Order should be: alpha, Bravo, Charlie (case-insensitive)
        CHECK(values[0] == L"alpha");
        CHECK(values[1] == L"BRAVO");
        CHECK(values[2] == L"Charlie");
        }
    }

// NOLINTEND
