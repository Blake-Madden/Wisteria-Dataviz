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
            TableCell(const CellValueType& value, const wxColour color) :
                m_value(value), m_color(color)
                {}
            /// @private
            TableCell() = default;
            /// @brief Gets the value as it is displayed in the cell.
            /// @returns The displayable string for the cell.
            wxString GetDisplayValue() const
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
        private:
            CellValueType m_value{ std::numeric_limits<double>::quiet_NaN() };
            wxColour m_color{ *wxWHITE };
            };

        explicit Table(Wisteria::Canvas* canvas);

        void SetTableSize(const size_t rows, const size_t cols)
            {
            m_table.resize(rows);
            for (auto& row : m_table)
                { row.resize(cols); }
            }
        void ClearTable()
            { m_table.clear(); }
        void SetCellValue(const size_t row, const size_t column, const CellValueType& value)
            {
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
