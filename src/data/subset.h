/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_SUBSET_H__
#define __WISTERIA_SUBSET_H__

#include "clone.h"

namespace Wisteria::Data
    {
    /// @brief Criterion used for matching a row in a dataset.
    struct ColumnFilterInfo
        {
        /// @brief The column name in the dataset to compare against.
        wxString m_columnName;
        /// @brief How to compare the values from the column against the provided value.
        Comparison m_comparisonType{ Comparison::Equals };
        /// @brief The value to compare with.
        DatasetValueType m_value;
        };

    /// @private
    class ColumnFilter
        {
    public:
        /// @brief Constructor.
        /// @param fromDataset The dataset to subset.
        /// @param subsetCriterion The filter criteria.
        ColumnFilter(const std::shared_ptr<const Dataset>& fromDataset,
            const ColumnFilterInfo subsetCriterion);
        /** @brief Determines if the given row in the dataset meets the criteria
                of this filter.
            @param rowPosition The row index in the dataset to review.
            @returns @c true if the row matches the filter, @c false otherwise.*/
        [[nodiscard]] bool MeetsCriterion(const size_t rowPosition) const;
    private:
        const Wisteria::Data::Column<wxString>* m_idColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_categoricalColumn;
        std::vector<Wisteria::Data::Column<wxDateTime>>::const_iterator m_dateColumn;

        GroupIdType m_groupIdValue;
        wxString m_stringValue;
        wxDateTime m_dateTimeValue{ wxInvalidDateTime };
        double m_doubleValue{ std::numeric_limits<double>::quiet_NaN() };

        Comparison m_comparisonType{ Comparison::Equals };

        ColumnType m_columnType{ ColumnType::Continuous };
        };

    /// @brief Interface for subsetting a dataset.
    class DatasetSubset final : public DatasetClone
        {
    public:
        /// @brief Constructor.
        DatasetSubset() = default;
        /// @private
        DatasetSubset(const DatasetClone&) = delete;
        /// @private
        DatasetSubset(DatasetClone&&) = delete;
        /// @private
        DatasetSubset& operator=(const DatasetClone&) = delete;
        /// @private
        DatasetSubset& operator=(DatasetClone&&) = delete;
        /** @brief Creates a subset, based on a single criterion.
            @param fromDataset The source datasource to subset.
            @param subsetCriterion The criterion, defining the column and value
                to filter on and how to compare the values.
            @returns The subset dataset.
            @throws std::runtime_error If column or filter value can't be found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] std::shared_ptr<Dataset> Subset(
            const std::shared_ptr<const Dataset>& fromDataset,
            const ColumnFilterInfo subsetCriterion);
    private:
        
        };
    }

/** @}*/

#endif //__WISTERIA_SUBSET_H__
