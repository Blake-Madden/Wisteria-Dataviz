///////////////////////////////////////////////////////////////////////////////
// Name:        excelreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "excelreader.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    void ExcelReader::LoadFile(const wxString& filePath)
        {
        m_filePath = filePath;
        MemoryMappedFile sourceFile(m_filePath, true, true);
        const ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                                 sourceFile.GetMapSize());

        // load the worksheet names
        std::wstring zipFileText = archive.ReadTextFile(L"xl/workbook.xml");
        m_xlsxTextExtractor.read_worksheet_names(zipFileText.c_str(), zipFileText.length());

        // load workbook relationships
        zipFileText = archive.ReadTextFile(L"xl/_rels/workbook.xml.rels");
        m_xlsxTextExtractor.read_relative_paths(zipFileText.c_str(), zipFileText.length());

        // resolve worksheet names to their XML paths
        m_xlsxTextExtractor.map_workbook_paths();

        // load the string table
        zipFileText = archive.ReadTextFile(L"xl/sharedStrings.xml");
        if (!zipFileText.empty())
            {
            m_xlsxTextExtractor.read_shared_strings(zipFileText.c_str(), zipFileText.length());
            }

        // load the styles
        zipFileText = archive.ReadTextFile(L"xl/styles.xml");
        m_xlsxTextExtractor.read_styles(zipFileText.c_str(), zipFileText.length());
        }

    //---------------------------------------------------
    lily_of_the_valley::xlsx_extract_text::worksheet
    ExcelReader::ReadWorksheetData(const std::variant<wxString, size_t>& worksheet)
        {
        MemoryMappedFile sourceFile(m_filePath, true, true);
        const ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                                 sourceFile.GetMapSize());

        const auto& worksheetPaths = m_xlsxTextExtractor.get_worksheet_paths();
        std::wstring sheetPath;

        // find the sheet by name
        if (const auto* const worksheetName{ std::get_if<wxString>(&worksheet) };
            worksheetName != nullptr)
            {
            const auto sheetPos =
                std::ranges::find_if(worksheetPaths, [&](const auto& wsPath)
                                     { return wsPath.first == worksheetName->wc_str(); });
            if (sheetPos == worksheetPaths.cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': Unable to find worksheet in Excel workbook."),
                                     *worksheetName)
                        .ToUTF8());
                }
            sheetPath = sheetPos->second;
            }
        // ...or index (1-based)
        else if (const auto* const worksheetIndex{ std::get_if<size_t>(&worksheet) };
                 worksheetIndex != nullptr)
            {
            if (*worksheetIndex == 0 || *worksheetIndex > worksheetPaths.size())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"Worksheet '%zu': worksheet out of range in Excel workbook."),
                        *worksheetIndex)
                        .ToUTF8());
                }
            sheetPath = worksheetPaths[*worksheetIndex - 1].second;
            }
        else
            {
            throw std::runtime_error(_(L"Unknown value specified for Excel worksheet.").ToUTF8());
            }

        const std::wstring sheetFile = archive.ReadTextFile(sheetPath);
        lily_of_the_valley::xlsx_extract_text::worksheet wkData;
        m_xlsxTextExtractor(sheetFile.c_str(), sheetFile.length(), wkData);
        return wkData;
        }

    //---------------------------------------------------
    wxString ExcelReader::ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
                                        const wchar_t delimiter)
        {
        auto wkData = ReadWorksheetData(worksheet);
        return lily_of_the_valley::xlsx_extract_text::get_worksheet_text(wkData, delimiter);
        }

    //---------------------------------------------------
    std::vector<std::vector<std::wstring>>
    ExcelReader::ReadWorksheetMatrix(const std::variant<wxString, size_t>& worksheet)
        {
        auto wkData = ReadWorksheetData(worksheet);
        return lily_of_the_valley::xlsx_extract_text::extract_worksheet_matrix(wkData);
        }

    //---------------------------------------------------
    ExcelReader::WorksheetContent
    ExcelReader::ReadWorksheetContent(const std::variant<wxString, size_t>& worksheet,
                                      const wchar_t delimiter)
        {
        auto wkData = ReadWorksheetData(worksheet);
        // get_worksheet_text() must run before extract_worksheet_matrix(), which
        // destructively moves each cell's value out of wkData
        WorksheetContent content;
        content.m_text =
            lily_of_the_valley::xlsx_extract_text::get_worksheet_text(wkData, delimiter);
        content.m_matrix = lily_of_the_valley::xlsx_extract_text::extract_worksheet_matrix(wkData);
        return content;
        }
    } // namespace Wisteria::Data
