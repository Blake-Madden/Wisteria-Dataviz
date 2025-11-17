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
    WaffleChart::WaffleChart(Canvas * canvas, const std::vector<GraphItems::ShapeInfo>& shapes)
        : Graph2D(canvas)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);

        LoadShapeGrid(shapes);
        }

    //----------------------------------------------------------------
    void WaffleChart::LoadShapeGrid(const std::vector<GraphItems::ShapeInfo>& shapes)
        {
        const size_t numberOfShapes =
            std::accumulate(shapes.begin(), shapes.end(), 0, [](const auto val, const auto& shp)
                            { return shp.GetRepeatCount() + val; });

        const auto numOfRows = static_cast<size_t>(std::ceil(std::sqrt(numberOfShapes)));
        m_matrix.resize(numOfRows);

        size_t currentRow{ 0 };
        size_t currentColumn{ 0 };
        const size_t maxCols{ numOfRows };

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

        const int cellsPerSide{ static_cast<int>(m_matrix.size()) };
        const int cellWidth{ safe_divide<int>(drawArea.GetWidth(), cellsPerSide) };
        const int cellHeight{ safe_divide<int>(drawArea.GetHeight(), cellsPerSide) };
        const int cellSize{ std::min(cellWidth, cellHeight) };

        const int totalGridWidth{ cellsPerSide * cellSize };
        const int totalGridHeight{ cellsPerSide * cellSize };

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
    } // namespace Wisteria::Graphs
