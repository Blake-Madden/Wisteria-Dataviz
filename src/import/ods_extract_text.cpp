///////////////////////////////////////////////////////////////////////////////
// Name:        ods_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ods_extract_text.h"
#include <algorithm>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    void ods_extract_text::read_worksheet_names(const wchar_t* text, const size_t text_length)
        {
        m_worksheet_names.clear();
        if (text == nullptr || text_length == 0)
            {
            return;
            }
        assert(text_length == std::wcslen(text));
        const wchar_t* const textEnd = text + text_length;

        // find <office:spreadsheet>
        const wchar_t* spreadsheet =
            html_extract_text::find_element(text, textEnd, L"office:spreadsheet", true);
        if (spreadsheet == nullptr)
            {
            return;
            }
        const wchar_t* const spreadsheetEnd =
            html_extract_text::find_closing_element(spreadsheet, textEnd, L"office:spreadsheet");
        if (spreadsheetEnd == nullptr)
            {
            return;
            }

        // iterate <table:table> elements and read their table:name attribute
        const wchar_t* tableTag = spreadsheet;
        while ((tableTag = html_extract_text::find_element(tableTag, spreadsheetEnd, L"table:table",
                                                           true)) != nullptr)
            {
            const std::wstring tableName =
                html_extract_text::read_attribute_as_string(tableTag, L"table:name", false, true);
            if (!tableName.empty())
                {
                m_worksheet_names.push_back(tableName);
                }
            // step past the opening tag to avoid re-matching
            tableTag = html_extract_text::find_close_tag(tableTag);
            if (tableTag == nullptr)
                {
                break;
                }
            // skip past the table body to find the next table
            const wchar_t* const tableEnd =
                html_extract_text::find_closing_element(tableTag, spreadsheetEnd, L"table:table");
            if (tableEnd == nullptr)
                {
                break;
                }
            tableTag = tableEnd;
            }
        }

    //------------------------------------------------------------------
    void ods_extract_text::operator()(const wchar_t* text, const size_t textLength, worksheet& data,
                                      const std::variant<std::wstring, size_t>& theWorksheet)
        {
        data.clear();
        if (text == nullptr || text[0] == 0 || textLength == 0)
            {
            return;
            }
        assert(textLength == std::wcslen(text));
        const wchar_t* const textEnd = text + textLength;

        // find <office:spreadsheet>
        const wchar_t* spreadsheet =
            html_extract_text::find_element(text, textEnd, L"office:spreadsheet", true);
        if (spreadsheet == nullptr)
            {
            return;
            }
        const wchar_t* const spreadsheetEnd =
            html_extract_text::find_closing_element(spreadsheet, textEnd, L"office:spreadsheet");
        if (spreadsheetEnd == nullptr)
            {
            return;
            }

        // find the target worksheet by name or 1-based index.
        // ODS stores all worksheets sequentially as <table:table> elements
        // within <office:spreadsheet>, so we iterate until we find a match.
        const wchar_t* targetTableStart{ nullptr };
        const wchar_t* targetTableEnd{ nullptr };

        const wchar_t* tableTag{ spreadsheet };
        size_t tableIndex{ 0 };
        while ((tableTag = html_extract_text::find_element(tableTag, spreadsheetEnd, L"table:table",
                                                           true)) != nullptr)
            {
            ++tableIndex;
            const std::wstring tableName =
                html_extract_text::read_attribute_as_string(tableTag, L"table:name", false, true);

            bool isMatch{ false };
            if (const auto* const namePtr = std::get_if<std::wstring>(&theWorksheet);
                namePtr != nullptr)
                {
                isMatch = (tableName == *namePtr);
                }
            else if (const auto* const indexPtr = std::get_if<size_t>(&theWorksheet);
                     indexPtr != nullptr)
                {
                isMatch = (tableIndex == *indexPtr);
                }

            tableTag = html_extract_text::find_close_tag(tableTag);
            if (tableTag == nullptr)
                {
                break;
                }

            const wchar_t* const tableEnd =
                html_extract_text::find_closing_element(tableTag, spreadsheetEnd, L"table:table");
            if (tableEnd == nullptr)
                {
                break;
                }

            if (isMatch)
                {
                targetTableStart = tableTag;
                targetTableEnd = tableEnd;
                break;
                }
            tableTag = tableEnd;
            }

        if (targetTableStart == nullptr || targetTableEnd == nullptr)
            {
            return;
            }

        // parse the rows within the matched table
        const wchar_t* rowTag{ targetTableStart };
        size_t currentRowNum{ 0 };

        while ((rowTag = html_extract_text::find_element(rowTag, targetTableEnd, L"table:table-row",
                                                         true)) != nullptr)
            {
            // ODS uses run-length encoding for repeated rows via the
            // table:number-rows-repeated attribute (e.g., 1000 trailing empty rows
            // compressed into a single element).
            const std::wstring rowsRepeatedStr = html_extract_text::read_attribute_as_string(
                rowTag, L"table:number-rows-repeated", false, false);
            const size_t rowsRepeated =
                rowsRepeatedStr.empty() ?
                    1 :
                    static_cast<size_t>(std::wcstol(rowsRepeatedStr.c_str(), nullptr, 10));

            const wchar_t* const rowEnd =
                html_extract_text::find_closing_element(rowTag, targetTableEnd, L"table:table-row");
            // self-terminating row (no closing </table:table-row> tag, meaning empty row).
            // Just advance the row counter without adding data to the matrix.
            if (rowEnd == nullptr)
                {
                // LibreOffice commonly pads sheets with thousands of empty repeated rows;
                // a large repetition count signals the end of meaningful data.
                if (rowsRepeated > 100)
                    {
                    break;
                    }
                currentRowNum += rowsRepeated;
                rowTag = html_extract_text::find_close_tag(rowTag);
                if (rowTag == nullptr)
                    {
                    break;
                    }
                continue;
                }

            // parse cells within this row
            worksheet_row parsedRow;
            size_t currentColNum = 0;
            const wchar_t* cellTag = rowTag;

            while (cellTag < rowEnd)
                {
                // look for table:table-cell or table:covered-table-cell.
                // Covered cells are placeholders for cells hidden by a merge span;
                // they occupy a column position but have no data.
                const wchar_t* nextCell =
                    html_extract_text::find_element(cellTag, rowEnd, L"table:table-cell", true);
                const wchar_t* nextCovered = html_extract_text::find_element(
                    cellTag, rowEnd, L"table:covered-table-cell", true);

                // pick whichever comes first in the XML
                bool isCovered = false;
                if (nextCell == nullptr && nextCovered == nullptr)
                    {
                    break;
                    }
                if (nextCell == nullptr || (nextCovered != nullptr && nextCovered < nextCell))
                    {
                    cellTag = nextCovered;
                    isCovered = true;
                    }
                else
                    {
                    cellTag = nextCell;
                    }

                // ODS also uses run-length encoding for repeated cells
                // (e.g., empty cells padding a row to the full column width).
                const std::wstring colsRepeatedStr = html_extract_text::read_attribute_as_string(
                    cellTag, L"table:number-columns-repeated", false, false);
                const size_t colsRepeated =
                    colsRepeatedStr.empty() ?
                        1 :
                        static_cast<size_t>(std::wcstol(colsRepeatedStr.c_str(), nullptr, 10));

                // read the cell value based on its office:value-type attribute.
                // Unlike XLSX (which stores dates as serial numbers requiring style-based
                // detection), ODS explicitly marks cell types: "string", "float", "date",
                // "boolean", "currency", "percentage", and "time".
                std::wstring cellValue;
                if (!isCovered)
                    {
                    const std::wstring valueType = html_extract_text::read_attribute_as_string(
                        cellTag, L"office:value-type", false, false);

                    if (valueType == L"string")
                        {
                        // ODS stores string values inline as <text:p> child elements
                        // (no shared string table like XLSX).
                        const wchar_t* closeTag = html_extract_text::find_close_tag(cellTag);
                        if (closeTag != nullptr)
                            {
                            ++closeTag;
                            const wchar_t* const cellEnd = html_extract_text::find_closing_element(
                                closeTag, rowEnd, L"table:table-cell");
                            if (cellEnd != nullptr)
                                {
                                // concatenate all <text:p> elements
                                const wchar_t* tpTag = closeTag;
                                bool firstParagraph = true;
                                while ((tpTag = html_extract_text::find_element(
                                            tpTag, cellEnd, L"text:p", true)) != nullptr)
                                    {
                                    tpTag = html_extract_text::find_close_tag(tpTag);
                                    if (tpTag == nullptr)
                                        {
                                        break;
                                        }
                                    ++tpTag;
                                    const wchar_t* const tpEnd =
                                        html_extract_text::find_closing_element(tpTag, cellEnd,
                                                                                L"text:p");
                                    if (tpEnd == nullptr)
                                        {
                                        break;
                                        }
                                    if (!firstParagraph)
                                        {
                                        cellValue += L' ';
                                        }
                                    if (tpEnd > tpTag)
                                        {
                                        // decode any HTML entities in the text
                                        if (m_html_extract(tpTag, tpEnd - tpTag, true, true) !=
                                                nullptr &&
                                            m_html_extract.get_filtered_text_length() > 0)
                                            {
                                            cellValue.append(
                                                m_html_extract.get_filtered_text(),
                                                m_html_extract.get_filtered_text_length());
                                            }
                                        }
                                    firstParagraph = false;
                                    tpTag = tpEnd;
                                    }
                                }
                            }
                        }
                    else if (valueType == L"float" || valueType == L"currency" ||
                             valueType == L"percentage")
                        {
                        cellValue = html_extract_text::read_attribute_as_string(
                            cellTag, L"office:value", false, false);
                        }
                    else if (valueType == L"date")
                        {
                        // ODS dates are ISO 8601 (e.g., "2024-03-15T08:30:00").
                        // Replace the 'T' separator with a space for downstream parsing.
                        cellValue = html_extract_text::read_attribute_as_string(
                            cellTag, L"office:date-value", false, false);
                        const auto tPos = cellValue.find(L'T');
                        if (tPos != std::wstring::npos)
                            {
                            cellValue[tPos] = L' ';
                            }
                        }
                    else if (valueType == L"time")
                        {
                        cellValue = html_extract_text::read_attribute_as_string(
                            cellTag, L"office:time-value", false, false);
                        }
                    else if (valueType == L"boolean")
                        {
                        const std::wstring boolVal = html_extract_text::read_attribute_as_string(
                            cellTag, L"office:boolean-value", false, false);
                        cellValue = (boolVal == L"true") ? L"TRUE" : L"FALSE";
                        }
                    // else: empty cell (no value-type), cellValue stays empty
                    }

                if (m_removeNewlinesAndTabs && !cellValue.empty())
                    {
                    std::ranges::transform(cellValue, cellValue.begin(),
                                           [](auto& ch) noexcept
                                           {
                                               return (ch == L'\n' || ch == L'\r' || ch == L'\t' ||
                                                       ch == L'\u00A0') ?
                                                          L' ' :
                                                          ch;
                                           });
                    }

                // if this is a large column repetition on an empty cell,
                // it's just padding; advance column position and skip it
                if (cellValue.empty() && colsRepeated > 100)
                    {
                    currentColNum += colsRepeated;
                    cellTag = html_extract_text::find_close_tag(cellTag);
                    if (cellTag == nullptr)
                        {
                        break;
                        }
                    ++cellTag;
                    continue;
                    }

                // add the cell(s) to the row
                for (size_t c = 0; c < colsRepeated; ++c)
                    {
                    ++currentColNum;
                    parsedRow.emplace_back(currentColNum, currentRowNum + 1, cellValue);
                    }

                cellTag = html_extract_text::find_close_tag(cellTag);
                if (cellTag == nullptr)
                    {
                    break;
                    }
                ++cellTag;
                }

            // if this is a large row repetition on an empty row, stop processing
            if (parsedRow.empty() && rowsRepeated > 100)
                {
                break;
                }

            // add the parsed row (repeated if necessary)
            ++currentRowNum;
            data.push_back(parsedRow);
            for (size_t r = 1; r < rowsRepeated; ++r)
                {
                ++currentRowNum;
                worksheet_row repeatedRow;
                repeatedRow.reserve(parsedRow.size());
                size_t colIndex = 0;
                for (const auto& cell : parsedRow)
                    {
                    ++colIndex;
                    repeatedRow.emplace_back(colIndex, currentRowNum, cell.get_value());
                    }
                data.push_back(std::move(repeatedRow));
                }

            rowTag = rowEnd;
            }

        fix_jagged_sheet(data);
        }
    } // namespace lily_of_the_valley
