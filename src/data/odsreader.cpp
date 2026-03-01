///////////////////////////////////////////////////////////////////////////////
// Name:        odsreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "odsreader.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    void OdsReader::LoadFile(const wxString& filePath)
        {
        m_filePath = filePath;
        MemoryMappedFile sourceFile(m_filePath, true, true);
        const ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                                 sourceFile.GetMapSize());

        // load worksheet names and content from content.xml
        const std::wstring zipFileText = archive.ReadTextFile(L"content.xml");
        m_odsTextExtractor.read_worksheet_names(zipFileText.c_str(), zipFileText.length());
        }

    //---------------------------------------------------
    wxString OdsReader::ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
                                      const wchar_t delimiter)
        {
        MemoryMappedFile sourceFile(m_filePath, true, true);
        const ZipCatalog archive(static_cast<const char*>(sourceFile.GetStream()),
                                 sourceFile.GetMapSize());

        const std::wstring contentXml = archive.ReadTextFile(L"content.xml");

        lily_of_the_valley::ods_extract_text::worksheet wkData;

        // convert the wxString/size_t variant to wstring/size_t for the extractor
        if (const auto* const worksheetName = std::get_if<wxString>(&worksheet);
            worksheetName != nullptr)
            {
            m_odsTextExtractor(contentXml.c_str(), contentXml.length(), wkData,
                               worksheetName->ToStdWstring());
            if (wkData.empty())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': Unable to find worksheet in ODS workbook."),
                                     *worksheetName)
                        .ToUTF8());
                }
            }
        else if (const auto* const worksheetIndex = std::get_if<size_t>(&worksheet);
                 worksheetIndex != nullptr)
            {
            if (*worksheetIndex == 0 ||
                *worksheetIndex > m_odsTextExtractor.get_worksheet_names().size())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"Worksheet '%zu': worksheet out of range in ODS workbook."),
                                     *worksheetIndex)
                        .ToUTF8());
                }
            m_odsTextExtractor(contentXml.c_str(), contentXml.length(), wkData, *worksheetIndex);
            }
        else
            {
            throw std::runtime_error(_(L"Unknown value specified for ODS worksheet.").ToUTF8());
            }

        return lily_of_the_valley::ods_extract_text::get_worksheet_text(wkData, delimiter);
        }
    } // namespace Wisteria::Data
