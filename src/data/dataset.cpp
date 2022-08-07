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
    ImportInfo::RegExMap ImportInfo::DatasetToRegExMap(const std::shared_ptr<Dataset>& dataset,
        const wxString& regexColumnName,
        const wxString& replacementColumnName)
        {
        const auto regexColumn = dataset->GetCategoricalColumn(regexColumnName);
        if (regexColumn == dataset->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': regex column not found."), regexColumnName).ToUTF8());
            }
        const auto replaceColumn = dataset->GetCategoricalColumn(replacementColumnName);
        if (replaceColumn == dataset->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': regex column not found."), replacementColumnName).ToUTF8());
            }

        ImportInfo::RegExMap reMap;
        wxString currentRegex;
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            currentRegex = regexColumn->GetCategoryLabelFromID(regexColumn->GetValue(i));
            wxRegEx testRe(currentRegex);
            if (currentRegex.empty() || !testRe.IsValid())
                {
                wxLogWarning(_(L"'%s': regular expression syntax error."), currentRegex);
                continue;
                }
            reMap.insert(std::make_pair(
                std::make_shared<wxRegEx>(currentRegex),
                replaceColumn->GetCategoryLabelFromID(replaceColumn->GetValue(i))));
            }

        return reMap;
        }

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
    GroupIdType Dataset::ConvertToGroupId(const wxString& input, const GroupIdType mdCode)
        {
        if (input.empty())
            { return mdCode; }
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
            { wxLogWarning(_(L"'%s': error parsing date."), input); }
        return dt;
        }

    //----------------------------------------------
    void Dataset::AddRow(const RowInfo& dataInfo)
        {
        // add new columns if included in row info but not previously defined

        // dates
        if (m_dateColumns.size() < dataInfo.m_dateColumns.size())
            {
            const auto columnsToAdd = dataInfo.m_dateColumns.size() - m_dateColumns.size();
            // try to add a descriptive and unique name as best as we can
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddDateColumn(wxString::Format(L"[DATE%zu]", i+1)); }
            }
        // continuous
        if (m_continuousColumns.size() < dataInfo.m_continuousValues.size())
            {
            const auto columnsToAdd = dataInfo.m_continuousValues.size() - m_continuousColumns.size();
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddContinuousColumn(wxString::Format(L"[CONTINUOUS%zu]", i+1)); }
            }
        // categoricals
        if (m_categoricalColumns.size() < dataInfo.m_categoryValues.size())
            {
            const auto columnsToAdd = dataInfo.m_categoryValues.size() - m_categoricalColumns.size();
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddCategoricalColumn(wxString::Format(L"[CATEGORICAL%zu]", i+1)); }
            }

        // fill the values

        // ID
        m_idColumn.AddValue(dataInfo.m_id);
        // dates
        for (size_t i = 0; i < dataInfo.m_dateColumns.size(); ++i)
            { m_dateColumns.at(i).AddValue(dataInfo.m_dateColumns.at(i)); }
        // categoricals
        for (size_t i = 0; i < dataInfo.m_categoryValues.size(); ++i)
            { m_categoricalColumns.at(i).AddValue(dataInfo.m_categoryValues.at(i)); }
        // continuous columns
        for (size_t i = 0; i < dataInfo.m_continuousValues.size(); ++i)
            { m_continuousColumns.at(i).AddValue(dataInfo.m_continuousValues.at(i)); }
        }

    //----------------------------------------------
    std::pair<wxString, wxString> Dataset::GetCategoricalMinMax(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        // check column being analyzed
        const auto catColumnIterator = GetCategoricalColumn(column);
        if (catColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found when calculating valid N."), column).ToUTF8());
            }

        // check grouping parameters
        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());
        wxASSERT_MSG(!groupColumn || groupId,
                 L"Group ID must be provided if using grouping for GetContinuousColumnValidN()!");
        if (groupColumn && groupColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping column not found when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }
        if (groupColumn && !groupId)
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping ID not specified for column when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }

        // No rows or all empty? Then return a range of empties
        if (GetCategoricalColumnValidN(column, groupColumn, groupId) == 0)
            { return std::make_pair(wxEmptyString, wxEmptyString); }

        const auto MDCode = ColumnWithStringTable::FindMissingDataCode(
            catColumnIterator->GetStringTable());
        std::set<wxString, StringCmpNoCase> strings;
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if ((!MDCode.has_value() ||
                 catColumnIterator->GetValue(i) != MDCode.value()) &&
                ((groupColumnIterator == GetCategoricalColumns().cend()) ||
                  groupColumnIterator->GetValue(i) == groupId.value()))
                {
                strings.insert(
                    catColumnIterator->GetCategoryLabelFromID(catColumnIterator->GetValue(i)));
                }
            }
        if (strings.empty())
            { return std::make_pair(wxEmptyString, wxEmptyString); }

        // if there are strings, then return their min and max
        // (the first and last elements in the already sorted set)
        return std::make_pair(*strings.cbegin(), *strings.crbegin());
        }

    //----------------------------------------------
    size_t Dataset::GetCategoricalColumnValidN(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        // check column being analyzed
        const auto catColumnIterator = GetCategoricalColumn(column);
        if (catColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found when calculating valid N."), column).ToUTF8());
            }

        // check grouping parameters
        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());
        wxASSERT_MSG(!groupColumn || groupId,
                 L"Group ID must be provided if using grouping for GetCategoricalColumnValidN()!");
        if (groupColumn && groupColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping column not found when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }
        if (groupColumn && !groupId)
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping ID not specified for column when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }

        size_t validN{ 0 };
        const auto MDCode = ColumnWithStringTable::FindMissingDataCode(
            catColumnIterator->GetStringTable());
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (groupColumnIterator != GetCategoricalColumns().cend())
                {
                if (groupColumnIterator->GetValue(i) == groupId.value() &&
                    (!MDCode.has_value() ||
                     catColumnIterator->GetValue(i) != MDCode.value()) )
                    { ++validN; }
                }
            else
                {
                if (!MDCode.has_value() ||
                    catColumnIterator->GetValue(i) != MDCode.value())
                    { ++validN; }
                }
            }
        return validN;
        }

    //----------------------------------------------
    std::pair<double, double> Dataset::GetContinuousMinMax(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        // check column being analyzed
        const auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found when calculating valid N."), column).ToUTF8());
            }

        // check grouping parameters
        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());
        wxASSERT_MSG(!groupColumn || groupId,
                 L"Group ID must be provided if using grouping for GetContinuousColumnValidN()!");
        if (groupColumn && groupColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping column not found when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }
        if (groupColumn && !groupId)
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping ID not specified for column when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }

        // No rows or all NaN? Then return a range of NaNs.
        if (GetContinuousColumnValidN(column, groupColumn, groupId) == 0)
            {
            return std::make_pair(std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::quiet_NaN());
            }

        auto minValue = std::numeric_limits<double>::max();
        auto maxValue = std::numeric_limits<double>::lowest();
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (!std::isnan(continuousColumnIterator->GetValue(i)) &&
                ((groupColumnIterator == GetCategoricalColumns().cend()) ||
                groupColumnIterator->GetValue(i) == groupId.value()))
                {
                minValue = std::min(minValue, continuousColumnIterator->GetValue(i));
                maxValue = std::max(maxValue, continuousColumnIterator->GetValue(i));
                }
            }
        return std::make_pair(minValue, maxValue);
        }

    //----------------------------------------------
    size_t Dataset::GetContinuousColumnValidN(const wxString& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        // check column being analyzed
        const auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found when calculating valid N."), column).ToUTF8());
            }

        // check grouping parameters
        const auto groupColumnIterator = (groupColumn.has_value() ?
            GetCategoricalColumn(groupColumn.value()) : GetCategoricalColumns().cend());
        wxASSERT_MSG(!groupColumn || groupId,
                 L"Group ID must be provided if using grouping for GetContinuousColumnValidN()!");
        if (groupColumn && groupColumnIterator == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping column not found when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }
        if (groupColumn && !groupId)
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': grouping ID not specified for column when calculating valid N."),
                groupColumn.value()).ToUTF8());
            }

        size_t validN{ 0 };
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (groupColumnIterator != GetCategoricalColumns().cend())
                {
                if (groupColumnIterator->GetValue(i) == groupId.value() &&
                    !std::isnan(continuousColumnIterator->GetValue(i)))
                    { ++validN; }
                }
            else
                {
                if (!std::isnan(continuousColumnIterator->GetValue(i)))
                    { ++validN; }
                }
            }
        return validN;
        }

    //----------------------------------------------
    bool Dataset::HasValidIdData() const
        {
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (GetIdColumn().GetValue(i).length())
                { return true; }
            }
        return false;
        }

    //----------------------------------------------
    void Dataset::SetColumnNames(const ImportInfo& info)
        {
        // add new columns if included in row info but not previously defined

        // dates
        if (m_dateColumns.size() < info.m_dateColumns.size())
            {
            const auto columnsToAdd = info.m_dateColumns.size() - m_dateColumns.size();
            // temporary placeholder name
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddDateColumn(wxString::Format(L"[DATE%zu]", i+1)); }
            }
        // continuous
        if (m_continuousColumns.size() < info.m_continuousColumns.size())
            {
            const auto columnsToAdd = info.m_continuousColumns.size() - m_continuousColumns.size();
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddContinuousColumn(wxString::Format(L"[CONTINUOUS%zu]", i+1)); }
            }
        // categoricals
        if (m_categoricalColumns.size() < info.m_categoricalColumns.size())
            {
            const auto columnsToAdd = info.m_categoricalColumns.size() - m_categoricalColumns.size();
            for (size_t i = 0; i < columnsToAdd; ++i)
                { AddCategoricalColumn(wxString::Format(L"[CATEGORICAL%zu]", i+1)); }
            }

        if (info.m_idColumn.length())
            { GetIdColumn().SetName(info.m_idColumn); }
        // date columns
        for (size_t i = 0; i < info.m_dateColumns.size(); ++i)
            { GetDateColumn(i).SetName(info.m_dateColumns.at(i).m_columnName); }
        // categorical columns
        for (size_t i = 0; i < info.m_categoricalColumns.size(); ++i)
            { GetCategoricalColumn(i).SetName(info.m_categoricalColumns.at(i).m_columnName); }
        // continuous
        for (size_t i = 0; i < info.m_continuousColumns.size(); ++i)
            { GetContinuousColumn(i).SetName(info.m_continuousColumns.at(i)); }
        }

    //----------------------------------------------
    ColumnWithStringTable& Dataset::AddCategoricalColumn(const wxString& columnName)
        {
        wxASSERT_MSG(columnName.length(),
            L"Column name is empty in call to AddCategoricalColumn()!");
        m_categoricalColumns.resize(m_categoricalColumns.size()+1);
        m_categoricalColumns.back().SetName(columnName);
        // add a string table with an empty value and fill the data with that
        // if there are existing rows in the data
        if (GetRowCount())
            {
            m_categoricalColumns.back().GetStringTable().
                insert(std::make_pair(0, wxString()));
            m_categoricalColumns.back().Resize(GetRowCount(), 0);
            }
        return m_categoricalColumns.back();
        }

    //----------------------------------------------
    ColumnWithStringTable& Dataset::AddCategoricalColumn(const wxString& columnName,
        const ColumnWithStringTable::StringTableType& stringTable)
        {
        wxASSERT_MSG(columnName.length(),
            L"Column name is empty in call to AddCategoricalColumn()!");
        m_categoricalColumns.resize(m_categoricalColumns.size()+1);
        m_categoricalColumns.back().SetName(columnName);
        m_categoricalColumns.back().GetStringTable() = stringTable;
        // if we have existing rows and need to fill this column
        if (GetRowCount())
            {
            if (stringTable.size() == 0)
                {
                m_categoricalColumns.back().GetStringTable().
                    insert(std::make_pair(0, wxString()));
                m_categoricalColumns.back().Resize(GetRowCount(), 0);
                }
            else
                {
                // find the key with an empty string connected to it and fill
                // the new rows with that key
                for (const auto& [key, value] : stringTable)
                    {
                    if (value.empty())
                        {
                        m_categoricalColumns.back().Resize(GetRowCount(), key);
                        return m_categoricalColumns.back();
                        }
                    }
                // no empty string in string table, so add one (with an ID one
                // higher than the last one) and fill the existing rows with that
                const auto& [lastKey, lastValue] = *stringTable.crbegin();
                m_categoricalColumns.back().GetStringTable().
                    insert(std::make_pair(lastKey+1, wxString()));
                m_categoricalColumns.back().Resize(GetRowCount(), lastKey+1);
                }
            }
        return m_categoricalColumns.back();
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
                                     wxSysErrorMsg(fl.GetLastError())).ToUTF8());
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
    void Dataset::ExportText(const wxString& filePath,
                             const wchar_t delimiter, const bool quoteColumns) const
        {
        const auto wrapText = [&](const wxString& val)
            {
            if (quoteColumns)
                {
                // convert double quotes in val to two double quotes,
                // then wrap text with double quotes
                wxString escapedText = val;
                escapedText.Replace(L"\"", L"\"\"", true);
                return wxString(L"\"" + escapedText + L"\"");
                }
            else
                { return val; }
            };
        // combines column names as a delimited string
        const auto concatColNames = [&](const auto& cols)
            {
            wxString colNames;
            for (const auto& catCol : cols)
                {
                colNames.append(wrapText(catCol.GetName())).
                    append(1, delimiter);
                }
            colNames.RemoveLast();
            return colNames;
            };
        wxString idName = HasValidIdData() ? wrapText(GetIdColumn().GetName()) : wxString();
        wxString continuousColumnNames = concatColNames(GetContinuousColumns());
        wxString catColumnNames = concatColNames(GetCategoricalColumns());
        wxString dataColumnNames = concatColNames(GetDateColumns());
        wxString colNames = (idName.length() ? idName + delimiter : wxString()) +
            (continuousColumnNames.length() ? continuousColumnNames + delimiter : wxString()) +
            (catColumnNames.length() ? catColumnNames + delimiter : wxString()) +
            (dataColumnNames.length() ? dataColumnNames + delimiter : wxString());
        if (colNames.length() && colNames.Last() == delimiter)
            { colNames.RemoveLast();}
        
        wxString fileContent = colNames + L'\n';

        // write the data
        wxString currentRow;
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            currentRow.clear();
            // ID
            if (HasValidIdData())
                { currentRow.append(wrapText(GetIdColumn().GetValue(i))).append(1, delimiter); }
            // continuous
            for (const auto& col : GetContinuousColumns())
                {
                currentRow.append(wrapText(std::isnan(col.GetValue(i)) ? wxString() :
                                  wxNumberFormatter::ToString(col.GetValue(i), 6,
                                      wxNumberFormatter::Style::Style_NoTrailingZeroes))).
                           append(1, delimiter);
                }
            // categoricals
            for (const auto& col : GetCategoricalColumns())
                {
                currentRow.append(wrapText(col.GetCategoryLabelFromID(col.GetValue(i)))).
                           append(1, delimiter);
                }
            // dates
            for (const auto& col : GetDateColumns())
                {
                currentRow.append(
                           wrapText(col.GetValue(i).IsValid() ? col.GetValue(i).FormatISOCombined() : wxString())).
                           append(1, delimiter);
                }
            if (currentRow.length() && currentRow.Last() == delimiter)
                { currentRow.RemoveLast();}
            fileContent.append(currentRow).append(1, L'\n');
            }

        wxFile fl(filePath, wxFile::write);
        if (!fl.IsOpened() || !fl.Write(fileContent))
            {
            throw std::runtime_error(wxString::Format(_(L"'%s':\n%s"), filePath,
                                     wxSysErrorMsg(fl.GetLastError())).ToUTF8());
            }
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
                                     wxSysErrorMsg(fl.GetLastError())).ToUTF8());
            }
        fileText.Trim(true).Trim(false);

        m_name = wxFileName(filePath).GetName();

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
        const auto throwIfColumnNotFound = [&preview](const auto& columnName,
                                                      const auto& foundIterator,
                                                      const bool allowEmptyColumnName)
            {
            if (allowEmptyColumnName && columnName.empty())
                { return; }
            if (foundIterator == preview.get_header_names().cend())
                {
                const wxString errorMsg = wxString::Format(L"'%s': column not found!", columnName.c_str());
                throw std::runtime_error(errorMsg.ToUTF8());
                }
            };

        // column index with specialized import method
        struct catIndexInfo
            {
            size_t m_index{ 0 };
            CategoricalImportMethod m_importMethod{ CategoricalImportMethod::ReadAsStrings };
            GroupIdType m_mdCode{ 0 };
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
        throwIfColumnNotFound(info.m_idColumn, idColumnIter, true);
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
            throwIfColumnNotFound(dateColumn.m_columnName, dateColumnIter, false);
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
            throwIfColumnNotFound(catColumn.m_columnName, catColumnIter, false);
            catColumnIndices.push_back(
                (catColumnIter != preview.get_header_names().cend()) ?
                std::optional<catIndexInfo>(catIndexInfo{
                    static_cast<size_t>(catColumnIter - preview.get_header_names().cbegin()),
                                        catColumn.m_importMethod, catColumn.m_mdCode }) :
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
            throwIfColumnNotFound(continuousColumn, continuousColumnIter, false);
            continuousColumnIndices.push_back(
                (continuousColumnIter != preview.get_header_names().cend()) ?
                std::optional<size_t>(continuousColumnIter - preview.get_header_names().cbegin()) :
                std::nullopt);
            }

        // performs user-provided text replacement commands
        const auto replaceStrings = [&](const wxString& str)
            {
            wxString alteredStr{ str };
            for (const auto& [re, replacement] : info.m_textImportReplacements)
                {
                if (re && re->IsValid())
                    { re->ReplaceAll(&alteredStr, replacement); }
                }
            return alteredStr;
            };

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
                                replaceStrings(currentRow.at(catColumnIndices.at(i).value().m_index))) );
                        }
                    else
                        {
                        catCodes.emplace_back(
                            ConvertToGroupId(
                                currentRow.at(catColumnIndices.at(i).value().m_index),
                                catColumnIndices.at(i).value().m_mdCode));
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
            GetCategoricalColumn(i).GetStringTable().clear();
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
