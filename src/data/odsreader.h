/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_ODS_H
#define WISTERIA_ODS_H

#include "../import/ods_extract_text.h"
#include "../util/zipcatalog.h"
#include <variant>
#include <wx/string.h>

namespace Wisteria::Data
    {
    /// @brief Interface for reading an ODS (OpenDocument Spreadsheet) file.
    /// @details This is a wrapper for lily_of_the_valley::ods_extract_text.
    class OdsReader
        {
      public:
        /// @brief Constructor.
        /// @param filePath The path to the ODS file to load.
        explicit OdsReader(wxString filePath) : m_filePath(std::move(filePath))
            {
            LoadFile(m_filePath);
            }

        /// @brief Loads an ODS file.
        /// @param filePath The path to the ODS file to load.
        void LoadFile(const wxString& filePath);

        /** @returns The list of worksheet names in the ODS file.*/
        [[nodiscard]]
        std::vector<std::wstring> GetWorksheetNames() const
            {
            return m_odsTextExtractor.get_worksheet_names();
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

      private:
        /// @brief Loads and parses a worksheet from `content.xml` into a cell matrix.
        /// @param worksheet The name or 1-based index of the worksheet to read.
        /// @returns The parsed worksheet.
        /// @throws std::runtime_error If the worksheet can't be found or the index is
        ///     out of range.
        [[nodiscard]]
        lily_of_the_valley::ods_extract_text::worksheet
        ReadWorksheetData(const std::variant<wxString, size_t>& worksheet);

        wxString m_filePath;
        lily_of_the_valley::ods_extract_text m_odsTextExtractor{ true };
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_ODS_H
