#include "table.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    wxString Table::TableCell::GetDisplayValue() const
        {
        if (const auto strVal{ std::get_if<wxString>(&m_value) };
            strVal != nullptr)
            { return *strVal; }
        else if (const auto dVal{ std::get_if<double>(&m_value) };
                 dVal != nullptr)
            {
            if (std::isnan(*dVal))
                { return wxEmptyString; }
            else if (m_valueFormat == CellFormat::Percent)
                {
                return wxNumberFormatter::ToString((*dVal)*100, m_precision,
                    wxNumberFormatter::Style::Style_None) + L"%";
                }
            else if (m_valueFormat == CellFormat::Accounting)
                {
                return
                    ((*dVal < 0) ? L"(" : L"") +
                    wxNumberFormatter::ToString(*dVal, m_precision,
                        wxNumberFormatter::Style::Style_WithThousandsSep) +
                    ((*dVal < 0) ? L")" : L"");
                }
            else
                {
                return wxNumberFormatter::ToString(*dVal, m_precision,
                    wxNumberFormatter::Style::Style_WithThousandsSep);
                }
            }
        else if (const auto doubleVal{ std::get_if<std::pair<double, double>>(&m_value) };
                 doubleVal != nullptr)
            {
            if (std::isnan(doubleVal->first) || std::isnan(doubleVal->second))
                { return wxEmptyString; }
            if (doubleVal->first > doubleVal->second)
                {
                return wxString::Format(L"%s : 1",
                    wxNumberFormatter::ToString(
                        safe_divide(doubleVal->first, doubleVal->second), m_precision,
                        wxNumberFormatter::Style::Style_WithThousandsSep |
                        wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            else
                {
                return wxString::Format(L"1 : %s",
                    wxNumberFormatter::ToString(
                        safe_divide(doubleVal->second, doubleVal->first), m_precision,
                        wxNumberFormatter::Style::Style_WithThousandsSep |
                        wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            }
        else if (const auto doubleVal2{ std::get_if<wxDateTime>(&m_value) };
                 doubleVal2 != nullptr)
            {
            if (!doubleVal2->IsValid())
                { return wxEmptyString; }
            return doubleVal2->FormatDate();
            }
        else
            { return wxEmptyString; }
        }

    //----------------------------------------------------------------
    Table::TableCell& Table::GetCell(const size_t row, const size_t column)
        {
        wxASSERT_MSG(row < GetRowCount(),
            wxString::Format(L"Invalid row index (%zu)!", row));
        wxASSERT_MSG(column < m_table[row].size(),
            wxString::Format(L"Invalid column index (%zu)!", column));
        if (row >= GetRowCount() || column >= m_table[row].size())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Invalid cell index (row %zu, column %zu)."),
                                    row, column).ToUTF8());
            }
        return m_table[row][column];
        }

    //----------------------------------------------------------------
    std::optional<Table::TableCell> Table::GetParentRowWiseCell(const size_t row,
        const size_t column)
        {
        if (row > 0 && // first row can't have a row-wise parent
            row < GetRowCount() &&
            column < m_table[row].size())
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
                        { return parentCell; }
                    else
                        { return std::nullopt; }
                    }
                --parentRow;
                }
            }
        return std::nullopt;
        }

    //----------------------------------------------------------------
    std::optional<Table::TableCell> Table::GetParentColumnWiseCell(const size_t row,
        const size_t column)
        {
        if (column > 0 && // first column can't have a column-wise parent
            column < GetColumnCount() &&
            row < GetRowCount())
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
                        { return parentCell; }
                    else
                        { return std::nullopt; }
                    }
                --parentColumn;
                }
            }
        return std::nullopt;
        }

    //----------------------------------------------------------------
    wxRect Table::GetCachedCellRect(const size_t row, const size_t column)
        {
        wxASSERT_MSG(row < m_cachedCellRects.size(),
            wxString::Format(L"Invalid row index (%zu)!", row));
        wxASSERT_MSG(column < m_cachedCellRects[row].size(),
            wxString::Format(L"Invalid column index (%zu)!", column));
        if (row >= m_cachedCellRects.size() || column >= m_cachedCellRects[row].size())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Invalid cell index (row %zu, column %zu)."),
                                    row, column).ToUTF8());
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
        const std::vector<wxString>& columns,
        const bool transpose /*= false*/)
        {
        ClearTable();

        if (transpose)
            {
            SetTableSize(columns.size(), data->GetRowCount()+1);

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
                        GetCell(currentRow, i+1).m_value = continuousCol->GetValue(i);
                        }
                    }
                else if (auto catCol = data->GetCategoricalColumn(colName);
                    catCol != data->GetCategoricalColumns().cend())
                    {
                    for (size_t i = 0; i < catCol->GetValues().size(); ++i)
                        {
                        GetCell(currentRow, i+1).m_value  =
                            catCol->GetCategoryLabelFromID(catCol->GetValue(i));
                        }
                    }
                else if (auto dateCol = data->GetDateColumn(colName);
                    dateCol != data->GetDateColumns().cend())
                    {
                    for (size_t i = 0; i < dateCol->GetValues().size(); ++i)
                        {
                        GetCell(currentRow, i+1).m_value = dateCol->GetValue(i);
                        }
                    }
                else
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"'%s': column not found for table."), colName).ToUTF8());
                    }
                ++currentRow;
                }
            }
        else
            {
            SetTableSize(data->GetRowCount()+1, columns.size());

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
                        GetCell(i+1, currentColumn).m_value = continuousCol->GetValue(i);
                        }
                    }
                else if (auto catCol = data->GetCategoricalColumn(colName);
                    catCol != data->GetCategoricalColumns().cend())
                    {
                    for (size_t i = 0; i < catCol->GetValues().size(); ++i)
                        {
                        GetCell(i+1, currentColumn).m_value =
                            catCol->GetCategoryLabelFromID(catCol->GetValue(i));
                        }
                    }
                else if (auto dateCol = data->GetDateColumn(colName);
                    dateCol != data->GetDateColumns().cend())
                    {
                    for (size_t i = 0; i < dateCol->GetValues().size(); ++i)
                        {
                        GetCell(i+1, currentColumn).m_value = dateCol->GetValue(i);
                        }
                    }
                else
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"'%s': column not found for table."), colName).ToUTF8());
                    }
                ++currentColumn;
                }
            }
        }

    //----------------------------------------------------------------
    void Table::CalculateAggregate(const AggregateInfo& aggInfo,
                                   Table::TableCell& aggCell,
                                   const std::vector<double>& values)
        {
        if (values.size())
            {
            if (aggInfo.m_type == AggregateType::Total)
                {
                aggCell.m_value = std::accumulate(values.cbegin(),
                                                  values.cend(), 0);
                }
            else if (aggInfo.m_type == AggregateType::ChangePercent &&
                values.size() > 1)
                {
                const auto oldValue = values.front();
                const auto newValue = values.back();
                aggCell.m_value = safe_divide(newValue-oldValue, oldValue);
                aggCell.SetFormat(CellFormat::Percent);
                }
            else if (aggInfo.m_type == AggregateType::Ratio &&
                values.size() > 1)
                {
                const auto firstValue = values.front();
                const auto secondValue = values.back();
                aggCell.m_value = std::make_pair(firstValue, secondValue);
                }
            }
        }

    //----------------------------------------------------------------
    void Table::InsertRowTotals(std::optional<wxColour> bkColor /*= std::nullopt*/)
        {
        if (GetColumnCount())
            {
            std::vector<std::pair<size_t, size_t>> indexAndRowCounts;
            for (size_t rowIndex = 0; rowIndex < GetRowCount(); ++rowIndex)
                {
                const auto& row = m_table[rowIndex];
                if (row.size() && row[0].m_rowCount > 1)
                    {
                    indexAndRowCounts.push_back(std::make_pair(rowIndex, row[0].m_rowCount));
                    }
                }
            
            // has groups, so add grand total and group subtotals
            if (indexAndRowCounts.size() &&
                // parent group, sub group, then value columns
                GetColumnCount() > 2 &&
                // first two column appear to be grouping labels
                GetCell(0, 0).IsText() && GetCell(0, 1).IsText())
                {
                InsertAggregateRow(Table::AggregateInfo(Table::AggregateType::Total),
                    _(L"Grand Total"), std::nullopt, bkColor);
                for (auto rowIter = indexAndRowCounts.crbegin();
                     rowIter != indexAndRowCounts.crend();
                     ++rowIter)
                    {
                    const auto lastSubgroupRow{ rowIter->first + rowIter->second - 1 };
                    InsertAggregateRow(
                        Table::AggregateInfo(Table::AggregateType::Total).
                            FirstCell(rowIter->first).LastCell(lastSubgroupRow),
                        std::nullopt, rowIter->first + rowIter->second, bkColor);
                    // make parent group consume first cell of subtotal row
                    GetCell(rowIter->first, 0).m_rowCount++;
                    GetCell(lastSubgroupRow+1, 1).m_value = _(L"Total");
                    }
                }
            // no groups, so just add an overall total row at the bottom
            else
                {
                InsertAggregateRow(Table::AggregateInfo(Table::AggregateType::Total),
                                   _(L"Total"), std::nullopt, bkColor);
                }
            }
        }

    //----------------------------------------------------------------
    void Table::InsertAggregateRow(const AggregateInfo& aggInfo,
                                   std::optional<wxString> rowName /*= std::nullopt*/,
                                   std::optional<size_t> rowIndex /*= std::nullopt*/,
                                   std::optional<wxColour> bkColor /*= std::nullopt*/)
        {
        if (GetColumnCount())
            {
            const auto rIndex = (rowIndex.has_value() ? rowIndex.value() : GetRowCount());
            InsertRow(rIndex);
            if (rowName.has_value())
                { GetCell(rIndex, 0).SetValue(rowName.value()); }
            BoldRow(rIndex);
            if (bkColor.has_value())
                { SetRowBackgroundColor(rIndex, bkColor.value()); }

            std::vector<double> colValues;
            for (size_t currentCol = 0; currentCol < GetColumnCount(); ++currentCol)
                {
                colValues.clear();
                // tally values from the whole row, unless a custom range was defined
                for (size_t currentRow = (aggInfo.m_cell1.has_value() ? aggInfo.m_cell1.value() : 0);
                     currentRow < (aggInfo.m_cell2.has_value() ? aggInfo.m_cell2.value()+1 : rIndex);
                     ++currentRow)
                    {
                    const auto& cell = GetCell(currentRow, currentCol);
                    if (cell.IsNumeric() && !std::isnan(cell.GetDoubleValue()))
                        { colValues.push_back(cell.GetDoubleValue()); }
                    CalculateAggregate(aggInfo, GetCell(rIndex, currentCol), colValues);
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::InsertAggregateColumn(const AggregateInfo& aggInfo,
                                      std::optional<wxString> colName /*= std::nullopt*/,
                                      std::optional<size_t> colIndex /*= std::nullopt*/,
                                      std::optional<wxColour> bkColor /*= std::nullopt*/)
        {
        if (GetColumnCount())
            {
            const auto columnIndex = (colIndex.has_value() ? colIndex.value() : GetColumnCount());
            InsertColumn(columnIndex);
            if (colName.has_value())
                { GetCell(0, columnIndex).SetValue(colName.value()); }
            BoldColumn(columnIndex);
            if (bkColor.has_value())
                { SetColumnBackgroundColor(columnIndex, bkColor.value()); }

            size_t currentRow{ 0 };
            std::vector<double> rowValues;
            for (const auto& row : m_table)
                {
                rowValues.clear();
                // tally values from the whole row, unless a custom range was defined
                for (size_t i = (aggInfo.m_cell1.has_value() ? aggInfo.m_cell1.value() : 0);
                     i < (aggInfo.m_cell2.has_value() ? aggInfo.m_cell2.value()+1 : columnIndex);
                     ++i)
                    {
                    const auto& cell = GetCell(currentRow, i);
                    if (cell.IsNumeric() && !std::isnan(cell.GetDoubleValue()))
                        { rowValues.push_back(cell.GetDoubleValue()); }
                    }
                CalculateAggregate(aggInfo, GetCell(currentRow, columnIndex), rowValues);
                ++currentRow;
                }
            }
        }

    //----------------------------------------------------------------
    void Table::AddCellAnnotation(const CellAnnotation& cellNote)
        {
        for (const auto& cell : cellNote.m_cells)
            { GetCell(cell.first, cell.second).Highlight(true); }
        m_cellAnnotations.emplace_back(cellNote);
        }

    //----------------------------------------------------------------
    void Table::AddCellAnnotation(CellAnnotation&& cellNote)
        {
        for (const auto& cell : cellNote.m_cells)
            { GetCell(cell.first, cell.second).Highlight(true); }
        m_cellAnnotations.emplace_back(cellNote);
        }

    //----------------------------------------------------------------
    void Table::ApplyAlternateRowColors(const wxColour alternateColor,
        const size_t startRow /*= 0*/,
        std::optional<size_t> startColumn /*= std::nullopt*/,
        std::optional<size_t> endColumn /*= std::nullopt*/)
        {
        bool isAlternate{ false };
        for (size_t i = startRow; i < GetRowCount(); ++i)
            {
            SetRowBackgroundColor(i, (isAlternate ? alternateColor : *wxWHITE),
                                  startColumn, endColumn);
            isAlternate = !isAlternate;
            }
        }

    //----------------------------------------------------------------
    void Table::SetRowBackgroundColor(const size_t row, const wxColour color,
        std::optional<size_t> startColumn /*= std::nullopt*/,
        std::optional<size_t> endColumn /*= std::nullopt*/)
        {
        if (row < GetRowCount() && m_table[row].size() > 0)
            {
            auto& currentRow = m_table[row];
            startColumn = startColumn.has_value() ? startColumn.value() : 0;
            // don't go beyond the last column
            endColumn = endColumn.has_value() ?
                std::min(endColumn.value(), currentRow.size()-1) : currentRow.size()-1;
            for (size_t i = startColumn.value(); i <= endColumn.value(); ++i)
                { currentRow[i].m_bgColor = color; }
            }
        }

    //----------------------------------------------------------------
    void Table::GroupRow(const size_t row)
        {
        if (row < GetRowCount())
            {
            auto& currentRow = m_table[row];
            if (currentRow.size() <= 1)
                { return; }
            for (size_t i = 0; i < currentRow.size()-1; /*in loop*/)
                {
                size_t startingCounter = i;
                while (i < currentRow.size()-1 &&
                    currentRow[i].IsText() && currentRow[i+1].IsText()&&
                    currentRow[i].GetDisplayValue().CmpNoCase(currentRow[i+1].GetDisplayValue()) == 0)
                    { ++i; }
                if (i > startingCounter)
                    {
                    currentRow[startingCounter].m_columnCount = (i-startingCounter)+1;
                    // clear the cells that are being eclipsed so that they don't
                    // affect column-width calculations
                    for (size_t clearCounter = startingCounter + 1;
                        clearCounter < (startingCounter + 1) + (i - startingCounter);
                        ++clearCounter)
                        { currentRow[clearCounter].SetValue(L""); }
                    }
                else
                    { ++i; }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::GroupColumn(const size_t column)
        {
        if (GetRowCount() > 0 && column < GetColumnCount())
            {
            for (size_t i = 0; i < GetRowCount(); /*in loop*/)
                {
                size_t startingCounter = i;
                while (i < GetRowCount()-1 &&
                    m_table[i][column].IsText() && m_table[i+1][column].IsText()&&
                    m_table[i][column].GetDisplayValue().CmpNoCase(m_table[i+1][column].GetDisplayValue()) == 0)
                    { ++i; }
                if (i > startingCounter)
                    {
                    m_table[startingCounter][column].m_rowCount = (i-startingCounter)+1;
                    // clear the cells that are being eclipsed so that they don't
                    // affect row-width calculations
                    for (size_t clearCounter = startingCounter + 1;
                        clearCounter < (startingCounter + 1) + (i - startingCounter);
                        ++clearCounter)
                        { m_table[clearCounter][column].SetValue(L""); }
                    }
                else
                    { ++i; }
                }
            }
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
                const auto val = GetCell(row, column).GetDoubleValue();
                if (!std::isnan(val))
                    { values.push_back(val); }
                }
            const auto meanVal = statistics::mean(values);
            const auto sdVal = statistics::standard_deviation(values, true);
            // get the z-scores and see who is an outlier
            for (size_t row = 0; row < GetRowCount(); ++row)
                {
                const auto val = GetCell(row, column).GetDoubleValue();
                if (!std::isnan(val))
                    {
                    const auto zScore = statistics::z_score(val, meanVal, sdVal);
                    if (zScore > outlierThreshold)
                        { outlierPositions.push_back(std::make_pair(row, column)); }
                    }
                }
            }
        return outlierPositions;
        }

    //----------------------------------------------------------------
    void Table::CalcMainTableSize(std::vector<wxCoord>& columnWidths,
                                  std::vector<wxCoord>& rowHeights,
                                  Label& measuringLabel,
                                  wxDC& dc) const
        {
        columnWidths.clear();
        columnWidths.resize(GetColumnCount());
        rowHeights.clear();
        rowHeights.resize(GetRowCount());

        size_t currentRow{ 0 }, currentColumn{ 0 };
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            for (const auto& cell : row)
                {
                // make empty cells at least a space so that an empty
                // row or column will at least have some width or height
                const auto cellText = cell.GetDisplayValue();
                measuringLabel.SetText((cellText.length() ? cellText : L" ") + cell.GetPrefix());
                if (cell.m_suggestedLineLength.has_value())
                    { measuringLabel.SplitTextToFitLength(cell.m_suggestedLineLength.value()); }
                measuringLabel.SetFont(cell.m_font);
                auto bBox = measuringLabel.GetBoundingBox(dc);
                // prefix will need 5 DPIs added to each side
                if (cell.GetPrefix().length())
                    { bBox.Inflate(wxSize(ScaleToScreenAndCanvas(5), 0)); }
                // if cell consumes multiple rows, then divides its height across them
                // and set the cells in the rows beneath to the remaining height
                rowHeights[currentRow] =
                    std::max(safe_divide(bBox.GetHeight(), cell.m_rowCount),
                             rowHeights[currentRow]);
                if (cell.m_rowCount > 1)
                    {
                    auto remainingRows = cell.m_rowCount - 1;
                    auto nextRow = currentRow+1;
                    while (remainingRows > 0 && nextRow < GetRowCount())
                        {
                        rowHeights[nextRow] =
                            std::max(safe_divide(bBox.GetWidth(), cell.m_rowCount),
                                     rowHeights[nextRow]);
                        ++nextRow;
                        --remainingRows;
                        }
                    }
                // if cell consumes multiple columns, then divides its width across them
                // and set the proceeding columns to the remaining width
                columnWidths[currentColumn] =
                    std::max(safe_divide(bBox.GetWidth(), cell.m_columnCount),
                             columnWidths[currentColumn]);
                if (cell.m_columnCount > 1)
                    {
                    auto remainingColumns = cell.m_columnCount - 1;
                    auto nextColumn = currentColumn+1;
                    while (remainingColumns > 0 && nextColumn < row.size())
                        {
                        columnWidths[nextColumn] =
                            std::max(safe_divide(bBox.GetWidth(), cell.m_columnCount),
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
                                     std::vector<wxCoord>& rowHeights,
                                     wxRect& drawArea, wxDC& dc) const
        {
        Label measuringLabel(GraphItemInfo().Pen(*wxBLACK_PEN).
            Padding(5, 5, 5, 5).
            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()));

        CalcMainTableSize(columnWidths, rowHeights, measuringLabel, dc);

        auto tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
        auto tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);

        // adjust if row heights collectively go outside of the drawing area
        if (tableHeight > drawArea.GetHeight())
            {
            const auto heightDiffProportion = safe_divide<double>(drawArea.GetHeight(), tableHeight);
            // take away a proportional amount of the difference from each row and column
            for (auto& row : rowHeights)
                { row *= heightDiffProportion; }
            for (auto& col : columnWidths)
                { col *= heightDiffProportion; }
            // re-tally everything
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            }
        
        // adjust if column widths collectively go outside of the drawing area
        if (tableWidth > drawArea.GetWidth())
            {
            const auto widthDiffProportion = safe_divide<double>(drawArea.GetWidth(), tableWidth);
            // take away a proportional amount of the difference from each row and column
            for (auto& row : rowHeights)
                { row *= widthDiffProportion; }
            for (auto& col : columnWidths)
                { col *= widthDiffProportion; }
            // re-tally everything
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            }

        // if requesting minimum width, then stretch it out if needed
        // (note that row heights are preserved)
        if (m_minWidthProportion.has_value() &&
            tableWidth < (drawArea.GetWidth() * m_minWidthProportion.value()))
            {
            const auto widthIncreaseProportion = safe_divide<double>(drawArea.GetWidth(), tableWidth);
            for (auto& col : columnWidths)
                { col *= widthIncreaseProportion; }
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            // may be off by a pixel or so from rounding, so fix that
            const auto roundingDiff = drawArea.GetWidth() - tableWidth;
            columnWidths.back() += roundingDiff;
            tableWidth += roundingDiff;
            }

        // if requesting minimum height, then stretch it out if needed
        // (note that column widths are preserved)
        if (m_minHeightProportion.has_value() &&
            tableHeight < (drawArea.GetHeight() * m_minHeightProportion.value()))
            {
            const auto heightIncreaseProportion = safe_divide<double>(drawArea.GetHeight(),
                                                                      tableHeight);
            for (auto& row : rowHeights)
                { row *= heightIncreaseProportion; }
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            // may be off by a pixel or so from rounding, so fix that
            const auto roundingDiff = drawArea.GetHeight() - tableHeight;
            rowHeights.back() += roundingDiff;
            tableHeight += roundingDiff;
            }

        // if there are annotations, add gutters for them
        wxCoord widestLeftNote{ 0 }, widestRightNote{ 0 };
        for (auto& note : m_cellAnnotations)
            {
            measuringLabel.SetText(note.m_note);
            if (DeduceGutterSide(note) == Side::Left)
                {
                widestLeftNote = std::max(widestLeftNote,
                                          measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            else
                {
                widestRightNote = std::max(widestRightNote,
                                           measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            }

        // if centering table, add extra spacing to keep labels fitting
        auto extraSpaceForCentering =
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

        // adjust the drawing area width, but only if it is smaller than it was before
        const auto newWidth = tableWidth + widestLeftNote + widestRightNote + extraSpaceForCentering;
        drawArea.SetSize(wxSize(std::min(drawArea.GetWidth(), newWidth), tableHeight));

        return wxSize(tableWidth, tableHeight);
        }

    //----------------------------------------------------------------
    void Table::RecalcSizes(wxDC& dc)
        {
        if (GetRowCount() == 0 || GetColumnCount() == 0)
            { return; }

        m_cachedCellRects.clear();

        Graph2D::RecalcSizes(dc);

        // inflates a rect representing the drawing area if
        // padding was added to the original drawing area
        const auto AddPaddingToRect = [&, this](const wxRect& rect)
            {
            wxRect adjustedRect = rect;
            if (!m_minWidthProportion.has_value() &&
                !m_minHeightProportion.has_value())
                { adjustedRect.Inflate(ScaleToScreenAndCanvas(5)); }
            return adjustedRect;
            };

        const wxRect originalFullGraphArea = GetBoundingBox(dc);
        wxRect fullGraphArea = originalFullGraphArea;
        wxRect drawArea = GetPlotAreaBoundingBox();
        auto graphDecorationHeight = fullGraphArea.GetHeight() - drawArea.GetHeight();
        auto graphDecorationWidth = fullGraphArea.GetWidth() - drawArea.GetWidth();

        // calculate the necessary heights of the rows and widths of the columns
        std::vector<wxCoord> columnWidths;
        std::vector<wxCoord> rowHeights;
        
        const auto tableSize = CalculateTableSize(columnWidths, rowHeights, drawArea, dc);
        const auto tableWidth = tableSize.GetWidth();
        const auto tableHeight = tableSize.GetHeight();

        // if the needed area for the table proper is less than the available drawing area,
        // then remove that extra space and recompute the layout
        fullGraphArea.SetHeight(
            std::min(
                AddPaddingToRect(drawArea).GetHeight() + graphDecorationHeight,
                fullGraphArea.GetHeight()));
        fullGraphArea.SetWidth(
            std::min(
                AddPaddingToRect(drawArea).GetWidth() + graphDecorationWidth,
                fullGraphArea.GetWidth()));
        SetBoundingBox(fullGraphArea, dc, GetScaling());
        Graph2D::RecalcSizes(dc);

        // offset the table later if being page aligned within its parent drawing area
        const wxCoord horizontalAlignmentOffset =
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned) ?
             (drawArea.GetWidth() - tableWidth) :
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered) ?
             safe_divide(drawArea.GetWidth() - tableWidth, 2) :
            0;

        const wxCoord verticalAlignmentOffset =
            (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned) ?
             (drawArea.GetHeight() - tableHeight) :
            (GetPageVerticalAlignment() == PageVerticalAlignment::Centered) ?
             safe_divide(drawArea.GetHeight() - tableHeight, 2) :
            0;

        // draw the text
        wxPoint pts[4];
        std::vector<std::shared_ptr<Label>> cellLabels;
        double smallestTextScaling{ std::numeric_limits<double>::max() };
        size_t currentRow{ 0 }, currentColumn{ 0 };
        wxCoord currentXPos{ drawArea.GetX() };
        wxCoord currentYPos{ drawArea.GetY() };
        int columnsToOverwrite{ 0 };
        std::set<std::pair<size_t, size_t>> rowCellsToSkip;
        m_cachedCellRects.resize(GetRowCount(), std::vector<wxRect>(GetColumnCount(), wxRect()));
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& cell : row)
                {
                // skip over rows being eclipsed because of previous cells
                // being multi-row or multi-column
                if (columnsToOverwrite > 0 ||
                    rowCellsToSkip.find(std::make_pair(currentRow, currentColumn)) != rowCellsToSkip.cend())
                    {
                    --columnsToOverwrite;
                    currentXPos += columnWidths[currentColumn];
                    ++currentColumn;
                    continue;
                    }
                columnsToOverwrite = cell.m_columnCount - 1;
                // build a list of cells in the proceeding rows that should be skipped
                // in the next loop if this one is mutli-row
                if (cell.m_rowCount > 1)
                    {
                    auto rowsToSkip = cell.m_rowCount - 1;
                    auto nextRow = currentRow+1;
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
                pts[0] = wxPoint(currentXPos, currentYPos);
                pts[1] = wxPoint((currentXPos + currentColumnWidth), currentYPos);
                pts[2] = wxPoint((currentXPos + currentColumnWidth),
                                 (currentYPos + currentColumnHeight));
                pts[3] = wxPoint(currentXPos, (currentYPos + currentColumnHeight));

                wxRect boxRect(pts[0], pts[2]);
                // if box is going outside of the table by a pixel or so, then trim that off
                if (boxRect.GetRight() > drawArea.GetRight())
                    {
                    const auto rightEdgeOverhang = boxRect.GetRight() - drawArea.GetRight();
                    boxRect.SetWidth(boxRect.GetWidth() - rightEdgeOverhang);
                    }

                const auto cellText = cell.GetDisplayValue();
                auto cellLabel = std::make_shared<Label>(
                    GraphItemInfo(cellText.length() ? cellText : L" ").
                    Pen(wxNullPen).Padding(5, 5, 5, 5).
                    Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                    Font(cell.m_font).
                    FontColor(
                        (cell.m_bgColor.IsOk() ?
                            ColorContrast::BlackOrWhiteContrast(cell.m_bgColor) : *wxBLACK)).
                    FontBackgroundColor(cell.m_bgColor.IsOk() ? cell.m_bgColor : *wxWHITE).
                    Anchoring(Anchoring::Center).
                    AnchorPoint(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                        boxRect.GetTop() + (boxRect.GetHeight() / 2))));
                if (cell.m_suggestedLineLength.has_value())
                    { cellLabel->SplitTextToFitLength(cell.m_suggestedLineLength.value()); }
                cellLabel->SetBoundingBox(boxRect, dc, GetScaling());
                // cache it for annotations and highlights
                m_cachedCellRects[currentRow][currentColumn] = boxRect;
                cellLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                // if an overriding horizontal alignment is in use, then use that
                if (cell.m_horizontalCellAlignment.has_value())
                    {
                    cellLabel->SetPageHorizontalAlignment(cell.m_horizontalCellAlignment.value());
                    }
                // otherwise, deduce the best alignment
                else
                    {
                    cellLabel->SetPageHorizontalAlignment(
                        ((cell.IsNumeric() || cell.IsDate()) ?
                         PageHorizontalAlignment::RightAligned :
                         cell.IsRatio() ? PageHorizontalAlignment::Centered :
                         // if text, center it if multi-column; otherwise, left align
                         cell.m_columnCount > 1 ? PageHorizontalAlignment::Centered :
                         PageHorizontalAlignment::LeftAligned));
                    }
                // if centered in cell, then center the text also (if multi-line)
                if (cellLabel->GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered)
                    { cellLabel->SetTextAlignment(TextAlignment::Centered); }

                smallestTextScaling = std::min(cellLabel->GetScaling(), smallestTextScaling);

                cellLabels.push_back(cellLabel); // need to homogenize scaling of text later

                // special character at the far-left edge (e.g., '$' in accounting formatting)
                if (cell.GetPrefix().length())
                    {
                    auto cellPrefixLabel = std::make_shared<Label>(
                    GraphItemInfo(cell.GetPrefix()).
                        Pen(wxNullPen).Padding(5, 5, 5, 5).
                        Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                        Font(cell.m_font).
                        FontColor(
                            (cell.m_bgColor.IsOk() ?
                                ColorContrast::BlackOrWhiteContrast(cell.m_bgColor) : *wxBLACK)).
                        FontBackgroundColor(wxTransparentColour).
                        Anchoring(Anchoring::TopLeftCorner).
                        AnchorPoint(boxRect.GetLeftTop()));
                    cellPrefixLabel->SetBoundingBox(boxRect, dc, GetScaling());
                    cellPrefixLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                    cellPrefixLabel->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
                    cellLabels.push_back(cellPrefixLabel);
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
            cellLabel->SetScaling(smallestTextScaling);
            // if using page alignment other than left aligned, then adjust its position
            if (horizontalAlignmentOffset > 0 || verticalAlignmentOffset > 0)
                { cellLabel->Offset(horizontalAlignmentOffset, verticalAlignmentOffset); }
            AddObject(cellLabel);
            }

        // draw the row borders
        currentRow = currentColumn = 0;
        rowCellsToSkip.clear();
        for (const auto& rowHeight : rowHeights)
            {
            currentColumn = 0;
            for (const auto& colWidth : columnWidths)
                {
                const auto& cell = GetCell(currentRow, currentColumn);
                // build a list of cells in the following rows that should be skipped
                // in the next loop if this one is mutli-row
                if (cell.m_rowCount > 1)
                    {
                    auto rowsToSkip = cell.m_rowCount - 1;
                    auto nextRow = currentRow+1;
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

        auto highlightedBorderLines = std::make_shared<Lines>(GetHighlightPen(), GetScaling());
        auto borderLines = std::make_shared<Lines>(GetPen(), GetScaling());
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        columnsToOverwrite = 0;
        for (const auto& rowHeight : rowHeights)
            {
            bool isPreviousColumnHighlighted{ false };
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& colWidth : columnWidths)
                {
                const auto& cell = GetCell(currentRow, currentColumn);
                auto parentColumnCell = GetParentColumnWiseCell(currentRow, currentColumn);
                // see if the above cell (or a cell above that which is eclipsing it)
                // is highlighted
                auto aboveCellHighlighted = (currentRow > 0) ?
                    GetCell(currentRow-1, currentColumn).IsHighlighted() :
                    false;
                if (currentRow > 0 && !aboveCellHighlighted)
                    {
                    auto aboveCellsParent = GetParentRowWiseCell(currentRow - 1, currentColumn);
                    if (aboveCellsParent.has_value())
                        {
                        aboveCellHighlighted = aboveCellsParent.value().IsHighlighted();
                        }
                    }
                // draw the horizontal line above the cell.
                // skip over cells being eclipsed by the previous one above that was multi-row
                if (rowCellsToSkip.find(std::make_pair(currentRow, currentColumn)) ==
                    rowCellsToSkip.cend() &&
                    // if top row and top border is turned off, then don't draw the line
                    !(currentRow == 0 && !cell.m_showTopBorder) &&
                    // if top border is not be drawn and cell above's bottom border is also
                    // not drawn, then don't draw the line
                    !(currentRow > 0 && !cell.m_showTopBorder &&
                      !GetCell(currentRow - 1, currentColumn).m_showBottomBorder))
                    {
                    if (cell.IsHighlighted() || aboveCellHighlighted ||
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
                // skip over cells being eclipsed by the previous one since it's mutli-column
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
                    auto parentCell = GetParentRowWiseCell(currentRow, currentColumn);
                    if (cell.IsHighlighted() ||
                        (parentCell.has_value() && parentCell.value().IsHighlighted()))
                        {
                        highlightedBorderLines->AddLine(wxPoint(currentXPos, currentYPos),
                            wxPoint(currentXPos, currentYPos + rowHeight));
                        isPreviousColumnHighlighted = true;
                        }
                    else if (isPreviousColumnHighlighted)
                        {
                        highlightedBorderLines->AddLine(wxPoint(currentXPos, currentYPos),
                            wxPoint(currentXPos, currentYPos + rowHeight));
                        isPreviousColumnHighlighted = false;
                        }
                    else
                        {
                        borderLines->AddLine(wxPoint(currentXPos, currentYPos),
                                             wxPoint(currentXPos, currentYPos+rowHeight));
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
            auto parentRowCell = GetParentRowWiseCell(currentRow, GetColumnCount() - 1);
            auto parentColumnCell = GetParentColumnWiseCell(currentRow, GetColumnCount() - 1);
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
            const auto& cell = GetCell(GetRowCount()-1, currentColumn);
            auto parentColumnCell = GetParentColumnWiseCell(GetRowCount() - 1, currentColumn);
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

        AddObject(borderLines);
        AddObject(highlightedBorderLines);

        // add gutter messages
        auto rightGutter = wxRect(
            wxPoint(drawArea.GetX() + horizontalAlignmentOffset + tableWidth,
                    drawArea.GetY() + verticalAlignmentOffset),
            wxSize(drawArea.GetWidth() - (horizontalAlignmentOffset + tableWidth),
                   drawArea.GetHeight()));
        auto leftGutter = wxRect(
            wxPoint(drawArea.GetX(), drawArea.GetY() + verticalAlignmentOffset),
            wxSize(horizontalAlignmentOffset, drawArea.GetHeight()));
        const auto connectionOverhangWidth = ScaleToScreenAndCanvas(m_connectionOverhangWidth);
        const auto labelSpacingFromLine = ScaleToScreenAndCanvas(m_labelSpacingFromLine);
        for (auto& note : m_cellAnnotations)
            {
            // sort by rows, top-to-bottom
            std::sort(note.m_cells.begin(), note.m_cells.end(),
                [](const auto& lv, const auto& rv) noexcept
                    { return lv.first < rv.first; });
            auto noteConnectionLines = std::make_shared<Lines>(GetHighlightPen(), GetScaling());
            wxCoord lowestY{ drawArea.GetBottom() }, highestY{ drawArea.GetTop() };
            auto gutterSide = DeduceGutterSide(note);
            if (gutterSide == Side::Right)
                {
                // draw lines from the middle of the cells to a little bit outside of the
                // table going into the right gutter
                for (const auto& cell : note.m_cells)
                    {
                    const auto cellRect = GetCachedCellRect(cell.first, cell.second);
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
                    wxPoint(rightGutter.GetX() + connectionOverhangWidth*2, cellsYMiddle));
                AddObject(noteConnectionLines);
                // add the note into the gutter
                auto noteLabel = std::make_shared<Label>(
                    GraphItemInfo(note.m_note).
                    Pen(wxNullPen).
                    // use same text scale as the table (or 1.0 if the table font is really small)
                    Scaling(std::max(1.0, smallestTextScaling)).
                    DPIScaling(GetDPIScaleFactor()).
                    Anchoring(Anchoring::BottomLeftCorner).
                    AnchorPoint(
                        wxPoint(rightGutter.GetX() +
                                (connectionOverhangWidth * 2) + labelSpacingFromLine,
                                cellsYMiddle)) );
                // if label is too long to fit, then split it length-wise to fit in the gutter
                const auto bBox = noteLabel->GetBoundingBox(dc);
                rightGutter.SetLeft(rightGutter.GetLeft() +
                                    (connectionOverhangWidth * 2) + labelSpacingFromLine);
                rightGutter.SetWidth(rightGutter.GetWidth() -
                                     (connectionOverhangWidth * 2) - labelSpacingFromLine);
                if (!Polygon::IsRectInsideRect(bBox, rightGutter))
                    {
                    noteLabel->SplitTextToFitBoundingBox(dc, rightGutter.GetSize());
                    const auto boundBox = noteLabel->GetBoundingBox(dc);
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, boundBox.GetHeight() / 2));
                    }
                else
                    {
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, bBox.GetHeight() / 2));
                    }
                AddObject(noteLabel);
                }
            else
                {
                // draw lines from the middle of the cells to a little bit outside of the
                // table going into the left gutter
                for (const auto& cell : note.m_cells)
                    {
                    const auto cellRect = GetCachedCellRect(cell.first, cell.second);
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
                    wxPoint(leftGutter.GetRight() - connectionOverhangWidth *2, cellsYMiddle));
                AddObject(noteConnectionLines);
                // add the note into the gutter
                auto noteLabel = std::make_shared<Label>(
                    GraphItemInfo(note.m_note).
                    Pen(wxNullPen).
                    // use same text scale as the table
                    Scaling(smallestTextScaling).DPIScaling(GetDPIScaleFactor()).
                    Anchoring(Anchoring::BottomRightCorner).
                    AnchorPoint(
                        wxPoint(leftGutter.GetRight() -
                                (connectionOverhangWidth * 2) - labelSpacingFromLine,
                                cellsYMiddle)) );

                // if label is too long to fit, then split it length-wise to fit in the gutter
                const auto bBox = noteLabel->GetBoundingBox(dc);
                leftGutter.SetWidth(leftGutter.GetWidth() -
                                    (connectionOverhangWidth * 2) - labelSpacingFromLine);
                if (!Polygon::IsRectInsideRect(bBox, leftGutter))
                    {
                    noteLabel->SplitTextToFitBoundingBox(dc, leftGutter.GetSize());
                    const auto boundBox = noteLabel->GetBoundingBox(dc);
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, boundBox.GetHeight() / 2));
                    }
                else
                    {
                    noteLabel->SetAnchorPoint(noteLabel->GetAnchorPoint() +
                                              wxPoint(0, bBox.GetHeight() / 2));
                    }
                AddObject(noteLabel);
                }
            // Base graph resets the scaling for caption to 1.0 and it's a
            // template object that's copied, so you need to adjust the font size
            // instead of the scaling.
            // By default, captions are 75% of the title's font size, so use that
            // and downscale that to what is being used for the cells' and annotations'
            // text, making a consistent look.
            GetCaption().GetFont().SetFractionalPointSize(
                (GetTitle().GetFont().GetFractionalPointSize() * .75) *
                 std::max(1.0, smallestTextScaling));
            }
        }
    }
