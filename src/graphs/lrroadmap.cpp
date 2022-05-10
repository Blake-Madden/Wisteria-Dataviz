///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lrroadmap.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    LRRoadmap::LRRoadmap(Canvas* canvas) :
            Graph2D(canvas)
        {
        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void LRRoadmap::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& predictorColumnName,
        const wxString& coefficentColumnName,
        const std::optional<wxString>& pValueColumnName)
        {
        if (data == nullptr)
            { return; }

        auto predictorColumn = data->GetCategoricalColumn(predictorColumnName);
        if (predictorColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': IV name column not found for roadmap."),
                predictorColumnName).ToUTF8());
            }
        auto coefficientColumn = data->GetContinuousColumn(coefficentColumnName);
        if (coefficientColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': coefficient column not found for roadmap."),
                coefficentColumnName).ToUTF8());
            }
        auto pValueColumn = (pValueColumnName.has_value() ?
            data->GetContinuousColumn(pValueColumnName.value()) :
            data->GetContinuousColumns().cend());
        if (pValueColumnName && pValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': p-value column not found for roadmap."),
                pValueColumnName.value()).ToUTF8());
            }

        auto cRange = std::minmax_element(coefficientColumn->GetValues().cbegin(),
            coefficientColumn->GetValues().cend(),
            [](auto lh, auto rh)
            { return std::abs(lh) < std::abs(rh); });
        m_coefficientsRange.first = std::abs(*cRange.first);
        m_coefficientsRange.second = std::abs(*cRange.second);

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            m_roadStops.emplace_back(
                RoadStopInfo(predictorColumn->GetCategoryLabelFromID(predictorColumn->GetValue(i))).
                Coefficient(coefficientColumn->GetValue(i)));
            }

        GetBottomXAxis().SetRange(0, 100, 0, 1, 1);
        GetLeftYAxis().SetRange(0, m_roadStops.size()+2, 0, 1, 1);
        }

    //----------------------------------------------------------------
    void LRRoadmap::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        
        // trim space off of area for the road so that there is space
        // for the markers
        auto roadRange = GetBottomXAxis().GetRange();
        const auto axisSpaceForMarkers = (roadRange.second - roadRange.first) / 5;
        roadRange.first += axisSpaceForMarkers;
        roadRange.second -= axisSpaceForMarkers;

        // left (negative coefficients) and right (positive) sides of the road
        const auto middleX =
            (GetBottomXAxis().GetRange().second - GetBottomXAxis().GetRange().first) / 2;
        const auto rightRoadRange = std::make_pair(middleX, roadRange.second);
        const auto leftRoadRange = std::make_pair(middleX, roadRange.first);

        std::pair<double, double> pointSizesRange = { 8, 40 };

        wxCoord xPt{ 0 }, yPt{ 0 };
        std::vector<wxPoint> pts;
        std::vector<std::shared_ptr<Point2D>> locations;
        std::vector<std::shared_ptr<Label>> locationLabels;
        auto labelConnectionLines =
            std::make_shared<Lines>(
                wxPen(ColorBrewer::GetColor(Colors::Color::WarmGray), 1, wxPenStyle::wxPENSTYLE_LONG_DASH),
                      GetScaling());

        // start of the road (bottom)
        if (GetBottomXAxis().GetPhysicalCoordinate(middleX, xPt))
            { pts.push_back({ xPt, GetBoundingBox(dc).GetBottom() }); }

        // the curves in the road
        for (size_t i = 0; i < m_roadStops.size(); ++i)
            {
            if (GetBottomXAxis().GetPhysicalCoordinate(
                    scale_within(std::abs(m_roadStops[i].m_coefficent),
                                 m_coefficientsRange,
                                 (m_roadStops[i].m_coefficent >= 0 ?
                                     rightRoadRange : leftRoadRange)), xPt) &&
                GetLeftYAxis().GetPhysicalCoordinate(i + 1, yPt))
                { pts.push_back({ xPt, yPt }); }

            // the location marker:
            // points are scale of one, their point size is calculated instead
            auto pt = std::make_shared<Point2D>(
                GraphItemInfo().Brush((m_roadStops[i].m_coefficent >= 0 ? *wxGREEN : *wxRED_BRUSH)).
                DPIScaling(GetDPIScaleFactor()).
                AnchorPoint({ xPt , yPt }),
                scale_within(std::abs(m_roadStops[i].m_coefficent), m_coefficientsRange, pointSizesRange),
                IconShape::LocationMarker);
            locations.push_back(pt);

            auto textLabel = std::make_shared<Label>(
                GraphItemInfo(GraphItemInfo(wxString::Format(L"%s (%s)",
                    m_roadStops[i].m_name,
                    wxNumberFormatter::ToString(m_roadStops[i].m_coefficent, 3,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes)))).
                Padding(4, 4, 4, 4).
                Scaling(GetScaling()).
                DPIScaling(GetDPIScaleFactor()).
                Pen(ColorBrewer::GetColor(Colors::Color::WarmGray)).
                FontBackgroundColor(*wxWHITE));
            if (GetLabelPlacement() == LabelPlacement::NextToParent)
                {
                textLabel->SetAnchorPoint((m_roadStops[i].m_coefficent >= 0 ?
                    pt->GetBoundingBox(dc).GetBottomRight() :
                    pt->GetBoundingBox(dc).GetBottomLeft()));
                textLabel->SetAnchoring((m_roadStops[i].m_coefficent >= 0 ?
                    Anchoring::BottomLeftCorner : Anchoring::BottomRightCorner));
                textLabel->ShowLabelWhenSelected(true);
                }
            else
                {
                textLabel->SetAnchorPoint((m_roadStops[i].m_coefficent >= 0 ?
                    wxPoint(GetPlotAreaBoundingBox().GetRight(),
                            pt->GetBoundingBox(dc).GetBottomRight().y) :
                    wxPoint(GetPlotAreaBoundingBox().GetLeft(),
                            pt->GetBoundingBox(dc).GetBottomLeft().y)));
                textLabel->SetAnchoring((m_roadStops[i].m_coefficent >= 0 ?
                    Anchoring::BottomRightCorner : Anchoring::BottomLeftCorner));
                labelConnectionLines->AddLine(textLabel->GetAnchorPoint(),
                                              pt->GetAnchorPoint());
                }
            textLabel->GetFont().MakeSmaller();
            locationLabels.push_back(textLabel);
            }

        // end of the road (top)
        if (GetBottomXAxis().GetPhysicalCoordinate(middleX, xPt))
            { pts.push_back({ xPt, GetBoundingBox(dc).GetTop() }); }

        // the road pavement
        auto pavement = std::make_shared<GraphItems::Points2D>(
            wxPen(wxPenInfo(*wxBLACK, 40)));
        pavement->SetDPIScaleFactor(GetDPIScaleFactor());
        pavement->GetClippingRect() = GetPlotAreaBoundingBox();
        pavement->SetLineStyle(LineStyle::Spline);
        for (const auto& pt : pts)
            {
            pavement->AddPoint(
                Point2D(GraphItemInfo().AnchorPoint({ pt.x, pt.y }).
                    DPIScaling(GetDPIScaleFactor()),
                    0, IconShape::BlankIcon), dc);
            }
        AddObject(pavement);

        // the lane separator
        auto laneSep = std::make_shared<GraphItems::Points2D>(
            wxPen(wxPenInfo(*wxYELLOW, (pavement->GetPen().GetWidth() / 10),
                            wxPenStyle::wxPENSTYLE_LONG_DASH)));
        laneSep->SetDPIScaleFactor(GetDPIScaleFactor());
        laneSep->GetClippingRect() = GetPlotAreaBoundingBox();
        laneSep->SetLineStyle(LineStyle::Spline);
        for (const auto& pt : pts)
            {
            laneSep->AddPoint(
                Point2D(GraphItemInfo().AnchorPoint({ pt.x, pt.y }).
                    DPIScaling(GetDPIScaleFactor()),
                    0, IconShape::BlankIcon), dc);
            }
        AddObject(laneSep);

        AddObject(labelConnectionLines);
        // adjust the labels to fit and make them use a uniform scale
        auto smallestLabelScaling = GetScaling();
        for (auto& locationLabel : locationLabels)
            {
            auto bBox = locationLabel->GetBoundingBox(dc);
            while (!Polygon::IsRectInsideRect(bBox, GetPlotAreaBoundingBox()) &&
                   locationLabel->GetScaling() >= 0.02)
                {
                locationLabel->SetScaling(locationLabel->GetScaling() - .02);
                bBox = locationLabel->GetBoundingBox(dc);
                }
            smallestLabelScaling = std::min(smallestLabelScaling, locationLabel->GetScaling());
            }
        for (auto& locationLabel : locationLabels)
            {
            locationLabel->SetScaling(smallestLabelScaling);
            AddObject(locationLabel);
            }
        // add the location markers on top, going from the horizon to the starting point
        std::reverse(locations.begin(), locations.end());
        for (auto& location : locations)
            { AddObject(location); }
        }
    }
