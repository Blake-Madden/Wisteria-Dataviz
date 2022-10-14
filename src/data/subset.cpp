#include "subset.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    ColumnFilter::ColumnFilter(const std::shared_ptr<const Dataset>& fromDataset,
        const ColumnFilterInfo subsetCriterion)
        {
        wxASSERT_MSG(fromDataset, L"Invalid dataset passed to column filter.");
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
                        throw std::runtime_error(wxString::Format(
                            _(L"'%s': column not found for filtering."),
                            subsetCriterion.m_columnName).ToUTF8());
                        }
                    }
                }
            }

        // if categorical, then set the comparison value
        if (m_columnType == ColumnType::Categorical)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                if (const auto val{ std::get_if<GroupIdType>(&value) };
                    val != nullptr)
                    {
                    m_groupIdValues.push_back(*val);
                    if (m_categoricalColumn->GetStringTable().find(*val) ==
                        m_categoricalColumn->GetStringTable().cend())
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"Group ID not found for '%s' column filter."),
                                             m_categoricalColumn->GetName()) );
                        }
                    }
                else if (const auto val{ std::get_if<wxString>(&value) };
                    val != nullptr)
                    {
                    const auto code = m_categoricalColumn->GetIDFromLabel(*val);
                    if (code.has_value())
                        { m_groupIdValues.push_back(code.value()); }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"'%s': string value not found for "
                                                "'%s' column filter."), *val,
                                             m_categoricalColumn->GetName()));
                        }
                    }
                else
                    {
                    throw std::runtime_error(_(L"Categorical column filter requires either "
                        "a groud ID or string value for filtering."));
                    }

                // group ID is set, but that will only work for == or !=.
                // if using other operators, then we need to use string comparisons later.
                if (m_comparisonType != Comparison::Equals &&
                    m_comparisonType != Comparison::NotEquals)
                    {
                    wxASSERT_MSG(m_groupIdValues.size(), L"No items in group IDs when building subset?!");
                    m_stringValues.push_back(m_categoricalColumn->GetLabelFromID(m_groupIdValues.back()));
                    }
                }
            }
        // continuous column
        else if (m_columnType == ColumnType::Continuous)
            {
            for (const auto& value : subsetCriterion.m_values)
                {
                if (const auto val{ std::get_if<double>(&value) };
                    val != nullptr)
                    { m_doubleValues.push_back(*val); }
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
                if (const auto val{ std::get_if<wxDateTime>(&value) };
                    val != nullptr)
                    { m_dateTimeValues.push_back(*val); }
                else if (const auto val{ std::get_if<wxString>(&value) };
                    val != nullptr)
                    {
                    wxDateTime dt;
                    if (dt.ParseDateTime(*val) || dt.ParseDate(*val))
                        { m_dateTimeValues.push_back(dt); }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: string unable to be parsed for date filter."),
                                             *val));
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
                if (const auto val{ std::get_if<wxString>(&value) };
                    val != nullptr)
                    { m_stringValues.push_back(*val); }
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
                        { return true; }
                    }
                }
            else
                {
                // < or > will require comparing as the strings though since
                // group IDs probably aren't ordered the same ways as the strings
                // would be alphabetically
                for (const auto& str : m_stringValues)
                    {
                    const auto currentString =
                        m_categoricalColumn->GetLabelFromID(
                            m_categoricalColumn->GetValue(rowPosition));
                    const auto cmpResult = currentString.CmpNoCase(str);
                    if (m_comparisonType == Comparison::LessThan ?
                            cmpResult < 0 :
                        m_comparisonType == Comparison::LessThanOrEqualTo ?
                            cmpResult <= 0 :
                        m_comparisonType == Comparison::GreaterThan ?
                            cmpResult > 0 :
                        // GreaterThanOrEqualTo
                            cmpResult >= 0)
                        { return true; }
                    }
                }
            }
        else if (m_columnType == ColumnType::Continuous)
            {
            for (const auto& val : m_doubleValues)
                {
                const auto& dVal = m_continuousColumn->GetValue(rowPosition);
                if (m_comparisonType == Comparison::Equals ?
                    compare_doubles(dVal, val) :
                    m_comparisonType == Comparison::NotEquals ?
                    !compare_doubles(dVal, val) :
                    m_comparisonType == Comparison::LessThan ?
                    compare_doubles_less(dVal, val) :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                    compare_doubles_less_or_equal(dVal, val) :
                    m_comparisonType == Comparison::GreaterThan ?
                    compare_doubles_greater(dVal, val) :
                    // GreaterThanOrEqualTo
                    compare_doubles_greater_or_equal(dVal, val))
                    { return true; }
                }
            }
        else if (m_columnType == ColumnType::Date)
            {
            for (const auto& dt : m_dateTimeValues)
                {
                const auto& dtVal = m_dateColumn->GetValue(rowPosition);
                if (m_comparisonType == Comparison::Equals ?
                    dtVal == dt :
                    m_comparisonType == Comparison::NotEquals ?
                    dtVal != dt :
                    m_comparisonType == Comparison::LessThan ?
                    dtVal < dt :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                    dtVal <= dt :
                    m_comparisonType == Comparison::GreaterThan ?
                    dtVal > dt :
                    // GreaterThanOrEqualTo
                    dtVal >= dt)
                    { return true; }
                }
            }
        else if (m_columnType == ColumnType::ID)
            {
            for (const auto& str : m_stringValues)
                {
                const auto cmpResult =
                    m_idColumn->GetValue(rowPosition).CmpNoCase(str);
                if (m_comparisonType == Comparison::Equals ?
                    cmpResult == 0 :
                    m_comparisonType == Comparison::NotEquals ?
                    cmpResult != 0 :
                    m_comparisonType == Comparison::LessThan ?
                    cmpResult < 0 :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                    cmpResult <= 0 :
                    m_comparisonType == Comparison::GreaterThan ?
                    cmpResult > 0 :
                    // GreaterThanOrEqualTo
                    cmpResult >= 0)
                    { return true; }
                }
            }
        return false;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetSimple(
            const std::shared_ptr<const Dataset>& fromDataset,
            const ColumnFilterInfo columnFilter)
        {
        if (fromDataset == nullptr)
            { return nullptr; }

        SetSourceData(fromDataset);

        ColumnFilter cf(GetSource(), columnFilter);

        while (HasMoreRows())
            {
            const auto nextRow = GetNextRowPosition();
            if (nextRow.has_value())
                {
                if (cf.MeetsCriterion(nextRow.value()))
                    { CopyNextRow(); }
                else
                    { SkipNextRow(); }
                }
            // shouldn't happen
            else
                { break; }
            }

        return GetClone();
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetOr(
            const std::shared_ptr<const Dataset>& fromDataset,
            const std::vector<ColumnFilterInfo>& columnFilters)
        {
        if (fromDataset == nullptr)
            { return nullptr; }

        SetSourceData(fromDataset);

        std::vector<ColumnFilter> cFilters;
        for (const auto& cf : columnFilters)
            { cFilters.push_back({ GetSource(), cf }); }

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
                    { CopyNextRow(); }
                else
                    { SkipNextRow(); }
                }
            // shouldn't happen
            else
                { break; }
            }

        return GetClone();
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Subset::SubsetAnd(
            const std::shared_ptr<const Dataset>& fromDataset,
            const std::vector<ColumnFilterInfo>& columnFilters)
        {
        if (fromDataset == nullptr)
            { return nullptr; }

        SetSourceData(fromDataset);

        std::vector<ColumnFilter> cFilters;
        for (const auto& cf : columnFilters)
            { cFilters.push_back({ GetSource(), cf }); }

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
                    { CopyNextRow(); }
                else
                    { SkipNextRow(); }
                }
            // shouldn't happen
            else
                { break; }
            }

        return GetClone();
        }
    }
