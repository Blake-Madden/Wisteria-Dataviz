///////////////////////////////////////////////////////////////////////////////
// Name:        excelreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
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

        // load the string table
        zipFileText = archive.ReadTextFile(L"xl/sharedStrings.xml");
        m_xlsxTextExtractor.read_shared_strings(zipFileText.c_str(), zipFileText.length());

        // load the styles
        zipFileText = archive.ReadTextFile(L"xl/styles.xml");
        m_xlsxTextExtractor.read_styles(zipFileText.c_str(), zipFileText.length());
        }

    //---------------------------------------------------
    wxString ExcelReader::ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
                                        const wchar_t delimiter)
        {
        MemoryMappedFile sourceFile(m_filePath, true, true);
        const ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                                 sourceFile.GetMapSize());

        // find the sheet by name
        if (const auto* const worksheetName{ std::get_if<wxString>(&worksheet) };
            worksheetName != nullptr)
            {
            const auto sheetPos = std::find(m_xlsxTextExtractor.get_worksheet_names().cbegin(),
                                            m_xlsxTextExtractor.get_worksheet_names().cend(),
                                            worksheetName->wc_str());
            if (sheetPos != m_xlsxTextExtractor.get_worksheet_names().cend())
                {
                const std::wstring sheetFile = archive.ReadTextFile(wxString::Format(
                    L"xl/worksheets/sheet%zu.xml",
                    (sheetPos - m_xlsxTextExtractor.get_worksheet_names().cbegin()) + 1));

                lily_of_the_valley::xlsx_extract_text::worksheet wkData;

                m_xlsxTextExtractor(sheetFile.c_str(), sheetFile.length(), wkData);
                return lily_of_the_valley::xlsx_extract_text::get_worksheet_text(wkData, delimiter);
                }
            throw std::runtime_error(
                wxString::Format(_(L"'%s': Unable to find worksheet in Excel workbook."),
                                 *worksheetName)
                    .ToUTF8());
            }
        // ...or index (1-based)
        if (const auto* const worksheetIndex{ std::get_if<size_t>(&worksheet) };
            worksheetIndex != nullptr)
            {
            if (*worksheetIndex > 0 &&
                *worksheetIndex <= m_xlsxTextExtractor.get_worksheet_names().size())
                {
                const std::wstring sheetFile = archive.ReadTextFile(
                    wxString::Format(L"xl/worksheets/sheet%zu.xml", *worksheetIndex));

                lily_of_the_valley::xlsx_extract_text::worksheet wkData;

                m_xlsxTextExtractor(sheetFile.c_str(), sheetFile.length(), wkData);
                return lily_of_the_valley::xlsx_extract_text::get_worksheet_text(wkData, delimiter);
                }
            throw std::runtime_error(
                wxString::Format(_(L"Worksheet '%zu': worksheet out of range in Excel workbook."),
                                 *worksheetIndex)
                    .ToUTF8());
            }
        throw std::runtime_error(_(L"Unknown value specified for Excel worksheet.").ToUTF8());
        }
    } // namespace Wisteria::Data
