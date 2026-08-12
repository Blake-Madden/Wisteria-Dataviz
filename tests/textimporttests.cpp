// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/import/text_row.h"
#include "../src/import/text_column.h"
#include "../src/import/text_matrix.h"
#include "../src/import/text_preview.h"
#include "../src/import/text_functional.h"

using namespace Catch::Matchers;

TEST_CASE("Cell trim", "[text import]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::cell_trim trim;
        CHECK(trim(nullptr, 5) == nullptr);
        }
    SECTION("Nothing")
        {
        const wchar_t* myString = L"Hello";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(std::wcscmp(start,L"Hello") == 0);
        CHECK(trim.get_trimmed_string_length() == 5);
        }
    SECTION("Nothing2")
        {
        const wchar_t* myString = L"H";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(std::wcscmp(start,L"H") == 0);
        CHECK(trim.get_trimmed_string_length() == 1);
        }
    SECTION("Nothing3")
        {
        const wchar_t* myString = L"";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString);
        CHECK(trim.get_trimmed_string_length() == 0);
        CHECK(start == myString);
        }
    SECTION("Trim Left")
        {
        const wchar_t* myString = L" \t \n\r\tHello";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(std::wcscmp(start,L"Hello") == 0);
        CHECK(trim.get_trimmed_string_length() == 5);
        }
    SECTION("Trim Right")
        {
        const wchar_t* myString = L"Hello \t \n\r\t";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(std::wcsncmp(start,L"Hello", 5) == 0);
        CHECK(trim.get_trimmed_string_length() == 5);
        }
    SECTION("Trim Both")
        {
        const wchar_t* myString = L"      \nHello \t \n\r\t";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(std::wcsncmp(start,L"Hello", 5) == 0);
        CHECK(trim.get_trimmed_string_length() == 5);
        }
    SECTION("Trim Both No Known Length")
        {
        const wchar_t* myString = L"      \nHello \t \n\r\t";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString);
        CHECK(std::wcsncmp(start,L"Hello", 5) == 0);
        CHECK(trim.get_trimmed_string_length() == 5);
        }
    SECTION("Trim All Spaces")
        {
        const wchar_t* myString = L"    \t";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, 5);
        CHECK(trim.get_trimmed_string_length() == 0);
        CHECK(start == myString+5);
        }
    SECTION("Trim All Spaces Followed By Text")
        {
        const wchar_t* myString = L"   some text";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, 3);
        CHECK(std::wcscmp(start,L"some text") == 0);
        CHECK(trim.get_trimmed_string_length() == 0);
        }
    SECTION("Trim Some Spaces Followed By Text")
        {
        const wchar_t* myString = L" some text";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, 3);
        CHECK(std::wcsncmp(start,L"so", 2) == 0);
        CHECK(trim.get_trimmed_string_length() == 2);
        }
    SECTION("Quoted Both Sides")
        {
        const wchar_t* myString = L"\"Hello\"";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(trim.get_trimmed_string_length() == 5);
        CHECK(std::wstring{start, trim.get_trimmed_string_length()} ==
            std::wstring{L"Hello"});
        }
    SECTION("Trailing Quote Without Leading Quote Not Stripped")
        {
        // a literal quote at the end of an unquoted cell isn't a CSV quote pair
        // and should be left alone
        const wchar_t* myString = L"Short for \"constructor.\"";
        lily_of_the_valley::cell_trim trim;
        const wchar_t* start = trim(myString, std::wcslen(myString));
        CHECK(trim.get_trimmed_string_length() == std::wcslen(myString));
        CHECK(std::wstring{start, trim.get_trimmed_string_length()} ==
              std::wstring{L"Short for \"constructor.\""});
        }
    }

TEST_CASE("Cell collapse quotes", "[text import]")
    {
    SECTION("No Quotes")
        {
        std::wstring value{ L"Hello" };
        lily_of_the_valley::cell_collapse_quotes<std::wstring> collapse;
        collapse(value);
        CHECK(value == std::wstring{ L"Hello" });
        }
    SECTION("Doubled Quote Collapses To One")
        {
        std::wstring value{ L"Say \"\"hello\"\" to them" };
        lily_of_the_valley::cell_collapse_quotes<std::wstring> collapse;
        collapse(value);
        CHECK(value == std::wstring{ L"Say \"hello\" to them" });
        }
    SECTION("Single Quote Left Alone")
        {
        // a lone (non-doubled) quote isn't an escape sequence, so it's untouched
        std::wstring value{ L"Short for \"constructor.\"" };
        lily_of_the_valley::cell_collapse_quotes<std::wstring> collapse;
        collapse(value);
        CHECK(value == std::wstring{ L"Short for \"constructor.\"" });
        }
    }

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

TEST_CASE("Tabbed Column With Embedded Quotes", "[text import]")
    {
    const wchar_t* fileText =
        L"CTOR\tConstructor\tA special method called when an object is created; "
        L"initializes the object's state. Short for \"constructor.\"\n"
        L"DTOR\tDestructor\tA special method called when an object is destroyed; "
        L"cleans up resources. Short for \"destructor.\"";

    std::vector<std::vector<std::wstring>> words;

    lily_of_the_valley::text_preview importPreview;

    lily_of_the_valley::text_column_delimited_character_parser parser(L'\t');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
        myColumn(parser, std::nullopt);
    lily_of_the_valley::text_row<std::wstring> myRow(std::nullopt);
    myRow.add_column(myColumn);
    myRow.allow_column_resizing(true);

    lily_of_the_valley::text_matrix<std::wstring>
        importer(&words);
    importer.add_row_definition(myRow);

    size_t rowCount = importPreview(fileText, L'\t', true, false);
    importer.read(fileText, rowCount, 3, true);

    CHECK(2 == rowCount);
    CHECK(2 == words.size());
    CHECK(3 == words[0].size());
    CHECK(words[0][0] == std::wstring{ L"CTOR" });
    CHECK(words[0][1] == std::wstring{ L"Constructor" });
    CHECK(words[0][2] ==
          std::wstring{ L"A special method called when an object is created; "
                        L"initializes the object's state. Short for \"constructor.\"" });
    CHECK(3 == words[1].size());
    CHECK(words[1][0] == std::wstring{ L"DTOR" });
    CHECK(words[1][1] == std::wstring{ L"Destructor" });
    CHECK(words[1][2] ==
          std::wstring{ L"A special method called when an object is destroyed; "
                        L"cleans up resources. Short for \"destructor.\"" });
    }

// NOLINTEND
// clang-format on
