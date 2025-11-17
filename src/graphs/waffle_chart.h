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
            and the number of times it should appear.
            The chart expands these into a grid, sizes each cell uniformly, and fits the matrix
            into the drawing area.*/
    class WaffleChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WaffleChart);
        WaffleChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param shapes The list of shapes (and respective repeat count) to draw across
                the waffle chart.*/
        explicit WaffleChart(Canvas* canvas,
                             const std::vector<std::pair<GraphItems::ShapeInfo, size_t>>& shapes);

      private:
        void RecalcSizes(wxDC& dc) final;

        [[deprecated("Waffle charts do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

        std::vector<std::vector<GraphItems::ShapeInfo>> m_matrix;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WAFFLE_CHART_H
