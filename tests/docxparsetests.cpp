// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/docx_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Word 2007 Parser", "[docx import]")
    {
    SECTION("Null")
        {
        word2007_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext(L"<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"Introductory Overview\">\n<param name=\"Local\" value=\"Statistics/Overview/Descriptives/IntroOverview.htm\">\n</object>\n<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"&quot;True&quot; Mean and Confidence Interval\">\n<param name=\"Local\" value=\"Statistics/Overviews/Descriptives/TrueMeanandConfidenceInt.html\">\n</object>", 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("List")
        {
        const wchar_t* text = L"<w:body><w:p w:rsidR=\"00CB314B\" w:rsidRDefault=\"004F174B\" w:rsidP=\"004F174B\"><w:pPr><w:pStyle w:val=\"ListParagraph\"/><w:numPr><w:ilvl w:val=\"0\"/><w:numId w:val=\"1\"/></w:numPr></w:pPr><w:r><w:t>List item 1</w:t></w:r></w:p><w:p w:rsidR=\"004F174B\" w:rsidRDefault=\"004F174B\" w:rsidP=\"004F174B\"><w:pPr><w:pStyle w:val=\"ListParagraph\"/><w:numPr><w:ilvl w:val=\"0\"/><w:numId w:val=\"1\"/></w:numPr></w:pPr><w:r><w:t>List item 2</w:t></w:r></w:p><w:sectPr w:rsidR=\"004F174B\" w:rsidSect=\"00CB314B\"><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/><w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"360\"/></w:sectPr></w:body>";
        word2007_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\n\tList item 1\n\n\tList item 2") == 0);
        CHECK(ext.get_filtered_text_length() == 28);
        }
    SECTION("Table")
        {
        const wchar_t* text = L"<w:body><w:tbl><w:tblPr><w:tblStyle w:val=\"TableGrid\"/><w:tblW w:w=\"0\" w:type=\"auto\"/><w:tblLook w:val=\"04A0\"/></w:tblPr><w:tblGrid><w:gridCol w:w=\"4788\"/><w:gridCol w:w=\"4788\"/></w:tblGrid><w:tr w:rsidR=\"00CE66DB\" w:rsidTr=\"00CE66DB\"><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>First</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Second</w:t></w:r></w:p></w:tc></w:tr><w:tr w:rsidR=\"00CE66DB\" w:rsidTr=\"00CE66DB\"><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Third</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Fourth</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p w:rsidR=\"004F174B\" w:rsidRPr=\"00CE66DB\" w:rsidRDefault=\"004F174B\" w:rsidP=\"00CE66DB\"/><w:sectPr w:rsidR=\"004F174B\" w:rsidRPr=\"00CE66DB\" w:rsidSect=\"00CB314B\"><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/><w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"360\"/></w:sectPr></w:body>";
        word2007_extract_text ext;
        ext.preserve_text_table_layout(true);
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\n\tFirst\tSecond\n\n\tThird\tFourth\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 32);
        }
    SECTION("Table Make Paragraphs")
        {
        const wchar_t* text = L"<w:body><w:tbl><w:tblPr><w:tblStyle w:val=\"TableGrid\"/><w:tblW w:w=\"0\" w:type=\"auto\"/><w:tblLook w:val=\"04A0\"/></w:tblPr><w:tblGrid><w:gridCol w:w=\"4788\"/><w:gridCol w:w=\"4788\"/></w:tblGrid><w:tr w:rsidR=\"00CE66DB\" w:rsidTr=\"00CE66DB\"><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>First</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Second</w:t></w:r></w:p></w:tc></w:tr><w:tr w:rsidR=\"00CE66DB\" w:rsidTr=\"00CE66DB\"><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Third</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w=\"4788\" w:type=\"dxa\"/></w:tcPr><w:p w:rsidR=\"00CE66DB\" w:rsidRDefault=\"00CE66DB\" w:rsidP=\"00CE66DB\"><w:r><w:t>Fourth</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p w:rsidR=\"004F174B\" w:rsidRPr=\"00CE66DB\" w:rsidRDefault=\"004F174B\" w:rsidP=\"00CE66DB\"/><w:sectPr w:rsidR=\"004F174B\" w:rsidRPr=\"00CE66DB\" w:rsidSect=\"00CB314B\"><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/><w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"360\"/></w:sectPr></w:body>";
        word2007_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\n\t\n\nFirst\t\n\nSecond\n\n\t\n\nThird\t\n\nFourth\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 40);
        }
    SECTION("Indenting")
        {
        const wchar_t* text = L"<w:body><w:p w:rsidR=\"004F174B\" w:rsidRDefault=\"000B7A40\" w:rsidP=\"000B7A40\"><w:pPr><w:ind w:left=\"720\"/></w:pPr><w:r><w:t>Indented Paragraph</w:t></w:r></w:p><w:p w:rsidR=\"000B7A40\" w:rsidRDefault=\"000B7A40\" w:rsidP=\"000B7A40\"><w:pPr><w:jc w:val=\"center\"/></w:pPr><w:r><w:t>Centered paragraph</w:t></w:r></w:p><w:p w:rsidR=\"000B7A40\" w:rsidRPr=\"000B7A40\" w:rsidRDefault=\"000B7A40\" w:rsidP=\"000B7A40\"><w:r><w:t>Regular paragraph</w:t></w:r></w:p><w:sectPr w:rsidR=\"000B7A40\" w:rsidRPr=\"000B7A40\" w:rsidSect=\"00CB314B\"><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/><w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"360\"/></w:sectPr></w:body>";
        word2007_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\n\tIndented Paragraph\n\n\tCentered paragraph\n\nRegular paragraph") == 0);
        CHECK(ext.get_filtered_text_length() == 61);
        }
    SECTION("Line Break")
        {
        const wchar_t* text = L"<w:body><w:p w:rsidR=\"000B7A40\" w:rsidRPr=\"000B7A40\" w:rsidRDefault=\"009C0747\" w:rsidP=\"000B7A40\"><w:r><w:t xml:space=\"preserve\">A </w:t></w:r><w:proofErr w:type=\"spellStart\"/><w:r><w:t>linebreak</w:t></w:r><w:proofErr w:type=\"spellEnd\"/><w:r><w:br/><w:t>here.</w:t></w:r></w:p><w:sectPr w:rsidR=\"000B7A40\" w:rsidRPr=\"000B7A40\" w:rsidSect=\"00CB314B\"><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\" w:header=\"720\" w:footer=\"720\" w:gutter=\"0\"/><w:cols w:space=\"720\"/><w:docGrid w:linePitch=\"360\"/></w:sectPr></w:body>";
        word2007_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nA linebreak\nhere.") == 0);
        CHECK(ext.get_filtered_text_length() == 19);
        }
    SECTION("Page Break")
        {
        const wchar_t* text = L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><w:document xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" xmlns:wps=\"http://schemas.microsoft.com/office/word/2010/wordprocessingShape\" xmlns:wpg=\"http://schemas.microsoft.com/office/word/2010/wordprocessingGroup\" xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" xmlns:wp14=\"http://schemas.microsoft.com/office/word/2010/wordprocessingDrawing\" xmlns:w14=\"http://schemas.microsoft.com/office/word/2010/wordml\" mc:Ignorable=\"w14 wp14\"><w:body><w:p><w:pPr><w:pStyle w:val=\"Normal\"/><w:rPr></w:rPr></w:pPr><w:r><w:rPr></w:rPr><w:t>Hello.</w:t></w:r></w:p><w:p><w:pPr><w:pStyle w:val=\"Normal\"/><w:rPr></w:rPr></w:pPr><w:r><w:rPr></w:rPr></w:r><w:r><w:br w:type=\"page\"/></w:r></w:p><w:p><w:pPr><w:pStyle w:val=\"Normal\"/><w:rPr></w:rPr></w:pPr><w:r><w:rPr></w:rPr><w:t>Here is page 2.</w:t></w:r></w:p><w:sectPr><w:type w:val=\"nextPage\"/><w:pgSz w:w=\"12240\" w:h=\"15840\"/><w:pgMar w:left=\"1134\" w:right=\"1134\" w:header=\"0\" w:top=\"1134\" w:footer=\"0\" w:bottom=\"1134\" w:gutter=\"0\"/><w:pgNumType w:fmt=\"decimal\"/><w:formProt w:val=\"false\"/><w:textDirection w:val=\"lrTb\"/></w:sectPr></w:body></w:document>";
        word2007_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nHello.\n\n\f\n\nHere is page 2.") == 0);
        CHECK(ext.get_filtered_text_length() == 28);
        }
    }

// NOLINTEND
// clang-format on
