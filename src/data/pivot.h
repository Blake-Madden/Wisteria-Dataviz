/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_PIVOT_H__
#define __WISTERIA_PIVOT_H__

#include "dataset.h"

namespace Wisteria::Data
    {
    /// @private
    /// @brief A pivoted (wider) row created from a "stacked" row.
    class PivotedWiderRow
        {
        friend class Pivot;
    public:
        PivotedWiderRow(const wxString& id,
            std::vector<std::pair<wxString, CategoricalOrIdDataType>>& idColumns,
            std::map<wxString, double, StringCmpNoCase>& pivotedColumns) :
            m_Id(id), m_idColumns(idColumns), m_pivotedColumns(pivotedColumns)
            {}
        [[nodiscard]] bool operator<(const PivotedWiderRow& that) const
            { return m_Id.CmpNoCase(that.m_Id) < 0; }
        /// @brief Combines rows with the same ID(s), adding new pivoted
        ///     columns, or combining values with common pivots.
        void Combine(const PivotedWiderRow& that);
        /// @brief Adds any missing pivoted columns and sets its value to NaN.
        void Expand(const std::set<wxString, StringCmpNoCase>& pivotedColumnNames,
                    const double fillValue = std::numeric_limits<double>::quiet_NaN());
    private:
        // ID hash, which is the ID column(s) names combined into one string
        wxString m_Id;
        // ID columns, used for grouping and comparing rows
        // (these should remain in the same order that the client specifies)
        std::vector<std::pair<wxString, CategoricalOrIdDataType>> m_idColumns;
        // Pivoted column(s) names and values
        // (these are sorted by name)
        std::map<wxString, double, StringCmpNoCase> m_pivotedColumns;
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
        Pivot(Pivot&&) = delete;
        /// @private
        Pivot& operator=(const Pivot&) = delete;
        /// @private
        Pivot& operator=(Pivot&&) = delete;
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
                columns created by teh labels from @c namesFromColumn.\n
                If multiple value columns are provided, then an extra column will be created
                for each label column for each values column.
            @param namesSep If multiple value columns are provided, then this separator will
                join the label from @c namesFromColumn and the value column name.
            @param namesPrefix A string to prepend to newly created pivot columns.
            @param fillValue If any observation is missing a label from @c namesFromColumn
                that other observations have, then an empty cell will be added for that
                column. This value will be used for fill this cell, with missing data
                being the default.
            @returns The pivoted dataset.*/
        [[nodiscard]] std::shared_ptr<Dataset> PivotWider(
            const std::shared_ptr<const Dataset>& dataset,
            const std::vector<wxString>& IdColumns,
            const wxString& namesFromColumn,
            const std::vector<wxString>& valuesFromColumns,
            const wxString& namesSep = L"_",
            const wxString& namesPrefix = wxEmptyString,
            const double fillValue = std::numeric_limits<double>::quiet_NaN());
        };
    }

/** @}*/

#endif //__WISTERIA_PIVOT_H__
