/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_COMMON_AXIS_BUILDER_H__
#define __WISTERIA_COMMON_AXIS_BUILDER_H__

#include "axis.h"
#include "../graphs/graph2d.h"
#include <vector>

namespace Wisteria
    {
    /** @brief Constructs a common axis for a canvas, connected to a list of graphs
            from the same row or column.*/
    class CommonAxisBuilder
        {
    public:
        /** @brief Builds a common axis for graphs along a row.
            @details This function will perform the following:
            - A new axis will be copied from the first graph's left axis
            - The first graph's left axis settings and scale will be applied to the
                other graphs in the list
            - The graphs will then have their left axes's labels turned off
                (since the common axis will be serving this purpose)
            - Finally, the newly constructed axis will be returned, where the caller
                should add it onto the canvas, to the right or left of the graphs

            @param canvas The canvas that the axis will be added to (later, by the caller).\n
                Note that The graphs connected to the axis should also be on this canvas,
                and also be in the same row of the canvas.
            @param graphs The graphs that will be connected to the common axis.\n
                Note that these graphs' left Y axis will have their labels turned off after
                calling this function.
            @param axisType The axis type to create; AxisType::LeftYAxis or AxisType::RightYAxis.
            @returns The common axis for the graphs, which should be added to the canvas
                (to the right or left of the graphs).
            @par Example
            @code
             // with "canvas" being the canvas containing the graphs and
             // "linePlot" and "boxPlot" being two graphs in the canvas's
             // first row:
             canvas->SetFixedObject(0, 0, linePlot);
             canvas->SetFixedObject(0, 1, boxPlot);
             canvas->SetFixedObject(0, 2,
                // construct a common axis connected to the line and box plots,
                // and add it to the right of them on the canvas
                CommonAxisBuilder::BuildYAxis(canvas,
                    { linePlot, boxPlot }, AxisType::RightYAxis));
            @endcode*/
        [[nodiscard]] static std::shared_ptr<GraphItems::Axis> BuildYAxis(Canvas* canvas,
            std::vector<std::shared_ptr<Graphs::Graph2D>> graphs,
            AxisType axisType);
        /** @brief Builds a common axis for graphs along a column.
            @details This function will perform the following:
            - A new axis will be copied from the first graph's bottom axis
            - The first graph's bottom axis settings and scale will be applied to the
                other graphs in the list
            - The graphs will then have their bottom axes's labels turned off
                (since the common axis will be serving this purpose)
            - Finally, the newly constructed axis will be returned, where the caller
                should add it onto the canvas, below or above the graphs
            - The caller will also need to set the proportions of the rows,
                using the returned axis's @c GetCanvasHeightProportion() method

            @param canvas The canvas that the axis will be added to (later, by the caller).\n
                Note that The graphs connected to the axis should also be on this canvas,
                and also be in the same column of the canvas.
            @param graphs The graphs that will be connected to the common axis.\n
                Note that these graphs' bottom X axis will have their labels turned off after
                calling this function.
            @param axisType The axis type to create; AxisType::BottomXAxis or AxisType::TopXAxis.
            @param useCommonLeftAxis If @c true, gets the min and max of the graphs' left X
                axes and sets all the graphs to use that range.\n
                This is useful for further homogenizing the graphs.
            @returns The common axis for the graphs, which should be added to the canvas
                (below or above the graphs).
            @par Example
            @code
             // with "canvas" being the canvas containing the graphs and
             // "linePlotAY1" and "linePlotAY2" being two graphs in the canvas's column:
             auto commonAxis = CommonAxisBuilder::BuildXAxis(canvas,
                { linePlotAY1 , linePlotAY2 }, AxisType::BottomXAxis);

             // add graphs and their shared axis into the first column
             canvas->SetFixedObject(0, 0, linePlotAY1);
             canvas->SetFixedObject(1, 0, linePlotAY2);
             canvas->SetFixedObject(2, 0, commonAxis);

             // adjust the heights of the rows, auto-fitting the common axis
             canvas->CalcRowDimensions();
            @endcode*/
        [[nodiscard]] static std::shared_ptr<GraphItems::Axis> BuildXAxis(Canvas* canvas,
            std::vector<std::shared_ptr<Graphs::Graph2D>> graphs,
            AxisType axisType,
            const bool useCommonLeftAxis = false);
        };
    };

/** @}*/

#endif //__WISTERIA_COMMON_AXIS_BUILDER_H__
