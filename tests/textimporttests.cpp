#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/import/text_row.h"
#include "../src/import/text_column.h"
#include "../src/import/text_matrix.h"
#include "../src/import/text_preview.h"

using namespace Catch::Matchers;

TEST_CASE("Test Tabbed Different Column", "[text import]")
	{
    const wchar_t* fileText = L"First:Second\tThird";

    std::vector<std::vector<std::wstring>> words;

    // single column of words
    lily_of_the_valley::text_preview importPreview;

    // semicolon column
    lily_of_the_valley::text_column_delimited_character_parser scParser(L':');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
        scColumn(scParser, 1);

    // to eol row and column parser
    lily_of_the_valley::text_column_delimited_character_parser parser(L'\t');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
        myColumn(parser, -1);
    lily_of_the_valley::text_row<std::wstring> myRow(std::nullopt);

    myRow.add_column(scColumn);
    myRow.add_column(myColumn);
    myRow.add_column(myColumn);
    myRow.allow_column_resizing(true);

    // preview the file, read it, write to the word list, and finally sort the word list
    lily_of_the_valley::text_matrix<std::wstring>
        importer(&words);
    importer.add_row_definition(myRow);

    size_t rowCount = importPreview(fileText, L'\t', true, false);
    importer.read(fileText, rowCount, 1, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(3 == words[0].size()); // 3 columns in first row
    CHECK(words[0][0] == L"First");
    CHECK(words[0][1] == L"Second");
    CHECK(words[0][2] == L"Third");

    // only read first row (parser will not try to regrow data)
    myRow.allow_column_resizing(false);
    importer.remove_rows();
    importer.add_row_definition(myRow);
    words.clear();

    importer.read(fileText, rowCount, 1, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(1 == words[0].size()); // only first column read
    CHECK(words[0][0] == L"First");
	}

TEST_CASE("Tabbed Unknown Column Count", "[text import]")
    {
    const wchar_t* fileText = L"First\tSecond\tThird";

    std::vector<std::vector<std::wstring>> words;

    // single column of words
    lily_of_the_valley::text_preview importPreview;

    // to eol row and column parser
    lily_of_the_valley::text_column_delimited_character_parser
                                              parser(L'\t');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
                                    myColumn(parser, std::nullopt);
    lily_of_the_valley::text_row<std::wstring> myRow(std::nullopt);
    myRow.add_column(myColumn);
    myRow.allow_column_resizing(true);

    // preview the file, read it, write to the word list, and finally sort the word list
    lily_of_the_valley::text_matrix<std::wstring>
        importer(&words);
    importer.add_row_definition(myRow);

    size_t rowCount = importPreview(fileText, L'\t', true, false);
    importer.read(fileText, rowCount, 1, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(3 == words[0].size()); // 3 columns in first row
    CHECK(words[0][0] == L"First");
    CHECK(words[0][1] == L"Second");
    CHECK(words[0][2] == L"Third");
    }

TEST_CASE("Standard Delimiters", "[text import]")
    {
    const wchar_t* fileText = L"First,Second;Third Fourth";

    std::vector<std::vector<std::wstring>> words;

    // single row of words
    lily_of_the_valley::text_preview importPreview;

    // to " ,;" column parser
    lily_of_the_valley::text_column_standard_delimiter_parser parser;
    lily_of_the_valley::text_column<decltype(parser)>
        myColumn(parser, std::nullopt);
    lily_of_the_valley::text_row<std::wstring> myRow(std::nullopt);
    myRow.add_column(myColumn);
    myRow.allow_column_resizing(false);

    // preview the file, read it, write to the word list, and finally sort the word list
    lily_of_the_valley::text_matrix<std::wstring>
        importer(&words);
    importer.add_row_definition(myRow);

    size_t rowCount = importPreview(fileText, L'\t', true, false);
    importer.read(fileText, rowCount, 4, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(4 == words[0].size());
    CHECK(words[0][0] == L"First");
    CHECK(words[0][1] == L"Second");
    CHECK(words[0][2] == L"Third");
    CHECK(words[0][3] == L"Fourth");
    }

TEST_CASE("Tabbed Known Column Count", "[text import]")
    {
    const wchar_t* fileText = L"First\tSecond\tThird";

    std::vector<std::vector<std::wstring>> words;

    // single column of words
    lily_of_the_valley::text_preview importPreview;

    // to eol row and column parser
    lily_of_the_valley::text_column_delimited_character_parser
                                              parser(L'\t');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
                                    myColumn(parser, std::nullopt);
    lily_of_the_valley::text_row<std::wstring> myRow(std::nullopt);
    myRow.add_column(myColumn);
    myRow.allow_column_resizing(false);

    // preview the file, read it, write to the word list, and finally sort the word list
    lily_of_the_valley::text_matrix<std::wstring>
        importer(&words);
    importer.add_row_definition(myRow);

    size_t rowCount = importPreview(fileText, L'\t', true, false);
    importer.read(fileText, rowCount, 3, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(3 == words[0].size());
    CHECK(words[0][0] == L"First");
    CHECK(words[0][1] == L"Second");
    CHECK(words[0][2] == L"Third");

    // just read in first 2 columns
    words.clear();
    importer.read(fileText, rowCount, 2, true);

    CHECK(1 == rowCount);
    CHECK(1 == words.size()); // only 1 row
    CHECK(2 == words[0].size());
    CHECK(words[0][0] == L"First");
    CHECK(words[0][1] == L"Second");
    }