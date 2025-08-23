/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef TEXT_ROW_H
#define TEXT_ROW_H

#include "text_column.h"
#include <utility>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief Class representing a row of text.
    template<typename string_typeT>
    class text_row
        {
      public:
        /// @brief Constructor.
        /// @param repeat_count How many times this row definition should be repeated by
        ///     the parent parser.\n
        ///     Set to @c std::nullopt to repeat the row until the end of file is reached.
        explicit text_row(const std::optional<size_t> repeat_count = std::nullopt) noexcept
            : m_repeat_count(repeat_count)
            {
            }

        /// @brief Assigns the container of strings that we will write to.
        /// @param values Where the strings will be written to.
        void set_values(std::vector<string_typeT>* values) noexcept
            {
            m_values = values;
            m_single_value = nullptr;
            }

        /// @brief Assigns the string that we will write to (this assumes a single column dataset).
        /// @param value Where the single value will be written to.
        void set_single_value(string_typeT* value) noexcept
            {
            m_single_value = value;
            m_values = nullptr;
            }

        /// @brief Sets whether consecutive column separators (e.g., tabs) should be treated as one.
        ///     If @c true, then three delimiters next to each other would be seen a one delimiter
        ///     and not interpreted as blank columns. If @c false, then this will result in two
        ///     empty columns being read.
        /// @param allow Set to @c true to treat consecutive delimiters as one.
        void treat_consecutive_delimiters_as_one(const bool allow = true) noexcept
            {
            m_treat_consecutive_delimiters_as_one = allow;
            }

        /// @brief Sets whether the row may have an unknown number of columns before parsing.
        /// @param allow @c true enable column resizing for the row.
        ///     If @c true, then the row will dynamically add columns while parsing.
        /// @note This is turned off by default, so the caller should enable this if the data
        ///     may be jagged or column count is unknown.
        void allow_column_resizing(const bool allow = true) noexcept
            {
            m_allow_column_values_resizing = allow;
            }

        /// @returns Whether the row may have an unknown number of columns before parsing.
        [[nodiscard]]
        bool is_column_resizing_enabled() const noexcept
            {
            return m_allow_column_values_resizing;
            }

        /// @brief Adds a column that looks for a standard character delimiter
        ///     (space, semicolon, or comma).
        /// @param column The column definition to add.
        void add_column(text_column<text_column_standard_delimiter_parser>& column)
            {
            m_standard_delimiter_columns.push_back(column);
            m_column_indices.push_back(std::make_pair(column_type::standard_delimiter,
                                                      m_standard_delimiter_columns.size() - 1));
            // if at least one column is set to NOT skip its text then set this flag to true
            if (column.get_parser().is_reading_text())
                {
                m_read_text = true;
                }
            }

        /// @brief Adds a column that looks for a single character delimiter.
        /// @param column The column definition to add.
        void add_column(text_column<text_column_delimited_character_parser>& column)
            {
            m_delimited_character_columns.push_back(column);
            m_column_indices.push_back(std::make_pair(column_type::delimited_character,
                                                      m_delimited_character_columns.size() - 1));
            // if at least one column is set to NOT skip its text then set this flag to true
            if (column.get_parser().is_reading_text())
                {
                m_read_text = true;
                }
            }

        /// @brief Adds a column that looks for a single character delimiter from
        ///     a list of characters.
        /// @param column The column definition to add.
        void add_column(text_column<text_column_delimited_multiple_character_parser>& column)
            {
            m_delimited_multiple_character_columns.push_back(column);
            m_column_indices.push_back(
                std::make_pair(column_type::delimited_multiple_character,
                               m_delimited_multiple_character_columns.size() - 1));
            // if at least one column is set to NOT skip its text then set this flag to true
            if (column.get_parser().is_reading_text())
                {
                m_read_text = true;
                }
            }

        /// @brief Adds a column that simply reads to the end of the line.
        /// @param column The column definition to add.
        void add_column(text_column<text_column_to_eol_parser>& column)
            {
            m_to_eol_columns.push_back(column);
            m_column_indices.push_back(
                std::make_pair(column_type::to_eol, m_to_eol_columns.size() - 1));
            // if at least one column is set to NOT skip its text then set this flag to true
            if (column.get_parser().is_reading_text())
                {
                m_read_text = true;
                }
            }

        /// @brief Adds a column that is of fixed width.
        /// @param column The column definition to add.
        void add_column(text_column<text_column_fixed_parser>& column)
            {
            m_fixed_width_columns.push_back(column);
            m_column_indices.push_back(
                std::make_pair(column_type::fixed_width, m_fixed_width_columns.size() - 1));
            // if at least one column is set to NOT skip its text then set this flag to true
            if (column.get_parser().is_reading_text())
                {
                m_read_text = true;
                }
            }

        /// @returns @c true if any of the row's columns definitions are set to read in data.
        ///     This can be @c false if all column parsers are meant to simply skip, essentially
        ///     reading in nothing for this row.
        [[nodiscard]]
        bool is_reading_text() const noexcept
            {
            return m_read_text;
            }

        /** @brief The main function which reads a block of text and divides it up into columns.
            @param text The text to parse.
            @returns The last position of the text before parsing ended. Usually, this will be the
                end of the file, unless there are undefined columns in the row of text
                and row parser isn't dynamically growing to include them.
            @warning Call set_values() or set_single_value() first to establish a destination for
                writing the columns as they are read in.*/
        const wchar_t* read(const wchar_t* text)
            {
            m_number_of_columns_last_read = 0;
            if (text == nullptr || text[0] == 0)
                {
                return nullptr;
                }
            cell_trim trim;
            cell_collapse_quotes<string_typeT> cellQuoteCollapse;
            const wchar_t* previousPosition{ text };
            const wchar_t* currentPosition{ text };
            size_t currentColumnIndex{ 0 };

            for (auto pos = m_column_indices.cbegin(); pos != m_column_indices.cend(); ++pos)
                {
                if (pos->first == column_type::delimited_character)
                    {
                    auto currentColumnIter = m_delimited_character_columns.begin() + pos->second;
                    // some column definitions may be used more than once for consecutive columns
                    for (size_t i = 0; (!currentColumnIter->get_repeat_count() ?
                                            true :
                                            i < currentColumnIter->get_repeat_count().value());
                         ++i)
                        {
                        /* Make sure we had enough strings to write next column into
                           if this fails (it shouldn't) then the caller failed to resize the string
                           vector properly. By default, we won't resize it for them either, because
                           then the greater text matrix will wind up being a jagged array.
                           Enable column count resizing to change that.*/
                        if (m_values)
                            {
                            if (currentColumnIndex >= m_values->size())
                                {
                                if (is_column_resizing_enabled())
                                    {
                                    m_values->resize(m_values->size() + 1);
                                    }
                                else
                                    {
                                    return currentPosition;
                                    }
                                }
                            }
                        // for optimized single-column reading return once a column is read
                        else if (is_reading_single_column() && currentColumnIndex >= 1)
                            {
                            return currentPosition;
                            }
                        previousPosition = currentPosition;
                        currentPosition = currentColumnIter->read(currentPosition);
                        // if the current row is blank then don't add anything to its columns
                        if (previousPosition == currentPosition && is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            else
                                {
                                return ++currentPosition;
                                }
                            }
                        // if this parser is NOT set to skip the column's text then read it in
                        if (currentColumnIter->get_parser().is_reading_text())
                            {
                            if (currentPosition == nullptr)
                                {
                                previousPosition = trim(previousPosition);
                                // if last read column is blank, then don't record it
                                // and just return
                                if (previousPosition == nullptr)
                                    {
                                    return nullptr;
                                    }
                                if (m_values)
                                    {
                                    m_values->at(currentColumnIndex++).assign(previousPosition);
                                    }
                                else if (is_reading_single_column())
                                    {
                                    m_single_value->assign(previousPosition);
                                    }
                                ++m_number_of_columns_last_read;
                                return nullptr;
                                }
                            previousPosition =
                                trim(previousPosition, currentPosition - previousPosition);
                            if (previousPosition == nullptr)
                                {
                                return nullptr;
                                }
                            if (m_values != nullptr)
                                {
                                m_values->at(currentColumnIndex)
                                    .assign(previousPosition, trim.get_trimmed_string_length());
                                cellQuoteCollapse(m_values->at(currentColumnIndex));
                                ++currentColumnIndex;
                                }
                            else if (is_reading_single_column())
                                {
                                m_single_value->assign(previousPosition,
                                                       trim.get_trimmed_string_length());
                                cellQuoteCollapse(*m_single_value);
                                ++currentColumnIndex;
                                }
                            ++m_number_of_columns_last_read;
                            }
                        if (currentPosition == nullptr)
                            {
                            return nullptr;
                            }
                        if (is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        if (currentPosition[0] == 0)
                            {
                            return nullptr;
                            }
                        // skip the delimiter(s)
                        if (m_treat_consecutive_delimiters_as_one)
                            {
                            while (currentColumnIter->get_parser().is_delimiter(currentPosition[0]))
                                {
                                ++currentPosition;
                                }
                            }
                        else
                            {
                            ++currentPosition;
                            }
                        }
                    }
                if (pos->first == column_type::standard_delimiter)
                    {
                    auto currentColumnIter = m_standard_delimiter_columns.begin() + pos->second;
                    // some column definitions may be used more than once for consecutive columns
                    for (size_t i = 0; (!currentColumnIter->get_repeat_count() ?
                                            true :
                                            i < currentColumnIter->get_repeat_count().value());
                         ++i)
                        {
                        /* Make sure we had enough strings to write next column into
                           if this fails (it shouldn't) then the caller failed to resize the string
                           vector properly. By default, we won't resize it for them either,
                           because then the greater text matrix will wind up being a jagged array.
                           Enable column count resizing to change that.*/
                        if (m_values)
                            {
                            if (currentColumnIndex >= m_values->size())
                                {
                                if (is_column_resizing_enabled())
                                    {
                                    m_values->resize(m_values->size() + 1);
                                    }
                                else
                                    {
                                    return currentPosition;
                                    }
                                }
                            }
                        // for optimized single-column reading return once a column is read
                        else if (is_reading_single_column() && currentColumnIndex >= 1)
                            {
                            return currentPosition;
                            }
                        previousPosition = currentPosition;
                        currentPosition = currentColumnIter->read(currentPosition);
                        // if the current row is blank then don't add anything to its columns
                        if (previousPosition == currentPosition && is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        // if this parser is NOT set to skip the column's text then read it in
                        if (currentColumnIter->get_parser().is_reading_text())
                            {
                            if (currentPosition == nullptr)
                                {
                                previousPosition = trim(previousPosition);
                                // if last read column is blank, then don't record it
                                // and just return
                                if (previousPosition == nullptr)
                                    {
                                    return nullptr;
                                    }
                                if (m_values != nullptr)
                                    {
                                    m_values->at(currentColumnIndex++).assign(previousPosition);
                                    }
                                else if (is_reading_single_column())
                                    {
                                    m_single_value->assign(previousPosition);
                                    }
                                ++m_number_of_columns_last_read;
                                return nullptr;
                                }
                            previousPosition =
                                trim(previousPosition, currentPosition - previousPosition);
                            if (previousPosition == nullptr)
                                {
                                return nullptr;
                                }
                            if (m_values != nullptr)
                                {
                                m_values->at(currentColumnIndex)
                                    .assign(previousPosition, trim.get_trimmed_string_length());
                                cellQuoteCollapse(m_values->at(currentColumnIndex));
                                ++currentColumnIndex;
                                }
                            else if (is_reading_single_column())
                                {
                                m_single_value->assign(previousPosition,
                                                       trim.get_trimmed_string_length());
                                cellQuoteCollapse(*m_single_value);
                                ++currentColumnIndex;
                                }
                            ++m_number_of_columns_last_read;
                            }
                        if (currentPosition == nullptr)
                            {
                            return nullptr;
                            }
                        if (is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        if (currentPosition[0] == 0)
                            {
                            return nullptr;
                            }
                        // skip the delimiter(s)
                        if (m_treat_consecutive_delimiters_as_one)
                            {
                            while (currentColumnIter->get_parser().is_delimiter(currentPosition[0]))
                                {
                                ++currentPosition;
                                }
                            }
                        else
                            {
                            ++currentPosition;
                            }
                        }
                    }
                else if (pos->first == column_type::delimited_multiple_character)
                    {
                    auto currentColumnIter =
                        m_delimited_multiple_character_columns.begin() + pos->second;
                    // some column definitions may be used more than once for consecutive columns
                    for (size_t i = 0; (!currentColumnIter->get_repeat_count() ?
                                            true :
                                            i < currentColumnIter->get_repeat_count().value());
                         ++i)
                        {
                        /* Make sure we had enough strings to write next column into
                           if this fails (it shouldn't) then the caller failed to resize the string
                           vector properly. By default, we won't resize it for them either,
                           because then the greater text matrix will wind up being a jagged array.
                           Enable column count resizing to change that.*/
                        if (m_values != nullptr)
                            {
                            if (currentColumnIndex >= m_values->size())
                                {
                                if (is_column_resizing_enabled())
                                    {
                                    m_values->resize(m_values->size() + 1);
                                    }
                                else
                                    {
                                    return currentPosition;
                                    }
                                }
                            }
                        // for optimized single-column reading return once a column is read
                        else if (is_reading_single_column() && currentColumnIndex >= 1)
                            {
                            return currentPosition;
                            }
                        previousPosition = currentPosition;
                        currentPosition = currentColumnIter->read(currentPosition);
                        // if the current row is blank then don't add anything to its columns
                        if (previousPosition == currentPosition && is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        // if this parser is NOT set to skip the column's text, then read it in
                        if (currentColumnIter->get_parser().is_reading_text())
                            {
                            if (currentPosition == nullptr)
                                {
                                previousPosition = trim(previousPosition);
                                if (previousPosition == nullptr)
                                    {
                                    return nullptr;
                                    }
                                if (m_values != nullptr)
                                    {
                                    m_values->at(currentColumnIndex++).assign(previousPosition);
                                    }
                                else if (is_reading_single_column())
                                    {
                                    m_single_value->assign(previousPosition);
                                    }
                                ++m_number_of_columns_last_read;
                                return nullptr;
                                }
                            previousPosition =
                                trim(previousPosition, currentPosition - previousPosition);
                            if (previousPosition == nullptr)
                                {
                                return nullptr;
                                }
                            if (m_values != nullptr)
                                {
                                m_values->at(currentColumnIndex)
                                    .assign(previousPosition, trim.get_trimmed_string_length());
                                cellQuoteCollapse(m_values->at(currentColumnIndex));
                                ++currentColumnIndex;
                                }
                            else if (is_reading_single_column())
                                {
                                m_single_value->assign(previousPosition,
                                                       trim.get_trimmed_string_length());
                                cellQuoteCollapse(*m_single_value);
                                ++currentColumnIndex;
                                }
                            ++m_number_of_columns_last_read;
                            }
                        if (currentPosition == nullptr)
                            {
                            return nullptr;
                            }
                        if (is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        if (currentPosition[0] == 0)
                            {
                            return nullptr;
                            }
                        // skip the delimiter(s)
                        if (m_treat_consecutive_delimiters_as_one)
                            {
                            while (currentColumnIter->get_parser().is_delimiter(currentPosition[0]))
                                {
                                ++currentPosition;
                                }
                            }
                        else
                            {
                            ++currentPosition;
                            }
                        }
                    }
                else if (pos->first == column_type::fixed_width)
                    {
                    auto currentColumnIter = m_fixed_width_columns.begin() + pos->second;
                    // some column definitions may be used more than once for consecutive columns
                    for (size_t i = 0; (!currentColumnIter->get_repeat_count() ?
                                            true :
                                            i < currentColumnIter->get_repeat_count().value());
                         ++i)
                        {
                        /* Make sure we had enough strings to write next column into
                           if this fails (it shouldn't) then the caller failed to resize the string
                           vector properly. By default, we won't resize it for them either,
                           because then the greater text matrix will wind up being a jagged array.
                           Enable column count resizing to change that.*/
                        if (m_values != nullptr)
                            {
                            if (currentColumnIndex >= m_values->size())
                                {
                                if (is_column_resizing_enabled())
                                    {
                                    m_values->resize(m_values->size() + 1);
                                    }
                                else
                                    {
                                    return currentPosition;
                                    }
                                }
                            }
                        // for optimized single-column reading return once a column is read
                        else if (is_reading_single_column() && currentColumnIndex >= 1)
                            {
                            return currentPosition;
                            }
                        previousPosition = currentPosition;
                        currentPosition = currentColumnIter->read(currentPosition);
                        // if the current row is blank then don't add anything to its columns
                        if (previousPosition == currentPosition && is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        // if this parser is NOT set to skip the column's text then read it in
                        if (currentColumnIter->get_parser().is_reading_text())
                            {
                            if (currentPosition == nullptr)
                                {
                                previousPosition = trim(previousPosition);
                                if (previousPosition == nullptr)
                                    {
                                    return nullptr;
                                    }
                                if (m_values != nullptr)
                                    {
                                    m_values->at(currentColumnIndex++).assign(previousPosition);
                                    }
                                else if (is_reading_single_column())
                                    {
                                    m_single_value->assign(previousPosition);
                                    }
                                ++m_number_of_columns_last_read;
                                return nullptr;
                                }
                            previousPosition =
                                trim(previousPosition, currentPosition - previousPosition);
                            if (previousPosition == nullptr)
                                {
                                return nullptr;
                                }
                            if (m_values != nullptr)
                                {
                                m_values->at(currentColumnIndex)
                                    .assign(previousPosition, trim.get_trimmed_string_length());
                                cellQuoteCollapse(m_values->at(currentColumnIndex));
                                ++currentColumnIndex;
                                }
                            else if (is_reading_single_column())
                                {
                                m_single_value->assign(previousPosition,
                                                       trim.get_trimmed_string_length());
                                cellQuoteCollapse(*m_single_value);
                                ++currentColumnIndex;
                                }
                            ++m_number_of_columns_last_read;
                            }
                        if (currentPosition == nullptr)
                            {
                            return nullptr;
                            }
                        if (is_eol(currentPosition[0]))
                            {
                            if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                                {
                                currentPosition += 2;
                                return currentPosition;
                                }
                            return ++currentPosition;
                            }
                        if (currentPosition[0] == 0)
                            {
                            return nullptr;
                            }
                        }
                    }
                else if (pos->first == column_type::to_eol)
                    {
                    auto currentColumnIter = m_to_eol_columns.begin() + pos->second;
                    /* Make sure we had enough strings to write next column into
                       if this fails (it shouldn't) then the caller failed to resize the string
                       vector properly. By default, we won't resize it for them either,
                       because then the greater text matrix will wind up being a jagged array.
                       Enable column count resizing to change that.*/
                    if (m_values != nullptr)
                        {
                        if (currentColumnIndex >= m_values->size())
                            {
                            if (is_column_resizing_enabled())
                                {
                                m_values->resize(m_values->size() + 1);
                                }
                            else
                                {
                                return currentPosition;
                                }
                            }
                        }
                    // for optimized single-column reading, return once a column is read
                    else if (is_reading_single_column() && currentColumnIndex >= 1)
                        {
                        return currentPosition;
                        }
                    previousPosition = currentPosition;
                    currentPosition = currentColumnIter->read(currentPosition);
                    // if the current row is blank then don't add anything to its columns
                    if (previousPosition == currentPosition && is_eol(currentPosition[0]))
                        {
                        if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                            {
                            currentPosition += 2;
                            return currentPosition;
                            }
                        return ++currentPosition;
                        }
                    // if this parser is NOT set to skip the column's text then read it in
                    if (currentColumnIter->get_parser().is_reading_text())
                        {
                        if (currentPosition == nullptr)
                            {
                            previousPosition = trim(previousPosition);
                            if (previousPosition == nullptr)
                                {
                                return nullptr;
                                }
                            if (m_values != nullptr)
                                {
                                m_values->at(currentColumnIndex++).assign(previousPosition);
                                }
                            else if (is_reading_single_column())
                                {
                                m_single_value->assign(previousPosition);
                                }
                            ++m_number_of_columns_last_read;
                            return nullptr;
                            }
                        previousPosition =
                            trim(previousPosition, currentPosition - previousPosition);
                        if (previousPosition == nullptr)
                            {
                            return nullptr;
                            }
                        if (m_values != nullptr)
                            {
                            m_values->at(currentColumnIndex)
                                .assign(previousPosition, trim.get_trimmed_string_length());
                            cellQuoteCollapse(m_values->at(currentColumnIndex));
                            ++currentColumnIndex;
                            }
                        else if (is_reading_single_column())
                            {
                            m_single_value->assign(previousPosition,
                                                   trim.get_trimmed_string_length());
                            cellQuoteCollapse(*m_single_value);
                            ++currentColumnIndex;
                            }
                        ++m_number_of_columns_last_read;
                        }
                    if (currentPosition == nullptr)
                        {
                        return nullptr;
                        }
                    if (is_eol(currentPosition[0]))
                        {
                        if (currentPosition[0] == L'\r' && currentPosition[1] == L'\n')
                            {
                            currentPosition += 2;
                            return currentPosition;
                            }
                        return ++currentPosition;
                        }
                    if (currentPosition[0] == 0)
                        {
                        return nullptr;
                        }
                    }
                }
            // in case we are ignoring this row, then just eat up its text and jump to the next line
            if (currentPosition == text)
                {
                while (!is_eol(currentPosition[0]))
                    {
                    if (currentPosition[0] == 0)
                        {
                        return nullptr;
                        }
                    ++currentPosition;
                    }
                ++currentPosition; // skip the EOL we just hit
                }
            return currentPosition;
            }

        /// @returns The number of times this row definition should be repeated
        ///     by the parent parser.
        /// @note This is optional and if not specified then this row definition should be repeated
        ///     by the parser until the end of file is reached.
        [[nodiscard]]
        inline std::optional<size_t> get_repeat_count() const noexcept
            {
            return m_repeat_count;
            }

        /// @returns The number of columns last read during the previous parsing.
        [[nodiscard]]
        inline size_t get_number_of_columns_last_read() const noexcept
            {
            return m_number_of_columns_last_read;
            }

      private:
        enum class column_type
            {
            standard_delimiter,
            delimited_character,
            delimited_multiple_character,
            fixed_width,
            to_eol
            };

        [[nodiscard]]
        bool is_reading_single_column() const noexcept
            {
            return (m_single_value != nullptr);
            }

        std::vector<string_typeT>* m_values{ nullptr };
        string_typeT* m_single_value{ nullptr };
        std::vector<std::pair<column_type, size_t>> m_column_indices;
        // different column parser types
        std::vector<text_column<text_column_standard_delimiter_parser>>
            m_standard_delimiter_columns;
        std::vector<text_column<text_column_delimited_character_parser>>
            m_delimited_character_columns;
        std::vector<text_column<text_column_fixed_parser>> m_fixed_width_columns;
        std::vector<text_column<text_column_to_eol_parser>> m_to_eol_columns;
        std::vector<text_column<text_column_delimited_multiple_character_parser>>
            m_delimited_multiple_character_columns;
        is_end_of_line is_eol;
        std::optional<size_t> m_repeat_count{ std::nullopt };
        bool m_treat_consecutive_delimiters_as_one{ false };
        bool m_allow_column_values_resizing{ false };
        /* this will be false if all column parsers are set to skip their text
           (and in essence, skip this row)*/
        bool m_read_text{ false };
        size_t m_number_of_columns_last_read{ 0 };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // TEXT_ROW_H
