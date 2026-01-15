///////////////////////////////////////////////////////////////////////////////
// Name:        commonaxisbuilder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
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

        if (canvas == nullptr || graphs.size() < 2)
            {
            return nullptr;
            }

        std::vector<std::shared_ptr<Graphs::Graph2D>> valid;
        valid.reserve(graphs.size());
        for (const auto& graph : graphs)
            {
            if (graph == nullptr)
                {
                continue;
                }
            valid.push_back(graph);
            }

        // need at least two usable graphs
        if (valid.size() < 2)
            {
            return nullptr;
            }

        // determine the full Y range across all graphs
        GraphItems::Axis axisTemplate{ valid.front()->GetLeftYAxis() };

        double minStart = axisTemplate.GetRange().first;
        double maxEnd = axisTemplate.GetRange().second;

        // find true global min start and max end
        for (const auto& graph : valid)
            {
            const auto range = graph->GetLeftYAxis().GetRange();
            minStart = std::min(minStart, range.first);
            maxEnd = std::max(maxEnd, range.second);
            }

        // pick the style from whichever graph extends farthest
        for (const auto& graph : valid)
            {
            if (graph->GetLeftYAxis().GetRange().second > axisTemplate.GetRange().second)
                {
                axisTemplate.CopySettings(graph->GetLeftYAxis());
                }
            }

        // enforce unified numeric range on the style template
        axisTemplate.SetRange(minStart, maxEnd, axisTemplate.GetPrecision());

        for (const auto& graph : valid)
            {
            // copy the left axis range from the tallest plot to this one,
            // then turn off the labels
            graph->GetLeftYAxis().CopySettings(axisTemplate);
            graph->GetLeftYAxis().SetRange(minStart, maxEnd, axisTemplate.GetPrecision());
            graph->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetLeftYAxis().GetTitle().Show(false);
            // turn off right too
            graph->GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetRightYAxis().GetTitle().Show(false);
            }

        // create a common axis, also copied from the tallest plot's left axis
        auto commonAxis = std::make_unique<GraphItems::Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(axisTemplate);
        // tell the canvas to align the axis line to the left side of its bounding box
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

        if (canvas == nullptr || graphs.size() < 2)
            {
            return nullptr;
            }

        std::vector<std::shared_ptr<Graphs::Graph2D>> valid;
        valid.reserve(graphs.size());
        for (const auto& graph : graphs)
            {
            if (graph == nullptr)
                {
                continue;
                }
            valid.push_back(graph);
            }

        // need at least two usable graphs
        if (valid.size() < 2)
            {
            return nullptr;
            }

        // determine the full X range across all graphs
        GraphItems::Axis axisTemplate{ valid.front()->GetBottomXAxis() };

        double minStart = axisTemplate.GetRange().first;
        double maxEnd = axisTemplate.GetRange().second;

        for (const auto& graph : valid)
            {
            const auto range = graph->GetBottomXAxis().GetRange();
            minStart = std::min(minStart, range.first);
            maxEnd = std::max(maxEnd, range.second);
            }

        for (const auto& graph : valid)
            {
            if (graph->GetBottomXAxis().GetRange().second > axisTemplate.GetRange().second)
                {
                axisTemplate.CopySettings(graph->GetBottomXAxis());
                }
            }

        axisTemplate.SetRange(minStart, maxEnd, axisTemplate.GetPrecision());

        // apply to each graph; copy the bottom axis range from the widest plot,
        // then turn off labels
        for (const auto& graph : valid)
            {
            graph->GetBottomXAxis().CopySettings(axisTemplate);
            graph->GetBottomXAxis().SetRange(minStart, maxEnd, axisTemplate.GetPrecision());
            graph->GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetBottomXAxis().GetTitle().Show(false);
            // turn off top too
            graph->GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            graph->GetTopXAxis().GetTitle().Show(false);
            }

        // create a common axis, also copied from the widest plot's bottom axis
        auto commonAxis = std::make_unique<GraphItems::Axis>(axisType);
        commonAxis->SetDPIScaleFactor(canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(axisTemplate);
        // anchor the axis line's top to the bottom side of its bounding box
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
            for (const auto& graph : graphs)
                {
                graph->GetLeftYAxis().SetRange(rangesMinMax.value().first,
                                               rangesMinMax.value().second,
                                               graph->GetLeftYAxis().GetPrecision());
                }
            }

        return commonAxis;
        }
    } // namespace Wisteria
