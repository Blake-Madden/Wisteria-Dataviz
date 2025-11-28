///////////////////////////////////////////////////////////////////////////////
// Name:        xlsx_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "xlsx_extract_text.h"
#include <algorithm>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    int xlsx_extract_text::dmy_to_excel_serial_date(int nDay, int nMonth, int nYear)
        {
        // Excel and Lotus 123 have a legacy issue with 29-02-1900. 1900 is not a
        // leap year, but Excel and Lotus 123 treat it like it is...
        if (nDay == 29 && nMonth == 02 && nYear == 1900)
            {
            return 60;
            }

        // DMY to Modified Julian calculated with an extra subtraction of 2,415,019.
        auto nSerialDate = ((1'461 * (nYear + 4'800 + ((nMonth - 14) / 12))) / 4) +
                           ((367 * (nMonth - 2 - 12 * ((nMonth - 14) / 12))) / 12) -
                           ((3 * (((nYear + 4'900 + ((nMonth - 14) / 12)) / 100))) / 4) + nDay -
                           2'415'019 - 32'075;

        if (nSerialDate <= 60)
            {
            // Because of the 29-02-1900 quirk, any serial date
            // from 60 and below is one off...compensate.
            --nSerialDate;
            }

        return nSerialDate;
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::excel_serial_date_to_dmy(int nSerialDate, int& nDay, int& nMonth,
                                                     int& nYear)
        {
        nDay = nMonth = nYear = 0;

        // Excel and Lotus 123 have a legacy issue with 29-02-1900. 1900 is not a
        // leap year, but Excel and Lotus 123 treat it like it is.
        if (nSerialDate == 60)
            {
            nDay = 29;
            nMonth = 2;
            nYear = 1900;

            return;
            }
        if (nSerialDate < 60)
            {
            // Because of the 29-02-1900 issue, any serial date
            // under 60 is one off... Compensate.
            ++nSerialDate;
            }

        // Modified Julian to DMY calculation with an addition of 2,415,019
        auto l = nSerialDate + 68'569 + 2'415'019;
        auto n = ((4 * l) / 146'097);
        l = l - ((146'097 * n + 3) / 4);
        auto i = ((4'000 * (l + 1)) / 1'461'001);
        l = l - ((1'461 * i) / 4) + 31;
        auto j = ((80 * l) / 2'447);
        nDay = l - ((2'447 * j) / 80);
        l = (j / 11);
        nMonth = j + 2 - (12 * l);
        nYear = 100 * (n - 49) + i + l;
        }

    //------------------------------------------------------------------
    std::pair<bool, std::wstring> xlsx_string_table_parse::operator()()
        {
        if (m_html_text == nullptr || m_html_text[0] == 0)
            {
            return return_finished();
            }

        while ((m_html_text = html_extract_text::find_element(m_html_text, m_html_text_end, L"si",
                                                              true)) != nullptr)
            {
            const wchar_t* const siElement = html_extract_text::find_close_tag(m_html_text);
            // ill-formed string item, just return empty and move onto the next one.
            if (siElement == nullptr)
                {
                m_html_text += 3; // step over bad "<si" element
                return std::make_pair(true, std::wstring{});
                }
            m_html_text = siElement + 1;
            const wchar_t* const endTag =
                html_extract_text::find_closing_element(m_html_text, m_html_text_end, L"si");
            // no matching end for string item, so the rest of the table is
            // ill formed, so we're done.
            if (endTag == nullptr)
                {
                return return_finished();
                }
            const wchar_t* stringTag = m_html_text;
            std::wstring currentString;
            while (stringTag != nullptr && stringTag < endTag)
                {
                stringTag = html_extract_text::find_element(stringTag, endTag, L"t", true);
                if (stringTag == nullptr)
                    {
                    break;
                    }
                stringTag = html_extract_text::find_close_tag(stringTag);
                if (stringTag == nullptr)
                    {
                    break;
                    }
                const wchar_t* const endStringTag =
                    html_extract_text::find_closing_element(++stringTag, endTag, L"t");
                if (endStringTag == nullptr)
                    {
                    break;
                    }
                // read in the string
                if (endStringTag - stringTag > 0)
                    {
                    if ((m_html_extract(stringTag, endStringTag - stringTag, true, true) !=
                         nullptr) &&
                        (m_html_extract.get_filtered_text_length() != 0U))
                        {
                        currentString.append(m_html_extract.get_filtered_text(),
                                             m_html_extract.get_filtered_text_length());
                        }
                    }
                }
            m_html_text = endTag;
            if (m_removeNewlinesAndTabs)
                {
                // will also replace non-breaking spaces
                std::ranges::transform(currentString, currentString.begin(),
                                       [](auto& ch) noexcept
                                       {
                                           return (ch == L'\n' || ch == L'\r' || ch == L'\t' ||
                                                   ch == L'\u00A0') ?
                                                      L' ' :
                                                      ch;
                                       });
                }
            return std::make_pair(true, currentString);
            }
        return return_finished();
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::get_cell_text(const wchar_t* cellName,
                                                  const worksheet& workSheet)
        {
        const auto cinfo = get_column_and_row_info(cellName);
        if (cinfo.second < 1 || cinfo.second > workSheet.size())
            {
            return std::wstring{};
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
    std::wstring xlsx_extract_text::get_cell_text(const wchar_t* cell_name,
                                                  const wchar_t* shared_strings,
                                                  const size_t shared_strings_length,
                                                  const wchar_t* worksheet_text,
                                                  const size_t worksheet_length) const
        {
        assert(shared_strings_length == std::wcslen(shared_strings));
        assert(worksheet_length == std::wcslen(worksheet_text));
        const wchar_t* const worksheetEnd = worksheet_text + worksheet_length;
        while ((worksheet_text = html_extract_text::find_element(worksheet_text, worksheetEnd, L"c",
                                                                 true)) != nullptr)
            {
            if (html_extract_text::read_attribute_as_string(worksheet_text, L"r", false, false) ==
                cell_name)
                {
                const std::pair<const wchar_t*, size_t> typeTag =
                    html_extract_text::read_attribute(worksheet_text, L"t", false, false);
                if (typeTag.second == 1 && *typeTag.first == L's')
                    {
                    const wchar_t* const cellEnd =
                        html_extract_text::find_closing_element(worksheet_text, worksheetEnd, L"c");
                    // found the cell, but its cell ending tag is missing? return empty.
                    if (cellEnd == nullptr)
                        {
                        return {};
                        }
                    const wchar_t* value =
                        html_extract_text::find_element(worksheet_text, cellEnd, L"v", true);
                    if (value != nullptr &&
                        (value = html_extract_text::find_close_tag(value)) != nullptr)
                        {
                        const wchar_t* const valueEnd =
                            html_extract_text::find_closing_element(++value, cellEnd, L"v");
                        if (valueEnd != nullptr)
                            {
                            const std::wstring valueIndex(value, valueEnd - value);
                            if (!valueIndex.empty())
                                {
                                const int stringTableIndex =
                                    static_cast<int>(std::wcstol(valueIndex.c_str(), nullptr, 10));
                                return get_shared_string(stringTableIndex, shared_strings,
                                                         shared_strings_length);
                                }
                            }
                        // found the cell, but value section is messed up or empty, so return empty
                        return {};
                        }
                    // found the cell, but value section is messed up or missing, so return empty
                    return {};
                    }
                // found the cell, but its type isn't text? return empty
                return {};
                }
            ++worksheet_text;
            }
        return {};
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::get_text_cell_names(const worksheet& wrk,
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
    std::wstring xlsx_extract_text::get_worksheet_text(const worksheet& wrk,
                                                       const wchar_t delim /*= L'\t'*/)
        {
        std::wstring dataText;
        const auto cellCount{ xlsx_extract_text::get_cell_count(wrk) };
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
    void xlsx_extract_text::operator()(const wchar_t* html_text, const size_t text_length,
                                       worksheet& data)
        {
        data.clear();
        if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
            {
            return;
            }
        assert(text_length == std::wcslen(html_text));

        const wchar_t* const endSentinel = html_text + text_length;

        // If the size of the table is already specified, then size our table before reading in the
        // values. Sometimes, sparse data will cause the empty cells to not be listed in the file
        // explicitly, so we need to fill those in with blank cells here.
        const wchar_t* const dimension =
            html_extract_text::find_element(html_text, endSentinel, L"dimension", true);
        if (dimension != nullptr)
            {
            const std::wstring dimensionRef =
                html_extract_text::read_attribute_as_string(dimension, L"ref", false, false);
            const size_t colonIndex = dimensionRef.find(L':');
            if (colonIndex != std::wstring::npos)
                {
                const auto dataStart =
                    get_column_and_row_info(dimensionRef.substr(0, colonIndex).c_str());
                const auto dataEnd =
                    get_column_and_row_info(dimensionRef.substr(colonIndex + 1).c_str());
                if (dataStart.second < dataEnd.second)
                    {
                    data.resize(std::min((dataEnd.second - dataStart.second) + 1, EXCEL_MAX_ROWS));
                    const size_t columnCount =
                        std::min((dataEnd.first.m_position - dataStart.first.m_position) + 1,
                                 EXCEL_MAX_COLUMNS);
                    // fill the rows with empty data cells
                    // (that will have the column names already set)
                    if (dataStart.first.m_position <= dataEnd.first.m_position)
                        {
                        for (auto rowPos = data.begin(); rowPos != data.end(); ++rowPos)
                            {
                            rowPos->resize(columnCount);
                            for (size_t i = 0; i < columnCount; ++i)
                                {
                                rowPos->operator[](i) =
                                    worksheet_cell(i + 1, (rowPos - data.begin()) + 1);
                                }
                            }
                        }
                    }
                }
            }

        worksheet_row cRow;
        worksheet_cell currentCell;
        std::wstring valueStr;
        std::pair<const wchar_t*, size_t> typeTag;
        std::pair<const wchar_t*, size_t> styleTag;
        while ((html_text = html_extract_text::find_element(html_text, endSentinel, L"row",
                                                            true)) != nullptr)
            {
            cRow.clear();
            const size_t rowNum = static_cast<size_t>(std::wcstol(
                html_extract_text::read_attribute_as_string(html_text, L"r", true, false).c_str(),
                nullptr, 10));
            worksheet_row& currentRow =
                (rowNum != 0 && rowNum <= data.size()) ? data[rowNum - 1] : cRow;
            auto cellPos = currentRow.begin();

            const wchar_t* const rowEnd =
                html_extract_text::find_closing_element(html_text, endSentinel, L"row");
            // if <row> is self terminating, then it's a blank row; move to the end tag and
            // go to next row
            if (rowEnd == nullptr)
                {
                html_text = html_extract_text::find_close_tag(html_text);
                if (html_text == nullptr || html_text >= endSentinel)
                    {
                    break;
                    }
                continue;
                }

            while ((html_text = html_extract_text::find_element(html_text, rowEnd, L"c", true)) !=
                   nullptr)
                {
                currentCell.set_name(
                    html_extract_text::read_attribute_as_string(html_text, L"r", false, false));
                currentCell.set_value(std::wstring{});
                // read the type ('t') attribute
                typeTag = html_extract_text::read_attribute(html_text, L"t", false, false);
                // read the style ('s') attribute
                styleTag = html_extract_text::read_attribute(html_text, L"s", false, false);
                const wchar_t* const cellEnd =
                    html_extract_text::find_closing_element(html_text, rowEnd, L"c");
                if (cellEnd != nullptr)
                    {
                    // - 'inlineStr' (inline string)
                    if (typeTag.first != nullptr &&
                        std::wcsncmp(typeTag.first, L"inlineStr", 9) == 0)
                        {
                        const wchar_t* isTag =
                            html_extract_text::find_element(html_text, cellEnd, L"is", true);
                        if (isTag != nullptr &&
                            (isTag = html_extract_text::find_close_tag(isTag)) != nullptr)
                            {
                            const wchar_t* const isEnd =
                                html_extract_text::find_closing_element(++isTag, cellEnd, L"is");
                            if (isEnd != nullptr)
                                {
                                const wchar_t* tTag =
                                    html_extract_text::find_element(isTag, isEnd, L"t", true);
                                if (tTag != nullptr &&
                                    (tTag = html_extract_text::find_close_tag(tTag)) != nullptr)
                                    {
                                    const wchar_t* const tEnd =
                                        html_extract_text::find_closing_element(++tTag, cellEnd,
                                                                                L"t");
                                    if (tEnd != nullptr)
                                        {
                                        valueStr.assign(tTag, tEnd - tTag);
                                        // read a value
                                        if (!valueStr.empty())
                                            {
                                            currentCell.set_value(valueStr);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    else
                        {
                        const wchar_t* valueTag =
                            html_extract_text::find_element(html_text, cellEnd, L"v", true);
                        if ((valueTag != nullptr) &&
                            (valueTag = html_extract_text::find_close_tag(valueTag)) != nullptr)
                            {
                            const wchar_t* const valueEnd =
                                html_extract_text::find_closing_element(++valueTag, cellEnd, L"v");
                            if (valueEnd != nullptr)
                                {
                                valueStr.assign(valueTag, valueEnd - valueTag);
                                // read a value
                                if (!valueStr.empty())
                                    {
                                    // First, convert based on types...
                                    // -----------------------------
                                    // if a shared string (type == 's'), convert the value
                                    // (which is an index into the string table)
                                    if (typeTag.second == 1 && typeTag.first != nullptr &&
                                        *typeTag.first == L's')
                                        {
                                        const int stringTableIndex = static_cast<int>(
                                            std::wcstol(valueStr.c_str(), nullptr, 10));
                                        if (stringTableIndex >= 0 &&
                                            static_cast<size_t>(stringTableIndex) <
                                                get_shared_strings().size())
                                            {
                                            currentCell.set_value(
                                                get_shared_string(stringTableIndex));
                                            }
                                        }
                                    // - 'b' (boolean): will be 0 or 1, so convert to
                                    // 'FALSE' or 'TRUE'
                                    else if (typeTag.second == 1 && typeTag.first != nullptr &&
                                             *typeTag.first == L'b')
                                        {
                                        const bool bVal = static_cast<bool>(
                                            std::wcstol(valueStr.c_str(), nullptr, 10));
                                        currentCell.set_value(bVal ? _DT(L"TRUE") : _DT(L"FALSE"));
                                        }
                                    // - 'e' (error): an error message (e.g., "#DIV/0!");
                                    //                treat this as missing data.
                                    else if (typeTag.second == 1 && typeTag.first != nullptr &&
                                             *typeTag.first == L'e')
                                        {
                                        currentCell.set_value(std::wstring{});
                                        }
                                    // These other types will just have their values read:
                                    // - 'n' (number): read its value (and maybe convert to date
                                    //                 based on its style [see below])
                                    // - 'str' (string formula): read its (calculated value)
                                    // - 'inlineStr' (inline string): this was handled up to.

                                    // ...then on style
                                    else if (styleTag.first != nullptr)
                                        {
                                        const auto styleIndex =
                                            std::wcstol(styleTag.first, nullptr, 10);
                                        // a date?
                                        if (m_date_format_indices.contains(styleIndex))
                                            {
                                            const auto serialDate =
                                                std::wcstol(valueStr.c_str(), nullptr, 10);
                                            int day{ 0 }, month{ 0 }, year{ 0 };
                                            excel_serial_date_to_dmy(serialDate, day, month, year);

                                            // read the time component (if present)
                                            const auto dateTimeSepPos = valueStr.find(L'.');
                                            if (dateTimeSepPos == std::wstring::npos)
                                                {
                                                // convert to YYYY-MM-DD
                                                currentCell.set_value(std::to_wstring(year) + L"-" +
                                                                      std::to_wstring(month) +
                                                                      L"-" + std::to_wstring(day));
                                                }
                                            else
                                                {
                                                const auto percentStrLength =
                                                    (valueStr.length() - dateTimeSepPos - 1);
                                                auto timeOfDay = std::wcstoll(
                                                    valueStr.c_str() + dateTimeSepPos + 1, nullptr,
                                                    10);
                                                const long double timeOfDayPercent =
                                                    timeOfDay / std::pow(10, percentStrLength);
                                                constexpr auto SECONDS_IN_DAY = 24 * 60 * 60;
                                                auto secondsFromTime =
                                                    SECONDS_IN_DAY * timeOfDayPercent;
                                                const int hourOfDay = static_cast<int>(
                                                    std::floor(secondsFromTime /
                                                               static_cast<double>(60 * 60)));
                                                secondsFromTime -=
                                                    (static_cast<double>(hourOfDay) * 60 * 60);
                                                const int minutesOfHour = static_cast<int>(
                                                    std::floor(secondsFromTime / 60));
                                                secondsFromTime -= static_cast<int>(
                                                    std::floor(minutesOfHour * 60));

                                                // convert to YYYY-MM-DD HH:MM:SS
                                                currentCell.set_value(
                                                    std::to_wstring(year) + L"-" +
                                                    std::to_wstring(month) + L"-" +
                                                    std::to_wstring(day) + L" " +
                                                    std::to_wstring(hourOfDay) + L":" +
                                                    std::to_wstring(minutesOfHour) + L":" +
                                                    std::to_wstring(
                                                        static_cast<int>(secondsFromTime)));
                                                }
                                            }
                                        else
                                            {
                                            currentCell.set_value(valueStr);
                                            }
                                        }
                                    // just a value, so read that as-is
                                    else
                                        {
                                        currentCell.set_value(valueStr);
                                        }
                                    }
                                }
                            }
                        }
                    }

                if (cellPos == currentRow.end() || *cellPos != currentCell)
                    {
                    cellPos = std::lower_bound(currentRow.begin(), currentRow.end(), currentCell);
                    }
                // if cell was already in the row, then just update its value with what we just read
                if (cellPos != currentRow.end() && *cellPos == currentCell)
                    {
                    cellPos->set_value(currentCell.get_value());
                    ++cellPos;
                    }
                // or if cell is out of order, insert it where it should be
                else if (cellPos != currentRow.end())
                    {
                    cellPos = currentRow.insert(cellPos, currentCell);
                    ++cellPos;
                    }
                // or if cell wasn't in the row information yet, which happens if dimension info
                // wasn't known ahead of time
                else
                    {
                    currentRow.push_back(currentCell);
                    // structure of row is different now, invalidate this to force a new search
                    cellPos = currentRow.end();
                    }
                if (cellEnd != nullptr)
                    {
                    html_text = cellEnd + 3;
                    }
                else
                    {
                    html_text += 2;
                    }
                }
            if (rowNum == 0 || rowNum > data.size())
                {
                data.push_back(currentRow);
                }
            html_text = rowEnd + 5;
            }

        fix_jagged_sheet(data);
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::read_shared_strings(const wchar_t* text, const size_t text_length,
                                                const bool truncate /*= false*/)
        {
        m_shared_strings.clear();
        if ((text == nullptr) || text_length == 0)
            {
            return;
            }
        assert(text_length == std::wcslen(text));
        // see how many strings are in here and allocate space for them
        const wchar_t* countInfo =
            html_extract_text::find_element(text, (text + text_length), L"sst", true);
        if (countInfo != nullptr)
            {
            const size_t stringCount = static_cast<size_t>(std::wcstol(
                html_extract_text::read_attribute_as_string(countInfo, L"uniqueCount", false, false)
                    .c_str(),
                nullptr, 10));
            if (stringCount == 0)
                {
                m_shared_strings.reserve(1'000);
                }
            // in case file has nonsense in it, don't allocate more than
            // max number of rows for the strings table
            else
                {
                m_shared_strings.reserve(std::min<size_t>(stringCount, EXCEL_MAX_ROWS));
                }
            }
        xlsx_string_table_parse tableParse(text, text_length, m_removeNewlinesAndTabs);
        std::pair<bool, std::wstring> nextString;
        while ((nextString = tableParse()).first)
            {
            if (truncate && nextString.second.length() > 256)
                {
                nextString.second.replace(nextString.second.begin() + 253, nextString.second.end(),
                                          L"...");
                }
            m_shared_strings.push_back(nextString.second);
            }
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::read_styles(const wchar_t* text, const size_t text_length)
        {
        m_date_format_indices.clear();
        if ((text == nullptr) || text_length == 0)
            {
            return;
            }
        assert(text_length == std::wcslen(text));
        // load the custom number formats
        std::set<size_t> dateFormatIds;
        const wchar_t* customNumberFormats =
            html_extract_text::find_element(text, (text + text_length), L"numFmts", true);
        if (customNumberFormats != nullptr)
            {
            const wchar_t* const endTag = html_extract_text::find_closing_element(
                customNumberFormats, (text + text_length), L"numFmts");
            if (endTag != nullptr)
                {
                const wchar_t* stringTag = customNumberFormats;

                // strip [] and "" sections from the format string
                // ("these are literal strings and color formatting codes")
                const auto stripFormatString = [](std::wstring& str)
                {
                    //[]
                    {
                    const auto bracePos = str.find(L'[');
                    if (bracePos != std::wstring::npos)
                        {
                        const auto closeBrace = str.find(L']', bracePos + 1);
                        if (closeBrace != std::wstring::npos)
                            {
                            str.erase(bracePos, (closeBrace - bracePos) + 1);
                            }
                        }
                    }
                    //""
                    {
                    const auto bracePos = str.find(L'"');
                    if (bracePos != std::wstring::npos)
                        {
                        const auto closeBrace = str.find(L'"', bracePos + 1);
                        if (closeBrace != std::wstring::npos)
                            {
                            str.erase(bracePos, (closeBrace - bracePos) + 1);
                            }
                        }
                    }
                };

                while ((stringTag != nullptr) && stringTag < endTag)
                    {
                    stringTag = html_extract_text::find_element(stringTag, endTag, L"numFmt", true);
                    if (stringTag == nullptr)
                        {
                        break;
                        }

                    const auto fmtId = std::wcstol(html_extract_text::read_attribute_as_string(
                                                       stringTag, L"numFmtId", false, false)
                                                       .c_str(),
                                                   nullptr, 10);
                    std::wstring formatStr = html_extract_text::read_attribute_as_string(
                        stringTag, L"formatCode", false, false);
                    stripFormatString(formatStr);
                    // Custom format is some sort of date if it contains
                    // year, day, week, or quarter markers OR
                    // "mmm" which means month usually when day isn't included.
                    // (You can't simply look for 'm' as that can be minute when
                    // the format is just a time.)
                    if (formatStr.find_first_of(L"ydwq") != std::wstring::npos ||
                        formatStr.find(L"mmm") != std::wstring::npos)
                        {
                        dateFormatIds.insert(fmtId);
                        }
                    stringTag = html_extract_text::find_close_tag(stringTag);
                    }
                }
            }
        // read the cell styles
        const wchar_t* cellStyles =
            html_extract_text::find_element(text, (text + text_length), L"cellXfs", true);
        if (cellStyles != nullptr)
            {
            const wchar_t* const endTag = html_extract_text::find_closing_element(
                cellStyles, (text + text_length), L"cellXfs");
            if (endTag != nullptr)
                {
                const wchar_t* stringTag = cellStyles;
                size_t currentIndex{ 0 };
                while (stringTag != nullptr && stringTag < endTag)
                    {
                    stringTag = html_extract_text::find_element(stringTag, endTag, L"xf", true);
                    if (stringTag == nullptr)
                        {
                        break;
                        }

                    const auto fmtId = std::wcstol(html_extract_text::read_attribute_as_string(
                                                       stringTag, L"numFmtId", false, false)
                                                       .c_str(),
                                                   nullptr, 10);
                    // built-in formats known to be date formats, or a custom date format
                    if ((fmtId >= 14 && fmtId <= 17) || fmtId == 22 ||
                        dateFormatIds.contains(fmtId))
                        {
                        m_date_format_indices.insert(currentIndex);
                        }
                    stringTag = html_extract_text::find_close_tag(stringTag);
                    ++currentIndex;
                    }
                }
            }
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::read_worksheet_names(const wchar_t* text, const size_t text_length)
        {
        m_worksheet_names.clear();
        if (text == nullptr || text_length == 0)
            {
            return;
            }
        assert(text_length <= std::wcslen(text));
        const wchar_t* const textEnd = text + text_length;
        text = html_extract_text::find_element(text, textEnd, L"sheets", true);
        if (text == nullptr)
            {
            return;
            }
        const wchar_t* const sheetsEnd =
            html_extract_text::find_closing_element(text + 6, textEnd, L"sheets");
        if (sheetsEnd != nullptr)
            {
            // go through all the worksheet names
            while ((text = html_extract_text::find_element(text, textEnd, L"sheet", true)) !=
                   nullptr)
                {
                // read in the name of the current worksheet
                std::wstring worksheetName =
                    html_extract_text::read_attribute_as_string(text, L"name", false, true);
                if (!worksheetName.empty())
                    {
                    m_worksheet_names.push_back(std::move(worksheetName));
                    }
                text += 5;
                }
            }
        }

    //------------------------------------------------------------------
    std::pair<bool, std::wstring> xlsx_extract_text::verify_sheet(const worksheet& data)
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
    std::wstring xlsx_extract_text::column_index_to_column_name(size_t col)
        {
        if (column_info::INVALID_POSITION == col)
            {
            return std::wstring{};
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
    std::pair<xlsx_extract_text::column_info, size_t>
    xlsx_extract_text::get_column_and_row_info(const wchar_t* cell_name)
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
    void xlsx_extract_text::fix_jagged_sheet(worksheet& data)
        {
        if (data.empty())
            {
            return;
            }
        // See if the data is jagged and if so see which row has
        // the most cells (that is what the other rows will be resized to).
        size_t largestRow = data[0].size();
        bool isJagged = false;
        for (const auto& datum : data)
            {
            if (datum.size() != largestRow)
                {
                largestRow = std::max(datum.size(), largestRow);
                isJagged = true;
                }
            }
        // If we have jagged data, then try to fill in the blanks.
        if (isJagged)
            {
            for (size_t rowCounter = 0; rowCounter < data.size(); ++rowCounter)
                {
                // Missing some cells in the row?
                // Fill them in with blank ones, based on what cells seem to be missing.
                if (data[rowCounter].size() < largestRow)
                    {
                    for (size_t columnCounter = 0; columnCounter < largestRow; ++columnCounter)
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
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::get_shared_string(const size_t index, const wchar_t* text,
                                                      const size_t text_length) const
        {
        assert(text_length <= std::wcslen(text));
        xlsx_string_table_parse tableParse(text, text_length, m_removeNewlinesAndTabs);
        std::pair<bool, std::wstring> nextString;
        size_t i = 0;
        while ((nextString = tableParse()).first)
            {
            if (i == index)
                {
                return nextString.second;
                }
            ++i;
            }
        return std::wstring{};
        }

    //------------------------------------------------------------------
    std::pair<size_t, size_t> xlsx_extract_text::split_column_info(std::wstring_view cell_name)
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
