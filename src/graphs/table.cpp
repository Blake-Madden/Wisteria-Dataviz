#include "table.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    wxString Table::TableCell::GetDisplayValue() const
        {
        if (const auto strVal{ std::get_if<wxString>(&m_value) }; strVal)
            { return *strVal; }
        else if (const auto dVal{ std::get_if<double>(&m_value) }; dVal)
            {
            if (std::isnan(*dVal))
                { return wxEmptyString; }
            else if (m_valueFormat == CellFormat::Percent)
                {
                return wxNumberFormatter::ToString((*dVal)*100, m_precision,
                    wxNumberFormatter::Style::Style_None) + L"%";
                }
            else
                {
                return wxNumberFormatter::ToString(*dVal, m_precision,
                    wxNumberFormatter::Style::Style_WithThousandsSep);
                }
            }
        else if (const auto dVal{ std::get_if<std::pair<double, double>>(&m_value) }; dVal)
            {
            if (std::isnan(dVal->first) || std::isnan(dVal->second))
                { return wxEmptyString; }
            if (dVal->first > dVal->second)
                {
                return wxString::Format(L"%s : 1",
                    wxNumberFormatter::ToString(
                        safe_divide(dVal->first, dVal->second), m_precision,
                        wxNumberFormatter::Style::Style_WithThousandsSep |
                        wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            else
                {
                return wxString::Format(L"1 : %s",
                    wxNumberFormatter::ToString(
                        safe_divide(dVal->second, dVal->first), m_precision,
                        wxNumberFormatter::Style::Style_WithThousandsSep |
                        wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            }
        else if (const auto dVal{ std::get_if<wxDateTime>(&m_value) }; dVal)
            {
            if (!dVal->IsValid())
                { return wxEmptyString; }
            return dVal->FormatDate();
            }
        else
            { return wxEmptyString; }
        }

    //----------------------------------------------------------------
    Table::Table(Wisteria::Canvas* canvas) : Graph2D(canvas)
        {
        GetPen() = ColorBrewer::GetColor(Colors::Color::AshGrey,
                                         Settings::GetTranslucencyValue());

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
        const std::initializer_list<wxString>& columns,
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
                        L"'%s': column not found for table.", colName).ToUTF8());
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
                        L"'%s': column not found for table.", colName).ToUTF8());
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
                aggCell.m_valueFormat = CellFormat::Percent;
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
            for (size_t rowIndex = 0; rowIndex < m_table.size(); ++rowIndex)
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
            InsertRow(rIndex, rowName);
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
            InsertColumn(columnIndex, colName);
            BoldColumn(columnIndex);
            if (bkColor.has_value())
                { SetColumnBackgroundColor(columnIndex, bkColor.value()); }

            size_t currentRow{ 0 };
            std::vector<double> rowValues;
            for (auto& row : m_table)
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
        if (row < m_table.size() && m_table[row].size() > 0)
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
        if (row < m_table.size())
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
                    }
                else
                    { ++i; }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::GroupColumn(const size_t column)
        {
        if (m_table.size() > 0 && column < m_table[0].size())
            {
            for (size_t i = 0; i < m_table.size(); /*in loop*/)
                {
                size_t startingCounter = i;
                while (i < m_table.size()-1 &&
                    m_table[i][column].IsText() && m_table[i+1][column].IsText()&&
                    m_table[i][column].GetDisplayValue().CmpNoCase(m_table[i+1][column].GetDisplayValue()) == 0)
                    { ++i; }
                if (i > startingCounter)
                    {
                    m_table[startingCounter][column].m_rowCount = (i-startingCounter)+1;
                    }
                else
                    { ++i; }
                }
            }
        }

    //----------------------------------------------------------------
    void Table::RecalcSizes(wxDC& dc)
        {
        if (m_table.size() == 0 || m_table[0].size() == 0)
            { return; }

        Graph2D::RecalcSizes(dc);

        wxRect drawArea = GetPlotAreaBoundingBox();
        // add some padding around the table, unless client is controlling the dimensions
        if (!m_minWidthProportion.has_value() &&
            !m_minHeightProportion.has_value())
            { drawArea.Deflate(ScaleToScreenAndCanvas(5)); }

        // calculate the necessary heights of the rows and widths of the column
        wxPoint pts[4];
        std::vector<wxCoord> rowHeights(m_table.size(), 0);
        std::vector<wxCoord> columnWidths(m_table[0].size(), 0);
        size_t currentRow{ 0 }, currentColumn{ 0 };
        Label measuringLabel(GraphItemInfo().Pen(*wxBLACK_PEN).
            Padding(5, 5, 5, 5).
            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()));
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            for (const auto& cell : row)
                {
                // make empty cells at least a space so that an empty
                // row or column will at least have some width or height
                const auto cellText = cell.GetDisplayValue();
                measuringLabel.SetText(cellText.length() ? cellText : L" ");
                if (cell.m_suggestedLineLength.has_value())
                    { measuringLabel.SplitTextToFitLength(cell.m_suggestedLineLength.value()); }
                measuringLabel.SetFont(cell.m_font);
                const auto bBox = measuringLabel.GetBoundingBox(dc);
                // if cell consumes multiple rows, then divides its height across them
                // and set the cells in the rows beneath to the remaining height
                rowHeights[currentRow] =
                    std::max(safe_divide(bBox.GetHeight(), cell.m_rowCount),
                             rowHeights[currentRow]);
                if (cell.m_rowCount > 1)
                    {
                    auto remainingRows = cell.m_rowCount - 1;
                    auto nextRow = currentRow+1;
                    while (remainingRows > 0 && nextRow < m_table.size())
                        {
                        rowHeights[nextRow++] =
                            safe_divide(bBox.GetWidth(), cell.m_rowCount);
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
                        columnWidths[nextColumn++] =
                            safe_divide(bBox.GetWidth(), cell.m_columnCount);
                        --remainingColumns;
                        }
                    }
                ++currentColumn;
                }
            ++currentRow;
            }

        auto tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
        auto tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);

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

        // offset the table later if being page aligned within its parent drawing area
        const wxCoord horizontalAlignment =
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned) ?
             (drawArea.GetWidth() - tableWidth) :
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered) ?
             safe_divide(drawArea.GetWidth() - tableWidth, 2) :
            0;

        const wxCoord verticalAlignment =
            (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned) ?
             (drawArea.GetHeight() - tableHeight) :
            (GetPageVerticalAlignment() == PageVerticalAlignment::Centered) ?
             safe_divide(drawArea.GetHeight() - tableHeight, 2) :
            0;

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

        // draw the text
        std::vector<std::shared_ptr<Label>> cellLabels;
        double smallestTextScaling{ std::numeric_limits<double>::max() };
        currentRow = currentColumn = 0;
        wxCoord currentXPos{ drawArea.GetX() };
        wxCoord currentYPos{ drawArea.GetY() };
        int columnsToOverwrite{ 0 };
        std::set<std::pair<size_t, size_t>> rowCellsToSkip;
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

                const wxRect boxRect(pts[0], pts[2]);

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
            if (horizontalAlignment > 0 || verticalAlignment > 0)
                { cellLabel->Offset(horizontalAlignment, verticalAlignment); }
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

        auto borderLines = std::make_shared<Lines>(GetPen(), GetScaling());
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        columnsToOverwrite = 0;
        for (const auto& rowHeight : rowHeights)
            {
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& colWidth : columnWidths)
                {
                const auto& cell = GetCell(currentRow, currentColumn);
                // skip over cells being eclipsed by the previous one above that was multi-row
                if (rowCellsToSkip.find(std::make_pair(currentRow, currentColumn)) ==
                    rowCellsToSkip.cend() &&
                    !(currentRow == 0 && !cell.m_showOuterTopBorder))
                    {
                    borderLines->AddLine(wxPoint(currentXPos, currentYPos),
                                         wxPoint(currentXPos+ colWidth, currentYPos));
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
                if (!(currentColumn == 0 && !cell.m_showOuterLeftBorder))
                    {
                    borderLines->AddLine(wxPoint(currentXPos, currentYPos),
                                         wxPoint(currentXPos, currentYPos+rowHeight));
                    }
                currentXPos += colWidth;
                ++currentColumn;
                }
            currentYPos += rowHeight;
            ++currentRow;
            }
        // outer right border
        currentYPos = drawArea.GetY();
        currentRow = currentColumn = 0;
        for (const auto& rowHeight : rowHeights)
            {
            const auto& cell = GetCell(currentRow, m_table[0].size()-1);
            if (cell.m_showOuterRightBorder)
                {
                borderLines->AddLine(wxPoint(drawArea.GetX() + tableWidth, currentYPos),
                                     wxPoint(drawArea.GetX() + tableWidth, currentYPos + rowHeight));
                }
            currentYPos += rowHeight;
            ++currentRow;
            }
        // outer bottom border
        currentXPos = drawArea.GetX();
        for (const auto& colWidth : columnWidths)
            {
            const auto& cell = GetCell(m_table.size()-1, currentColumn);
            if (cell.m_showOuterBottomBorder)
                {
                borderLines->AddLine(wxPoint(currentXPos, drawArea.GetY() + tableHeight),
                                     wxPoint(currentXPos + colWidth, drawArea.GetY() + tableHeight));
                }
            currentXPos += colWidth;
            ++currentColumn;
            }

        // if using page alignment other than left aligned, then adjust its position
        if (horizontalAlignment > 0 || verticalAlignment > 0)
            { borderLines->Offset(horizontalAlignment, verticalAlignment); }
        AddObject(borderLines);
        }
    }
