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
        CommonAxisBuilder::BuildYAxis(Canvas* canvas,
            std::vector<std::shared_ptr<Graphs::Graph2D>> graphs,
            AxisType axisType)
        {
        wxASSERT_MSG(axisType == AxisType::LeftYAxis || axisType == AxisType::RightYAxis,
            L"BuildYAxis() requires a left or right axis type to be specified!");
        // fix bogus axis type
        if (axisType != AxisType::LeftYAxis && axisType != AxisType::RightYAxis)
            { axisType = AxisType::LeftYAxis; }

        if (graphs.size() < 2)
            { return nullptr; }
        for (auto graphIter = graphs.begin() + 1; graphIter < graphs.end(); ++graphIter)
            {
            // copy the left axis range from the first plot to this one,
            // then turn off the labels
            graphIter->get()->GetLeftYAxis().CopySettings(graphs.begin()->get()->GetLeftYAxis());
            graphIter->get()->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graphIter->get()->GetLeftYAxis().GetTitle().Show(false);
            // turn off right too
            graphIter->get()->GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graphIter->get()->GetRightYAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the first plot's left axis
        auto commonAxis = std::make_shared<Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetLeftYAxis());
        commonAxis->GetTitle() = graphs.begin()->get()->GetLeftYAxis().GetTitle();
        // tell the canvas to align the axis line to the left side of its
        // bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(0, 0, 0, 10);
        // Get the canvas size of the axis and add it to the canvas.
        commonAxis->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(commonAxis));
        commonAxis->SetFixedWidthOnCanvas(true);

        // now that we are done copying the left axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetLeftYAxis().GetTitle().Show(false);
        graphs.begin()->get()->GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetRightYAxis().GetTitle().Show(false);

        // tell the canvas to align the plots and stand-alone axes across each row
        canvas->AlignRowContent(true);

        return commonAxis;
        }

    //----------------------------------------------------------------
    std::shared_ptr<Axis>
        CommonAxisBuilder::BuildXAxis(Canvas* canvas,
            std::vector<std::shared_ptr<Graphs::Graph2D>> graphs,
            AxisType axisType,
            const bool useCommonLeftAxis /*= false*/)
        {
        wxASSERT_MSG(axisType == AxisType::BottomXAxis || axisType == AxisType::TopXAxis,
            L"BuildXAxis() requires a bottom or top axis type to be specified!");
        // fix bogus axis type
        if (axisType != AxisType::BottomXAxis && axisType != AxisType::TopXAxis)
            { axisType = AxisType::BottomXAxis; }

        if (graphs.size() < 2)
            { return nullptr; }
        for (auto graphIter = graphs.begin() + 1; graphIter < graphs.end(); ++graphIter)
            {
            // copy the bottom axis range from the first plot to this one,
            // then turn off the labels
            graphIter->get()->GetBottomXAxis().CopySettings(graphs.begin()->get()->GetBottomXAxis());
            graphIter->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graphIter->get()->GetBottomXAxis().GetTitle().Show(false);
            // turn off top too
            graphIter->get()->GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graphIter->get()->GetTopXAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the first plot's bottom axis
        auto commonAxis = std::make_shared<Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(graphs.begin()->get()->GetBottomXAxis());
        commonAxis->GetTitle() = graphs.begin()->get()->GetBottomXAxis().GetTitle();
        // tell the canvas to align the axis line to the bottom side of its bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        commonAxis->SetCanvasMargins(10, 0, 5, 0);
        // get the canvas size of the axis and add it to the canvas
        commonAxis->SetCanvasHeightProportion(canvas->CalcMinHeightProportion(commonAxis));
        commonAxis->FitCanvasRowHeightToContent(true);

        // now that we are done copying the bottom axis from the first plot,
        // hide the first plot's axis labels
        graphs.begin()->get()->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetBottomXAxis().GetTitle().Show(false);
        graphs.begin()->get()->GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        graphs.begin()->get()->GetTopXAxis().GetTitle().Show(false);

        // tell the canvas to align the plots and stand-alone axes across each column
        canvas->AlignColumnContent(true);

        if (useCommonLeftAxis)
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
