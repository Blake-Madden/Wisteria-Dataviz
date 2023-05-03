/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __TEXT_MATRIX_H__
#define __TEXT_MATRIX_H__

#include "text_column.h"
#include "text_row.h"
#include <optional>

/// @brief Text importing library.
/// @details This is low-level code. Prefer using Wisteria::Data::Dataset::ImportText()
///     to import data.
namespace lily_of_the_valley
    {
    // some useful typedefs
    /// @brief A standard delimited column.
    using standard_delimited_character_column = text_column<text_column_delimited_character_parser>;
    /// @brief A standard row.
    using standard_row = text_row<std::wstring>;

    /// @brief A tabular text importing interface.
    template<typename string_typeT>
    class text_matrix
        {
    public:
        /// @brief Use this constructor to load a matrix (2D array) file.
        /// @param matrix The string matrix to read the parsed data to.
        explicit text_matrix(std::vector<std::vector<string_typeT>>* matrix)
            : m_matrix(matrix)
            {}
        /** @brief Use this constructor if loading a single column file.
            @details This is optimal for situations where you want to simply load
                the entire file into a single vector, instead of a 2D matrix.
            @param vector The string vector to write the parsed data to.*/
        explicit text_matrix(std::vector<string_typeT>* vector)
            : m_vector(vector)
            {}
        /// @private
        text_matrix() = delete;

        /** @brief Adds a row definition to the parser.
            @param row The row definition to add.*/
        void add_row_definition(const text_row<string_typeT>& row)
            { m_rows.push_back(row); }
        /// @brief Clears the row definitions from the parser.
        void remove_rows() noexcept
            { m_rows.clear(); }
        /** @brief The main function which reads a block of text and divides it up into columns & rows
                and returns the number of rows read.
            @details The row count must be correct (it won't read any more than what you specify here). The
                column count must be the widest row. Different rows may have a different number
                of columns, but you should specify the widest possible amount of columns to avoid a jagged array.
            @param text The text to parser and import tabular data from.
            @param row_count The number of rows to import (use the text_preview to determine this).
            @param column_count The number of columns to import. Note that this will be overridden
                if any row definitions allow for dynamic column growth.
            @param ignore_blank_lines Whether blank lines should be skipped.
            @returns The number of rows read.*/
        size_t read(const wchar_t* text, const size_t row_count, const size_t column_count,
                    const bool ignore_blank_lines = false)
            {
            if (text == nullptr || text[0] == 0 || row_count == 0)
                { return 0; }
            const wchar_t* currentPosition = text;
            size_t currentRowIndex = 0;

            if (m_matrix)
                {
                m_matrix->clear();
                m_matrix->resize(row_count);
                }
            else
                {
                m_vector->clear();
                m_vector->resize(row_count);
                }
            for (auto pos = m_rows.begin();
                pos != m_rows.end();
                ++pos)
                {
                // some row definitions may be used more than once for consecutive rows
                for (size_t i = 0;
                    (!pos->get_repeat_count() ? true : i < pos->get_repeat_count().value());
                    ++i)
                    {
                    if (currentRowIndex >= row_count)
                        {
                        recode_md_code();
                        return currentRowIndex;
                        }
                    if (m_matrix)
                        {
                        m_matrix->at(currentRowIndex).resize(column_count);
                        pos->set_values(&m_matrix->at(currentRowIndex));
                        }
                    else
                        { pos->set_single_value(&m_vector->at(currentRowIndex)); }
                    // if ignoring blank lines, then let this row parser go until it reads in one valid row
                    if (ignore_blank_lines)
                        {
                        /* If ignoring blank lines then use the current row reader's iteration until it actually
                           reads in a row of text. If we are using a row reader for 5 rows and we have 2 rows of text,
                           a blank row, and three more rows of text then we read 2, skip the blank, and then read the
                           next three. If we are NOT ignoring blank rows, then we would just read the first
                           five rows and treat the blank one as one of the iterations of the current row reader.*/
                        for (;;)
                            {
                            const wchar_t* originalPosition = currentPosition;
                            currentPosition = pos->read(currentPosition);
                            // if any columns were read then stop with the current iteration of this row reader
                            if (pos->get_number_of_columns_last_read() != 0)
                                {
                                if (m_matrix)
                                    {
                                    // ...but, make sure at least one of the row's columns actually contains text
                                    bool columnWithTextFound = false;
                                    auto currentRowIter = m_matrix->at(currentRowIndex).cbegin();
                                    auto currentRowIterEnd = m_matrix->at(currentRowIndex).cend();
                                    while (currentRowIter != currentRowIterEnd)
                                        {
                                        if (!currentRowIter->empty())
                                            {
                                            columnWithTextFound = true;
                                            break;
                                            }
                                        ++currentRowIter;
                                        }
                                    if (columnWithTextFound)
                                        { break; }
                                    }
                                else
                                    {
                                    if (!m_vector->at(currentRowIndex).empty())
                                        { break; }
                                    }
                                }
                            // if no columns were read then check to see if we are at the end of the file
                            if (currentPosition == nullptr)
                                {
                                // resize to the number of row that were actually read in
                                if (m_matrix)
                                    { m_matrix->resize(currentRowIndex); }
                                else
                                    { m_vector->resize(currentRowIndex); }
                                recode_md_code();
                                return currentRowIndex;
                                }
                            /* check to see if the row reader went to the end of its line
                               (this may not happen if it doesn't have enough columns specifiers).
                               If not, then scan to the end of the line ourselves.*/
                            if (currentPosition > text && !is_eol(currentPosition[-1]))
                                {
                                for (;;)
                                    {
                                    ++currentPosition;
                                    if (currentPosition[0] == 0 || is_eol(currentPosition[0]))
                                        { break; }
                                    }
                                }
                            /* Otherwise, we are at the end of the line, so move to the start of the next one.
                               Skip the line feed if following a carriage return--
                               this is the standard for Windows files.*/
                            if (currentPosition > text && currentPosition[-1] == 13 && currentPosition[0] == 10)
                                { ++currentPosition; }
                            /* if this row reader's columns are all set to skip the columns of text
                               then that is why no columns were read, so we should break here. Of course,
                               make sure the read line really wasn't blank by checking if the start of the line
                               was an EOL.*/
                            if (!pos->is_reading_text() && !is_eol(originalPosition[0]))
                                { break; }
                            }
                        if (pos->is_reading_text())
                            {
                            // Finished reading all columns for this row. Now, if this row has a dynamic number of columns,
                            // then trim any extra (blank ones at the end) that were added while reading
                            // Note that this only applies for multi-column data (i.e., "matrix" mode).
                            if (m_matrix)
                                {
                                if (pos->is_column_resizing_enabled() &&
                                    pos->get_number_of_columns_last_read() < m_matrix->at(currentRowIndex).size())
                                    { m_matrix->at(currentRowIndex).resize(pos->get_number_of_columns_last_read()); }
                                }
                            ++currentRowIndex;
                            }
                        }
                    else
                        {
                        currentPosition = pos->read(currentPosition);
                        if (pos->is_reading_text())
                            {
                            // Finished reading all columns for this row. Now, if this row has a dynamic number of columns,
                            // then trim any extra (blank ones at the end) that were added while reading,
                            // Note that this only applies for multi-column data (i.e., "matrix" mode).
                            if (m_matrix)
                                {
                                if (pos->is_column_resizing_enabled() &&
                                    pos->get_number_of_columns_last_read() < m_matrix->at(currentRowIndex).size())
                                    { m_matrix->at(currentRowIndex).resize(pos->get_number_of_columns_last_read()); }
                                }
                            ++currentRowIndex;
                            }
                        }
                    if (currentPosition == nullptr)
                        {
                        // resize to the number of rows that were actually read in
                        if (m_matrix)
                            { m_matrix->resize(currentRowIndex); }
                        else
                            { m_vector->resize(currentRowIndex); }
                        recode_md_code();
                        return currentRowIndex;
                        }
                    /* check to see if the row reader went to the end of its line
                       (this may not happen if it doesn't have enough columns specifiers).
                       If not, then scan to the end of the line ourselves.*/
                    if (currentPosition > text && !is_eol(currentPosition[-1]))
                        {
                        for (;;)
                            {
                            ++currentPosition;
                            if (currentPosition[0] == 0 || is_eol(currentPosition[0]))
                                { break; }
                            }
                        }
                    /* skip the line feed if following a carriage return--
                       this is the standard for Windows files.*/
                    if (currentPosition > text && currentPosition[-1] == 13 && currentPosition[0] == 10)
                        { ++currentPosition; }
                    }
                }
            if (m_matrix)
                { m_matrix->resize(currentRowIndex); }
            else
                { m_vector->resize(currentRowIndex); }

            recode_md_code();
            return currentRowIndex;
            }
        /// @brief Sets the values to treat as missing data (e.g., "NULL," "NA," etc.).
        /// @param mdCodes The values to treat as missing data.
        void set_missing_data_codes(const std::optional<std::vector<string_typeT>>& mdCodes) noexcept
            { m_mdVals = mdCodes; }
    private:
        void recode_md_code()
            {
            if (m_mdVals)
                {
                if (m_matrix)
                    {
                    #pragma omp parallel for
                    for (int64_t row = 0; row < static_cast<int64_t>(m_matrix->size()); ++row)
                        {
                        for (auto& cell : (*m_matrix)[row])
                            {
                            for (const auto& mdVal : m_mdVals.value())
                                {
                                if (cell == mdVal)
                                    {
                                    cell.clear();
                                    break;
                                    }
                                }
                            }
                        }
                    }
                else
                    {
                    for (auto& cell : *m_vector)
                        {
                        for (const auto& mdVal : m_mdVals.value())
                            {
                            if (cell == mdVal)
                                {
                                cell.clear();
                                break;
                                }
                            }
                        }
                    }
                }
            }
        std::vector<std::vector<string_typeT>>* m_matrix{ nullptr };
        std::vector<string_typeT>* m_vector{ nullptr };
        std::vector<text_row<string_typeT>> m_rows;
        is_end_of_line is_eol;
        std::optional<std::vector<string_typeT>> m_mdVals{ std::nullopt };
        };
    };

/** @}*/

#endif //__TEXT_MATRIX_H__
