///////////////////////////////////////////////////////////////////////////////
// Name:        excelreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "excelreader.h"

using namespace lily_of_the_valley;

namespace Wisteria::Data
    {
    //---------------------------------------------------
    void ExcelReader::LoadFile(const wxString& filePath)
        {
        m_filePath = filePath;
        MemoryMappedFile sourceFile(m_filePath, true, true);
        ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                           sourceFile.GetMapSize());

        // load the worksheet names
        wxString zipFileText = archive.ReadTextFile(L"xl/workbook.xml");
        m_xlsxTextExtractor.read_worksheet_names(
            zipFileText.wc_str(), zipFileText.length());

        // load the string table
        zipFileText = archive.ReadTextFile(L"xl/sharedStrings.xml");
        m_xlsxTextExtractor.read_shared_strings(
            zipFileText.wc_str(), zipFileText.length());

        // load the styles
        zipFileText = archive.ReadTextFile(L"xl/styles.xml");
        m_xlsxTextExtractor.read_styles(
            zipFileText.wc_str(), zipFileText.length());
        }

    //---------------------------------------------------
    wxString ExcelReader::ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
                                        const wchar_t delimiter)
        {
        MemoryMappedFile sourceFile(m_filePath, true, true);
        ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                           sourceFile.GetMapSize());

        // find the sheet by name
        if (const auto worksheetName{ std::get_if<wxString>(&worksheet) };
            worksheetName != nullptr)
            {
            const auto sheetPos = std::find(m_xlsxTextExtractor.get_worksheet_names().cbegin(),
                m_xlsxTextExtractor.get_worksheet_names().cend(), worksheetName->wc_str());
            if (sheetPos != m_xlsxTextExtractor.get_worksheet_names().cend())
                {
                const wxString sheetFile = archive.ReadTextFile(
                    wxString::Format(L"xl/worksheets/sheet%zu.xml",
                        (sheetPos - m_xlsxTextExtractor.get_worksheet_names().cbegin()) + 1));

                xlsx_extract_text::worksheet wkData;

                m_xlsxTextExtractor(sheetFile.wc_str(), sheetFile.length(), wkData);
                return xlsx_extract_text::get_worksheet_text(wkData, delimiter);
                }
            else
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': Unable to find worksheet in Excel workbook."), *worksheetName).ToUTF8());
                }
            }
        // ...or index (1-based)
        else if (const auto worksheetIndex{ std::get_if<size_t>(&worksheet) };
            worksheetIndex != nullptr)
            {
            if (*worksheetIndex < m_xlsxTextExtractor.get_worksheet_names().size())
                {
                const wxString sheetFile = archive.ReadTextFile(
                    wxString::Format(L"xl/worksheets/sheet%zu.xml",
                        *worksheetIndex));

                xlsx_extract_text::worksheet wkData;

                m_xlsxTextExtractor(sheetFile.wc_str(), sheetFile.length(), wkData);
                return xlsx_extract_text::get_worksheet_text(wkData, delimiter);
                }
            else
                {
                throw std::runtime_error(wxString::Format(
                    _(L"Worksheet '%zu': worksheet out of range in Excel workbook."),
                    *worksheetIndex).ToUTF8());
                }
            }
        else
            {
            throw std::runtime_error(
                _(L"Unknown value specified for Excel worksheet.").ToUTF8());
            }
        }
    }
