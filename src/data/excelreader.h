/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_EXCEL_H
#define WISTERIA_EXCEL_H

#include "../import/xlsx_extract_text.h"
#include "../util/zipcatalog.h"
#include <ranges>
#include <variant>
#include <wx/string.h>

namespace Wisteria::Data
    {
    /// @brief Interface for reading an Excel 2007+ (XLSX) spreadsheet.
    /// @details This is a wrapper for lily_of_the_valley::xlsx_extract_text.
    class ExcelReader
        {
      public:
        /// @brief Constructor.
        /// @param filePath The path to the Excel file to load.
        explicit ExcelReader(wxString filePath) : m_filePath(std::move(filePath))
            {
            LoadFile(m_filePath);
            }

        /// @brief Loads an Excel file.
        /// @param filePath The path to the Excel file to load.
        void LoadFile(const wxString& filePath);

        /** @returns The list of worksheet names in the Excel file.*/
        [[nodiscard]]
        std::vector<std::wstring> GetWorksheetNames() const
            {
            return m_xlsxTextExtractor.get_worksheet_names();
            }

        /** @brief Reads a worksheet from the loaded workbook.
            @param worksheet The name or 1-based index of the worksheet to read.
            @param delimiter The character to delimit the columns with.
            @returns The worksheet data, delimited as text.
            @throws std::runtime_error If the worksheet can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        wxString ReadWorksheet(const std::variant<wxString, size_t>& worksheet,
                               wchar_t delimiter = L'\t');

        /** @brief Reads a worksheet from the loaded workbook as a matrix of strings.
            @param worksheet The name or 1-based index of the worksheet to read.
            @returns The worksheet's cells, row by row, with each cell's content preserved
                exactly (no delimiting, quoting, or trimming).
            @throws std::runtime_error If the worksheet can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        std::vector<std::vector<std::wstring>>
        ReadWorksheetMatrix(const std::variant<wxString, size_t>& worksheet);

        /// @brief Both representations of a worksheet, as read by ReadWorksheetContent().
        struct WorksheetContent
            {
            /// @brief The worksheet data, delimited as text.
            wxString m_text;
            /// @brief The worksheet's cells, row by row.
            std::vector<std::vector<std::wstring>> m_matrix;
            };

        /** @brief Reads a worksheet as both delimited text and a cell matrix.
            @details Prefer this over calling ReadWorksheet() and ReadWorksheetMatrix()
                separately when both forms are needed (e.g., deducing column types from
                the text form and then importing from the matrix form), as this only
                parses the worksheet's XML once.
            @param worksheet The name or 1-based index of the worksheet to read.
            @param delimiter The character to delimit the columns with (for the text form).
            @returns The worksheet's text and matrix representations.
            @throws std::runtime_error If the worksheet can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        WorksheetContent ReadWorksheetContent(const std::variant<wxString, size_t>& worksheet,
                                              wchar_t delimiter = L'\t');

      private:
        /// @brief Loads and parses a worksheet's XML into a cell matrix.
        /// @param worksheet The name or 1-based index of the worksheet to read.
        /// @returns The parsed worksheet.
        /// @throws std::runtime_error If the worksheet can't be found or the index is
        ///     out of range.
        [[nodiscard]]
        lily_of_the_valley::xlsx_extract_text::worksheet
        ReadWorksheetData(const std::variant<wxString, size_t>& worksheet);

        wxString m_filePath;
        lily_of_the_valley::xlsx_extract_text m_xlsxTextExtractor{ true };
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_EXCEL_H
