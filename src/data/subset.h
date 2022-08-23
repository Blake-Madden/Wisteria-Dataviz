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

    /** @brief Interface for subsetting a dataset.
        @par Example:
        @code
         auto theData = std::make_shared<Data::Dataset>();
         theData->ImportCSV(L"/home/emma/datasets/Spelling Grades.csv",
            ImportInfo().
            ContinuousColumns({ L"AVG_GRADE"}).
            CategoricalColumns({
                { L"Gender" },
                { L"WEEK_NAME" }
                }));
         Subset dsSubset;
         // dataset with only female observations
         const auto subset =
            dsSubset.SubsetSimple(theData,
                ColumnFilterInfo{ L"Gender", Comparison::Equals, L"Female" });
         // "subset" can now be exported or plotted

         // dataset with only female observations starting from Week 3 or later
         const auto subset2 =
            dsSubset.SubsetAnd(theData,
                {
                ColumnFilterInfo{ L"Gender", Comparison::Equals, L"Female" },
                ColumnFilterInfo{ L"WEEK_NAME", Comparison::GreaterThanOrEqualTo, L"Week 3" }
                });
        @endcode
    */
    class Subset final : public DatasetClone
        {
    public:
        /// @brief Constructor.
        Subset() = default;
        /// @private
        Subset(const Subset&) = delete;
        /// @private
        Subset(Subset&&) = delete;
        /// @private
        Subset& operator=(const Subset&) = delete;
        /// @private
        Subset& operator=(Subset&&) = delete;
        /** @brief Creates a subset, based on a single criterion.
            @param fromDataset The source datasource to subset.
            @param columnFilter The criterion for subsetting, defining the column and value
                to filter on and how to compare the values.
            @returns The subset dataset.
            @throws std::runtime_error If the column or filter value can't be found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] std::shared_ptr<Dataset> SubsetSimple(
            const std::shared_ptr<const Dataset>& fromDataset,
            const ColumnFilterInfo columnFilter);
        /** @brief Creates a subset, based on multiple filters that are ORed together.
            @details In other words, if any of the filters match against an observation
                then it will included in the subset.
            @param fromDataset The source datasource to subset.
            @param columnFilters The criteria for subsetting, defining the columns, values
                to filter on, and how to compare the values.
            @returns The subset dataset.
            @throws std::runtime_error If any column or filter value can't be found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] std::shared_ptr<Dataset> SubsetOr(
            const std::shared_ptr<const Dataset>& fromDataset,
            const std::vector<ColumnFilterInfo>& columnFilters);
        /** @brief Creates a subset, based on multiple filters that are ANDed together.
            @details In other words, all the filters must match against an observation
                for it to be included in the subset.
            @param fromDataset The source datasource to subset.
            @param columnFilters The criteria for subsetting, defining the columns, values
                to filter on, and how to compare the values.
            @returns The subset dataset.
            @throws std::runtime_error If any column or filter value can't be found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] std::shared_ptr<Dataset> SubsetAnd(
            const std::shared_ptr<const Dataset>& fromDataset,
            const std::vector<ColumnFilterInfo>& columnFilters);
        };
    }

/** @}*/

#endif //__WISTERIA_SUBSET_H__
