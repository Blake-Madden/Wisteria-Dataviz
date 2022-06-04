#include "table.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    Table::Table(Wisteria::Canvas* canvas) : Graph2D(canvas)
        {
        // arbitrary ranges, just need to create any sort of plotting area
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void Table::RecalcSizes(wxDC& dc)
        {
        if (m_table.size() == 0 || m_table[0].size() == 0)
            { return; }

        Graph2D::RecalcSizes(dc);

        const wxRect drawArea = GetPlotAreaBoundingBox();

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
                const auto bBox = measuringLabel.GetBoundingBox(dc);
                rowHeights[currentRow] = std::max(bBox.GetHeight(), rowHeights[currentRow]);
                columnWidths[currentColumn] = std::max(bBox.GetWidth(),
                                                       columnWidths[currentColumn]);
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

        // if requesting minimum height, then stretch it out if needed
        // (note that column widths are preserved)
        if (m_minHeightProportion.has_value() &&
            tableHeight < (drawArea.GetHeight() * m_minHeightProportion.value()))
            {
            const auto heightIncreaseProportion = safe_divide<double>(drawArea.GetHeight(), tableHeight);
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
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& cell : row)
                {
                pts[0] = wxPoint(currentXPos, currentYPos);
                pts[1] = wxPoint((currentXPos + columnWidths[currentColumn]), currentYPos);
                pts[2] = wxPoint((currentXPos + columnWidths[currentColumn]),
                                 (currentYPos + rowHeights[currentRow]));
                pts[3] = wxPoint(currentXPos, (currentYPos + rowHeights[currentRow]));

                const wxRect boxRect(pts[0], pts[2]);

                const auto cellText = cell.GetDisplayValue();
                auto cellLabel = std::make_shared<Label>(
                    GraphItemInfo(cellText.length() ? cellText : L" ").
                    Pen(wxNullPen).Padding(5, 5, 5, 5).
                    Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                    FontColor(
                        (cell.m_bgColor.IsOk() ?
                            ColorContrast::BlackOrWhiteContrast(cell.m_bgColor) : *wxBLACK)).
                    FontBackgroundColor(cell.m_bgColor.IsOk() ? cell.m_bgColor : *wxWHITE).
                    Anchoring(Anchoring::Center).
                    AnchorPoint(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                        boxRect.GetTop() + (boxRect.GetHeight() / 2))));
                cellLabel->SetBoundingBox(boxRect, dc, GetScaling());
                cellLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                cellLabel->SetPageHorizontalAlignment(
                    (cell.IsNumeric() ?
                     PageHorizontalAlignment::RightAligned :
                     PageHorizontalAlignment::LeftAligned));

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
            AddObject(cellLabel);
            }

        auto borderLines = std::make_shared<Lines>(GetPen(), GetScaling());
        // draw the row borders
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        for (const auto& row : rowHeights)
            {
            borderLines->AddLine(wxPoint(drawArea.GetX(), currentYPos),
                                 wxPoint(currentXPos+tableWidth, currentYPos));
            currentYPos += row;
            }
        borderLines->AddLine(wxPoint(drawArea.GetX(), currentYPos),
                             wxPoint(currentXPos+tableWidth, currentYPos));

        // draw the column borders
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        for (const auto& col : columnWidths)
            {
            borderLines->AddLine(wxPoint(currentXPos, drawArea.GetY()),
                                 wxPoint(currentXPos, currentYPos+tableHeight));
            currentXPos += col;
            }
        borderLines->AddLine(wxPoint(currentXPos, drawArea.GetY()),
                             wxPoint(currentXPos, currentYPos+tableHeight));
        AddObject(borderLines);
        }
    }
