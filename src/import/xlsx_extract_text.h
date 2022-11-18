/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __XLSX_TEXT_EXTRACT_H__
#define __XLSX_TEXT_EXTRACT_H__

#include <cmath>
#include <vector>
#include <sstream>
#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /// @brief Functor that accepts the sharedStrings.xml from a Microsoft® Excel (2007+) file
    ///     and returns the string table item in it, one-by-one.
    class xlsx_string_table_parse
        {
    public:
        /** @brief Constructor.
            @param html_text The HTML text to analyze.
            @param length The length of html_text.*/
        xlsx_string_table_parse(const wchar_t* html_text, const size_t length) noexcept :
                m_html_text(html_text), m_html_text_end(html_text+length)
            {}
        /** @brief Main function that returns the next string in the file.
            @returns True and the string if another item is found, false and empty string otherwise.*/
        [[nodiscard]] const std::pair<bool,std::wstring> operator()();
    private:
        /// @brief Sets the internal text pointer to null.
        /// @returns False and empty string to indicate that the parsing is done.
        [[nodiscard]] std::pair<bool,std::wstring> return_finished() noexcept
            {
            m_html_text = nullptr;
            return std::pair<bool,std::wstring>(false,L"");
            }
        const wchar_t* m_html_text{ nullptr };
        const wchar_t* const m_html_text_end{ nullptr };
        html_extract_text m_html_extract;
        };

    /// @brief Class to extract text from an Excel stream (specifically, the sheet[PAGE].xml files).
    class xlsx_extract_text
        {
    public:
        /// The maximum number of rows an Excel file can have (Excel 2013 specs, http://office.microsoft.com/en-us/excel-help/excel-specifications-and-limits-HA103980614.aspx).
        static constexpr size_t ExcelMaxRows = 1048576; //1024*1024
        /// The maximum number of columns an Excel file can have (Excel 2013 specs, http://office.microsoft.com/en-us/excel-help/excel-specifications-and-limits-HA103980614.aspx).
        static constexpr size_t ExcelMaxColumns = 16384; //1024*16
        /// A cell in an Excel file, which stores the position of the cell and if a string then index into the
        /// Excel file's string table (which would be invalid_index [-1] if a numeric value).
        class text_cell
            {
        public:
            /// @private
            static constexpr size_t invalid_index = static_cast<size_t>(-1);
            /// @brief Default constructor.
            text_cell() noexcept : m_column_position(invalid_index), m_row_position(invalid_index), m_string_table_index(invalid_index) {}
            /// @brief Constructor that accepts the @c of the cell. @sa set_name().
            /// @param name The name for the cell (e.g., "D7").
            text_cell(const std::wstring& name) :
                m_column_position(invalid_index), m_row_position(invalid_index), m_string_table_index(invalid_index)
                { set_name(name); }
            /** @brief Constructor.
                @param column The (1-based) column index of the cell.
                @param row The (1-based) row index of the cell.
                @param string_table_index The cell's (0-based) index into the master string table (not used is not a text cell).*/
            text_cell(const size_t column, const size_t row, const size_t string_table_index = invalid_index) noexcept :
                m_column_position(column), m_row_position(row), m_string_table_index(string_table_index)
                {}
            /// @returns True if @c that cell's column comes before this cell's column.
            /// @param that The other cell to compare against.
            /// If they are in the same column, then their rows are compared.
            [[nodiscard]] bool operator<(const text_cell& that) const noexcept
                {
                return (m_column_position < that.m_column_position) ? true :
                    (m_column_position > that.m_column_position) ? false :
                    (m_row_position < that.m_row_position);
                }
            /// @returns True if @c that cell is at the same column and row as this.
            /// @param that The other cell to compare against.
            [[nodiscard]] bool operator==(const text_cell& that) const noexcept
                { return (m_column_position ==  that.m_column_position) && (m_row_position == that.m_row_position); }
            /// @returns True if @c that cell is at a different place from this one.
            /// @param that The other cell to compare against.
            [[nodiscard]] bool operator!=(const text_cell& that) const noexcept
                { return (m_column_position !=  that.m_column_position) || (m_row_position != that.m_row_position); }
            /// @brief Sets the @c column and @c row indices for the cell.
            /// @param column The column index.
            /// @param row The row index.
            void set_column_and_row(const size_t column, const size_t row) noexcept
                {
                m_column_position = column;
                m_row_position = row;
                }
            /// @brief Sets the @c name of the cell, which will internally be stored as column and row indices.
            /// @param name The name for the cell.
            void set_name(const std::wstring& name)
                {
                const std::pair<column_info,size_t> cellInfo = get_column_and_row_info(name.c_str());
                m_column_position = cellInfo.first.m_position;
                m_row_position = cellInfo.second;
                }
            /// @returns The name of the cell (e.g., "D7").
            [[nodiscard]] std::wstring get_name() const
                {
                wchar_t cellNumber[24];
                if (string_util::itoa(m_row_position, cellNumber, 24) == -1)
                    { return L""; }
                return column_index_to_column_name(m_column_position)+cellNumber;
                }
            /// @brief Sets the (0-based) index into the string table that represents the value of this cell.
            /// @param index The index into the string table for this cell.
            void set_string_table_index(const size_t index) noexcept
                { m_string_table_index = index; }
            /// @returns The (0-based) index into the string table that represents this
            /// cell's string value. If this cell is numeric, then will return invalid_index;
            [[nodiscard]] size_t get_string_table_index() const noexcept
                { return m_string_table_index; }
        private:
            size_t m_column_position{ 0 }; //1-indexed, 'A' is column 1
            size_t m_row_position{ 0 }; //1-indexed, rows are referenced this way in the Excel file
            size_t m_string_table_index{ 0 }; //0-indexed, that's how it's used internally in the Excel file
            };

        /// @brief Information about a column, including its (1-based) position in the worksheet.
        /// @private
        struct column_info
            {
            static constexpr size_t invalid_position = static_cast<size_t>(-1);
            column_info() noexcept : m_position(invalid_position) {}
            column_info(size_t position) noexcept : m_position(position) {}
            size_t m_position{ invalid_position };
            };

        /// A row of cells.
        using cell_row = std::vector<text_cell>;
        /// A matrix of cells (i.e., a table or worksheet).
        using worksheet = std::vector<cell_row>;
        /// The table which stores the unique strings throughout the Excel file.
        using string_table = std::vector<std::wstring> ;

        /** @brief Retrieves the text from the given cell.
            @param cellNname The cell to retrieve the text from (e.g., "A13").
            @param workSheet The worksheet to read the text from.
            @returns The cell's textual content.*/
        [[nodiscard]] std::wstring get_cell_text(const wchar_t* cellNname, const worksheet& workSheet);

        /** @brief Retrieves the text from the given cell, given the specified shared strings
                file and worksheet.
            @details This is useful for when you just need one cell's text,
                without having to read in the entire string table.
            @param cell_name The cell to retrieve the text from (e.g., "A13").
            @param shared_strings The content of sharedStrings.xml.
            @param shared_strings_length The length of sharedStrings.xml.
            @param worksheet_text The content of sheet[n].xml.
            @param worksheet_length The length of sheet[n].xml.
            @returns The cell's textual content.*/
        [[nodiscard]] static std::wstring get_cell_text(const wchar_t* cell_name,
                                          const wchar_t* shared_strings, const size_t shared_strings_length,
                                          const wchar_t* worksheet_text, const size_t worksheet_length);

        /** @brief Retrieves a list of cells in the worksheet that has text in them.
            @param wrk The worksheet to review.
            @param cells Where to save the list of cells (by name) that have textual content.*/
        static void get_text_cell_names(const worksheet& wrk, std::vector<std::wstring>& cells);

        /** @brief Main interface for extracting plain text from an Excel worksheet.
            @note Call read_shared_strings() beforehand so that the string table for the
                worksheet is loaded.
            @param html_text The sheet[PAGE].xml text to strip. sheet[PAGE].xml is extracted from
                an XLSX file, which is a zip file.
            @param text_length The length of the text.
            @param data The data matrix to copy the sheet text into.*/
        void operator()(const wchar_t* html_text,
                        const size_t text_length,
                        worksheet& data);

        /** @brief Retrieves the string table from the sharedStrings.xml file.
            @param text The content of sharedStrings.xml.
            @param text_length The length of the text.
            @param truncate Whether to truncate the labels to 256 characters.\n
                This is useful if doing a preview of the file nd you don't need the
                full text of each cell loaded into memory.*/
        void read_shared_strings(const wchar_t* text, const size_t text_length,
                                 const bool truncate = false);

        /** @brief Retrieves the worksheet names from the workbook.xml file.
            @param text The content of workbook.xml.
            @param text_length The length of the text.*/
        void read_worksheet_names(const wchar_t* text, const size_t text_length);

        /** @returns The list of worksheet names in the Excel file.
            @note You much call read_worksheet_names() first to load these names.*/
        [[nodiscard]] const std::vector<std::wstring>& get_worksheet_names() const noexcept
            { return m_worksheet_names; }

        /** @returns The string from the specified index.
                This assumes that the string table has already been loaded via read_shared_strings().
            @param index The (zero-based) index into the string table.*/
        [[nodiscard]] std::wstring get_shared_string(const size_t index) const
            {
            return (index < get_shared_strings().size()) ?
                get_shared_strings().at(index) : L"";
            }

        /** @brief Verifies that a worksheet isn't jagged and the cells are in the proper order.
            @note This is expensive and should only be used for debugging purposes.
            @param data The worksheet to verify.
            @returns a boolean indicating whether the sheet is OK and upon failure the name of
                the first cell that was missing or out of order.*/
        [[nodiscard]] static std::pair<bool,std::wstring> verify_sheet(const worksheet& data);
#ifndef __UNITTEST
    private:
#endif
        /** @returns The column name from a column index (1-indexed).
            @param col The column number.
            @returns The name of the column. For example, 30 will return "AD").*/
        [[nodiscard]] static std::wstring column_index_to_column_name(size_t col);
        /** Parses a cell reference and returns the column name, column number (1-indexed), and row number (1-indexed).
            @param cell_name The name of the cell to parse (e.g., "AA23").
            @returns The column name, column number (1-indexed), and row number (1-indexed.
             Row and/or column numbers will be -1 if name format is wrong.
             For example, "A7" will return "A", 1 (column 1), and 7 (row 7).*/
        [[nodiscard]] static std::pair<column_info,size_t> get_column_and_row_info(const wchar_t* cell_name);

        /** Adds cells to rows with missing cells. This may happen with files missing its
            dimension specifications and has missing cells in its data section. This should
            never happen, but just in case we are working with a very ill-formed file we
            will try to fix it as best we can.
            @param data the Worksheet to review.*/
        static void fix_jagged_sheet(worksheet& data);

        /** Retrieves a string from the sharedStrings.xml file. This is meant for just finding one string
             when it is needed without having to load the entire string table.
            @param index The (zero-based) index of the string to get from the string table.
            @param text The content of sharedStrings.xml.
            @param text_length The length of the text.
            @returns The string at the given index of the string table, or empty string if not found.*/
        [[nodiscard]] static std::wstring get_shared_string(const size_t index, const wchar_t* text, const size_t text_length);

        /** @brief Splits a cell name into the column name and row number.
            @param cell_name The cell name (e.g., "A2").
            @returns a pair representing where the row number begins in the string (parsing from the
             start of the cell name up to this index yields the column name) and the row number.
             The first index will be set to -1 upon failure.*/
        [[nodiscard]] static std::pair<size_t,size_t> split_column_info(const wchar_t* cell_name);

        /// @returns The string table.
        [[nodiscard]] const string_table& get_shared_strings() const noexcept
            { return m_shared_strings; }

        std::vector<std::wstring> m_worksheet_names;
        string_table m_shared_strings;
        };
    }

/** @}*/

#endif //__XLSX_TEXT_EXTRACT_H__
