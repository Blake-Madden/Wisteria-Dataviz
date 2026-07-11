// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include "../src/import/pdf_extract_text.h"

using namespace lily_of_the_valley;

TEST_CASE("PDF Import", "[pdf import]")
    {
    SECTION("Nulls")
        {
        pdf_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext("some text", 0) == nullptr);
        }
    SECTION("Missing Header")
        {
        pdf_extract_text ext;
        CHECK_THROWS_AS(ext("some text", 9), pdf_extract_text::pdf_header_not_found);
        }
    SECTION("Encrypted")
        {
        const char* text = R"PDF(%PDF-1.4
trailer
<< /Encrypt 5 0 R /Root 1 0 R >>)PDF";
        pdf_extract_text ext;
        CHECK_THROWS_AS(ext(text, std::strlen(text)), pdf_extract_text::pdf_encrypted);
        }
    SECTION("No Pages")
        {
        const char* text = "%PDF-1.4\nnothing else in here";
        pdf_extract_text ext;
        CHECK(ext(text, std::strlen(text)) != nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(!ext.get_log().empty());
        }
    SECTION("Simple")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Hello, world!) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hello, world!") == 0);
        }
    SECTION("Nested Parentheses")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Strings may contain (nested) parentheses.) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)),
                          L"Strings may contain (nested) parentheses.") == 0);
        }
    SECTION("Escape Commands")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Tab\there\nand \(parens\) \\ \101) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Tab\there\nand (parens) \\ A") == 0);
        }
    SECTION("Escaped New Lines")
        {
        const char* text = "%PDF-1.4\n"
            "1 0 obj\n<< /Type /Page /Contents 2 0 R >>\nendobj\n"
            "2 0 obj\n<< >>\nstream\n"
            "BT (These \\\ntwo strings \\\nare the same.) Tj ET\n"
            "endstream\nendobj";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"These two strings are the same.") == 0);
        }
    SECTION("Hex String")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT <48656C6C6F> Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hello") == 0);
        }
    SECTION("Td New Line")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT
(First line) Tj
0 -14 Td
(Second line) Tj
ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"First line\nSecond line") == 0);
        }
    SECTION("Td Paragraph Break")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (One) Tj 0 -40 Td (Two) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"One\n\nTwo") == 0);
        }
    SECTION("Tm New Line")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT
1 0 0 1 72 700 Tm (Line A) Tj
1 0 0 1 72 686 Tm (Line B) Tj
ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Line A\nLine B") == 0);
        }
    SECTION("Tm Separate Text Objects On Same Line")
        {
        // Each label is its own independent BT/Tm/Tj/ET block (one DrawText() call
        // per word), rather than a single Tj/TJ run, e.g. "OVER ADD DIFF" captions
        // below three squares in a demo image. Without X-tracking on Tm, these
        // silently ran together as "OVERADDDIFF".
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 1 0 0 1 72 400 Tm (OVER) Tj ET
BT 1 0 0 1 210 400 Tm (ADD) Tj ET
BT 1 0 0 1 340 400 Tm (DIFF) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"OVER ADD DIFF") == 0);
        }
    SECTION("Tm Diagonal Gap (Rotated Text Runs)")
        {
        // Two independently-drawn rotated text labels (each its own DrawText()
        // call) land on the same nominal baseline but far apart, e.g. two
        // "Rotated text" labels in a demo image, previously concatenated as
        // "Rotated textRotated text".
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 0.978 -0.208 0.208 0.978 320 720 Tm (Rotated text) Tj ET
BT 0.978 0.208 -0.208 0.978 460 720 Tm (Rotated text) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Rotated text Rotated text") == 0);
        }
    SECTION("Tm Small Gap Not Treated As Word Break")
        {
        // A small position nudge between two Tm blocks (e.g., precise kerning
        // between two halves of one word) should not be treated as a word gap.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 1 0 0 1 72 400 Tm (Wo) Tj ET
BT 1 0 0 1 80 400 Tm (rd) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Word") == 0);
        }
    SECTION("Rotated Tm: Td Move Not Rotated")
        {
        // Tm establishes a 90-degree-rotated text line matrix (local +x maps to
        // page -y, local +y maps to page +x). The following Td's operands are in
        // that rotated text space, so a purely-horizontal local step (-14, 0)
        // lands one line further down the page (a page-space y move), the same
        // kind of step as the "Tm New Line" case above. handle_relative_move
        // transforms the Td operands through the text matrix before treating
        // them as a page-space delta, so this is recognized as a line break
        // rather than glued together as a small, sub-threshold horizontal nudge.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT
0 1 -1 0 400 300 Tm (First) Tj
-14 0 Td (Second) Tj
ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"First\nSecond") == 0);
        }
    SECTION("Vertical Writing Mode Column Break (Predefined Unicode CMap)")
        {
        // A Type0 font with a vertical predefined Unicode encoding
        // (/UniJIS-UCS2-V, the vertical counterpart of /UniJIS-UCS2-H) lays out
        // columns along the page's x-axis: a same-row move to a new column is
        // the vertical-mode equivalent of a new line, not a same-line word gap.
        // handle_absolute_move checks the line-step axis against the current
        // font's writing mode, so this is recognized as two columns separated
        // by a newline rather than two runs on the same line separated by a space.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf 1 0 0 1 400 700 Tm <0043006F006C00200041> Tj ET
BT /F1 12 Tf 1 0 0 1 386 700 Tm <0043006F006C00200042> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /UniJIS-UCS2-V >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Col A\nCol B") == 0);
        }
    SECTION("Vertical Writing Mode Same-Column Continuation (Predefined Unicode CMap)")
        {
        // The mirror image of the column-break case: two runs of the same
        // vertical-mode font stepping down the page (a y move) by less than a
        // line height are continuing down the same column, not starting a new
        // line. Since the line-step axis for a vertical-mode font is x, not y,
        // this small y move is recognized as a continuation and joined into one
        // word rather than split across two lines.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf 1 0 0 1 72 700 Tm <0057006F> Tj ET
BT /F1 12 Tf 1 0 0 1 72 692 Tm <00720064> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /UniJIS-UCS2-V >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Word") == 0);
        }
    SECTION("T-star New Line")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Line A) Tj T* (Line B) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Line A\nLine B") == 0);
        }
    SECTION("TJ Kerning And Word Gaps")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT [(Hel) -50 (lo) -300 (world)] TJ ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hello world") == 0);
        }
    SECTION("TJ Word Gap Widened By Tz")
        {
        // a -100 adjustment alone wouldn't cross the -150 word-gap threshold, but
        // doubling the horizontal scale doubles its displayed width to -200, so it should
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 200 Tz [(Hel) -100 (lo)] TJ ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hel lo") == 0);
        }
    SECTION("TJ Word Gap Narrowed By Tz")
        {
        // a -300 adjustment alone would cross the -150 word-gap threshold, but halving
        // the horizontal scale halves its displayed width to -150, so it shouldn't
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 50 Tz [(Hel) -300 (lo)] TJ ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hello") == 0);
        }
    SECTION("Bullet List Items")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (\225 First item) Tj 0 -14 Td (\225 Second item) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)),
                          L"\t• First item\n\t• Second item") == 0);
        }
    SECTION("Bullet Glyph And Label In Separate Text Objects")
        {
        // mirrors how some PDF generators draw a checkbox/bullet glyph and its label using
        // two separate BT...ET text objects on the same line, each with its own Td that lands
        // on the same absolute y position (since BT resets the text line matrix to identity)
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 0 700 Td (\225) Tj ET
BT 20 700 Td (Create a new folder.) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\t• Create a new folder.") == 0);
        }
    SECTION("Multiple Pages In Tree Order")
        {
        // the page tree's Kids order (4, then 3) is document order,
        // even though object 3 appears first in the file
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Catalog /Pages 2 0 R >>
endobj
2 0 obj
<< /Type /Pages /Kids [4 0 R 3 0 R] /Count 2 >>
endobj
3 0 obj
<< /Type /Page /Parent 2 0 R /Contents 6 0 R >>
endobj
4 0 obj
<< /Type /Page /Parent 2 0 R /Contents 5 0 R >>
endobj
5 0 obj
<< >>
stream
BT (First page) Tj ET
endstream
endobj
6 0 obj
<< >>
stream
BT (Second page) Tj ET
endstream
endobj
trailer
<< /Root 1 0 R >>)PDF";
        pdf_extract_text ext;
        // each page ends on its own line and pages are separated by a blank line,
        // so that trailing content (e.g., a page footer) never runs directly into
        // the next page's text
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"First page\n\nSecond page") == 0);
        }
    SECTION("Freed Object Excluded From Fallback Page Scan")
        {
        // object 1 was deleted in an incremental update: its body is still
        // physically present in the file (incremental updates never rewrite
        // earlier bytes), but the xref table marks it "f" (free). With no
        // /Root to walk a page tree from, the fallback scan (which just
        // looks for /Type /Page objects) should honor that and skip it.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Deleted page text) Tj ET
endstream
endobj
xref
0 3
0000000000 65535 f
0000000009 00000 f
0000000058 00000 f
trailer
<< /Size 3 >>)PDF";
        pdf_extract_text ext;
        CHECK(ext(text, std::strlen(text)) != nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(!ext.get_log().empty());
        }
    SECTION("Contents Array")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents [2 0 R 3 0 R] >>
endobj
2 0 obj
<< >>
stream
BT (Part one) Tj ET
endstream
endobj
3 0 obj
<< >>
stream
BT 0 -14 Td (part two) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Part one\npart two") == 0);
        }
    SECTION("Metadata")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Body) Tj ET
endstream
endobj
7 0 obj
<< /Title (My Title) /Author (Jane Doe) /Subject (Testing) /Keywords (pdf, tests) >>
endobj
trailer
<< /Info 7 0 R >>)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Body") == 0);
        CHECK(ext.get_title() == L"My Title");
        CHECK(ext.get_author() == L"Jane Doe");
        CHECK(ext.get_subject() == L"Testing");
        CHECK(ext.get_keywords() == L"pdf, tests");
        }
    SECTION("Metadata UTF-16")
        {
        // a title stored as UTF-16BE (with a byte order mark)
        const char* text = R"PDF(%PDF-1.4
7 0 obj
<< /Title (\376\377\000T\000e\000s\000t) >>
endobj
trailer
<< /Info 7 0 R >>)PDF";
        pdf_extract_text ext;
        ext(text, std::strlen(text));
        CHECK(ext.get_title() == L"Test");
        }
    SECTION("Bare Carriage Return In Literal String")
        {
        // a raw (unescaped) CR byte inside a literal string should be normalized to '\n',
        // same as PDF viewers do
        const std::string text = std::string(R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT (Body) Tj ET
endstream
endobj
7 0 obj
<< /Title (Line1)PDF") +
            "\r" + R"PDF(Line2) >>
endobj
trailer
<< /Info 7 0 R >>)PDF";
        pdf_extract_text ext;
        ext(text.c_str(), text.length());
        CHECK(ext.get_title() == L"Line1\nLine2");
        }
    SECTION("ToUnicode CMap")
        {
        // a composite (2-byte code) font with a ToUnicode CMap
        // (bfchar entries for H and e; a bfrange covering l-o)
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <00480065006C006C006F> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
2 beginbfchar
<0048> <0048>
<0065> <0065>
endbfchar
1 beginbfrange
<006C> <006F> <006C>
endbfrange
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hello") == 0);
        }
    SECTION("ToUnicode Ligature Expansion")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <00010065006C0064> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
3 beginbfchar
<0001> <FB01>
<0065> <0065>
<006C> <006C>
endbfchar
1 beginbfrange
<0064> <0064> <0064>
endbfrange
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"field") == 0);
        }
    SECTION("Symbol Font Without Encoding Decodes Through Symbol Table")
        {
        // A Symbol-named font with no /Encoding and no /ToUnicode is identified by
        // BaseFont as a symbol font, but a symbol font's codes are its own private
        // encoding, not Latin text. Falling back to WinAnsi turns byte 0x61/0x62
        // ('a'/'b') into literal "ab" instead of the Greek letters (alpha, beta)
        // that Adobe's Symbol encoding assigns to those codes.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf (ab) Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1 /BaseFont /Symbol >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\x03B1\x03B2") == 0);
        }
    SECTION("ZapfDingbats Font Without Encoding Decodes Through Dingbats Table")
        {
        // Same issue as the Symbol font case above, but for ZapfDingbats: byte 0x21
        // ('!') is Adobe's "a1" dingbat glyph (U+2701, upper blade scissors), not the
        // literal exclamation point that the WinAnsi fallback produces.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf (!) Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1 /BaseFont /ZapfDingbats >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\x2701") == 0);
        }
    SECTION("Symbol Font Bullet Byte")
        {
        // Byte 0xB7 is Adobe SymbolEncoding's "bullet" glyph (U+2022), the same
        // code point WinAnsi assigns to a completely different byte (0x95). A Symbol
        // font's 0xB7 must resolve through the Symbol table, not be left to whatever
        // WinAnsi happens to say about that byte. The decoded bullet lands at the
        // start of a line, so the usual list-item formatting (a leading tab) applies,
        // same as the WinAnsi bullet tests above.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf (\267) Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1 /BaseFont /Symbol >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\t\x2022") == 0);
        }
    SECTION("Embedded TrueType Cmap Fallback")
        {
        // A simple TrueType font with no /ToUnicode and no /Differences should fall
        // back to its embedded /FontFile2 cmap table: codes 0x01/0x02 map to glyphs
        // 10/11 via a (1, 0) format 0 subtable, and those glyphs map to 'H'/'i' via a
        // (3, 1) format 4 subtable, so chaining the two should recover "Hi".
        auto appendU16 = [](std::string& buf, const uint16_t value)
            {
            buf += static_cast<char>((value >> 8) & 0xFF);
            buf += static_cast<char>(value & 0xFF);
            };
        auto appendU32 = [](std::string& buf, const uint32_t value)
            {
            buf += static_cast<char>((value >> 24) & 0xFF);
            buf += static_cast<char>((value >> 16) & 0xFF);
            buf += static_cast<char>((value >> 8) & 0xFF);
            buf += static_cast<char>(value & 0xFF);
            };

        // format 0 subtable: code -> glyph index (a flat 256-byte array)
        std::string format0;
        appendU16(format0, 0);   // format
        appendU16(format0, 262); // length (6-byte header + 256 glyph IDs)
        appendU16(format0, 0);   // language
        std::string glyphIds(256, '\0');
        glyphIds[1] = static_cast<char>(10);
        glyphIds[2] = static_cast<char>(11);
        format0 += glyphIds;

        // format 4 subtable: Unicode -> glyph index (2 real segments + terminator)
        std::string format4;
        appendU16(format4, 4);      // format
        appendU16(format4, 40);    // length
        appendU16(format4, 0);      // language
        appendU16(format4, 6);      // segCountX2 (3 segments)
        appendU16(format4, 4);      // searchRange
        appendU16(format4, 1);      // entrySelector
        appendU16(format4, 2);      // rangeShift
        appendU16(format4, 0x0048); // endCode[0] ('H')
        appendU16(format4, 0x0069); // endCode[1] ('i')
        appendU16(format4, 0xFFFF); // endCode[2] (terminator)
        appendU16(format4, 0);      // reservedPad
        appendU16(format4, 0x0048); // startCode[0]
        appendU16(format4, 0x0069); // startCode[1]
        appendU16(format4, 0xFFFF); // startCode[2]
        appendU16(format4, static_cast<uint16_t>(10 - 0x0048)); // idDelta[0]: 'H' -> glyph 10
        appendU16(format4, static_cast<uint16_t>(11 - 0x0069)); // idDelta[1]: 'i' -> glyph 11
        appendU16(format4, 1);      // idDelta[2] (terminator; maps to glyph 0)
        appendU16(format4, 0);      // idRangeOffset[0]
        appendU16(format4, 0);      // idRangeOffset[1]
        appendU16(format4, 0);      // idRangeOffset[2]

        // cmap table: header, 2 encoding records, then the two subtables
        std::string cmapTable;
        appendU16(cmapTable, 0); // version
        appendU16(cmapTable, 2); // numTables
        appendU16(cmapTable, 1);
        appendU16(cmapTable, 0);
        appendU32(cmapTable, 20); // (1, 0) Mac Roman -> format0 subtable, right after the records
        appendU16(cmapTable, 3);
        appendU16(cmapTable, 1);
        appendU32(cmapTable, static_cast<uint32_t>(20 + format0.length())); // (3, 1) Windows Unicode
        cmapTable += format0;
        cmapTable += format4;

        // sfnt wrapper: header plus a single "cmap" table record
        std::string font;
        appendU32(font, 0x00010000); // sfntVersion
        appendU16(font, 1);          // numTables
        appendU16(font, 0);
        appendU16(font, 0);
        appendU16(font, 0); // searchRange/entrySelector/rangeShift (unused)
        font += "cmap";
        appendU32(font, 0);                                  // checksum (unused)
        appendU32(font, 12 + 16);                             // offset, right after this record
        appendU32(font, static_cast<uint32_t>(cmapTable.length())); // length
        font += cmapTable;

        std::string text{
            "%PDF-1.4\n"
            "1 0 obj\n<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>\n"
            "endobj\n"
            "2 0 obj\n<< >>\nstream\nBT /F1 12 Tf <0102> Tj ET\nendstream\nendobj\n"
            "3 0 obj\n<< /Type /Font /Subtype /TrueType /BaseFont /Fallback "
            "/FontDescriptor 4 0 R >>\nendobj\n"
            "4 0 obj\n<< /Type /FontDescriptor /FontFile2 5 0 R >>\nendobj\n"
        };
        text += "5 0 obj\n<< /Length " + std::to_string(font.length()) + " >>\nstream\n";
        text += font;
        text += "\nendstream\nendobj";

        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text.data(), text.length()), L"Hi") == 0);
        }
    SECTION("ToUnicode CMap With Mixed-Width Codespace Ranges")
        {
        // a CMap that declares both a 1-byte codespace range (<00>-<80>) and a 2-byte
        // one (<8100>-<FFFF>); the content stream mixes a 1-byte code (0x41) with a
        // 2-byte code (0x8100) in the same string
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <418100> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
2 begincodespacerange
<00> <80>
<8100> <FFFF>
endcodespacerange
2 beginbfchar
<41> <0041>
<8100> <4E2D>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        const std::wstring expected{ L'A', static_cast<wchar_t>(0x4E2D) };
        CHECK(std::wcscmp(ext(text, std::strlen(text)), expected.c_str()) == 0);
        }
    SECTION("MacRoman Encoding")
        {
        // a simple font with no ToUnicode CMap, using /MacRomanEncoding; byte 0x80 is
        // "Ä" (U+00C4) there, versus "€" (U+20AC) under the default WinAnsi/CP1252
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <80> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1 /Encoding /MacRomanEncoding >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Ä") == 0);
        }
    SECTION("Standard Encoding (via BaseEncoding)")
        {
        // a simple font with no ToUnicode CMap, using an /Encoding dictionary whose
        // /BaseEncoding is /StandardEncoding; byte 0xA8 is "¤" there, versus
        // "¨" under the default WinAnsi/CP1252 (where the upper range is Latin-1)
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <A8> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1 /Encoding << /BaseEncoding /StandardEncoding >> >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"¤") == 0);
        }
    SECTION("Differences Array With Glyph Name Table")
        {
        // a simple font with a /Differences array remapping code 65 ('A') to the
        // Euro glyph and code 66 ('B') to a (multi-codepoint) Hebrew combining
        // sequence; both are resolved via a loaded glyph name table, overriding
        // what the base WinAnsi encoding would otherwise give those codes
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <4142> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1
   /Encoding << /Differences [ 65 /Euro 66 /dalethatafpatah ] >> >>
endobj)PDF";
        pdf_extract_text ext;
        ext.load_glyph_name_table(L"# comment line\n"
                                  L"\n"
                                  L"Euro;20AC\n"
                                  L"# another comment\n"
                                  L"dalethatafpatah;05D3 05B2\n");
        const std::wstring expected{ static_cast<wchar_t>(0x20AC), static_cast<wchar_t>(0x05D3),
                                     static_cast<wchar_t>(0x05B2) };
        CHECK(std::wcscmp(ext(text, std::strlen(text)), expected.c_str()) == 0);
        }
    SECTION("Differences Array Without Glyph Name Table Loaded")
        {
        // the same /Differences array as above, but without ever calling
        // load_glyph_name_table(): the codes should fall back to the base
        // (WinAnsi) encoding, i.e., plain "AB"
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <4142> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type1
   /Encoding << /Differences [ 65 /Euro 66 /dalethatafpatah ] >> >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"AB") == 0);
        }
    SECTION("Raw Surrogate Pair Without ToUnicode")
        {
        // a Type0/Identity-H font with no ToUnicode CMap, whose content stream
        // encodes an emoji directly as CIDs equal to its UTF-16 surrogate pair
        // (U+1F600 GRINNING FACE = D83D DE00)
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <D83DDE00> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H >>
endobj)PDF";
        pdf_extract_text ext;
        const std::wstring result{ ext(text, std::strlen(text)) };
        std::wstring expected;
        if constexpr (sizeof(wchar_t) > 2)
            {
            expected += static_cast<wchar_t>(0x1F600);
            }
        else
            {
            expected += static_cast<wchar_t>(0xD83D);
            expected += static_cast<wchar_t>(0xDE00);
            }
        CHECK(result == expected);
        }
    SECTION("CID-to-Unicode Table Resolves Non-Unicode CID Ordering")
        {
        // A Type0/CIDFontType2 font with no ToUnicode CMap, whose CIDs are only
        // meaningful relative to its CIDSystemInfo ordering (e.g., a real
        // Adobe-Japan1 font). Loading an external CID-to-Unicode table for that
        // ordering (in the same cidrange/cidchar format as Adobe's CMap resource
        // files) should recover its text.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <00010002> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /DescendantFonts [5 0 R] >>
endobj
5 0 obj
<< /Type /Font /Subtype /CIDFontType2
   /CIDSystemInfo << /Registry (Adobe) /Ordering (TestOrdering) /Supplement 1 >> >>
endobj)PDF";
        pdf_extract_text ext;
        ext.load_cid_to_unicode_table("Adobe-TestOrdering",
                                      L"1 begincidrange\n<0041> <0042> 1\nendcidrange\n");
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"AB") == 0);
        }
    SECTION("CID-to-Unicode Table Ignored When ToUnicode Present")
        {
        // when a font has its own ToUnicode CMap, that takes priority over any
        // loaded CID-to-Unicode table, even for a matching ordering
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <0001> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /DescendantFonts [5 0 R]
   /ToUnicode 6 0 R >>
endobj
5 0 obj
<< /Type /Font /Subtype /CIDFontType2
   /CIDSystemInfo << /Registry (Adobe) /Ordering (TestOrdering) /Supplement 1 >> >>
endobj
6 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
1 beginbfchar
<0001> <005A>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        ext.load_cid_to_unicode_table("Adobe-TestOrdering",
                                      L"1 begincidrange\n<0041> <0041> 1\nendcidrange\n");
        // the ToUnicode CMap maps CID 1 to 'Z' (0x005A); the CID table (which would
        // have mapped it to 'A') should be ignored
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Z") == 0);
        }
    SECTION("Predefined CJK CMap Encoding With Charset Converter")
        {
        // a Type0 font using one of Adobe's predefined legacy CJK CMaps as its
        // /Encoding (no ToUnicode CMap): the string bytes are Big5 text and should
        // be handed, whole, to the connected charset converter along with the
        // charset that /ETenms-B5-H implies
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <A4A4A4E5> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /ETenms-B5-H >>
endobj)PDF";
        pdf_extract_text ext;
        // a stand-in "converter" that verifies what it was handed and returns
        // the "converted" text (the real bytes are Big5 for U+4E2D U+6587)
        ext.set_charset_converter(
            [](std::string_view sourceBytes, std::string_view charsetName) -> std::wstring
            {
                CHECK(sourceBytes == "\xA4\xA4\xA4\xE5");
                CHECK(charsetName == "CP950");
                return L"Converted content";
            });
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Converted content") == 0);
        }
    SECTION("CID-to-Unicode Table Ignored For Legacy Charset Encoding")
        {
        // A Type0 font using one of Adobe's predefined legacy CJK CMaps as its
        // /Encoding (e.g., /ETenms-B5-H) has string bytes that are charset text
        // (Big5 here), not raw CIDs, even when its DescendantFont's CIDSystemInfo
        // ordering matches a loaded CID-to-Unicode table. The charset converter
        // must still be used; treating the Big5 byte pairs as CIDs and looking
        // them up in the table would produce the wrong characters.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <A4A4A4E5> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /ETenms-B5-H /DescendantFonts [5 0 R] >>
endobj
5 0 obj
<< /Type /Font /Subtype /CIDFontType0
   /CIDSystemInfo << /Registry (Adobe) /Ordering (TestOrdering) /Supplement 1 >> >>
endobj)PDF";
        pdf_extract_text ext;
        // if wrongly consulted, this table would map the byte pairs 0xA4A4 and
        // 0xA4E5 (read as CIDs) to 'X' and 'Y'
        ext.load_cid_to_unicode_table("Adobe-TestOrdering",
                                      L"2 begincidchar\n<0058> 42148\n<0059> 42213\n"
                                      L"endcidchar\n");
        ext.set_charset_converter(
            [](std::string_view sourceBytes, std::string_view charsetName) -> std::wstring
            {
                CHECK(sourceBytes == "\xA4\xA4\xA4\xE5");
                CHECK(charsetName == "CP950");
                return L"Converted content";
            });
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Converted content") == 0);
        }
    SECTION("Predefined CJK CMap Encoding Without Charset Converter")
        {
        // the same Big5-encoded document, but with no charset converter connected:
        // the text can't be decoded, so it should be skipped (not emitted as
        // garbage codes) and the skip should be logged
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <A4A4A4E5> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /ETenms-B5-H >>
endobj)PDF";
        pdf_extract_text ext;
        CHECK(ext(text, std::strlen(text)) != nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext.get_log().find(L"charset converter") != std::wstring::npos);
        }
    SECTION("Predefined Unicode CMap Encoding")
        {
        // a Type0 font using one of Adobe's predefined Unicode CMaps as its
        // /Encoding (no ToUnicode CMap): the string bytes are UTF-16BE and are
        // decoded directly, with no charset converter needed
        // (D55C AE00 is U+D55C U+AE00, Korean "hangul")
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <D55CAE00> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /UniKS-UCS2-H >>
endobj)PDF";
        pdf_extract_text ext;
        const std::wstring expected{ static_cast<wchar_t>(0xD55C), static_cast<wchar_t>(0xAE00) };
        CHECK(std::wcscmp(ext(text, std::strlen(text)), expected.c_str()) == 0);
        }
    SECTION("Predefined CMap Name Classification")
        {
        // the legacy CJK CMap name -> charset mapping
        CHECK(pdf_text_decoder::predefined_cmap_charset("90ms-RKSJ-H") == "CP932");
        CHECK(pdf_text_decoder::predefined_cmap_charset("90msp-RKSJ-V") == "CP932");
        CHECK(pdf_text_decoder::predefined_cmap_charset("Ext-RKSJ-H") == "CP932");
        CHECK(pdf_text_decoder::predefined_cmap_charset("EUC-H") == "EUC-JP");
        CHECK(pdf_text_decoder::predefined_cmap_charset("EUC-V") == "EUC-JP");
        CHECK(pdf_text_decoder::predefined_cmap_charset("ETenms-B5-H") == "CP950");
        CHECK(pdf_text_decoder::predefined_cmap_charset("ETen-B5-V") == "CP950");
        CHECK(pdf_text_decoder::predefined_cmap_charset("HKscs-B5-H") == "CP950");
        CHECK(pdf_text_decoder::predefined_cmap_charset("B5pc-H") == "CP950");
        CHECK(pdf_text_decoder::predefined_cmap_charset("GBK-EUC-H") == "CP936");
        CHECK(pdf_text_decoder::predefined_cmap_charset("GBKp-EUC-V") == "CP936");
        CHECK(pdf_text_decoder::predefined_cmap_charset("GBK2K-H") == "GB18030");
        CHECK(pdf_text_decoder::predefined_cmap_charset("GB-EUC-H") == "GB2312");
        CHECK(pdf_text_decoder::predefined_cmap_charset("GBpc-EUC-H") == "GB2312");
        CHECK(pdf_text_decoder::predefined_cmap_charset("KSCms-UHC-H") == "CP949");
        CHECK(pdf_text_decoder::predefined_cmap_charset("KSCms-UHC-HW-V") == "CP949");
        CHECK(pdf_text_decoder::predefined_cmap_charset("KSC-EUC-H") == "EUC-KR");
        // not predefined CJK CMaps
        CHECK(pdf_text_decoder::predefined_cmap_charset("Identity-H").empty());
        CHECK(pdf_text_decoder::predefined_cmap_charset("WinAnsiEncoding").empty());
        CHECK(pdf_text_decoder::predefined_cmap_charset("UniJIS-UCS2-H").empty());
        // the Unicode CMap names
        CHECK(pdf_text_decoder::is_unicode_cmap_name("UniJIS-UCS2-H"));
        CHECK(pdf_text_decoder::is_unicode_cmap_name("UniJIS-UCS2-HW-V"));
        CHECK(pdf_text_decoder::is_unicode_cmap_name("UniGB-UTF16-H"));
        CHECK(pdf_text_decoder::is_unicode_cmap_name("UniCNS-UCS2-V"));
        CHECK(pdf_text_decoder::is_unicode_cmap_name("UniKS-UTF16-V"));
        CHECK_FALSE(pdf_text_decoder::is_unicode_cmap_name("Identity-H"));
        CHECK_FALSE(pdf_text_decoder::is_unicode_cmap_name("ETenms-B5-H"));
        CHECK_FALSE(pdf_text_decoder::is_unicode_cmap_name("WinAnsiEncoding"));
        }
    SECTION("Usecmap Name Parsing")
        {
        CHECK(pdf_text_decoder::parse_usecmap_name(
                  "/CMapName /Custom-UCS2-H def\n/UniKS-UCS2-H usecmap\n") == "UniKS-UCS2-H");
        // extra whitespace between the name and the keyword is allowed
        CHECK(pdf_text_decoder::parse_usecmap_name("/ETenms-B5-H   \t\nusecmap") == "ETenms-B5-H");
        // no usecmap directive at all
        CHECK(pdf_text_decoder::parse_usecmap_name("/CMapName /Custom-H def").empty());
        // "usecmap" not preceded by a /Name token
        CHECK(pdf_text_decoder::parse_usecmap_name("usecmap").empty());
        CHECK(pdf_text_decoder::parse_usecmap_name("123 usecmap").empty());
        CHECK(pdf_text_decoder::parse_usecmap_name("").empty());
        }
    SECTION("Embedded CMap Stream With Usecmap (Predefined Unicode)")
        {
        // a Type0 font whose /Encoding is an indirect reference to a font-embedded
        // CMap stream (as used by subsetted CID fonts), rather than a predefined
        // CMap name directly. The stream itself chains to a predefined Unicode CMap
        // via "usecmap", so the font's string bytes should be decoded as UTF-16BE
        // (D55C AE00 is U+D55C U+AE00, Korean "hangul"), just like if /Encoding had
        // named /UniKS-UCS2-H directly.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <D55CAE00> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding 4 0 R >>
endobj
4 0 obj
<< >>
stream
/CMapName /Custom-Subset-H def
/UniKS-UCS2-H usecmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        const std::wstring expected{ static_cast<wchar_t>(0xD55C), static_cast<wchar_t>(0xAE00) };
        CHECK(std::wcscmp(ext(text, std::strlen(text)), expected.c_str()) == 0);
        }
    SECTION("Embedded CMap Stream With Usecmap (Predefined Legacy Charset)")
        {
        // same as the "Predefined CJK CMap Encoding With Charset Converter" case,
        // but /Encoding is an indirect reference to an embedded CMap stream that
        // chains to /ETenms-B5-H via usecmap, rather than naming it directly
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf <A4A4A4E5> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding 4 0 R >>
endobj
4 0 obj
<< >>
stream
/ETenms-B5-H usecmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        ext.set_charset_converter(
            [](std::string_view sourceBytes, std::string_view charsetName) -> std::wstring
            {
                CHECK(sourceBytes == "\xA4\xA4\xA4\xE5");
                CHECK(charsetName == "CP950");
                return L"Converted content";
            });
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Converted content") == 0);
        }
    SECTION("Embedded CMap Stream Without Usecmap")
        {
        // an embedded CMap stream with no usecmap directive (e.g., one that defines
        // its own cidrange table directly): there's no predefined CMap to chain to,
        // so the font falls back to the default (WinAnsi) single-byte decoding
        // rather than crashing or throwing
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf (A) Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding 4 0 R >>
endobj
4 0 obj
<< >>
stream
/CMapName /Custom-Subset-H def
1 begincidrange
<00> <FF> 0
endcidrange
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(ext(text, std::strlen(text)) != nullptr);
        }
    SECTION("Embedded CMap Stream With WMode But No Usecmap")
        {
        // an embedded CMap stream with no usecmap directive (so there's no predefined
        // CMap's name to infer vertical-ness from), but which declares /WMode 1
        // itself, the way a subsetted CID font's own vertical CMap would. The font's
        // /ToUnicode CMap supplies the readable text; the /Encoding CMap's only job
        // here is establishing the codespace and the writing mode. This is the same
        // column-break layout as the predefined-CMap vertical writing mode test: a
        // same-row move to a new column is a new line in vertical mode, not a
        // same-line word gap.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 12 Tf 1 0 0 1 400 700 Tm <0043006F006C00200041> Tj ET
BT /F1 12 Tf 1 0 0 1 386 700 Tm <0043006F006C00200042> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding 4 0 R /ToUnicode 5 0 R >>
endobj
4 0 obj
<< >>
stream
/CMapName /Custom-Subset-V def
/WMode 1 def
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
endstream
endobj
5 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
1 beginbfrange
<0020> <007A> <0020>
endbfrange
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Col A\nCol B") == 0);
        }
    SECTION("Inline Image With False EI Match In Binary Data")
        {
        // an inline image with a declared /L length of 8 bytes, whose data contains
        // a decoy " EI " that a naive scan would mistake for the image's end.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< >>
stream
BT 0 700 Td (Before) Tj ET
BI /L 8 ID AB EI (C EI
BT 0 680 Td (After) Tj ET
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Before\nAfter") == 0);
        }
    SECTION("Devanagari Text Via ToUnicode CMap")
        {
        // CIDs and ToUnicode mapping taken from a real Hindi (Type0/Identity-H,
        // CIDFontType2) PDF; the CIDs are subsetted-font-specific glyph indices
        // with no relation to Unicode, so ToUnicode is the only way to recover
        // the text: "\x092D\x093E\x0930\x0924 \x0914\x0930" (Bharat aur, "India and").
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 15 Tf <00970221009A008F0003006B009A> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
6 beginbfchar
<0003> <0020>
<008F> <0924>
<0097> <092D>
<009A> <0930>
<0221> <093E>
<006B> <0914>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\x092D\x093E\x0930\x0924 \x0914\x0930") == 0);
        }
    SECTION("Arabic RTL Run Reordering")
        {
        // This run's glyphs are stored in visual order, the reverse of Arabic
        // reading order. Reversed, it spells "العربية" ("the Arabic") with
        // valid glyph joining.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 36 Tf <0045003d000400520034001300b1> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
7 beginbfchar
<0004> <FE91>
<0013> <FEDF>
<0034> <FECC>
<003d> <FEF4>
<0045> <FE94>
<0052> <FEAE>
<00b1> <0627>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)),
                          L"\x0627\xFEDF\xFECC\xFEAE\xFE91\xFEF4\xFE94") == 0);
        }
    SECTION("Hebrew RTL Run Reordering")
        {
        // This run's glyphs are stored in visual order, the reverse of Hebrew
        // reading order. Reversed, it spells "שלום" ("Shalom").
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 36 Tf <0001000200030004> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
4 beginbfchar
<0001> <05DD>
<0002> <05D5>
<0003> <05DC>
<0004> <05E9>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"\x05E9\x05DC\x05D5\x05DD") == 0);
        }
    SECTION("Arabic-Indic Digits Are Not Reversed")
        {
        // This run's glyphs are stored in visual order. The Arabic letters (ب د)
        // need reversing, but the Arabic-Indic digits (١٢٣, "123") must keep their
        // own left-to-right order, since digits aren't mirrored in RTL text.
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R /Resources << /Font << /F1 3 0 R >> >> >>
endobj
2 0 obj
<< >>
stream
BT /F1 36 Tf <00010002000300040005> Tj ET
endstream
endobj
3 0 obj
<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /ToUnicode 4 0 R >>
endobj
4 0 obj
<< >>
stream
begincmap
1 begincodespacerange
<0000> <FFFF>
endcodespacerange
5 beginbfchar
<0001> <0661>
<0002> <0662>
<0003> <0663>
<0004> <062F>
<0005> <0628>
endbfchar
endcmap
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)),
                          L"\x0661\x0662\x0663\x0628\x062F") == 0);
        }
    SECTION("Decompressor Functor")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< /Filter /FlateDecode /Length 10 >>
stream
0123456789
endstream
endobj)PDF";
        pdf_extract_text ext;
        // a stand-in "decompressor" that verifies what it was handed and
        // returns the "uncompressed" content stream
        ext.set_stream_decompressor(
            [](std::string_view compressed) -> std::string
            {
                CHECK(compressed == "0123456789");
                return "BT (Inflated content) Tj ET";
            });
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Inflated content") == 0);
        }
    SECTION("Compressed Content Without Decompressor")
        {
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< /Filter /FlateDecode /Length 10 >>
stream
0123456789
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(ext(text, std::strlen(text)) != nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext.get_log().find(L"decompressor") != std::wstring::npos);
        }
    SECTION("ASCIIHex Filter")
        {
        // "BT (Hex) Tj ET", hex encoded
        const char* text = R"PDF(%PDF-1.4
1 0 obj
<< /Type /Page /Contents 2 0 R >>
endobj
2 0 obj
<< /Filter /ASCIIHexDecode >>
stream
42542028486578292054 6A204554>
endstream
endobj)PDF";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text, std::strlen(text)), L"Hex") == 0);
        }
    SECTION("LZW Filter")
        {
        // "BT (LZW works) Tj ET", LZW encoded (the PDF/TIFF flavor, with "early change"
        // widening of the codes), including its conventional leading clear-table code
        static constexpr unsigned char lzwData[]{
            0x80, 0x10, 0x8A, 0x82, 0x01, 0x41, 0x30, 0xB4, 0x57, 0x10, 0x1D, 0xCD, 0xE7,
            0x23, 0x59, 0xCC, 0x52, 0x20, 0x2A, 0x1A, 0x84, 0x04, 0x52, 0xA4, 0x04
        };
        std::string text{ "%PDF-1.4\n"
                          "1 0 obj\n<< /Type /Page /Contents 2 0 R >>\nendobj\n"
                          "2 0 obj\n<< /Filter /LZWDecode /Length 25 >>\nstream\n" };
        text.append(reinterpret_cast<const char*>(lzwData), sizeof(lzwData));
        text += "\nendstream\nendobj";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text.data(), text.length()), L"LZW works") == 0);
        }
    SECTION("MD5 Known Vectors")
        {
        // RFC 1321, Appendix A.5
        CHECK(pdf_decryptor::md5_digest("") ==
              std::string("\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e", 16));
        CHECK(pdf_decryptor::md5_digest("abc") ==
              std::string("\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72", 16));
        }
    SECTION("RC4 Known Vector")
        {
        // a well-known RC4 test vector (key "Key", plaintext "Plaintext")
        const std::string ciphertext{ pdf_decryptor::rc4_crypt("Key", "Plaintext") };
        CHECK(ciphertext ==
              std::string("\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3", 9));
        // RC4 is symmetric, so decrypting the ciphertext recovers the plaintext
        CHECK(pdf_decryptor::rc4_crypt("Key", ciphertext) == "Plaintext");
        }
    SECTION("Encrypted PDF (RC4, Empty User Password)")
        {
        // RC4-128 (/V 2 /R 3) encrypted content stream for "BT (Secret) Tj ET",
        // protected with an empty user password (only an owner password/restrictions
        // are set). The encryption key, /O, /U, and the encrypted stream bytes below
        // were computed offline (and round-tripped) against this decoder's exact
        // key-derivation algorithm.
        static constexpr unsigned char cipherData[]{
            0xA0, 0x65, 0xCC, 0x8C, 0xAE, 0xF1, 0xF8, 0x70, 0xCE,
            0x0E, 0x46, 0xAE, 0x22, 0x08, 0x7C, 0x02, 0xB9
        };
        std::string text{ "%PDF-1.4\n"
                          "1 0 obj\n<< /Type /Page /Contents 2 0 R >>\nendobj\n"
                          "2 0 obj\n<< /Length 17 >>\nstream\n" };
        text.append(reinterpret_cast<const char*>(cipherData), sizeof(cipherData));
        text += "\nendstream\nendobj\n"
                "5 0 obj\n"
                "<< /Filter /Standard /V 2 /R 3 /Length 128\n"
                "   /O <0000000000000000000000000000000000000000000000000000000000000000>\n"
                "   /U <3f2c1f7fec88c547dc5f5a6f2ee230ad00000000000000000000000000000000>\n"
                "   /P -44 >>\n"
                "endobj\n"
                "trailer\n"
                "<< /Encrypt 5 0 R /ID [<000102030405060708090a0b0c0d0e0f> "
                "<000102030405060708090a0b0c0d0e0f>] >>";
        pdf_extract_text ext;
        CHECK(std::wcscmp(ext(text.data(), text.length()), L"Secret") == 0);
        }
    SECTION("Encrypted PDF With Wrong Key Length Fails Closed")
        {
        // same as above, but /O is altered by one byte, which changes the derived
        // encryption key; authentication against /U should then fail and throw,
        // rather than silently emitting garbage text
        static constexpr unsigned char cipherData[]{
            0xA0, 0x65, 0xCC, 0x8C, 0xAE, 0xF1, 0xF8, 0x70, 0xCE,
            0x0E, 0x46, 0xAE, 0x22, 0x08, 0x7C, 0x02, 0xB9
        };
        std::string text{ "%PDF-1.4\n"
                          "1 0 obj\n<< /Type /Page /Contents 2 0 R >>\nendobj\n"
                          "2 0 obj\n<< /Length 17 >>\nstream\n" };
        text.append(reinterpret_cast<const char*>(cipherData), sizeof(cipherData));
        text += "\nendstream\nendobj\n"
                "5 0 obj\n"
                "<< /Filter /Standard /V 2 /R 3 /Length 128\n"
                "   /O <0100000000000000000000000000000000000000000000000000000000000000>\n"
                "   /U <3f2c1f7fec88c547dc5f5a6f2ee230ad00000000000000000000000000000000>\n"
                "   /P -44 >>\n"
                "endobj\n"
                "trailer\n"
                "<< /Encrypt 5 0 R /ID [<000102030405060708090a0b0c0d0e0f> "
                "<000102030405060708090a0b0c0d0e0f>] >>";
        pdf_extract_text ext;
        CHECK_THROWS_AS(ext(text.data(), text.length()), pdf_extract_text::pdf_encrypted);
        }
    }

// NOLINTEND
// clang-format on
