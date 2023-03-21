///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "roadmap.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    Roadmap::Roadmap(Canvas* canvas) :
            Graph2D(canvas)
        {
        // Axes aren't actually shown, just used for placing the objects.
        // But client might want to add axis titles or brackets, so hide
        // the lines and labels explicitly instead of entirely hiding the axes.
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetBottomXAxis().GetAxisLinePen() = wxNullPen;
        GetBottomXAxis().GetGridlinePen() = wxNullPen;

        GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetTopXAxis().GetAxisLinePen() = wxNullPen;
        GetTopXAxis().GetGridlinePen() = wxNullPen;

        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().GetAxisLinePen() = wxNullPen;
        GetLeftYAxis().GetGridlinePen() = wxNullPen;

        GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetRightYAxis().GetAxisLinePen() = wxNullPen;
        GetRightYAxis().GetGridlinePen() = wxNullPen;

        GetBottomXAxis().SetRange(0, 100, 0, 1, 1);
        }

    
    //----------------------------------------------------------------
    void Roadmap::RecalcSizes(wxDC& dc)
        {
        GetLeftYAxis().SetRange(0, GetRoadStops().size() + 2, 0, 1, 1);

        Graph2D::RecalcSizes(dc);

        // trim space off of area for the road so that there is space
        // for the markers
        auto roadRange = GetBottomXAxis().GetRange();
        const auto axisSpaceForMarkers = (roadRange.second - roadRange.first) / 5;
        roadRange.first += axisSpaceForMarkers;
        roadRange.second -= axisSpaceForMarkers;

        // left (negative items) and right (positive) sides of the road
        const auto middleX =
            (GetBottomXAxis().GetRange().second - GetBottomXAxis().GetRange().first) / 2;
        const auto rightRoadRange = std::make_pair(middleX, roadRange.second);
        const auto leftRoadRange = std::make_pair(middleX, roadRange.first);

        // the scale for the location markers (in DIPs);
        // 4 is probably the best looking small points, and 20 is a large enough
        // while still being reasonable
        std::pair<double, double> pointSizesRange = { 4, 20 };

        wxCoord xPt{ 0 }, yPt{ 0 };
        std::vector<wxPoint> pts;
        std::vector<std::shared_ptr<Point2D>> locations;
        std::vector<std::shared_ptr<Label>> locationLabels;
        auto labelConnectionLines =
            std::make_shared<Lines>(
                wxPen(ColorBrewer::GetColor(Colors::Color::WarmGray), 1,
                      wxPenStyle::wxPENSTYLE_LONG_DASH),
                GetScaling());

        // start of the road (bottom)
        if (GetBottomXAxis().GetPhysicalCoordinate(middleX, xPt))
            { pts.push_back({ xPt, GetBoundingBox(dc).GetBottom() }); }

        // the curves in the road
        for (size_t i = 0; i < GetRoadStops().size(); ++i)
            {
            if (GetBottomXAxis().GetPhysicalCoordinate(
                    scale_within(std::abs(GetRoadStops()[i].GetValue()),
                                 std::make_pair(0.0, GetMagnitude()),
                                 (GetRoadStops()[i].GetValue() >= 0 ?
                                     rightRoadRange : leftRoadRange)), xPt) &&
                GetLeftYAxis().GetPhysicalCoordinate(i + 1, yPt))
                { pts.push_back({ xPt, yPt }); }

            // the location marker:
            auto pt = std::make_shared<Point2D>(
                GraphItemInfo().Brush((GetRoadStops()[i].GetValue() >= 0 ?
                    GetPositiveIcon().second :
                    GetNegativeIcon().second)).
                DPIScaling(GetDPIScaleFactor()).
                Scaling(GetScaling()).
                AnchorPoint({ xPt , yPt }),
                scale_within(std::abs(GetRoadStops()[i].GetValue()),
                             std::make_pair(0.0, GetMagnitude()), pointSizesRange),
                (GetRoadStops()[i].GetValue() >= 0 ?
                    GetPositiveIcon().first : GetNegativeIcon().first));
            locations.push_back(pt);

            const wxString markerText =
                (m_markerLabelDisplay == MarkerLabelDisplay::NameAndValue) ?
                     wxString::Format(L"%s (%s)",
                        GetRoadStops()[i].GetName(),
                            wxNumberFormatter::ToString(GetRoadStops()[i].GetValue(), 3,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes)) : 
                (m_markerLabelDisplay == MarkerLabelDisplay::NameAndAbsoluteValue) ?
                    wxString::Format(L"%s (%s)",
                        GetRoadStops()[i].GetName(),
                            wxNumberFormatter::ToString(std::abs(GetRoadStops()[i].GetValue()), 3,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                GetRoadStops()[i].GetName();

            auto markerLabel = std::make_shared<Label>(
                GraphItemInfo(GraphItemInfo(markerText).
                Scaling(GetScaling()).
                DPIScaling(GetDPIScaleFactor()).
                Pen(wxNullPen).
                FontBackgroundColor(*wxWHITE)) );
            markerLabel->ShowLabelWhenSelected(true);
            if (GetLabelPlacement() == LabelPlacement::NextToParent)
                {
                markerLabel->SetAnchorPoint((GetRoadStops()[i].GetValue() >= 0 ?
                    pt->GetBoundingBox(dc).GetBottomRight() :
                    pt->GetBoundingBox(dc).GetBottomLeft()));
                markerLabel->SetAnchoring((GetRoadStops()[i].GetValue() >= 0 ?
                    Anchoring::BottomLeftCorner : Anchoring::BottomRightCorner));
                }
            else
                {
                markerLabel->SetAnchorPoint((GetRoadStops()[i].GetValue() >= 0 ?
                    wxPoint(GetPlotAreaBoundingBox().GetRight(),
                            pt->GetBoundingBox(dc).GetBottomRight().y) :
                    wxPoint(GetPlotAreaBoundingBox().GetLeft(),
                            pt->GetBoundingBox(dc).GetBottomLeft().y)));
                markerLabel->SetAnchoring((GetRoadStops()[i].GetValue() >= 0 ?
                    Anchoring::BottomRightCorner : Anchoring::BottomLeftCorner));
                labelConnectionLines->AddLine(markerLabel->GetAnchorPoint(),
                                              pt->GetAnchorPoint());
                }
            markerLabel->GetFont().MakeSmaller();
            locationLabels.push_back(markerLabel);
            }

        // end of the road (top)
        if (GetBottomXAxis().GetPhysicalCoordinate(middleX, xPt))
            { pts.push_back({ xPt, GetBoundingBox(dc).GetTop() }); }

        // the road pavement
        wxASSERT_MSG(m_roadPen.IsOk(), L"Valid road pen needed to draw road!");
        wxPen scaledRoadPen = m_roadPen;
        scaledRoadPen.SetWidth(ScaleToScreenAndCanvas(scaledRoadPen.GetWidth()));
        auto pavement = std::make_shared<GraphItems::Points2D>(scaledRoadPen);
        pavement->SetDPIScaleFactor(GetDPIScaleFactor());
        pavement->GetClippingRect() = GetPlotAreaBoundingBox();
        pavement->SetLineStyle(LineStyle::Spline);
        for (const auto& pt : pts)
            {
            pavement->AddPoint(
                Point2D(GraphItemInfo().AnchorPoint({ pt.x, pt.y }).
                    DPIScaling(GetDPIScaleFactor()),
                    0, IconShape::Blank), dc);
            }
        AddObject(pavement);

        // the lane separator, which is a tenth as wide as the road
        if (GetLaneSeparatorStyle() != LaneSeparatorStyle::NoDisplay &&
            m_laneSeparatorPen.IsOk())
            {
            m_laneSeparatorPen.SetWidth(
                pavement->GetPen().GetWidth() /
                (m_laneSeparatorStyle == LaneSeparatorStyle::DoubleLine ? 5 : 10));
            auto laneSep = std::make_shared<GraphItems::Points2D>(m_laneSeparatorPen);
            laneSep->SetDPIScaleFactor(GetDPIScaleFactor());
            laneSep->GetClippingRect() = GetPlotAreaBoundingBox();
            laneSep->SetLineStyle(LineStyle::Spline);
            for (const auto& pt : pts)
                {
                laneSep->AddPoint(
                    Point2D(GraphItemInfo().
                        AnchorPoint(pt).
                        DPIScaling(GetDPIScaleFactor()),
                        0, IconShape::Blank), dc);
                }
            AddObject(laneSep);
            // if a double line, then draw a road colored line down the middle
            // of the lane separator to make it look like two lines
            if (m_laneSeparatorStyle == LaneSeparatorStyle::DoubleLine)
                {
                wxPen lineSepPen = wxPen(m_roadPen.GetColour(),
                                         m_laneSeparatorPen.GetWidth() * math_constants::third);
                auto laneSepRoadLine = std::make_shared<GraphItems::Points2D>(lineSepPen);
                laneSepRoadLine->SetDPIScaleFactor(GetDPIScaleFactor());
                laneSepRoadLine->GetClippingRect() = GetPlotAreaBoundingBox();
                laneSepRoadLine->SetLineStyle(LineStyle::Spline);
                for (const auto& pt : pts)
                    {
                    laneSepRoadLine->AddPoint(
                        Point2D(GraphItemInfo().
                            AnchorPoint(pt).
                            DPIScaling(GetDPIScaleFactor()),
                            0, IconShape::Blank), dc);
                    }
                AddObject(laneSepRoadLine);
                }
            }

        AddObject(labelConnectionLines);
        // adjust the labels to fit and make them use a uniform scale
        auto smallestLabelScaling = GetScaling();
        auto leftTextArea = GetPlotAreaBoundingBox();
        auto rightTextArea = GetPlotAreaBoundingBox();
        wxCoord coord{ 0 };
        if (GetBottomXAxis().GetPhysicalCoordinate(roadRange.first, coord))
            { leftTextArea.SetRight(coord); }
        if (GetBottomXAxis().GetPhysicalCoordinate(roadRange.second, coord))
            {
            rightTextArea.SetLeft(coord);
            rightTextArea.SetRight(GetPlotAreaBoundingBox().GetRight());
            }
        
        constexpr double smallestLabelScalingAllowable{ 0.5 };
        for (auto& locationLabel : locationLabels)
            {
            auto largerRect = (GetLabelPlacement() == LabelPlacement::NextToParent ?
                GetPlotAreaBoundingBox() :
                locationLabel->GetAnchoring() == Anchoring::BottomLeftCorner ?
                leftTextArea : rightTextArea);
            const auto bBox = locationLabel->GetBoundingBox(dc);
            if (!Polygon::IsRectInsideRect(bBox, largerRect) )
                {
                const double overhang = (bBox.GetLeft() < largerRect.GetLeft() ?
                    largerRect.GetLeft() - bBox.GetLeft() :
                    bBox.GetRight() - largerRect.GetRight());
                const auto inverseProportion = 1 - (safe_divide<double>(overhang, bBox.GetWidth()));
                locationLabel->SetScaling(locationLabel->GetScaling() *
                    // don't go any smaller than half scale
                    std::max(inverseProportion, smallestLabelScalingAllowable));
                }
            smallestLabelScaling = std::min(smallestLabelScaling, locationLabel->GetScaling());
            if (compare_doubles(smallestLabelScaling, smallestLabelScalingAllowable))
                { break;}
            }
        for (auto& locationLabel : locationLabels)
            {
            locationLabel->SetScaling(smallestLabelScaling);
            AddObject(locationLabel);
            }
        // add the location markers on top, going forward from the horizon to the starting point
        std::reverse(locations.begin(), locations.end());
        for (auto& location : locations)
            { AddObject(location); }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> Roadmap::CreateLegend(
        const LegendOptions& options)
        {
        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        wxString legendText = GetPositiveLegendLabel() + L"\n" + GetNegativeLegendLabel();
        legend->GetLegendIcons().emplace_back(
                LegendIcon(GetPositiveIcon().first, *wxBLACK,
                    GetPositiveIcon().second));
        legend->GetLegendIcons().emplace_back(
                LegendIcon(GetNegativeIcon().first, *wxBLACK,
                    GetNegativeIcon().second));

        if (options.IsIncludingHeader())
            {
            legendText.Prepend(_(L"Key\n"));
            legend->GetHeaderInfo().Enable(true).
                LabelAlignment(TextAlignment::Centered).GetFont().MakeBold().MakeLarger();
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, options.GetPlacementHint());
        return legend;
        }
    }
