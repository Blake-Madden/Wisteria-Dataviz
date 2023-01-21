#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/rtf_encode.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("RTF Encode", "[rtf encode]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::rtf_encode_text encode;
        CHECK(encode.needs_to_be_encoded({ L"text", 0 }) == false);
        CHECK(encode({ L"text", 0 }) == L"");
        }
    SECTION("Plain Text")
        {
        lily_of_the_valley::rtf_encode_text encode;
        const wchar_t* text = L"hello, world";
        CHECK(encode.needs_to_be_encoded({ text }) == false);
        CHECK(encode({ text }) == L"hello, world");
        }
    SECTION("Whitespace")
        {
        lily_of_the_valley::rtf_encode_text encode;
        const wchar_t* text = L"hello\tworld";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\tab world");
        text = L"hello\nworld";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\par\nworld");
        text = L"hello\n\rworld";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\par\nworld");
        }
    SECTION("With Rtf Tags")
        {
        lily_of_the_valley::rtf_encode_text encode;
        const wchar_t* text = L"hello\\ world{}";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\\\ world\\{\\}");
        }
    SECTION("Extended Ascii")
        {
        lily_of_the_valley::rtf_encode_text encode;
        const wchar_t* text = L"helloâ";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\'E2");
        }
    SECTION("Unicode")
        {
        lily_of_the_valley::rtf_encode_text encode;
        const wchar_t* text = L"hello\x432";
        CHECK(encode.needs_to_be_encoded({ text }));
        CHECK(encode({ text }) == L"hello\\u1074?");
        }
    }