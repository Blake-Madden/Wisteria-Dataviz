// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/doc_extract_text.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

// Builds a byte buffer from raw byte values (some exceed signed char's
// range, so this avoids brace-init narrowing errors from writing them
// directly into a char array/vector).
static std::vector<char> doc_bytes(std::initializer_list<int> values)
    {
    std::vector<char> result;
    result.reserve(values.size());
    for (const int value : values)
        {
        result.push_back(static_cast<char>(value));
        }
    return result;
    }

// Converts plain ASCII text into raw bytes (as if read as MBCS).
static std::vector<char> doc_text(const std::string& text)
    {
    return std::vector<char>(text.cbegin(), text.cend());
    }

// Encodes a wide string as raw little-endian UTF-16 bytes, matching how
// Unicode DOC text streams are laid out.
static std::vector<char> doc_utf16_bytes(const std::wstring& text)
    {
    std::vector<char> result;
    result.reserve(text.size() * 2);
    for (const wchar_t wch : text)
        {
        const auto codeUnit = static_cast<uint16_t>(wch);
        result.push_back(static_cast<char>(codeUnit & 0xFF));
        result.push_back(static_cast<char>((codeUnit >> 8) & 0xFF));
        }
    return result;
    }

static void doc_append(std::vector<char>& dest, const std::vector<char>& src)
    {
    dest.insert(dest.end(), src.cbegin(), src.cend());
    }

// Directly injects raw DOC text-body bytes into a synthetic "WordDocument"
// stream entry and runs it through load_stream(), bypassing the outer CFB
// container (header/BAT/directory) parsing entirely. The entry is sized at
// 4 KB so it stays out of the small-block (SBAT) addressing path, which
// would otherwise require a populated root storage entry.
static void doc_run_load_stream(word1997_extract_text& parser, const std::vector<char>& content)
    {
    constexpr size_t headerSkip{ 512 };
    constexpr size_t contentRegion{ 4096 };

    std::vector<char> fileBuffer(headerSkip + contentRegion, 0);
    std::memcpy(fileBuffer.data() + headerSkip, content.data(), content.size());

    word1997_extract_text::cfb_iostream baseStream(fileBuffer.data(), fileBuffer.size());
    word1997_extract_text::file_system_entry entry(baseStream);
    entry.m_type = word1997_extract_text::file_system_entry_type::stream_type;
    entry.m_size = contentRegion;
    entry.m_sectors = { 0, 1, 2, 3, 4, 5, 6, 7 };
    entry.open();

    parser.m_text_body_stream_length = static_cast<unsigned long>(content.size());
    parser.load_stream(&entry);
    }

// Same idea as doc_run_load_stream(), but prepends a synthetic 128-byte FIB
// header (with the given flags) and calls load_document(), which reads the
// flags and text-body boundaries from that header before decoding the body.
static void doc_run_load_document(word1997_extract_text& parser, uint16_t fibFlags,
                                   const std::vector<char>& textContent)
    {
    constexpr size_t headerSkip{ 512 };
    constexpr size_t contentRegion{ 4096 };
    constexpr size_t fibHeaderSize{ 128 };

    std::vector<char> fibHeader(fibHeaderSize, 0);
    // FIB flags word (fComplex, fEncrypted, etc.) lives at offset 10.
    fibHeader[10] = static_cast<char>(fibFlags & 0xFF);
    fibHeader[11] = static_cast<char>((fibFlags >> 8) & 0xFF);

    // Text starts right after this header (no extra skip), and ends after
    // textContent's bytes.
    const auto strStart = static_cast<uint32_t>(fibHeaderSize);
    const auto strEnd = static_cast<uint32_t>(fibHeaderSize + textContent.size());
    for (size_t i = 0; i < 4; ++i)
        {
        fibHeader[24 + i] = static_cast<char>((strStart >> (8 * i)) & 0xFF);
        fibHeader[28 + i] = static_cast<char>((strEnd >> (8 * i)) & 0xFF);
        }

    std::vector<char> content(fibHeader);
    doc_append(content, textContent);

    std::vector<char> fileBuffer(headerSkip + contentRegion, 0);
    std::memcpy(fileBuffer.data() + headerSkip, content.data(), content.size());

    word1997_extract_text::cfb_iostream baseStream(fileBuffer.data(), fileBuffer.size());
    word1997_extract_text::file_system_entry entry(baseStream);
    entry.m_type = word1997_extract_text::file_system_entry_type::stream_type;
    entry.m_size = contentRegion;
    entry.m_sectors = { 0, 1, 2, 3, 4, 5, 6, 7 };
    entry.open();

    parser.load_document(&entry);
    }

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
    SECTION("Empty Buffer")
        {
        word1997_extract_text wordParse;
        CHECK(wordParse(nullptr, 0) == nullptr);
        const char* text = "Hello";
        CHECK(wordParse(text, 0) == nullptr);
        }
    SECTION("Whitespace Only Buffer")
        {
        word1997_extract_text wordParse;
        const char* text = "   \t\n  ";
        CHECK(wordParse(text, std::strlen(text)) == nullptr);
        }
    SECTION("Unrecognized Header Throws")
        {
        word1997_extract_text wordParse;
        const char* text = "Just some plain text without any markup at all.";
        CHECK_THROWS_AS(wordParse(text, std::strlen(text)),
                        word1997_extract_text::msword_header_not_found);
        }
    SECTION("Word File Really HTML")
        {
        word1997_extract_text wordParse;
        const char* text = "<b>Hello World</b>";
        const wchar_t* p = wordParse(text, std::strlen(text));
        CHECK(std::wcscmp(p, L"Hello World") == 0);
        }
    SECTION("CFB Header Signature")
        {
        word1997_extract_text wordParse;
        const auto validMagic = doc_bytes({ 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 });
        CHECK(wordParse.starts_with_doc_header(validMagic.data(), validMagic.size()));

        const auto betaMagic = doc_bytes({ 14, 17, 0xFC, 13, 0xD0, 0xCF, 17, 14 });
        CHECK(wordParse.starts_with_doc_header(betaMagic.data(), betaMagic.size()));

        const char* garbage = "NotAWordDoc";
        CHECK_FALSE(wordParse.starts_with_doc_header(garbage, std::strlen(garbage)));

        // buffer too short to hold the full magic number
        CHECK_FALSE(wordParse.starts_with_doc_header(validMagic.data(), 4));
        }
    SECTION("Endian Helpers")
        {
        const auto littleOne = doc_bytes({ 0x01, 0x00, 0x00, 0x00 });
        CHECK(word1997_extract_text::read_uint(littleOne.data(), 0) == 1U);
        CHECK(word1997_extract_text::read_int(littleOne.data(), 0) == 1);

        const auto allOnes = doc_bytes({ 0xFF, 0xFF, 0xFF, 0xFF });
        CHECK(word1997_extract_text::read_uint(allOnes.data(), 0) == 4294967295U);
        CHECK(word1997_extract_text::read_int(allOnes.data(), 0) == -1);

        const auto shortVal = doc_bytes({ 0x34, 0x12 });
        CHECK(word1997_extract_text::read_short(shortVal.data(), 0) == 0x1234);
        }
    SECTION("Paragraph Helpers")
        {
        CHECK(word1997_extract_text::paragraph_ends_with_crlf(L"Hello\n"));
        CHECK(word1997_extract_text::paragraph_ends_with_crlf(L"Hello\r"));
        CHECK_FALSE(word1997_extract_text::paragraph_ends_with_crlf(L"Hello"));
        CHECK_FALSE(word1997_extract_text::paragraph_ends_with_crlf(L""));

        CHECK(word1997_extract_text::paragraph_begins_with(L"  HYPERLINK foo", L"HYPERLINK"));
        CHECK(word1997_extract_text::paragraph_begins_with(L"PAGEREF bar", L"PAGE"));
        CHECK_FALSE(word1997_extract_text::paragraph_begins_with(L"Hello", L"HYPERLINK"));
        CHECK_FALSE(word1997_extract_text::paragraph_begins_with(L"", L"HYPERLINK"));
        CHECK_FALSE(word1997_extract_text::paragraph_begins_with(L"Hello", L""));
        }
    SECTION("Binary Stream Detection")
        {
        const auto isBinary = [](const std::vector<char>& buffer)
        {
            return word1997_extract_text::is_buffer_binary_stream(
                reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
        };

        // plain text, no embedded nulls
        CHECK_FALSE(isBinary(doc_text("Hi there")));
        // all zeros (a fully-zeroed-out embedded object) isn't treated as binary
        CHECK_FALSE(isBinary(doc_bytes({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 })));
        // a single (non-terminator) null isn't enough to flag binary
        CHECK_FALSE(isBinary(doc_bytes({ 'A', 0x00, 'B', 'C' })));
        // a Unicode null terminator (double zero) followed by more data is binary
        CHECK(isBinary(doc_bytes({ 'A', 0x00, 0x00, 'B', 'C' })));
        }
    SECTION("Safe Math Helpers")
        {
        CHECK(word1997_extract_text::safe_modulus<size_t>(10, 3) == 1);
        CHECK(word1997_extract_text::safe_modulus<size_t>(10, 0) == 0);
        CHECK(word1997_extract_text::safe_modulus<size_t>(0, 5) == 0);

        CHECK(word1997_extract_text::safe_divide<size_t>(10, 3) == 3);
        CHECK(word1997_extract_text::safe_divide<size_t>(10, 0) == 0);
        CHECK(word1997_extract_text::safe_divide<size_t>(0, 5) == 0);
        }
    SECTION("Text Body Plain MBCS Paragraph")
        {
        word1997_extract_text wordParse;
        doc_run_load_stream(wordParse, doc_text("Hello world"));
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"Hello world") == 0);
        }
    SECTION("Text Body Plain UTF-16 Paragraph")
        {
        word1997_extract_text wordParse;
        doc_run_load_stream(wordParse, doc_utf16_bytes(L"Hi there"));
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"Hi there") == 0);
        }
    SECTION("Text Body CJK Mixed With Latin")
        {
        word1997_extract_text wordParse;
        doc_run_load_stream(wordParse, doc_utf16_bytes(L"Hello 世界"));
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"Hello 世界") == 0);
        }
    SECTION("Text Body MS-1252 Special Characters")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        doc_append(content, doc_bytes({ 0x93 })); // smart open quote
        doc_append(content, doc_text("Hello"));
        // smart close quote, space, em dash, space, ellipsis
        doc_append(content, doc_bytes({ 0x94, 0x20, 0x97, 0x20, 0x85 }));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"“Hello” — …") == 0);
        }
    SECTION("Text Body Bullet Character Mapping")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        doc_append(content, doc_bytes({ 0x20, 0x95 })); // space, bullet
        doc_append(content, doc_text("Item"));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L" •Item") == 0);
        }
    SECTION("Text Body Soft And Non-Breaking Hyphen")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        doc_append(content, doc_text("well"));
        doc_append(content, doc_bytes({ 0x1E })); // non-breaking hyphen -> '-'
        doc_append(content, doc_text("known"));
        doc_append(content, doc_bytes({ 0x20 }));
        doc_append(content, doc_text("co"));
        doc_append(content, doc_bytes({ 0x1F })); // soft hyphen -> stripped
        doc_append(content, doc_text("op"));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"well-known coop") == 0);
        }
    SECTION("Text Body Ligatures And Private-Use Characters")
        {
        word1997_extract_text wordParse;
        // 0xF001 (private-use "fi") and 0xFB02 (regular "fl" ligature)
        doc_run_load_stream(wordParse, doc_utf16_bytes(L"le ﬂow"));
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"file flow") == 0);
        }
    SECTION("Text Body Table Cell Tabs")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        // two consecutive table-cell markers convert to a tab + row-end newline
        doc_append(content, doc_bytes({ 0x20, 0x07, 0x07 }));
        doc_append(content, doc_text("X"));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L" \t\nX") == 0);
        }
    SECTION("Text Body Hyperlink Field Stripping")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        doc_append(content, doc_text("Before "));
        doc_append(content, doc_bytes({ 0x13 }));
        doc_append(content, doc_text("HYPERLINK"));
        doc_append(content, doc_bytes({ 0x14 }));
        doc_append(content, doc_text("displayed text"));
        doc_append(content, doc_bytes({ 0x15 }));
        doc_append(content, doc_text(" After"));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"Before displayed text After") == 0);
        }
    SECTION("Text Body PAGE Field Stripping")
        {
        word1997_extract_text wordParse;
        std::vector<char> content{};
        doc_append(content, doc_text("See page "));
        doc_append(content, doc_bytes({ 0x13 }));
        doc_append(content, doc_text("PAGE"));
        doc_append(content, doc_bytes({ 0x15 }));
        doc_append(content, doc_text(" of 10"));
        doc_run_load_stream(wordParse, content);
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"See page  of 10") == 0);
        }
    SECTION("Document Body Fast-Saved Flag Throws")
        {
        word1997_extract_text wordParse;
        CHECK_THROWS_AS(doc_run_load_document(wordParse, 0x0004, {}),
                        word1997_extract_text::msword_fastsaved);
        }
    SECTION("Document Body Encrypted Flag Throws")
        {
        word1997_extract_text wordParse;
        CHECK_THROWS_AS(doc_run_load_document(wordParse, 0x0100, {}),
                        word1997_extract_text::msword_encrypted);
        }
    SECTION("Document Body Decodes Text Via FIB")
        {
        word1997_extract_text wordParse;
        doc_run_load_document(wordParse, 0x0000, doc_text("Hello world"));
        CHECK(std::wcscmp(wordParse.get_filtered_text(), L"Hello world") == 0);
        }
    }

// NOLINTEND
// clang-format on
