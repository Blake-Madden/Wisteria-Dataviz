// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/pptx_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("PowerPoint Parser", "[pptx import]")
    {
    SECTION("Null")
        {
        pptx_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext(L"<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"Introductory Overview\">\n<param name=\"Local\" value=\"BasicStatistics/Overview/Descriptive/DescriptiveStatisticsIntroductoryOverview.htm\">\n</object>\n<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"&quot;True&quot; Mean and Confidence Interval\">\n<param name=\"Local\" value=\"BasicStatistics/Overview/Descriptive/DescriptiveStatisticsTrueMeanandConfidenceInterval.htm\">\n</object>", 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Paragraph")
        {
        const wchar_t* text = L"<p:txBody>\n<a:bodyPr />\n<a:lstStyle />\n<a:p>\n<a:r>\n<a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" />\n<a:t>Mixed paragraph page</a:t>\n</a:r>\n<a:endParaRPr lang=\"en-GB\" dirty=\"0\" />\n</a:p>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Mixed paragraph page\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 22);
        }
    SECTION("Break")
        {
        const wchar_t* text = L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr><p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Title 1\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"ctrTitle\"/></p:nvPr></p:nvSpPr><p:spPr/><p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:endParaRPr lang=\"en-US\" dirty=\"0\"/></a:p></p:txBody></p:sp><p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Subtitle 2\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"subTitle\" idx=\"1\"/></p:nvPr></p:nvSpPr><p:spPr/><p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\"/><a:t>Some text</a:t></a:r><a:br><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\"/></a:br><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\"/><a:t>and more.</a:t></a:r><a:endParaRPr lang=\"en-US\" dirty=\"0\"/></a:p></p:txBody></p:sp></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nSome text\nand more.\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 23);
        }
    SECTION("Two Line Paragraph")
        {
        const wchar_t* text = L"<a:p>\n<a:pPr>\n<a:buNone />\n</a:pPr>\n<a:r>\n<a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" />\n<a:t>Here is just a regular paragraph</a:t>\n</a:r>\n</a:p>\n<a:p>\n<a:pPr>\n<a:buNone /> \n</a:pPr>\n<a:r>\n<a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /> \n<a:t>across two lines.</a:t> \n</a:r>\n<a:endParaRPr lang=\"en-GB\" dirty=\"0\" /> \n</a:p>\n</p:txBody>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Here is just a regular paragraph\nacross two lines.\n") == 0);
        CHECK(ext.get_filtered_text_length() == 51);
        }
    SECTION("Indented")
        {
        const wchar_t* text = L"<a:p>\n<a:pPr lvl=\"1\" /><a:r><a:rPr lang=\"en-US\" dirty=\"0\" err=\"1\" smtClean=\"0\" /><a:t>Subitem</a:t></a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /><a:t> 1</a:t></a:r></a:p>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\tSubitem 1\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 12);
        }
    SECTION("Not Indented")
        {
        // lvl (indentation level) is set to zero
        const wchar_t* text = L"<a:p>\n<a:pPr lvl=\"0\" /><a:r><a:rPr lang=\"en-US\" dirty=\"0\" err=\"1\" smtClean=\"0\" /><a:t>Subitem</a:t></a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /><a:t> 1</a:t></a:r></a:p>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Subitem 1\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 11);
        }
    SECTION("Space")
        {
        const wchar_t* text = L"<a:p>\n<a:pPr lvl=\"0\" /><a:r><a:rPr lang=\"en-US\" dirty=\"0\" err=\"1\" smtClean=\"0\" /><a:t>Subitem</a:t></a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /></a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /><a:t>1</a:t></a:r></a:p>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Subitem 1\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 11);
        }
    SECTION("Split Sentence")
        {
        const wchar_t* text = L"<a:p><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /> <a:t>Some</a:t> </a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" err=\"1\" smtClean=\"0\" /> <a:t> spellling</a:t> </a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /> <a:t /> </a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" err=\"1\" smtClean=\"0\" /> <a:t> errorrrs</a:t> </a:r><a:r><a:rPr lang=\"en-US\" dirty=\"0\" smtClean=\"0\" /> <a:t> here.</a:t> </a:r></a:p>";
        pptx_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"Some spellling  errorrrs here.\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 32);
        }
    }

// NOLINTEND
// clang-format on
