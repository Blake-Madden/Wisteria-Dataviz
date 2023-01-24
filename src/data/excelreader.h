/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_EXCEL_H__
#define __WISTERIA_EXCEL_H__

#include <variant>
#include <wx/wx.h>
#include <wx/string.h>
#include "../import/xlsx_extract_text.h"
#include "../util/zipcatalog.h"
#include "../util/memorymappedfile.h"

namespace Wisteria::Data
    {
    /// @brief Interface for reading an Excel 2007+ (XLSX) spreadsheet.
    /// @details This is a wrapper for lily_of_the_valley::xlsx_extract_text.
    class ExcelReader
        {
    public:
        /// @brief Constructor.
        /// @param filePath The path to the Excel file to load.
        explicit ExcelReader(const wxString& filePath) : m_filePath(filePath)
            { LoadFile(filePath); }
        /// @brief Loads an Excel file.
        /// @param filePath The path to the Excel file to load.
        void LoadFile(const wxString& filePath);
        /** @returns The list of worksheet names in the Excel file.*/
        [[nodiscard]]
        const std::vector<std::wstring>& GetWorksheetNames() const noexcept
            { return m_xlsxTextExtractor.get_worksheet_names(); }
        /** @brief Reads a worksheet from the loaded workbook.
            @param worksheet The name or 1-based index of the worksheet to read.
            @param delimiter The charater to delimit the columns with.
            @returns The worksheet data, delimited as text.
            @throws std::runtime_error If the worksheet can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        wxString ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
            const wchar_t delimiter = L'\t');
    private:
        wxString m_filePath;
        lily_of_the_valley::xlsx_extract_text m_xlsxTextExtractor;
        };
    }

/** @}*/

#endif //__WISTERIA_EXCEL_H__
