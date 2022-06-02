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
#include <initializer_list>

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
                - Finally, the newly constructed axis will be returned, where the client
                  should add it onto the canvas, right of the graphs

            @param canvas The canvas that the axis will be added to (later, but the caller).\n
                Note that The graphs connected to the axis should also be on this canvas,
                and also be in the same row of the canvas.
            @param graphs The graphs that will be connected to the common axis.\n
                Note that these graphs' left Y axis will have their labels turned off after
                calling this function.
            @returns The common axis for the graphs, which should be added to the canvas
                (right of the graphs).
            @par Example
            @code
             // with "canvas" being the canvas containing your graphs and
             // "linePlot" and "boxPlot" being two graphs in the canvas's
             // first row:
             canvas->SetFixedObject(0, 2,
                // construct a common axis connected to the line and box plots,
                // and add it to the righ of them on the canvas
                CommonAxisBuilder::BuildRightAxis(canvas,
                    { linePlot, boxPlot}));
            @endcode*/
        static [[nodiscard]] std::shared_ptr<GraphItems::Axis> BuildRightAxis(Canvas* canvas,
            std::initializer_list<std::shared_ptr<Graphs::Graph2D>> graphs);
        };
    };

/** @}*/

#endif //__WISTERIA_COMMON_AXIS_BUILDER_H__
