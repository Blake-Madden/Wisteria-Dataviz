///////////////////////////////////////////////////////////////////////////////
// Name:        spreadsheet_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "spreadsheet_extract_text.h"
#include <algorithm>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    std::wstring spreadsheet_extract_text::get_cell_text(const wchar_t* cellName,
                                                         const worksheet& workSheet)
        {
        const auto cinfo = get_column_and_row_info(cellName);
        if (cinfo.second < 1 || cinfo.second > workSheet.size())
            {
            return {};
            }
        const worksheet_row& currentRow = workSheet[cinfo.second - 1];
        // out-of-range column index? Do a search for the cell by name.
        if (cinfo.first.m_position < 1 || cinfo.first.m_position > currentRow.size())
            {
            const auto cellPos =
                std::lower_bound(currentRow.begin(), currentRow.end(), worksheet_cell(cellName));
            return (cellPos != currentRow.end() && cellPos->get_name() == cellName) ?
                       cellPos->get_value() :
                       std::wstring{};
            }
        if (currentRow.operator[](cinfo.first.m_position - 1).get_name() == cellName)
            {
            return currentRow.operator[](cinfo.first.m_position - 1).get_value();
            }
        // If item at given index doesn't match by name, then there must have been
        // missing cells in the file (and no dimension info), so our matrix is sparse.
        // So brute force search for the cell by name.
        const auto cellPos =
            std::lower_bound(currentRow.cbegin(), currentRow.cend(), worksheet_cell(cellName));
        return (cellPos != currentRow.cend() && cellPos->get_name() == cellName) ?
                   cellPos->get_value() :
                   std::wstring{};
        }

    //------------------------------------------------------------------
    void spreadsheet_extract_text::get_text_cell_names(const worksheet& wrk,
                                                       std::vector<std::wstring>& cells)
        {
        cells.clear();
        if (wrk.empty())
            {
            return;
            }
        // assume that a fourth the worksheet is text
        cells.reserve(wrk.size() * (wrk[0].size() / 4));
        for (const auto& rowPos : wrk)
            {
            for (const auto& rowPo : rowPos)
                {
                if (!rowPo.get_value().empty())
                    {
                    cells.push_back(rowPo.get_name());
                    }
                }
            }
        }

    //------------------------------------------------------------------
    std::wstring spreadsheet_extract_text::get_worksheet_text(const worksheet& wrk,
                                                              const wchar_t delim /*= L'\t'*/)
        {
        std::wstring dataText;
        const auto cellCount{ spreadsheet_extract_text::get_cell_count(wrk) };
        if (cellCount == 0)
            {
            return {};
            }

        dataText.reserve(cellCount * 5);
        for (const auto& row : wrk)
            {
            for (const auto& cell : row)
                {
                dataText.append(cell.get_value()).append(1, delim);
                }
            // make extra tab at the end a new line for the next row
            dataText.back() = L'\n';
            }
        // chop off extra newline at the end
        if (!dataText.empty() && dataText.back() == L'\n')
            {
            dataText.erase(dataText.end() - 1);
            }

        return dataText;
        }

    //------------------------------------------------------------------
    std::pair<bool, std::wstring> spreadsheet_extract_text::verify_sheet(const worksheet& data)
        {
        for (size_t rowCounter = 0; rowCounter < data.size(); ++rowCounter)
            {
            for (size_t columnCounter = 0; columnCounter < data[rowCounter].size(); ++columnCounter)
                {
                const worksheet_cell currentCell(column_index_to_column_name(columnCounter + 1) +
                                                 std::to_wstring(rowCounter + 1));
                const auto cellPos = std::lower_bound(data[rowCounter].cbegin(),
                                                      data[rowCounter].cend(), currentCell);
                // if cell was already in the row, then move on
                if (cellPos != data[rowCounter].cend() && *cellPos == currentCell)
                    {
                    continue;
                    }
                return std::make_pair(false, currentCell.get_name());
                }
            }
        return std::make_pair(true, std::wstring{});
        }

    //------------------------------------------------------------------
    std::wstring spreadsheet_extract_text::column_index_to_column_name(size_t col)
        {
        if (column_info::INVALID_POSITION == col)
            {
            return {};
            }
        std::wstring columnName;
        while (col > 0)
            {
            constexpr auto ALPHABET_SIZE{ 26 };
            const wchar_t modulo = (col - 1) % ALPHABET_SIZE;
            columnName.insert(columnName.begin(), 1, L'A' + modulo);
            col = ((col - modulo) / ALPHABET_SIZE);
            }
        return columnName;
        }

    //------------------------------------------------------------------
    std::pair<spreadsheet_extract_text::column_info, size_t>
    spreadsheet_extract_text::get_column_and_row_info(const wchar_t* cell_name)
        {
        assert(cell_name);
        if (cell_name == nullptr)
            {
            return std::make_pair(column_info(), column_info::INVALID_POSITION);
            }
        const std::pair<size_t, size_t> cellInfo = split_column_info(cell_name);
        if (cellInfo.first == worksheet_cell::INVALID_INDEX)
            {
            return std::make_pair(column_info(), column_info::INVALID_POSITION);
            }

        column_info cinfo(0);
        for (size_t i = 0; i < cellInfo.first; ++i)
            {
            const wchar_t currentLetter = std::towupper(cell_name[i]);
            // if non letter in column name, then something is wrong;
            // set the column to bogus and quit
            if (!(currentLetter >= L'A' && currentLetter <= L'Z'))
                {
                cinfo.m_position = column_info::INVALID_POSITION;
                break;
                }
            // change 'A' (value 65) to 1, 'B' to 2, etc.
            const wchar_t letterVal = currentLetter - 64;
            cinfo.m_position +=
                letterVal *
                static_cast<size_t>(std::pow(26, static_cast<double>(cellInfo.first - 1 - i)));
            }

        return std::make_pair(cinfo, cellInfo.second);
        }

    //------------------------------------------------------------------
    void spreadsheet_extract_text::fill_missing_cells(worksheet& data)
        {
        if (data.empty())
            {
            return;
            }
        // Find the highest column position across all rows.
        // This is the true column span, which may be larger than any
        // row's cell count when interior cells are missing.
        size_t maxColumnPosition{ 0 };
        for (const auto& datum : data)
            {
            if (!datum.empty())
                {
                maxColumnPosition = std::max(datum.back().get_column_position(), maxColumnPosition);
                }
            }
            // Fill in blank cells at any missing column positions in each row.
            {
            for (size_t rowCounter = 0; rowCounter < data.size(); ++rowCounter)
                {
                for (size_t columnCounter = 0; columnCounter < maxColumnPosition; ++columnCounter)
                    {
                    const worksheet_cell currentCell(
                        column_index_to_column_name(columnCounter + 1) +
                        std::to_wstring(rowCounter + 1));
                    const auto cellPos = std::lower_bound(data[rowCounter].begin(),
                                                          data[rowCounter].end(), currentCell);
                    // if cell was already in the row, then move on
                    if (cellPos != data[rowCounter].end() && *cellPos == currentCell)
                        {
                        continue;
                        }
                    if (cellPos != data[rowCounter].end())
                        {
                        data[rowCounter].insert(cellPos, currentCell);
                        }
                    else
                        {
                        data[rowCounter].push_back(currentCell);
                        }
                    }
                }
            }
        }

    //------------------------------------------------------------------
    std::pair<size_t, size_t>
    spreadsheet_extract_text::split_column_info(std::wstring_view cell_name)
        {
        assert(!cell_name.empty());
        size_t numStart = cell_name.find_last_not_of(L"0123456789");
        if (numStart == std::wstring::npos || cell_name.length() == numStart + 1)
            {
            return std::make_pair(std::wstring::npos, 0);
            }
        cell_name.remove_prefix(++numStart);
        const auto row = static_cast<size_t>(std::wcstol(cell_name.data(), nullptr, 10));
        if (row == 0)
            {
            return std::make_pair(std::wstring::npos, 0);
            }
        return std::make_pair(numStart, row);
        }
    } // namespace lily_of_the_valley
