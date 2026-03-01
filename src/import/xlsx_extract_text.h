/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef XLSX_TEXT_EXTRACT_H
#define XLSX_TEXT_EXTRACT_H

#include "spreadsheet_extract_text.h"
#include <ranges>

namespace lily_of_the_valley
    {
    /// @brief Functor that accepts the sharedStrings.xml from a Microsoft® *Excel* (2007+) file
    ///     and returns the string table item in it, one-by-one.
    class xlsx_string_table_parse
        {
      public:
        /** @brief Constructor.
            @param html_text The HTML text to analyze.
            @param length The length of html_text.
            @param removeNewlinesAndTabs @c true to replace any newlines or tabs in
                cells' text with spaces.\n
                This is recommended @c true (the default) for traditional data, where there may be
                column headers with newlines that you wish to "clean."\n
                This is recommended @c false if cells represent complex text and tabs and
                newlines should be preserved.*/
        xlsx_string_table_parse(const wchar_t* html_text, const size_t length,
                                const bool removeNewlinesAndTabs)
            : m_html_text(html_text), m_html_text_end(html_text + length),
              m_removeNewlinesAndTabs(removeNewlinesAndTabs)
            {
            }

        /** @brief Main function that returns the next string in the file.
            @returns @c true and the string if another item is found,
                @c false and empty string otherwise.*/
        [[nodiscard]]
        std::pair<bool, std::wstring> operator()();

      private:
        /// @brief Sets the internal text pointer to null.
        /// @returns @c false and empty string to indicate that the parsing is done.
        [[nodiscard]]
        std::pair<bool, std::wstring> return_finished() noexcept
            {
            m_html_text = nullptr;
            return std::make_pair(false, std::wstring{});
            }

        const wchar_t* m_html_text{ nullptr };
        const wchar_t* const m_html_text_end{ nullptr };
        html_extract_text m_html_extract;
        bool m_removeNewlinesAndTabs{ true };
        };

    /// @brief Class to extract text from an Excel stream
    ///     (specifically, the sheet[PAGE].xml files).
    /// @details References:\n
    ///     \n
    ///     https://www.brendanlong.com/the-minimum-viable-xlsx-reader.html\n
    ///     https://github.com/brendanlong/ocaml-ooxml\n
    ///     https://support.microsoft.com/en-us/office/excel-specifications-and-limits-1672b34d-7043-467e-8e27-269d656771c3
    class xlsx_extract_text : public spreadsheet_extract_text
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
        explicit xlsx_extract_text(const bool removeNewlinesAndTabs)
            : spreadsheet_extract_text(removeNewlinesAndTabs)
            {
            }

        /// The maximum number of rows an Excel file can have
        constexpr static size_t EXCEL_MAX_ROWS = 1'048'576; // 1024 * 1024;
        /// The maximum number of columns an Excel file can have
        constexpr static size_t EXCEL_MAX_COLUMNS = 16'384; // 1024 * 16;

        /** @brief Retrieves the text from a given cell, given the specified shared strings
                file and worksheet.
            @details This is useful for when you just need one cell's text,
                without having to read in the entire string table.
            @param cell_name The cell to retrieve the text from (e.g., "A13").
            @param shared_strings The content of `xl/sharedStrings.xml`.
            @param shared_strings_length The length of `xl/sharedStrings.xml`.
            @param worksheet_text The content of sheet[n].xml.
            @param worksheet_length The length of sheet[n].xml.
            @returns The cell's textual content.*/
        [[nodiscard]]
        std::wstring get_cell_text(const wchar_t* cell_name, const wchar_t* shared_strings,
                                   size_t shared_strings_length, const wchar_t* worksheet_text,
                                   size_t worksheet_length) const;

        /** @brief Main interface for extracting plain text from an Excel worksheet.
            @note Call read_shared_strings() and read_styles() beforehand so that the
                string table and number formats for the worksheet are loaded.
            @param html_text The sheet[PAGE].xml text to strip. sheet[PAGE].xml is extracted from
                an XLSX file, which is a zip file.
            @param text_length The length of the text.
            @param[out] data The data matrix (worksheet) to copy the sheet text into.
            @sa get_worksheet_text().*/
        void operator()(const wchar_t* html_text, size_t text_length, worksheet& data);

        /** @brief Retrieves the string table from the `xl/sharedStrings.xml` file.
            @param text The content of `xl/sharedStrings.xml`.
            @param text_length The length of the text.
            @param truncate Whether to truncate the labels to 256 characters.\n
                This is useful if doing a preview of the file and you don't need the
                full text of each cell loaded into memory.*/
        void read_shared_strings(const wchar_t* text, size_t text_length, bool truncate = false);

        /** @brief Retrieves the number formatting styles from the `xl/styles.xml` file.
            @param text The content of `xl/styles.xml`.
            @param text_length The length of the text.*/
        void read_styles(const wchar_t* text, size_t text_length);

        /** @brief Retrieves the worksheet names from the `xl/workbook.xml` file.
            @param text The content of `xl/workbook.xml`.
            @param text_length The length of the text.*/
        void read_worksheet_names(const wchar_t* text, size_t text_length);

        /** @brief Parses the workbook relationships file for worksheet target paths.
            @details Reads the contents of `xl/_rels/workbook.xml.rels` and extracts the
                relationship ID and target path pairs defined by each
                `<Relationship>` element. The parsed results are stored internally
                and can be accessed via get_relative_paths().
            @param xml A pointer to the XML text to parse.
            @param length The length of the XML text, in characters.
            @note Call this after read_worksheet_names() and then call map_workbook_paths().*/
        void read_relative_paths(const wchar_t* xml, const size_t length);

        /** @brief Resolves worksheet names to their XML file paths.
            @details Merges worksheet name and relationship information parsed from
                `workbook.xml` and `workbook.xml.rels` to produce an ordered list
                of worksheet XML paths relative to the `xl/` directory.
            @note Call this after read_worksheet_names() and read_relative_paths().*/
        void map_workbook_paths();

        /** @returns The list of worksheet names and respective relative ID in the Excel file.
            @note You must call read_worksheet_names() first to load these names.*/
        [[nodiscard]]
        const std::vector<std::pair<std::wstring, std::wstring>>&
        get_worksheet_names_and_ids() const noexcept
            {
            return m_worksheet_names_and_ids;
            }

        /** @returns The list of worksheet names in the Excel file.
            @note You must call read_worksheet_names() first to load these names.*/
        [[nodiscard]]
        std::vector<std::wstring> get_worksheet_names() const
            {
            const auto names =
                m_worksheet_paths | std::views::transform([](const auto& ws) { return ws.first; });

            return { names.begin(), names.end() };
            }

        /** @brief Gets the relative path mappings defined in the workbook relationships file.
            @details Returns the mapping of relationship IDs to their corresponding target
                paths as defined in `xl/_rels/workbook.xml.rels`. These paths are
                relative to the `xl/` directory in the XLSX archive.
            @returns A map of relationship IDs to their relative target paths.*/
        [[nodiscard]]
        const std::unordered_map<std::wstring, std::wstring>& get_relative_paths() const noexcept
            {
            return m_relative_paths;
            }

        /** @brief Gets the resolved worksheet names and their file paths within the archive.
            @returns An ordered list of worksheet names paired with their
                corresponding XML file paths. The order of the entries matches the
                worksheet order defined in `workbook.xml`.*/
        const std::vector<std::pair<std::wstring, std::wstring>>&
        get_worksheet_paths() const noexcept
            {
            return m_worksheet_paths;
            }

        /** @brief Converts an Excel serial date to day, month, and year values.
            @param nSerialDate The serial date to split into DMY components.
            @param[out] nDay The day from the serial value.
            @param[out] nMonth The month from the serial value.
            @param[out] nYear The year from the serial value.
            @warning This uses the Excel leap year bug calculation.
                (Technically, this was a Lotus 1-2-3 bug that they emulated for compatibility.)
                Values before 2/2/1990 will be one off compared to LibreOffice,
                    Google Sheets, and others.
            @details Source:
            https://www.codeproject.com/Articles/2750/Excel-Serial-Date-to-Day-Month-Year-and-Vice-Versa*/
        static void excel_serial_date_to_dmy(int nSerialDate, int& nDay, int& nMonth, int& nYear);

        /** @brief Converts day, month, and year values into an *Excel* serial date.
            @returns The date converted into an Excel serial date.
            @param nDay The day.
            @param nMonth The month.
            @param nYear The year.
            @warning This uses the Excel leap year bug calculation. Values before 2/2/1990 will be
                one off compared to LibreOffice, Google Sheets, and others.
            @details Source:
            https://www.codeproject.com/Articles/2750/Excel-Serial-Date-to-Day-Month-Year-and-Vice-Versa*/
        [[nodiscard]]
        static int dmy_to_excel_serial_date(int nDay, int nMonth, int nYear);

#ifndef __UNITTEST
      private:
#endif
        /** @returns The string from the specified index.
            @warning This assumes that the string table has already been
                loaded via read_shared_strings().
            @param index The (zero-based) index into the string table.*/
        [[nodiscard]]
        std::wstring get_shared_string(const size_t index) const
            {
            return (index < get_shared_strings().size()) ? get_shared_strings()[index] :
                                                           std::wstring();
            }

        /** @brief Retrieves a string from the `xl/sharedStrings.xml` file.
            @details This is meant for just finding one string when it is needed
                without having to load the entire string table.
            @param index The (zero-based) index of the string to get from the string table.
            @param text The content of `xl/sharedStrings.xml`.
            @param text_length The length of the text.
            @returns The string at the given index of the string table,
                or empty string if not found.*/
        [[nodiscard]]
        std::wstring get_shared_string(size_t index, const wchar_t* text, size_t text_length) const;

        /// @returns The string table.
        [[nodiscard]]
        const string_table& get_shared_strings() const noexcept
            {
            return m_shared_strings;
            }

        // name & relative ID (workbook.xml)
        std::vector<std::pair<std::wstring, std::wstring>> m_worksheet_names_and_ids;
        // relative ID & its path (workbook.xml.rels)
        std::unordered_map<std::wstring, std::wstring> m_relative_paths;
        // name & path
        std::vector<std::pair<std::wstring, std::wstring>> m_worksheet_paths;
        // style indices that use a date format
        std::set<size_t> m_date_format_indices;
        string_table m_shared_strings;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // XLSX_TEXT_EXTRACT_H
