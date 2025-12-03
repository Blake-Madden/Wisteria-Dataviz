/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WAFFLE_CHART_H
#define WISTERIA_WAFFLE_CHART_H

#include "../base/fillableshape.h"
#include "../base/shapes.h"
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A chart that arranges repeated shapes into a square-like grid.
        @details Unlike other graphs that take a Data::Dataset, a Waffle Chart is built from a
            vector of shape definitions, where each entry includes a GraphItems::ShapeInfo
            (which contains the number of times it should repeat).
            The chart expands these into a grid, sizes each cell uniformly, and fits the grid
            into the drawing area.
        @image html WaffleChart.png width=90%*/
    class WaffleChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WaffleChart);
        WaffleChart() = default;

      public:
        /// @brief Adds extra cells (using the specified index into the shapes)
        ///     to ensure that the waffle has a minimum number of cells.
        struct GridRounding
            {
            size_t m_numberOfCells{ 100 };
            size_t m_shapesIndex{ 0 };
            };

        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param shapes The list of shapes (and respective repeat counts) to draw across
                the waffle chart.
            @param gridRound Ensures that a minimum number of cells are drawn. If the total
                number of cells from the shapes' repeat counts is less than this, then extra
                cells will be added to the specified shape (by index). This is useful for
                when a cell represents percentages and the cells don't add to 100 because of
                rounding issues.
            @param rowCount The number of rows to split the shapes into. This is optional,
                and by default the grid will have an equal number of rows and columns.
            @par Example:
            @code
            auto plot = std::make_shared<WaffleChart>(
                subframe->m_canvas,
                std::vector<GraphItems::ShapeInfo>{
                    // mostly transparent shapes
                    { GraphItems::ShapeInfo{}
                          .Shape(Icons::IconShape::BusinessWoman)
                          .Brush(*wxTRANSPARENT_BRUSH)
                          .Pen(Colors::ColorContrast::ChangeOpacity(*wxBLACK, 75))
                          .Repeat(61) },
                    { GraphItems::ShapeInfo{}
                          .Shape(Icons::IconShape::Man)
                          .Brush(*wxTRANSPARENT_BRUSH)
                          .Repeat(29) },
                    // fill with solid colors
                    { GraphItems::ShapeInfo{}
                          .Shape(Icons::IconShape::BusinessWoman)
                          .Brush(Colors::ColorBrewer::GetColor(Colors::Color::BabyPink))
                          .Pen(Colors::ColorContrast::ChangeOpacity(*wxBLACK, 75))
                          .Repeat(6) },
                    { GraphItems::ShapeInfo{}
                          .Shape(Icons::IconShape::Man)
                          .Brush(Colors::ColorBrewer::GetColor(Colors::Color::BabyBlue))
                          .Repeat(4) } });
            @endcode*/
        explicit WaffleChart(Canvas* canvas, std::vector<GraphItems::ShapeInfo> shapes,
                             const std::optional<GridRounding>& gridRound = std::nullopt,
                             const std::optional<size_t> rowCount = std::nullopt);

      private:
        void LoadShapeGrid(std::vector<GraphItems::ShapeInfo>& shapes,
                           const std::optional<GridRounding>& gridRound,
                           const std::optional<size_t> rowCount);
        void RecalcSizes(wxDC& dc) final;

        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

        std::vector<std::vector<GraphItems::ShapeInfo>> m_matrix;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WAFFLE_CHART_H
