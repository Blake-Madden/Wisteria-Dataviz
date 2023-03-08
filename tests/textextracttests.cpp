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
        CHECK(ext.get_filtered_text() == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("External Buffer")
        {
        lily_of_the_valley::extract_text ext;
        wchar_t buffer[20];
        ext.set_writable_buffer(buffer,sizeof(buffer)/sizeof(wchar_t));
        // redundant call shouldn't hurt anything
        ext.set_writable_buffer(buffer,sizeof(buffer)/sizeof(wchar_t));
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"01234",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"01234") == 0);
        ext.allocate_text_buffer(10);
        // should still be using external buffer, plenty of room
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"56789",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"56789") == 0);
        // upon destruction, should NOT try to delete external buffer
        }
    SECTION("External Buffer Switch To Internal")
        {
        lily_of_the_valley::extract_text ext;
        wchar_t buffer[20];
        ext.set_writable_buffer(buffer,sizeof(buffer)/sizeof(wchar_t));
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"01234",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"01234") == 0);
        ext.allocate_text_buffer(10);
        // should still be using external buffer, plenty of room
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"56789",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"56789") == 0);
        ext.allocate_text_buffer(21);
        // not enough memory, switch to internal
        CHECK(ext.is_using_internal_buffer());
        ext.add_character(L'a');
        ext.add_character(L'b');
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab") == 0);
        ext.add_characters(L"012",3);
        ext.add_characters(L"34",2);
        CHECK(ext.get_filtered_text_length() == 7);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab01234") == 0);
        }
    SECTION("External Buffer Switch From Internal")
        {
        lily_of_the_valley::extract_text ext;
        wchar_t buffer[20];
        ext.allocate_text_buffer(21);
        // should be using internal one after allocation request
        CHECK(ext.is_using_internal_buffer());
        // now switch to smaller external buffer, delete old internal one
        ext.set_writable_buffer(buffer,sizeof(buffer)/sizeof(wchar_t));
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"01234",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"01234") == 0);
        ext.allocate_text_buffer(10);
        // should still be using external buffer, plenty of room
        CHECK(ext.is_using_internal_buffer() == false);
        ext.add_characters(L"56789",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"56789") == 0);
        ext.allocate_text_buffer(21);
        // not enough memory, switch to internal
        CHECK(ext.is_using_internal_buffer());
        ext.add_character(L'a');
        ext.add_character(L'b');
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab") == 0);
        ext.add_characters(L"012",3);
        ext.add_characters(L"34",2);
        CHECK(ext.get_filtered_text_length() == 7);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab01234") == 0);
        }
    SECTION("Add Characters")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(5);
        CHECK(ext.get_filtered_text_length() == 0);
        ext.add_characters(L"01234",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"01234") == 0);
        ext.allocate_text_buffer(10);
        ext.add_characters(L"56789",5);
        CHECK(ext.get_filtered_text_length() == 5);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"56789") == 0);
        ext.allocate_text_buffer(12);
        ext.add_character(L'a');
        ext.add_character(L'b');
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab") == 0);
        ext.add_characters(L"012",3);
        ext.add_characters(L"34",2);
        CHECK(ext.get_filtered_text_length() == 7);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"ab01234") == 0);
        }
    SECTION("Add Character Repeated")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(10);
        ext.add_character(L'a');
        ext.add_character(L'b', 3);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"abbb") == 0);
        CHECK(ext.get_filtered_text_length() == 4);
        ext.add_character(L'c', 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"abbbcc") == 0);
        CHECK(ext.get_filtered_text_length() == 6);
        }
    SECTION("Trim")
        {
        lily_of_the_valley::extract_text ext;
        ext.allocate_text_buffer(10);
        ext.add_characters(L"Hi   ",5);
        ext.trim();
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"Hi") == 0);
        // nothing to trim
        ext.trim();
        CHECK(ext.get_filtered_text_length() == 2);
        CHECK(std::wcscmp(ext.get_filtered_text(), L"Hi") == 0);
        }
    }
