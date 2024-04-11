///////////////////////////////////////////////////////////////////////////////
// Name:        subset.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "subset.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    ColumnFilter::ColumnFilter(const std::shared_ptr<const Dataset>& fromDataset,
                               const ColumnFilterInfo& subsetCriterion)
        {
        assert(fromDataset && L"Invalid dataset passed to column filter.");
        // reset everything
        m_idColumn = nullptr;
        m_continuousColumn = fromDataset->GetContinuousColumns().cend();
        m_categoricalColumn = fromDataset->GetCategoricalColumns().cend();
        m_dateColumn = fromDataset->GetDateColumns().cend();

        m_groupIdValues.clear();
        m_stringValues.clear();
        m_dateTimeValues.clear();
        m_doubleValues.clear();

        // set the comparison method
        m_comparisonType = subsetCriterion.m_comparisonType;

        // find the variable that we are filtering with
        if (fromDataset->GetIdColumn().GetName().CmpNoCase(subsetCriterion.m_columnName) == 0)
            {
            m_idColumn = &fromDataset->GetIdColumn();
            m_columnType = ColumnType::ID;
            }
        else
            {
            m_continuousColumn = fromDataset->GetContinuousColumn(subsetCriterion.m_columnName);
            m_columnType = ColumnType::Continuous;
            if (m_continuousColumn == fromDataset->GetContinuousColumns().cend())
                {
                // if not found, look for it as a categorical
                m_categoricalColumn =
                    fromDataset->GetCategoricalColumn(subsetCriterion.m_columnName);
                m_columnType = ColumnType::Categorical;
                if (m_categoricalColumn == fromDataset->GetCategoricalColumns().cend())
                    {
                    // try date columns
                    m_dateColumn = fromDataset->GetDateColumn(subsetCriterion.m_columnName);
                    m_columnType = ColumnType::Date;
                    if (m_dateColumn == fromDataset->GetDateColumns().cend())
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"'%s': column not found for filtering."),
                                             subsetCriterion.m_columnName)
                                .ToUTF8());
                        }
                    }
                }
            }

        // if categorical, then set the comparison value
        if (m_columnType == ColumnType::Categorical)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                // use group IDs if using == or !=, as this is efficient comparison
                if (m_comparisonType == Comparison::Equals ||
                    m_comparisonType == Comparison::NotEquals)
                    {
                    // using a string table and a number was passed in,
                    // so treat it as a group ID
                    if (const auto IdVal{ std::get_if<GroupIdType>(&value) };
                        m_categoricalColumn->HasValidStringTableEntries() && IdVal != nullptr)
                        {
                        m_groupIdValues.push_back(*IdVal);
                        if (m_categoricalColumn->GetStringTable().find(*IdVal) ==
                            m_categoricalColumn->GetStringTable().cend())
                            {
                            throw std::runtime_error(
                                wxString::Format(_(L"Group ID not found for '%s' column filter."),
                                                 m_categoricalColumn->GetName()));
                            }
                        }
                    // no real string table, so column may be filled with discrete values instead,
                    // treat numbers as such
                    else if (const auto doubleVal{ std::get_if<double>(&value) };
                             !m_categoricalColumn->HasValidStringTableEntries() &&
                             doubleVal != nullptr)
                        {
                        m_doubleValues.push_back(*doubleVal);
                        }
                    // using string table and a string was passed in
                    else if (const auto strVal{ std::get_if<wxString>(&value) };
                             m_categoricalColumn->HasValidStringTableEntries() && strVal != nullptr)
                        {
                        const auto code = m_categoricalColumn->GetIDFromLabel(*strVal);
                        if (code.has_value())
                            {
                            m_groupIdValues.push_back(code.value());
                            }
                        else
                            {
                            throw std::runtime_error(
                                wxString::Format(_(L"'%s': string value not found for "
                                                   "'%s' column filter."),
                                                 *strVal, m_categoricalColumn->GetName()));
                            }
                        }
                    else if (const auto strVal2{ std::get_if<wxString>(&value) };
                             !m_categoricalColumn->HasValidStringTableEntries() &&
                             strVal2 != nullptr)
                        {
                        double numVal{ 0.0 };
                        if (strVal2->ToDouble(&numVal))
                            {
                            m_doubleValues.push_back(numVal);
                            }
                        else
                            {
                            throw std::runtime_error(wxString::Format(
                                _(L"'%s': string value not found for "
                                  "'%s' column filter. "
                                  "Column does not have a string table, and string could not "
                                  "be converted to a discrete value."),
                                *strVal2, m_categoricalColumn->GetName()));
                            }
                        }
                    else
                        {
                        throw std::runtime_error(_(L"Categorical column filter requires either "
                                                   "a group ID or string value for filtering."));
                        }
                    }
                // if using other operators, then we need to use string comparisons later
                else
                    {
                    if (const auto IdVal{ std::get_if<GroupIdType>(&value) }; IdVal != nullptr)
                        {
                        m_stringValues.push_back(m_categoricalColumn->GetLabelFromID(*IdVal));
                        }
                    else if (const auto strVal{ std::get_if<wxString>(&value) }; strVal != nullptr)
                        {
                        m_stringValues.push_back(*strVal);
                        }
                    else
                        {
                        throw std::runtime_error(_(L"Categorical column filter requires either "
                                                   "a group ID or string value for filtering."));
                        }
                    }
                }
            }
        // continuous column
        else if (m_columnType == ColumnType::Continuous)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                if (const auto doubleVal{ std::get_if<double>(&value) }; doubleVal != nullptr)
                    {
                    m_doubleValues.push_back(*doubleVal);
                    }
                else if (const auto strVal{ std::get_if<wxString>(&value) }; strVal != nullptr)
                    {
                    double convertedVal{ 0.0 };
                    if (strVal->ToDouble(&convertedVal))
                        {
                        m_doubleValues.push_back(convertedVal);
                        }
                    else
                        {
                        throw std::runtime_error(_(L"Continuous column filter requires "
                                                   "a double value for filtering."));
                        }
                    }
                else
                    {
                    throw std::runtime_error(_(L"Continuous column filter requires "
                                               "a double value for filtering."));
                    }
                }
            }
        // datetime column
        else if (m_columnType == ColumnType::Date)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                if (const auto dateVal{ std::get_if<wxDateTime>(&value) }; dateVal != nullptr)
                    {
                    m_dateTimeValues.push_back(*dateVal);
                    }
                else if (const auto strVal{ std::get_if<wxString>(&value) }; strVal != nullptr)
                    {
                    wxDateTime dt;
                    if (dt.ParseDateTime(*strVal) || dt.ParseDate(*strVal))
                        {
                        m_dateTimeValues.push_back(dt);
                        }
                    else
                        {
                        throw std::runtime_error(wxString::Format(
                            _(L"%s: string unable to be parsed for date filter."), *strVal));
                        }
                    }
                else
                    {
                    throw std::runtime_error(_(L"Date column filter requires "
                                               "a datetime or string value for filtering."));
                    }
                }
            }
        // ID column, comparing as strings
        else if (m_columnType == ColumnType::ID)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                if (const auto val{ std::get_if<wxString>(&value) }; val != nullptr)
                    {
                    m_stringValues.push_back(*val);
                    }
                else
                    {
                    throw std::runtime_error(_(L"ID column filter requires "
                                               "a string value for filtering."));
                    }
                }
            }
        }

    //---------------------------------------------------
    bool ColumnFilter::MeetsCriterion(const size_t rowPosition) const
        {
        if (m_columnType == ColumnType::Categorical)
            {
            // categorical is using discrete values instead of a string table
            if (m_doubleValues.size())
                {
                const auto& dVal = m_categoricalColumn->GetValue(rowPosition);
                for (const auto& val : m_doubleValues)
                    {
                    if (m_comparisonType == Comparison::Equals    ? compare_doubles(dVal, val) :
                        m_comparisonType == Comparison::NotEquals ? !compare_doubles(dVal, val) :
                        m_comparisonType == Comparison::LessThan ? compare_doubles_less(dVal, val) :
                        m_comparisonType == Comparison::LessThanOrEqualTo ?
                                                                   compare_doubles_less_or_equal(
                                                                       dVal, val) :
                        m_comparisonType == Comparison::GreaterThan ? compare_doubles_greater(dVal,
                                                                                              val) :
                                                                      // GreaterThanOrEqualTo
                            compare_doubles_greater_or_equal(dVal, val))
                        {
                        return true;
                        }
                    }
                }
            else
                {
                if (m_comparisonType == Comparison::Equals ||
                    m_comparisonType == Comparison::NotEquals)
                    {
                    for (const auto& idVal : m_groupIdValues)
                        {
                        // more optimal to compare integral types, so do that if
                        // reviewing == or !=
                        if (m_comparisonType == Comparison::Equals ?
                                m_categoricalColumn->GetValue(rowPosition) == idVal :
                                m_categoricalColumn->GetValue(rowPosition) != idVal)
                            {
                            return true;
                            }
                        }
                    }
                else
                    {
                    // < or > will require comparing as the strings though since
                    // the underlying (integral) group IDs probably aren't ordered the same way
                    // as the strings would be alphabetically
                    for (const auto& str : m_stringValues)
                        {
                        const auto currentString = m_categoricalColumn->GetLabelFromID(
                            m_categoricalColumn->GetValue(rowPosition));
                        const auto cmpResult = currentString.CmpNoCase(str);
                        if (m_comparisonType == Comparison::LessThan          ? cmpResult < 0 :
                            m_comparisonType == Comparison::LessThanOrEqualTo ? cmpResult <= 0 :
                            m_comparisonType == Comparison::GreaterThan       ? cmpResult > 0 :
                                                                          // GreaterThanOrEqualTo
                                                                          cmpResult >= 0)
                            {
                            return true;
                            }
                        }
                    }
                }
            }
        else if (m_columnType == ColumnType::Continuous)
            {
            const auto& dVal = m_continuousColumn->GetValue(rowPosition);
            for (const auto& val : m_doubleValues)
                {
                if (m_comparisonType == Comparison::Equals    ? compare_doubles(dVal, val) :
                    m_comparisonType == Comparison::NotEquals ? !compare_doubles(dVal, val) :
                    m_comparisonType == Comparison::LessThan  ? compare_doubles_less(dVal, val) :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                                                               compare_doubles_less_or_equal(dVal,
                                                                                             val) :
                    m_comparisonType == Comparison::GreaterThan ?
                                                               compare_doubles_greater(dVal, val) :
                                                               // GreaterThanOrEqualTo
                        compare_doubles_greater_or_equal(dVal, val))
                    {
                    return true;
                    }
                }
            }
        else if (m_columnType == ColumnType::Date)
            {
            const auto& dtVal = m_dateColumn->GetValue(rowPosition);
            for (const auto& dt : m_dateTimeValues)
                {
                if (m_comparisonType == Comparison::Equals            ? dtVal == dt :
                    m_comparisonType == Comparison::NotEquals         ? dtVal != dt :
                    m_comparisonType == Comparison::LessThan          ? dtVal < dt :
                    m_comparisonType == Comparison::LessThanOrEqualTo ? dtVal <= dt :
                    m_comparisonType == Comparison::GreaterThan       ? dtVal > dt :
                                                                        // GreaterThanOrEqualTo
                                                                        dtVal >= dt)
                    {
                    return true;
                    }
                }
            }
        else if (m_columnType == ColumnType::ID)
            {
            for (const auto& str : m_stringValues)
                {
                const auto cmpResult = m_idColumn->GetValue(rowPosition).CmpNoCase(str);
                if (m_comparisonType == Comparison::Equals            ? cmpResult == 0 :
                    m_comparisonType == Comparison::NotEquals         ? cmpResult != 0 :
                    m_comparisonType == Comparison::LessThan          ? cmpResult < 0 :
                    m_comparisonType == Comparison::LessThanOrEqualTo ? cmpResult <= 0 :
                    m_comparisonType == Comparison::GreaterThan       ? cmpResult > 0 :
                                                                        // GreaterThanOrEqualTo
                                                                        cmpResult >= 0)
                    {
                    return true;
                    }
                }
            }
        return false;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetSimple(const std::shared_ptr<const Dataset>& fromDataset,
                                                  const ColumnFilterInfo& columnFilter)
        {
        if (fromDataset == nullptr)
            {
            return nullptr;
            }

        SetSourceData(fromDataset);

        ColumnFilter cf(GetSource(), columnFilter);

        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                if (cf.MeetsCriterion(nextRow.value()))
                    {
                    CopyNextRow();
                    }
                else
                    {
                    SkipNextRow();
                    }
                }
            // shouldn't happen
            else
                {
                break;
                }
            }

        return GetClone();
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetOr(const std::shared_ptr<const Dataset>& fromDataset,
                                              const std::vector<ColumnFilterInfo>& columnFilters)
        {
        if (fromDataset == nullptr)
            {
            return nullptr;
            }

        SetSourceData(fromDataset);

        std::vector<ColumnFilter> cFilters;
        for (const auto& cf : columnFilters)
            {
            cFilters.push_back({ GetSource(), cf });
            }

        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                bool hadMatch{ false };
                for (const auto& cf : cFilters)
                    {
                    if (cf.MeetsCriterion(nextRow.value()))
                        {
                        hadMatch = true;
                        break;
                        }
                    }
                // if any criterion matches, then copy observation and go to next
                if (hadMatch)
                    {
                    CopyNextRow();
                    }
                else
                    {
                    SkipNextRow();
                    }
                }
            // shouldn't happen
            else
                {
                break;
                }
            }

        return GetClone();
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetAnd(const std::shared_ptr<const Dataset>& fromDataset,
                                               const std::vector<ColumnFilterInfo>& columnFilters)
        {
        if (fromDataset == nullptr)
            {
            return nullptr;
            }

        SetSourceData(fromDataset);

        std::vector<ColumnFilter> cFilters;
        for (const auto& cf : columnFilters)
            {
            cFilters.push_back({ GetSource(), cf });
            }

        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                bool allMatched{ true };
                for (const auto& cf : cFilters)
                    {
                    // if any criterion doesn't match, then bail
                    if (!cf.MeetsCriterion(nextRow.value()))
                        {
                        allMatched = false;
                        break;
                        }
                    }
                // if all criteria matched, then copy
                if (allMatched)
                    {
                    CopyNextRow();
                    }
                else
                    {
                    SkipNextRow();
                    }
                }
            // shouldn't happen
            else
                {
                break;
                }
            }

        return GetClone();
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset>
    Subset::SubsetSection(const std::shared_ptr<const Dataset>& fromDataset, const wxString& column,
                          const wxString& startRowLabel, const wxString& endRowLabel,
                          const bool includeSentinelLabels)
        {
        if (fromDataset == nullptr)
            {
            return nullptr;
            }

        SetSourceData(fromDataset);

        ColumnFilter startFilter(GetSource(),
                                 ColumnFilterInfo{ column, Comparison::Equals, { startRowLabel } });
        ColumnFilter endFilter(GetSource(),
                               ColumnFilterInfo{ column, Comparison::Equals, { endRowLabel } });

        // get to the starting point
        bool foundStartRow{ false };
        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                if (startFilter.MeetsCriterion(nextRow.value()))
                    {
                    if (includeSentinelLabels)
                        {
                        CopyNextRow();
                        }
                    else
                        {
                        SkipNextRow();
                        }
                    foundStartRow = true;
                    break;
                    }
                else
                    {
                    SkipNextRow();
                    }
                }
            // shouldn't happen
            else
                {
                break;
                }
            }

        if (!foundStartRow)
            {
            return nullptr;
            }

        // read until we find the requested end row or we read the end of the dataset
        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                if (!endFilter.MeetsCriterion(nextRow.value()))
                    {
                    CopyNextRow();
                    }
                // copy the end sentinel row, but then stop
                else
                    {
                    if (includeSentinelLabels)
                        {
                        CopyNextRow();
                        }
                    break;
                    }
                }
            // shouldn't happen
            else
                {
                break;
                }
            }

        return GetClone();
        }
    } // namespace Wisteria::Data
