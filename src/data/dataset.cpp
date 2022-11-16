///////////////////////////////////////////////////////////////////////////////
// Name:        dataset.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "dataset.h"

using namespace lily_of_the_valley;

namespace Wisteria::Data
    {
    //----------------------------------------------
    void ColumnWithStringTable::FillWithMissingData()
        {
        auto mdCode = FindMissingDataCode();
        if (!mdCode.has_value())
            {
            GetStringTable().insert(std::make_pair(GetNextKey(), wxEmptyString));
            mdCode = FindMissingDataCode();
            }
        wxASSERT_MSG(mdCode, L"Error creating MD code for caterical column!");
        if (mdCode.has_value())
            { Fill(mdCode.value()); }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': unable to fill column with missing data."), GetName()).ToUTF8());
            }
        }

    //----------------------------------------------
    bool ColumnWithStringTable::IsMissingData(const size_t index) const
        {
        wxASSERT_MSG(index < GetRowCount(), L"Invalid index in call to Column::IsMissingData()");
        if (index >= GetRowCount())
                { return false; }
        auto mdCode = FindMissingDataCode();
        if (!mdCode.has_value())
            { return false; }
        return GetValue(index) == mdCode.value();
        }

    //----------------------------------------------
    void Dataset::MutateCategoricalColumn(const wxString& srcColumnName,
                                          const wxString& targetColumnName,
                                          const RegExMap& replacementMap)
        {
        if (replacementMap.empty())
            {
            throw std::runtime_error(
                _(L"Replacement map empty for category mutation.").ToUTF8());
            }
       
        // get target iterator first as this may need to add a new column and invalid
        // other colum iterators
        auto targetVar = GetCategoricalColumnWritable(targetColumnName);
        if (targetVar == GetCategoricalColumns().end())
            {
            AddCategoricalColumn(targetColumnName);
            targetVar = GetCategoricalColumnWritable(targetColumnName);
            if (targetVar == GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': column not found for category mutation."), srcColumnName).ToUTF8());
                }
            }

        const auto srcVar = GetCategoricalColumn(srcColumnName);
        if (srcVar == GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found for category mutation."), srcColumnName).ToUTF8());
            }

        // prep target string table
        GroupIdType mdCode{ 0 };
        targetVar->GetStringTable().clear();
        targetVar->GetStringTable().insert(std::make_pair(mdCode, wxEmptyString));
        std::map<wxString, GroupIdType, StringCmpNoCase> idMap;
        for (const auto& re : replacementMap)
            {
            const auto nextId{ targetVar->GetNextKey() };
            targetVar->GetStringTable().insert(std::make_pair(nextId, re.second));
            const auto [insertPos, inserted] = idMap.insert(std::make_pair(re.second, nextId));
            if (!inserted)
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': duplicate string replacement encountered for category mutation."),
                    re.second).ToUTF8());
                }
            }

        // mutate the data
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            bool foundMatch{ false };
            for (const auto& re : replacementMap)
                {
                const auto currentSrcLabel = srcVar->GetValueAsLabel(i);
                if (re.first->Matches(currentSrcLabel))
                    {
                    const auto foundId = idMap.find(re.second);
                    if (foundId != idMap.cend())
                        {
                        targetVar->SetValue(i, foundId->second);
                        }
                    else
                        {
                        // shouldn't happen
                        wxFAIL_MSG(wxString::Format(
                            _(L"'%s': internal error finding mapped value for category mutation."),
                            re.second));
                        throw std::runtime_error(wxString::Format(
                            _(L"'%s': internal error finding mapped value for category mutation."),
                            re.second).ToUTF8());
                        }
                    foundMatch = true;
                    }
                }
            if (!foundMatch)
                { targetVar->SetValue(i, mdCode); }
            }
        }

    //----------------------------------------------
    void ColumnWithStringTable::CollapseStringTable()
        {
        multi_value_aggregate_map<wxString, GroupIdType> dupMap;
        for (const auto& [id, strValue] : GetStringTable())
            { dupMap.insert(strValue, id); }

        // cppcheck-suppress unusedVariable
        for (const auto& [str, idsAndFreq] : dupMap.get_data())
            {
            // found a string that appears more than once in the string table
            if (idsAndFreq.first.size() > 1)
                {
                auto idsIter = idsAndFreq.first.cbegin();
                const auto mainID = *idsIter;
                std::advance(idsIter, 1);
                // remove duplicates of string from string table and replace
                // their codes in the data with the code from the first
                // instance of that string (that remains in the string table)
                while (idsIter != idsAndFreq.first.cend())
                    {
                    const auto removeID = *idsIter;
                    m_stringTable.erase(m_stringTable.find(removeID));
                    Recode(removeID, mainID);
                    std::advance(idsIter, 1);
                    }
                }
            }
        }

    //----------------------------------------------
    void ColumnWithStringTable::CollapseExcept(const std::vector<wxString>& labelsToKeep,
                                               const wxString& otherLabel /*= _("Other")*/)
        {
        bool recodingNeeded{ false };
        
        for (auto& strEntry : GetStringTable())
            {
            if (const auto foundPos = std::find_if(labelsToKeep.cbegin(), labelsToKeep.cend(),
                [&strEntry](const auto& labelToKeep)
                { return strEntry.second.CmpNoCase(labelToKeep) == 0; });
                foundPos == labelsToKeep.cend())
                {
                strEntry.second = otherLabel;
                recodingNeeded = true;
                }
            }
        if (recodingNeeded)
            { CollapseStringTable(); }
        }

    //----------------------------------------------
    void Dataset::CollapseExcept(const wxString& colName,
                                 const std::vector<wxString>& labelsToKeep,
                                 const wxString& otherLabel /*= _("Other")*/)
        {
        auto catColumn = GetCategoricalColumnWritable(colName);
        if (catColumn == GetCategoricalColumns().end())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found for category collapsing."), colName).ToUTF8());
            }
        catColumn->CollapseExcept(labelsToKeep, otherLabel);
        }

    //----------------------------------------------
    void ColumnWithStringTable::CollapseMin(const size_t minVal,
                                            const wxString& otherLabel /*= _("Other")*/)
        {
        frequency_set<GroupIdType> vals;
        std::for_each(GetData().cbegin(), GetData().cend(),
            [&vals](const auto& datum)
            { vals.insert(datum); });
        bool recodingNeeded{ false };
        for (const auto& [id, count] : vals.get_data())
            {
            if (count < minVal)
                {
                auto foundPos = GetStringTable().find(id);
                wxASSERT_MSG(foundPos != GetStringTable().cend(),
                             L"Unable to find key in string table!");
                if (foundPos != GetStringTable().cend())
                    { foundPos->second = otherLabel; }
                recodingNeeded = true;
                }
            }
        if (recodingNeeded)
            { CollapseStringTable(); }
        }

    //----------------------------------------------
    void Dataset::CollapseMin(const wxString& colName, const size_t minVal,
                              const wxString& otherLabel /*= _("Other")*/)
        {
        auto catColumn = GetCategoricalColumnWritable(colName);
        if (catColumn == GetCategoricalColumns().end())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found for category collapsing."), colName).ToUTF8());
            }
        catColumn->CollapseMin(minVal, otherLabel);
        }

    //----------------------------------------------
    void ColumnWithStringTable::RecodeRE(const wxString& pattern,
                                         const wxString& replace)
        {
        wxRegEx re(pattern);
        if (re.IsValid())
            {
            for (auto& str : m_stringTable)
                { re.ReplaceAll(&str.second, replace); }
            }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': invalid regex used for recoding column."), pattern).ToUTF8());
            }
        CollapseStringTable();
        }

    //----------------------------------------------
    void Dataset::RecodeRE(const wxString& colName, const wxString& pattern,
                           const wxString& replace)
        {
        auto catColumn = GetCategoricalColumnWritable(colName);
        if (catColumn == GetCategoricalColumns().end())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found for regex recoding."), colName).ToUTF8());
            }
        catColumn->RecodeRE(pattern, replace);
        }

    //----------------------------------------------
    void Dataset::RenameColumn(const wxString& colName, const wxString& newColName)
        {
        if (newColName.empty())
            {
            throw std::runtime_error(
                _(L"New column name cannot be empty.").ToUTF8());
            }

        // make sure new name isn't in dataset already (unless old name and new name
        // are the same). New and old might be different if they are changing the casing.
        if (colName.CmpNoCase(newColName) != 0 &&
            ContainsColumn(newColName))
            {
            throw std::runtime_error(
                _(L"New column name already exists in the dataset.").ToUTF8());
            }

        auto continuousCol = GetContinuousColumnWritable(colName);
        auto catCol = GetCategoricalColumnWritable(colName);
        auto dateCol = GetDateColumnWritable(colName);

        if (GetIdColumn().GetName().CmpNoCase(colName) == 0)
            { GetIdColumn().SetName(newColName); }
        else if (continuousCol != GetContinuousColumns().end())
            { continuousCol->SetName(newColName); }
        else if (catCol != GetCategoricalColumns().end())
            { catCol->SetName(newColName); }
        else if (dateCol != GetDateColumns().end())
            { dateCol->SetName(newColName); }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': column not found for renaming."), newColName).ToUTF8());
            }
        }

    //----------------------------------------------
    bool Dataset::ContainsColumn(const wxString& colName) const noexcept
        {
        auto continuousCol = GetContinuousColumn(colName);
        auto catCol = GetCategoricalColumn(colName);
        auto dateCol = GetDateColumn(colName);

        if (GetIdColumn().GetName().CmpNoCase(colName) == 0)
            { return true; }
        else if (continuousCol != GetContinuousColumns().end())
            { return true; }
        else if (catCol != GetCategoricalColumns().end())
            { return true; }
        else if (dateCol != GetDateColumns().end())
            { return true; }
        return false;
        }

    //----------------------------------------------
    RegExMap ImportInfo::DatasetToRegExMap(const std::shared_ptr<Dataset>& dataset,
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

        RegExMap reMap;
        wxString currentRegex;
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            currentRegex = regexColumn->GetLabelFromID(regexColumn->GetValue(i));
            wxRegEx testRe(currentRegex);
            if (currentRegex.empty() || !testRe.IsValid())
                {
                wxLogWarning(_(L"'%s': regular expression syntax error."), currentRegex);
                continue;
                }
            reMap.push_back(std::make_pair(
                std::make_shared<wxRegEx>(currentRegex),
                replaceColumn->GetLabelFromID(replaceColumn->GetValue(i))));
            }

        return reMap;
        }

    //----------------------------------------------
    double Dataset::ConvertToDouble(const wxString& input, double MDRecodeValue)
        {
        if (input.empty())
            { return MDRecodeValue; }
        else
            {
            double val{ 0 };
            if (input.ToCDouble(&val))
                { return val; }
            else
                { return MDRecodeValue; }
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
                    catColumnIterator->GetLabelFromID(catColumnIterator->GetValue(i)));
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
    std::pair<double, double> Dataset::GetContinuousMinMax(const std::variant<wxString,
        size_t>& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        const auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                _(L"Column not found when calculating min/max.").ToUTF8());
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
    double Dataset::GetContinuousTotal(const std::variant<wxString, size_t>& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        const auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                _(L"Column not found when calculating total.").ToUTF8());
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

        // No rows or all NaN? Then return NaN.
        if (GetContinuousColumnValidN(column, groupColumn, groupId) == 0)
            { return std::numeric_limits<double>::quiet_NaN(); }

        double totalVal{ 0 };
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            if (!std::isnan(continuousColumnIterator->GetValue(i)) &&
                ((groupColumnIterator == GetCategoricalColumns().cend()) ||
                groupColumnIterator->GetValue(i) == groupId.value()))
                { totalVal += continuousColumnIterator->GetValue(i); }
            }
        return totalVal;
        }

    //----------------------------------------------
    size_t Dataset::GetContinuousColumnValidN(const std::variant<wxString, size_t>& column,
        const std::optional<wxString>& groupColumn,
        const std::optional<GroupIdType> groupId) const
        {
        const auto continuousColumnIterator = GetContinuousColumn(column);
        if (continuousColumnIterator == GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                _(L"Column not found when calculating valid N.").ToUTF8());
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
    ImportInfo Dataset::ImportInfoFromPreview(const ColumnPreviewInfo& previewInfo)
        {
        std::vector<wxString> continuousVars;
        std::vector<Data::ImportInfo::CategoricalImportInfo> catInfo;
        std::vector<Data::ImportInfo::DateImportInfo> dateInfo;
        for (const auto& colInfo : previewInfo)
            {
            if (colInfo.second == Data::Dataset::ColumnImportType::Integer)
                {
                catInfo.push_back(
                    { colInfo.first, CategoricalImportMethod::ReadAsIntegers });
                }
            else if (colInfo.second == Data::Dataset::ColumnImportType::String)
                {
                catInfo.push_back(
                    { colInfo.first, CategoricalImportMethod::ReadAsStrings });
                }
            else if (colInfo.second == Data::Dataset::ColumnImportType::Date)
                {
                dateInfo.push_back(
                    { colInfo.first, DateImportMethod::Automatic, L"" });
                }
            else if (colInfo.second == Data::Dataset::ColumnImportType::FloatingPoint)
                {
                continuousVars.push_back(colInfo.first);
                }
            }
        return ImportInfo().DateColumns(dateInfo).ContinuousColumns(continuousVars).
            CategoricalColumns(catInfo);
        }

    //----------------------------------------------
    Dataset::ColumnPreviewInfo Dataset::ReadColumnInfo(const wxString& filePath,
        const wchar_t delimiter,
        std::optional<size_t> rowPreviewCount /*= std::nullopt*/,
        size_t headerRowLine /*= 0*/)
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

        lily_of_the_valley::text_column<text_column_to_eol_parser>
            noReadColumn(lily_of_the_valley::text_column_to_eol_parser{ false });
        if (headerRowLine > 0)
            {
            // skip initial lines of text that the caller asked to skip
            lily_of_the_valley::text_row<wxString> noReadRowsStart{ headerRowLine };
            noReadRowsStart.add_column(noReadColumn);
            importer.add_row_definition(noReadRowsStart);
            }

        // skip the header
        lily_of_the_valley::text_row<wxString> noReadRow{ 1 };
        noReadRow.add_column(noReadColumn);
        importer.add_row_definition(noReadRow);

        lily_of_the_valley::standard_delimited_character_column
            deliminatedColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter });
        lily_of_the_valley::text_row<wxString> row{};
        row.add_column(deliminatedColumn);
        importer.add_row_definition(row);

        lily_of_the_valley::text_preview preview;
        std::vector<std::pair<wxString, ColumnImportType>> columnInfo;
        // read either first few rows or entire file, whichever is less
        size_t rowCount = std::min<size_t>(preview(fileText.wc_str(), delimiter, false, false, headerRowLine),
                                           rowPreviewCount.has_value() ?
                                               (rowPreviewCount.value() + 1/*header*/) : 100);

        // see if there are any duplicate column names
        std::set<wxString, StringCmpNoCase> colNames;
        for (const auto& headerName : preview.get_header_names())
            {
            const auto [iter, inserted] = colNames.insert(headerName);
            if (!inserted)
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': column name appears more than once in dataset."),
                    *iter).ToUTF8());
                }
            }

        if (rowCount > 0)
            {
            dataStrings.resize(rowCount);
            rowCount = importer.read(fileText.wc_str(), rowCount, preview.get_header_names().size(), false);
            }
        else
            { return columnInfo; }

        wxLogNull nl;
        wxRegEx fpRegex(L"^[0-9]+[.,][0-9]+$");
        for (size_t colIndex = 0; colIndex < preview.get_header_names().size(); ++colIndex)
            {
            // assume column's data is integral unless something in the first
            // few rows looks like a string
            ColumnImportType currentColumnType{ ColumnImportType::Integer };
            for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
                {
                const auto& currentCell = dataStrings.at(rowIndex).at(colIndex);
                // can't deduce anything from MD
                if (currentCell.empty())
                    { continue; }
                /* "23.06" can be converted to a date, so for that pattern mark it as a
                    float before the date conversion check.*/
                if (fpRegex.Matches(currentCell))
                    {
                    // switch from integer to fp, but keep going in case there
                    // is a string or date further down this column
                    currentColumnType = ColumnImportType::FloatingPoint;
                    }
                else if (ConvertToDate(currentCell, DateImportMethod::Automatic, L"").IsValid())
                    {
                    currentColumnType = ColumnImportType::Date;
                    break;
                    }
                const auto parsedNumber =
                    ConvertToDouble(currentCell, std::numeric_limits<double>::quiet_NaN());
                if (std::isnan(parsedNumber))
                    {
                    currentColumnType = ColumnImportType::String;
                    break;
                    }
                else if (!compare_doubles(get_mantissa(parsedNumber), 0))
                    { currentColumnType = ColumnImportType::FloatingPoint; }
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
        wxString catColumnNames = concatColNames(GetCategoricalColumns());
        wxString dateColumnNames = concatColNames(GetDateColumns());
        wxString continuousColumnNames = concatColNames(GetContinuousColumns());

        wxString colNames = (idName.length() ? idName + delimiter : wxString()) +
            (catColumnNames.length() ? catColumnNames + delimiter : wxString()) +
            (dateColumnNames.length() ? dateColumnNames + delimiter : wxString()) +
            (continuousColumnNames.length() ? continuousColumnNames + delimiter : wxString());
        if (colNames.length() && colNames[colNames.length()-1] == delimiter)
            { colNames.RemoveLast(); }
        
        wxString fileContent = colNames + L'\n';

        // write the data
        wxString currentRow;
        for (size_t i = 0; i < GetRowCount(); ++i)
            {
            currentRow.clear();
            // ID
            if (HasValidIdData())
                { currentRow.append(wrapText(GetIdColumn().GetValue(i))).append(1, delimiter); }
            // categoricals
            for (const auto& col : GetCategoricalColumns())
                {
                currentRow.append(wrapText(col.GetLabelFromID(col.GetValue(i)))).
                           append(1, delimiter);
                }
            // dates
            for (const auto& col : GetDateColumns())
                {
                currentRow.append(
                           wrapText(col.GetValue(i).IsValid() ?
                               col.GetValue(i).FormatISOCombined() :
                               wxString())).
                           append(1, delimiter);
                }
            // continuous
            for (const auto& col : GetContinuousColumns())
                {
                currentRow.append(wrapText(std::isnan(col.GetValue(i)) ? wxString() :
                                  wxNumberFormatter::ToString(col.GetValue(i), 6,
                                      wxNumberFormatter::Style::Style_NoTrailingZeroes))).
                           append(1, delimiter);
                }
            if (currentRow.length() && currentRow[currentRow.length()-1] == delimiter)
                { currentRow.RemoveLast(); }
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
    void Dataset::ImportTextRaw(const wxString& fileText, const ImportInfo& info,
                                const wchar_t delimiter)
        {
        // reset
        Clear();
        m_dateColumns.clear();
        m_categoricalColumns.clear();
        m_continuousColumns.clear();
        m_name.clear();

        SetImportContinuousMDRecodeValue(info.m_continousMDRecodeValue);

        std::vector<std::vector<wxString>> dataStrings;

        lily_of_the_valley::text_matrix<wxString> importer{ &dataStrings };

        lily_of_the_valley::text_column<text_column_to_eol_parser>
            noReadColumn(lily_of_the_valley::text_column_to_eol_parser{ false });
        if (info.GetRowsToSkip() > 0)
            {
            // skip initial lines of text that the caller asked to skip
            lily_of_the_valley::text_row<wxString> noReadRowsStart{ info.GetRowsToSkip() };
            noReadRowsStart.add_column(noReadColumn);
            importer.add_row_definition(noReadRowsStart);
            }

        // skip the header
        lily_of_the_valley::text_row<wxString> noReadRow{ 1 };
        noReadRow.add_column(noReadColumn);
        importer.add_row_definition(noReadRow);

        lily_of_the_valley::standard_delimited_character_column
            deliminatedColumn(lily_of_the_valley::text_column_delimited_character_parser{ delimiter });
        lily_of_the_valley::text_row<wxString> row{};
        row.add_column(deliminatedColumn);
        importer.add_row_definition(row);

        lily_of_the_valley::text_preview preview;
        // see how many lines are in the file and resize the container
        const size_t rowCount = preview(fileText.wc_str(), delimiter, false, false, info.GetRowsToSkip());
        if (rowCount > 0)
            {
            dataStrings.resize(rowCount);
            importer.read(fileText.wc_str(), rowCount, preview.get_header_names().size(), false);
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
            // cppcheck-suppress unusedStructMember
            size_t m_index{ 0 };
            CategoricalImportMethod m_importMethod{ CategoricalImportMethod::ReadAsStrings };
            // cppcheck-suppress unusedStructMember
            GroupIdType m_mdCode{ 0 };
            };

        // column index with specialized import method
        struct dateIndexInfo
            {
            // cppcheck-suppress unusedStructMember
            size_t m_index{ 0 };
            DateImportMethod m_importMethod{ DateImportMethod::Automatic };
            wxString m_formatStr;
            };

        // find the column indices into the data that match the column names
        // from the client and map them as they requested
        const auto idColumnIter = std::find_if(preview.get_header_names().cbegin(),
            preview.get_header_names().cend(),
            [&info](const auto& item)
                { return info.m_idColumn.CmpNoCase(item.c_str()) == 0; });
        throwIfColumnNotFound(info.m_idColumn, idColumnIter, true);
        const std::optional<size_t> idColumnIndex =
            (idColumnIter != preview.get_header_names().cend()) ?
            std::optional<size_t>(idColumnIter - preview.get_header_names().cbegin()) :
            std::nullopt;

        // find the supplied date columns
        std::vector<std::optional<dateIndexInfo>> dateColumnIndices;
        for (const auto& dateColumn : info.m_dateColumns)
            {
            const auto dateColumnIter = std::find_if(preview.get_header_names().cbegin(),
                preview.get_header_names().cend(),
                [&dateColumn](const auto& item)
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
                [&catColumn](const auto& item)
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
                [&continuousColumn](const auto& item)
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
                        ConvertToDouble(
                            currentRow.at(continuousColumnIndices.at(i).value()),
                            m_importContinousMDRecodeValue));
                    }
                }
            currentItem.Continuous(continuousValues);

            // ID column
            if (idColumnIndex.has_value())
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

    //----------------------------------------------
    void Dataset::ImportText(const wxFileName& filePath, const ImportInfo& info,
                             const wchar_t delimiter)
        {
        wxString fileText;
        wxFile fl(filePath.GetFullPath());
        if (!fl.IsOpened() || !fl.ReadAll(&fileText))
            {
            throw std::runtime_error(wxString::Format(_(L"'%s':\n%s"), filePath.GetFullPath(),
                                     wxSysErrorMsg(fl.GetLastError())).ToUTF8());
            }
        fileText.Trim(true).Trim(false);

        ImportTextRaw(fileText, info, delimiter);

        m_name = filePath.GetName();
        }

    //----------------------------------------------
    std::optional<ColumnIterator> Dataset::FindColumn(const wxString& colName)
        {
        if (GetIdColumn().GetName().CmpNoCase(colName) == 0)
            { return &GetIdColumn(); }
        else if (auto foundVar = GetCategoricalColumnWritable(colName);
            foundVar != GetCategoricalColumns().end())
            { return foundVar; }
        else if (auto foundVar = GetContinuousColumnWritable(colName);
            foundVar != GetContinuousColumns().end())
            { return foundVar; }
        else if (auto foundVar = GetDateColumnWritable(colName);
            foundVar != GetDateColumns().end())
            { return foundVar; }
        else
            { return std::nullopt; }
        }
    }
