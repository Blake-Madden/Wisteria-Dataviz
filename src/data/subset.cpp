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

        m_groupIdValue = 0;
        m_stringValue.clear();
        m_dateTimeValue = wxInvalidDateTime;
        m_doubleValue = std::numeric_limits<double>::quiet_NaN();

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
            if (const auto val{ std::get_if<GroupIdType>(&subsetCriterion.m_value) };
                val != nullptr)
                {
                m_groupIdValue = *val;
                if (m_categoricalColumn->GetStringTable().find(m_groupIdValue) ==
                    m_categoricalColumn->GetStringTable().cend())
                    {
                    throw std::runtime_error(
                        _(L"Group ID not found for categorical column filter."));
                    }
                }
            else if (const auto val{ std::get_if<wxString>(&subsetCriterion.m_value) };
                val != nullptr)
                {
                const auto code = m_categoricalColumn->GetIDFromLabel(*val);
                if (code.has_value())
                    { m_groupIdValue = code.value(); }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': string value not found for "
                                            "categorical column filter."), *val));
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
                m_stringValue = m_categoricalColumn->GetLabelFromID(m_groupIdValue);
                }
            }
        // continuous column
        else if (m_columnType == ColumnType::Continuous)
            {
            if (const auto val{ std::get_if<double>(&subsetCriterion.m_value) };
                val != nullptr)
                { m_doubleValue = *val; }
            else
                {
                throw std::runtime_error(_(L"Continuous column filter requires "
                    "a double value for filtering."));
                }
            }
        // datetime column
        else if (m_columnType == ColumnType::Date)
            {
            if (const auto val{ std::get_if<wxDateTime>(&subsetCriterion.m_value) };
                val != nullptr)
                { m_dateTimeValue = *val; }
            else if (const auto val{ std::get_if<wxString>(&subsetCriterion.m_value) };
                val != nullptr)
                {
                wxDateTime dt;
                if (dt.ParseDateTime(*val) || dt.ParseDate(*val))
                    { m_dateTimeValue = dt; }
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
        // ID column, comparing as strings
        else if (m_columnType == ColumnType::ID)
            {
            if (const auto val{ std::get_if<wxString>(&subsetCriterion.m_value) };
                val != nullptr)
                { m_stringValue = *val; }
            else
                {
                throw std::runtime_error(_(L"ID column filter requires "
                    "a string value for filtering."));
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
                // more optimal to compare integral types, so do that if
                // reviewing == or !=
                return (
                    m_comparisonType == Comparison::Equals ?
                        m_categoricalColumn->GetValue(rowPosition) == m_groupIdValue :
                        m_categoricalColumn->GetValue(rowPosition) != m_groupIdValue);
                }
            else
                {
                // < or > will require comparing as the strings though since
                // group IDs probably aren't ordered the same ways as the strings
                // would be alphabetically
                const auto currentString =
                    m_categoricalColumn->GetLabelFromID(
                        m_categoricalColumn->GetValue(rowPosition));
                const auto cmpResult = currentString.CmpNoCase(m_stringValue);
                return (
                    m_comparisonType == Comparison::LessThan ?
                        cmpResult < 0 :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                        cmpResult <= 0 :
                    m_comparisonType == Comparison::GreaterThan ?
                        cmpResult > 0 :
                    // GreaterThanOrEqualTo
                        cmpResult >= 0);
                }
            }
        else if (m_columnType == ColumnType::Continuous)
            {
            const auto& dVal = m_continuousColumn->GetValue(rowPosition);
            return (m_comparisonType == Comparison::Equals ?
                        compare_doubles(dVal, m_doubleValue) :
                    m_comparisonType == Comparison::NotEquals ?
                        !compare_doubles(dVal, m_doubleValue) :
                    m_comparisonType == Comparison::LessThan ?
                        compare_doubles_less(dVal, m_doubleValue) :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                        compare_doubles_less_or_equal(dVal, m_doubleValue) :
                    m_comparisonType == Comparison::GreaterThan ?
                        compare_doubles_greater(dVal, m_doubleValue) :
                    // GreaterThanOrEqualTo
                        compare_doubles_greater_or_equal(dVal, m_doubleValue));
            }
        else if (m_columnType == ColumnType::Date)
            {
            const auto& dtVal = m_dateColumn->GetValue(rowPosition);
            return (m_comparisonType == Comparison::Equals ?
                        dtVal == m_dateTimeValue :
                    m_comparisonType == Comparison::NotEquals ?
                        dtVal != m_dateTimeValue :
                    m_comparisonType == Comparison::LessThan ?
                        dtVal < m_dateTimeValue :
                    m_comparisonType == Comparison::LessThanOrEqualTo ?
                        dtVal <= m_dateTimeValue :
                    m_comparisonType == Comparison::GreaterThan ?
                        dtVal > m_dateTimeValue :
                    // GreaterThanOrEqualTo
                        dtVal >= m_dateTimeValue);
            }
        else if (m_columnType == ColumnType::ID)
            {
            const auto cmpResult =
                m_idColumn->GetValue(rowPosition).CmpNoCase(m_stringValue);
            return (m_comparisonType == Comparison::Equals ?
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
                        cmpResult >= 0);
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
