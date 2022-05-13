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
        const std::optional<wxString>& pValueColumnName /*= std::nullopt*/,
        const std::optional<double> pLevel /*= std::nullopt*/,
        const std::optional<Predictors> preditorsToIncludes /*= std::nullopt*/,
        const std::optional<wxString> dvName /*= std::nullopt*/)
        {
        if (data == nullptr)
            { return; }

        if (dvName)
            { m_dvLabel = dvName.value(); }

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

        const auto includePredictor = [&](const double value, const std::optional<double> pValue)
            {
            if (std::isnan(value))
                { return false; }
            else if (pLevel.has_value() && !std::isnan(pLevel.value()) &&
                pValue.has_value() &&
                (std::isnan(pValue.value()) || pValue >= pLevel.value()) )
                { return false; }
            // default to all included
            else if (!preditorsToIncludes.has_value())
                { return true; }
            else if ((preditorsToIncludes.value() & Predictors::All) == Predictors::All)
                { return true; }
            else if ((preditorsToIncludes.value() & Predictors::Negative) == Predictors::Negative &&
                value < 0)
                { return true; }
            else if ((preditorsToIncludes.value() & Predictors::Neutral) == Predictors::Neutral &&
                value == 0)
                { return true; }
            else if ((preditorsToIncludes.value() & Predictors::Positive) == Predictors::Positive &&
                value > 0)
                { return true; }
            else
                { return false; }
            };

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (includePredictor(coefficientColumn->GetValue(i),
                                 (pValueColumn != data->GetContinuousColumns().cend() ?
                                  std::optional<double>(pValueColumn->GetValue(i)) : std::nullopt)) )
                {
                m_roadStops.emplace_back(
                    RoadStopInfo(predictorColumn->GetCategoryLabelFromID(predictorColumn->GetValue(i))).
                    Coefficient(coefficientColumn->GetValue(i)));
                }
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
                GraphItemInfo().Brush((m_roadStops[i].m_coefficent >= 0 ?
                    ColorBrewer::GetColor(Color::KellyGreen) :
                    ColorBrewer::GetColor(Color::Tomato))).
                DPIScaling(GetDPIScaleFactor()).
                AnchorPoint({ xPt , yPt }),
                scale_within(std::abs(m_roadStops[i].m_coefficent),
                             m_coefficientsRange, pointSizesRange),
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
                Pen(wxNullPen).
                FontBackgroundColor(*wxWHITE));
            textLabel->ShowLabelWhenSelected(true);
            if (GetLabelPlacement() == LabelPlacement::NextToParent)
                {
                textLabel->SetAnchorPoint((m_roadStops[i].m_coefficent >= 0 ?
                    pt->GetBoundingBox(dc).GetBottomRight() :
                    pt->GetBoundingBox(dc).GetBottomLeft()));
                textLabel->SetAnchoring((m_roadStops[i].m_coefficent >= 0 ?
                    Anchoring::BottomLeftCorner : Anchoring::BottomRightCorner));
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
                    0, IconShape::BlankIcon), dc);
            }
        AddObject(pavement);

        // the lane separator, which is a tenth as wide as the road
        auto laneSep = std::make_shared<GraphItems::Points2D>(
            wxPen(wxPenInfo(ColorBrewer::GetColor(Colors::Color::SchoolBusYellow),
                            (pavement->GetPen().GetWidth() / 10),
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
        // add the location markers on top, going from the horizon to the starting point
        std::reverse(locations.begin(), locations.end());
        for (auto& location : locations)
            { AddObject(location); }
        }

    //----------------------------------------------------------------
    void LRRoadmap::AddDefaultCaption()
        {
        GetCaption().SetText(wxString::Format(
            _("The larger the map marker and deeper the curve, "
              "the stronger the item's association with %s"), m_dvLabel));
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> LRRoadmap::CreateLegend(
        const LegendCanvasPlacementHint hint, const bool includeHeader)
        {
        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        wxString legendText = wxString::Format(_("Positively associated with %s"), m_dvLabel) + L"\n" +
                              wxString::Format(_("Negatively associated with %s"), m_dvLabel);
        legend->GetLegendIcons().emplace_back(
                LegendIcon(IconShape::LocationMarker, *wxBLACK, ColorBrewer::GetColor(Color::KellyGreen)));
        legend->GetLegendIcons().emplace_back(
            LegendIcon(IconShape::LocationMarker, *wxBLACK, ColorBrewer::GetColor(Color::Tomato)));

        if (includeHeader)
            {
            legendText.Prepend(_(L"Key\n"));
            legend->GetHeaderInfo().Enable(true).
                LabelAlignment(TextAlignment::Centered).GetFont().MakeBold().MakeLarger();
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
