#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/cpp_extract_text.h"

using namespace lily_of_the_valley;
using namespace Catch::Matchers;

TEST_CASE("Check CPP", "[cpp]")
	{
    SECTION("NULL")
        {
        cpp_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext(L"//some comments\nsome code", 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Remove Indenting In Block Comment")
        {
        const wchar_t* text = L"/**  \tSome long\n\n   comments here\n   and here.*/\n\nSome code*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some long\n\ncomments here\nand here.") == 0);
        CHECK(ext.get_filtered_text_length() == 34);
        }
    SECTION("Email Address")
        {
        const wchar_t* text = L"/**!@author blake@mail.com*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"author: blake@mail.com") == 0);
        CHECK(ext.get_filtered_text_length() == 22);
        }
    SECTION("Read Author")
        {
        const wchar_t* text = L"/**!@author Joe Smith\nHere is some text.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"author: Joe Smith\n\nHere is some text.") == 0);
        CHECK(ext.get_filtered_text_length() == 37);
        CHECK(ext.get_author() == std::wstring(L"Joe Smith"));
        }
    SECTION("Doxygen Param")
        {
        const wchar_t* text = L"/***!Some \n@param [in,out] value The Value\n@param Other the other one.*/\n\nSome code*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some \n\nvalue: The Value\n\nOther: the other one.") == 0);
        CHECK(ext.get_filtered_text_length() == 46);
        }
    SECTION("Doxygen Tag To Skip")
        {
        const wchar_t* text = L"/*!\\brief some text here\nand here.\n@{";
        cpp_extract_text ext;
        const std::wstring output = ext(text, std::wcslen(text));
        const std::wstring correct(L"some text here\nand here.");
        CHECK(correct == output);
        CHECK(ext.get_filtered_text_length() == 24);
        }
    SECTION("Doxygen Html Block")
        {
        const wchar_t* text = L"/**\\htmlonly\nDutch includes\n&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&#228;  &#235;  &#239;  &#246;  &#252;  &#225;  &#233;  &#237;  &#243;  &#250;  &#232;\n\\endhtmlonly\nAnother comment.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L" Dutch includes      -ä  ë  ï  ö  ü  á  é  í  ó  ú  è \n\nAnother comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 72);
        }
    SECTION("Doxygen Empty Html Block")
        {
        const wchar_t* text = L"/**\\htmlonly\nendhtmlonly\nAnother comment.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nAnother comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 18);
        }
    SECTION("Doxygen Bad Html Block")
        {
        const wchar_t* text = L"/**\\htmlonly\nAnother comment.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\nAnother comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 17);
        }
    SECTION("Doxygen Param With Tag")
        {
        const wchar_t* text = L"/**!Some \n@param [in,out] value The Value used for @MyClass here.*/\n\nSome code*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some \n\nvalue: The Value used for MyClass here.") == 0);
        CHECK(ext.get_filtered_text_length() == 46);
        }
    SECTION("Doxygen Single Line Tags")
        {
        const wchar_t* text = L"/*!\\class MyClass\n\t\tA description*/\n\nSome code*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\nMyClass\n\nA description") == 0);
        CHECK(ext.get_filtered_text_length() == 23);
        }
    SECTION("Doxygen Single Line Tags Copy Tag")
        {
        const wchar_t* text = L"/*!\\author Blake Madden\n@date 2013*/\n\nSome code*/";
        cpp_extract_text ext;
        std::wstring output = ext(text, std::wcslen(text));
        CHECK(output == L"author: Blake Madden\n\ndate: 2013");
        CHECK(ext.get_filtered_text_length() == 32);
        }
    SECTION("Doxygen Regular Tag")
        {
        const wchar_t* text = L"/*!Some \n\\class Class\n@class\tThe other one.*/\n\nSome code*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some \n\nClass\n\n\nThe other one.") == 0);
        CHECK(ext.get_filtered_text_length() == 29);
        }
    SECTION("Strip Escapes")
        {
        const wchar_t* text = LR"(_(L"Hello, \"Carl\"."))";
        cpp_extract_text ext;
        const std::wstring output = ext(text, std::wcslen(text));
        CHECK(std::wstring(L"Hello, \"Carl\".") == output);
        CHECK(ext.get_filtered_text_length() == 14);
        }
    SECTION("Block Comment")
        {
        const wchar_t* text = L"/*!Some \ncomment*/\n\nSome code\n/**A doxygen comment.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some \ncomment\n\nA doxygen comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 33);
        }
    SECTION("Regular Block Comment")
        {
        const wchar_t* text = L"/*Some \ncomment*/\n\nSome code\n/**A doxygen comment.*/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"A doxygen comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 18);
        }
    SECTION("Bad Block Comment")
        {
        const wchar_t* text = L"/******Some \ncomment";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some \ncomment") == 0);
        CHECK(ext.get_filtered_text_length() == 13);
        }
    SECTION("Empty Block Comment")
        {
        const wchar_t* text = L"/***/\n/***/";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"") == 0);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Line Comment")
        {
        const wchar_t* text = L"/// \tSome comment\n//!Another comment\nSome code\n///A doxygen comment.";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some comment\nAnother comment\n\nA doxygen comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 48);
        }
    SECTION("Regular Line Comment")
        {
        const wchar_t* text = L"//Some comment\n//Another comment\nSome code\n///A doxygen comment.";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"A doxygen comment.") == 0);
        CHECK(ext.get_filtered_text_length() == 18);
        }
    SECTION("Multiple Line Comment")
        {
        const wchar_t* text = L"///Some comment\n  ///Another comment\nSome code";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some comment\nAnother comment") == 0);
        CHECK(ext.get_filtered_text_length() == 28);
        }
    SECTION("Multiple Line Comment Split By Code")
        {
        const wchar_t* text = L"///Some comment\nSome code\n///Another comment";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some comment\n\nAnother comment") == 0);
        CHECK(ext.get_filtered_text_length() == 29);
        }
    SECTION("Empty Line Comment")
        {
        const wchar_t* text = L"///\n///";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"") == 0);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Get Tex tString")
        {
        const wchar_t* text = L"string blah = _(\"My text here\");";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"My text here") == 0);
        CHECK(ext.get_filtered_text_length() == 12);
        }

    SECTION("Get Text Embedded Strings String")
        {
        const wchar_t* text = LR"(string blah = _(L"My text \\\"here\\\"");)";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, LR"(My text \"here\")") == 0);
        CHECK(ext.get_filtered_text_length() == 16);
        }

    SECTION("Get Text String With NewLines")
        {
        const wchar_t* text = LR"(string blah = _(L"My text\nSecond Line\rThird\tLine");)";
        cpp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"My text\nSecond Line\nThird\tLine") == 0);
        CHECK(ext.get_filtered_text_length() == 30);

        // boundary test (trailing newline is trimmed by design)
        text = LR"(string blah = _(L"\nMy text\nSecond Line\rThird\tLine\n");)";
        output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\nMy text\nSecond Line\nThird\tLine") == 0);
        CHECK(ext.get_filtered_text_length() == 31);
        }
    }