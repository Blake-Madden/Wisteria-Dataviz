///////////////////////////////////////////////////////////////////////////////
// Name:        waffle_chart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "waffle_chart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WaffleChart, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WaffleChart::WaffleChart(Canvas * canvas, std::vector<GraphItems::ShapeInfo> shapes,
                             const std::optional<GridRounding>& gridRound /*= std::nullopt*/,
                             const std::optional<size_t> rowCount /*= std::nullopt*/)
        : Graph2D(canvas)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);

        LoadShapeGrid(shapes, gridRound, rowCount);
        }

    //----------------------------------------------------------------
    void WaffleChart::LoadShapeGrid(std::vector<GraphItems::ShapeInfo> & shapes,
                                    const std::optional<GridRounding>& gridRound,
                                    const std::optional<size_t> rowCount)
        {
        if (rowCount && rowCount.value() == 0)
            {
            throw std::runtime_error(
                _(L"Requested row count for waffle chart cannot be zero.").ToUTF8());
            }

        size_t numberOfShapes =
            std::accumulate(shapes.begin(), shapes.end(), 0, [](const auto val, const auto& shp)
                            { return shp.GetRepeatCount() + val; });
        if (gridRound && numberOfShapes < gridRound.value().m_numberOfCells &&
            gridRound.value().m_shapesIndex < shapes.size())
            {
            shapes[gridRound.value().m_shapesIndex].Repeat(
                shapes[gridRound.value().m_shapesIndex].GetRepeatCount() +
                (gridRound.value().m_numberOfCells - numberOfShapes));
            numberOfShapes = gridRound.value().m_numberOfCells;
            }

        const auto numOfRows =
            rowCount.value_or(static_cast<size_t>(std::ceil(std::sqrt(numberOfShapes))));
        m_matrix.resize(numOfRows);

        size_t currentRow{ 0 };
        size_t currentColumn{ 0 };
        const size_t maxCols{ rowCount ? static_cast<size_t>(std::ceil(safe_divide<double>(
                                             numberOfShapes, rowCount.value()))) :
                                         numOfRows };

        for (const auto& shape : shapes)
            {
            for (size_t i = 0; i < shape.GetRepeatCount(); ++i)
                {
                m_matrix[currentRow].push_back(shape);

                ++currentColumn;
                // if row is full, go to next one
                if (currentColumn >= maxCols)
                    {
                    currentColumn = 0;
                    ++currentRow;

                    if (currentRow >= numOfRows)
                        {
                        break;
                        }
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void WaffleChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        // size the boxes to fit in the area available
        const wxRect drawArea = GetPlotAreaBoundingBox();

        const int rows{ static_cast<int>(m_matrix.size()) };

        int maxCols{ 0 };
        for (const auto& r : m_matrix)
            {
            maxCols = std::max(maxCols, static_cast<int>(r.size()));
            }

        const int cellWidth{ safe_divide<int>(drawArea.GetWidth(), maxCols) };
        const int cellHeight{ safe_divide<int>(drawArea.GetHeight(), rows) };
        const int cellSize{ std::min(cellWidth, cellHeight) };

        const int totalGridWidth{ maxCols * cellSize };
        const int totalGridHeight{ rows * cellSize };

        const int offsetX{ drawArea.GetX() +
                           safe_divide<int>(drawArea.GetWidth() - totalGridWidth, 2) };
        const int offsetY{ drawArea.GetY() +
                           safe_divide<int>(drawArea.GetHeight() - totalGridHeight, 2) };

        for (size_t row = 0; row < m_matrix.size(); ++row)
            {
            for (size_t column = 0; column < m_matrix[row].size(); ++column)
                {
                const auto& shpInfo{ m_matrix[row][column] };
                const wxPoint topLeft{ offsetX + static_cast<int>(column) * cellSize,
                                       offsetY + static_cast<int>(row) * cellSize };

                if (shpInfo.GetFillPercent() < math_constants::full)
                    {
                    AddObject(std::make_unique<GraphItems::FillableShape>(
                        GraphItems::GraphItemInfo()
                            .Pen(shpInfo.GetPen())
                            .Brush(shpInfo.GetBrush())
                            .Selectable(false)
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(topLeft),
                        shpInfo.GetShape(), wxSize{ cellSize, cellSize },
                        shpInfo.GetFillPercent()));
                    }
                else
                    {
                    AddObject(std::make_unique<GraphItems::Shape>(
                        GraphItems::GraphItemInfo()
                            .Pen(shpInfo.GetPen())
                            .Brush(shpInfo.GetBrush())
                            .Selectable(false)
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(topLeft),
                        shpInfo.GetShape(), wxSize{ cellSize, cellSize }));
                    }
                }
            }
        }

    //-------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> WaffleChart::CreateLegend(const LegendOptions& options)
        {
        // Base legend label container
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo()
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        // collect unique legend entries by:
        //   - shape icon
        //   - brush color
        struct LegendEntry
            {
            Icons::IconShape shape;
            wxColour fillColor;
            wxString label;
            GraphItems::ShapeInfo shapeInfo;
            };

        std::vector<LegendEntry> entries;

        const auto findMatch = [&](const GraphItems::ShapeInfo& shp)
        {
            return std::ranges::find_if(entries,
                                        [&](const LegendEntry& entry)
                                        {
                                            return entry.shape == shp.GetShape() &&
                                                   entry.fillColor == shp.GetBrush().GetColour();
                                        });
        };

        for (const auto& row : m_matrix)
            {
            for (const auto& shp : row)
                {
                auto it = findMatch(shp);
                if (it != entries.cend())
                    {
                    // duplicate based on (shape & fill color)
                    // (update label to the newest one)
                    it->label = shp.GetText();
                    }
                else
                    {
                    entries.push_back(
                        { shp.GetShape(), shp.GetBrush().GetColour(), shp.GetText(), shp });
                    }
                }
            }

        // build legend text & icons
        wxString legendText;
        size_t count{ 0 };

        for (const auto& entry : entries)
            {
            if (count == Settings::GetMaxLegendItemCount())
                {
                legendText.append(L"\u2026");
                break;
                }

            wxString label = entry.label;

            if (label.length() > Settings::GetMaxLegendTextLength() &&
                Settings::GetMaxLegendTextLength() >= 1)
                {
                label.erase(Settings::GetMaxLegendTextLength() - 1);
                label.append(L"\u2026");
                }

            legendText.append(label).append(L"\n");

            // add icon
            legend->GetLegendIcons().emplace_back(entry.shapeInfo.GetShape(),
                                                  entry.shapeInfo.GetPen(), entry.fillColor);
            ++count;
            }

        legend->SetText(legendText.Trim());

        // placement and sizing adjustments
        AdjustLegendSettings(*legend, options.GetPlacementHint());

        return legend;
        }
    } // namespace Wisteria::Graphs
