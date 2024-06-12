// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/import/html_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;
using namespace html_utilities;

TEST_CASE("StrIStr Not Quoted", "[html import]")
    {
    SECTION("Nulls")
        {
        CHECK(html_extract_text::stristr_not_quoted(nullptr, 0, L"HelLo", 5) == nullptr);
        CHECK(html_extract_text::stristr_not_quoted(L"HelLo", 5, nullptr, 0) == nullptr);
        CHECK(html_extract_text::stristr_not_quoted(nullptr, 0, nullptr, 0) == nullptr);
        }
    SECTION("FindFirstItem")
        {
        // should find at the beginning
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"HelLo", 5) == buffer);
        buffer = L"\"\"hello, world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"HelLo", 5) == buffer+2);
        buffer = L"\"hello, world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"HelLo", 5) == nullptr);
        }
    SECTION("Last Item In Sequence String")
        {
        // should find last item in sequence
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+7);
        buffer = L"hello, \"\"world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+9);
        buffer = L"hello, \"world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == nullptr);
        buffer = L"\"hello, \"\"world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == nullptr);
        buffer = L"hello, \"world\"";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == nullptr);
        }
    SECTION("Middle Item In Sequence String")
        {
        // should find middle item in sequence
        const wchar_t* buffer = L"hello, world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+7);
        buffer = L"\"hello, \"world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+9);
        buffer = L"hello, \"world!!!\" Goodbye!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == nullptr);
        }
    SECTION("Second Item In Sequence String")
        {
        const wchar_t* buffer = L"hello, \"world!!!\" Goodbye, cruel world!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        buffer = L"hello, \"world!!!\" Goodbye, cruel WORLD!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        buffer = L"hello, \"WORLD!!!\" Goodbye, cruel woRLd";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        }
    SECTION("Second Item In Sequence String Single Quote")
        {
        const wchar_t* buffer = L"hello, \'world!!!\' Goodbye, cruel world!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        buffer = L"hello, \'world!!!\' Goodbye, cruel WORLD!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        buffer = L"hello, \'WORLD!!!\' Goodbye, cruel woRLd";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        }
    SECTION("Item In Mixed Quotes")
        {
        const wchar_t* buffer = L"hello, \'world!!!\" Goodbye, cruel world!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+33);
        buffer = L"hello, \"wor'ld!!!\" Goodbye, cruel WORLD!";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+34);
        buffer = L"hello, \"WO'R'LD!!!\" Goodbye, cruel woRLd";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"WORLD", 5) == buffer+35);
        }
    SECTION("Item Letter Mix Up")
        {
        const wchar_t* buffer = L"a:r><a:rPr lang=\"en-US\" i=\"1\" smtClean=\"0\"/>";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"smtClean", 8) == buffer+30);
        }
    SECTION("Find Nothing")
        {
        // should find nothing and return null
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"Help", 4) == nullptr);
        }
    SECTION("Empty String")
        {
        //should find nothing and return null
        CHECK(html_extract_text::stristr_not_quoted(L"", 0, L"Hello", 5) == nullptr);
        }
    SECTION("Substring Too Big")
        {
        const wchar_t* buffer = L"Hello";
        CHECK(html_extract_text::stristr_not_quoted(buffer, std::wcslen(buffer), L"Hello World", 11) == nullptr);
        }
    }

TEST_CASE("Str Chr Not Quoted", "[html import]")
	{
    SECTION("Nulls")
        {
        CHECK(html_extract_text::strchr_not_quoted(nullptr, L'a') == nullptr);
        }
    SECTION("Find First Item")
        {
        // should find at the beginning
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'h') == buffer);
        buffer = L"\"\"hello, world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'h') == buffer+2);
        buffer = L"\"hello, world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'h') == nullptr);
        }
    SECTION("Last Item In Sequence String")
        {
        // should find last item in sequence
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+7);
        buffer = L"hello, \"\"world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+9);
        buffer = L"hello, \"world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == nullptr);
        buffer = L"\"hello, \"\"world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == nullptr);
        buffer = L"hello, \"world\"";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == nullptr);
        }
    SECTION("Middle Item In Sequence String")
        {
        // should find middle item in sequence
        const wchar_t* buffer = L"hello, world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+7);
        buffer = L"\"hello, \"world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+9);
        buffer = L"hello, \"world!!!\" Goodbye!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == nullptr);
        }
    SECTION("Middle Item In Sequence String Single Quote")
        {
        // should find middle item in sequence
        const wchar_t* buffer = L"hello, world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+7);
        buffer = L"\'hello, \'world!!! Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+9);
        buffer = L"hello, \'world!!!\' Goodbye!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == nullptr);
        }
    SECTION("Second Item In Sequence String")
        {
        const wchar_t* buffer = L"hello, \"world!!!\" Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        buffer = L"hello, \"world!!!\" Goodbye, cruel WORLD!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'W') == buffer+33);
        buffer = L"hello, \"WORLD!!!\" Goodbye, cruel woRLd";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        }
    SECTION("Second Item In Sequence String Single Quote")
        {
        const wchar_t* buffer = L"hello, \'world!!!\' Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        buffer = L"hello, \'world!!!\' Goodbye, cruel WORLD!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'W') == buffer+33);
        buffer = L"hello, \'WORLD!!!\' Goodbye, cruel woRLd";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        }
    SECTION("Second Item In Mixed Quotes")
        {
        const wchar_t* buffer = L"hello, \'world!!!\" Goodbye, cruel world!";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        buffer = L"hello, \"wor'ld!!!\" Goodbye, cruel WORLD!"; // single should be ignored
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'W') == buffer+34);
        buffer = L"hello, \"wo'r'ld!!!\" Goodbye, cruel WORLD!"; // singles should be ignored
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'W') == buffer+35);
        buffer = L"hello, \'WORLD!!!\' Goodbye, cruel woRLd";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'w') == buffer+33);
        }
    SECTION("Find Nothing")
        {
        //should find nothing and return null
        const wchar_t* buffer = L"hello, world";
        CHECK(html_extract_text::strchr_not_quoted(buffer, L'<') == nullptr);
        }
    SECTION("Empty String")
        {
        //should find nothing and return null
        CHECK(html_extract_text::strchr_not_quoted(L"", L'w') == nullptr);
        }
	}

TEST_CASE("HTML parser subscripts", "[html import]")
    {
        SECTION("Superscript")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"H<sup>2</sup>O<sup>37i</sup>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"H²O³⁷ⁱ") == p);
        text = L"H<sup>2</sup>O<sup>37Zi</sup>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"H²O³⁷Zⁱ") == p);
        }
    SECTION("Subscript")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"H<sub>2</sub>O<sub>37h</sub>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"H₂O₃₇ₕ") == p);
        text = L"H<sub>2</sub>O<sub>37Zh</sub>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"H₂O₃₇Zₕ") == p);
        }
    SECTION("Not really a script")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<sub>Hello22</sub> some text <sub>Hello2</sub>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello22 some text Hello2") == p);

        text = L"<sup>Hello2</sup>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello2") == p);
        }
    }

TEST_CASE("HTML parser tags", "[html import]")
    {
    SECTION("Find Tag")
        {
        const wchar_t* text = L"body bgcolor=\'#FF0000\' color=\'#FF0000\'>there<br />world<br >!";
        CHECK(html_extract_text::find_tag(text, L"bgcolor", false) == text+5);
        CHECK(html_extract_text::find_tag(text, L"BGCOLOR", false) == text+5);
        CHECK(html_extract_text::find_tag(text, L"color", false) == text+23);
        CHECK(html_extract_text::find_tag(text, L"width", false) == nullptr);
        CHECK(html_extract_text::find_tag(nullptr, L"width", false) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"", false) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"body", false) == text);
        }
    SECTION("Find Tag 2")
        {
        const wchar_t* text = L"body style=\"color=#FF0000 width=250\">there<br />world<br >!";
        CHECK(html_extract_text::find_tag(text, L"STYLE", false) == text+5);
        CHECK(html_extract_text::find_tag(text, L"color", false) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"width", false) == nullptr);
        CHECK(html_extract_text::find_tag(nullptr, L"width", false) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"", false) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"body", false) == text);
        }
    SECTION("Find Tag Quotable")
        {
        const wchar_t* text = L"body style=\"color=#FF0000 width=250\">there<br />world<br >!";
        CHECK(html_extract_text::find_tag(text, L"STYLE", true) == text+5);
        CHECK(html_extract_text::find_tag(text, L"color", true) == text+12);
        CHECK(html_extract_text::find_tag(text, L"width", true) == text+26);
        CHECK(html_extract_text::find_tag(nullptr, L"width", true) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"", true) == nullptr);
        CHECK(html_extract_text::find_tag(text, L"body", true) == text);
        }
    }

TEST_CASE("HTML Parser", "[html import]")
    {
    SECTION("Find Bookmark")
        {
        const wchar_t* text = L"<a name=\"copyright\" />blah blah<a name=\"books\" /><h2>Also <a nam=\"bogustag\">by Mark ZZZZZ</h2><a />";
        std::pair<const wchar_t*, std::wstring> retVal = html_extract_text::find_bookmark(text, text + wcslen(text));
        CHECK(retVal.first == text);
        CHECK(retVal.second == L"copyright");
        retVal = html_extract_text::find_bookmark(text + 2, text + wcslen(text));
        CHECK(retVal.first == text + 31);
        CHECK(retVal.second == L"books");
        retVal = html_extract_text::find_bookmark(text + 33, text + wcslen(text));
        CHECK(retVal.first == nullptr);
        CHECK(retVal.second == L"");
        retVal = html_extract_text::find_bookmark(text, nullptr);
        CHECK(retVal.first == nullptr);
        CHECK(retVal.second == L"");

        const wchar_t* textNoElements = L"blah blah <h2>Also  by Mark ZZZZZ</h2> ";
        retVal = html_extract_text::find_bookmark(textNoElements, textNoElements + wcslen(textNoElements));
        CHECK(retVal.first == nullptr);
        CHECK(retVal.second == L"");

        const wchar_t* textWithPound = L"<a name=\"#copyright\" />";
        retVal = html_extract_text::find_bookmark(textWithPound, textWithPound + wcslen(textWithPound));
        CHECK(retVal.first == textWithPound);
        CHECK(retVal.second == L"copyright");
        }
    SECTION("Multiple Breaks")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p><tt>Chapter 1<br>\n<br>\nIt was the best days of our lives.</tt></p>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\nChapter 1\n \n It was the best days of our lives.\n\n") == 0);
        }
    SECTION("Extended Ascii")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        p = filter_html(L"cl&#255;ich&#201;", 17, true, false);
        CHECK(std::wcscmp(p, L"clÿichÉ") == 0);
        }
    SECTION("Extended Ascii Broken")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        p = filter_html(L"cl&#;ich&#g;", 12, true, false);
        CHECK(std::wcscmp(p, L"cl&#;ich&#g;") == 0);
        }
    SECTION("Extended Ascii Hex")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        p = filter_html(L"cl&#xFF;ich&#Xc9;", 17, true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"clÿichÉ" });
        }
    SECTION("Extended Ascii Hex Broken")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        p = filter_html(L"cl&#x;ich&#xG7;", 15, true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"cl&#x;ich&#xG7;" });
        }
    SECTION("Ligatures")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        const wchar_t* text = L"&#xFB01;t as a &#xFB01;ddle. &#xFB02;y away, &#64258;y away.";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"fit as a fiddle. fly away, fly away." });
        }
    SECTION("Paragraph")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<p>there</p><p someattribute=\"7\">world<paragraph>!";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\nthere\n\n\n\nworld!") == 0);
        text = L"hello<div>there</div><div someattribute=\"7\">world</div><paragraph>!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\nthere\n\n\n\nworld\n\n!") == 0);
        text = L"hello<DIV>there</DIV><DIV someattribute=\"7\">world</DIV><PARAGRAPH>!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\nthere\n\n\n\nworld\n\n!") == 0);
        }
    SECTION("Title")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<title>MyTitle</title>there<br >!";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hellothere\n!") == 0);
        CHECK(std::wstring(L"MyTitle") == filter_html.get_title());
        text = L"hello<TITLE>My&amp;Title</TITLE>there<br >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hellothere\n!") == 0);
        CHECK(std::wstring(L"My&Title") == filter_html.get_title());

        // empty title
        text = L"hello<TITLE></TITLE>there<br >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hellothere\n!") == 0);
        CHECK(filter_html.get_title().empty());

        // malformed title
        text = L"hello<TITLE>My title</li>there<br >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hellothere\n!") == 0);
        CHECK(filter_html.get_title().empty());

        // malformed title, nothing beyond title
        text = L"hello<TITLE>My title";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);
        CHECK(filter_html.get_title().empty());

        text = L"hello<TITLE>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);
        CHECK(filter_html.get_title().empty());

        // no title
        text = L"hellothere<br >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hellothere\n!") == 0);
        CHECK(filter_html.get_title().empty());
        }
    SECTION("Preformatted Text")
        {
        const wchar_t* text = L"hello\n<PRE>Some \nPreformatted text</pre>!";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello Some \nPreformatted text!") == 0);
        }
    SECTION("List")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<LI>Item 1</li><li someattribute=4>Item 2</li>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\tItem 1\n\tItem 2") == 0);
        }
    SECTION("Definition")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<dl><dt>Item 1</dt><dd someattribute=4>The definition</dd><dt>Item 2</dt><dd someattribute=4>The definition2</dd></dl>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"hello\n\n\n\nItem 1:\tThe definition\n\nItem 2:\tThe definition2\n\n") == p);
        text = L"hello<DL><DT>Item 1</DT><DD someattribute=4>The definition</DD></DL>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"hello\n\n\n\nItem 1:\tThe definition\n\n") == p);
        }
    SECTION("Table")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<h1>hello</h1><table><tr><td>item 1</td><td>item 2</td></tr></table>there";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\nhello\n\n\n\n\n\n\titem 1\titem 2\n\nthere") == 0);
        text = L"<H1>hello</H1><TABLE><TR><TD>item 1</TD><TD>item 2</TD></TR></TABLE>there";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\nhello\n\n\n\n\n\n\titem 1\titem 2\n\nthere") == 0);
        }
    SECTION("Script")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<SCRIPT>var blah;</script> there<script language=\"assemby\">mov eax, 5;</SCRIPt>!";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);
        text = L"hello<SCRIPT>var blah;</script> there<noscript>scripting turned off</noSCRIPt><NOSCRIPT whatever= 9>scripting turned off</noSCRIPt>!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);

        // malformed script
        text = L"hello<SCRIPT>var blah;</li> there<noscript>scripting turned off</noSCRIPt><NOSCRIPT whatever= 9>scripting turned off</noSCRIPt>!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);

        text = L"hello<SCRIPT>var blah;";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);

        text = L"hello<SCRIPT>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);

        // malformed noscript
        text = L"hello<SCRIPT>var blah;</SCRIPT> there<noscript>scripting turned off</li>!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);

        text = L"hello<SCRIPT>var blah;</SCRIPT> there<noscript>scripting turned off";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there") == 0);

        text = L"hello<SCRIPT>var blah;</SCRIPT> there<noscript>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there") == 0);
        }
    SECTION("Style")
        {
        const wchar_t* text = L"hello<STYLE>class a = <i>;</style>&nbsp;there<style whatever>!</sTyle>!";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);

        // malformed style
        text = L"hello<STYLE>class a = rgb(255,255,255);</li>&nbsp;there!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there!") == 0);

        // malformed scrstyleipt, no more valid HTML after bad <style>
        text = L"hello<STYLE>class a = rgb(255,255,255);&nbsp;there!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);

        text = L"hello<STYLE>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello") == 0);
        }
    SECTION("Comments")
        {
        const wchar_t* text = L"hello<!--there<br>-->&nBsp;world!";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello world!") == 0);
        }
    SECTION("Named Entities")
        {
        const wchar_t* text = L"hello&LT;there&COPY;";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello<there©") == 0);
        }
    SECTION("Heavy Formatting")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"h<span style=\'italics\'>ello</span><em> th</em><u>e</u>re world! \"Nice\" <span style=\"italics\">t</span>o meet <img src=\"file.png\" alt=\"\">you &amp; you!";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there world! \"Nice\" to meet you & you!") == 0);
        text = L"h<SPAN STYLE=\'italics\'>ello</SPAN><EM> th</EM><u>e</u>re world! \"Nice\" <SPAN STYLE=\"italics\">t</SPAN>o meet <IMG SRC=\"file.png\" alt=\"\">you &amp; you!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello there world! \"Nice\" to meet you & you!") == 0);
        text = L"<img src=\"file.png\" alt=\"some text\">Hi!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Hi!") == 0);
        text = L"<img src=\"images/biosoft.gif\" biotechnology=\"\" software=\"\" internet=\"\" journal=\"\" praises=\"\" statistics=\"\" s=\"\" quality,=\"\" customizability=\"\" and=\"\" selection=\"\" of=\"\" graphics=\"\" valign=\"bottom\" vspace=\"5\" width=\"258\" align=\"left\" height=\"23\" hspace=\"10\">A comprehensive (4 pages long) review of";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"A comprehensive (4 pages long) review of") == 0);
        }
    SECTION("Read Element As String")
        {
        const wchar_t* text = L"<h1>My header</H1>";
        CHECK(std::wstring(L"My header") == html_extract_text::read_element_as_string(text, text+std::wcslen(text), L"h1"));
        text = L"<h1>   My header</H1>";
        CHECK(std::wstring(L"My header") == html_extract_text::read_element_as_string(text, text + std::wcslen(text), L"h1"));
        text = L"<h1>My header   </H1>";
        CHECK(std::wstring(L"My header") == html_extract_text::read_element_as_string(text, text + std::wcslen(text), L"h1"));
        text = L"<h1>   My header   </H1>";
        CHECK(std::wstring(L"My header") == html_extract_text::read_element_as_string(text, text + std::wcslen(text), L"h1"));
        text = L"<h1></H1>";
        CHECK(std::wstring(L"") == html_extract_text::read_element_as_string(text, text + std::wcslen(text), L"h1"));
        // malformed
        text = L"<h1>My header</l1>";
        CHECK(std::wstring(L"") == html_extract_text::read_element_as_string(text, text+std::wcslen(text), L"h1"));
        }
    SECTION("Read Tag As Long")
        {
        const wchar_t* text = L"body height= 275 style=\"width=250\">there<br />world<br >!";
        CHECK(275 == html_extract_text::read_attribute_as_long(text, L"height", false));
        CHECK(250 == html_extract_text::read_attribute_as_long(text, L"width", true));
        CHECK(0 == html_extract_text::read_attribute_as_long(text, L"size", true)); // doesn't exit
        CHECK(0 == html_extract_text::read_attribute_as_long(nullptr, L"width", true));
        CHECK(0 == html_extract_text::read_attribute_as_long(text, std::wstring_view( L"bogus", 0 ), true));
        }
    SECTION("Read Empty Attribute")
        {
        const wchar_t* text = L"body style =\"\" info =' ' height=275>there<br />world<br >!";
        CHECK(275 == html_extract_text::read_attribute_as_long(text, L"height", false));
        CHECK(std::wstring(L"") == html_extract_text::read_attribute_as_string(text, L"style", false, false));
        CHECK(std::wstring(L"") == html_extract_text::read_attribute_as_string(text, L"info", false, false));
        CHECK(std::wstring(L"") == html_extract_text::read_attribute_as_string(text, L"info", false, true));

        text = L"body style =\"\" info ='num value' height=275>there<br />world<br >!";
        CHECK(std::wstring(L"num") == html_extract_text::read_attribute_as_string(text, L"info", false, false));
        CHECK(std::wstring(L"num value") == html_extract_text::read_attribute_as_string(text, L"info", false, true));
        }
    SECTION("Read Tag Quotable")
        {
        const wchar_t* text = L"body style=\"color=#FF0000 width=250\">there<br />world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"color", true, false) == L"#FF0000");
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, false) == L"250");
        CHECK(html_extract_text::read_attribute_as_string(text, L"size", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(nullptr, L"width", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(text, { L"bogus", 0 }, true, false) == L"");
        }
    SECTION("Read Tags")
        {
        const wchar_t* text = L"body bgcolor=#FF0000>there<style width=250>world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"bgcolor", true, false) == L"#FF0000");
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, false) == L""); // width is in another tag
        CHECK(html_extract_text::read_attribute_as_string(text, L"size", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(nullptr, L"width", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(text, { L"", 0 }, true, false) == L"");
        }
    SECTION("Read Tags Quoted")
        {
        const wchar_t* text = L"body bgcolor=\"#FF0000\">there<style width=250>world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"bgcolor", true, false) == L"#FF0000");
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, false) == L""); // width is in another tag
        CHECK(html_extract_text::read_attribute_as_string(text, L"size", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(nullptr, L"width", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(text, { L"", 0 }, true, false) == L"");
        }
    SECTION("Read Tags Css")
        {
        const wchar_t* text = L"body style=\"color: #FF0000;\">there<style width=250>world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"color", true, false) == L"#FF0000");
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, false) == L""); // width is in another tag
        CHECK(html_extract_text::read_attribute_as_string(text, L"size", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(nullptr, L"width", true, false) == L"");
        CHECK(html_extract_text::read_attribute_as_string(text, { L"", 0 }, true, false) == L"");
        }
    SECTION("Read Tags Spaces And Quotes Combinations")
        {
        const wchar_t* text = L"body style=\'font-weight: really bold;\'>";
        CHECK(html_extract_text::read_attribute_as_string(text, L"font-weight", false, false) == L"");//inside of quotes, won't be found
        CHECK(html_extract_text::read_attribute_as_string(text, L"font-weight", false, true) == L"");//inside of quotes, won't be found
        CHECK(html_extract_text::read_attribute_as_string(text, L"font-weight", true, false) == L"really");//not allowing spaces in value, "bold" won't be seen
        CHECK(html_extract_text::read_attribute_as_string(text, L"font-weight", true, true) == L"really bold");//will be read properly

        text = L"width=250 px>world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", false, false) == L"250");//not allowing spaces in value, "px" won't be seen
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, false) == L"250");//not allowing spaces in value, "px" won't be seen
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", true, true) == L"250 px");//will be read properly
        CHECK(html_extract_text::read_attribute_as_string(text, L"width", false, true) == L"250 px");//will be read properly
        }
    SECTION("Read Tags With Spaces")
        {
        const wchar_t* text = L"body style=\"Color Value\">there<style width=250>world<br >!";
        CHECK(html_extract_text::read_attribute_as_string(text, L"style", false, true) == L"Color Value");
        CHECK(html_extract_text::read_attribute_as_string(text, L"style", false, false) == L"Color");
        }
    SECTION("Get Element Name")
        {
        CHECK(html_extract_text::get_element_name(L"body style=\"color=#FF0000\">there<style width=250>world<br >!", true) == L"body");
        CHECK(html_extract_text::get_element_name(L"br>there<style width=250>world<br >!", true) == L"br");
        CHECK(html_extract_text::get_element_name(L"br/>there<style width=250>world<br >!", true) == L"br");
        CHECK(html_extract_text::get_element_name(L"/br", true) == L"/br");
        CHECK(html_extract_text::get_element_name(L"br>", true) == L"br");
        CHECK(html_extract_text::get_element_name(L"br", true) == L"br");
        CHECK(html_extract_text::get_element_name(nullptr, true).empty());
        CHECK(html_extract_text::get_element_name(L"", true) == L"");
        }
    SECTION("Get Body")
        {
        CHECK(html_extract_text::get_body(L"<html bgcolor=\"red\"><body style=\"color=#FF0000\">there<style width=250>world<br >!</body>") ==
            L"there<style width=250>world<br >!");
        CHECK(html_extract_text::get_body(L"<html bgcolor=\"red\"><bd style=\"color=#FF0000\">there<style width=250>world<br >!</body>") ==
            L"<html bgcolor=\"red\"><bd style=\"color=#FF0000\">there<style width=250>world<br >!</body>");
        CHECK(html_extract_text::get_body(L"").empty());
        }
    SECTION("Null")
        {
        html_extract_text filter_html;
        const wchar_t* p = filter_html(nullptr, 5, true, false);
        CHECK(p == nullptr);
        p = filter_html(L"hello", 0, true, false);
        CHECK(p == nullptr);
        }
    SECTION("Test Missing Semicolon")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"AR&D experts";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"AR&D experts") == 0);
        }
    SECTION("Missing Semicolon No Spaces Either")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"AR&Dexperts";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"AR&Dexperts") == 0);
        }
    SECTION("Missing Semicolon With Space")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"AR& D experts";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"AR& D experts") == 0);
        }
    SECTION("Missing Semicolons Without Space")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"Con & Industrial (C&I) relies on thousands";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Con & Industrial (C&I) relies on thousands") == 0);
        }
    SECTION("Missing Semicolon Valid Entity")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&amp Service";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"& Service") == 0);
        }
    SECTION("Page Breaks")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Page 1</p><p style=\"margin-bottom: 0in; line-height: 100%; page-break-before: Always\">Here is page 2</p><h1 style=\"margin-bottom: 0in; line-height: 100%; page-break-before: AUTo\"><p>Page 3</p></body></html>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\nPage 1\n\n\n\n\fHere is page 2\n\n\n\n\f\n\nPage 3\n\n") == 0);
        }
    SECTION("Valid Entity")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&amp; Service";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"& Service") == 0);
        }
    SECTION("Valid Entity Uppercased")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&AMP; Service";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"& Service") == 0);
        }
    SECTION("Invalid Entity")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&amv; Service";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"? Service") == 0);
        }
    SECTION("Entity With Bad Amp")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&amp;le;";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"≤") == 0);

        text = L"&amp;amp; is an ampersand.";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"& is an ampersand.") == 0);

        // not a known entity, so read as is
        text = L"&amp;blah;.";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&blah;.") == 0);

        // really screwed up, just fix as best you can
        text = L"&amp;amp;amp; is an ampersand.";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&amp; is an ampersand.") == 0);

        // bounds checking
        text = L"&amp;";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&") == 0);

        text = L"&amp; ";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"& ") == 0);
        }
    SECTION("Stray Less Than")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"1 is < 5, right?";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5, right?") == p);
        text = L"<body>1 is < 5 and <i>6</i> is > 5, right?</body>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5 and 6 is > 5, right?") == p);
        text = L"<body>1 is <&nbsp;5 and <i>6</i> is > 5, right?</body>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5 and 6 is > 5, right?") == p);
        text = L"<body>1 is < 5 and <i>6</i> is > 5, right?</body> and 4 < 7";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5 and 6 is > 5, right? and 4 < 7") == p);
        text = L"<body>1 is <&nbsp;5 and <i>6</i> is > 5, right?</body> and 4 < 7";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5 and 6 is > 5, right? and 4 < 7") == p);
        text = L"<body>1 is <&NBSP;5 and <i>6</i> is > 5, right?</body> and 4 < 7";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"1 is < 5 and 6 is > 5, right? and 4 < 7") == p);
        }
    SECTION("Missing Tags")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<style=\'italics' <i>hello</i> there!</body>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        // missing > will cause parser to go to closed unquoted <.  It will then
        // feed in some extra junk into the output, but at least "hello" won't be lost
        CHECK(std::wstring(L"<style=\'italics' hello there!") == p);
        }
    SECTION("Extra Tags")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<img alt=\">\">Well, <i>hello</i> there!</body>";
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Well, hello there!") == p);
        }
    SECTION("CDATA")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"Here is some <![cDaTa[more & text]]> here.";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Here is some more & text here.") == 0);
        }
    SECTION("CDATA Embedded HTML")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"Here is some <![cDaTa[more &amp; text]]> here.";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Here is some more &amp; text here.") == 0);
        }
    SECTION("Bad CDATA")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"Here is some <![cDaTa[more & text here.";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Here is some more & text here.") == 0);
        }
    SECTION("Breaks")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"hello<br>there<br />world<br >!";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\nthere\nworld\n!") == 0);
        text = L"hello<hr>there<HR />world<hr >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\nthere\n\nworld\n\n!") == 0);
        text = L"hello<HR>there<HR />world<HR >!";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"hello\n\nthere\n\nworld\n\n!") == 0);
        }
    SECTION("Mail To Telephone Spaces")
        {
        // should add missing space between word and mail/phone links
        html_extract_text filter_html;
        const wchar_t* text = L"Contact<a href='mailto:person@mail.com'>mailto:person@mail.com for details.";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Contact mailto:person@mail.com for details.") == 0);

        text = L"Contact<a href='tel:555-5555'>555-5555 for details.";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"Contact 555-5555 for details.") == 0);
        }
    SECTION("Link list with break")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a><br ><a href=''>Email</a>, <a href=''>Mail</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\tPrayer Card\n\n\tEmail, \n\tMail, \n\tCall 555-5555" });
        }
    SECTION("Link list with image")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a><img src='flower.png'><a href=''>Email</a>, <a href=''>Mail</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\tPrayer Card\n\tEmail, \n\tMail, \n\tCall 555-5555" });
        }
    SECTION("Link list")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a><a href=''>Email</a>, <a href=''>Mail</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\tPrayer Card\n\tEmail, \n\tMail, \n\tCall 555-5555" });
        }
    SECTION("Link list lots of spaces")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a><a href=''>Email</a>      ,        <a href=''>Mail</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\tPrayer Card\n\tEmail      ,        \n\tMail, \n\tCall 555-5555" });
        }
    SECTION("Link list with trailing content")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a> <a href=''>Email</a>, <a href=''>Mail</a>, <a href=''>Call</a> 555-5555<p>Some more content</p>";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\tPrayer Card \n\tEmail, \n\tMail, \n\tCall 555-5555\n\nSome more content\n\n" });
        }
    SECTION("Link list empty")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''></a><a href=''></a><a href=''></a><a href=''></a>";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\t\n\t\n\t\n\t" });
        }
    SECTION("Link list empty trailing content")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''></a><a href=''></a><a href=''></a><a href=''></a><p>Some more content</p>";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\n\n\t\n\t\n\t\n\t\n\nSome more content\n\n" });
        }
    SECTION("Link list breaks, overlapping anchors")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card<a href=''> Email</a></a>, <a href=''>Mail</a>, <a href=''>Call</a> 555-5555<p>Some more content</p>";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\nPrayer Card Email, Mail, Call 555-5555\n\nSome more content\n\n" });
        }
    SECTION("Link list breaks, not enough links")
        {
        // needs 4 links, only has 3
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\nPrayer Card, Call 555-5555" });
        }
    SECTION("Link list breaks from extra text content")
        {
        // text content between links causes them to not be a link list
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a> (extras available!) <a href=''>Email</a> <a href=''>Mail</a> <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\nPrayer Card (extras available!) Email Mail Call 555-5555" });
        }
    SECTION("Link list breaks from too wide extra content")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<p>Contact:</p><a href=''>Prayer Card</a> <a href=''>Email</a>, ||<a href=''>Mail</a>, <a href=''>Call</a> 555-5555";
        const std::wstring res = filter_html(text, std::wcslen(text), true, false);
        CHECK(res == std::wstring{ L"\n\nContact:\n\nPrayer Card Email, ||Mail, Call 555-5555" });
        }
    SECTION("Template placeHolders")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<a class = "breadcrumbs__link" href = "index.php">Mr. ${_EscapeTool.xml($level.title)}</a>)";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Mr. ") == std::wstring(p));

        text = L"<a>$5.00</a>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"$5.00") == std::wstring(p));

        text = L"<a>I have $5.00</a>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"I have $5.00") == std::wstring(p));

        text = LR"(<a class = "breadcrumbs__link" href = "index.php">Mr. ${ Smith</a>)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Mr. ${ Smith") == std::wstring(p));

        text = LR"(<a class = "breadcrumbs__link" href = "index.php">Mr. $</a>)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Mr. $") == std::wstring(p));

        text = LR"(<a class = "breadcrumbs__link" href = "index.php">Mr. ${</a>)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Mr. ${") == std::wstring(p));
        }
    SECTION("Entity Names")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"&clubs;&dagger;&trade;&euro;&le;&minus;&uarr;";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"♣†™€≤−↑") == 0);
        }
    SECTION("Charset")
        {
        const char* text = "<meta http-equiv=\"Expires\" content=\"Sat, 16 Nov 2002 00:00:01 GMT\" />"
                            "<meta http-equiv=\"Content-type\" content=\"text/html;charset=utf-8\" />";
        CHECK(html_extract_text::parse_charset(nullptr, 1) == "");
        CHECK(html_extract_text::parse_charset("nothing in here", 15) == "");
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        text = "<meta http-equiv=\"Content-type\" content=\"text/html;charset='utf-8'\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        text = "<meta http-equiv=\"Content-type\" content=\"text/html;charset= utf-8 \" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        text = "<meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-type\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        text = "<meta http-equiv=\"Content-type\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "");
        text = "<meta http-equiv=\"Content-type\" /><meta http-equiv=\"Content-type\" content=\"text/html;charset=utf-8\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        // missing "charset", which is wrong but it happens
        text = "<meta http-equiv=\"Content-type\" content=\"text/html; 'utf-8'\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        text = "<meta http-equiv=\"Content-type\" content=\"text/html; utf-8\" />";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "utf-8");
        }
    SECTION("Charset XML")
        {
        const char* text = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
        CHECK(html_extract_text::parse_charset(text, strlen(text)) == "UTF-8");
        const char* text2 = "<?xml version=\"1.0\" standalone=\"yes\"?>";
        CHECK(html_extract_text::parse_charset(text2, strlen(text2)) == "");
        }
    SECTION("Option List")
        {
        const wchar_t* text = L"<select><option>Volvo</option><option>Saab</option><option>Mercedes</option></select>";
        html_extract_text filter_html;
        std::wstring p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"\n\n\n\nVolvo\n\nSaab\n\nMercedes\n\n") == p);
        text = L"<SELECT><OPTION>Volvo</OPTION><OPTION>Saab</OPTION><OPTION>Mercedes</OPTION></SELECT>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"\n\n\n\nVolvo\n\nSaab\n\nMercedes\n\n") == p);
        }
    SECTION("Unordered List")
        {
        const wchar_t* text = L"<ul><li>Volvo</li></ul>";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\n\n\tVolvo\n\n") == 0);
        text = L"<UL><LI>Volvo</LI></UL>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\n\n\tVolvo\n\n") == 0);
        }
    SECTION("Ordered List")
        {
        const wchar_t* text = L"<ol><li>Volvo</li></ol>";
        html_extract_text filter_html;
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\n\n\tVolvo\n\n") == 0);
        text = L"<OL><LI>Volvo</LI></OL>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"\n\n\n\tVolvo\n\n") == 0);
        }
    SECTION("Find Element")
        {
        const wchar_t* const text = L"<br />world<br ><br-eak><br><br>";
        const wchar_t* next = html_extract_text::find_element(text, text+wcslen(text), L"br", true);
        CHECK(next == text);
        ++next;
        next = html_extract_text::find_element(next, next+wcslen(next), L"br", true);
        CHECK(next == text+11);
        ++next;
        next = html_extract_text::find_element(next, next+wcslen(next), L"br", true);
        CHECK(next == text+24);
        ++next;
        // note that we are stopping short of the last break in our search as part of the test
        const wchar_t* badNext = html_extract_text::find_element(next, next+(wcslen(next)-4), L"br", true);
        CHECK(badNext == nullptr);
        ++next;
        next = html_extract_text::find_element(next, next+wcslen(next), L"br", true);
        CHECK(next == text+28);

        // test nulls
        CHECK(html_extract_text::find_element(nullptr, text+wcslen(text), L"br", true) == nullptr);
        CHECK(html_extract_text::find_element(text, nullptr, L"br", true) == nullptr);
        CHECK(html_extract_text::find_element(text, text+wcslen(text), L"", true) == nullptr);
        }
    SECTION("Find Closing Element")
        {
        const wchar_t* const text = L" </br ></br eak></br></br>";
        const wchar_t* next = html_extract_text::find_closing_element(text, text+wcslen(text), L"br");
        CHECK(next == text+1);
        ++next;
        next = html_extract_text::find_closing_element(next, next+wcslen(next), L"br");
        CHECK(next == text+7);
        ++next;
        next = html_extract_text::find_closing_element(next, next+wcslen(next), L"br");
        CHECK(next == text+16);
        ++next;
        // note that we are stopping short of the last break in our search as part of the test
        const wchar_t* badNext = html_extract_text::find_closing_element(next, next+(wcslen(next)-4), L"br");
        CHECK(badNext == nullptr);
        ++next;
        next = html_extract_text::find_closing_element(next, next+wcslen(next), L"br");
        CHECK(next == text+21);

        // test nulls
        CHECK(html_extract_text::find_closing_element(nullptr, text+wcslen(text), L"br") == nullptr);
        CHECK(html_extract_text::find_closing_element(text, nullptr, L"br") == nullptr);
        CHECK(html_extract_text::find_closing_element(text, text+wcslen(text), L"") == nullptr);
        }
    SECTION("Find Closing Element Overlap")
        {
        const wchar_t* const text = L"<table>text<table>more text</table><br /> </table>";
        const wchar_t* next = html_extract_text::find_closing_element(text, text+wcslen(text), L"table");
        CHECK(next == text+42);
        }
    SECTION("FindClosingElementNoClosing")
        {
        const wchar_t* const text = L"<table>text<table>more text</table><br /><";
        const wchar_t* next = html_extract_text::find_closing_element(text, text+wcslen(text), L"table");
        CHECK(next == nullptr);
        }
    SECTION("FindClosingElementNoHtml")
        {
        const wchar_t* const text = L"This isn't real HTML text";
        const wchar_t* next = html_extract_text::find_closing_element(text, text+wcslen(text), L"table");
        CHECK(next == nullptr);
        }
    SECTION("FindClosingElementBadFormatting")
        {
        const wchar_t* const text = L"<table>text<table>more text</table><<br /> </table>";
        const wchar_t* next = html_extract_text::find_closing_element(text, text+wcslen(text), L"table");
        CHECK(next == text+43);
        }
    SECTION("Descriptions")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<head>
                              <meta charset="utf-8" />
                              <meta http-equiv="X-UA-Compatible" content="IE=edge" />
                              <title>About the Author | Readability Studio 2021 Manual</title>
                              <meta name="description" content=" About the Author  &amp; Readability Studio 2021 Manual" />
                              <meta name="author" content="Blake Madden" />
                            </head>)";
        [[maybe_unused]] const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"About the Author & Readability Studio 2021 Manual") == filter_html.get_description());
        }
    SECTION("Author")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<head>
                              <meta charset="utf-8" />
                              <meta http-equiv="X-UA-Compatible" content="IE=edge" />
                              <title>About the Author  | Readability Studio 2021 Manual </title>
                              <meta name="description" content="About the Author &amp; Readability Studio 2021 Manual" />
                              <meta name="author" content=" Blake  &amp; Nancy" />
                            </head>)";
        [[maybe_unused]] const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Blake & Nancy") == filter_html.get_author());
        }
    SECTION("Keywords")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<head>
                              <meta charset="utf-8" />
                              <meta http-equiv="X-UA-Compatible" content="IE=edge" />
                              <meta name="keywords" content=" Debugging &amp; Testing" />
                              <meta name="description" content="About the Author &amp; Readability Studio 2021 Manual" />
                              <meta name="author" content="Blake &amp; Nancy" />
                            </head>)";
        [[maybe_unused]] const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Debugging & Testing") == filter_html.get_keywords());
        }
    SECTION("Subject")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<head>\n<subject>Anthro. &amp; Geo Studies</subject>\n</head>";
        [[maybe_unused]] const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Anthro. & Geo Studies") == filter_html.get_subject());
        }
    SECTION("Filled With Nulls")
        {
        html_extract_text filter_html;
        wchar_t* text = new wchar_t[101];
        std::wmemset(text, 0, 101); // 101 place is the last null terminator
        std::wstring_view SPAN_V{ L"<span>List.</span> \r\n (pane)" };
        std::copy(SPAN_V.cbegin(), SPAN_V.cend(), text);
        const wchar_t* p = filter_html(text, 100, true, false);
        CHECK(std::wcscmp(p, L"List.    (pane)") == 0);
        delete[] text;
        }
    SECTION("Embedded JS Quotes")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<html>Hello <input type='submit' id='gform_submit_button_12' class='gform_button button' value='Submit'  onclick='if(window["gf_submitting_12"]){return false;}  window["gf_submitting_12"]=true;  ' onkeypress='if( event.keyCode == 13 ){ if(window["gf_submitting_12"]){return false;} window["gf_submitting_12"]=true;  jQuery("#gform_12").trigger("submit",[true]); }' />there</html>)";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello \n\nthere") == p);
        }
    SECTION("Embedded JS Quotes 2")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(<html>Hello <input type="submit" id="gform_submit_button_12" class="gform_button button" value="Submit"  onclick="if(window['gf_submitting_12']){return false;}  window['gf_submitting_12']=true;  " onkeypress="if( event.keyCode == 13 ){ if(window["gf_submitting_12"]){return false;} window["gf_submitting_12"]=true;  jQuery('#gform_12').trigger('submit', [true]); }" />there</html>)";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello \n\nthere") == p);
        }
    SECTION("Elements with Quotes")
        {
        html_extract_text filter_html;
        const wchar_t* text = LR"(Hello <a hef="submit">there)";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        text = LR"(Hello <a hef='submit'>there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        text = LR"(Hello <a hef='su"b"mit'>there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        text = LR"(Hello <a hef="sub'm'it">there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        text = LR"(Hello <a hef='submit' name="name" value="5">there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        text = LR"(Hello <a hef='su<>bmit' name="na<>me" value="<5">there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello there") == p);

        // Mismatch, will be trash. Just read what we can.
        text = LR"(Hello <a hef='submit">there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello ") == p);

        text = LR"(Hello <a hef="submit'>there)";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring(L"Hello ") == p);
        }
    SECTION("Compare Entities")
        {
        const wchar_t* text = L"<span>List.</span> \r\n (pane)";
        CHECK(html_extract_text::compare_element(text+1, L"sPaN", false));
        CHECK_FALSE(html_extract_text::compare_element(text+1, L"sPa", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+1, L"sPaN", false));
        CHECK(html_extract_text::compare_element_case_sensitive(text+1, L"span", false));
        }
    SECTION("Compare Entities Ignore Terminated")
        {
        const wchar_t* text = L"<span/>List.<span><span /> \r\n <span  ";
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+1, L"span", false));
        CHECK(html_extract_text::compare_element_case_sensitive(text+13, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+13, L"SPAN", false)); // wrong case
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+19, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+31, L"span", false));

        CHECK_FALSE(html_extract_text::compare_element(text+1, L"span", false));
        CHECK(html_extract_text::compare_element(text+13, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element(text+19, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element(text+31, L"span", false));
        }
    SECTION("Compare Entities Ignore Terminated Has Atritubes")
        {
        const wchar_t* text = L"<span/>List.<span bg=\"red\"><span bg=\"red\"/> \r\n <span";
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+1, L"span", false));
        CHECK(html_extract_text::compare_element_case_sensitive(text+13, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+28, L"SPAN", false)); // wrong case
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+28, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+48, L"span", false));

        CHECK_FALSE(html_extract_text::compare_element(text+1, L"span", false));
        CHECK(html_extract_text::compare_element(text+13, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element(text+28, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element(text+48, L"span", false));
        }
    SECTION("Compare Entities Null And Empty")
        {
        const wchar_t* text = L"<span>List.</span> \r\n (pane)";
        CHECK_FALSE(html_extract_text::compare_element(L"", L"sPaN", false));
        CHECK_FALSE(html_extract_text::compare_element(nullptr, L"sPa", false));
        CHECK_FALSE(html_extract_text::compare_element(text, L"", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(L"", L"sPaN", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(nullptr, L"span", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text, L"", false));
        }
    SECTION("Compare Entities One Character")
        {
        const wchar_t* text = L"<v>List.</v> \r\n (pane)";
        CHECK(html_extract_text::compare_element(text+1, L"V", false));
        CHECK(html_extract_text::compare_element(text+1, L"v", false));
        CHECK_FALSE(html_extract_text::compare_element(text+1, L"g", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text+1, L"V", false));
        CHECK(html_extract_text::compare_element_case_sensitive(text+1, L"v", false));
        }
    SECTION("Compare Entities One Character Null And Empty")
        {
        const wchar_t* text = L"<v>List.</v> \r\n (pane)";
        CHECK_FALSE(html_extract_text::compare_element(L"", L"v", false));
        CHECK_FALSE(html_extract_text::compare_element(nullptr, L"v", false));
        CHECK_FALSE(html_extract_text::compare_element(text, L"", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(L"", L"v", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(nullptr, L"v", false));
        CHECK_FALSE(html_extract_text::compare_element_case_sensitive(text, L"", false));
        }
    SECTION("NewLine Removal")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<span>List.</span> \r\n (pane)";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"List.    (pane)" });
        }
    SECTION("Pre")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<pre>Line\n\nLine2\n\nLine3</pre>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"Line\n\nLine2\n\nLine3" });
        }
    SECTION("Ignore Soft Hyphen")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"inter&shy;ntional";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"interntional" });

        text = L"inter&#173;ntional";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"interntional" });

        text = L"inter&#xAD;ntional";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wstring{ p } == std::wstring{ L"interntional" });
        }
    SECTION("Symbol Font")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<font face=\"symbol\">&amp;ABGDEZHQIKLMNXOPRSTUFCYWVJABGDEZHQIKLMNXOPRSTUFCYW</font><font face=\"Arial\">Some regular text.</font>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩςϑΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩSome regular text.") == 0);
        text = L"<FONT FACE=\"SYMBOL\">&amp;ABGDEZHQIKLMNXOPRSTUFCYWVJABGDEZHQIKLMNXOPRSTUFCYW</FONT><FONT FACE=\"Arial\">Some regular text.</FONT>";
        p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩςϑΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩSome regular text.") == 0);
        }
    SECTION("Symbol Serif Font")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<font face=\"Symbol,Serif\">&amp;ABGDEZHQIKLMNXOPRSTUFCYWVJABGDEZHQIKLMNXOPRSTUFCYW</font><font face=\"Arial\">Some regular text.</font>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"&ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩςϑΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩSome regular text.") == 0);
        }
    SECTION("Symbol Math")
        {
        html_extract_text filter_html;
        const wchar_t* text = L"<font face=\"Symbol,Serif\">£-­</font><font face=\"Arial\">Some regular text.</font>";
        const wchar_t* p = filter_html(text, std::wcslen(text), true, false);
        CHECK(std::wcscmp(p, L"≤−↑Some regular text.") == 0);
        }
    }

TEST_CASE("JS Parser", "[html import]")
    {
    SECTION("Cookie Null")
        {
        std::wstring_view text = LR"()";

        CHECK(javascript_hyperlink_parse::get_cookies(text).empty());
        }
    SECTION("Cookie")
        {
        std::wstring_view text =
            LR"(Click to continue
<script language=javascript>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="theToken=1; expires=" + expires + "; path="/"";
</script>
Some more html text on the page.)";

        CHECK(javascript_hyperlink_parse::get_cookies(text) == std::wstring{ L"theToken=1" });
        }
    SECTION("Cookies")
        {
        std::wstring_view text = LR"(Click to continue
<script>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="max-age=1";
</script>
<script language=javascript>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="theToken=1; expires=" + expires + "; path="/"";
</script>
Some more html text on the page.)";

        CHECK(javascript_hyperlink_parse::get_cookies(text) == std::wstring{ L"max-age=1; theToken=1" });
        }
    SECTION("Cookies Missing Value")
        {
        std::wstring_view text = LR"(Click to continue
<script>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="name=" + name;
</script>
<script language=javascript>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="theToken=1; expires=" + expires + "; path="/"";
</script>
Some more html text on the page.)";

        CHECK(javascript_hyperlink_parse::get_cookies(text) ==
              std::wstring{ L"theToken=1" });
        }
    SECTION("Cookies Bad Script Section")
        {
        std::wstring_view text = LR"(Click to continue
<script>
var expires = dateToUTCString();
var name = 'Joe';
document.cookie ="name=" + name;

Some more html text on the page.)";

        CHECK(javascript_hyperlink_parse::get_cookies(text).empty());
        }
    SECTION("Cookies No Script Section")
        {
        std::wstring_view text = LR"(Click to continue
Some more html text on the page.)";

        CHECK(javascript_hyperlink_parse::get_cookies(text).empty());
        }
    }

TEST_CASE("Hyperlink Parser", "[html import]")
    {
    SECTION("Null")
        {
        const wchar_t* text2 = nullptr;
        html_hyperlink_parse parse2(text2, 0);

        CHECK(parse2() == nullptr);
        CHECK(parse2.get_base_url() == nullptr);
        CHECK(parse2.get_base_url_length() == 0);
        }

    SECTION("Url End Null")
        {
        CHECK(html_hyperlink_parse::find_url_end(nullptr) == nullptr);
        }

    SECTION("Url End")
        {
        const wchar_t* text = L"http://A&Pcompany.com?=a\'some text";
        CHECK(html_hyperlink_parse::find_url_end(text) == text+24);
        }

    SECTION("Hyperlink Image Map")
        {
        const wchar_t* text = L"<area shape=\"circle\" coords=\"171,156,6\" alt=\"\" href=\"www.mysite\">";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"www.mysite", 10) == 0);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 10);
        }

    SECTION("Script links")
        {
        const wchar_t* text = L"menunum=0;menus=new Array();_d=document;function addmenu(){menunum++;menus[menunum]=menu;}function dumpmenus(){mt=\"<script language=javascript>\";for(a=1;a<menus.length;a++){mt+=\" menu\"+a+\"=menus[\"+a+\"];\"}mt+=\"<\\script>\"\"www.yahoo.com/page.htm\";_d.write(mt)}";
        javascript_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wstring{ parse(), parse.get_current_hyperlink_length() } == std::wstring{ L"www.yahoo.com/page.htm" });
        }
    SECTION("Script links 2")
        {
        const wchar_t* text = L"effect = \"fade(duration=0.3);Shadow(color='#777777', Direction=135, Strength=5)\" // Special";
        javascript_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(parse() ==  nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        }
    SECTION("Script links 3")
        {
        std::wstring_view text = LR"(a=e+"getval(.gui"; src="www.yahoo.com")";

        javascript_hyperlink_parse js(text.data(), text.length());
        js();
        CHECK(std::wstring{ js.get_current_link(), js.get_current_hyperlink_length() } == std::wstring{ L"www.yahoo.com" });
        CHECK(js() == nullptr);
        }

    SECTION("Url End Not Found")
        {
        const wchar_t* text = L"http://company.com?=a";
        CHECK(html_hyperlink_parse::find_url_end(text) == text+21);
        }

    SECTION("Bad Link")
        {
        const wchar_t* text = L"<A HREF=\"../company/success_stories/pdf/casestudy_gp_<i>STATISTICS Enterprise/QC</i>2.pdf\" target=_blank><IMG SRC=\"images/gp.gif\" WIDTH=130 HEIGHT=60 ALT=\"\" border=\"0\"></center></a>";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        const wchar_t* parsed = parse();

        CHECK(std::wcsncmp(parsed, L"../company/success_stories/pdf/casestudy_gp_<i>STATISTICS Enterprise/QC</i>2.pdf", 80) == 0);

        html_extract_text extract;
        const wchar_t* extracted = extract(text+9, 80, true, false);

        CHECK(std::wcscmp(extracted, L"../company/success_stories/pdf/casestudy_gp_STATISTICS Enterprise/QC2.pdf") == 0);
        }

    SECTION("Redirect")
        {
        const wchar_t* text = L"<meta name=layout-width content=717><meta name=date content=\"06 12, 2001 2:34:12 PM\"><meta HTTP-EQUIV=REFRESH CONTENT=\"0;URL=Results.htm\">";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"Results.htm", 11) == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("Redirect With Quotes")
        {
        const wchar_t* text = L"<meta name=layout-width content=717><meta name=date content=\"06 12, 2001 2:34:12 PM\"><meta HTTP-EQUIV=REFRESH CONTENT=\"0;URL='Results.htm'\">";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"Results.htm", 11) == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("Redirect Malformed")
        {
        const wchar_t* text = L"<meta name=layout-width content=717><meta name=date content=\"06 12, 2001 2:34:12 PM\"><meta HTTP-EQUIV=REFRESH CONTENT=\"0;URL=Results.htm <a href=\"page.htm\">";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"page.htm", 8) == 0);
        CHECK(parse() == nullptr);
        }
    
    SECTION("Leading space")
        {
        const wchar_t* text = LR"(<a href=" https://depauwtigers.com/landing/index" target="_blank">Athletics</a>)";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wstring{ parse(), parse.get_current_hyperlink_length() } == std::wstring{ L"https://depauwtigers.com/landing/index" });
        CHECK(parse() == nullptr);
        }

    SECTION("Hyperlink")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a>"
            "some text <iMg SRc=image.png>picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a>"
            " <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse.get_base_url(), L"www.mysite", 10) == 0);
        CHECK(parse.get_base_url_length() == 10);

        CHECK(std::wcsncmp(parse(), L"www.page.com", 12) == 0);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 12);

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.is_current_link_an_image() == true);
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(std::wcsncmp(parse(), L"404", 3) == 0);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 3);

        //couple of empty hyperlinks will be skipped here

        auto result = parse();
        CHECK(std::wstring(L"/scripts/statmenu4.js") == std::wstring(result, parse.get_current_hyperlink_length()));
        CHECK(parse.is_current_link_a_javascript() == true);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK(parse.get_current_hyperlink_length() == 21);

        CHECK(parse() == nullptr);

        CHECK(parse() == nullptr);
        }

    SECTION("Hyperlink And Script")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\">image1.src = \"www.pages2.com/images/lblinkon.gif\";</Script><scripT type=\"text/javascript\">image1.src = \"www.yahoo.com/images/lblinkon2.gif\";</Script><A hRef=\"www.page2.com\">page</a>";
        html_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse.get_base_url(), L"www.mysite", 10) == 0);
        CHECK(parse.get_base_url_length() == 10);

        CHECK(std::wcsncmp(parse(), L"www.page.com", 12) == 0);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 12);

        CHECK(std::wcsncmp(parse(), L"/scripts/statmenu4.js", 21) == 0);
        CHECK(parse.is_current_link_a_javascript() == true);
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK(parse.get_current_hyperlink_length() == 21);

        CHECK(std::wstring{ parse(), parse.get_current_hyperlink_length() } == std::wstring{  L"www.pages2.com/images/lblinkon.gif" });
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 34);

        CHECK(std::wstring{ parse(), parse.get_current_hyperlink_length() } == std::wstring{ L"www.yahoo.com/images/lblinkon2.gif" });
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 34);

        CHECK(std::wstring{ parse(), parse.get_current_hyperlink_length() } == std::wstring{ L"www.page2.com" });
        CHECK_FALSE(parse.is_current_link_an_image());
        CHECK_FALSE(parse.is_current_link_a_javascript());
        CHECK(parse.get_current_hyperlink_length() == 13);
        }
    }

TEST_CASE("Html Url Format", "[html import]")
    {
    SECTION("Domains")
        {
        html_url_format formatHtml(L"http://www.business.yahoo.com");
        formatHtml({ L"http://www.sales.mycompany.com", 30 }, false);
        CHECK(formatHtml.get_root_domain() == L"yahoo.com");
        CHECK(formatHtml.get_root_full_domain() == L"http://www.business.yahoo.com");
        CHECK(formatHtml.get_full_domain() == L"http://www.sales.mycompany.com");
        CHECK(formatHtml.get_domain() == L"mycompany.com");
        }
    SECTION("Domains2")
        {
        html_url_format formatHtml(L"http://www.business.yahoo.com/index.htm");
        formatHtml({ L"http://www.sales.mycompany.com/index.htm", 40 }, false);
        CHECK(formatHtml.get_root_domain() == L"yahoo.com");
        CHECK(formatHtml.get_root_subdomain() == L"business.yahoo.com");
        CHECK(formatHtml.get_root_full_domain() == L"http://www.business.yahoo.com");
        CHECK(formatHtml.get_full_domain() == L"http://www.sales.mycompany.com");
        CHECK(formatHtml.get_domain() == L"mycompany.com");
        CHECK(formatHtml.get_subdomain() == L"sales.mycompany.com");
        }
    SECTION("Absolute Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t* p = formatHtml({ L"http://blah.com/page.html", 25 }, false);
        CHECK(std::wstring{ p } == std::wstring{ L"http://blah.com/page.html" });
        }
    SECTION("SMS Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t* p = formatHtml(L"sms:?&body=http://yahoo.com/page1.html", false);
        CHECK(std::wstring{ p } == std::wstring{ L"http://yahoo.com/page1.html" });
        }
    SECTION("Trailing Quote Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        CHECK(std::wstring{ formatHtml(L"http://yahoo.com/pic.jpg&quot;alt='page'&quot;", true) } ==
              std::wstring{ L"http://yahoo.com/pic.jpg" });
        CHECK(formatHtml(L"&quot;&quot;&quot;", true) == nullptr);
        }
    SECTION("Trailing Ampersand Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        CHECK(std::wstring{ formatHtml(L"http://yahoo.com/pic.jpg&amp;&quot;alt='page'&quot;", true) } ==
              std::wstring{ L"http://yahoo.com/pic.jpg" });
        CHECK(formatHtml(L"&amp;&amp;&amp;", true) == nullptr);
        }
    SECTION("Font Size Bad Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        CHECK(formatHtml(L"12.3vw", true) == nullptr);
        CHECK(formatHtml(L"100em", true) == nullptr);
        CHECK(formatHtml(L"100rem", true) == nullptr);
        CHECK(formatHtml(L"1.2pt", true) == nullptr);
        CHECK(formatHtml(L"1.2px", true) == nullptr);
        CHECK(formatHtml(L"1.2vh", true) == nullptr);
        CHECK(formatHtml(L"1.2ex", true) == nullptr);
        CHECK(formatHtml(L"1.2in", true) == nullptr);
        CHECK(formatHtml(L"12.3vw/", true) == nullptr);
        CHECK(std::wstring{ formatHtml(L"118.125512/image.png", true) } ==
              std::wstring{ L"http://mypage.com/blahblahblah/118.125512/image.png" });
        }
    SECTION("Base Domain Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t* p = formatHtml({ L"/page.html", 10 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        }
     SECTION("Relative Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t* p = formatHtml({ L"page.html", 9 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/blahblahblah/page.html") == 0);
        }
    SECTION("Relative Link2")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t* p = formatHtml({ L"./page.html", 11 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/blahblahblah/page.html") == 0);
        p = formatHtml({ L"/page.html" }, false);
        CHECK(std::wstring{ p } == std::wstring{ L"http://mypage.com/page.html" });
        p = formatHtml({ L"//page.html" }, false);
        CHECK(std::wstring{ p } == std::wstring{ L"http://mypage.com/page.html" });
        }
    SECTION("Relative Link3")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/index.html");
        const wchar_t* p = formatHtml({ L"../page.html", 12 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        }
    SECTION("Relative Link4")
        {
        html_url_format formatHtml(L"http://mypage.com/first/second/third/index.html");
        const wchar_t* p = formatHtml({ L"../../../page.html#start", 18 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        CHECK(formatHtml.get_domain() == L"mypage.com");
        }
    SECTION("Relative Link Outside Link")
        {
        html_url_format formatHtml(L"http://mypage.com/blahblahblah/");
        const wchar_t*  p = formatHtml({ L"//www.yahoo.com" }, false);
        CHECK(std::wstring{ p } == std::wstring{ L"www.yahoo.com" });
        }
    SECTION("Relative Link Bad")
        {
        html_url_format formatHtml(L"http://mypage.com/");
        const wchar_t* p = formatHtml({ L"../../../page.html#start", 18 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        }
    SECTION("Relative Link Bad2")
        {
        html_url_format formatHtml(L"http://mypage.com/");
        const wchar_t* p = formatHtml({ L"../page.html#start", 12 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        CHECK(formatHtml.get_domain() == L"mypage.com");
        }
    SECTION("Relative Link Bad3")
        {
        html_url_format formatHtml(L"http://mypage.com");
        const wchar_t* p = formatHtml({ L"../page.html#start", 12 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        CHECK(formatHtml.get_domain() == L"mypage.com");
        }
    SECTION("Query Link")
        {
        html_url_format formatHtml(L"http://mypage.com/query.php?blah");
        const wchar_t* p = formatHtml({ L"page.html", 9 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        p = formatHtml({ L"?page.html", 10 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/query.php?page.html") == 0);
        }
    SECTION("Bookmark Link")
        {
        html_url_format formatHtml(L"http://mypage.com/");
        const wchar_t* p = formatHtml({ L"page.html#blah", 14 }, false);
        CHECK(std::wcscmp(p, L"http://mypage.com/page.html") == 0);
        }
    SECTION("Get Domain")
        {
        html_url_format formatHtml(L"http://pages.mypage.com/blah/blah");
        CHECK(formatHtml.get_full_domain() == L"http://pages.mypage.com");
        CHECK(formatHtml.get_domain() == L"mypage.com");
        }
    SECTION("Get Directory Path")
        {
        html_url_format formatHtml(L"http://mypage.com/blah/blah.html");
        CHECK(formatHtml.get_directory_path() == L"mypage.com/blah");
        }
    SECTION("Get Directory Path With Subdomain")
        {
        html_url_format formatHtml(L"http://business.mypage.com/blah/blah.html");
        CHECK(formatHtml.get_directory_path() == L"business.mypage.com/blah");
        }
    SECTION("No Protocal")
        {
        html_url_format formatHtml(L"www.mypage.com");
        const wchar_t* p = formatHtml({ L"page.html", 9 }, false);
        CHECK(std::wcscmp(p, L"www.mypage.com/page.html") == 0);
        }
    SECTION("Url Image Parse")
        {
        html_url_format formatHtml(L"");
        CHECK(formatHtml.parse_image_name_from_url(L"www.mypage.com?Image=hi.jpg&loc=location") == L"hi.jpg");
        CHECK(formatHtml.parse_image_name_from_url(L"www.mypage.com?loc=location&Image=hi.jpg") == L"hi.jpg");
        CHECK(formatHtml.parse_image_name_from_url(L"www.mypage.com?loc=location&pic=hi.jpg") == L"");
        CHECK(formatHtml.parse_image_name_from_url(L"www.mypage.com/hi.jpg") == L"");
        CHECK(formatHtml.parse_image_name_from_url(L"") == L"");
        }
    SECTION("Url TLD Parse")
        {
        html_url_format formatHtml(L"");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"") == L"");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"wwW.mypage.com?Image=hi.jpg&loc=location") == L"com");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"https://wwW.mypage.com?Image=hi.jpg&loc=location") == L"com");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"www.mypage.org/index.htm") == L"org");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"www.mypage.co.uk/index.htm") == L"co.uk");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"wWw.mypage.co.uk") == L"co.uk");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"http://mypage.co.uk/") == L"co.uk");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"www.mypage") == L"");
        CHECK(formatHtml.parse_top_level_domain_from_url(L"www.mypage.") == L"");
        }
    SECTION("Is Url TLD Parse")
        {
        html_url_format formatHtml(L"");
        CHECK(formatHtml.is_url_top_level_domain(L"") == false);
        CHECK(formatHtml.is_url_top_level_domain(L"www.mypage.org/index.htm") == false);
        CHECK(formatHtml.is_url_top_level_domain(L"www.mypage.co.uk/") == true);
        CHECK(formatHtml.is_url_top_level_domain(L"www.mypage.co.uk") == true);
        CHECK(formatHtml.is_url_top_level_domain(L"http://www.mypage.co.uk/") == true);
        CHECK(formatHtml.is_url_top_level_domain(L"http://www.mypage.co.uk") == true);
        CHECK(formatHtml.is_url_top_level_domain(L"http://www.mypage.co.uk/index.htm") == false);
        }
    }

TEST_CASE("Html Image Parse", "[html import]")
    {
    SECTION("Null")
        {
        const wchar_t* text2 = nullptr;
        html_image_parse parse(text2, 0);

        CHECK(parse() == nullptr);
        }

    SECTION("Image")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=image.png>picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(parse() == nullptr);

        CHECK(parse() == nullptr);
        }

    SECTION("Image Malformed SRC")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc =\"image.png\">picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(parse() == nullptr);

        CHECK(parse() == nullptr);
        }

    SECTION("Image With Spaces")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=\"my image.png\">picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"my image.png", 12) == 0);
        CHECK(parse.get_current_hyperlink_length() == 12);

        CHECK(parse() == nullptr);

        CHECK(parse() == nullptr);
        }

    SECTION("Image With Extra Spaces")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=image.png  >picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(parse() == nullptr);

        CHECK(parse() == nullptr);
        }

    SECTION("Image With Slash")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=\"images/image.png\">picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"images/image.png", 16) == 0);
        CHECK(parse.get_current_hyperlink_length() == 16);

        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("Image With Terminating Slash")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=\"images/image.png\"/><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"images/image.png", 16) == 0);
        CHECK(parse.get_current_hyperlink_length() == 16);

        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("Image Base-64 Encoded")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <img SRc=\"data:image/gif;base64,R0lGODlhAQABAIAAAP///wAAACH5BAEAAAAALAAAAAABAAEAAAICRAEAOw==\"/><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        // don't pick up the encoded image data
        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        }

    SECTION("Images")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=image.png>picture</img><a href=\'404\'>404</A> <img src=mypic.jpg></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(std::wcsncmp(parse(), L"mypic.jpg", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("Images Alt Tags")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg alt=\"sometext\" SrC=image.png>picture</img><a href=\'404\'>404</A> <img src=mypic.jpg></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"image.png", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(std::wcsncmp(parse(), L"mypic.jpg", 9) == 0);
        CHECK(parse.get_current_hyperlink_length() == 9);

        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        CHECK(parse() == nullptr);
        }

    SECTION("No Images")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg>picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_image_parse parse(text, std::wcslen(text) );

        CHECK(parse() == nullptr);
        }
    }

TEST_CASE("JS Link Parse", "[html import]")
    {
    SECTION("Null")
        {
        const wchar_t* text = nullptr;
        javascript_hyperlink_parse parse(text, 0);

        CHECK(parse() == nullptr);
        CHECK(parse.get_current_hyperlink_length() == 0);
        }

    SECTION("Links")
        {
        const wchar_t* text = L",,\"http://www.myco.com\",\"&nbsp;\"\"http://www.myco2.com\",\"myco Inc.\",1";
        javascript_hyperlink_parse parse(text, std::wcslen(text) );

        CHECK(std::wcsncmp(parse(), L"http://www.myco.com", 19) == 0);
        CHECK(std::wcsncmp(parse(), L"http://www.myco2.com", 20) == 0);
        CHECK(parse() == nullptr);
        }
    }

TEST_CASE("HTML Link Strip", "[html import]")
    {
     SECTION("Null")
        {
        const wchar_t* text = nullptr;
        html_strip_hyperlinks strip;

        CHECK(strip(text,0) == nullptr);
        CHECK(strip.get_filtered_text_length() == 0);
        }
    SECTION("Simple")
        {
        const wchar_t* text = L"Hello <A hRef=\"www.page.com\"><b>there</b></a>! some text!";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"Hello <b>there</b>! some text!";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    SECTION("Complex")
        {
        const wchar_t* text = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello <A hRef=\"www.page.com\">page</a> some text <iMg SRc=image.png>picture</img><a href=\'404\'>404</A> <img></img><a href=\"\"></a> <a href=></a><scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"<heAD><baSE hrEf=\"www.mysite\"></base></HEAD> Hello page some text <iMg SRc=image.png>picture</img>404 <img></img> <scripT type=\"text/javascript\" sRC=\"/scripts/statmenu4.js\"></Script>";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    SECTION("Enclosed")
        {
        const wchar_t* text = L"<A hRef=\"www.page.com\"><b>Hello  there</b></a>";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"<b>Hello  there</b>";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    SECTION("No Links")
        {
        const wchar_t* text = L"Hello <b>there</b>! some text!";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"Hello <b>there</b>! some text!";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    SECTION("All Links")
        {
        const wchar_t* text = L"<A hRef=\"www.page.com\"></a><a></a>";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    // bookmarks should NOT be stripped
    SECTION("Bookmark")
        {
        const wchar_t* text = L"Hello <A hRef=\"www.name.com\"><b>there</b></a>! <a name=\"blah\">some</a> text!";
        html_strip_hyperlinks strip;
        const wchar_t* expected = L"Hello <b>there</b>! <a name=\"blah\">some</a> text!";

        CHECK(std::wcscmp(strip(text,std::wcslen(text)), expected) == 0);
        CHECK(strip.get_filtered_text_length() == std::wcslen(expected));
        }
    }

// NOLINTEND
// clang-format on
