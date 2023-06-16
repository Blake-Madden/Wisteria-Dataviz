///////////////////////////////////////////////////////////////////////////////
// Name:        xlsx_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "xlsx_extract_text.h"

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    int xlsx_extract_text::dmy_to_excel_serial_date(int nDay, int nMonth, int nYear)
        {
        // Excel and Lotus 123 have a legacy issue with 29-02-1900. 1900 is not a
        // leap year, but Excel and Lotus 123 treat it like it is...
        if (nDay == 29 && nMonth == 02 && nYear == 1900)
            { return 60; }

        // DMY to Modified Julian calculated with an extra subtraction of 2,415,019.
        auto nSerialDate = 
                int(( 1'461 * ( nYear + 4'800 + int(( nMonth - 14 ) / 12) ) ) / 4) +
                int(( 367 * ( nMonth - 2 - 12 * ( ( nMonth - 14 ) / 12 ) ) ) / 12) -
                int(( 3 * ( int(( nYear + 4'900 + int(( nMonth - 14 ) / 12) ) / 100) ) ) / 4) +
                nDay - 2'415'019 - 32'075;

        if (nSerialDate < 60)
            {
            // Because of the 29-02-1900 quirk, any serial date 
            // under 60 is one off... Compensate.
            --nSerialDate;
            }

        return nSerialDate;
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::excel_serial_date_to_dmy(int nSerialDate, int &nDay,
                                                     int &nMonth, int &nYear)
        {
        nDay = nMonth = nYear = 0;

        // Excel and Lotus 123 have a legacy issue with 29-02-1900. 1900 is not a
        // leap year, but Excel and Lotus 123 treat it like it is...
        if (nSerialDate == 60)
            {
            nDay = 29;
            nMonth = 2;
            nYear = 1900;

            return;
            }
        else if (nSerialDate < 60)
            {
            // Because of the 29-02-1900 issue, any serial date 
            // under 60 is one off... Compensate.
            ++nSerialDate;
            }

        // Modified Julian to DMY calculation with an addition of 2,415,019
        auto l = nSerialDate + 68'569 + 2'415'019;
        auto n = int(( 4 * l ) / 146'097);
                l = l - int(( 146'097 * n + 3 ) / 4);
        auto i = int(( 4'000 * ( l + 1 ) ) / 1'461'001);
            l = l - int(( 1'461 * i ) / 4) + 31;
        auto j = int(( 80 * l ) / 2'447);
            nDay = l - int(( 2'447 * j ) / 80);
            l = int(j / 11);
            nMonth = j + 2 - ( 12 * l );
        nYear = 100 * ( n - 49 ) + i + l;
        }

    //------------------------------------------------------------------
    const std::pair<bool,std::wstring> xlsx_string_table_parse::operator()()
        {
        if (!m_html_text || m_html_text[0] == 0)
            { return return_finished(); }

        while ((m_html_text = html_extract_text::find_element(m_html_text, m_html_text_end, L"si", 2)) != nullptr)
            {
            const wchar_t* const siElement = html_extract_text::find_close_tag(m_html_text);
            // ill-formed string item, just return empty and move onto the next one.
            if (!siElement)
                {
                m_html_text += 3; // step over bad "<si" element
                return std::make_pair(true, std::wstring{});
                }
            m_html_text = siElement+1;
            const wchar_t* const endTag = html_extract_text::find_closing_element(m_html_text, m_html_text_end, L"si", 2);
            // no matching end for string item, so the rest of the table is ill formed, so we're done.
            if (!endTag)
                { return return_finished(); }
            const wchar_t* stringTag = m_html_text;
            std::wstring currentString;
            while (stringTag && stringTag < endTag)
                {
                stringTag = html_extract_text::find_element(stringTag, endTag, L"t", 1);
                if (!stringTag)
                    { break; }
                stringTag = html_extract_text::find_close_tag(stringTag);
                if (!stringTag)
                    { break; }
                const wchar_t* const endStringTag =
                    html_extract_text::find_closing_element(++stringTag, endTag, L"t", 1);
                if (!endStringTag)
                    { break; }
                // read in the string
                if (endStringTag-stringTag > 0)
                    {
                    if (m_html_extract(stringTag, endStringTag-stringTag, true, true) &&
                        m_html_extract.get_filtered_text_length())
                        {
                        currentString.append(m_html_extract.get_filtered_text(),
                                             m_html_extract.get_filtered_text_length());
                        }
                    }
                }
            m_html_text = endTag;
            if (m_removeNewlinesAndTabs)
                {
                std::transform(currentString.begin(), currentString.end(), currentString.begin(),
                    [](auto& ch) noexcept
                    { return (ch == L'\n' || ch == L'\r' || ch == L'\t') ? L' ' : ch; });
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
            { return std::wstring{}; }
        const worksheet_row& currentRow = workSheet[cinfo.second-1];
        // out-of-range column index? Do a search for the cell by name.
        if (cinfo.first.m_position < 1 || cinfo.first.m_position > currentRow.size())
            {
            const auto cellPos = std::lower_bound(currentRow.begin(), currentRow.end(),
                                                  worksheet_cell(cellName));
            return (cellPos != currentRow.end() && cellPos->get_name() == cellName) ?
                cellPos->get_value() : std::wstring{};
            }
        if (currentRow.operator[](cinfo.first.m_position-1).get_name() == cellName)
            { return currentRow.operator[](cinfo.first.m_position-1).get_value(); }
        // If item at given index doesn't match by name, then there must have been
        // missing cells in the file (and no dimension info), so our matrix is sparse.
        // So brute force search for the cell by name.
        else
            {
            const auto cellPos = std::lower_bound(currentRow.cbegin(), currentRow.cend(),
                                                  worksheet_cell(cellName));
            return (cellPos != currentRow.cend() && cellPos->get_name() == cellName) ?
                cellPos->get_value() : std::wstring{};
            }
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::get_cell_text(const wchar_t* cell_name,
                                          const wchar_t* shared_strings, const size_t shared_strings_length,
                                          const wchar_t* worksheet_text, const size_t worksheet_length) const
        {
        assert(shared_strings_length == std::wcslen(shared_strings) );
        assert(worksheet_length == std::wcslen(worksheet_text) );
        const wchar_t* const worksheetEnd = worksheet_text+worksheet_length;
        std::pair<const wchar_t*, size_t> typeTag;
        while ((worksheet_text =
                html_extract_text::find_element(worksheet_text, worksheetEnd, L"c", 1)) != nullptr)
            {
            if (html_extract_text::read_attribute_as_string(worksheet_text, L"r", 1, false, false) == cell_name)
                {
                typeTag = html_extract_text::read_attribute(worksheet_text, L"t", 1, false, false);
                if (typeTag.second == 1 && *typeTag.first == L's')
                    {
                    const wchar_t* const cellEnd =
                        html_extract_text::find_closing_element(worksheet_text, worksheetEnd, L"c", 1);
                    // found the cell, but its cell ending tag is missing? return empty.
                    if (!cellEnd)
                        { return std::wstring{}; }
                    const wchar_t* value =
                        html_extract_text::find_element(worksheet_text, cellEnd, L"v", 1);
                    if (value && (value = html_extract_text::find_close_tag(value)) != nullptr)
                        {
                        const wchar_t* const valueEnd =
                            html_extract_text::find_closing_element(++value, cellEnd, L"v", 1);
                        if (valueEnd)
                            {
                            std::wstring valueIndex(value, valueEnd-value);
                            if (valueIndex.length())
                                {
                                const int stringTableIndex = string_util::atoi(valueIndex.c_str());
                                return get_shared_string(stringTableIndex, shared_strings, shared_strings_length);
                                }
                            }
                        // found the cell, but value section is messed up or empty, so return empty
                        return std::wstring{};
                        }
                    // found the cell, but value section is messed up or missing, so return empty
                    return std::wstring{};
                    }
                // found the cell, but its type isn't text? return empty
                else
                    { return std::wstring{}; }
                }
            ++worksheet_text;
            }
        return std::wstring{};
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::get_text_cell_names(const worksheet& wrk,
                                                std::vector<std::wstring>& cells)
        {
        cells.clear();
        if (wrk.size() == 0)
            { return; }
        // assume that a fourth the worksheet is text
        cells.reserve(wrk.size()*(wrk[0].size()/4));
        for (std::vector<worksheet_row>::const_iterator rowPos = wrk.begin();
            rowPos != wrk.end();
            ++rowPos)
            {
            for (worksheet_row::const_iterator cellPos = rowPos->begin();
                cellPos != rowPos->end();
                ++cellPos)
                {
                if (cellPos->get_value().length())
                    { cells.push_back(cellPos->get_name()); }
                }
            }
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::get_worksheet_text(
        const worksheet& wrk,
        const wchar_t delim /*= L'\t'*/)
        {
        std::wstring dataText;
        const auto cellCount{ xlsx_extract_text::get_cell_count(wrk) };
        if (cellCount == 0)
            { return std::wstring(); }

        dataText.reserve(cellCount * 5);
        for (const auto& row : wrk)
            {
            for (const auto& cell : row)
                { dataText.append(cell.get_value()).append(1, delim); }
            // make extra tab at the end a new line for the next row
            dataText.back() = L'\n';
            }
        // chop off extra newline at the end
        if (dataText.length() && dataText.back() == L'\n')
            { dataText.erase(dataText.end() - 1); }

        return dataText;
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::operator()(const wchar_t* html_text,
                                       const size_t text_length,
                                       worksheet& data)
        {
        data.clear();
        if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
            { return; }
        assert(text_length == std::wcslen(html_text) );

        const wchar_t* const endSentinel = html_text+text_length;

        // If the size of the table is already specified, then size our table before reading in the values.
        // Sometimes, sparse data will cause the empty cells to not be listed in the file explicitly, so we
        // need to fill those in with blank cells here.
        const wchar_t* const dimension =
            html_extract_text::find_element(html_text, endSentinel, L"dimension", 9);
        if (dimension)
            {
            std::wstring dimensionRef =
                html_extract_text::read_attribute_as_string(dimension, L"ref", 3, false);
            const size_t colonIndex = dimensionRef.find(L':');
            if (colonIndex != std::wstring::npos)
                {
                const auto dataStart =
                    get_column_and_row_info(dimensionRef.substr(0, colonIndex).c_str());
                const auto dataEnd =
                    get_column_and_row_info(dimensionRef.substr(colonIndex+1).c_str());
                if (dataStart.second < dataEnd.second)
                    {
                    data.resize(std::min((dataEnd.second-dataStart.second)+1, ExcelMaxRows));
                    const size_t columnCount =
                        std::min((dataEnd.first.m_position-dataStart.first.m_position)+1,
                                 ExcelMaxColumns);
                    // fill the rows with empty data cells
                    // (that will have the column names already set)
                    if (dataStart.first.m_position <= dataEnd.first.m_position)
                        {
                        for (worksheet::iterator rowPos = data.begin(); rowPos != data.end(); ++rowPos)
                            {
                            rowPos->resize(columnCount);
                            for (size_t i = 0; i < columnCount; ++i)
                                { rowPos->operator[](i) = worksheet_cell(i+1, (rowPos-data.begin())+1); }
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
        while ((html_text =
                html_extract_text::find_element(html_text, endSentinel, L"row", 3)) != nullptr)
            {
            cRow.clear();
            const size_t rowNum =
                string_util::atol(html_extract_text::read_attribute_as_string(html_text, L"r", 1, false).c_str());
            worksheet_row& currentRow = (rowNum != 0 && rowNum <= data.size()) ? data[rowNum-1] : cRow;
            worksheet_row::iterator cellPos = currentRow.begin();

            const wchar_t* const rowEnd =
                html_extract_text::find_closing_element(html_text, endSentinel, L"row", 3);
            // if <row> is self terminating, then it's a blank row; move to the end tag and
            // go to next row
            if (!rowEnd)
                {
                html_text = html_extract_text::find_close_tag(html_text);
                if (html_text == nullptr || html_text >= endSentinel)
                    { break; }
                continue;
                }

            while ((html_text =
                    html_extract_text::find_element(html_text, rowEnd, L"c", 1)) != nullptr)
                {
                currentCell.set_name(
                    html_extract_text::read_attribute_as_string(html_text, L"r", 1, false, false));
                currentCell.set_value(std::wstring{});
                // read the type ('t') attribute
                typeTag =
                    html_extract_text::read_attribute(html_text, L"t", 1, false, false);
                // read the style ('s') attribute
                styleTag =
                    html_extract_text::read_attribute(html_text, L"s", 1, false, false);
                const wchar_t* const cellEnd =
                    html_extract_text::find_closing_element(html_text, rowEnd, L"c", 1);
                if (cellEnd)
                    {
                    // - 'inlineStr' (inline string)
                    if (typeTag.first != nullptr &&
                        std::wcsncmp(typeTag.first, L"inlineStr", 9) == 0)
                        {
                        const wchar_t* isTag =
                            html_extract_text::find_element(html_text, cellEnd, L"is", 2);
                        if (isTag &&
                            (isTag = html_extract_text::find_close_tag(isTag)) != nullptr)
                            {
                            const wchar_t* const isEnd =
                                html_extract_text::find_closing_element(++isTag, cellEnd, L"is", 2);
                            if (isEnd)
                                {
                                const wchar_t* tTag =
                                    html_extract_text::find_element(isTag, isEnd, L"t", 1);
                                if (tTag &&
                                    (tTag = html_extract_text::find_close_tag(tTag)) != nullptr)
                                    {
                                    const wchar_t* const tEnd =
                                        html_extract_text::find_closing_element(++tTag, cellEnd, L"t", 1);
                                    if (tEnd)
                                        {
                                        valueStr.assign(tTag, tEnd-tTag);
                                        // read a value
                                        if (valueStr.length())
                                            { currentCell.set_value(valueStr);  }
                                        }
                                    }
                                }
                            }
                        }
                    else
                        {
                        const wchar_t* valueTag =
                            html_extract_text::find_element(html_text, cellEnd, L"v", 1);
                        if (valueTag &&
                            (valueTag = html_extract_text::find_close_tag(valueTag)) != nullptr)
                            {
                            const wchar_t* const valueEnd =
                                html_extract_text::find_closing_element(++valueTag, cellEnd, L"v", 1);
                            if (valueEnd)
                                {
                                valueStr.assign(valueTag, valueEnd-valueTag);
                                // read a value
                                if (valueStr.length())
                                    {
                                    // First, convert based on types...
                                    // -----------------------------
                                    // if a shared string (type == 's'), convert the value
                                    // (which is an index into the string table)
                                    if (typeTag.second == 1 && *typeTag.first == L's')
                                        {
                                        const int stringTableIndex =
                                            string_util::atoi(valueStr.c_str());
                                        if (stringTableIndex >= 0 &&
                                            static_cast<size_t>(stringTableIndex) <
                                                get_shared_strings().size())
                                            {
                                            currentCell.set_value(get_shared_string(stringTableIndex));
                                            }
                                        }
                                    // - 'b' (boolean): will be 0 or 1, so convert to 'FALSE' or 'TRUE'
                                    else if (typeTag.second == 1 && *typeTag.first == L'b')
                                        {
                                        const bool bVal =
                                            static_cast<bool>(string_util::atoi(valueStr.c_str()));
                                        if (bVal)
                                            {
                                            currentCell.set_value(bVal ? _DT(L"TRUE") : _DT(L"FALSE"));
                                            }
                                        }
                                    // - 'e' (error): an error message (e.g., "#DIV/0!");
                                    //                treat this as missing data.
                                    else if (typeTag.second == 1 && *typeTag.first == L'e')
                                        { currentCell.set_value(std::wstring{}); }
                                    // These other types will just have their values read:
                                    // - 'n' (number): read its value (and maybe convert to date
                                    //                 based on its style [see below])
                                    // - 'str' (string formula): read its (calculated value)
                                    // - 'inlineStr' (inline string): this was handled up to.

                                    // ...then on style
                                    else if (styleTag.first != nullptr)
                                        {
                                        const auto styleIndex = string_util::atol(styleTag.first);
                                        // a date?
                                        if (m_date_format_indices.find(styleIndex) !=
                                            m_date_format_indices.cend())
                                            {
                                            const auto serialDate = string_util::atol(valueStr.c_str());
                                            int day{ 0 }, month{ 0 }, year{ 0 };
                                            excel_serial_date_to_dmy(serialDate, day, month, year);

                                            // read the time component (if present)
                                            const auto dateTimeSepPos = valueStr.find(L'.');
                                            if (dateTimeSepPos == std::wstring::npos)
                                                {
                                                // convert to YYYY-MM-DD
                                                currentCell.set_value(
                                                    std::to_wstring(year) + L"-" +
                                                    std::to_wstring(month) + L"-" +
                                                    std::to_wstring(day));
                                                }
                                            else
                                                {
                                                const auto percentStrLength =
                                                    (valueStr.length() - dateTimeSepPos - 1);
                                                wchar_t* dummy{ nullptr };
                                                auto timeOfDay =
                                                    std::wcstoll(valueStr.c_str() + dateTimeSepPos + 1, &dummy, 10);
                                                long double timeOfDayPercent =
                                                    timeOfDay / std::pow(10, percentStrLength);
                                                constexpr auto secondsInDay = 24 * 60 * 60;
                                                auto secondsFromTime = secondsInDay * timeOfDayPercent;
                                                const int hourOfDay =
                                                    static_cast<int>(std::floor(secondsFromTime /
                                                        static_cast<double>(60 * 60)));
                                                secondsFromTime -= (static_cast<double>(hourOfDay) * 60 * 60);
                                                const int minutesOfHour =
                                                    static_cast<int>(std::floor(secondsFromTime / 60));
                                                secondsFromTime -= static_cast<int>(std::floor(minutesOfHour * 60));

                                                // convert to YYYY-MM-DD HH:MM:SS
                                                currentCell.set_value(
                                                    std::to_wstring(year) + L"-" +
                                                    std::to_wstring(month) + L"-" +
                                                    std::to_wstring(day) + L" " +
                                                    std::to_wstring(hourOfDay) + L":" +
                                                    std::to_wstring(minutesOfHour) + L":" +
                                                    std::to_wstring(static_cast<int>(secondsFromTime)));
                                                }
                                            }
                                        else
                                            { currentCell.set_value(valueStr); }
                                        }
                                    // just a value, so read that as-is
                                    else
                                        { currentCell.set_value(valueStr); }
                                    }
                                }
                            }
                        }
                    }

                if (cellPos == currentRow.end() || *cellPos != currentCell)
                    { cellPos = std::lower_bound(currentRow.begin(), currentRow.end(), currentCell); }
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
                if (cellEnd)
                    { html_text = cellEnd+3; }
                else
                    { html_text += 2; }
                }
            if (rowNum == 0 || rowNum > data.size())
                { data.push_back(currentRow); }
            html_text = rowEnd+5;
            }

        fix_jagged_sheet(data);
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::read_shared_strings(const wchar_t* text, const size_t text_length,
                                                const bool truncate /*= false*/)
        {
        m_shared_strings.clear();
        if (!text || text_length == 0)
            { return; }
        assert(text_length == std::wcslen(text) );
        // see how many strings are in here and allocate space for them
        const wchar_t* countInfo =
            html_extract_text::find_element(text, (text+text_length), L"sst", 3);
        if (countInfo)
            {
            const size_t stringCount =
                string_util::atol(
                    html_extract_text::read_attribute_as_string(countInfo, L"uniqueCount", 11, false).c_str());
            if (stringCount == 0)
                { m_shared_strings.reserve(1'000); }
            // in case file has nonsense in it, don't allocate more than
            // max number of rows for the strings table
            else
                { m_shared_strings.reserve(std::min<size_t>(stringCount,ExcelMaxRows)); }
            }
        xlsx_string_table_parse tableParse(text, text_length, m_removeNewlinesAndTabs);
        std::pair<bool,std::wstring> nextString;
        while ((nextString = tableParse()).first)
            {
            if (truncate && nextString.second.length() > 256)
                {
                nextString.second.replace(nextString.second.begin() + 253,
                    nextString.second.end(), L"...");
                }
            m_shared_strings.push_back(nextString.second);
            }
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::read_styles(const wchar_t* text, const size_t text_length)
        {
        m_date_format_indices.clear();
        if (!text || text_length == 0)
            { return; }
        assert(text_length == std::wcslen(text) );
        // load the custom number formats
        std::set<size_t> dateFormatIds;
        const wchar_t* customNumberFormats =
            html_extract_text::find_element(text, (text+text_length), L"numFmts", 7);
        if (customNumberFormats)
            {
            const wchar_t* const endTag =
                html_extract_text::find_closing_element(customNumberFormats,
                                                        (text + text_length), L"numFmts", 7);
            if (endTag)
                {
                const wchar_t* stringTag = customNumberFormats;
                std::wstring formatStr;

                // strip [] and "" sections from the format string
                // ("these are literal strings and color formatting codes")
                const auto stripFormatString = [](std::wstring& str)
                    {
                    //[]
                        {
                        const auto bracePos = str.find(L'[');
                        if (bracePos != std::wstring::npos)
                            {
                            const auto closeBrace = str.find(L']', bracePos+1);
                            if (closeBrace != std::wstring::npos)
                                { str.erase(bracePos, (closeBrace - bracePos)+1); }
                            }
                        }
                    //""
                        {
                        const auto bracePos = str.find(L'"');
                        if (bracePos != std::wstring::npos)
                            {
                            const auto closeBrace = str.find(L'"', bracePos+1);
                            if (closeBrace != std::wstring::npos)
                                { str.erase(bracePos, (closeBrace - bracePos)+1); }
                            }
                        }
                    };

                while (stringTag && stringTag < endTag)
                    {
                    stringTag = html_extract_text::find_element(stringTag, endTag, L"numFmt", 6);
                    if (!stringTag)
                        { break; }

                    const auto fmtId =
                        string_util::atol(
                            html_extract_text::read_attribute_as_string(stringTag, L"numFmtId",
                                                                        8, false).c_str());
                    formatStr = html_extract_text::read_attribute_as_string(
                        stringTag, L"formatCode", 10, false);
                    stripFormatString(formatStr);
                    // Custom format is some sort of date if it contains
                    // year, day, week, or quarter markers OR
                    // "mmm" which means month usually when day isn't included.
                    // (You can't simply look for 'm' as that can be minute when
                    // the format is just a time.)
                    if (formatStr.find_first_of(L"ydwq") != std::wstring::npos ||
                        formatStr.find(L"mmm") != std::wstring::npos)
                        { dateFormatIds.insert(fmtId); }
                    stringTag = html_extract_text::find_close_tag(stringTag);
                    }
                }
            }
        // read the cell styles
        const wchar_t* cellStyles =
            html_extract_text::find_element(text, (text+text_length), L"cellXfs", 7);
        if (cellStyles)
            {
            const wchar_t* const endTag =
                html_extract_text::find_closing_element(cellStyles, (text+text_length), L"cellXfs", 7);
            if (endTag)
                {
                const wchar_t* stringTag = cellStyles;
                size_t currentIndex{ 0 };
                while (stringTag && stringTag < endTag)
                    {
                    stringTag = html_extract_text::find_element(stringTag, endTag, L"xf", 2);
                    if (!stringTag)
                        { break; }

                    const auto fmtId =
                        string_util::atol(
                            html_extract_text::read_attribute_as_string(stringTag, L"numFmtId",
                                                                        8, false).c_str());
                    // built-in formats known to be date formats, or a custom date format
                    if ((fmtId >= 14 && fmtId <= 17) || fmtId == 22 ||
                        dateFormatIds.find(fmtId) != dateFormatIds.cend())
                        { m_date_format_indices.insert(currentIndex); }
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
        if (!text || text_length == 0)
            { return; }
        assert(text_length <= std::wcslen(text) );
        const wchar_t* const textEnd = text+text_length;
        text = html_extract_text::find_element(text, textEnd, L"sheets", 6);
        if (!text)
            { return; }
        const wchar_t* const sheetsEnd =
            html_extract_text::find_closing_element(text+6, textEnd, L"sheets", 6);
        if (sheetsEnd)
            {
            // go through all of the worksheet names
            while ((text =
                   html_extract_text::find_element(text, textEnd, L"sheet", 5)) != nullptr)
                {
                // read in the name of the current worksheet
                std::wstring worksheetName =
                    html_extract_text::read_attribute_as_string(text, L"name", 4, false, true);
                if (!worksheetName.empty())
                    { m_worksheet_names.push_back(worksheetName); }
                text += 5;
                }
            }
        }

    //------------------------------------------------------------------
    std::pair<bool,std::wstring> xlsx_extract_text::verify_sheet(const worksheet& data)
        {
        for (size_t rowCounter = 0; rowCounter < data.size(); ++rowCounter)
            {
            for (size_t columnCounter = 0;
                 columnCounter < data[rowCounter].size();
                 ++columnCounter)
                {
                const worksheet_cell currentCell(column_index_to_column_name(columnCounter+1) +
                                                 std::to_wstring(rowCounter+1));
                const auto cellPos = std::lower_bound(data[rowCounter].cbegin(),
                                                      data[rowCounter].cend(), currentCell);
                // if cell was already in the row, then move on
                if (cellPos != data[rowCounter].cend() && *cellPos == currentCell)
                    { continue; }
                else
                    { return std::make_pair(false, currentCell.get_name()); }
                }
            }
        return std::make_pair(true, std::wstring{});
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::column_index_to_column_name(size_t col)
        {
        if (column_info::invalid_position == col)
            { return std::wstring{}; }
        std::wstring columnName;
        constexpr auto alphabetSize{ 26 };
        while (col > 0)
            {
            const wchar_t modulo = (col-1) % alphabetSize;
            columnName.insert(columnName.begin(), 1, L'A' + modulo);
            col = ((col-modulo)/alphabetSize);
            }
        return columnName;
        }

    //------------------------------------------------------------------
    std::pair<xlsx_extract_text::column_info,size_t> xlsx_extract_text::get_column_and_row_info(
        const wchar_t* cell_name)
        {
        assert(cell_name);
        if (cell_name == nullptr)
            { return std::make_pair(column_info(), column_info::invalid_position); }
        std::pair<size_t,size_t> cellInfo = split_column_info(cell_name);
        if (cellInfo.first == worksheet_cell::invalid_index)
            { return std::make_pair(column_info(), column_info::invalid_position); }

        column_info cinfo(0);
        for (size_t i = 0; i < cellInfo.first; ++i)
            {
            const wchar_t currentLetter = std::towupper(cell_name[i]);
            // if non letter in column name, then something is wrong;
            // set the column to bogus and quit
            if (!(currentLetter >= L'A' && currentLetter <= L'Z'))
                {
                cinfo.m_position = column_info::invalid_position;
                break;
                }
            // change 'A' (value 65) to 1, 'B' to 2, etc.
            const wchar_t letterVal = currentLetter - 64;
            cinfo.m_position +=
                letterVal *
                static_cast<size_t>(std::pow(26,static_cast<double>(cellInfo.first-1-i)));
            }

        return std::make_pair(cinfo, cellInfo.second);
        }

    //------------------------------------------------------------------
    void xlsx_extract_text::fix_jagged_sheet(worksheet& data)
        {
        if (data.size() == 0)
            { return; }
        // See if the data is jagged and if so see which row has
        // the most cells (that is what the other rows will be resized to).
        size_t largestRow = data[0].size();
        bool isJagged = false;
        /// @todo maybe make this parallel
        for (size_t i = 0; i < data.size(); ++i)
            {
            if (data[i].size() != largestRow)
                {
                largestRow = std::max(data[i].size(), largestRow);
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
                        const worksheet_cell currentCell(column_index_to_column_name(columnCounter+1) +
                                                         std::to_wstring(rowCounter+1));
                        const auto cellPos = std::lower_bound(data[rowCounter].begin(),
                                                              data[rowCounter].end(), currentCell);
                        // if cell was already in the row, then move on
                        if (cellPos != data[rowCounter].end() && *cellPos == currentCell)
                            { continue; }
                        else if (cellPos != data[rowCounter].end())
                            { data[rowCounter].insert(cellPos, currentCell); }
                        else
                            { data[rowCounter].push_back(currentCell); }
                        }
                    }
                }
            }
        }

    //------------------------------------------------------------------
    std::wstring xlsx_extract_text::get_shared_string(const size_t index, const wchar_t* text,
                                                      const size_t text_length) const
        {
        assert(text_length <= std::wcslen(text) );
        xlsx_string_table_parse tableParse(text, text_length, m_removeNewlinesAndTabs);
        std::pair<bool,std::wstring> nextString;
        size_t i = 0;
        while ((nextString = tableParse()).first)
            {
            if (i == index)
                { return nextString.second; }
            ++i;
            }
        return std::wstring{};
        }

    //------------------------------------------------------------------
    std::pair<size_t,size_t> xlsx_extract_text::split_column_info(const wchar_t* cell_name)
        {
        assert(cell_name);
        size_t numStart = string_util::find_last_not_of(cell_name, L"0123456789");
        if (numStart == std::wstring::npos ||
            cell_name[numStart] == 0 || cell_name[numStart+1] == 0)
            { return std::make_pair(-1, 0); }
        const size_t row = string_util::atol(cell_name+(++numStart));
        if (row == 0)
            { return std::make_pair(-1, 0); }
        return std::make_pair(numStart, row);
        }
    }
