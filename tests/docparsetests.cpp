// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/doc_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("DOC Parser", "[doc import]")
    {
    SECTION("Word File Really RTF")
        {
        word1997_extract_text wordParse;
        const char* rtf = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}{\\f1\\froman\\fprq2\\fcharset0 Batang;}}{\\colortbl ;\\red192\\green192\\blue192;\\red128\\green128\\blue0;\\red0\\green0\\blue128;}{\\*\\generator Msftedit 5.41.15.1515;}\\viewkind4\\uc1\\pard\\f0\\fs20 H\\b e\\ul\\i r\\ulnone\\b0\\i0 e is s\\cf1\\ul\\b om\\cf0\\ulnone\\b0 e t\\i\\f1\\fs56 ex\\i0\\f0\\fs20 t t\\i\\fs48 h\\i0\\fs20 at is \\cf2 for\\cf3\\ul\\i ma\\cf2\\ulnone\\i0 tted\\cf0 .\\par}";
        const wchar_t* p = wordParse(rtf, std::strlen(rtf));
        CHECK(std::wcscmp(p, L"\nHere is some text that is formatted.\n") == 0);
        CHECK(wordParse.get_filtered_text_length() == 38);
        }
    SECTION("Ole2 Stream Seek End")
        {
        const char* buffer = "Here is some text to stream.";
        const size_t buffSize = std::strlen(buffer);
        word1997_extract_text::cfb_iostream stream(buffer, buffSize);
        CHECK(stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == buffSize);
        // go back to start
        stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg);
        CHECK(stream.seek(-5, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == buffSize-5);
        // go back to start
        stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg);
        CHECK(stream.seek(99999, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == buffSize);
        // Backwards

        // go back to start
        stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg);
        // should move backwards to the start
        CHECK(stream.seek(-99999, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == 0);
        // go back to start
        stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg);
        // should move backwards to the start
        CHECK(stream.seek(-28, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == 0);
        // go back to start
        stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg);
        // should move backwards to the start
        CHECK(stream.seek(-29, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_end) == 0);
        }
    SECTION("Ole2 Stream Seek Set")
        {
        const char* buffer = "Here is some text to stream.";
        const size_t buffSize = std::strlen(buffer);
        lily_of_the_valley::word1997_extract_text::cfb_iostream stream(buffer, buffSize);
        CHECK(stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg) == 0);
        CHECK(stream.seek(10, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg) == 10);
        // negative positions should move you to the start
        CHECK(stream.seek(-10, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg) == 0);
        // going too far will move you to the end
        CHECK(stream.seek(9999, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_beg) == buffSize);
        }
    SECTION("Ole2 Stream Seek Cur")
        {
        const char* buffer = "Here is some text to stream.";
        const size_t buffSize = std::strlen(buffer);
        lily_of_the_valley::word1997_extract_text::cfb_iostream stream(buffer, buffSize);
        CHECK(stream.seek(0, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_cur) == 0);
        CHECK(stream.seek(10, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_cur) == 10);
        CHECK(stream.seek(-5, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_cur) == 5);
        // negative positions should move you to the start
        CHECK(stream.seek(-6, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_cur) == 0);
        // going too far will move you to the end
        CHECK(stream.seek(9999, word1997_extract_text::cfb_iostream::cfb_stream_seek_type::seek_cur) == buffSize);
        }
    }

// NOLINTEND
// clang-format on
