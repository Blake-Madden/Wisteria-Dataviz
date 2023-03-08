#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/odt_odp_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("OpenDocument Parser", "[odt import]")
    {
    SECTION("Null")
        {
        odt_odp_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        CHECK(ext(L"<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"Introductory Overview\">\n<param name=\"Local\" value=\"BasicStatistics/Overview/Descriptive/DescriptiveStatisticsIntroductoryOverview.htm\">\n</object>\n<li><object type=\"text/sitemap\">\n<param name=\"Name\" value=\"&quot;True&quot; Mean and Confidence Interval\">\n<param name=\"Local\" value=\"BasicStatistics/Overview/Descriptive/DescriptiveStatisticsTrueMeanandConfidenceInterval.htm\">\n</object>", 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("List")
        {
        const wchar_t* text = L"<text:list text:style-name=\"LFO1\" text:continue-numbering=\"true\"><text:list-item><text:p text:style-name=\"P1\">List item 1</text:p></text:list-item><text:list-item><text:p text:style-name=\"P2\">List item 2</text:p></text:list-item></text:list><text:p text:style-name=\"P12\">Regular paragraph</text:p>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\t\n\nList item 1\n\t\n\nList item 2\n\nRegular paragraph") == 0);
        CHECK(ext.get_filtered_text_length() == 49);
        }
    SECTION("Annotation")
        {
        const wchar_t* text = L"<text:list text:style-name=\"LFO1\" text:continue-numbering=\"true\"><text:list-item><text:p text:style-name=\"P1\">List item 1</text:p></text:list-item><text:list-item><text:p text:style-name=\"P2\">List item 2</text:p></text:list-item></text:list><text:p text:style-name=\"P12\">Regular paragraph</text:p><officeooo:annotation svg:y=\"0cm\" svg:x=\"0cm\">\n<dc:creator>Blake</dc:creator>\n<dc:date>2010-12-05T12:28:24.625000000</dc:date>\n<text:p>Just a random comment</text:p>\n</officeooo:annotation><!--hi>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\t\n\nList item 1\n\t\n\nList item 2\n\nRegular paragraph") == 0);
        CHECK(ext.get_filtered_text_length() == 49);
        }
    SECTION("Table")
        {
        const wchar_t* text = L"<table:table-cell table:style-name=\"TableCell5\"><text:p text:style-name=\"P6\">First</text:p></table:table-cell><table:table-cell table:style-name=\"TableCell7\"><text:p text:style-name=\"P8\">Second</text:p><text:p text:style-name=\"P9\"/></table:table-cell></table:table-row><table:table-row table:style-name=\"TableRow10\"><table:table-cell table:style-name=\"TableCell11\"><text:p text:style-name=\"P12\">Third</text:p></table:table-cell><table:table-cell table:style-name=\"TableCell13\"><text:p text:style-name=\"P14\">fourth</text:p></table:table-cell></table:table-row></table:table><text:p text:style-name=\"P15\"/><text:p text:style-name=\"P12\">Regular paragraph</text:p></office:text>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\t\n\nFirst\t\n\nSecond\n\n\n\n\t\n\nThird\t\n\nfourth\n\n\n\nRegular paragraph") == 0);
        CHECK(ext.get_filtered_text_length() == 59);
        }
    SECTION("Indenting")
        {
        const wchar_t* text = L"<office:automatic-styles><style:style style:name=\"P1\" style:parent-style-name=\"Normal\" style:master-page-name=\"MP0\" style:family=\"paragraph\"><style:paragraph-properties fo:margin-left=\"1.5in\"><style:tab-stops/></style:paragraph-properties></style:style><style:style style:name=\"P2\" style:parent-style-name=\"Normal\" style:family=\"paragraph\"><style:paragraph-properties fo:text-align=\"center\"/></style:style></office:automatic-styles><office:body><office:text text:use-soft-page-breaks=\"true\"><text:p text:style-name=\"P1\">Indented paragraph.</text:p><text:p text:style-name=\"P2\">Centered paragraph.</text:p><text:p text:style-name=\"Normal\">Regular paragraph.</text:p></office:text></office:body>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\n\tIndented paragraph.\n\n\tCentered paragraph.\n\nRegular paragraph.") == 0);
        CHECK(ext.get_filtered_text_length() == 64);
        }
    SECTION("Line Break")
        {
        const wchar_t* text = L"<office:text text:use-soft-page-breaks=\"true\"><text:p text:style-name=\"P1\">A linebreak<text:line-break/>here.</text:p></office:text>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nA linebreak\nhere.") == 0);
        CHECK_EQUAL((size_t)19, ext.get_filtered_text_length());
        }
    SECTION("Page Break")
        {
        const wchar_t* text = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?><office:document-content xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\" xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\" xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\" xmlns:number=\"urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0\" xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\" xmlns:chart=\"urn:oasis:names:tc:opendocument:xmlns:chart:1.0\" xmlns:dr3d=\"urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0\" xmlns:math=\"http://www.w3.org/1998/Math/MathML\" xmlns:form=\"urn:oasis:names:tc:opendocument:xmlns:form:1.0\" xmlns:script=\"urn:oasis:names:tc:opendocument:xmlns:script:1.0\" xmlns:ooo=\"http://openoffice.org/2004/office\" xmlns:ooow=\"http://openoffice.org/2004/writer\" xmlns:oooc=\"http://openoffice.org/2004/calc\" xmlns:dom=\"http://www.w3.org/2001/xml-events\" xmlns:xforms=\"http://www.w3.org/2002/xforms\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:rpt=\"http://openoffice.org/2005/report\" xmlns:of=\"urn:oasis:names:tc:opendocument:xmlns:of:1.2\" xmlns:xhtml=\"http://www.w3.org/1999/xhtml\" xmlns:grddl=\"http://www.w3.org/2003/g/data-view#\" xmlns:officeooo=\"http://openoffice.org/2009/office\" xmlns:tableooo=\"http://openoffice.org/2009/table\" xmlns:drawooo=\"http://openoffice.org/2010/draw\" xmlns:calcext=\"urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0\" xmlns:loext=\"urn:org:documentfoundation:names:experimental:office:xmlns:loext:1.0\" xmlns:field=\"urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0\" xmlns:formx=\"urn:openoffice:names:experimental:ooxml-odf-interop:xmlns:form:1.0\" xmlns:css3t=\"http://www.w3.org/TR/css3-text/\" office:version=\"1.2\"><office:scripts/><office:font-face-decls><style:font-face style:name=\"Arial1\" svg:font-family=\"Arial\" style:font-family-generic=\"swiss\"/><style:font-face style:name=\"Liberation Serif\" svg:font-family=\"&apos;Liberation Serif&apos;\" style:font-family-generic=\"roman\" style:font-pitch=\"variable\"/><style:font-face style:name=\"Liberation Sans\" svg:font-family=\"&apos;Liberation Sans&apos;\" style:font-family-generic=\"swiss\" style:font-pitch=\"variable\"/><style:font-face style:name=\"Arial\" svg:font-family=\"Arial\" style:font-family-generic=\"system\" style:font-pitch=\"variable\"/><style:font-face style:name=\"Microsoft YaHei\" svg:font-family=\"&apos;Microsoft YaHei&apos;\" style:font-family-generic=\"system\" style:font-pitch=\"variable\"/><style:font-face style:name=\"SimSun\" svg:font-family=\"SimSun\" style:font-family-generic=\"system\" style:font-pitch=\"variable\"/></office:font-face-decls><office:automatic-styles><style:style style:name=\"P1\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"><style:text-properties officeooo:rsid=\"000420d5\" officeooo:paragraph-rsid=\"000420d5\"/></style:style><style:style style:name=\"P2\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"><style:paragraph-properties fo:break-before=\"page\"/><style:text-properties officeooo:rsid=\"000420d5\" officeooo:paragraph-rsid=\"000420d5\"/></style:style></office:automatic-styles><office:body><office:text text:use-soft-page-breaks=\"true\"><text:sequence-decls><text:sequence-decl text:display-outline-level=\"0\" text:name=\"Illustration\"/><text:sequence-decl text:display-outline-level=\"0\" text:name=\"Table\"/><text:sequence-decl text:display-outline-level=\"0\" text:name=\"Text\"/><text:sequence-decl text:display-outline-level=\"0\" text:name=\"Drawing\"/></text:sequence-decls><text:p text:style-name=\"P1\">Hello.</text:p><text:p text:style-name=\"P1\"/><text:p text:style-name=\"P2\">Here is page 2.</text:p></office:text></office:body></office:document-content>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nHello.\n\n\f\n\nHere is page 2.") == 0);
        CHECK_EQUAL((size_t)28, ext.get_filtered_text_length());
        }
    SECTION("Spaces")
        {
        const wchar_t* text = L"<office:text text:use-soft-page-breaks=\"true\"><text:p text:style-name=\"P1\">will be posted on<text:s text:c=\"3\"/>Website</text:p></office:text>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nwill be posted on   Website") == 0);
        CHECK(ext.get_filtered_text_length() == 29);
        }
    SECTION("Spaces Too Many")
        {
        const wchar_t* text = L"<office:text text:use-soft-page-breaks=\"true\"><text:p text:style-name=\"P1\">will be posted on<text:s text:c=\"3000\"/>Website</text:p></office:text>";
        odt_odp_extract_text ext;
        const wchar_t* output = ext(text, std::wcslen(text));
        CHECK(std::wcscmp(output, L"\n\nwill be posted on          Website") == 0);
        CHECK(ext.get_filtered_text_length() == 36);
        }
    }
