#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Text Extract Base", "[textextract]")
    {
    SECTION("InitialState")
        {
        lily_of_the_valley::extract_text ext;
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Add Characters")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(5);
        CHECK(ext.get_filtered_text_length() == 0);
        ext.add_characters({ L"01234", 5 });
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"01234") == 0);
        ext.allocate_text_buffer(10);
        ext.add_characters({ L"56789", 5 });
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"56789") == 0);
        ext.allocate_text_buffer(12);
        ext.add_character(L'a');
        ext.add_character(L'b');
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab") == 0);
        ext.add_characters({ L"012", 3 });
        ext.add_characters({ L"34", 2 });
        CHECK(ext.get_filtered_text_length() == 7);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab01234") == 0);
        }
    SECTION("Add Character Repeated")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(10);
        ext.add_character(L'a');
        ext.fill_with_character(3, L'b');
        CHECK(std::wcscmp(ext.get_filtered_text(), L"abbb") == 0);
        CHECK(ext.get_filtered_text_length() == 4);
        ext.fill_with_character(2, L'c');
        CHECK(std::wcscmp(ext.get_filtered_text(), L"abbbcc") == 0);
        CHECK(ext.get_filtered_text_length() == 6);
        }
    SECTION("Trim")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(10);
        ext.add_characters({ L"Hi   ", 5 });
        ext.trim();
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"Hi") == 0);
        // nothing to trim
        ext.trim();
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"Hi") == 0);
        }
    }
