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
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(nullptr, 1) == false);
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(L"text", 0) == false);
        CPPUNIT_ASSERT(encode(nullptr, 1, true) == L"");
        CPPUNIT_ASSERT(encode(L"text", 0, true) == L"");
        }
    SECTION("Plain Text")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello, world";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)) == false);
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello, world");
        }
    SECTION("Whitespace")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello\tworld";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello&nbsp;&nbsp;&nbsp;world");
        text = L"hello\nworld";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello<p></p>world");
        text = L"hello\n\rworld";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello<p></p>world");
        text = L"hello    world";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello &nbsp;&nbsp;&nbsp;world");
        text = L"hello  world";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello &nbsp;world");
        }
    SECTION("Illegal Symbols")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello&<>\"\'world";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"hello&#38;&#60;&#62;&#34;&#39;world");
        }
    SECTION("Unicode")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"heâllo\x432";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)));
        CPPUNIT_ASSERT(encode(text, std::wcslen(text), true) == L"he&#226;llo&#1074;");
        }
    SECTION("Size Not Specified")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello, world";
        CPPUNIT_ASSERT(encode.needs_to_be_encoded(text, std::wcslen(text)) == false);
        CPPUNIT_ASSERT(encode(text, static_cast<size_t>(-1), true) == L"hello, world");
        }
    }