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
        CHECK(md.is_metadata_section(nullptr) == false);
        CHECK(md(nullptr, 100) == nullptr);
        CHECK(md.find_metadata_section_end(nullptr) == nullptr);
        CHECK(md(L"some MD text", 0) == nullptr);
        }

    SECTION("Meta Sections")
        {
        // multimarkdown
        auto mdText = L"keyvalue:12\nKey2: re\n\nHere is the *actual* \\*text to **review**.";
        lily_of_the_valley::markdown_extract_text md;
        CHECK(md.is_metadata_section(mdText));
        // pandoc
        mdText = L"% title: my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.is_metadata_section(mdText));
        // YAML
        mdText = L"---\n   title:my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.is_metadata_section(mdText));
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
    }
