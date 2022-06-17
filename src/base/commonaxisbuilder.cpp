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
            graphIter->get()->GetLeftYAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the first plot's left axis
        auto commonAxis = std::make_shared<Axis>(AxisType::RightYAxis);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetLeftYAxis());
        commonAxis->GetTitle() = graphs.begin()->get()->GetLeftYAxis().GetTitle();
        // tell the canvas to align the axis line to the left side of its
        // bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(0, 0, 0, 10);
        // Get the canvas size of the axis and add it to the canvas.
        commonAxis->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(commonAxis));
        commonAxis->FitContentWidthToCanvas(true);

        // now that we are done copying the left axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetLeftYAxis().GetTitle().Show(false);

        // tell the canvas to align the plots and stand-alone axes across each row
        canvas->AlignRowContent(true);

        return commonAxis;
        }

    //----------------------------------------------------------------
    std::shared_ptr<Axis>
        CommonAxisBuilder::BuildBottomAxis(Canvas* canvas,
            std::initializer_list<std::shared_ptr<Graphs::Graph2D>> graphs,
            const bool applyCommonLeftAxis /*= false*/)
        {
        if (graphs.size() < 2)
            { return nullptr; }
        for (auto graphIter = graphs.begin() + 1; graphIter < graphs.end(); ++graphIter)
            {
            // copy the bottom axis range from the first plot to this one,
            // then turn off the labels
            graphIter->get()->GetBottomXAxis().CopySettings(graphs.begin()->get()->GetBottomXAxis());
            graphIter->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graphIter->get()->GetBottomXAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the first plot's bottom axis
        auto commonAxis = std::make_shared<Axis>(AxisType::BottomXAxis);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetBottomXAxis());
        commonAxis->GetTitle() = graphs.begin()->get()->GetBottomXAxis().GetTitle();
        // tell the canvas to align the axis line to the bottom side of its  bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(10, 0, 5, 0);
        // get the canvas size of the axis and add it to the canvas
        commonAxis->SetCanvasHeightProportion(canvas->CalcMinHeightProportion(commonAxis));
        commonAxis->FitCanvasHeightToContent(true);

        // now that we are done copying the bottom axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetBottomXAxis().GetTitle().Show(false);

        // tell the canvas to align the plots and stand-alone axes across each column
        canvas->AlignColumnContent(true);

        if (applyCommonLeftAxis)
            {
            std::optional<std::pair<double, double>> rangesMinMax;
            for (const auto& graph : graphs)
                {
                const auto minMaxVals = graph->GetLeftYAxis().GetRange();
                if (rangesMinMax.has_value())
                    {
                    rangesMinMax.value().first = std::min(minMaxVals.first,
                                                          rangesMinMax.value().first);
                    rangesMinMax.value().second = std::max(minMaxVals.second,
                                                           rangesMinMax.value().second);
                    }
                else
                    { rangesMinMax = minMaxVals; }
                }
            for (auto& graph : graphs)
                {
                graph->GetLeftYAxis().SetRange(
                    rangesMinMax.value().first, rangesMinMax.value().second,
                    graph->GetLeftYAxis().GetPrecision());
                }
            }

        return commonAxis;
        }
    }
