///////////////////////////////////////////////////////////////////////////////
// Name:        commonaxisbuilder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "commonaxisbuilder.h"

namespace Wisteria
    {
    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Axis>
    CommonAxisBuilder::BuildYAxis(Canvas* canvas,
                                  const std::vector<std::shared_ptr<Graphs::Graph2D>>& graphs,
                                  AxisType axisType)
        {
        assert((axisType == AxisType::LeftYAxis || axisType == AxisType::RightYAxis) &&
               L"BuildYAxis() requires a left or right axis type to be specified!");
        // fix bogus axis type
        if (axisType != AxisType::LeftYAxis && axisType != AxisType::RightYAxis)
            {
            axisType = AxisType::LeftYAxis;
            }

        if (graphs.size() < 2)
            {
            return nullptr;
            }

        // see which plot has the largest range end and use that
        // (note that we will be assuming all plots are using the same range start [usually zero])
        GraphItems::Axis axisWithMaxRangeEnd{ graphs.cbegin()->get()->GetLeftYAxis() };
        for (const auto& graph : graphs)
            {
            if (graph->GetLeftYAxis().GetRange().second > axisWithMaxRangeEnd.GetRange().second)
                {
                axisWithMaxRangeEnd.CopySettings(graph->GetLeftYAxis());
                }
            }
        for (const auto& graph : graphs)
            {
            // copy the left axis range from the tallest plot to this one,
            // then turn off the labels
            graph->GetLeftYAxis().CopySettings(axisWithMaxRangeEnd);
            graph->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetLeftYAxis().GetTitle().Show(false);
            // turn off right too
            graph->GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetRightYAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the tallest plot's left axis
        auto commonAxis = std::make_unique<GraphItems::Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(axisWithMaxRangeEnd);
        // tell the canvas to align the axis line to the left side of its
        // bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        // Get the canvas size of the axis and add it to the canvas.
        commonAxis->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(*commonAxis));
        commonAxis->SetFixedWidthOnCanvas(true);

        // tell the canvas to align the plots and stand-alone axes across each row
        canvas->AlignRowContent(true);

        return commonAxis;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Axis>
    CommonAxisBuilder::BuildXAxis(Canvas* canvas,
                                  const std::vector<std::shared_ptr<Graphs::Graph2D>>& graphs,
                                  AxisType axisType, const bool useCommonLeftAxis /*= false*/)
        {
        assert((axisType == AxisType::BottomXAxis || axisType == AxisType::TopXAxis) &&
               L"BuildXAxis() requires a bottom or top axis type to be specified!");
        // fix bogus axis type
        if (axisType != AxisType::BottomXAxis && axisType != AxisType::TopXAxis)
            {
            axisType = AxisType::BottomXAxis;
            }

        if (graphs.size() < 2)
            {
            return nullptr;
            }

        // see which plot has the largest range end and use that
        // (note that we will be assuming all plots are using the same range start [usually zero])
        GraphItems::Axis axisWithMaxRangeEnd{ graphs.cbegin()->get()->GetBottomXAxis() };
        for (const auto& graph : graphs)
            {
            if (graph->GetBottomXAxis().GetRange().second > axisWithMaxRangeEnd.GetRange().second)
                {
                axisWithMaxRangeEnd.CopySettings(graph->GetBottomXAxis());
                }
            }
        for (const auto& graph : graphs)
            {
            // copy the bottom axis range from the widest plot to this one,
            // then turn off the labels
            graph->GetBottomXAxis().CopySettings(axisWithMaxRangeEnd);
            graph->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetBottomXAxis().GetTitle().Show(false);
            // turn off top too
            graph->GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetTopXAxis().GetTitle().Show(false);
            }
        // create a common axis, also copied from the widest plot's bottom axis
        auto commonAxis = std::make_unique<GraphItems::Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(axisWithMaxRangeEnd);
        // tell the canvas to align the axis line to the bottom side of its bounding box
        commonAxis->SetAnchoring(Anchoring::TopLeftCorner);
        // get the canvas size of the axis and add it to the canvas
        commonAxis->SetCanvasHeightProportion(canvas->CalcMinHeightProportion(*commonAxis));
        commonAxis->FitCanvasRowHeightToContent(true);

        // tell the canvas to align the plots and stand-alone axes down each column
        canvas->AlignColumnContent(true);

        if (useCommonLeftAxis)
            {
            std::optional<std::pair<double, double>> rangesMinMax;
            for (const auto& graph : graphs)
                {
                const auto minMaxVals = graph->GetLeftYAxis().GetRange();
                if (rangesMinMax.has_value())
                    {
                    rangesMinMax.value().first =
                        std::min(minMaxVals.first, rangesMinMax.value().first);
                    rangesMinMax.value().second =
                        std::max(minMaxVals.second, rangesMinMax.value().second);
                    }
                else
                    {
                    rangesMinMax = minMaxVals;
                    }
                }
            for (auto& graph : graphs)
                {
                graph->GetLeftYAxis().SetRange(rangesMinMax.value().first,
                                               rangesMinMax.value().second,
                                               graph->GetLeftYAxis().GetPrecision());
                }
            }

        return commonAxis;
        }
    } // namespace Wisteria
