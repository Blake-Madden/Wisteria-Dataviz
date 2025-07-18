/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PIVOT_H
#define WISTERIA_PIVOT_H

#include "dataset.h"

namespace Wisteria::Data
    {
    /// @private
    /// @brief A pivoted (wider) row created from a "stacked" row.
    class PivotedWiderRow
        {
        friend class Pivot;

      public:
        PivotedWiderRow(wxString identifier,
                        const std::vector<std::pair<wxString, CategoricalOrIdDataType>>& idColumns,
                        const std::map<wxString, double, wxStringLessNoCase>& pivotedColumns)
            : m_id(std::move(identifier)), m_idColumns(idColumns), m_pivotedColumns(pivotedColumns)
            {
            }

        /// @private
        [[nodiscard]]
        bool operator<(const PivotedWiderRow& that) const
            {
            return m_id.CmpNoCase(that.m_id) < 0;
            }

        /// @brief Combines rows with the same ID(s), adding new pivoted
        ///     columns, or combining values with common pivots.
        void Combine(const PivotedWiderRow& that);
        /// @brief Adds any missing pivoted columns and sets its value to NaN.
        void Expand(const std::set<wxString, wxStringLessNoCase>& pivotedColumnNames,
                    const double fillValue = std::numeric_limits<double>::quiet_NaN());

      private:
        // ID hash, which is the ID column(s) names combined into one string
        wxString m_id;
        // ID columns, used for grouping and comparing rows
        // (these should remain in the same order that the client specifies)
        std::vector<std::pair<wxString, CategoricalOrIdDataType>> m_idColumns;
        // Pivoted column(s) names and values (these are sorted by name)
        std::map<wxString, double, wxStringLessNoCase> m_pivotedColumns;
        };

    /** @brief Pivots a dataset wider (a.k.a. unstacking or melting data).*/
    class Pivot
        {
      public:
        /// @brief Constructor.
        Pivot() = default;
        /// @private
        Pivot(const Pivot&) = delete;
        /// @private
        Pivot& operator=(const Pivot&) = delete;
        /** @brief Creates a single row for each observation, with a categorical column
                split into new columns, filled with respective values from specified
                continuous columns.
            @details Pivoting wider is also known as unstacking, melting, or spreading
                a dataset and is useful for breaking a grouping variable into new columns,
                ensuring that each row is a unique observation.
            @param dataset The dataset to pivot.
            @param IdColumns The column(s) used to identify a unique observation.
            @param namesFromColumn The categorical column that will have each label
                converted into a new column.
            @param valuesFromColumns Continuous columns that will be copied into the new
                columns created by the labels from @c namesFromColumn.\n
                If multiple value columns are provided, then an extra column will be created
                for each label column from every value column.\n
                If no columns are provided, then frequency counts of each unique combination
                of ID labels will be used as the value.
            @param namesSep If multiple value columns are provided, then this separator will
                join the label from @c namesFromColumn and the value column name.
            @param namesPrefix A string to prepend to newly created pivot columns.
            @param fillValue If any observation is missing a label from @c namesFromColumn
                that other observations have, then an empty cell will be added for that
                column. This value will be used for fill this cell, with missing data (i.e., NaN)
                being the default.
            @todo Add unit test.
            @returns The pivoted dataset.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        PivotWider(const std::shared_ptr<const Dataset>& dataset,
                   const std::vector<wxString>& IdColumns, const wxString& namesFromColumn,
                   const std::vector<wxString>& valuesFromColumns, const wxString& namesSep = L"_",
                   const wxString& namesPrefix = wxEmptyString,
                   const double fillValue = std::numeric_limits<double>::quiet_NaN());

        /** @brief Combines multiple columns into a grouping column and value column.\n
                Multiple target grouping columns can also be created if a regular expression
                is provided to split the initial columns' names.
            @details Pivoting longer is also known as stacking, casting, or gathering
                a dataset. This is useful for converting a "one row for each observation" dataset
                into a dataset with grouping columns
                (which is generally more appropriate for analyses).
            @param dataset The dataset to pivot.
            @param columnsToKeep The columns to not pivot. These will be copied to the new dataset
                and will have their values filled in all new rows created from their observation.
                These would usually be the ID and grouping columns.\n
                These columns can be of any type, including the ID column.\n
                Note that the output will be sorted based on the ordering of these columns.
            @param fromColumns The continuous column(s) to pivot into longer format.
            @param namesTo The target column(s) to move the names from the @c fromColumns into.\n
                Basically, this will be a grouping column that uses the original
                column names as its groups.
            @param valuesTo The column to move the values from the @c fromColumns into.\n
                This will essentially be the values from the @c fromColumns stacked on
                top of each other.
                The original column name for each value will appear next to it in the
                @c namesTo column.
            @param namesPattern If needing to split the names of the columns into
                multiple target columns, this regular expression can be used.
                It should contain capture groups, where each group
                will be the name of a new target column.\n
                Leave blank (the default) to use the full name(s) of the
                @c fromColumns as the labels.
            @todo Add unit test.
            @returns The pivoted dataset.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        PivotLonger(const std::shared_ptr<const Dataset>& dataset,
                    const std::vector<wxString>& columnsToKeep,
                    const std::vector<wxString>& fromColumns, const std::vector<wxString>& namesTo,
                    const wxString& valuesTo, const wxString& namesPattern = wxEmptyString);
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_PIVOT_H
