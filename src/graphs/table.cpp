///////////////////////////////////////////////////////////////////////////////
// Name:        table.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "table.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::Table, Wisteria::Graphs::Graph2D)

    using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void Table::TableCell::SetFormat(const TableCellFormat cellFormat) noexcept
        {
        m_valueFormat = cellFormat;
        if (m_valueFormat == TableCellFormat::General)
            {
            m_precision = 0;
            m_prefix.clear();
            m_horizontalCellAlignment = std::nullopt;
            }
        else if (m_valueFormat == TableCellFormat::Percent)
            {
            m_precision = 0;
            m_prefix.clear();
            m_horizontalCellAlignment = PageHorizontalAlignment::RightAligned;
            }
        else if (m_valueFormat == TableCellFormat::PercentChange ||
                 m_valueFormat == TableCellFormat::GeneralChange)
            {
            m_precision = 0;
            m_prefix = L"\u2014"; // em dash, will be changed dynamically later based on value
            m_horizontalCellAlignment = PageHorizontalAlignment::RightAligned;
            }
        else if (m_valueFormat == TableCellFormat::Accounting)
            {
            m_precision = 2;
            m_prefix = Wisteria::Settings::GetCurrencySymbol();
            m_horizontalCellAlignment = PageHorizontalAlignment::RightAligned;
            }
        }

    //----------------------------------------------------------------
    wxString Table::TableCell::GetDisplayValue() const
        {
        if (const auto strVal{ std::get_if<wxString>(&m_value) }; strVal != nullptr)
            {
            return *strVal;
            }
        else if (const auto dVal{ std::get_if<double>(&m_value) }; dVal != nullptr)
            {
            if (std::isnan(*dVal))
                {
                return wxString{};
                }
            else if (m_valueFormat == TableCellFormat::Percent ||
                     m_valueFormat == TableCellFormat::PercentChange)
                {
                return wxString::Format(
                    /* TRANSLATORS: Percentage value and percentage symbol (%%).
                       '%%' can be changed and/or moved within string. */
                    _(L"%s%%"),
                    wxNumberFormatter::ToString((*dVal) * 100, m_precision,
                                                wxNumberFormatter::Style::Style_WithThousandsSep));
                }
            else if (m_valueFormat == TableCellFormat::Accounting)
                {
                if (GetSuppressionThreshold().has_value() &&
                    *dVal < GetSuppressionThreshold().value())
                    {
                    return GetSuppressionLabel();
                    }
                return ((*dVal < 0) ? wxString{ L"(" } : wxString{}) +
                       wxNumberFormatter::ToString(
                           *dVal, m_precision, wxNumberFormatter::Style::Style_WithThousandsSep) +
                       ((*dVal < 0) ? wxString{ L")" } : wxString{});
                }
            else
                {
                if (GetSuppressionThreshold().has_value() &&
                    *dVal < GetSuppressionThreshold().value())
                    {
                    return GetSuppressionLabel();
                    }
                return wxNumberFormatter::ToString(
                    *dVal, m_precision, wxNumberFormatter::Style::Style_WithThousandsSep);
                }
            }
        else if (const auto doubleVal{ std::get_if<std::pair<double, double>>(&m_value) };
                 doubleVal != nullptr)
            {
            if (std::isnan(doubleVal->first) || std::isnan(doubleVal->second))
                {
                return wxString{};
                }
            if (doubleVal->first > doubleVal->second)
                {
                return wxString::Format(
                    L"%s : 1", wxNumberFormatter::ToString(
                                   safe_divide(doubleVal->first, doubleVal->second), m_precision,
                                   wxNumberFormatter::Style::Style_WithThousandsSep |
                                       wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            else
                {
                return wxString::Format(
                    L"1 : %s", wxNumberFormatter::ToString(
                                   safe_divide(doubleVal->second, doubleVal->first), m_precision,
                                   wxNumberFormatter::Style::Style_WithThousandsSep |
                                       wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            }
        else if (const auto doubleVal2{ std::get_if<wxDateTime>(&m_value) }; doubleVal2 != nullptr)
            {
            if (!doubleVal2->IsValid())
                {
                return wxString{};
                }
            return doubleVal2->FormatDate();
            }
        else
            {
            return wxString{};
            }
        }

    //----------------------------------------------------------------
    void Table::InsertGroupHeader(const std::vector<wxString>& groups)
        {
        if (GetRowCount() > 0 && InsertRow(0))
            {
            for (size_t col = 0; col < GetColumnCount(); ++col)
                {
                auto currentVal = GetCell(1, col).GetDisplayValue();
                for (const auto& gr : groups)
                    {
                    if (currentVal.starts_with(gr))
                        {
                        GetCell(0, col).SetValue(gr);
                        GetCell(1, col).SetValue(currentVal.erase(0, gr.length()).Trim(false));
                        }
                    }
                }
            GroupRow(0);
            m_hasGroupHeader = true;
            }
        }

    //----------------------------------------------------------------
    Table::TableCell& Table::GetCell(const size_t row, const size_t column)
        {
        wxASSERT_LEVEL_2_MSG(row < GetRowCount(),
                             wxString::Format(L"Invalid row index (%zu)!", row));
        wxASSERT_LEVEL_2_MSG(column < m_table[row].size(),
                             wxString::Format(L"Invalid column index (%zu)!", column));
        if (row >= GetRowCount() || column >= m_table[row].size())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Invalid cell index (row %zu, column %zu)."), row, column)
                    .ToUTF8());
            }
        return m_table[row][column];
        }

    //----------------------------------------------------------------
    std::optional<Table::TableCell> Table::GetEclipsingRowWiseCell(const size_t row,
                                                                   const size_t column)
        {
        if (row > 0 && // first row can't have a row-wise parent
            row < GetRowCount() && column < m_table[row].size())
            {
            int parentRow = row - 1;
            while (parentRow >= 0)
                {
                // going backwards, find first cell above that is multi-row and
                // see if it overlays this cell
                const auto& parentCell = GetCell(parentRow, column);
                if (parentCell.m_rowCount > 1)
                    {
                    if (static_cast<size_t>(parentCell.m_rowCount + parentRow) > row)
                        {
                        return parentCell;
                        }
                    else
                        {
                        return std::nullopt;
                        }
                    }
                --parentRow;
                }
            }
        return std::nullopt;
        }

    //----------------------------------------------------------------
    std::optional<Table::TableCell> Table::GetEclipsingColumnWiseCell(const size_t row,
                                                                      const size_t column)
        {
        if (column > 0 && // first column can't have a column-wise parent
            column < GetColumnCount() && row < GetRowCount())
            {
            int parentColumn = column - 1;
            while (parentColumn >= 0)
                {
                // going backwards, find first cell to the left that is multi-column and
                // see if it overlays this cell
                const auto& parentCell = GetCell(row, parentColumn);
                if (parentCell.m_columnCount > 1)
                    {
                    if (static_cast<size_t>(parentCell.m_columnCount + parentColumn) > column)
                        {
                        return parentCell;
                        }
                    else
                        {
                        return std::nullopt;
                        }
                    }
                --parentColumn;
                }
            }
        return std::nullopt;
        }

    //----------------------------------------------------------------
    wxRect Table::GetCachedCellRect(const size_t row, const size_t column) const
        {
        wxASSERT_LEVEL_2_MSG(row < m_cachedCellRects.size(),
                             wxString::Format(L"Invalid row index (%zu)!", row));
        wxASSERT_LEVEL_2_MSG(column < m_cachedCellRects[row].size(),
                             wxString::Format(L"Invalid column index (%zu)!", column));
        if (row >= m_cachedCellRects.size() || column >= m_cachedCellRects[row].size())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Invalid cell index (row %zu, column %zu)."), row, column)
                    .ToUTF8());
            }
        return m_cachedCellRects[row][column];
        }

    //----------------------------------------------------------------
    Table::Table(Wisteria::Canvas* canvas) : Graph2D(canvas)
        {
        GetPen() = ColorBrewer::GetColor(Colors::Color::Almond);

        // arbitrary ranges, just need to create any sort of plotting area
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void Table::SetData(const std::shared_ptr<const Data::Dataset>& data,
                        const std::vector<wxString>& columns, const bool transpose /*= false*/)
        {
        ClearTable();

        m_dataSize = std::make_pair(data->GetRowCount() + 1, // include the column name
                                    columns.size());

        if (transpose)
            {
            SetTableSize(columns.size(), data->GetRowCount() + 1);

            size_t currentRow{ 0 };

            for (const auto& colName : columns)
                {
                // the row header
                GetCell(currentRow, 0).m_value = colName;
                if (auto continuousCol = data->GetContinuousColumn(colName);
                    continuousCol != data->GetContinuousColumns().cend())
                    {
                    for (size_t i = 0; i < continuousCol->GetValues().size(); ++i)
                        {
                        GetCell(currentRow, i + 1).m_value = continuousCol->GetValue(i);
                        }
                    }
                else if (auto catCol = data->GetCategoricalColumn(colName);
                         catCol != data->GetCategoricalColumns().cend())
                    {
                    for (size_t i = 0; i < catCol->GetValues().size(); ++i)
                        {
                        GetCell(currentRow, i + 1).m_value =
                            catCol->GetLabelFromID(catCol->GetValue(i));
                        }
                    }
                else if (auto dateCol = data->GetDateColumn(colName);
                         dateCol != data->GetDateColumns().cend())
                    {
                    for (size_t i = 0; i < dateCol->GetValues().size(); ++i)
                        {
                        GetCell(currentRow, i + 1).m_value = dateCol->GetValue(i);
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': column not found for table."), colName)
                            .ToUTF8());
                    }
                ++currentRow;
                }
            }
        else
            {
            SetTableSize(data->GetRowCount() + 1, columns.size());

            size_t currentColumn{ 0 };

            for (const auto& colName : columns)
                {
                // the column header
                GetCell(0, currentColumn).m_value = colName;
                if (auto continuousCol = data->GetContinuousColumn(colName);
                    continuousCol != data->GetContinuousColumns().cend())
                    {
                    for (size_t i = 0; i < continuousCol->GetValues().size(); ++i)
                        {
                        GetCell(i + 1, currentColumn).m_value = continuousCol->GetValue(i);
                        }
                    }
                else if (auto catCol = data->GetCategoricalColumn(colName);
                         catCol != data->GetCategoricalColumns().cend())
                    {
                    for (size_t i = 0; i < catCol->GetValues().size(); ++i)
                        {
                        GetCell(i + 1, currentColumn).m_value =
                            catCol->GetLabelFromID(catCol->GetValue(i));
                        }
                    }
                else if (auto dateCol = data->GetDateColumn(colName);
                         dateCol != data->GetDateColumns().cend())
                    {
                    for (size_t i = 0; i < dateCol->GetValues().size(); ++i)
                        {
                        GetCell(i + 1, currentColumn).m_value = dateCol->GetValue(i);
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': column not found for table."), colName)
                            .ToUTF8());
                    }
                ++currentColumn;
                }
            }
        }

    //----------------------------------------------------------------
    void Table::Sort(const size_t columnToSort, std::vector<wxString> labels,
                     const SortDirection direction)
        {
        if (m_table.empty())
            {
            return;
            }

        // insert the column header at the top of the labels
        labels.insert(labels.cbegin(), 1, m_table[0][columnToSort].GetDisplayValue());
        std::set<wxString> columnLabels;
        // make sure all cell labels are unique in the column
        for (size_t i = 0; i < m_table.size(); ++i)
            {
            columnLabels.insert(m_table[i][columnToSort].GetDisplayValue());
            }
        if (columnLabels.size() != m_table.size())
            {
            return;
            }

        ClearTableInformation();

        std::vector<wxString> otherLabels;
        // get the cells labels that the caller did not specify
        for (size_t i = 0; i < m_table.size(); ++i)
            {
            const auto& cellLabel{ m_table[i][columnToSort] };
            const auto foundPos =
                std::ranges::find_if(std::as_const(labels), [&cellLabel](const auto& label)
                                     { return label.CmpNoCase(cellLabel.GetDisplayValue()) == 0; });
            if (foundPos == labels.cend())
                {
                otherLabels.push_back(cellLabel.GetDisplayValue());
                }
            }
        // sort them, set the expected order, and copy them after the labels the caller provided
        std::ranges::sort(otherLabels,
                          [](const auto& lhv, const auto& rhv) { return lhv.CmpNoCase(rhv) < 0; });
        if (direction == SortDirection::SortDescending)
            {
            std::ranges::reverse(otherLabels);
            }
        std::ranges::copy(std::as_const(otherLabels), std::back_inserter(labels));

        // adapted from
        // https://stackoverflow.com/questions/69764803/how-to-sort-reorder-a-vec-by-indices
        std::vector<size_t> indices;
        const auto reorderRows = [&indices, this]()
        {
            for (size_t idx = 0; idx < m_table.size(); ++idx)
                {
                if (indices[idx] != idx)
                    {
                    auto current_idx = idx;
                    while (true)
                        {
                        const auto target_idx = indices[current_idx];
                        indices[current_idx] = current_idx;
                        if (indices[target_idx] == target_idx)
                            {
                            break;
                            }
                        std::swap(m_table[current_idx], m_table[target_idx]);
                        current_idx = target_idx;
                        }
                    }
                }
        };

        // get the indices into the rows based on the order of the provided labels
        for (const auto& label : labels)
            {
            std::optional<size_t> foundRow{ std::nullopt };
            for (size_t i = 0; i < m_table.size(); ++i)
                {
                if (const auto& cellLabel{ m_table[i][columnToSort] };
                    label.CmpNoCase(cellLabel.GetDisplayValue()) == 0)
                    {
                    foundRow = i;
                    break;
                    }
                }
            if (foundRow)
                {
                indices.push_back(foundRow.value());
                }
            else // shouldn't happen
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': cell label not found when sorting."), label)
                        .ToUTF8());
                }
            }

        reorderRows();
        }

    //----------------------------------------------------------------
    void Table::Sort(const size_t columnToSort, const SortDirection direction)
        {
        if (m_table.empty())
            {
            return;
            }

        ClearTableInformation();

        std::sort(m_table.begin() + 1 /* don't sort header */, m_table.end(),
                  [&columnToSort, &direction](const auto& lhv, const auto& rhv)
                  {
                      return (direction == SortDirection::SortAscending) ?
                                 lhv[columnToSort].GetDisplayValue().CmpNoCase(
                                     rhv[columnToSort].GetDisplayValue()) < 0 :
                                 rhv[columnToSort].GetDisplayValue().CmpNoCase(
                                     lhv[columnToSort].GetDisplayValue()) < 0;
                  });
        }

    //----------------------------------------------------------------
    void Table::CalculateAggregate(const AggregateInfo& aggInfo, Table::TableCell& aggCell,
                                   const std::vector<double>& values)
        {
        if (values.size())
            {
            if (aggInfo.m_type == AggregateType::Total)
                {
                aggCell.m_value = std::accumulate(values.cbegin(), values.cend(), 0.0);
                }
            else if (aggInfo.m_type == AggregateType::ChangePercent && values.size() > 1)
                {
                const auto oldValue = values.front();
                const auto newValue = values.back();
                // if starting value is zero, then percent change is invalid
                // since you can't divide with it
                if (oldValue == 0 || std::isnan(oldValue) || std::isnan(newValue))
                    {
                    aggCell.m_value = std::numeric_limits<double>::quiet_NaN();
                    }
                else
                    {
                    aggCell.m_value = safe_divide(newValue - oldValue, oldValue);
                    aggCell.m_colorCodePrefix = true;
                    }
                aggCell.SetFormat(TableCellFormat::PercentChange);
                }
            else if (aggInfo.m_type == AggregateType::Change && values.size() > 1)
                {
                const auto firstValue = values.front();
                const auto secondValue = values.back();
                aggCell.m_value = secondValue - firstValue;
                aggCell.SetFormat(TableCellFormat::GeneralChange);
                aggCell.m_colorCodePrefix = true;
                }
            else if (aggInfo.m_type == AggregateType::Ratio && values.size() > 1)
                {
                const auto firstValue = values.front();
                const auto secondValue = values.back();
                aggCell.m_value = std::make_pair(firstValue, secondValue);
                }
            }
        }

    //----------------------------------------------------------------
    void Table::InsertRowTotals(const std::optional<wxColour>& bkColor /*= std::nullopt*/)
        {
        if (GetColumnCount() &&
            // at least one row of data beneath header(s)
            GetRowCount() > static_cast<size_t>(1 + (m_hasGroupHeader ? 1 : 0)))
            {
            std::vector<std::pair<size_t, size_t>> indexAndRowCounts;
            for (size_t rowIndex = 1 + (m_hasGroupHeader ? 1 : 0) /* skip header(s)*/;
                 rowIndex < GetRowCount(); ++rowIndex)
                {
                const auto& row = m_table[rowIndex];
                if (row.size() && row[0].m_rowCount >= 1)
                    {
                    indexAndRowCounts.push_back(std::make_pair(rowIndex, row[0].m_rowCount));
                    }
                }

            // has groups, so add grand total and group subtotals
            if (indexAndRowCounts.size() &&
                // parent group, sub group, then value columns
                GetColumnCount() > 2 &&
                // first two columns in the first row of data appear to be grouping labels
                GetCell(1, 0).IsText() && GetCell(1, 1).IsText())
                {
                InsertAggregateRow(Table::AggregateInfo(AggregateType::Total), _(L"Grand Total"),
                                   std::nullopt, bkColor);
                for (auto rowIter = indexAndRowCounts.crbegin();
                     rowIter != indexAndRowCounts.crend(); ++rowIter)
                    {
                    const auto lastSubgroupRow{ rowIter->first + rowIter->second - 1 };
                    InsertAggregateRow(Table::AggregateInfo(AggregateType::Total)
                                           .FirstCell(rowIter->first)
                                           .LastCell(lastSubgroupRow),
                                       std::nullopt, rowIter->first + rowIter->second, bkColor);
                    // make parent group consume first cell of subtotal row
                    GetCell(rowIter->first, 0).m_rowCount++;
                    GetCell(lastSubgroupRow + 1, 1).m_value = _(L"Total");
                    }
                }
            // no groups, so just add an overall total row at the bottom
            else
                {
                InsertAggregateRow(Table::AggregateInfo(AggregateType::Total), _(L"Total"),
                                   std::nullopt, bkColor);
                }
            }
        }

    //----------------------------------------------------------------
    void Table::InsertAggregateRow(const AggregateInfo& aggInfo,
                                   std::optional<wxString> rowName /*= std::nullopt*/,
                                   std::optional<size_t> rowIndex /*= std::nullopt*/,
                                   const std::optional<wxColour>& bkColor /*= std::nullopt*/,
                                   std::optional<std::bitset<4>> borders /*= std::nullopt*/)
        {
        if (GetColumnCount())
            {
            const auto rIndex = (rowIndex.has_value() ? rowIndex.value() : GetRowCount());
            InsertRow(rIndex);
            m_currentAggregateRows.insert(rIndex);
            m_aggregateRows.insert(std::make_pair(rIndex, aggInfo.m_type));
            if (rowName.has_value())
                {
                GetCell(rIndex, 0).SetValue(rowName.value());
                }
            BoldRow(rIndex, std::nullopt);

            SetRowBackgroundColor(rIndex,
                                  bkColor.has_value() ?
                                      bkColor.value() :
                                      ColorBrewer::GetColor(Colors::Color::LightGray),
                                  std::nullopt);

            // if overriding default borders for the cells in this column
            if (borders.has_value())
                {
                SetRowBorders(rIndex, borders.value().test(0), borders.value().test(1),
                              borders.value().test(2), borders.value().test(3));
                }

            std::vector<double> colValues;
            for (size_t currentCol = (rowName.has_value() ? 1 : 0); currentCol < GetColumnCount();
                 ++currentCol)
                {
                colValues.clear();
                // tally values from the whole row, unless a custom range was defined
                for (size_t currentRow =
                         (aggInfo.m_cell1.has_value() ? aggInfo.m_cell1.value() : 0);
                     currentRow <
                     (aggInfo.m_cell2.has_value() ? aggInfo.m_cell2.value() + 1 : rIndex);
                     ++currentRow)
                    {
                    // skip over columns that are aggregates
                    if (m_currentAggregateColumns.contains(currentCol))
                        {
                        continue;
                        }
                    const auto& cell = GetCell(currentRow, currentCol);
                    if (cell.IsNumeric() && !std::isnan(cell.GetDoubleValue()))
                        {
                        const double valueOrSuppressed =
                            (cell.GetSuppressionThreshold().has_value() &&
                             cell.GetDoubleValue() < cell.GetSuppressionThreshold()) ?
                                std::numeric_limits<double>::quiet_NaN() :
                                cell.GetDoubleValue();
                        // throw out unused precision
                        if (cell.GetFormat() == TableCellFormat::Percent ||
                            cell.GetFormat() == TableCellFormat::PercentChange)
                            {
                            colValues.push_back(round_decimal_place(
                                valueOrSuppressed * 100, (cell.GetPrecision() == 0) ?
                                                             0 :
                                                             std::pow(10, cell.GetPrecision())));
                            }
                        else
                            {
                            colValues.push_back(round_decimal_place(
                                valueOrSuppressed, (cell.GetPrecision() == 0) ?
                                                       0 :
                                                       std::pow(10, cell.GetPrecision())));
                            }
                        }
                    // If NOT totaling (e.g., change percent), custom start and/or end column was
                    // requested and it is not a valid number, then insert NaN.
                    const bool onCustomStartRow{ aggInfo.m_cell1.has_value() &&
                                                 aggInfo.m_cell1.value() == currentRow };
                    const bool onCustomEndRow{ aggInfo.m_cell2.has_value() &&
                                               aggInfo.m_cell2.value() == currentRow };
                    if (aggInfo.m_type != AggregateType::Total &&
                        (onCustomStartRow || onCustomEndRow) &&
                        (!cell.IsNumeric() || std::isnan(cell.GetDoubleValue())))
                        {
                        colValues.push_back(cell.GetDoubleValue());
                        }
                    CalculateAggregate(aggInfo, GetCell(rIndex, currentCol), colValues);
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void
    Table::InsertAggregateColumn(const AggregateInfo& aggInfo,
                                 const std::optional<wxString>& colName /*= std::nullopt*/,
                                 const std::optional<size_t> colIndex /*= std::nullopt*/,
                                 const std::optional<bool> useAdjacentColors /*= std::nullopt*/,
                                 const std::optional<wxColour>& bkColor /*= std::nullopt*/,
                                 const std::optional<std::bitset<4>> borders /*= std::nullopt*/)
        {
        if (GetColumnCount())
            {
            const auto columnIndex = (colIndex.has_value() ? colIndex.value() : GetColumnCount());
            InsertColumn(columnIndex);
            m_currentAggregateColumns.insert(columnIndex);
            m_aggregateColumns.insert(std::make_pair(columnIndex, aggInfo.m_type));
            if (colName.has_value())
                {
                GetCell(0, columnIndex).SetValue(colName.value());
                GetCell(0, columnIndex).SetTextAlignment(TextAlignment::Centered);
                GetCell(0, columnIndex)
                    .SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                }
            BoldColumn(columnIndex, std::nullopt);

            if (useAdjacentColors.has_value() && useAdjacentColors.value() &&
                (columnIndex > 0 || GetColumnCount() > 1))
                {
                const auto adjacentColumnIndex =
                    ((columnIndex > 0) ? columnIndex - 1 : columnIndex + 1);
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    auto& cell = m_table[i][columnIndex];
                    cell.m_bgColor = m_table[i][adjacentColumnIndex].m_bgColor;
                    }
                }
            else if (bkColor.has_value() && bkColor.value().IsOk())
                {
                SetColumnBackgroundColor(columnIndex, bkColor.value(), std::nullopt);
                }
            else
                {
                SetColumnBackgroundColor(
                    columnIndex, ColorBrewer::GetColor(Colors::Color::LightGray), std::nullopt);
                }
            // if overriding default borders for the cells in this column
            if (borders.has_value())
                {
                SetColumnBorders(columnIndex, borders.value().test(0), borders.value().test(1),
                                 borders.value().test(2), borders.value().test(3));
                }

            std::vector<double> rowValues;
            for (size_t rowCounter = (colName.has_value() ? 1 : 0); rowCounter < m_table.size();
                 ++rowCounter)
                {
                rowValues.clear();
                std::pair<size_t, wxString> numOfSuppressedValues{ 0, wxString{} };
                // tally values from the whole row, unless a custom range was defined
                for (size_t i = (aggInfo.m_cell1.has_value() ? aggInfo.m_cell1.value() : 0);
                     i < (aggInfo.m_cell2.has_value() ? aggInfo.m_cell2.value() + 1 : columnIndex);
                     ++i)
                    {
                    // skip over columns that are aggregates
                    if (m_currentAggregateRows.contains(i))
                        {
                        continue;
                        }
                    const auto& cell = GetCell(rowCounter, i);
                    if (cell.IsNumeric() && !std::isnan(cell.GetDoubleValue()))
                        {
                        const double valueOrSuppressed =
                            (cell.GetSuppressionThreshold().has_value() &&
                             cell.GetDoubleValue() < cell.GetSuppressionThreshold()) ?
                                std::numeric_limits<double>::quiet_NaN() :
                                cell.GetDoubleValue();
                        // throw out unused precision
                        if (cell.GetFormat() == TableCellFormat::Percent ||
                            cell.GetFormat() == TableCellFormat::PercentChange)
                            {
                            rowValues.push_back(round_decimal_place(
                                valueOrSuppressed * 100, (cell.GetPrecision() == 0) ?
                                                             0 :
                                                             std::pow(10, cell.GetPrecision())));
                            }
                        else
                            {
                            rowValues.push_back(round_decimal_place(
                                valueOrSuppressed, (cell.GetPrecision() == 0) ?
                                                       0 :
                                                       std::pow(10, cell.GetPrecision())));
                            }
                        }
                    // If NOT totaling (e.g., change percent), custom start and/or end column was
                    // requested and it is not a valid number, then insert NaN.
                    const bool onCustomStartColumn{ aggInfo.m_cell1.has_value() &&
                                                    aggInfo.m_cell1.value() == i };
                    const bool onCustomEndColumn{ aggInfo.m_cell2.has_value() &&
                                                  aggInfo.m_cell2.value() == i };
                    if (aggInfo.m_type != AggregateType::Total &&
                        (onCustomStartColumn || onCustomEndColumn) &&
                        (!cell.IsNumeric() || std::isnan(cell.GetDoubleValue())))
                        {
                        rowValues.push_back(cell.GetDoubleValue());
                        }
                    }
                CalculateAggregate(aggInfo, GetCell(rowCounter, columnIndex), rowValues);
                }
            }
        }

    //----------------------------------------------------------------
    void Table::AddCellAnnotation(const CellAnnotation& cellNote)
        {
        if (cellNote.m_cells.empty())
            {
            return;
            }

        // turn off customized full-width specification if adding annotations
        if (GetMinWidthProportion().has_value() &&
            compare_doubles(GetMinWidthProportion().value(), math_constants::full))
            {
            SetMinWidthProportion(std::nullopt);
            }

        for (const auto& cell : cellNote.m_cells)
            {
            GetCell(cell.m_row, cell.m_column).Highlight(true);
            if (cellNote.m_bgColor.IsOk())
                {
                GetCell(cell.m_row, cell.m_column).SetBackgroundColor(cellNote.m_bgColor);
                }
            }
        m_cellAnnotations.push_back(cellNote);
        }

    //----------------------------------------------------------------
    void Table::AddCellAnnotation(CellAnnotation&& cellNote)
        {
        if (cellNote.m_cells.empty())
            {
            return;
            }
        // turn off customized full-width specification if adding annotations
        if (GetMinWidthProportion().has_value() &&
            compare_doubles(GetMinWidthProportion().value(), math_constants::full))
            {
            SetMinWidthProportion(std::nullopt);
            }

        for (const auto& cell : cellNote.m_cells)
            {
            GetCell(cell.m_row, cell.m_column).Highlight(true);
            if (cellNote.m_bgColor.IsOk())
                {
                GetCell(cell.m_row, cell.m_column).SetBackgroundColor(cellNote.m_bgColor);
                }
            }
        m_cellAnnotations.push_back(cellNote);
        }

    //----------------------------------------------------------------
    void Table::ApplyAlternateRowColors(
        const wxColour& baseColor, const size_t startRow /*= 0*/,
        const std::optional<std::set<size_t>>& columnStops /*= std::nullopt*/)
        {
        if (!baseColor.IsOk())
            {
            return;
            }

        const auto alternateColor = ColorContrast::ShadeOrTint(baseColor, math_constants::tenth);

        // start with base color
        SetRowBackgroundColor(startRow, baseColor, columnStops);
        // start alternating
        const auto rowStop =
            (m_clearTrailingRowFormatting ? std::min(m_dataSize.first, GetRowCount()) :
                                            GetRowCount());
        for (size_t row = startRow + 1; row < rowStop; ++row)
            {
            auto& currentRow = m_table[row];
            for (size_t col = 0; col < currentRow.size(); ++col)
                {
                // explicitly skipping this column
                if (columnStops.has_value() && columnStops.value().contains(col))
                    {
                    continue;
                    }
                // If this cell eclipsed by the cell above it? Skip if so.
                else if (const auto parentCell = GetEclipsingRowWiseCell(row, col);
                         parentCell.has_value())
                    {
                    continue;
                    }
                else
                    {
                    // Use the alternate color based on the next visible cell above
                    // this one. If the cell above is eclipsed by a multi-row cell,
                    // then use the cell eclipsing that one.
                    const auto& cellAbove = m_table[row - 1][col];
                    const auto cellAboveParent = GetEclipsingRowWiseCell(row - 1, col);
                    const wxColour colorAbove = cellAboveParent.has_value() ?
                                                    cellAboveParent.value().m_bgColor :
                                                    cellAbove.m_bgColor;
                    currentRow[col].m_bgColor =
                        (colorAbove == baseColor ? alternateColor : baseColor);
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::SetRowBackgroundColor(
        const size_t row, const wxColour& color,
        const std::optional<std::set<size_t>>& columnStops /*= std::nullopt*/)
        {
        if (row < GetRowCount() && m_table[row].size() > 0)
            {
            auto& currentRow = m_table[row];
            for (size_t i = 0; i < currentRow.size(); ++i)
                {
                if (columnStops.has_value() && columnStops.value().contains(i))
                    {
                    continue;
                    }
                currentRow[i].m_bgColor = color;
                }
            }
        }

    //----------------------------------------------------------------
    void Table::GroupRow(const size_t row)
        {
        if (row < GetRowCount())
            {
            auto& currentRow = m_table[row];
            if (currentRow.size() <= 1)
                {
                return;
                }
            for (size_t i = 0; i < currentRow.size() - 1; /*in loop*/)
                {
                size_t startingCounter = i;
                while (i < currentRow.size() - 1 && currentRow[i].IsText() &&
                       currentRow[i + 1].IsText() &&
                       currentRow[i].GetDisplayValue().CmpNoCase(
                           currentRow[i + 1].GetDisplayValue()) == 0)
                    {
                    ++i;
                    }
                if (i > startingCounter)
                    {
                    currentRow[startingCounter].m_columnCount = (i - startingCounter) + 1;
                    // clear the cells that are being eclipsed so that they don't
                    // affect column-width calculations
                    for (size_t clearCounter = startingCounter + 1;
                         clearCounter < (startingCounter + 1) + (i - startingCounter);
                         ++clearCounter)
                        {
                        currentRow[clearCounter].SetValue(wxString{});
                        }
                    ++i;
                    }
                else
                    {
                    ++i;
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::GroupColumn(const size_t column)
        {
        if (GetRowCount() > 1 && column < GetColumnCount())
            {
            for (size_t i = 1 + (m_hasGroupHeader ? 1 : 0) /* skip header*/; i < GetRowCount();
                 /*in loop*/)
                {
                const size_t startingCounter = i;
                while (i < GetRowCount() - 1 && m_table[i][column].IsText() &&
                       m_table[i + 1][column].IsText() &&
                       m_table[i][column].GetDisplayValue().CmpNoCase(
                           m_table[i + 1][column].GetDisplayValue()) == 0)
                    {
                    ++i;
                    }
                if (i > startingCounter)
                    {
                    m_table[startingCounter][column].m_rowCount = (i - startingCounter) + 1;
                    // clear the cells that are being eclipsed so that they don't
                    // affect row-width calculations
                    for (size_t clearCounter = startingCounter + 1;
                         clearCounter < (startingCounter + 1) + (i - startingCounter);
                         ++clearCounter)
                        {
                        m_table[clearCounter][column].SetValue(wxString{});
                        m_table[clearCounter][column].m_rowCount = 0;
                        }
                    ++i;
                    }
                else
                    {
                    ++i;
                    }
                }
            }
        }

    //----------------------------------------------------------------
    std::vector<Table::CellPosition> Table::GetTopN(const size_t column, const size_t N /*= 1*/)
        {
        std::vector<CellPosition> topNPositions;

        if (N == 0)
            {
            return topNPositions;
            }

        if (column < GetColumnCount())
            {
            std::set<double, std::greater<double>> values;
            for (size_t row = 0; row < GetRowCount(); ++row)
                {
                // skip over aggregate rows (i.e., subtotals)
                if (m_aggregateRows.contains(row))
                    {
                    continue;
                    }
                if (const auto val = GetCell(row, column).GetDoubleValue(); !std::isnan(val))
                    {
                    values.insert(val);
                    }
                }

            std::vector<double> maxValues;
            std::copy_n(values.cbegin(), std::min(values.size(), N), std::back_inserter(maxValues));
            if (maxValues.empty())
                {
                return topNPositions;
                }
            const auto maxValBaseline = *std::ranges::min_element(std::as_const(maxValues));

            // get the positions of cells less than or equal to the top N values
            for (size_t row = 0; row < GetRowCount(); ++row)
                {
                // skip over aggregate rows (i.e., subtotals)
                if (m_aggregateRows.contains(row))
                    {
                    continue;
                    }
                if (const auto val = GetCell(row, column).GetDoubleValue();
                    !std::isnan(val) && compare_doubles_greater_or_equal(val, maxValBaseline))
                    {
                    topNPositions.push_back({ row, column });
                    }
                }
            }
        return topNPositions;
        }

    //----------------------------------------------------------------
    std::vector<Table::CellPosition> Table::GetOutliers(const size_t column,
                                                        const double outlierThreshold /*= 3.0*/)
        {
        std::vector<CellPosition> outlierPositions;
        if (column < GetColumnCount())
            {
            std::vector<double> values;
            values.reserve(GetRowCount());
            for (size_t row = 0; row < GetRowCount(); ++row)
                {
                // skip over aggregate rows (i.e., subtotals)
                if (m_aggregateRows.contains(row))
                    {
                    continue;
                    }
                if (auto val = GetCell(row, column).GetDoubleValue(); !std::isnan(val))
                    {
                    values.push_back(std::move(val));
                    }
                }
            const auto meanVal = statistics::mean(values);
            const auto sdVal = statistics::standard_deviation(values, true);
            // get the z-scores and see who is an outlier
            for (size_t row = 0; row < GetRowCount(); ++row)
                {
                // skip over aggregate rows (i.e., subtotals)
                if (m_aggregateRows.contains(row))
                    {
                    continue;
                    }
                if (const auto val = GetCell(row, column).GetDoubleValue(); !std::isnan(val))
                    {
                    if (const auto zScore = statistics::z_score(val, meanVal, sdVal);
                        std::abs(zScore) > outlierThreshold)
                        {
                        outlierPositions.push_back({ row, column });
                        }
                    }
                }
            }
        return outlierPositions;
        }

    //----------------------------------------------------------------
    void Table::CalcMainTableSize(std::vector<wxCoord>& columnWidths,
                                  std::vector<wxCoord>& rowHeights, Label& measuringLabel,
                                  wxDC& dc) const
        {
        columnWidths.clear();
        columnWidths.resize(GetColumnCount());
        rowHeights.clear();
        rowHeights.resize(GetRowCount());

        size_t currentRow{ 0 };
        for (const auto& row : m_table)
            {
            size_t currentColumn{ 0 };
            for (const auto& cell : row)
                {
                // make empty cells at least a space so that an empty
                // row or column will at least have some width or height
                const auto cellText = cell.GetDisplayValue();
                measuringLabel.SetText((cellText.length() ? cellText : wxString(L" ")) +
                                       cell.GetPrefix());

                AdjustTextLabelToCell(cell, measuringLabel);
                auto bBox = measuringLabel.GetBoundingBox(dc);
                // prefix will need 5 DIPs added to each side
                if (cell.GetPrefix().length())
                    {
                    bBox.Inflate(wxSize(ScaleToScreenAndCanvas(10), 0));
                    }
                // if cell consumes multiple rows, then divide its height across them
                // and set the cells in the rows beneath to the remaining height
                rowHeights[currentRow] = std::max(
                    safe_divide<int>(bBox.GetHeight(), cell.m_rowCount), rowHeights[currentRow]);
                if (cell.m_rowCount > 1)
                    {
                    auto remainingRows = cell.m_rowCount - 1;
                    auto nextRow = currentRow + 1;
                    while (remainingRows > 0 && nextRow < GetRowCount())
                        {
                        rowHeights[nextRow] =
                            std::max(safe_divide<int>(bBox.GetHeight(), cell.m_rowCount),
                                     rowHeights[nextRow]);
                        ++nextRow;
                        --remainingRows;
                        }
                    }
                // if cell consumes multiple columns, then divide its width across them
                // and set the proceeding columns to the remaining width
                columnWidths[currentColumn] =
                    std::max(safe_divide<int>(bBox.GetWidth(), cell.m_columnCount),
                             columnWidths[currentColumn]);
                if (cell.m_columnCount > 1)
                    {
                    auto remainingColumns = cell.m_columnCount - 1;
                    auto nextColumn = currentColumn + 1;
                    while (remainingColumns > 0 && nextColumn < row.size())
                        {
                        columnWidths[nextColumn] =
                            std::max(safe_divide<int>(bBox.GetWidth(), cell.m_columnCount),
                                     columnWidths[nextColumn]);
                        ++nextColumn;
                        --remainingColumns;
                        }
                    }
                ++currentColumn;
                }
            ++currentRow;
            }
        }

    //----------------------------------------------------------------
    wxSize Table::CalculateTableSize(std::vector<wxCoord>& columnWidths,
                                     std::vector<wxCoord>& rowHeights, wxRect& drawArea,
                                     wxDC& dc) const
        {
        Label measuringLabel(GraphItemInfo()
                                 .Pen(wxPen{})
                                 .Padding(5, 5, 5, 5)
                                 .Scaling(GetScaling())
                                 .DPIScaling(GetDPIScaleFactor()));

        // if there are annotations, add gutters for them
        wxCoord widestLeftNote{ 0 }, widestRightNote{ 0 };
        for (const auto& note : m_cellAnnotations)
            {
            measuringLabel.SetText(note.m_note);
            if (DeduceGutterSide(note) == Side::Left)
                {
                widestLeftNote =
                    std::max(widestLeftNote, measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            else
                {
                widestRightNote =
                    std::max(widestRightNote, measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            }

        // if centering table, add extra spacing to keep labels fitting
        const auto extraSpaceForCentering =
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered) ?
                std::abs(widestLeftNote - widestRightNote) :
                0;
        // space for connection lines to notes
        widestLeftNote += (widestLeftNote > 0) ?
                              (ScaleToScreenAndCanvas(m_connectionOverhangWidth) * 2) +
                                  ScaleToScreenAndCanvas(m_labelSpacingFromLine) :
                              0;
        widestRightNote += (widestRightNote > 0) ?
                               (ScaleToScreenAndCanvas(m_connectionOverhangWidth) * 2) +
                                   ScaleToScreenAndCanvas(m_labelSpacingFromLine) :
                               0;

        CalcMainTableSize(columnWidths, rowHeights, measuringLabel, dc);

        auto tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
        auto tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);

        // adjust if row heights collectively go outside the drawing area
        if (tableHeight > drawArea.GetHeight())
            {
            const auto heightDiffProportion =
                safe_divide<double>(drawArea.GetHeight(), tableHeight);
            // take away a proportional amount of the difference from each row and column
            for (auto& row : rowHeights)
                {
                row *= heightDiffProportion;
                }
            for (auto& col : columnWidths)
                {
                col *= heightDiffProportion;
                }
            // re-tally everything
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            }

        // adjust if column widths collectively go outside the drawing area
        if (tableWidth + (widestLeftNote + widestRightNote + extraSpaceForCentering) >
            drawArea.GetWidth())
            {
            const auto widthDiffProportion = safe_divide<double>(
                drawArea.GetWidth(),
                tableWidth + (widestLeftNote + widestRightNote + extraSpaceForCentering));
            // take away a proportional amount of the difference from each row and column
            for (auto& row : rowHeights)
                {
                row *= widthDiffProportion;
                }
            for (auto& col : columnWidths)
                {
                col *= widthDiffProportion;
                }
            // re-tally everything
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            }

        // if requesting minimum width, then stretch it out if needed
        // (note that row heights are preserved)
        if (GetMinWidthProportion().has_value() &&
            tableWidth < (drawArea.GetWidth() * GetMinWidthProportion().value()))
            {
            const auto widthIncreaseProportion =
                safe_divide<double>(drawArea.GetWidth(), tableWidth);
            for (auto& col : columnWidths)
                {
                col *= widthIncreaseProportion;
                }
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            // may be off by a pixel or so from rounding, so fix that
            const auto roundingDiff = drawArea.GetWidth() - tableWidth;
            columnWidths.back() += roundingDiff;
            tableWidth += roundingDiff;
            }

        // if requesting minimum height, then stretch it out if needed
        // (note that column widths are preserved)
        if (GetMinHeightProportion().has_value() &&
            tableHeight < (drawArea.GetHeight() * GetMinHeightProportion().value()))
            {
            const auto heightIncreaseProportion =
                safe_divide<double>(drawArea.GetHeight(), tableHeight);
            for (auto& row : rowHeights)
                {
                row *= heightIncreaseProportion;
                }
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            // may be off by a pixel or so from rounding, so fix that
            const auto roundingDiff = drawArea.GetHeight() - tableHeight;
            rowHeights.back() += roundingDiff;
            tableHeight += roundingDiff;
            }

        // adjust the drawing area width, but only if it is smaller than it was before
        drawArea.SetSize(
            wxSize(std::min(drawArea.GetWidth(),
                            tableWidth + widestLeftNote + widestRightNote + extraSpaceForCentering),
                   tableHeight));

        return wxSize(tableWidth, tableHeight);
        }

    //----------------------------------------------------------------
    void Table::RecalcSizes(wxDC& dc)
        {
        if (GetRowCount() == 0 || GetColumnCount() == 0)
            {
            return;
            }

        m_cachedCellRects.clear();

        Graph2D::RecalcSizes(dc);

        // inflates a rect representing the drawing area if
        // padding was added to the original drawing area
        const auto AddPaddingToRect = [&, this](const wxRect& rect)
        {
            wxRect adjustedRect = rect;
            if (!GetMinWidthProportion().has_value() && !GetMinHeightProportion().has_value())
                {
                adjustedRect.Inflate(ScaleToScreenAndCanvas(5));
                }
            return adjustedRect;
        };

        const wxRect originalFullGraphArea = GetBoundingBox(dc);
        wxRect fullGraphArea = originalFullGraphArea;
        wxRect drawArea = GetPlotAreaBoundingBox();
        const auto graphDecorationHeight = fullGraphArea.GetHeight() - drawArea.GetHeight();
        const auto graphDecorationWidth = fullGraphArea.GetWidth() - drawArea.GetWidth();

        // calculate the necessary heights of the rows and widths of the columns
        std::vector<wxCoord> columnWidths;
        std::vector<wxCoord> rowHeights;

        const auto tableSize = CalculateTableSize(columnWidths, rowHeights, drawArea, dc);
        const auto tableWidth = tableSize.GetWidth();
        const auto tableHeight = tableSize.GetHeight();

        // if the needed area for the table proper is less than the available drawing area,
        // then remove that extra space and recompute the layout
        fullGraphArea.SetHeight(
            std::min(AddPaddingToRect(drawArea).GetHeight() + graphDecorationHeight,
                     fullGraphArea.GetHeight()));
        fullGraphArea.SetWidth(
            std::min(AddPaddingToRect(drawArea).GetWidth() + graphDecorationWidth,
                     fullGraphArea.GetWidth()));
        SetBoundingBox(fullGraphArea, dc, GetScaling());
        Graph2D::RecalcSizes(dc);

        // offset the table later if being page aligned within its parent drawing area
        const wxCoord horizontalAlignmentOffset =
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned) ?
                ((originalFullGraphArea.GetWidth() - tableWidth) -
                 (drawArea.x - originalFullGraphArea.x)) :
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered) ?
                safe_divide((originalFullGraphArea.GetWidth() - tableWidth) -
                                (drawArea.x - originalFullGraphArea.x),
                            2) :
                0;

        const wxCoord verticalAlignmentOffset =
            (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned) ?
                (originalFullGraphArea.GetHeight() - tableHeight) :
            (GetPageVerticalAlignment() == PageVerticalAlignment::Centered) ?
                safe_divide(originalFullGraphArea.GetHeight() - tableHeight, 2) :
                0;

        // measure the text
        std::array<wxPoint, 4> pts{};
        std::vector<std::unique_ptr<Label>> cellLabels;
        double smallestTextScaling{ std::numeric_limits<double>::max() };
        size_t currentRow{ 0 }, currentColumn{ 0 };
        wxCoord currentXPos{ drawArea.GetX() };
        wxCoord currentYPos{ drawArea.GetY() };
        int columnsToOverwrite{ 0 };
        std::set<std::pair<size_t, size_t>> rowCellsToSkip;
        m_cachedCellRects.resize(GetRowCount(), std::vector<wxRect>(GetColumnCount(), wxRect{}));
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& cell : row)
                {
                // skip over rows being eclipsed because of previous cells
                // being multi-row or multi-column
                if (columnsToOverwrite > 0 ||
                    rowCellsToSkip.contains(std::make_pair(currentRow, currentColumn)))
                    {
                    --columnsToOverwrite;
                    currentXPos += columnWidths[currentColumn];
                    ++currentColumn;
                    continue;
                    }
                columnsToOverwrite = cell.m_columnCount - 1;
                // build a list of cells in the proceeding rows that should be skipped
                // in the next loop if this one is multi-row
                if (cell.m_rowCount > 1)
                    {
                    auto rowsToSkip = cell.m_rowCount - 1;
                    auto nextRow = currentRow + 1;
                    while (rowsToSkip > 0)
                        {
                        rowCellsToSkip.insert(std::make_pair(nextRow++, currentColumn));
                        --rowsToSkip;
                        }
                    }
                // get the current column's width, factoring in whether it is multi-column
                wxCoord currentColumnWidth = columnWidths[currentColumn];
                if (cell.m_columnCount > 1)
                    {
                    auto remainingColumns = cell.m_columnCount - 1;
                    auto nextColumn = currentColumn + 1;
                    while (remainingColumns && nextColumn < columnWidths.size())
                        {
                        currentColumnWidth += columnWidths[nextColumn++];
                        --remainingColumns;
                        }
                    }
                // do the same for the height if it is multi-row
                wxCoord currentColumnHeight = rowHeights[currentRow];
                if (cell.m_rowCount > 1)
                    {
                    auto remainingRows = cell.m_rowCount - 1;
                    auto nextRow = currentRow + 1;
                    while (remainingRows && nextRow < rowHeights.size())
                        {
                        currentColumnHeight += rowHeights[nextRow++];
                        --remainingRows;
                        }
                    }
                // top left corner, going clockwise
                pts[0] = wxPoint{ currentXPos, currentYPos };
                pts[1] = wxPoint{ (currentXPos + currentColumnWidth), currentYPos };
                pts[2] = wxPoint{ (currentXPos + currentColumnWidth),
                                  (currentYPos + currentColumnHeight) };
                pts[3] = wxPoint{ currentXPos, (currentYPos + currentColumnHeight) };

                wxRect boxRect(pts[0], pts[2]);
                // if box is going outside the table by a pixel or so, then trim that off
                if (boxRect.GetRight() > drawArea.GetRight())
                    {
                    const auto rightEdgeOverhang = boxRect.GetRight() - drawArea.GetRight();
                    boxRect.SetWidth(boxRect.GetWidth() - rightEdgeOverhang);
                    }

                // If prefix is user-supplied (not something we are controlling), not being color
                // coded, and cell is left aligned, then we can just add the prefix to the cell's
                // label. Otherwise, we need to make it as a separate label and place that on the
                // left side.
                const bool isPrefixSeparateLabel =
                    (cell.m_horizontalCellAlignment == PageHorizontalAlignment::RightAligned ||
                     cell.m_horizontalCellAlignment == PageHorizontalAlignment::Centered ||
                     cell.m_colorCodePrefix ||
                     cell.m_valueFormat == TableCellFormat::PercentChange ||
                     cell.m_valueFormat == TableCellFormat::GeneralChange ||
                     cell.m_valueFormat == TableCellFormat::Accounting);

                const auto cellText = cell.GetDisplayValue();
                auto cellLabel = std::make_unique<Label>(
                    GraphItemInfo((isPrefixSeparateLabel ? wxString{} : cell.GetPrefix()) +
                                  (cellText.length() ? cellText : wxString(L" ")))
                        .Pen(wxNullPen)
                        .Padding(5, 5, 5, 5)
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Font(cell.m_font)
                        .FontColor(((cell.m_bgColor.IsOk() && !cell.m_bgColor.IsTransparent()) ?
                                        ColorContrast::BlackOrWhiteContrast(cell.m_bgColor) :
                                        Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                            GetPlotOrCanvasColor())))
                        .FontBackgroundColor(cell.m_bgColor.IsOk() ? cell.m_bgColor : *wxWHITE)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .AnchorPoint(boxRect.GetTopLeft()));

                // cache it for annotations and highlights
                m_cachedCellRects[currentRow][currentColumn] = boxRect;

                AdjustTextLabelToCell(cell, *cellLabel);
                cellLabel->SetLineSpacing(0);

                cellLabel->SetBoundingBox(boxRect, dc, GetScaling());
                smallestTextScaling = std::min(cellLabel->GetScaling(), smallestTextScaling);

                cellLabels.push_back(
                    std::move(cellLabel)); // need to homogenize scaling of text later

                // special character at the far-left edge (e.g., '$' in accounting formatting)
                if (cell.GetPrefix().length() && isPrefixSeparateLabel)
                    {
                    const wxString prefix =
                        ((cell.m_valueFormat == TableCellFormat::PercentChange ||
                          cell.m_valueFormat == TableCellFormat::GeneralChange)) ?
                            // down and up arrow emojis
                            wxString((std::isnan(cell.GetDoubleValue()) ? L" " :
                                      (cell.GetDoubleValue() == 0)      ? L"\u2014" :
                                      cell.GetDoubleValue() < 0         ? L"\u25BC" :
                                                                          L"\u25B2")) :
                            cell.GetPrefix();
                    const auto cellBkColor{ cell.m_bgColor.IsOk() ? cell.m_bgColor : *wxWHITE };
                    auto cellPrefixLabel = std::make_unique<Label>(
                        GraphItemInfo(prefix)
                            .Pen(wxNullPen)
                            .Padding(5, 5, 5, 5)
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Font(cell.m_font)
                            .FontColor(((cell.m_bgColor.IsOk() && !cell.m_bgColor.IsTransparent()) ?
                                            ColorContrast::BlackOrWhiteContrast(cell.m_bgColor) :
                                            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                                GetPlotOrCanvasColor())))
                            .FontBackgroundColor(wxTransparentColour)
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(boxRect.GetLeftTop()));
                    if (cell.m_colorCodePrefix)
                        {
                        cellPrefixLabel->SetFontColor(
                            (cell.GetDoubleValue() < 0) ?
                                (ColorContrast::IsDark(cellBkColor) ?
                                     ColorBrewer::GetColor(Colors::Color::RosePink) :
                                     ColorBrewer::GetColor(Colors::Color::Red)) :
                                (ColorContrast::IsDark(cellBkColor) ?
                                     ColorBrewer::GetColor(Colors::Color::MintGreen) :
                                     ColorBrewer::GetColor(Colors::Color::HunterGreen)));
                        }
                    cellPrefixLabel->SetBoundingBox(boxRect, dc, GetScaling());
                    cellPrefixLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                    cellPrefixLabel->SetPageHorizontalAlignment(
                        PageHorizontalAlignment::LeftAligned);
                    cellLabels.push_back(std::move(cellPrefixLabel));
                    }

                currentXPos += columnWidths[currentColumn];
                ++currentColumn;
                }
            currentYPos += rowHeights[currentRow];
            ++currentRow;
            }

        // homogenize cells' text scaling to the smallest size and add them
        for (auto& cellLabel : cellLabels)
            {
            const wxRect bBox = cellLabel->GetBoundingBox(dc);
            cellLabel->SetScaling(smallestTextScaling);
            // Changing the scaling to something smaller will scale down the bounding box,
            // so reset that back to the original box while preserving the new scaling.
            // In this case, the text will be shrunk to the same scale as the other cells,
            // but its box size (including the background color-filled area) will remain
            // the same.
            cellLabel->LockBoundingBoxScaling();
            cellLabel->SetBoundingBox(bBox, dc, GetScaling());
            cellLabel->UnlockBoundingBoxScaling();
            // if using page alignment other than left aligned, then adjust its position
            if (horizontalAlignmentOffset > 0 || verticalAlignmentOffset > 0)
                {
                cellLabel->Offset(horizontalAlignmentOffset, verticalAlignmentOffset);
                }
            AddObject(std::move(cellLabel));
            }

        // draw the row borders
        currentRow = currentColumn = 0;
        rowCellsToSkip.clear();
        for (size_t rowHeightCounter = 0; rowHeightCounter < rowHeights.size(); ++rowHeightCounter)
            {
            currentColumn = 0;
            for (size_t colWidthCounter = 0; colWidthCounter < columnWidths.size();
                 ++colWidthCounter)
                {
                // build a list of cells in the following rows that should be skipped
                // in the next loop if this one is multi-row
                if (const auto& cell = GetCell(currentRow, currentColumn); cell.m_rowCount > 1)
                    {
                    auto rowsToSkip = cell.m_rowCount - 1;
                    auto nextRow = currentRow + 1;
                    while (rowsToSkip > 0)
                        {
                        rowCellsToSkip.insert(std::make_pair(nextRow++, currentColumn));
                        --rowsToSkip;
                        }
                    }
                ++currentColumn;
                }
            ++currentRow;
            }

        auto highlightedBorderLines = std::make_unique<Lines>(GetHighlightPen(), GetScaling());
        auto borderLines = std::make_unique<Lines>(GetPen(), GetScaling());
        borderLines->GetPen().SetCap(wxPenCap::wxCAP_BUTT);
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        columnsToOverwrite = 0;
        if (m_clearTrailingRowFormatting && m_dataSize.first < rowHeights.size())
            {
            rowHeights.erase(rowHeights.begin() + m_dataSize.first, rowHeights.end());
            }
        for (const auto& rowHeight : rowHeights)
            {
            bool isPreviousColumnHighlighted{ false };
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& colWidth : columnWidths)
                {
                const auto& cell = GetCell(currentRow, currentColumn);
                auto parentColumnCell = GetEclipsingColumnWiseCell(currentRow, currentColumn);
                // see if the above cell (or a cell above that which is eclipsing it)
                // is highlighted
                auto aboveCellHighlighted =
                    (currentRow > 0) ? (GetCell(currentRow - 1, currentColumn).IsHighlighted() &&
                                        GetCell(currentRow - 1, currentColumn).m_showBottomBorder) :
                                       false;
                if (currentRow > 0 && !aboveCellHighlighted)
                    {
                    if (auto aboveCellsParent =
                            GetEclipsingRowWiseCell(currentRow - 1, currentColumn);
                        aboveCellsParent.has_value())
                        {
                        aboveCellHighlighted = aboveCellsParent.value().IsHighlighted();
                        }
                    }
                // draw the horizontal line above the cell.
                // skip over cells being eclipsed by the previous one above that was multi-row
                if (!rowCellsToSkip.contains(std::make_pair(currentRow, currentColumn)) &&
                    // if top row and top border is turned off, then don't draw the line
                    !(currentRow == 0 && !cell.m_showTopBorder) &&
                    // if top border is not to be drawn and cell above's bottom border is also
                    // not drawn, then don't draw the line
                    !(currentRow > 0 && !cell.m_showTopBorder &&
                      !GetCell(currentRow - 1, currentColumn).m_showBottomBorder))
                    {
                    if ((cell.IsHighlighted() && cell.m_showTopBorder) || aboveCellHighlighted ||
                        (parentColumnCell.has_value() && parentColumnCell.value().IsHighlighted()))
                        {
                        highlightedBorderLines->AddLine(
                            wxPoint(currentXPos, currentYPos),
                            wxPoint(currentXPos + colWidth, currentYPos));
                        }
                    else
                        {
                        borderLines->AddLine(wxPoint(currentXPos, currentYPos),
                                             wxPoint(currentXPos + colWidth, currentYPos));
                        }
                    }
                // skip over cells being eclipsed by the previous one since it's multi-column
                if (columnsToOverwrite > 0)
                    {
                    --columnsToOverwrite;
                    currentXPos += colWidth;
                    ++currentColumn;
                    continue;
                    }
                columnsToOverwrite = cell.m_columnCount - 1;
                // draw vertical line to the left of the cell
                if (!(currentColumn == 0 && !cell.m_showLeftBorder) &&
                    !(currentColumn > 0 && !cell.m_showLeftBorder &&
                      !GetCell(currentRow, currentColumn - 1).m_showRightBorder))
                    {
                    auto parentCell = GetEclipsingRowWiseCell(currentRow, currentColumn);
                    if (cell.IsHighlighted())
                        {
                        if (cell.m_showLeftBorder || isPreviousColumnHighlighted)
                            {
                            highlightedBorderLines->AddLine(
                                wxPoint(currentXPos, currentYPos),
                                wxPoint(currentXPos, currentYPos + rowHeight));
                            }
                        isPreviousColumnHighlighted = cell.m_showRightBorder;
                        }
                    else if (parentCell.has_value() && parentCell.value().IsHighlighted() &&
                             parentCell.value().m_showRightBorder)
                        {
                        highlightedBorderLines->AddLine(
                            wxPoint{ currentXPos, currentYPos },
                            wxPoint{ currentXPos, currentYPos + rowHeight });
                        isPreviousColumnHighlighted = true;
                        }
                    else if (isPreviousColumnHighlighted)
                        {
                        highlightedBorderLines->AddLine(
                            wxPoint{ currentXPos, currentYPos },
                            wxPoint{ currentXPos, currentYPos + rowHeight });
                        isPreviousColumnHighlighted = false;
                        }
                    else
                        {
                        borderLines->AddLine(wxPoint{ currentXPos, currentYPos },
                                             wxPoint{ currentXPos, currentYPos + rowHeight });
                        }
                    }
                currentXPos += colWidth;
                ++currentColumn;
                }
            currentYPos += rowHeight;
            ++currentRow;
            }
        // outer-right border
        currentYPos = drawArea.GetY();
        currentRow = currentColumn = 0;
        for (const auto& rowHeight : rowHeights)
            {
            const auto& cell = GetCell(currentRow, GetColumnCount() - 1);
            const auto& parentRowCell = GetEclipsingRowWiseCell(currentRow, GetColumnCount() - 1);
            const auto& parentColumnCell =
                GetEclipsingColumnWiseCell(currentRow, GetColumnCount() - 1);
            if (cell.m_showRightBorder)
                {
                if (cell.IsHighlighted() ||
                    (parentRowCell.has_value() && parentRowCell.value().IsHighlighted()) ||
                    (parentColumnCell.has_value() && parentColumnCell.value().IsHighlighted()))
                    {
                    highlightedBorderLines->AddLine(
                        wxPoint(drawArea.GetX() + tableWidth, currentYPos),
                        wxPoint(drawArea.GetX() + tableWidth, currentYPos + rowHeight));
                    }
                else
                    {
                    borderLines->AddLine(
                        wxPoint(drawArea.GetX() + tableWidth, currentYPos),
                        wxPoint(drawArea.GetX() + tableWidth, currentYPos + rowHeight));
                    }
                }
            currentYPos += rowHeight;
            ++currentRow;
            }
        // outer-bottom border
        currentXPos = drawArea.GetX();
        for (const auto& colWidth : columnWidths)
            {
            const auto& cell = GetCell(GetRowCount() - 1, currentColumn);
            const auto& parentColumnCell =
                GetEclipsingColumnWiseCell(GetRowCount() - 1, currentColumn);
            if (cell.m_showBottomBorder)
                {
                if (cell.IsHighlighted() ||
                    (parentColumnCell.has_value() && parentColumnCell.value().IsHighlighted()))
                    {
                    highlightedBorderLines->AddLine(
                        wxPoint(currentXPos, drawArea.GetY() + tableHeight),
                        wxPoint(currentXPos + colWidth, drawArea.GetY() + tableHeight));
                    }
                else
                    {
                    borderLines->AddLine(
                        wxPoint(currentXPos, drawArea.GetY() + tableHeight),
                        wxPoint(currentXPos + colWidth, drawArea.GetY() + tableHeight));
                    }
                }
            currentXPos += colWidth;
            ++currentColumn;
            }

        // if using page alignment other than left aligned, then adjust its position
        if (horizontalAlignmentOffset > 0 || verticalAlignmentOffset > 0)
            {
            borderLines->Offset(horizontalAlignmentOffset, verticalAlignmentOffset);
            highlightedBorderLines->Offset(horizontalAlignmentOffset, verticalAlignmentOffset);
            for (auto& row : m_cachedCellRects)
                {
                for (auto& cell : row)
                    {
                    cell.Offset(wxPoint(horizontalAlignmentOffset, verticalAlignmentOffset));
                    }
                }
            }

        AddObject(std::move(borderLines));
        AddObject(std::move(highlightedBorderLines));

        // add gutter messages
        const auto rightGutter =
            wxRect(wxPoint(drawArea.GetX() + horizontalAlignmentOffset + tableWidth,
                           drawArea.GetY() + verticalAlignmentOffset),
                   wxSize(drawArea.GetWidth() - (horizontalAlignmentOffset + tableWidth),
                          drawArea.GetHeight()));
        const auto leftGutter =
            wxRect(wxPoint(drawArea.GetX(), drawArea.GetY() + verticalAlignmentOffset),
                   wxSize(horizontalAlignmentOffset, drawArea.GetHeight()));
        const auto connectionOverhangWidth = ScaleToScreenAndCanvas(m_connectionOverhangWidth);
        const auto labelSpacingFromLine = ScaleToScreenAndCanvas(m_labelSpacingFromLine);
        for (auto& note : m_cellAnnotations)
            {
            // sort by rows, top-to-bottom
            std::ranges::sort(note.m_cells, [](const auto& lv, const auto& rv) noexcept
                              { return lv.m_row < rv.m_row; });
            auto noteConnectionLines = std::make_unique<Lines>(
                note.m_connectionLinePen.has_value() ? note.m_connectionLinePen.value() :
                                                       GetHighlightPen(),
                GetScaling());
            noteConnectionLines->GetPen().SetCap(wxPenCap::wxCAP_BUTT);
            wxCoord lowestY{ drawArea.GetBottom() }, highestY{ drawArea.GetTop() };
            if (auto gutterSide = DeduceGutterSide(note); gutterSide == Side::Right)
                {
                // draw lines from the middle of the cells to a little bit outside the
                // table going into the right gutter
                for (const auto& cell : note.m_cells)
                    {
                    const auto cellRect = GetCachedCellRect(cell.m_row, cell.m_column);
                    const auto middleOfCellY = cellRect.GetY() + cellRect.GetHeight() / 2;
                    lowestY = std::min(lowestY, middleOfCellY);
                    highestY = std::max(highestY, middleOfCellY);
                    noteConnectionLines->AddLine(
                        wxPoint(cellRect.GetX() + cellRect.GetWidth(), middleOfCellY),
                        wxPoint(rightGutter.GetX() + connectionOverhangWidth, middleOfCellY));
                    }
                // connect the protruding nubs with a vertical line
                noteConnectionLines->AddLine(
                    wxPoint(rightGutter.GetX() + connectionOverhangWidth, lowestY),
                    wxPoint(rightGutter.GetX() + connectionOverhangWidth, highestY));
                const auto cellsYMiddle = ((highestY - lowestY) / 2) + lowestY;
                noteConnectionLines->AddLine(
                    wxPoint(rightGutter.GetX() + connectionOverhangWidth, cellsYMiddle),
                    wxPoint(rightGutter.GetX() + connectionOverhangWidth * 2, cellsYMiddle));
                AddObject(std::move(noteConnectionLines));

                // add the note into the gutter
                auto noteLabel = std::make_unique<Label>(
                    GraphItemInfo(note.m_note)
                        .Pen(wxNullPen)
                        .FontColor(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                            GetPlotOrCanvasColor()))
                        // use same text scale as the table (or 1.0 if the table font is tiny)
                        .Scaling(std::max(1.0, smallestTextScaling))
                        .DPIScaling(GetDPIScaleFactor())
                        .Anchoring(Anchoring::BottomLeftCorner)
                        .AnchorPoint(wxPoint(rightGutter.GetX() + (connectionOverhangWidth * 2) +
                                                 labelSpacingFromLine,
                                             cellsYMiddle)));
                // if label is too long to fit, then split it length-wise to fit in the gutter
                const auto bBox = noteLabel->GetBoundingBox(dc);
                auto rightGutterTextArea{ rightGutter };
                rightGutterTextArea.SetLeft(rightGutterTextArea.GetLeft() +
                                            (connectionOverhangWidth * 2) + labelSpacingFromLine);
                rightGutterTextArea.SetWidth(rightGutterTextArea.GetWidth() -
                                             (connectionOverhangWidth * 2) - labelSpacingFromLine);
                if (!Polygon::IsRectInsideRect(bBox, rightGutterTextArea))
                    {
                    noteLabel->SplitTextToFitBoundingBox(dc, rightGutterTextArea.GetSize());
                    const auto boundBox = noteLabel->GetBoundingBox(dc);
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, boundBox.GetHeight() / 2));
                    }
                else
                    {
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, bBox.GetHeight() / 2));
                    }
                AddObject(std::move(noteLabel));
                }
            else
                {
                // draw lines from the middle of the cells to a little bit outside the
                // table going into the left gutter
                for (const auto& cell : note.m_cells)
                    {
                    const auto cellRect = GetCachedCellRect(cell.m_row, cell.m_column);
                    const auto middleOfCellY = cellRect.GetY() + cellRect.GetHeight() / 2;
                    lowestY = std::min(lowestY, middleOfCellY);
                    highestY = std::max(highestY, middleOfCellY);
                    noteConnectionLines->AddLine(
                        wxPoint(cellRect.GetX(), middleOfCellY),
                        wxPoint(leftGutter.GetRight() - connectionOverhangWidth, middleOfCellY));
                    }
                // connect the protruding nubs with a vertical line
                noteConnectionLines->AddLine(
                    wxPoint(leftGutter.GetRight() - connectionOverhangWidth, lowestY),
                    wxPoint(leftGutter.GetRight() - connectionOverhangWidth, highestY));
                const auto cellsYMiddle = ((highestY - lowestY) / 2) + lowestY;
                noteConnectionLines->AddLine(
                    wxPoint(leftGutter.GetRight() - connectionOverhangWidth, cellsYMiddle),
                    wxPoint(leftGutter.GetRight() - connectionOverhangWidth * 2, cellsYMiddle));
                AddObject(std::move(noteConnectionLines));

                // add the note into the gutter
                auto noteLabel = std::make_unique<Label>(
                    GraphItemInfo(note.m_note)
                        .Pen(wxNullPen)
                        // use same text scale as the table
                        .Scaling(smallestTextScaling)
                        .DPIScaling(GetDPIScaleFactor())
                        .Anchoring(Anchoring::BottomRightCorner)
                        .AnchorPoint(wxPoint(leftGutter.GetRight() - (connectionOverhangWidth * 2) -
                                                 labelSpacingFromLine,
                                             cellsYMiddle)));

                // if label is too long to fit, then split it length-wise to fit in the gutter
                const auto bBox = noteLabel->GetBoundingBox(dc);
                auto leftGutterTextArea{ leftGutter };
                leftGutterTextArea.SetWidth(leftGutterTextArea.GetWidth() -
                                            (connectionOverhangWidth * 2) - labelSpacingFromLine);
                if (!Polygon::IsRectInsideRect(bBox, leftGutterTextArea))
                    {
                    noteLabel->SplitTextToFitBoundingBox(dc, leftGutterTextArea.GetSize());
                    const auto boundBox = noteLabel->GetBoundingBox(dc);
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, boundBox.GetHeight() / 2));
                    }
                else
                    {
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, bBox.GetHeight() / 2));
                    }
                AddObject(std::move(noteLabel));
                }
            // Base graph resets the scaling for caption to 1.0 and it's a
            // template object that's copied, so you need to adjust the font size
            // instead of the scaling.
            // By default, captions are 75% of the title's font size, so use that
            // and downscale that to what is being used for the cells' and annotations'
            // text, making a consistent look.
            GetCaption().GetFont().SetFractionalPointSize(
                (GetTitle().GetFont().GetFractionalPointSize() * math_constants::three_quarters) *
                std::max(1.0, smallestTextScaling));
            }
        }

    //----------------------------------------------------------------
    void Table::AdjustTextLabelToCell(const TableCell& cell, GraphItems::Label& cellLabel) const
        {
        cellLabel.SetScaling(GetScaling());
        if (cell.m_suggestedLineLength.has_value())
            {
            cellLabel.SplitTextToFitLength(cell.m_suggestedLineLength.value());
            }
        cellLabel.SetFont(cell.m_font);
        if (cell.m_leftImage.IsOk())
            {
            cellLabel.SetLeftImage(cell.m_leftImage);
            }

        cellLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        // if an overriding horizontal alignment is in use, then use that
        if (cell.m_horizontalCellAlignment.has_value())
            {
            cellLabel.SetPageHorizontalAlignment(cell.m_horizontalCellAlignment.value());
            }
        // otherwise, deduce the best alignment
        else
            {
            cellLabel.SetPageHorizontalAlignment(
                ((cell.IsNumeric() || cell.IsDate()) ? PageHorizontalAlignment::RightAligned :
                 cell.IsRatio()                      ? PageHorizontalAlignment::Centered :
                                  // if it's text, center it if multi-column; otherwise, left align
                     cell.m_columnCount > 1 ? PageHorizontalAlignment::Centered :
                                              PageHorizontalAlignment::LeftAligned));
            }
        // if centered in cell, then center the text also (if multi-line)
        if (cellLabel.GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered &&
            !cell.m_textAlignment.has_value())
            {
            cellLabel.SetTextAlignment(TextAlignment::Centered);
            }

        // user-defined text alignment
        if (cell.m_textAlignment.has_value())
            {
            cellLabel.SetTextAlignment(cell.m_textAlignment.value());
            }
        }

    //----------------------------------------------------------------
    Table::TableCell* Table::FindCell(const wxString& textToFind)
        {
        for (auto& row : m_table)
            {
            for (auto& cell : row)
                {
                if (cell.GetDisplayValue().CmpNoCase(textToFind) == 0)
                    {
                    return &cell;
                    }
                }
            }
        return nullptr;
        }

    //----------------------------------------------------------------
    std::optional<Table::CellPosition> Table::FindCellPosition(const wxString& textToFind) const
        {
        if (m_table.empty())
            {
            return std::nullopt;
            }

        const size_t colCount{ m_table[0].size() };
        for (size_t i = 0; i < m_table.size(); ++i)
            {
            for (size_t j = 0; j < colCount; ++j)
                {
                if (auto& cell = m_table[i][j]; cell.GetDisplayValue().CmpNoCase(textToFind) == 0)
                    {
                    return CellPosition{ i, j };
                    }
                }
            }
        return std::nullopt;
        }

    //----------------------------------------------------------------
    std::optional<size_t> Table::FindColumnIndex(const wxString& textToFind) const
        {
        if (m_table.empty())
            {
            return std::nullopt;
            }

        auto row{ m_table[0] };
        for (size_t i = 0; i < row.size(); ++i)
            {
            if (row[i].GetDisplayValue().CmpNoCase(textToFind) == 0)
                {
                return i;
                }
            }

        return std::nullopt;
        }

    //----------------------------------------------------------------
    std::optional<size_t> Table::FindRowIndex(const wxString& textToFind) const
        {
        if (GetColumnCount() > 0)
            {
            for (size_t i = 0; i < m_table.size(); ++i)
                {
                if (auto& cell = m_table[i][0]; cell.GetDisplayValue().CmpNoCase(textToFind) == 0)
                    {
                    return i;
                    }
                }
            }

        return std::nullopt;
        }

    //----------------------------------------------------------------
    void Table::AddFootnote(const wxString& cellValue, const wxString& footnote)
        {
        // clang-format off
        static const std::map<uint8_t, std::wstring> footnoteChars =
            {
            { 0, L"\u2070" },
            { 1, L"\u00B9" },
            { 2, L"\u00B2" },
            { 3, L"\u00B3" },
            { 4, L"\u2074" },
            { 5, L"\u2075" },
            { 6, L"\u2076" },
            { 7, L"\u2077" },
            { 8, L"\u2078" },
            { 9, L"\u2079" }
        };
        // clang-format on

        if (const auto ftChar = footnoteChars.find(m_footnotes.size() + 1);
            ftChar != footnoteChars.cend())
            {
            if (auto* foundCell = FindCell(cellValue))
                {
                // if changing format to text, then preserve its original alignment
                if (foundCell->IsDate() || foundCell->IsNumeric())
                    {
                    foundCell->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
                    }
                else if (foundCell->IsRatio())
                    {
                    foundCell->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                    }
                foundCell->SetValue(foundCell->GetDisplayValue() + ftChar->second);
                }
            else
                {
                return;
                }
            }
        else
            {
            return;
            }

        m_footnotes.push_back(footnote);

        // build the caption
        wxString footnoteCaption;
        for (size_t i = 0; i < m_footnotes.size(); ++i)
            {
            if (m_footnotes[i].length())
                {
                footnoteCaption += wxString::Format(L"%zu. %s\n", i + 1, m_footnotes[i]);
                }
            }
        footnoteCaption.Trim();
        GetCaption().SetText(footnoteCaption);
        }
    } // namespace Wisteria::Graphs
