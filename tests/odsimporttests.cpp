// NOLINTBEGIN

#include "../src/import/ods_extract_text.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("ODS worksheet names", "[ods][names]")
    {
    SECTION("Single worksheet")
        {
        const wchar_t contentXml[] =
            L"<?xml version=\"1.0\"?>"
            L"<office:document-content"
            L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
            L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\">"
            L"<office:body><office:spreadsheet>"
            L"<table:table table:name=\"Sheet1\">"
            L"</table:table>"
            L"</office:spreadsheet></office:body>"
            L"</office:document-content>";

        ods_extract_text ext{ true };
        ext.read_worksheet_names(contentXml, std::wcslen(contentXml));

        const auto names = ext.get_worksheet_names();
        REQUIRE(names.size() == 1);
        CHECK(names[0] == L"Sheet1");
        }

    SECTION("Multiple worksheets")
        {
        const wchar_t contentXml[] =
            L"<?xml version=\"1.0\"?>"
            L"<office:document-content"
            L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
            L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\">"
            L"<office:body><office:spreadsheet>"
            L"<table:table table:name=\"Overview\">"
            L"</table:table>"
            L"<table:table table:name=\"Metrics\">"
            L"</table:table>"
            L"<table:table table:name=\"RawData\">"
            L"</table:table>"
            L"</office:spreadsheet></office:body>"
            L"</office:document-content>";

        ods_extract_text ext{ true };
        ext.read_worksheet_names(contentXml, std::wcslen(contentXml));

        const auto names = ext.get_worksheet_names();
        REQUIRE(names.size() == 3);
        CHECK(names[0] == L"Overview");
        CHECK(names[1] == L"Metrics");
        CHECK(names[2] == L"RawData");
        }

    SECTION("Null input")
        {
        ods_extract_text ext{ true };
        ext.read_worksheet_names(nullptr, 0);
        CHECK(ext.get_worksheet_names().empty());
        }

    SECTION("Empty input")
        {
        ods_extract_text ext{ true };
        ext.read_worksheet_names(L"", 0);
        CHECK(ext.get_worksheet_names().empty());
        }

    SECTION("No spreadsheet element")
        {
        const wchar_t contentXml[] =
            L"<?xml version=\"1.0\"?>"
            L"<office:document-content"
            L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\">"
            L"<office:body></office:body>"
            L"</office:document-content>";

        ods_extract_text ext{ true };
        ext.read_worksheet_names(contentXml, std::wcslen(contentXml));
        CHECK(ext.get_worksheet_names().empty());
        }
    }

TEST_CASE("ODS read string cells", "[ods][strings]")
    {
    // a 2x2 sheet: header row ("Name", "City") and one data row ("Alice", "Paris")
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Name</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"string\"><text:p>City</text:p></table:table-cell>"
        L"</table:table-row>"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Alice</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Paris</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;

    SECTION("Select by name")
        {
        ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Sheet1" });
        REQUIRE(wrk.size() == 2);
        REQUIRE(wrk[0].size() == 2);
        REQUIRE(wrk[1].size() == 2);
        CHECK(wrk[0][0].get_value() == L"Name");
        CHECK(wrk[0][1].get_value() == L"City");
        CHECK(wrk[1][0].get_value() == L"Alice");
        CHECK(wrk[1][1].get_value() == L"Paris");
        }

    SECTION("Select by 1-based index")
        {
        ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));
        REQUIRE(wrk.size() == 2);
        CHECK(wrk[0][0].get_value() == L"Name");
        CHECK(wrk[1][0].get_value() == L"Alice");
        }

    SECTION("Nonexistent worksheet name returns empty")
        {
        ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"NoSuchSheet" });
        CHECK(wrk.empty());
        }

    SECTION("Out-of-range index returns empty")
        {
        ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(99));
        CHECK(wrk.empty());
        }
    }

TEST_CASE("ODS read numeric cells", "[ods][numeric]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Data\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"float\" office:value=\"42\">"
        L"<text:p>42</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"float\" office:value=\"3.14\">"
        L"<text:p>3.14</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Data" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 2);
    CHECK(wrk[0][0].get_value() == L"42");
    CHECK(wrk[0][1].get_value() == L"3.14");
    }

TEST_CASE("ODS read date cells", "[ods][date]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Dates\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"date\" office:date-value=\"2024-03-15\">"
        L"<text:p>03/15/2024</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"date\" office:date-value=\"2024-03-15T08:30:00\">"
        L"<text:p>03/15/2024 08:30</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Dates" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 2);
    // date-only: kept as-is
    CHECK(wrk[0][0].get_value() == L"2024-03-15");
    // date-time: T replaced with space
    CHECK(wrk[0][1].get_value() == L"2024-03-15 08:30:00");
    }

TEST_CASE("ODS read boolean cells", "[ods][boolean]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Bools\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"boolean\" office:boolean-value=\"true\">"
        L"<text:p>TRUE</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"boolean\" office:boolean-value=\"false\">"
        L"<text:p>FALSE</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Bools" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 2);
    CHECK(wrk[0][0].get_value() == L"TRUE");
    CHECK(wrk[0][1].get_value() == L"FALSE");
    }

TEST_CASE("ODS read currency and percentage cells", "[ods][currency][percentage]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Money\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"currency\" office:value=\"99.95\">"
        L"<text:p>$99.95</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"percentage\" office:value=\"0.75\">"
        L"<text:p>75%</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Money" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 2);
    CHECK(wrk[0][0].get_value() == L"99.95");
    CHECK(wrk[0][1].get_value() == L"0.75");
    }

TEST_CASE("ODS read time cells", "[ods][time]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Times\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"time\" office:time-value=\"PT08H30M00S\">"
        L"<text:p>08:30:00</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Times" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 1);
    CHECK(wrk[0][0].get_value() == L"PT08H30M00S");
    }

TEST_CASE("ODS column-repeated cells", "[ods][repeat][columns]")
    {
    // one row with a cell repeated 3 times via table:number-columns-repeated
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\" table:number-columns-repeated=\"3\">"
        L"<text:p>X</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 3);
    CHECK(wrk[0][0].get_value() == L"X");
    CHECK(wrk[0][1].get_value() == L"X");
    CHECK(wrk[0][2].get_value() == L"X");
    }

TEST_CASE("ODS row-repeated rows", "[ods][repeat][rows]")
    {
    // a single row repeated 3 times
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row table:number-rows-repeated=\"3\">"
        L"<table:table-cell office:value-type=\"string\"><text:p>Y</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"float\" office:value=\"7\">"
        L"<text:p>7</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 3);
    for (size_t r = 0; r < 3; ++r)
        {
        REQUIRE(wrk[r].size() == 2);
        CHECK(wrk[r][0].get_value() == L"Y");
        CHECK(wrk[r][1].get_value() == L"7");
        }
    }

TEST_CASE("ODS large empty column repetition is skipped", "[ods][repeat][padding]")
    {
    // an empty cell repeated 1000 times (LibreOffice padding), followed by a real cell
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>A</text:p></table:table-cell>"
        L"<table:table-cell table:number-columns-repeated=\"1000\"/>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    // should have just 1 real cell, not 1001
    CHECK(wrk[0].size() == 1);
    CHECK(wrk[0][0].get_value() == L"A");
    }

TEST_CASE("ODS large empty row repetition stops parsing", "[ods][repeat][trailing]")
    {
    // a real row followed by 1000 empty repeated rows (LibreOffice trailing padding)
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Data</text:p></table:table-cell>"
        L"</table:table-row>"
        L"<table:table-row table:number-rows-repeated=\"1000\">"
        L"<table:table-cell table:number-columns-repeated=\"1000\"/>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    // only the one real data row
    REQUIRE(wrk.size() == 1);
    CHECK(wrk[0][0].get_value() == L"Data");
    }

TEST_CASE("ODS covered table cells (merged regions)", "[ods][covered]")
    {
    // cell A1 spans 2 columns, so B1 is a covered-table-cell placeholder
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\""
        L" table:number-columns-spanned=\"2\" table:number-rows-spanned=\"1\">"
        L"<text:p>Merged</text:p></table:table-cell>"
        L"<table:covered-table-cell/>"
        L"<table:table-cell office:value-type=\"string\"><text:p>C1</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 3);
    CHECK(wrk[0][0].get_value() == L"Merged");
    CHECK(wrk[0][1].get_value().empty()); // covered cell
    CHECK(wrk[0][2].get_value() == L"C1");
    }

TEST_CASE("ODS multiple worksheets, select second by index", "[ods][multisheet]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"First\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>A</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"<table:table table:name=\"Second\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>B</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };

    SECTION("By index")
        {
        ods_extract_text::worksheet wrk;
        ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(2));
        REQUIRE(wrk.size() == 1);
        CHECK(wrk[0][0].get_value() == L"B");
        }

    SECTION("By name")
        {
        ods_extract_text::worksheet wrk;
        ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Second" });
        REQUIRE(wrk.size() == 1);
        CHECK(wrk[0][0].get_value() == L"B");
        }
    }

TEST_CASE("ODS null and zero-length input", "[ods][null]")
    {
    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;

    ext(nullptr, 72, wrk, static_cast<size_t>(1));
    CHECK(wrk.empty());

    ext(L"some text", 0, wrk, static_cast<size_t>(1));
    CHECK(wrk.empty());
    }

TEST_CASE("ODS mixed cell types in one row", "[ods][mixed]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Mixed\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Name</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"float\" office:value=\"25\">"
        L"<text:p>25</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"date\" office:date-value=\"2024-01-15\">"
        L"<text:p>01/15/2024</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"boolean\" office:boolean-value=\"true\">"
        L"<text:p>TRUE</text:p></table:table-cell>"
        L"<table:table-cell/>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, std::wstring{ L"Mixed" });

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 5);
    CHECK(wrk[0][0].get_value() == L"Name");
    CHECK(wrk[0][1].get_value() == L"25");
    CHECK(wrk[0][2].get_value() == L"2024-01-15");
    CHECK(wrk[0][3].get_value() == L"TRUE");
    CHECK(wrk[0][4].get_value().empty());
    }

TEST_CASE("ODS multi-paragraph string cell", "[ods][strings][multiline]")
    {
    // a cell with two <text:p> elements, joined by a space
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\">"
        L"<text:p>Line one</text:p>"
        L"<text:p>Line two</text:p>"
        L"</table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 1);
    CHECK(wrk[0][0].get_value() == L"Line one Line two");
    }

TEST_CASE("ODS HTML entities in string cells", "[ods][strings][entities]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\">"
        L"<text:p>A &amp; B &lt; C</text:p>"
        L"</table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 1);
    CHECK(wrk[0][0].get_value() == L"A & B < C");
    }

TEST_CASE("ODS get_worksheet_text output", "[ods][text]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Name</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Score</text:p></table:table-cell>"
        L"</table:table-row>"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Alice</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"float\" office:value=\"95\">"
        L"<text:p>95</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    const std::wstring text = ods_extract_text::get_worksheet_text(wrk);
    CHECK(text == std::wstring{ L"Name\tScore\nAlice\t95" });
    }

TEST_CASE("ODS verify_sheet on parsed data", "[ods][verify]")
    {
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>A</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"string\"><text:p>B</text:p></table:table-cell>"
        L"</table:table-row>"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>C</text:p></table:table-cell>"
        L"<table:table-cell office:value-type=\"string\"><text:p>D</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    CHECK(ods_extract_text::verify_sheet(wrk).first);
    }

TEST_CASE("ODS empty cells between data cells", "[ods][empty]")
    {
    // row with: string, empty, string (the empty cell has no value-type)
    const wchar_t contentXml[] =
        L"<?xml version=\"1.0\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"Sheet1\">"
        L"<table:table-row>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Left</text:p></table:table-cell>"
        L"<table:table-cell/>"
        L"<table:table-cell office:value-type=\"string\"><text:p>Right</text:p></table:table-cell>"
        L"</table:table-row>"
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

    ods_extract_text ext{ true };
    ods_extract_text::worksheet wrk;
    ext(contentXml, std::wcslen(contentXml), wrk, static_cast<size_t>(1));

    REQUIRE(wrk.size() == 1);
    REQUIRE(wrk[0].size() == 3);
    CHECK(wrk[0][0].get_value() == L"Left");
    CHECK(wrk[0][1].get_value().empty());
    CHECK(wrk[0][2].get_value() == L"Right");
    }

// NOLINTEND
