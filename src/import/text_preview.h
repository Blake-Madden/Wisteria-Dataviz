/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __TEXT_PREVIEW_H__
#define __TEXT_PREVIEW_H__

#include <map>
#include "text_functional.h"
#include "text_matrix.h"

namespace lily_of_the_valley
    {
    /** @brief Functor for previewing a delimited file.
        @details That is to say, it reports the number of rows
            of a text file based on supplied line end criteria.*/
    class text_preview
        {
    public:
        /** @brief Main interface for previewing a file.
            @returns The number of rows in the files.
            @param text A wide character stream t to parse.
            @param headerRowDelimiter The delimiter to use to determine the number of columns when
                parsing the header.
            @param ignoreBlankLines Specifies whether or not to count empty rows as lines.
                If set to @c true, then consecutive line ends will be skipped when counting the
                lines in the file. Note that a row of column delimiters (with no data) will be seen
                as a row however because we aren't using advanced column parsing logic in here.
            @param storeRowInfo Specifies whether to store the beginning and end of each line
                (as an index into the text block) into a map.
                This is useful if the caller needs to further review a preview line-by-line.
            @param skipRows The number of rows to skip before reading the text.
            @warning Setting @c storeRowInfo to @c true will impact the preview's performance.*/
        [[nodiscard]] size_t operator()(const wchar_t* text, const wchar_t headerRowDelimiter,
                          const bool ignoreBlankLines,
                          const bool storeRowInfo,
                          size_t skipRows = 0)
            {
            assert(text);
            m_header_names.clear();
            m_lines.clear();
            m_row_count = 0;

            if (text == nullptr || text[0] == 0)
                { return 0; }

            lily_of_the_valley::text_column<text_column_to_eol_parser>
                noReadColumn(lily_of_the_valley::text_column_to_eol_parser{ false });
            lily_of_the_valley::text_row<std::wstring> noReadRowsStart;
            noReadRowsStart.add_column(noReadColumn);
            while (skipRows > 0)
                {
                // skip initial lines of text that the caller asked to skip
                text = noReadRowsStart.read(text);
                --skipRows;
                }
            
            standard_delimited_character_column deliminatedColumn(
                text_column_delimited_character_parser{ headerRowDelimiter });
            text_row<std::wstring> headerRow(1);
            headerRow.add_column(deliminatedColumn);
            headerRow.set_values(&m_header_names);
            headerRow.allow_column_resizing(true);
            headerRow.read(text);
            cell_collapse_quotes<std::wstring> collapseQuotes;
            // cppcheck-suppress knownEmptyContainer
            for (auto& header : m_header_names)
                { collapseQuotes(header); }

            const wchar_t* currentPos = text;
            const wchar_t* lineStart = nullptr;
            while (currentPos[0])
                {
                lineStart = currentPos;
                // find the end of the current line
                while (currentPos[0])
                    {
                    if (is_eol(currentPos[0]) )
                        { break; }
                    ++currentPos;
                    }
                ++m_row_count;
                if (storeRowInfo)
                    { m_lines.insert(std::make_pair(lineStart, currentPos)); }
                // if at end of the text, then stop
                if (currentPos[0] == 0)
                    { break; }
                // otherwise, go to the start of the next line:
                // skip the line feed if following a carriage return--
                // this is the standard for Windows files.
                if (currentPos[0] == 13 && currentPos[1] == 10)
                    { currentPos += 2; }
                else
                    { ++currentPos; }
                // just in case there is a blank line at the very end of the file then include it
                if (currentPos[0] == 0)
                    {
                    ++m_row_count;
                    break;
                    }
                // eat up any more new lines if we are ignoring blank lines
                if (ignoreBlankLines)
                    {
                    while (is_eol(currentPos[0]) )
                        { ++currentPos; }
                    }
                }
            return m_row_count;
            }
        /// @returns The number of rows from the last preview.
        [[nodiscard]] size_t get_row_count() const noexcept
            { return m_row_count; }
        /// @returns The names of the column header (from the first row) from the last preview.
        [[nodiscard]] const std::vector<std::wstring>& get_header_names() const noexcept
            { return m_header_names; }
        /// @returns The definition information of the lines.
        /// @warning If the parameter @c storeRowInfo in the preview call was @c false, then this
        ///     will be empty.
        [[nodiscard]] const std::map<const wchar_t*, const wchar_t*>& get_line_info() const noexcept
            { return m_lines; }
    private:
        std::map<const wchar_t*, const wchar_t*> m_lines;
        std::vector<std::wstring> m_header_names;
        size_t m_row_count{ 0 };
        is_end_of_line is_eol;
        };
    }

/** @}*/

#endif //__TEXT_PREVIEW_H__
