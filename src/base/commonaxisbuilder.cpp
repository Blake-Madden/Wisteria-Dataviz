///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "commonaxisbuilder.h"

using namespace Wisteria::GraphItems;

namespace Wisteria
    {
    //----------------------------------------------------------------
    std::shared_ptr<Axis>
        CommonAxisBuilder::BuildRightAxis(Canvas* canvas,
            std::initializer_list<std::shared_ptr<Graphs::Graph2D>> graphs)
        {
        if (graphs.size() < 2)
            { return nullptr; }
        for (auto graphIter = graphs.begin() + 1; graphIter < graphs.end(); ++graphIter)
            {
            // copy the left axis range from the first plot to this one,
            // then turn off the labels
            graphIter->get()->GetLeftYAxis().CopySettings(graphs.begin()->get()->GetLeftYAxis());
            graphIter->get()->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            }
        // create a common axis, also copied from the first plot's left axis
        auto commonAxis = std::make_shared<Axis>(AxisType::RightYAxis);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetLeftYAxis());
        // tell the canvas to align the axis line to the left side of its
        // bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(0, 0, 0, 10);
        // Get the canvas size of the axis and add it to the canvas.
        commonAxis->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(commonAxis));

        // now that we are done copying the left axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        // tell the canvas to align the plots and stand-alone axes across each row
        canvas->AlignRowContent(true);

        return commonAxis;
        }

    //----------------------------------------------------------------
    std::shared_ptr<Axis>
        CommonAxisBuilder::BuildBottomAxis(Canvas* canvas,
            std::initializer_list<std::shared_ptr<Graphs::Graph2D>> graphs)
        {
        if (graphs.size() < 2)
            { return nullptr; }
        for (auto graphIter = graphs.begin() + 1; graphIter < graphs.end(); ++graphIter)
            {
            // copy the bottom axis range from the first plot to this one,
            // then turn off the labels
            graphIter->get()->GetBottomXAxis().CopySettings(graphs.begin()->get()->GetBottomXAxis());
            graphIter->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            }
        // create a common axis, also copied from the first plot's bottom axis
        auto commonAxis = std::make_shared<Axis>(AxisType::BottomXAxis);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetBottomXAxis());
        // tell the canvas to align the axis line to the bottom side of its
        // bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(10, 0, 5, 0);
        // Get the canvas size of the axis and add it to the canvas.
        commonAxis->SetCanvasHeightProportion(canvas->CalcMinHeightProportion(commonAxis));

        // now that we are done copying the bottom axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        // tell the canvas to align the plots and stand-alone axes across each column
        canvas->AlignColumnContent(true);

        return commonAxis;
        }
    }
