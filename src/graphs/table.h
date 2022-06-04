/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_TABLE_H__
#define __WISTERIA_TABLE_H__

#include <vector>
#include <variant>
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    class Table final : public Graph2D
        {
    public:
        /// @brief Types of values that can be used for a cell.
        using CellValueType = std::variant<double, wxString>;

        /// @brief A cell in the table.
        class TableCell
            {
            friend class Table;
        public:
            /// @brief Constructor.
            /// @param value The value for the cell.
            /// @param bgColor The cell's background color.
            TableCell(const CellValueType& value, const wxColour bgColor) :
                m_value(value), m_bgColor(bgColor)
                {}
            /// @private
            TableCell() = default;
            /// @brief Gets the value as it is displayed in the cell.
            /// @returns The displayable string for the cell.
            [[nodiscard]] wxString GetDisplayValue() const
                {
                if (const auto strVal{ std::get_if<wxString>(&m_value) }; strVal)
                    { return *strVal; }
                else if (const auto dVal{ std::get_if<double>(&m_value) }; dVal)
                    {
                    if (std::isnan(*dVal))
                        { return wxEmptyString; }
                    return wxNumberFormatter::ToString(*dVal, 0,
                        wxNumberFormatter::Style::Style_WithThousandsSep);
                    }
                else
                    { return wxEmptyString; }
                }
            /// @returns @c true if the cell is text.
            [[nodiscard]] bool IsText() const noexcept
                { return (std::get_if<wxString>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a number.
            [[nodiscard]] bool IsNumeric() const noexcept
                { return (std::get_if<double>(&m_value) != nullptr); }
        private:
            CellValueType m_value{ std::numeric_limits<double>::quiet_NaN() };
            wxColour m_bgColor{ *wxWHITE };
            };

        /// @brief Constructor.
        /// @param canvas The canvas to draw the table on.
        explicit Table(Wisteria::Canvas* canvas);

        /// @brief Sets the size of the table.
        /// @param rows The number of rows.
        /// @param cols The number of columns.
        /// @note If the table is being made smaller, then existing content
        ///     outside of the new size will be removed; other existing content
        ///     will be preserved.\n
        ///     Call ClearTable() to clear any existing content if you wish to
        ///     reset the table.
        void SetTableSize(const size_t rows, const size_t cols)
            {
            m_table.resize(rows);
            for (auto& row : m_table)
                { row.resize(cols); }
            }
        /// @brief Empties the contents of the table.
        void ClearTable()
            { m_table.clear(); }
        /// @brief Sets the value for a cell at a given position.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @param value The value to apply.
        void SetCellValue(const size_t row, const size_t column,
                          const CellValueType& value)
            {
            wxASSERT_MSG(row < m_table.size(), L"Invalid row index!");
            wxASSERT_MSG(column < m_table[row].size(), L"Invalid column index!");
            if (row < m_table.size())
                {
                if (column < m_table[row].size())
                    { m_table[row][column].m_value = value; }
                }
            }
    private:
        void RecalcSizes(wxDC& dc) final;
        std::vector<std::vector<TableCell>> m_table;
        };
    }

/** @}*/

#endif //__WISTERIA_TABLE_H__
