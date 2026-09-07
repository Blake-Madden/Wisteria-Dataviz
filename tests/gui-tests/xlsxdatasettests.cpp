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
static std::vector<std::vector<wxString>> ReadSimpleCsv(const wxString& path)
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

static wxString XmlEscape(wxString text)
    {
    text.Replace(L"&", L"&amp;");
    text.Replace(L"<", L"&lt;");
    text.Replace(L">", L"&gt;");
    return text;
    }

// Builds a minimal but valid XLSX package holding one worksheet whose cells are all
// inline strings, then writes it to disk at xlsxPath.
static void WriteMockXlsx(const wxString& xlsxPath, const wxString& sheetName,
                          const std::vector<std::vector<wxString>>& rows)
    {
    const size_t columnCount = rows.empty() ? 0 : rows.front().size();

    wxString sheetData;
    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
        {
        sheetData += wxString::Format(L"<row r=\"%zu\">", rowIndex + 1);
        for (size_t colIndex = 0; colIndex < rows[rowIndex].size(); ++colIndex)
            {
            const wxString cellRef =
                wxString::Format(L"%c%zu", static_cast<wxChar>(L'A' + colIndex), rowIndex + 1);
            sheetData += wxString::Format(
                L"<c r=\"%s\" t=\"inlineStr\"><is><t xml:space=\"preserve\">%s</t></is></c>",
                cellRef, XmlEscape(rows[rowIndex][colIndex]));
            }
        sheetData += L"</row>";
        }
    const wxString dimensionRef =
        wxString::Format(L"A1:%c%zu", static_cast<wxChar>(L'A' + (columnCount ? columnCount - 1 : 0)),
                         rows.size());

    const wxString sheetXml =
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        L"<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        L"<dimension ref=\"" + dimensionRef + L"\"/><sheetData>" + sheetData +
        L"</sheetData></worksheet>";

    const wxString workbookXml =
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        L"<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        L"<sheets><sheet name=\"" + XmlEscape(sheetName) +
        L"\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>";

    const wxString workbookRels =
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        L"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        L"<Relationship Id=\"rId1\" "
        L"Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
        L"Target=\"worksheets/sheet1.xml\"/></Relationships>";

    const wxString stylesXml =
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        L"<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        L"<cellXfs count=\"1\"><xf numFmtId=\"0\"/></cellXfs></styleSheet>";

        {
        wxFFileOutputStream fileStream(xlsxPath);
        REQUIRE(fileStream.IsOk());
        wxZipOutputStream zipStream(fileStream);
        ZipCatalog::WriteText(zipStream, L"xl/workbook.xml", workbookXml);
        ZipCatalog::WriteText(zipStream, L"xl/_rels/workbook.xml.rels", workbookRels);
        ZipCatalog::WriteText(zipStream, L"xl/styles.xml", stylesXml);
        ZipCatalog::WriteText(zipStream, L"xl/worksheets/sheet1.xml", sheetXml);
        REQUIRE(zipStream.Close());
        }
    }

// Absolute path to a file under datasets/historical/ next to the test binary.
static wxString HistoricalPath(const wxString& fileName)
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    return appDir + L"/datasets/historical/" + fileName;
    }

// Joins a row matrix back into comma-delimited text (no escaping: callers control
// the cell contents).
static wxString RowsToCsv(const std::vector<std::vector<wxString>>& rows)
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

static void WriteTextFile(const wxString& path, const wxString& content)
    {
    wxFile outFile(path, wxFile::write);
    REQUIRE(outFile.IsOpened());
    REQUIRE(outFile.Write(content, wxConvUTF8));
    }

// Wraps every header cell in a matched pair of double quotes and pads every data
// cell with surrounding spaces. cell_trim() and cell_collapse_quotes() must undo
// all of it identically on the CSV importer and the XLSX matrix path.
static std::vector<std::vector<wxString>>
DecorateCells(std::vector<std::vector<wxString>> rows)
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

// crickets.csv: id column "rownames", string column "species", numeric "temp"/"rate".
static ImportInfo CricketsImportInfo()
    {
    return ImportInfo()
        .IdColumn(L"rownames")
        .ContinuousColumns({ L"temp", L"rate" })
        .CategoricalColumns({ { L"species", CategoricalImportMethod::ReadAsStrings } });
    }

static void CheckCricketsDatasetsEqual(Dataset& lhs, Dataset& rhs)
    {
    REQUIRE(lhs.GetRowCount() == rhs.GetRowCount());
    for (const wxString& colName : { wxString(L"temp"), wxString(L"rate") })
        {
        const auto lhsCol = lhs.GetContinuousColumn(colName);
        const auto rhsCol = rhs.GetContinuousColumn(colName);
        REQUIRE(lhsCol != lhs.GetContinuousColumns().cend());
        REQUIRE(rhsCol != rhs.GetContinuousColumns().cend());
        CHECK(lhsCol->GetValues() == rhsCol->GetValues());
        }
    const auto lhsSpecies = lhs.GetCategoricalColumn(L"species");
    const auto rhsSpecies = rhs.GetCategoricalColumn(L"species");
    REQUIRE(lhsSpecies != lhs.GetCategoricalColumns().cend());
    REQUIRE(rhsSpecies != rhs.GetCategoricalColumns().cend());
    for (size_t rowIndex = 0; rowIndex < lhs.GetRowCount(); ++rowIndex)
        {
        CHECK(lhsSpecies->GetValueAsLabel(rowIndex) == rhsSpecies->GetValueAsLabel(rowIndex));
        CHECK(lhs.GetIdColumn().GetValue(rowIndex) == rhs.GetIdColumn().GetValue(rowIndex));
        }
    }

// ---------------------------------------------------------------------------
// LoadWorksheetMatrix() must normalize cells the same way the delimited
// text path does, so that column-name lookups and cell values agree.
// ---------------------------------------------------------------------------
TEST_CASE("XLSX matrix import trims whitespace and quotes like the CSV path",
          "[data][xlsx][import][normalize]")
    {
    const wxString csvPath{ HistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto plainRows = ReadSimpleCsv(csvPath);
    REQUIRE(plainRows.size() > 1);
    REQUIRE(plainRows.front() ==
            std::vector<wxString>{ L"rownames", L"species", L"temp", L"rate" });

    const auto decoratedRows = DecorateCells(plainRows);
    const wxString decoratedCsvPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_norm_") +
                                     L".csv" };
    const wxString decoratedXlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_norm_") +
                                      L".xlsx" };
    WriteTextFile(decoratedCsvPath, RowsToCsv(decoratedRows));
    WriteMockXlsx(decoratedXlsxPath, L"crickets", decoratedRows);

    Dataset dsPlainCsv;
    dsPlainCsv.ImportCSV(csvPath, CricketsImportInfo());

    // the ImportInfo asks for the clean column names; if the decorated header row
    // were not normalized identically this would throw "column not found"
    Dataset dsDecoratedXlsx;
    REQUIRE_NOTHROW(dsDecoratedXlsx.ImportExcel(decoratedXlsxPath, static_cast<size_t>(1),
                                               CricketsImportInfo()));
    Dataset dsDecoratedCsv;
    REQUIRE_NOTHROW(dsDecoratedCsv.ImportCSV(decoratedCsvPath, CricketsImportInfo()));

    CheckCricketsDatasetsEqual(dsDecoratedXlsx, dsPlainCsv);
    CheckCricketsDatasetsEqual(dsDecoratedXlsx, dsDecoratedCsv);

    wxRemoveFile(decoratedCsvPath);
    wxRemoveFile(decoratedXlsxPath);
    }

TEST_CASE("XLSX matrix import collapses doubled quotes like the CSV path",
          "[data][xlsx][import][normalize]")
    {
    // a species label stored with spreadsheet-style doubled quotes ("" -> ")
    const std::vector<std::vector<wxString>> rows{
        { L"rownames", L"species", L"temp", L"rate" },
        { L"1", L"O. \"\"exclamationis\"\"", L"20.8", L"67.9" },
        { L"2", L"O. niveus", L"17.2", L"44.3" },
    };

    const wxString csvPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_quote_") + L".csv" };
    const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_quote_") + L".xlsx" };
    WriteTextFile(csvPath, RowsToCsv(rows));
    WriteMockXlsx(xlsxPath, L"crickets", rows);

    Dataset dsXlsx;
    dsXlsx.ImportExcel(xlsxPath, static_cast<size_t>(1), CricketsImportInfo());
    Dataset dsCsv;
    dsCsv.ImportCSV(csvPath, CricketsImportInfo());

    const auto xlsxSpecies = dsXlsx.GetCategoricalColumn(L"species");
    const auto csvSpecies = dsCsv.GetCategoricalColumn(L"species");
    REQUIRE(xlsxSpecies != dsXlsx.GetCategoricalColumns().cend());
    REQUIRE(csvSpecies != dsCsv.GetCategoricalColumns().cend());
    CHECK(xlsxSpecies->GetValueAsLabel(0) == L"O. \"exclamationis\"");
    CHECK(xlsxSpecies->GetValueAsLabel(0) == csvSpecies->GetValueAsLabel(0));
    CHECK(xlsxSpecies->GetValueAsLabel(1) == L"O. niveus");

    wxRemoveFile(csvPath);
    wxRemoveFile(xlsxPath);
    }

// ---------------------------------------------------------------------------
// the ReadColumnInfo() preview (what the import dialog shows) must match
// the columns ImportExcel() actually resolves.
// ---------------------------------------------------------------------------
TEST_CASE("XLSX column preview matches the CSV preview and drives a matching import",
          "[data][xlsx][import][preview]")
    {
    const wxString csvPath{ HistoricalPath(L"Nightingale.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto rows = ReadSimpleCsv(csvPath);
    REQUIRE(rows.size() > 1);

    const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_prev_") + L".xlsx" };
    WriteMockXlsx(xlsxPath, L"Nightingale", rows);

    const auto csvPreview = Dataset::ReadColumnInfo(csvPath);
    const auto xlsxPreview = Dataset::ReadColumnInfo(xlsxPath);

    REQUIRE(xlsxPreview.size() == rows.front().size());
    REQUIRE(xlsxPreview.size() == csvPreview.size());
    for (size_t colIndex = 0; colIndex < csvPreview.size(); ++colIndex)
        {
        INFO("column index " << colIndex);
        CHECK(xlsxPreview[colIndex].m_name == rows.front()[colIndex]);
        CHECK(xlsxPreview[colIndex].m_name == csvPreview[colIndex].m_name);
        CHECK(xlsxPreview[colIndex].m_type == csvPreview[colIndex].m_type);
        CHECK(xlsxPreview[colIndex].m_currencySymbol == csvPreview[colIndex].m_currencySymbol);
        }

    // the previewed column set must resolve cleanly at import time on both paths
    const auto importInfo = Dataset::ImportInfoFromPreview(csvPreview);

    Dataset dsXlsx;
    REQUIRE_NOTHROW(dsXlsx.ImportExcel(xlsxPath, static_cast<size_t>(1), importInfo));
    Dataset dsCsv;
    dsCsv.ImportCSV(csvPath, importInfo);

    CHECK(dsXlsx.GetRowCount() == rows.size() - 1);
    CHECK(dsXlsx.GetRowCount() == dsCsv.GetRowCount());
    REQUIRE(dsXlsx.GetContinuousColumns().size() == dsCsv.GetContinuousColumns().size());
    CHECK(dsXlsx.GetCategoricalColumns().size() == dsCsv.GetCategoricalColumns().size());
    CHECK(dsXlsx.GetDateColumns().size() == dsCsv.GetDateColumns().size());

    for (const auto& csvCol : dsCsv.GetContinuousColumns())
        {
        const auto xlsxCol = dsXlsx.GetContinuousColumn(csvCol.GetName());
        REQUIRE(xlsxCol != dsXlsx.GetContinuousColumns().cend());
        CHECK(xlsxCol->GetValues() == csvCol.GetValues());
        }

    wxRemoveFile(xlsxPath);
    }

// ---------------------------------------------------------------------------
// ImportInfo::SkipRows() on the matrix path.
// ---------------------------------------------------------------------------
TEST_CASE("XLSX matrix import honors SkipRows", "[data][xlsx][import][skiprows]")
    {
    const wxString csvPath{ HistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto dataRows = ReadSimpleCsv(csvPath);
    REQUIRE(dataRows.size() > 1);

    Dataset dsPlain;
    dsPlain.ImportCSV(csvPath, CricketsImportInfo());

    SECTION("Rows before the header row are skipped")
        {
        std::vector<std::vector<wxString>> rows{
            { L"# exported by mocktool", L"", L"", L"" },
            { L"# source: crickets.csv", L"", L"", L"" },
            { L"# two comment rows precede the header", L"", L"", L"" },
        };
        rows.insert(rows.end(), dataRows.cbegin(), dataRows.cend());

        const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_skip_") + L".xlsx" };
        WriteMockXlsx(xlsxPath, L"crickets", rows);

        Dataset dsSkipped;
        dsSkipped.ImportExcel(xlsxPath, static_cast<size_t>(1), CricketsImportInfo().SkipRows(3));
        CheckCricketsDatasetsEqual(dsSkipped, dsPlain);

        wxRemoveFile(xlsxPath);
        }

    SECTION("SkipRows at or past the data yields an empty dataset without throwing")
        {
        const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_skip_") + L".xlsx" };
        WriteMockXlsx(xlsxPath, L"crickets", dataRows);

        // leaves only the header row, so there are no data rows to load
        Dataset dsHeaderOnly;
        REQUIRE_NOTHROW(dsHeaderOnly.ImportExcel(xlsxPath, static_cast<size_t>(1),
                                                CricketsImportInfo().SkipRows(dataRows.size() - 1)));
        CHECK(dsHeaderOnly.GetRowCount() == 0);
        CHECK(dsHeaderOnly.GetContinuousColumns().empty());

        // skips past every row in the sheet
        Dataset dsWayPast;
        REQUIRE_NOTHROW(dsWayPast.ImportExcel(xlsxPath, static_cast<size_t>(1),
                                             CricketsImportInfo().SkipRows(dataRows.size() + 25)));
        CHECK(dsWayPast.GetRowCount() == 0);
        CHECK(dsWayPast.GetContinuousColumns().empty());

        wxRemoveFile(xlsxPath);
        }
    }

// ---------------------------------------------------------------------------
// ExcelReader::ReadWorksheetData() rejects invalid worksheet selectors.
// ---------------------------------------------------------------------------
TEST_CASE("XLSX import rejects invalid worksheet selectors", "[data][xlsx][import][errors]")
    {
    const wxString csvPath{ HistoricalPath(L"crickets.csv") };
    REQUIRE(wxFileName::FileExists(csvPath));
    const auto rows = ReadSimpleCsv(csvPath);
    REQUIRE(rows.size() > 1);

    const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_err_") + L".xlsx" };
    WriteMockXlsx(xlsxPath, L"crickets", rows);

    SECTION("Unknown worksheet name")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(
            ds.ImportExcel(xlsxPath, wxString(L"does not exist"), CricketsImportInfo()),
            std::runtime_error);
        }

    SECTION("Worksheet index 0 (indices are 1-based)")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(ds.ImportExcel(xlsxPath, static_cast<size_t>(0), CricketsImportInfo()),
                          std::runtime_error);
        }

    SECTION("Worksheet index past the last sheet")
        {
        Dataset ds;
        REQUIRE_THROWS_AS(ds.ImportExcel(xlsxPath, static_cast<size_t>(2), CricketsImportInfo()),
                          std::runtime_error);
        }

    SECTION("ReadColumnInfo surfaces the same error")
        {
        REQUIRE_THROWS_AS(
            (void)Dataset::ReadColumnInfo(xlsxPath, ImportInfo{}, std::nullopt,
                                          wxString(L"missing")),
            std::runtime_error);
        }

    wxRemoveFile(xlsxPath);
    }

TEST_CASE("XLSX import matches the CSV path for a historical dataset", "[data][xlsx][import]")
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxString csvPath{ appDir + L"/datasets/historical/crickets.csv" };
    REQUIRE(wxFileName::FileExists(csvPath));

    const auto rows = ReadSimpleCsv(csvPath);
    REQUIRE(rows.size() > 1);
    REQUIRE(rows.front() == std::vector<wxString>{ L"rownames", L"species", L"temp", L"rate" });

    const wxString xlsxPath{ wxFileName::CreateTempFileName(L"wisteria_xlsx_rt_") + L".xlsx" };
    WriteMockXlsx(xlsxPath, L"crickets", rows);

    const auto makeImportInfo = []()
    {
        return ImportInfo()
            .IdColumn(L"rownames")
            .ContinuousColumns({ L"temp", L"rate" })
            .CategoricalColumns({ { L"species", CategoricalImportMethod::ReadAsStrings } });
    };

    Dataset dsCsv;
    dsCsv.ImportCSV(csvPath, makeImportInfo());

    Dataset dsXlsx;
    dsXlsx.ImportExcel(xlsxPath, static_cast<size_t>(1), makeImportInfo());

    SECTION("Same shape")
        {
        CHECK(dsXlsx.GetRowCount() == rows.size() - 1);
        CHECK(dsXlsx.GetRowCount() == dsCsv.GetRowCount());
        CHECK(dsXlsx.GetContinuousColumns().size() == dsCsv.GetContinuousColumns().size());
        CHECK(dsXlsx.GetCategoricalColumns().size() == dsCsv.GetCategoricalColumns().size());
        }

    SECTION("Continuous columns are identical")
        {
        for (const wxString& colName : { wxString(L"temp"), wxString(L"rate") })
            {
            const auto xlsxCol = dsXlsx.GetContinuousColumn(colName);
            const auto csvCol = dsCsv.GetContinuousColumn(colName);
            REQUIRE(xlsxCol != dsXlsx.GetContinuousColumns().cend());
            REQUIRE(csvCol != dsCsv.GetContinuousColumns().cend());
            CHECK(xlsxCol->GetValues() == csvCol->GetValues());
            }
        // a value pulled straight from crickets.csv row 1
        CHECK_THAT(dsXlsx.GetContinuousColumn(L"temp")->GetValues().at(0),
                   Catch::Matchers::WithinRel(20.8, 1e-9));
        }

    SECTION("Categorical labels are identical row by row")
        {
        const auto xlsxCol = dsXlsx.GetCategoricalColumn(L"species");
        const auto csvCol = dsCsv.GetCategoricalColumn(L"species");
        REQUIRE(xlsxCol != dsXlsx.GetCategoricalColumns().cend());
        REQUIRE(csvCol != dsCsv.GetCategoricalColumns().cend());
        for (size_t i = 0; i < dsXlsx.GetRowCount(); ++i)
            {
            CHECK(xlsxCol->GetValueAsLabel(i) == csvCol->GetValueAsLabel(i));
            }
        CHECK(xlsxCol->GetValueAsLabel(0) == L"O. exclamationis");
        CHECK(xlsxCol->GetValueAsLabel(dsXlsx.GetRowCount() - 1) == L"O. niveus");
        }

    SECTION("ID column is identical")
        {
        for (size_t i = 0; i < dsXlsx.GetRowCount(); ++i)
            {
            CHECK(dsXlsx.GetIdColumn().GetValue(i) == dsCsv.GetIdColumn().GetValue(i));
            }
        CHECK(dsXlsx.GetIdColumn().GetValue(0) == L"1");
        }

    SECTION("Worksheet can also be selected by name")
        {
        Dataset dsByName;
        dsByName.ImportExcel(xlsxPath, wxString(L"crickets"), makeImportInfo());
        CHECK(dsByName.GetRowCount() == dsCsv.GetRowCount());
        CHECK(dsByName.GetContinuousColumn(L"rate")->GetValues() ==
              dsCsv.GetContinuousColumn(L"rate")->GetValues());
        }

    wxRemoveFile(xlsxPath);
    }

// NOLINTEND
