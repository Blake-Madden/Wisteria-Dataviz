// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/html_encode.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("HTML Encode", "[html encode]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::html_encode_text encode;
        CHECK_FALSE(encode.needs_to_be_encoded({ L"text", 0 }));
        CHECK(encode({ L"text", 0 }, true) == L"");
        }
    SECTION("Plain Text")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello, world";
        CHECK_FALSE(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello, world");
        }
    SECTION("Whitespace")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello\tworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello&nbsp;&nbsp;&nbsp;world");
        text = L"hello\nworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello\n\rworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello    world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;&nbsp;&nbsp;world");
        text = L"hello  world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;world");
        }
    SECTION("Illegal Symbols")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello&<>\"\'world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello&#38;&#60;&#62;&#34;&#39;world");
        }
    SECTION("Unicode")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"heâllo\x432";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"he&#226;llo&#1074;");
        }
    }

// NOLINTEND
// clang-format on
