/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef SPREADSHEET_TEXT_EXTRACT_H
#define SPREADSHEET_TEXT_EXTRACT_H

#include "html_extract_text.h"
#include <sstream>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief Base class for spreadsheet text extractors (e.g., XLSX, ODS).
    /// @details Provides shared types (worksheet_cell, worksheet, etc.)
    ///     and static utility methods for working with cell-based data.
    class spreadsheet_extract_text
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
        explicit spreadsheet_extract_text(const bool removeNewlinesAndTabs)
            : m_removeNewlinesAndTabs(removeNewlinesAndTabs)
            {
            }

        /// @brief Destructor.
        virtual ~spreadsheet_extract_text() = default;

        /// A cell in a spreadsheet, which stores the position of the cell
        ///     and its string value.
        class worksheet_cell
            {
          public:
            /// @private
            constexpr static size_t INVALID_INDEX = static_cast<size_t>(-1);
            /// @private
            worksheet_cell() = default;

            /// @brief Constructor that accepts the name of the cell. @sa set_name().
            /// @param name The name for the cell (e.g., "D7").
            explicit worksheet_cell(const std::wstring& name) { set_name(name); }

            /** @brief Constructor.
                @param column The (1-based) column index of the cell.
                @param row The (1-based) row index of the cell.
                @param value The cell's value.*/
            worksheet_cell(const size_t column, const size_t row,
                           std::wstring value = std::wstring{}) noexcept
                : m_column_position(column), m_row_position(row), m_value(std::move(value))
                {
                }

            /// @returns @c true if @c that cell's column comes before this cell's column.
            /// @param that The other cell to compare against.\n
            ///     If they are in the same column, then their rows are compared.
            [[nodiscard]]
            bool operator<(const worksheet_cell& that) const noexcept
                {
                return (m_column_position < that.m_column_position) ?
                           true :
                       (m_column_position > that.m_column_position) ?
                           false :
                           (m_row_position < that.m_row_position);
                }

            /// @returns @c true if @c that cell is at the same column and row as this.
            /// @param that The other cell to compare against.
            [[nodiscard]]
            bool operator==(const worksheet_cell& that) const noexcept
                {
                return (m_column_position == that.m_column_position) &&
                       (m_row_position == that.m_row_position);
                }

            /// @returns @c true if @c that cell is at a different place from this one.
            /// @param that The other cell to compare against.
            [[nodiscard]]
            bool operator!=(const worksheet_cell& that) const noexcept
                {
                return (m_column_position != that.m_column_position) ||
                       (m_row_position != that.m_row_position);
                }

            /// @brief Sets the @c column and @c row indices for the cell.
            /// @param column The column index.
            /// @param row The row index.
            void set_column_and_row(const size_t column, const size_t row) noexcept
                {
                m_column_position = column;
                m_row_position = row;
                }

            /// @brief Sets the @c name of the cell, which will internally be
            ///     stored as column and row indices.
            /// @param name The name for the cell.
            void set_name(const std::wstring& name)
                {
                const auto cellInfo = get_column_and_row_info(name.c_str());
                m_column_position = cellInfo.first.m_position;
                m_row_position = cellInfo.second;
                }

            /// @returns The name of the cell (e.g., "D7").
            [[nodiscard]]
            std::wstring get_name() const
                {
                return column_index_to_column_name(m_column_position) +
                       std::to_wstring(m_row_position);
                }

            /// @returns The (1-based) column position of the cell.
            [[nodiscard]]
            size_t get_column_position() const noexcept
                {
                return m_column_position;
                }

            /// @brief Sets the string value of the cell.
            /// @param value The value for the cell.
            void set_value(const std::wstring& value) { m_value = value; }

            /// @returns The cell's value.
            [[nodiscard]]
            const std::wstring& get_value() const noexcept
                {
                return m_value;
                }

          private:
            // 1-indexed, 'A' is column 1
            size_t m_column_position{ INVALID_INDEX };
            // 1-indexed, rows are referenced this way in the spreadsheet file
            size_t m_row_position{ INVALID_INDEX };
            // the string value in the cell
            std::wstring m_value;
            };

        /// @brief Information about a column, including its (1-based) position in the worksheet.
        /// @private
        struct column_info
            {
            constexpr static size_t INVALID_POSITION = static_cast<size_t>(-1);
            column_info() = default;

            explicit column_info(size_t position) noexcept : m_position(position) {}

            size_t m_position{ INVALID_POSITION };
            };

        /// A row of cells.
        using worksheet_row = std::vector<worksheet_cell>;
        /// A matrix of cells (i.e., a table or worksheet).
        using worksheet = std::vector<worksheet_row>;
        /// The table which stores the unique strings throughout a spreadsheet file.
        using string_table = std::vector<std::wstring>;

        /** @param removeNewlinesAndTabs Set to @c true to replace any newlines or tabs in cells'
                text with spaces.\n
                This is recommended @c true (the default) for traditional
                data, where there may be column headers with newlines that you wish to "clean."\n
                This is also recommended @c true if you plan to export the file as text later
                or when calling get_worksheet_text().\n
                This is recommended @c false if cells represent complex text and tabs and
                newlines should be preserved. For this situation, call get_cell_text() instead of
                get_worksheet_text() to read the text as expected.*/
        void remove_newlines_and_tabs_when_reading(const bool removeNewlinesAndTabs)
            {
            m_removeNewlinesAndTabs = removeNewlinesAndTabs;
            }

        /** @brief Retrieves the text from a given cell.
            @param cellName The cell to retrieve the text from (e.g., "A13").
            @param workSheet The worksheet to read the text from.
            @returns The cell's textual content.*/
        [[nodiscard]]
        static std::wstring get_cell_text(const wchar_t* cellName, const worksheet& workSheet);

        /** @brief Retrieves a list of cells in the worksheet that has text in them.
            @param wrk The worksheet to review.
            @param cells Where to save the list of cells (by name) that have textual content.*/
        static void get_text_cell_names(const worksheet& wrk, std::vector<std::wstring>& cells);

        /** @brief Gets the number of cells in a worksheet.
            @param wrk The worksheet to review.
            @returns The number of cells in the worksheet.*/
        [[nodiscard]]
        static size_t get_cell_count(const worksheet& wrk)
            {
            return (!wrk.empty()) ?
                       wrk.size() /* row count*/ * wrk[0].size() /* column count of first row*/ :
                       0;
            }

        /** @brief Gets the worksheet as delimited text.
            @param wrk The worksheet to format.
            @param delim The delimiter to separate the columns with.
            @returns The worksheet as delimited text.
            @note It is recommended to set remove_newlines_and_tabs_when_reading() to @c true
                before calling this.*/
        [[nodiscard]]
        static std::wstring get_worksheet_text(const worksheet& wrk, wchar_t delim = L'\t');

        /** @brief Verifies that a worksheet isn't jagged and the cells are in the proper order.
            @note This is expensive and should only be used for debugging purposes.
            @param data The worksheet to verify.
            @returns a boolean indicating whether the sheet is OK and upon failure the name of
                the first cell that was missing or out of order.*/
        [[nodiscard]]
        static std::pair<bool, std::wstring> verify_sheet(const worksheet& data);

        /** @brief Sets the string used to separate the messages in the log report.
            @details By default, messages are separated by newlines, so call this to separate them
                by something like commas (if you are needing a single-line report).
            @param separator The separator character to use.*/
        void set_log_message_separator(const std::wstring& separator)
            {
            m_log_message_separator = separator;
            }

        /// @brief Empties the log of any previous parsing issues.
        void clear_log() const { m_log.clear(); }

#ifndef __UNITTEST
      protected:
#endif
        /** @brief Adds a message to the report logging system.
            @param message The message to log.*/
        void log_message(const std::wstring& message) const
            {
            if (m_log.empty())
                {
                m_log.append(message);
                } // first message won't need a separator in front of it
            else
                {
                m_log.append(m_log_message_separator + message);
                }
            }

        /** @returns The column name from a column index (1-indexed).
            @param col The column number.
            @returns The name of the column. For example, @c 30 will return "AD".*/
        [[nodiscard]]
        static std::wstring column_index_to_column_name(size_t col);
        /** @brief Parses a cell reference and returns the column name,
                column number (1-indexed), and row number (1-indexed).
            @param cell_name The name of the cell to parse (e.g., "AA23").
            @returns The column name, column number (1-indexed), and row number (1-indexed).
                Row and/or column numbers will be -1 if name format is wrong.
                For example, "A7" will return "A", 1 (column 1), and 7 (row 7).*/
        [[nodiscard]]
        static std::pair<column_info, size_t> get_column_and_row_info(const wchar_t* cell_name);

        /** @brief Fills in blank cells at any missing column positions across all rows.
            After parsing, rows may be sparse (e.g., a row with data at columns A, C, E
            will be missing B and D). This finds the widest row and ensures every row
            has a cell at every column position, inserting blank cells where needed.
            @param[in,out] data The worksheet to fill in.*/
        static void fill_missing_cells(worksheet& data);

        /** @brief Splits a cell name into the column name and row number.
            @param cell_name The cell name (e.g., "A2").
            @returns a pair representing where the row number begins in the string
                (parsing from the start of the cell name up to this index yields the column name)
                and the row number.\n
                The first index will be set to `std::wstring::npos` upon failure.*/
        [[nodiscard]]
        static std::pair<size_t, size_t> split_column_info(std::wstring_view cell_name);

        /// Whether to replace newlines and tabs when reading cell content.
        bool m_removeNewlinesAndTabs{ true };

        /// @private
        mutable std::wstring m_log;
        /// @private
        std::wstring m_log_message_separator{ L"\n" };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // SPREADSHEET_TEXT_EXTRACT_H
