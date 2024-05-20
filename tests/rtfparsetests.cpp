// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/rtf_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("RTF Parser", "[rtf import]")
    {
    SECTION("Null")
        {
        rtf_extract_text filter_rtf;
        const wchar_t* p = filter_rtf(nullptr, 5);
        CHECK(p == nullptr);
        p = filter_rtf("hello", 0);
        CHECK(p == nullptr);
        }
    SECTION("Escapes")
        {
        rtf_extract_text filter_rtf;
        // non-breaking space
        const char* rtf = "\\pard\\cf1\\f0\\fs24 crew of Apollo\\~11 consisted\\par\\cf0";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\ncrew of Apollo 11 consisted\n") == 0);
        // non-breaking hyphen
        rtf = "\\pard\\cf1\\f0\\fs24 the EU's willy\\_nilly expansion\\par\\cf0";
        p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nthe EU's willy-nilly expansion\n") == 0);
        }
    SECTION("Line inside word")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "\\pard\\cf0\\f0\\fs24 Hello \\par\\par\\par The\nre\\par\\cf0";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nHello \n\n\nThere\n") == 0);
        }
    SECTION("Paragraph")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "\\pard\\cf0\\f0\\fs24 Hello \\par\\par\\par There\\par\\cf0";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nHello \n\n\nThere\n") == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<br />\nHello <br />\n<br />\n<br />\nThere<br />\n") == std::wstring(p));
        }
    SECTION("Heavy formatting to text")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}"
                          "{\\f1\\froman\\fprq2\\fcharset0 Batang;}}{\\colortbl ;\\red192\\green192\\blue192;"
                          "\\red128\\green128\\blue0;\\red0\\green0\\blue128;}{\\*\\generator Msftedit 5.41.15.1515;}"
                          "\\viewkind4\\uc1\\pard\\f0\\fs20 H\\b e\\ul\\i r\\ulnone\\b0\\i0 e is s\\cf1\\ul\\b om\\cf0"
                          "\\ulnone\\b0 e t\\i\\f1\\fs56 ex\\i0\\f0\\fs20 t t\\i\\fs48 h\\i0\\fs20 at is \\cf2 for"
                          "\\cf3\\ul\\i ma\\cf2\\ulnone\\i0 tted\\cf0 .\\par}";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nHere is some text that is formatted.\n") == 0);
        }
    SECTION("List")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\pntext\\f1\\'B7\\tab}listitem 1\\par{\\pntext\\f1\\'B7\\tab}listitem 2\\line listitem 3";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\tlistitem 1\n\tlistitem 2\n\tlistitem 3") == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"&nbsp;&nbsp;&nbsp;&nbsp;listitem 1<br />\n&nbsp;&nbsp;&nbsp;&nbsp;listitem 2<br />\n"
                             "&nbsp;&nbsp;&nbsp;&nbsp;listitem 3") == 0);
        }
    SECTION("Ext Ascii")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "\\pard\\cf0\\f0\\fs24 \\'e1\\'df Hello";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\náß Hello") == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<br />\n&#225;&#223; Hello") == std::wstring(p));
        }
    SECTION("Unicode simple")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "\\u223? \\u120? Hello";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"ß x Hello") == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"&#223; x Hello") == 0);
        }
    SECTION("Unicode")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = R"(\uc1\u21487* \uc1\u-28589* Hello)";
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"可 道 Hello") == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"&#21487; &#36947; Hello") == 0);
        }
    SECTION("Empty style")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "\\highlight3 Into a \\highlight2 granite-ware\\highlight3  \\highlight2 saucepan";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"Into a granite-ware saucepan") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"Into a granite-ware saucepan") == std::wstring(p));
        }
    SECTION("Bold to html")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\b blah}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span style='font-weight:bold;'>blah</span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"blah") == std::wstring(p));
        }
    SECTION("Italic to html")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\i blah}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span style='font-style:italic;'>blah</span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"blah") == std::wstring(p));
        }
    SECTION("Underline to html")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\ul blah}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span style='text-decoration:underline;'>blah</span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"blah") == std::wstring(p));
        }
    SECTION("Strike through to html")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\strike blah}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span style='text-decoration:line-through;'>blah</span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"blah") == std::wstring(p));
        }
    SECTION("Foreground color to html")
        {
        const char* rtf = R"({{\colortbl ;\red255\green0\blue0;}\cf1\f0\fs20 a {\cf1 b} {\cf1 c}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="fc1">a <span class="fc1">b</span> <span class="fc1">c</span></span>)") == std::wstring(p));
    
        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cf1 Hello} {\cf2  there },you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="fc1">Hello</span> <span class="fc2"> there </span>,you.)") == std::wstring(p));
        }
    SECTION("Grouped foreground color to html")
        {
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const char* rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cf1 Hello} {\cf2 there} ,you.)";
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="fc1">Hello</span> <span class="fc2">there</span> ,you.)") == std::wstring(p));

        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cf1 Hello\cf0} {\cf2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="fc1">Hello</span> <span class="fc2">there</span> ,you.)") == std::wstring(p));

        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cf1  \cf2 Hello} {\cf2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="fc1"> <span class="fc2">Hello</span></span> <span class="fc2">there</span> ,you.)") == std::wstring(p));
        }
    SECTION("Background color to html")
        {
        const char* rtf = "{\\colortbl ;\\red255\\green0\\blue0;}{\\cb1\\f0\\fs20 a} {\\cb1 b}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span class=\"bc1\">a</span> <span class=\"bc1\">b</span>") == std::wstring(p));
    
        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cb1 Hello}{\cb2  there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1">Hello</span><span class="bc2"> there</span> ,you.)") == std::wstring(p));
    
        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\highlight1 Hello} {\highlight2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1">Hello</span> <span class="bc2">there</span> ,you.)") == std::wstring(p));
        }
    SECTION("Background color mixed commands to html")
        {
        const char* rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\highlight1 They met with many }{\highlight2 difficulties}{\highlight1\f1\emdash\f0  for ins }{\highlight3 tance})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1">They met with many </span><span class="bc2">difficulties</span><span class="bc1">&mdash; for ins </span><span class="bc3">tance</span>)") == std::wstring(p));
        }
    SECTION("Grouped background color to html")
        {
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const char* rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cb1 Hello} {\cb2 there} ,you.)";
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1">Hello</span> <span class="bc2">there</span> ,you.)") == std::wstring(p));

        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cb1 Hello\cb0} {\cb2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1">Hello</span> <span class="bc2">there</span> ,you.)") == std::wstring(p));

        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\cb1  \cb2 Hello} {\cb2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1"> <span class="bc2">Hello</span></span> <span class="bc2">there</span> ,you.)") == std::wstring(p));

        rtf = R"({\colortbl ;\red255\green0\blue0;\red0\green255\blue0;\red0\green0\blue255;}{\highlight1  \highlight2 Hello} {\highlight2 there} ,you.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(LR"(<span class="bc1"> <span class="bc2">Hello</span></span> <span class="bc2">there</span> ,you.)") == std::wstring(p));
        }
    SECTION("Heavy formatting tags to html")
        {
        const char* rtf = "<>&\"\'  ";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"&#60;&#62;&#38;&#34;&#39; &nbsp;") == 0);

        rtf = "{\\colortbl ;\\red0\\green0\\blue0;\\red175\\green175\\blue175;\\red152\\green251\\blue152;\\red255\\green128\\blue128;\\red0\\green255\\blue255;}"
                "{\\highlight3 Word had gone round during the day that old }{\\highlight2\\strike Major}{\\highlight3 , "
                "the prize }{\\highlight2\\strike Middle}{\\highlight3 \\\\ }{\\highlight2\\strike White}{\\highlight3  boar}";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span class=\"bc3\">Word had gone round during the day that old </span>"
        "<span class=\"bc2\"><span style='text-decoration:line-through;'>Major</span></span>"
        "<span class=\"bc3\">, the prize </span>"
        "<span class=\"bc2\"><span style='text-decoration:line-through;'>Middle</span></span>"
        "<span class=\"bc3\">\\ </span>"
        "<span class=\"bc2\"><span style='text-decoration:line-through;'>White</span></span>"
        "<span class=\"bc3\"> boar</span>") == std::wstring(p));
        }
    SECTION("Overlapping styles")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\colortbl ;\\red255\\green0\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}"
        "{\\highlight3 Into }{\\highlight1\\b a }{\\highlight2 a \\ul\\strike granite-ware} {\\highlight3 word.}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span class=\"bc3\">Into </span><span class=\"bc1\"><span style='font-weight:bold;'>a "
            "</span></span><span class=\"bc2\">a <span style='text-decoration:underline;'>"
            "<span style='text-decoration:line-through;'>granite-ware</span></span></span> "
            "<span class=\"bc3\">word.</span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"Into a a granite-ware word.") == std::wstring(p));
        }
    SECTION("Overlapping styles spaces")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\colortbl ;\\red255\\green0\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}{\\highlight3 Into a }"
                            "{\\highlight2\\strike granite-ware}{\\highlight3  }{\\highlight2\\strike saucepan}";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"<span class=\"bc3\">Into a </span><span class=\"bc2\">"
            "<span style='text-decoration:line-through;'>granite-ware</span>"
            "</span><span class=\"bc3\"> </span><span class=\"bc2\">"
            "<span style='text-decoration:line-through;'>saucepan</span></span>") == std::wstring(p));
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"Into a granite-ware saucepan") == std::wstring(p));
        }
    SECTION("Spaces to Html")
        {
        const char* rtf = "        ";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L" &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;") == 0);
        // try plain text too
        rtf_extract_text filter_rtfText;
        p = filter_rtfText(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"        ") == 0);
        }
    SECTION("Read title")
        {
        const char* rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"My title") == filter_rtf_html.get_title());
        }
    SECTION("Read subject")
        {
        const char* rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify") ==
            filter_rtf_html.get_subject());

        rtf = R"({\info{\title My title\}}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify}})";
        blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"My title}") == filter_rtf_html.get_title());
        CHECK(std::wstring(L"I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify") ==
            filter_rtf_html.get_subject());
        }
    SECTION("Read author")
        {
        const char* rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, it's exactly twice the length it should be to qualify}{\keywords testing}{\doccomm My fantastic comments.}{\author Joe Smith}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"Joe Smith") == filter_rtf_html.get_author());

        rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, "
                "it's exactly twice the length it should be to qualify}{\keywords testing}{\doccomm My fantastic comments.}{\author Ren\'c9e}})";
        blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"RenÉe") == filter_rtf_html.get_author());
        }
    SECTION("Read comments")
        {
        const char* rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, "
                            "it's exactly twice the length it should be to qualify}{\keywords testing}{\doccomm My fantastic comments.}{\author Joe Smith}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"My fantastic comments.") == filter_rtf_html.get_comments());
        }
    SECTION("Read keywords")
        {
        const char* rtf = R"({\info{\title My title}{\subject I originally wrote this in fulfillment of a writing exercise, but true to my long-windedness, "
                            "it's exactly twice the length it should be to qualify}{\keywords testing}{\doccomm My fantastic comments.}{\author Joe Smith}})";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"testing") == filter_rtf_html.get_keywords());
        }
    SECTION("Ignore list level")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = R"({{\leveltext\'01\u-3913;}\par Hello!})";
        const std::wstring p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"\nHello!") == p);
        }
    SECTION("Grouped italics to Html")
        {
        const char* rtf = R"(I saw {\i Brazil} yesterday.)";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-style:italic;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\i {Brazil}} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-style:italic;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\i {{Braz}}il} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-style:italic;'>Brazil</span> yesterday.") == std::wstring(p));

        // make sure regular text extraction isn't picking up italics tags
        rtf_extract_text filter_rtf;
        rtf = R"(I saw {\i {{Braz}}il} \i yesterday\i0 .)";
        p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw Brazil yesterday.") == std::wstring(p));
        }
    SECTION("Grouped bold to Html")
        {
        const char* rtf = R"(I saw {\b Brazil} yesterday.)";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-weight:bold;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\b {Brazil}} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-weight:bold;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\b {{Braz}}il} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='font-weight:bold;'>Brazil</span> yesterday.") == std::wstring(p));

        // make sure regular text extraction isn't picking up italics tags
        rtf_extract_text filter_rtf;
        rtf = R"(I saw {\b {{Braz}}il} \b yesterday\b0 .)";
        p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw Brazil yesterday.") == std::wstring(p));
        }
    SECTION("Grouped underline to Html")
        {
        const char* rtf = R"(I saw {\ul Brazil} yesterday.)";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:underline;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\ul Brazil\ulnone} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:underline;'>Brazil<span style='text-decoration:none;'>"
                            "</span></span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\ul {Brazil}} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:underline;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\ul {{Braz}}il} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:underline;'>Brazil</span> yesterday.") == std::wstring(p));

        // make sure regular text extraction isn't picking up italics tags
        rtf_extract_text filter_rtf;
        rtf = R"(I saw {\ul {{Braz}}il} \ul yesterday\ulnone .)";
        p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw Brazil yesterday.") == std::wstring(p));
        }
    SECTION("Grouped strikethrough to Html")
        {
        const char* rtf = R"(I saw {\strike Brazil} yesterday.)";
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        const wchar_t* p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:line-through;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\strike {Brazil}} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:line-through;'>Brazil</span> yesterday.") == std::wstring(p));

        rtf = R"(I saw {\strike {{Braz}}il} yesterday.)";
        p = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw <span style='text-decoration:line-through;'>Brazil</span> yesterday.") == std::wstring(p));

        // make sure regular text extraction isn't picking up italics tags
        rtf_extract_text filter_rtf;
        rtf = R"(I saw {\strike {{Braz}}il} \strike yesterday\strike0 .)";
        p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wstring(L"I saw Brazil yesterday.") == std::wstring(p));
        }
    SECTION("Font")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Book Antiqua;}}"
                          "{\\colortbl ;\\red0\\green0\\blue0;\\red152\\green251\\blue152;}"
                          "{\\*\\generator Msftedit 5.41.15.1507;}\\viewkind4\\uc1\\pard\\cf2\\f0\\fs24\\'e1\\'df }";
        [[maybe_unused]] auto blah = filter_rtf(rtf, std::strlen(rtf));
        // text parser should just ignore the font info, so this should just default to Arial
        CHECK(filter_rtf.get_font() == "Arial");
        CHECK(filter_rtf.get_font_size() == 12);
        // color table should be ignored too, and text color should just be zeroed out
        rtf_color color = filter_rtf.get_font_color();
        CHECK(color.red == 0);
        CHECK(color.green == 0);
        CHECK(color.blue == 0);
        // test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(filter_rtf_html.get_font() == "Book Antiqua");
        CHECK(filter_rtf_html.get_font_size() == 12);
        color = filter_rtf_html.get_font_color();
        CHECK(color.red == 152);
        CHECK(color.green == 251);
        CHECK(color.blue == 152);
        }
    SECTION("Css")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Book Antiqua;}}"
                            "{\\colortbl ;\\red255\\green0\\blue0;\\red152\\green251\\blue152;}"
                            "{\\*\\generator Msftedit 5.41.15.1507;}\\viewkind4\\uc1\\pard\\cf2\\f0\\fs24\\'e1\\'df }";
        //test html convertor
        rtf_extract_text filter_rtf_html(rtf_extract_text::rtf_extraction_type::rtf_to_html);
        [[maybe_unused]] auto blah = filter_rtf_html(rtf, std::strlen(rtf));
        CHECK(filter_rtf_html.get_style_section() == L".bc0 {background-color:#FFFFFF;}\n.fc0 {color:#000000;}\n"
            ".bc1 {background-color:#FF0000;}\n.fc1 {color:#FF0000;}\n.bc2 {background-color:#98FB98;}\n.fc2 {color:#98FB98;}");
        }
    SECTION("Page breaks")
        {
        rtf_extract_text filter_rtf;
        const char* rtf = "{\\rtf1\\ansi\\deff3\\adeflang1025"
            "{\\fonttbl{\\f0\\froman\\fprq2\\fcharset0 Times New Roman;}{\\f1\\froman\\fprq2\\fcharset2 Symbol;}"
            "{\\f2\\fswiss\\fprq2\\fcharset0 Arial;}{\\f3\\froman\\fprq2\\fcharset0 Liberation Serif{\\*\\falt Times New Roman};}"
            "{\\f4\\fswiss\\fprq2\\fcharset0 Liberation Sans{\\*\\falt Arial};}{\\f5\\fnil\\fprq2\\fcharset0 Microsoft YaHei;}"
            "{\\f6\\fnil\\fprq2\\fcharset0 Arial;}{\\f7\\fswiss\\fprq0\\fcharset0 Arial;}}"
            "{\\colortbl;\\red0\\green0\\blue0;\\red128\\green128\\blue128;}"
            "{\\stylesheet{\\s0\\snext0\\nowidctlpar{\\*\\hyphen2\\hyphlead2\\hyphtrail2\\hyphmax0}\\aspalpha\\ltrpar\\cf0\\kerning1"
            "\\dbch\\af8\\langfe2052\\dbch\\af6\\afs24\\alang1081\\loch\\f3\\fs24\\lang1033 Normal;}"
            "{\\s15\\sbasedon0\\snext16\\sb240\\sa120\\keepn\\dbch\\af5\\dbch\\af6\\afs28\\loch\\f4\\fs28 Heading;}"
            "{\\s16\\sbasedon0\\snext16\\sl288\\slmult1\\sb0\\sa140 Text Body;}"
            "{\\s17\\sbasedon16\\snext17\\sl288\\slmult1\\sb0\\sa140\\dbch\\af7 List;}"
            "{\\s18\\sbasedon0\\snext18\\sb120\\sa120\\noline\\i\\dbch\\af7\\afs24\\ai\\fs24 Caption;}"
            "{\\s19\\sbasedon0\\snext19\\noline\\dbch\\af7 Index;}"
            "}{\\*\\generator LibreOffice/4.4.3.2$Windows_x86 LibreOffice_project/88805f81e9fe61362df02b9941de8e38a9b5fd16}"
            "{\\info{\\creatim\\yr2016\\mo10\\dy28\\hr9\\min41}{\\revtim\\yr2016\\mo10\\dy28\\hr9\\min42}{\\printim\\yr0\\mo0\\dy0\\hr0\\min0}}\\deftab709"
            "\\viewscale100"
            "{\\*\\pgdsctbl"
            "{\\pgdsc0\\pgdscuse451\\pgwsxn12240\\pghsxn15840\\marglsxn1134\\margrsxn1134\\margtsxn1134\\margbsxn1134\\pgdscnxt0 Default Style;}}"
            "\\formshade\\paperh15840\\paperw12240\\margl1134\\margr1134\\margt1134\\margb1134\\sectd\\sbknone\\sectunlocked1\\pgndec\\pgwsxn12240"
            "\\pghsxn15840\\marglsxn1134\\margrsxn1134\\margtsxn1134\\margbsxn1134\\ftnbj\\ftnstart1\\ftnrstcont\\ftnnar\\aenddoc\\aftnrstcont\\aftnstart1\\aftnnrlc"
            "{\\*\\ftnsep\\chftnsep}\\pgndec\\pard\\plain \\s0\\nowidctlpar{\\*\\hyphen2\\hyphlead2\\hyphtrail2\\hyphmax0}\\aspalpha"
            "\\ltrpar\\cf0\\kerning1\\dbch\\af8\\langfe2052\\dbch\\af6\\afs24\\alang1081\\loch\\f3\\fs24\\lang1033{\\rtlch \\ltrch\\loch "
            "Hello.}"
            "\\par \\pard\\plain \\s0\\nowidctlpar{\\*\\hyphen2\\hyphlead2\\hyphtrail2\\hyphmax0}\\aspalpha\\ltrpar\\cf0\\kerning1\\dbch\\af8"
            "\\langfe2052\\dbch\\af6\\afs24\\alang1081\\loch\\f3\\fs24\\lang1033\\rtlch \\ltrch\\loch "
            "\\par \\pard\\plain \\s0\\nowidctlpar{\\*\\hyphen2\\hyphlead2\\hyphtrail2\\hyphmax0}\\aspalpha\\ltrpar\\cf0\\kerning1\\dbch\\af8"
            "\\langfe2052\\dbch\\af6\\afs24\\alang1081\\loch\\f3\\fs24\\lang1033\\pagebb{\\rtlch \\ltrch\\loch "
            "Here is page 2.}"
            "\\par }";
        // test html convertor
        const wchar_t* p = filter_rtf(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nHello.\n\n\n\n\fHere is page 2.\n") == 0);
        }
    }

// NOLINTEND
// clang-format on
