// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/data/dataset.h"
#include "../../src/util/zipcatalog.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <stdexcept>
#include <vector>

using namespace Wisteria;
using namespace Wisteria::Data;

// crickets.csv has no quoted fields or embedded commas, so a plain split is enough.
static std::vector<std::vector<wxString>> OdsTestReadSimpleCsv(const wxString& path)
    {
    std::vector<std::vector<wxString>> rows;
    wxTextFile textFile;
    REQUIRE(textFile.Open(path));
    for (size_t lineIndex = 0; lineIndex < textFile.GetLineCount(); ++lineIndex)
        {
        const wxString& line = textFile.GetLine(lineIndex);
        if (line.empty())
            {
            continue;
            }
        std::vector<wxString> cells;
        wxString cell;
        for (const auto ch : line)
            {
            if (ch == L',')
                {
                cells.push_back(cell);
                cell.clear();
                }
            else
                {
                cell += ch;
                }
            }
        cells.push_back(cell);
        rows.push_back(std::move(cells));
        }
    return rows;
    }

static wxString OdsTestXmlEscape(wxString text)
    {
    text.Replace(L"&", L"&amp;");
    text.Replace(L"<", L"&lt;");
    text.Replace(L">", L"&gt;");
    return text;
    }

// Builds a minimal ODS package (just content.xml) holding one string-typed sheet,
// then writes it to disk at odsPath.
static void WriteMockOds(const wxString& odsPath, const wxString& sheetName,
                         const std::vector<std::vector<wxString>>& rows)
    {
    wxString tableRows;
    for (const auto& row : rows)
        {
        tableRows += L"<table:table-row>";
        for (const auto& value : row)
            {
            tableRows += L"<table:table-cell office:value-type=\"string\"><text:p>" +
                         OdsTestXmlEscape(value) + L"</text:p></table:table-cell>";
            }
        tableRows += L"</table:table-row>";
        }

    const wxString contentXml =
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        L"<office:document-content"
        L" xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
        L" xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\""
        L" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">"
        L"<office:body><office:spreadsheet>"
        L"<table:table table:name=\"" + OdsTestXmlEscape(sheetName) + L"\">" + tableRows +
        L"</table:table>"
        L"</office:spreadsheet></office:body>"
        L"</office:document-content>";

        {
        wxFFileOutputStream fileStream(odsPath);
        REQUIRE(fileStream.IsOk());
        wxZipOutputStream zipStream(fileStream);
        ZipCatalog::WriteText(zipStream, L"content.xml", contentXml);
        REQUIRE(zipStream.Close());
        }
    }

// crickets.csv: index column "rownames", string column "species", numeric "temp" and "rate".
static ImportInfo OdsTestCricketsImportInfo()
    {
    return ImportInfo()
        .IdColumn(L"rownames")
        .ContinuousColumns({ L"temp", L"rate" })
        .CategoricalColumns({ { L"species", CategoricalImportMethod::ReadAsStrings } });
    }

// defined further down, next to the original round-trip test
static void OdsTestCheckMatchesCsv(Dataset& dsOds, Dataset& dsCsv, size_t dataRowCount);

// Absolute path to a file under datasets/historical/ next to the test binary.
static wxString OdsTestHistoricalPath(const wxString& fileName)
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    return appDir + L"/datasets/historical/" + fileName;
    }

// Joins a row matrix back into comma-delimited text (no escaping: callers control
// the cell contents).
static wxString OdsTestRowsToCsv(const std::vector<std::vector<wxString>>& rows)
    {
    wxString out;
    for (const auto& row : rows)
        {
        for (size_t colIndex = 0; colIndex < row.size(); ++colIndex)
            {
            if (colIndex != 0)
                {
                out += L',';
                }
            out += row[colIndex];
            }
        out += L'\n';
        }
    return out;
    }

static void OdsTestWriteTextFile(const wxString& path, const wxString& content)
    {
    wxFile outFile(path, wxFile::write);
    REQUIRE(outFile.IsOpened());
    REQUIRE(outFile.Write(content, wxConvUTF8));
    }

// Wraps every header cell in a matched pair of double quotes and pads every data
// cell with surrounding spaces. cell_trim() and cell_collapse_quotes() must undo
// all of it identically on the CSV importer and the ODS matrix path.
static std::vector<std::vector<wxString>>
OdsTestDecorateCells(std::vector<std::vector<wxString>> rows)
    {
    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
        {
        for (wxString& cell : rows[rowIndex])
            {
            cell = (rowIndex == 0) ? (L"\"" + cell + L"\"") : (L"  " + cell + L"  ");
            }
        }
    return rows;
    }

// ---------------------------------------------------------------------------
// LoadWorksheetMatrix() must normalize cells the same way the delimited
// text path does (column-name lookups and cell values must agree).
// ---------------------------------------------------------------------------
TEST_CASE("ODS matrix import trims whitespace and quotes like the CSV path",
          "[data][ods][import][normalize]")
    {
    const wxString csvPath{ OdsTestHistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto plainRows = OdsTestReadSimpleCsv(csvPath);
    REQUIRE(plainRows.size() > 1);
    REQUIRE(plainRows.front() ==
            std::vector<wxString>{ L"rownames", L"species", L"temp", L"rate" });

    const auto decoratedRows = OdsTestDecorateCells(plainRows);
    const wxString decoratedCsvPath{ wxFileName::CreateTempFileName(L"wisteria_ods_norm_") +
                                     L".csv" };
    const wxString decoratedOdsPath{ wxFileName::CreateTempFileName(L"wisteria_ods_norm_") +
                                     L".ods" };
    OdsTestWriteTextFile(decoratedCsvPath, OdsTestRowsToCsv(decoratedRows));
    WriteMockOds(decoratedOdsPath, L"crickets", decoratedRows);

    Dataset dsPlainCsv;
    dsPlainCsv.ImportCSV(csvPath, OdsTestCricketsImportInfo());

    // the ImportInfo asks for the clean column names; if the decorated header row
    // were not normalized identically this would throw "column not found"
    Dataset dsDecoratedOds;
    REQUIRE_NOTHROW(dsDecoratedOds.ImportOds(decoratedOdsPath, static_cast<size_t>(1),
                                             OdsTestCricketsImportInfo()));
    OdsTestCheckMatchesCsv(dsDecoratedOds, dsPlainCsv, plainRows.size() - 1);

    wxRemoveFile(decoratedCsvPath);
    wxRemoveFile(decoratedOdsPath);
    }

// ---------------------------------------------------------------------------
// ImportInfo::SkipRows() on the matrix path.
// ---------------------------------------------------------------------------
TEST_CASE("ODS matrix import honors SkipRows", "[data][ods][import][skiprows]")
    {
    const wxString csvPath{ OdsTestHistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto dataRows = OdsTestReadSimpleCsv(csvPath);
    REQUIRE(dataRows.size() > 1);

    Dataset dsPlain;
    dsPlain.ImportCSV(csvPath, OdsTestCricketsImportInfo());

    SECTION("Rows before the header row are skipped")
        {
        std::vector<std::vector<wxString>> rows{
            { L"# exported by mocktool", L"", L"", L"" },
            { L"# source: crickets.csv", L"", L"", L"" },
            { L"# two comment rows precede the header", L"", L"", L"" },
        };
        rows.insert(rows.end(), dataRows.cbegin(), dataRows.cend());

        const wxString odsPath{ wxFileName::CreateTempFileName(L"wisteria_ods_skip_") + L".ods" };
        WriteMockOds(odsPath, L"crickets", rows);

        Dataset dsSkipped;
        dsSkipped.ImportOds(odsPath, static_cast<size_t>(1), OdsTestCricketsImportInfo().SkipRows(3));
        OdsTestCheckMatchesCsv(dsSkipped, dsPlain, dataRows.size() - 1);

        wxRemoveFile(odsPath);
        }

    SECTION("SkipRows at or past the data yields an empty dataset without throwing")
        {
        const wxString odsPath{ wxFileName::CreateTempFileName(L"wisteria_ods_skip_") + L".ods" };
        WriteMockOds(odsPath, L"crickets", dataRows);

        Dataset dsHeaderOnly;
        REQUIRE_NOTHROW(dsHeaderOnly.ImportOds(
            odsPath, static_cast<size_t>(1),
            OdsTestCricketsImportInfo().SkipRows(dataRows.size() - 1)));
        CHECK(dsHeaderOnly.GetRowCount() == 0);
        CHECK(dsHeaderOnly.GetContinuousColumns().empty());

        Dataset dsWayPast;
        REQUIRE_NOTHROW(dsWayPast.ImportOds(
            odsPath, static_cast<size_t>(1),
            OdsTestCricketsImportInfo().SkipRows(dataRows.size() + 25)));
        CHECK(dsWayPast.GetRowCount() == 0);
        CHECK(dsWayPast.GetContinuousColumns().empty());

        wxRemoveFile(odsPath);
        }
    }

// ---------------------------------------------------------------------------
// OdsReader::ReadWorksheetData() rejects invalid worksheet selectors.
// ---------------------------------------------------------------------------
TEST_CASE("ODS import rejects invalid worksheet selectors", "[data][ods][import][errors]")
    {
    const wxString csvPath{ OdsTestHistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));
    const auto rows = OdsTestReadSimpleCsv(csvPath);
    REQUIRE(rows.size() > 1);

    const wxString odsPath{ wxFileName::CreateTempFileName(L"wisteria_ods_err_") + L".ods" };
    WriteMockOds(odsPath, L"crickets", rows);

    SECTION("Unknown worksheet name")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(
            ds.ImportOds(odsPath, wxString(L"does not exist"), OdsTestCricketsImportInfo()),
            std::runtime_error);
        }

    SECTION("Worksheet index 0 (indices are 1-based)")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(ds.ImportOds(odsPath, static_cast<size_t>(0), OdsTestCricketsImportInfo()),
                          std::runtime_error);
        }

    SECTION("Worksheet index past the last sheet")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(ds.ImportOds(odsPath, static_cast<size_t>(2), OdsTestCricketsImportInfo()),
                          std::runtime_error);
        }

    wxRemoveFile(odsPath);
    }

static void OdsTestCheckMatchesCsv(Dataset& dsOds, Dataset& dsCsv, const size_t dataRowCount)
    {
    CHECK(dsOds.GetRowCount() == dataRowCount);
    CHECK(dsOds.GetRowCount() == dsCsv.GetRowCount());
    CHECK(dsOds.GetContinuousColumns().size() == dsCsv.GetContinuousColumns().size());
    CHECK(dsOds.GetCategoricalColumns().size() == dsCsv.GetCategoricalColumns().size());

    for (const wxString& colName : { wxString(L"temp"), wxString(L"rate") })
        {
        const auto odsCol = dsOds.GetContinuousColumn(colName);
        const auto csvCol = dsCsv.GetContinuousColumn(colName);
        REQUIRE(odsCol != dsOds.GetContinuousColumns().cend());
        REQUIRE(csvCol != dsCsv.GetContinuousColumns().cend());
        CHECK(odsCol->GetValues() == csvCol->GetValues());
        }

    const auto odsSpecies = dsOds.GetCategoricalColumn(L"species");
    const auto csvSpecies = dsCsv.GetCategoricalColumn(L"species");
    REQUIRE(odsSpecies != dsOds.GetCategoricalColumns().cend());
    REQUIRE(csvSpecies != dsCsv.GetCategoricalColumns().cend());
    for (size_t i = 0; i < dsOds.GetRowCount(); ++i)
        {
        CHECK(odsSpecies->GetValueAsLabel(i) == csvSpecies->GetValueAsLabel(i));
        CHECK(dsOds.GetIdColumn().GetValue(i) == dsCsv.GetIdColumn().GetValue(i));
        }

    // spot checks against known values from crickets.csv
    CHECK(odsSpecies->GetValueAsLabel(0) == L"O. exclamationis");
    CHECK(odsSpecies->GetValueAsLabel(dsOds.GetRowCount() - 1) == L"O. niveus");
    CHECK(dsOds.GetIdColumn().GetValue(0) == L"1");
    CHECK_THAT(dsOds.GetContinuousColumn(L"temp")->GetValues().at(0),
               Catch::Matchers::WithinRel(20.8, 1e-9));
    }

TEST_CASE("ODS import matches the CSV path for a historical dataset", "[data][ods][import]")
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxString csvPath{ appDir + L"/datasets/historical/crickets.csv" };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto rows = OdsTestReadSimpleCsv(csvPath);
    REQUIRE(rows.size() > 1);
    REQUIRE(rows.front() == std::vector<wxString>{ L"rownames", L"species", L"temp", L"rate" });

    const wxString odsPath{ wxFileName::CreateTempFileName(L"wisteria_ods_rt_") + L".ods" };
    WriteMockOds(odsPath, L"crickets", rows);

    Dataset dsCsv;
    dsCsv.ImportCSV(csvPath, OdsTestCricketsImportInfo());

    SECTION("Worksheet selected by index")
        {
        Dataset dsOds;
        dsOds.ImportOds(odsPath, static_cast<size_t>(1), OdsTestCricketsImportInfo());
        OdsTestCheckMatchesCsv(dsOds, dsCsv, rows.size() - 1);
        }

    SECTION("Worksheet selected by name")
        {
        Dataset dsOds;
        dsOds.ImportOds(odsPath, wxString(L"crickets"), OdsTestCricketsImportInfo());
        OdsTestCheckMatchesCsv(dsOds, dsCsv, rows.size() - 1);
        }

    wxRemoveFile(odsPath);
    }

// NOLINTEND
