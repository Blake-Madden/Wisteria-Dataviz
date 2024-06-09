// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/markdown_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Markdown Parser", "[md import]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(md.has_metadata_section(nullptr) == false);
        CHECK(md.find_metadata_section_end(nullptr) == nullptr);
        CHECK(md({ L"some MD text", 0 }) == nullptr);
        }

    SECTION("Meta Sections")
        {
        lily_of_the_valley::markdown_extract_text md;
        // YAML
        auto mdText = L"---\n   title:my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.has_metadata_section(mdText));
        }

    SECTION("Meta Section End")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"---\ntitle:my book\n---\nHere is the *actual* text to **review**." }) } ==
            std::wstring{ L"Here is the actual text to review." });
        }

    SECTION("HTML List")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ LR"(following list:

<ul>
<li>Interval</li>
<li>Period</li>
<li>Int</li>
<li>More</li>
</ul>

The End.)" }) } ==
             std::wstring{ LR"(following list:



 
	Interval 
	Period 
	Int 
	More 


 The End.)" });
        }

    SECTION("HTML Table")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ LR"(following table:

<table>
<tr>
<th>Interval</th>
<th>Period</th>
<th>Int</th>
<th>More</th>
</tr>
<tr>
<td>0 to 20</td>
<td>8</td>
<td>Supported.</td>
<td>Not supported.</td>
</tr>
</table>

The End.)" }) } ==
             std::wstring{ LR"(following table:



 

 Interval Period Int More  

 	0 to 20 	8 	Supported. 	Not supported.  


 The End.)" });
        }

    SECTION("Newlines")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{
                  md({ L"---\ntitle:my book\n---\nHere is the *actual* text to **review**." }) } ==
              std::wstring{ L"Here is the actual text to review." });
        }

    SECTION("Header")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"# Header1\n### Header2\n Not a #header" }) } ==
              std::wstring{ L"Header1\n\nHeader2\n\n Not a #header" });
        CHECK(std::wstring{ md({ L"Header1\n=========\nHeader2\n--\nNot a =header" }) } ==
            std::wstring{ L"Header1\n\nHeader2\n\nNot a =header" });
        CHECK(std::wstring{ md({ L"# Header1 {.unnumbered}\nText" }) } ==
            std::wstring{ L"Header1 \n\nText" });
        }

    SECTION("Emphasis")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This is *italic* and **bold** and also __italic__. 2 \\* 2." }) } ==
              std::wstring{ L"This is italic and bold and also italic. 2 * 2." });
        CHECK(std::wstring{ md({ L"This is _italic and **bold** text_." }) } ==
            std::wstring{ L"This is italic and bold text." });
        // can't handle "This is *italic and **bold** text*", will have to be a known limitation
        CHECK(std::wstring{ md({ L"**PGF\\_HOT**" }) } ==
            std::wstring{ L"PGF_HOT" });
        CHECK(std::wstring{ md({ L"TIFF _spe_ci**f**i_c_ *options*" }) } ==
            std::wstring{ L"TIFF spe_ci**f**i_c options" });
        CHECK(std::wstring{ md({ L"2 * 2" }) } ==
            std::wstring{ L"2 * 2" });
        // unescaped _ in front will get lost, but read in the rest of it
        CHECK(std::wstring{ md({ L"A **_variant_t** _object_" }) } ==
            std::wstring{ L"A variant_t object" });
        }

    SECTION("Blockquoes")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"> This is a quote\n\n>\n\n> End of Quote" }) } ==
              std::wstring{ L"\tThis is a quote\n\n\t\n\n\tEnd of Quote" });
        // nested
        CHECK(std::wstring{ md({ L"> This is a quote\n\n>\n\n>> End of Quote" }) } ==
            std::wstring{ L"\tThis is a quote\n\n\t\n\n\t\tEnd of Quote" });
        // with header
        CHECK(std::wstring{ md({ L"> # This is a quote header\n\n>\n\n>> End of Quote" }) } ==
            std::wstring{ L"\tThis is a quote header\n\n\t\n\n\t\tEnd of Quote" });
        // indented
        CHECK(std::wstring{ md({ L"    This is a quote\n    End of Quote" }) } ==
            std::wstring{ L"    This is a quote\n    End of Quote" });
        }

    SECTION("Inline code")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This `is code`." }) } ==
              std::wstring{ L"This is code." });
        CHECK(std::wstring{ md({ L"This ```is code```." }) } ==
            std::wstring{ L"This is code." });
        CHECK(std::wstring{ md({ L"Code `r 2+2`." }) } ==
              std::wstring{ L"Code 2+2." });
        CHECK(std::wstring{ md({ L"``2`2`` `shared_ptr`" }) } ==
              std::wstring{ L"2`2 shared_ptr" });
        CHECK(std::wstring{ md({ L"### `std::basic_istream::read` processing of `\\r\\n`` =>`\\n`\n `shared_ptr`" }) } ==
              std::wstring{ L"std::basic_istream::read processing of \\r\\n =>n\n shared_ptr" });
        }

    SECTION("Code block")
        {
        lily_of_the_valley::markdown_extract_text md;
        // inline (you aren't supposed to do this with ```, but people do)
        CHECK(std::wstring{ md({ L"This\n```\nis code\r\nhere```\n." }) } ==
              std::wstring{ L"This \n\tis code\r\n\there\n\n." });
        // remove lang info
        auto blah = std::wstring{ md({ L"This\n```cpp\nis code\r\nhere\n```\n." }) };
        CHECK(std::wstring{ md({ L"This\n```cpp\nis code\r\nhere\n```\n." }) } ==
            std::wstring{ L"This \n\tis code\r\n\there\n\t\n\n." });
        }

    SECTION("Images")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png) the penguin." }) } ==
              std::wstring{ L"Tux  the penguin." });
        // malformed
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot" }) } ==
            std::wstring{ L"Tux [Tux, the Linux mascot" });
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux (/assets/tux.png" });
        }

    SECTION("Links")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"go to [https://visualstudio.microsoft.com/vs/pricing/](https://visualstudio.microsoft.com/vs/pricing/) to explore." }) } ==
              std::wstring{ L"go to https://visualstudio.microsoft.com/vs/pricing/ to explore." });
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot](/assets/tux.png) the penguin." }) } ==
              std::wstring{ L"Tux the Linux mascot the penguin." });
        CHECK(std::wstring{ md({ L"Tux [the **Linux** mascot](/assets/tux.png) the penguin." }) } ==
            std::wstring{ L"Tux the Linux mascot the penguin." });
        // malformed
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot" }) } ==
            std::wstring{ L"Tux [the Linux mascot" });
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux [the Linux mascot](/assets/tux.png" });
        CHECK(std::wstring{ md({ L"The third member function inserts the sequence [`first`, `last`). You use it" }) } ==
            std::wstring{ L"The third member function inserts the sequence [first, last). You use it" });
        // missing link
        CHECK(std::wstring{ md({ L"as an **[out]** parameter." }) } ==
            std::wstring{ L"as an [out] parameter." });
        }

    SECTION("Unordered Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"- list one\nhere\n- Item 2\n\nSome -text." }) } ==
              std::wstring{ L"- list one here\n- Item 2\n\nSome -text." });
        }

    SECTION("Unordered Nested Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"- list one\n  - Item 2" }) } ==
              std::wstring{ L"- list one\n  - Item 2" });
        }

    SECTION("Ordered Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"1. list one\nhere\n256. Item\n2" }) } ==
              std::wstring{ L"1. list one here\n256. Item 2" });
        }

    SECTION("Table")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| --- | ----------- |\n| Header | Title |" }) } ==
              std::wstring{ L"\t| Syntax \t| Description \t|\n\t| --- \t| ----------- \t|\n\t| Header \t| Title \t|" });
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| :-- | ----------: |\n| Header | Title |" }) } ==
              std::wstring{ L"\t| Syntax \t| Description \t|\n\t| :-- \t| ----------: \t|\n\t| Header \t| Title \t|" });
        }

    SECTION("Superscript")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"5^th^ edition \\^5" }) } ==
              std::wstring{ L"5th edition ^5" });
        }

    SECTION("HTML angle")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This is <em>bold</em>." }) } ==
              std::wstring{ L"This is bold." });
        CHECK(std::wstring{ md({ L"This is <a href=\"#bm\">bold</a>." }) } ==
            std::wstring{ L"This is bold." });
        CHECK(std::wstring{ md({ L"<dl> <dt>**PGF\\_HOT**</dt> </dl>" }) } ==
              std::wstring{ L" PGF_HOT " });
        CHECK(std::wstring{ md({ L"2 > 1" }) } ==
            std::wstring{ L"2 > 1" });
        CHECK(std::wstring{ md({ L"2 < 5" }) } ==
            std::wstring{ L"2 < 5" });
        CHECK(std::wstring{ md({ L"Go to <https://website> for more info." }) } ==
            std::wstring{ L"Go to <https://website> for more info." });
        }

    SECTION("HTML tags")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Hi &gt; there &amp; you &quot;." }) } ==
              std::wstring{ L"Hi > there & you \"." });
        CHECK(std::wstring{ md({ L"You &amp me." }) } ==
            std::wstring{ L"You &amp me." });
        CHECK(std::wstring{ md({ L"organization&#39;s" }) } ==
            std::wstring{ L"organization's" });
        CHECK(std::wstring{ md({ L"organization&#X27;s" }) } ==
            std::wstring{ L"organization's" });
        }

    SECTION("Formatted link")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"`shared_ptr`" }) } ==
            std::wstring{ L"shared_ptr" });
        CHECK(std::wstring{ md({ L"[`shared_ptr`](www.website)" }) } ==
              std::wstring{ L"shared_ptr" });
        }
    }

// NOLINTEND
// clang-format on
