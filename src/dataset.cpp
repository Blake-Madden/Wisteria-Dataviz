///////////////////////////////////////////////////////////////////////////////
// Name:        dataset.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "dataset.h"

namespace Wisteria::Data
    {
    //----------------------------------------------
    double Dataset::ConvertToDouble(const wxString& input)
        {
        if (input.empty())
            { return std::numeric_limits<double>::quiet_NaN(); }
        else
            {
            double val{ 0 };
            if (input.ToCDouble(&val))
                { return val; }
            else
                { return std::numeric_limits<double>::quiet_NaN(); }
            }
        }

    //----------------------------------------------
    GroupIdType Dataset::ConvertToGroupId(const wxString& input)
        {
        if (input.empty())
            { return 0; }
        else
            {
            wchar_t* end{ nullptr };
            GroupIdType value = std::wcstoull(input.c_str(), &end, 10);
            return (input.c_str() == end) ? 0 : value;
            }
        }

    //----------------------------------------------
    wxDateTime Dataset::ConvertToDate(const wxString& input,
                                      const DateImportMethod method,
                                      const wxString& formatStr)
        {
        if (input.empty())
            { return wxInvalidDateTime; }

        wxDateTime dt;
        wxString::const_iterator end;
        switch (method)
            {
        case DateImportMethod::Automatic:
            // try reading as date & time, and fall back to just date if that fails
            if (!dt.ParseDateTime(input, &end))
                { dt.ParseDate(input, &end); }
            break;
        case DateImportMethod::IsoDate:
            dt.ParseISODate(input);
            break;
        case DateImportMethod::IsoCombined:
            dt.ParseISOCombined(input);
            break;
        case DateImportMethod::Rfc822:
            dt.ParseRfc822Date(input, &end);
            break;
        case DateImportMethod::StrptimeFormatString:
            dt.ParseFormat(input, formatStr, &end);
            break;
            }
        if (!dt.IsValid())
            { wxLogWarning(L"'%s': error parsing date.", input); }
        return dt;
        }

    //----------------------------------------------
    void Dataset::AddRow(const RowInfo& dataInfo)
        {
        m_idColumn.AddValue(dataInfo.m_id);
        // dates
        if (m_dateColumns.size() < dataInfo.m_dateColumns.size())
            { m_dateColumns.resize(dataInfo.m_dateColumns.size()); }
        for (size_t i = 0; i < dataInfo.m_dateColumns.size(); ++i)
            { m_dateColumns.at(i).AddValue(dataInfo.m_dateColumns.at(i)); }
        // categoricals
        if (m_categoricalColumns.size() < dataInfo.m_categoryValues.size())
            { m_categoricalColumns.resize(dataInfo.m_categoryValues.size()); }
        for (size_t i = 0; i < dataInfo.m_categoryValues.size(); ++i)
            { m_categoricalColumns.at(i).AddValue(dataInfo.m_categoryValues.at(i)); }
        // continuous columns
        if (m_continuousColumns.size() < dataInfo.m_continuousValues.size())
            { m_continuousColumns.resize(dataInfo.m_continuousValues.size()); }
        for (size_t i = 0; i < dataInfo.m_continuousValues.size(); ++i)
            { m_continuousColumns.at(i).AddValue(dataInfo.m_continuousValues.at(i)); }
        }

    //----------------------------------------------
    std::pair<double, double> Dataset::GetContinuousMinMax(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const GroupIdType groupId) const
        {
        auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend() ||
            !HasValidContinuousData(column, groupColumn, groupId))
            {
            return std::make_pair(std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::quiet_NaN());
            }

        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());

        auto minValue = std::numeric_limits<double>::max();
        auto maxValue = std::numeric_limits<double>::min();
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if ((groupColumnIterator == GetCategoricalColumns().cend()) ||
                groupColumnIterator->GetValue(i) == groupId)
                {
                minValue = std::min(minValue, continuousColumnIterator->GetValue(i));
                maxValue = std::max(maxValue, continuousColumnIterator->GetValue(i));
                }
            }
        return std::make_pair(minValue, maxValue);
        }

    //----------------------------------------------
    bool Dataset::HasValidContinuousData(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const GroupIdType groupId) const
        {
        auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            { return false; }

        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());

        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (groupColumnIterator != GetCategoricalColumns().cend())
                {
                if (groupColumnIterator->GetValue(i) == groupId &&
                    !std::isnan(continuousColumnIterator->GetValue(i)))
                    { return true; }
                }
            else
                {
                if (!std::isnan(continuousColumnIterator->GetValue(i)))
                    { return true; }
                }
            }
        return false;
        }

    //----------------------------------------------
    void Dataset::SetColumnNames(const ImportInfo& info)
        {
        if (info.m_idColumn.length())
            { GetIdColumn().SetTitle(info.m_idColumn); }
        // date columns
        if (m_dateColumns.size() < info.m_dateColumns.size())
            { m_dateColumns.resize(info.m_dateColumns.size()); }
        for (size_t i = 0; i < info.m_dateColumns.size(); ++i)
            { GetDateColumn(i).SetTitle(info.m_dateColumns.at(i).m_columnName); }
        // categorical columns
        if (m_categoricalColumns.size() < info.m_categoricalColumns.size())
            { m_categoricalColumns.resize(info.m_categoricalColumns.size()); }
        for (size_t i = 0; i < info.m_categoricalColumns.size(); ++i)
            { GetCategoricalColumn(i).SetTitle(info.m_categoricalColumns.at(i).m_columnName); }
        // continuous
        if (m_continuousColumns.size() < info.m_continuousColumns.size())
            { m_continuousColumns.resize(info.m_continuousColumns.size()); }
        for (size_t i = 0; i < info.m_continuousColumns.size(); ++i)
            { GetContinuousColumn(i).SetTitle(info.m_continuousColumns.at(i)); }
        }

    //----------------------------------------------
    Dataset::ColumnPreviewInfo Dataset::ReadColumnInfo(const wxString& filePath, const wchar_t delimiter,
                                                       const size_t rowPreviewCount /*= 100*/)
        {
        wxString fileText;
        wxFile fl(filePath);
        if (!fl.IsOpened() || !fl.ReadAll(&fileText))
            {
            throw std::runtime_error(wxString::Format(_(L"'%s':\n%s"), filePath,
                                     wxSysErrorMsg(fl.GetLastError())));
            }
        fileText.Trim(true).Trim(false);
        std::vector<std::vector<wxString>> dataStrings;

        lily_of_the_valley::text_matrix<wxString> importer{ &dataStrings };

        // skip the header
        lily_of_the_valley::standard_delimited_character_column
            noReadColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter, false });
        lily_of_the_valley::text_row<wxString> noReadRow{ 1 };
        noReadRow.add_column(noReadColumn);
        importer.add_row(noReadRow);

        lily_of_the_valley::standard_delimited_character_column
            deliminatedColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter });
        lily_of_the_valley::text_row<wxString> row;
        row.add_column(deliminatedColumn);
        importer.add_row(row);

        lily_of_the_valley::text_preview preview;
        std::vector<std::pair<wxString, ColumnImportType>> columnInfo;
        // read either first few rows or entire file, whichever is less
        size_t rowCount = std::min<size_t>(preview(fileText, delimiter, false, false),
                                           rowPreviewCount+1/*header*/);
        if (rowCount > 0)
            {
            dataStrings.resize(rowCount);
            importer.read(fileText, rowCount, preview.get_header_names().size(), false);
            }
        else
            { return columnInfo; }
        // ignore the first line (header) now
        --rowCount;

        for (size_t colIndex = 0; colIndex < preview.get_header_names().size(); ++colIndex)
            {
            // assume column's data is numeric unless something in the first
            // few rows look like a string
            ColumnImportType currentColumnType{ ColumnImportType::Numeric };
            for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
                {
                const auto& currentCell = dataStrings.at(rowIndex).at(colIndex);
                wxLogNull nl;
                if (currentCell.length() && ConvertToDate(currentCell, DateImportMethod::Automatic, L"").IsValid())
                    {
                    currentColumnType = ColumnImportType::Date;
                    break;
                    }
                if (currentCell.length() && std::isnan(ConvertToDouble(currentCell)))
                    {
                    currentColumnType = ColumnImportType::String;
                    break;
                    }
                }
            columnInfo.push_back(std::make_pair(
                preview.get_header_names().at(colIndex).c_str(),
                currentColumnType));
            }
        return columnInfo;
        }

    //----------------------------------------------
    void Dataset::ImportText(const wxString& filePath, const ImportInfo& info,
                             const wchar_t delimiter)
        {
        // reset
        Clear();
        m_dateColumns.clear();
        m_categoricalColumns.clear();
        m_continuousColumns.clear();

        wxString fileText;
        wxFile fl(filePath);
        if (!fl.IsOpened() || !fl.ReadAll(&fileText))
            {
            throw std::runtime_error(wxString::Format(_(L"'%s':\n%s"), filePath,
                                     wxSysErrorMsg(fl.GetLastError())));
            }
        fileText.Trim(true).Trim(false);
        std::vector<std::vector<wxString>> dataStrings;

        lily_of_the_valley::text_matrix<wxString> importer{ &dataStrings };

        // skip the header
        lily_of_the_valley::standard_delimited_character_column
            noReadColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter, false });
        lily_of_the_valley::text_row<wxString> noReadRow{ 1 };
        noReadRow.add_column(noReadColumn);
        importer.add_row(noReadRow);

        lily_of_the_valley::standard_delimited_character_column
            deliminatedColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter });
        lily_of_the_valley::text_row<wxString> row;
        row.add_column(deliminatedColumn);
        importer.add_row(row);

        lily_of_the_valley::text_preview preview;
        // see how many lines are in the file and resize the container
        const size_t rowCount = preview(fileText, delimiter, false, false);
        if (rowCount > 0)
            {
            dataStrings.resize(rowCount);
            importer.read(fileText, rowCount, preview.get_header_names().size(), false);
            Reserve(rowCount);
            }
        else
            { return; }

        // checks for columns client requested that aren't in the file
        const auto throwIfColumnNotFound = [&preview](const auto& columnName, const auto& foundIterator)
            {
            if (columnName.length() && foundIterator == preview.get_header_names().cend())
                {
                const wxString errorMsg = wxString::Format(L"'%s': column not found!", columnName.c_str());
                wxFAIL_MSG(errorMsg);
                throw std::runtime_error(errorMsg.ToStdString());
                }
            };

        // column index with specialized import method
        struct catIndexInfo
            {
            size_t m_index{ 0 };
            CategoricalImportMethod m_importMethod{ CategoricalImportMethod::ReadAsStrings };
            };

        // column index with specialized import method
        struct dateIndexInfo
            {
            size_t m_index{ 0 };
            DateImportMethod m_importMethod{ DateImportMethod::Automatic };
            wxString m_formatStr;
            };

        // find the column indices into the data that match the column names
        // from the client and map them as they requested
        const auto idColumnIter = std::find_if(preview.get_header_names().cbegin(),
            preview.get_header_names().cend(),
            [&info](const auto& item) noexcept
                { return info.m_idColumn.CmpNoCase(item.c_str()) == 0; });
        throwIfColumnNotFound(info.m_idColumn, idColumnIter);
        const std::optional<size_t> idColumnIndex = (idColumnIter != preview.get_header_names().cend()) ?
            std::optional<size_t>(idColumnIter - preview.get_header_names().cbegin()) : std::nullopt;

        // find the supplied date columns
        std::vector<std::optional<dateIndexInfo>> dateColumnIndices;
        for (const auto& dateColumn : info.m_dateColumns)
            {
            const auto dateColumnIter = std::find_if(preview.get_header_names().cbegin(),
                preview.get_header_names().cend(),
                [&dateColumn](const auto& item) noexcept
                    { return dateColumn.m_columnName.CmpNoCase(item.c_str()) == 0; });
            throwIfColumnNotFound(dateColumn.m_columnName, dateColumnIter);
            dateColumnIndices.push_back(
                (dateColumnIter != preview.get_header_names().cend()) ?
                std::optional<dateIndexInfo>(dateIndexInfo{
                    static_cast<size_t>(dateColumnIter - preview.get_header_names().cbegin()),
                    dateColumn.m_importMethod, dateColumn.m_strptimeFormatString }) :
                std::nullopt);
            }

        // find the supplied categorical columns
        std::vector<std::optional<catIndexInfo>> catColumnIndices;
        for (const auto& catColumn : info.m_categoricalColumns)
            {
            const auto catColumnIter = std::find_if(preview.get_header_names().cbegin(),
                preview.get_header_names().cend(),
                [&catColumn](const auto& item) noexcept
                    { return catColumn.m_columnName.CmpNoCase(item.c_str()) == 0; });
            throwIfColumnNotFound(catColumn.m_columnName, catColumnIter);
            catColumnIndices.push_back(
                (catColumnIter != preview.get_header_names().cend()) ?
                std::optional<catIndexInfo>(catIndexInfo{
                    static_cast<size_t>(catColumnIter - preview.get_header_names().cbegin()),
                                        catColumn.m_importMethod }) :
                std::nullopt);
            }

        // find the supplied continuous columns
        std::vector<std::optional<size_t>> continuousColumnIndices;
        for (const auto& continuousColumn : info.m_continuousColumns)
            {
            const auto continuousColumnIter = std::find_if(preview.get_header_names().cbegin(),
                preview.get_header_names().cend(),
                [&continuousColumn](const auto& item) noexcept
                    { return continuousColumn.CmpNoCase(item.c_str()) == 0; });
            throwIfColumnNotFound(continuousColumn, continuousColumnIter);
            continuousColumnIndices.push_back(
                (continuousColumnIter != preview.get_header_names().cend()) ?
                std::optional<size_t>(continuousColumnIter - preview.get_header_names().cbegin()) :
                std::nullopt);
            }

        // setup the string tables
        struct StringTableBuilder
            {
        public:
            [[nodiscard]] GroupIdType LoadCode(const wxString& code)
                {
                auto foundGroup = m_strings.find(code);
                if (foundGroup == m_strings.end())
                    {
                    foundGroup = m_strings.insert(
                        std::make_pair(code, currentId++)).first;
                    }
                return foundGroup->second;
                }
            [[nodiscard]] const auto& GetStrings() const noexcept
                { return m_strings; }
        private:
            GroupIdType currentId{ 0 };
            std::map<wxString, GroupIdType, StringCmpNoCase> m_strings;
            };

        StringTableBuilder groups;
        std::vector<StringTableBuilder> categoricalVars{ catColumnIndices.size() };

        // load the data
        RowInfo currentItem;
        std::vector<wxDateTime> dateValues;
        std::vector<Data::GroupIdType> catCodes;
        std::vector<double> continuousValues;
        for (const auto& currentRow : dataStrings)
            {
            // read in the values that client specified by column name

            // dates
            dateValues.clear();
            for (size_t i = 0; i < dateColumnIndices.size(); ++i)
                {
                if (dateColumnIndices.at(i))
                    {
                    const auto& currentDateInfo{ dateColumnIndices.at(i).value() };
                    dateValues.emplace_back(
                        ConvertToDate(currentRow.at(currentDateInfo.m_index),
                            currentDateInfo.m_importMethod, currentDateInfo.m_formatStr));
                    }
                }
            currentItem.Dates(dateValues);

            // categoricals
            catCodes.clear();
            for (size_t i = 0; i < catColumnIndices.size(); ++i)
                {
                if (catColumnIndices.at(i))
                    {
                    if (catColumnIndices.at(i).value().m_importMethod == CategoricalImportMethod::ReadAsStrings)
                        {
                        catCodes.emplace_back(
                            categoricalVars.at(i).LoadCode(
                                currentRow.at(catColumnIndices.at(i).value().m_index)));
                        }
                    else
                        {
                        catCodes.emplace_back(
                            ConvertToGroupId(currentRow.at(catColumnIndices.at(i).value().m_index)));
                        }
                    }
                }
            currentItem.Categoricals(catCodes);

            // continuous columns
            continuousValues.clear();
            for (size_t i = 0; i < continuousColumnIndices.size(); ++i)
                {
                if (continuousColumnIndices.at(i))
                    {
                    continuousValues.emplace_back(
                        ConvertToDouble(currentRow.at(continuousColumnIndices.at(i).value())));
                    }
                }
            currentItem.Continuous(continuousValues);

            // ID column
            if (idColumnIndex)
                { currentItem.Id(currentRow.at(idColumnIndex.value())); }
            AddRow(currentItem);
            }
        // set string tables for categoricals
        // (just applies for columns using CategoricalImportMethod::ReadAsStrings)
        for (size_t i = 0; i < categoricalVars.size(); ++i)
            {
            for (const auto& item : categoricalVars.at(i).GetStrings())
                {
                GetCategoricalColumn(i).GetStringTable().insert(
                    std::make_pair(item.second, item.first));
                }
            }

        // set the names for the columns
        SetColumnNames(info);
        }
    }
