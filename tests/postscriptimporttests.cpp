#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/postscript_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Postscript Import", "[postscript import]")
	{
    SECTION("Nulls")
        {
        postscript_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext("some text", 0) == nullptr);
        }
    SECTION("Version2")
        {
        const char* text = "%!PS-Adobe-2.0\n(This is a string)";
        postscript_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"This is a string") == 0);
        }
    SECTION("Version3NotSupported")
        {
        const char* text = "%!PS-Adobe-3.0\n(This is a string)";
        postscript_extract_text ext;
        CHECK_THROWS(ext(text, std::strlen(text)));
        }
    SECTION("MissingHeader")
        {
        postscript_extract_text ext;
        CHECK_THROWS(ext("some text", 9));
        }
    SECTION("Simple")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(This is a string)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"This is a string") == 0);
        text = "%!PS-Adobe-2.0\n(Strings may contain newlines\nand such.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Strings may contain newlines\nand such.") == 0);
        text = "%!PS-Adobe-2.0\n(Strings may contain special characters *!&}^% and\nbalanced parentheses ( ) (and so on).)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Strings may contain special characters *!&}^% and\nbalanced parentheses ( ) (and so on).") == 0);
        text = "%!PS-Adobe-2.0\n(The following is an empty string.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"The following is an empty string.") == 0);
        text = "%!PS-Adobe-2.0\n(It has 0 (zero) length.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"It has 0 (zero) length.") == 0);
        }
    SECTION("EscapeCommands")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(Thi\\\\s\\ni\\(\\)s\\ra\\tstring)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Thi\\s\ni()s\ra\tstring") == 0);
        }
    SECTION("EscapedNewLines")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(These \\\ntwo strings \\\nare the same.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"These two strings are the same.") == 0);
        }
    SECTION("Ligatures")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(These\\0013\\014\\15\\000016\\017\\025\\032\\033\\034.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Thesefffiflffiffl*naeoefi.") == 0);
        }
    SECTION("Octal")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(These\\053.)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"These+.") == 0);
        }
    SECTION("HypenatedWord")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\ni(Commu-)10941800 y(nity)g(News))";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Commu-\nnity News") == 0);
        text = "%!PS-Adobe-2.0\ni(Commu-)1094\n1800 y(nity)g(News).)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Commu-\nnity News") == 0);
        text = "%!PS-Adobe-2.0\ni(Commu-)1094\r1800 y(nity)g(News).)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Commu-\nnity News") == 0);
        }
    SECTION("NewLineCommand")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(This is a string)105 y Fe(New Line)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"This is a string\nNew Line") == 0);
        }
    SECTION("NewPage")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n %%Page: 1 11 (This is a string)105 %%Page: 2 22 Fe(New Line)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"This is a string\f New Line") == 0);
        }
    SECTION("DVIPSQuoteWorkaround")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n%%Creator: dvips 5.521 Copyright 1986, 1993 Radical Eye Software%%\n(\\\\This is a string\")";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\"This is a string\"") == 0);
        }
    SECTION("GCommand")
        {
        postscript_extract_text ext;
        const char* text = "%!PS-Adobe-2.0\n(the)g(temp)-5 b(er)g(a)g(ture)b(is)g(low)";
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"the temperature is low") == 0);
        }
    }
