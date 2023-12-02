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
        // pandoc
        auto mdText = L"% title: my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.has_metadata_section(mdText));
        // YAML
        mdText = L"---\n   title:my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.has_metadata_section(mdText));
        }

    SECTION("Meta Section End")
        {
        // test \n
        auto mdText = L"keyvalue:12\nKey2: re\n\nHere is the *actual* \\*text to **review**.";
        lily_of_the_valley::markdown_extract_text md;
        auto end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+22);

        // test \r
        mdText = L"keyvalue:12\rKey2: re\r\rHere is the *actual* \\*text to **review**.";
        end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+22);

        // test \r\n combinations
        mdText = L"keyvalue:12\r\nKey2: re\r\n\r\nHere is the *actual* \\*text to **review**.";
        end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+24);

        mdText = L"keyvalue:12\r\nKey2: re\r\n\nHere is the *actual* \\*text to **review**.";
        end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+24);

        // test being at the end of the text
        mdText = L"title\r\n";
        end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+7);

        // test being at the end of the text, because there was no blank lines
        mdText = L"title";
        end = md.find_metadata_section_end(mdText);
        CHECK(end == mdText+5);
        }

    SECTION("Newlines")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"# Header\nThis is\na line.\r\nThis is the same line.  \nThis is a new line.\r\n\r\nAnother line. \nSame line." }) } ==
              std::wstring{ L"Header\n\nThis is a line. This is the same line.  \n\nThis is a new line.\n\nAnother line.  Same line." });
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
        CHECK(std::wstring{ md({ L"**PGF\\_HOT**" }) } ==
            std::wstring{ L"PGF_HOT" });
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
        }

    SECTION("Code block")
        {
        lily_of_the_valley::markdown_extract_text md;
        auto blah = std::wstring{ md({ L"This\n```\nis code\r\nhere```\n." }) };
        CHECK(std::wstring{ md({ L"This\n```\nis code\r\nhere```\n." }) } ==
              std::wstring{ L"This \n\tis code\r\n\there\n\n." });
        }

    SECTION("Images")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png) the penguin." }) } ==
              std::wstring{ L"Tux  the penguin." });
        // malformed
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot" }) } ==
            std::wstring{ L"Tux " });
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux " });
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
            std::wstring{ L"Tux " });
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux " });
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
              std::wstring{ L" | Syntax  | Description  |\n | ---  | -----------  |\n | Header  | Title  |" });
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| :-- | ----------: |\n| Header | Title |" }) } ==
              std::wstring{ L" | Syntax  | Description  |\n | :--  | ----------:  |\n | Header  | Title  |" });
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
