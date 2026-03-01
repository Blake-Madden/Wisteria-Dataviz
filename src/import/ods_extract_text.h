/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef ODS_TEXT_EXTRACT_H
#define ODS_TEXT_EXTRACT_H

#include "spreadsheet_extract_text.h"
#include <variant>

namespace lily_of_the_valley
    {
    /// @brief Class to extract text from an ODS (OpenDocument Spreadsheet) stream
    ///     (specifically, the "content.xml" file).
    /// @details ODS files are ZIP archives. The cell data, worksheet names, and
    ///     inline strings are all stored in a single "content.xml" file.
    class ods_extract_text : public spreadsheet_extract_text
        {
      public:
        /** @brief Constructor.
            @param removeNewlinesAndTabs Set to @c true to replace any newlines or tabs in cells'
                text with spaces.\n This is recommended @c true (the default) for traditional data,
                where there may be column headers with newlines that you wish to "clean."\n
                This is also recommended @c true if you plan to export the file as text later or
                when calling get_worksheet_text().\n This is recommended @c false if cells
                represent complex text and tabs and newlines should be preserved.
                For this situation, call get_cell_text() instead of get_worksheet_text()
                to read the text as expected.*/
        explicit ods_extract_text(const bool removeNewlinesAndTabs)
            : spreadsheet_extract_text(removeNewlinesAndTabs)
            {
            }

        /** @brief Reads the worksheet names from the "content.xml" file.
            @param text The content of "content.xml".
            @param text_length The length of the text.*/
        void read_worksheet_names(const wchar_t* text, size_t text_length);

        /** @brief Main interface for extracting data from an ODS worksheet.
            @note Call read_worksheet_names() beforehand so that the
                worksheet names are available.
            @param text The "content.xml" text to parse. "content.xml" is extracted from
                an ODS file, which is a ZIP archive.
            @param text_length The length of the text.
            @param[out] data The data matrix (worksheet) to copy the sheet data into.
            @param worksheet The worksheet to extract, identified by name or 1-based index.
            @sa get_worksheet_text().*/
        void operator()(const wchar_t* text, size_t text_length, worksheet& data,
                        const std::variant<std::wstring, size_t>& worksheet);

        /** @returns The list of worksheet names in the ODS file.
            @note You must call read_worksheet_names() first to load these names.*/
        [[nodiscard]]
        std::vector<std::wstring> get_worksheet_names() const
            {
            return m_worksheet_names;
            }

#ifndef __UNITTEST
      private:
#endif
        std::vector<std::wstring> m_worksheet_names;

        html_extract_text m_html_extract;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // ODS_TEXT_EXTRACT_H
