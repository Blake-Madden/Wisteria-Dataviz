///////////////////////////////////////////////////////////////////////////////
// Name:        plot2d.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "graph2d.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void Graph2D::AddReferenceLinesAndAreasToLegend(std::shared_ptr<GraphItems::Label>& legend) const
        {
        if (GetReferenceLines().empty() && GetReferenceAreas().empty())
            { return; }

        legend->GetLegendIcons().push_back(
            LegendIcon(IconShape::HorizontalSeparator, wxPen(*wxBLACK, 2), wxNullColour));
        wxString textLines;

        // combine lines with the same color and label
        std::vector<ReferenceLine> refLines{ GetReferenceLines() };
        std::sort(refLines.begin(), refLines.end(),
                [](const auto& left, const auto& right) noexcept
                {
                return (left.m_compKey.CmpNoCase(right.m_compKey) < 0);
                });
        refLines.erase(
            std::unique(refLines.begin(), refLines.end(),
                [](const auto& left, const auto& right) noexcept
                {
                return (left.m_label.CmpNoCase(right.m_label) == 0 &&
                        left.m_lineColor == right.m_lineColor);
                }),
            refLines.end());
        // resort by axis position and add to the legend
        std::sort(refLines.begin(), refLines.end());
        for (const auto& refLine : refLines)
            {
            textLines += refLine.m_label + L"\n";
            legend->GetLegendIcons().push_back(
                LegendIcon(IconShape::HorizontalLineIcon,
                           wxPen(refLine.m_lineColor, 2, refLine.m_linePenStyle),
                           ColorContrast::ChangeOpacity(refLine.m_lineColor,
                                                        Settings::GetTranslucencyValue())));
            }

        // combine areas with the same color and label
        std::vector<ReferenceArea> refAreas{ GetReferenceAreas() };
        std::sort(refAreas.begin(), refAreas.end(),
            [](const auto& left, const auto& right) noexcept
            {
            return (left.m_compKey.CmpNoCase(right.m_compKey) < 0);
            });
        refAreas.erase(
            std::unique(refAreas.begin(), refAreas.end(),
                [](const auto& left, const auto& right) noexcept
                {
                return (left.m_label.CmpNoCase(right.m_label) == 0 &&
                        left.m_lineColor == right.m_lineColor);
                }),
            refAreas.end());
        // resort by axis position and add to the legend
        std::sort(refAreas.begin(), refAreas.end());
        for (const auto& refArea : refAreas)
            {
            textLines += refArea.m_label + L"\n";
            legend->GetLegendIcons().push_back(
                LegendIcon(IconShape::SquareIcon,
                           wxPen(refArea.m_lineColor, 2, refArea.m_linePenStyle),
                           ColorContrast::ChangeOpacity(refArea.m_lineColor,
                                                        Settings::GetTranslucencyValue())));
            }
        legend->SetText(legend->GetText() + L"\n \n" + textLines.Trim());
        }

    //----------------------------------------------------------------
    void Graph2D::AdjustLegendSettings(std::shared_ptr<GraphItems::Label>& legend,
                                       const LegendCanvasPlacementHint hint)
        {
        legend->SetBoxCorners(BoxCorners::Rounded);
        if (hint == LegendCanvasPlacementHint::EmbeddedOnGraph)
            {
            legend->GetGraphItemInfo().Pen(*wxBLACK_PEN).
                Padding(4, 4, 4, (legend->HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 4)).
                FontBackgroundColor(*wxWHITE);
            legend->GetFont().MakeSmaller();
            legend->GetHeaderInfo().GetFont().MakeSmaller();
            }
        else if (hint == LegendCanvasPlacementHint::LeftOfGraph)
            {
            legend->SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(legend));
            legend->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
            legend->AdjustingBoundingBoxToContent(true);
            legend->GetGraphItemInfo().Pen(wxNullPen).
                Padding(0, 0, 0, (legend->HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0)).
                CanvasPadding(4, 4, 4, 4).
                FitContentWidthToCanvas(true);
            legend->GetFont().MakeSmaller();
            legend->GetHeaderInfo().GetFont().MakeSmaller();
            }
        else if (hint == LegendCanvasPlacementHint::RightOfGraph)
            {
            legend->SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(legend));
            legend->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
            legend->AdjustingBoundingBoxToContent(true);
            legend->GetGraphItemInfo().Pen(wxNullPen).
                Padding(0, 0, 0, (legend->HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0)).
                CanvasPadding(4, 4, 4, 4).
                FitContentWidthToCanvas(true);
            legend->GetFont().MakeSmaller();
            legend->GetHeaderInfo().GetFont().MakeSmaller();
            }
        // don't make font smaller since canvases' aspect ratio makes it so that making it
        // taller won't increase the height of the area as much as the width if the legend
        // was off to the right of the graph
        else if (hint == LegendCanvasPlacementHint::AboveOrBeneathGraph)
            {
            legend->AdjustingBoundingBoxToContent(true);
            legend->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
            legend->GetGraphItemInfo().Pen(wxNullPen).
                Padding(0, 0, 0, (legend->HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0)).
                CanvasPadding(4, 4, 4, 4).
                FitCanvasHeightToContent(true);
            }
        }

    //----------------------------------------------------------------
    Graph2D::Graph2D(Canvas* canvas)
        {
        wxASSERT_MSG(canvas, L"Cannot use a null canvas with a plot!");
        SetDPIScaleFactor(canvas ? canvas->GetDPIScaleFactor() : 1);
        SetCanvas(canvas);
 
        GetTitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);

        // Use smaller fonts for the subtitle and caption by default.
        // Normally, scaling is what controls the font sizes, but these objects
        // have their scaling set to the parents on RecalcAllSizes().
        // This way, the client can change the font sizes of these items
        // if they want without having to deal with scaling.
        GetSubtitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetSubtitle().GetFont().SetFractionalPointSize(
            GetTitle().GetFont().GetFractionalPointSize() * .75);

        GetCaption().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetCaption().GetFont().SetFractionalPointSize(
            GetTitle().GetFont().GetFractionalPointSize() * .75);
        GetCaption().SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::DimGray));
        }

    //----------------------------------------------------------------
    void Graph2D::SetDPIScaleFactor(const double scaling)
        {
        GraphItemBase::SetDPIScaleFactor(scaling);
        // set axes' DPI information
        GetLeftYAxis().SetDPIScaleFactor(scaling);
        GetRightYAxis().SetDPIScaleFactor(scaling);
        GetBottomXAxis().SetDPIScaleFactor(scaling);
        GetTopXAxis().SetDPIScaleFactor(scaling);
        for (auto& customAxis : GetCustomAxes())
            { customAxis.SetDPIScaleFactor(scaling); }

        GetTitle().SetDPIScaleFactor(scaling);
        GetSubtitle().SetDPIScaleFactor(scaling);
        GetCaption().SetDPIScaleFactor(scaling);

        for (auto& object : m_plotObjects)
            { object->SetDPIScaleFactor(scaling); }
        for (auto& object : m_embeddedObjects)
            {
            if (object.m_object != nullptr)
                { object.m_object->SetDPIScaleFactor(scaling); }
            }
        }

    //----------------------------------------------------------------
    void Graph2D::GetAxesOverhang(long& leftMargin, long& rightMargin, long& topMargin,
                                  long& bottomMargin, wxDC& dc) const
        {
        leftMargin = rightMargin = topMargin = bottomMargin = 0;
        std::vector<long> topMarginVals, bottomMarginVals, leftMarginVals, rightMarginVals;

        const auto addGutterDifferences = [this, &topMarginVals, &bottomMarginVals,
                                           &leftMarginVals, &rightMarginVals]
            (const wxRect gutter)
            {
            topMarginVals.emplace_back(GetLeftYAxis().GetTopPoint().y - gutter.GetTop());
            bottomMarginVals.emplace_back(gutter.GetBottom() - GetLeftYAxis().GetBottomPoint().y);

            leftMarginVals.emplace_back(GetBottomXAxis().GetLeftPoint().x - gutter.GetLeft());
            rightMarginVals.emplace_back(gutter.GetRight() - GetBottomXAxis().GetRightPoint().x);
            };

        addGutterDifferences(GetLeftYAxis().GetBoundingBox(dc));
        addGutterDifferences(GetRightYAxis().GetBoundingBox(dc));
        addGutterDifferences(GetBottomXAxis().GetBoundingBox(dc));
        addGutterDifferences(GetTopXAxis().GetBoundingBox(dc));

        // Adjust for any custom axes also.
        // Note that we are only interested in how much the custom axes overhang the main.
        for (const auto& customAxis : GetCustomAxes())
            { addGutterDifferences(customAxis.GetBoundingBox(dc)); }

        topMargin = *std::max_element(topMarginVals.cbegin(), topMarginVals.cend());
        bottomMargin = *std::max_element(bottomMarginVals.cbegin(), bottomMarginVals.cend());
        leftMargin = *std::max_element(leftMarginVals.cbegin(), leftMarginVals.cend());
        rightMargin = *std::max_element(rightMarginVals.cbegin(), rightMarginVals.cend());
        }

    //----------------------------------------------------------------
    void Graph2D::DrawSelectionLabel(wxDC& dc, [[maybe_unused]] const double scaling,
                                     [[maybe_unused]] const wxRect boundingBox) const
        {
        for (const auto& object : m_plotObjects)
            { object->DrawSelectionLabel(dc, GetScaling(), GetPlotAreaBoundingBox()); }
        for (const auto& object : m_embeddedObjects)
            { object.m_object->DrawSelectionLabel(dc, GetScaling(), GetPlotAreaBoundingBox()); }
        }

    //----------------------------------------------------------------
    void Graph2D::AdjustPlotArea(wxDC& dc)
        {
        // sets the physical points for the axes
        const auto adjustAxesPoints = [&dc, this]()
            {
            GetBottomXAxis().SetPoints(GetPlotAreaBoundingBox().GetLeftBottom(),
                                       GetPlotAreaBoundingBox().GetRightBottom(), dc);
            GetTopXAxis().SetPoints(GetPlotAreaBoundingBox().GetTopLeft(),
                                    GetPlotAreaBoundingBox().GetTopRight(), dc);
            GetLeftYAxis().SetPoints(GetPlotAreaBoundingBox().GetTopLeft(),
                                     GetPlotAreaBoundingBox().GetLeftBottom(), dc);
            GetRightYAxis().SetPoints(GetPlotAreaBoundingBox().GetRightTop(),
                                      GetPlotAreaBoundingBox().GetRightBottom(), dc);
            wxCoord yStartCoordinate(0), yEndCoordinate(0), xStartCoordinate(0), xEndCoordinate(0);
            const auto [rangeYStart, rangeYEnd] = GetLeftYAxis().GetRange();
            const auto [rangeXStart, rangeXEnd] = GetBottomXAxis().GetRange();
            if (GetLeftYAxis().GetPhysicalCoordinate(rangeYStart, yStartCoordinate) &&
                GetLeftYAxis().GetPhysicalCoordinate(rangeYEnd, yEndCoordinate) &&
                GetBottomXAxis().GetPhysicalCoordinate(rangeXStart, xStartCoordinate) &&
                GetBottomXAxis().GetPhysicalCoordinate(rangeXEnd, xEndCoordinate))
                {
                for (auto& customAxis : GetCustomAxes())
                    {
                    wxCoord x(0), y(0);
                    if (GetBottomXAxis().GetPhysicalCoordinate(customAxis.GetCustomXPosition(), x) &&
                        GetLeftYAxis().GetPhysicalCoordinate(customAxis.GetCustomYPosition(), y))
                        {
                        if (customAxis.IsVertical())
                            {
                            customAxis.SetPhysicalCustomXPosition(x);
                            customAxis.SetPhysicalCustomYPosition(y);
                            wxCoord yStartCoordinateOffsetted{ 0 };
                            if (customAxis.GetPhysicalCustomYPosition() != -1 &&
                                GetLeftYAxis().GetPhysicalCoordinate(
                                    rangeYStart+customAxis.GetOffsetFromParentAxis(), yStartCoordinateOffsetted))
                                {
                                customAxis.SetPoints(
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(), customAxis.GetPhysicalCustomYPosition()),
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(), yStartCoordinateOffsetted),
                                    dc);
                                }
                            }
                        else
                            {
                            customAxis.SetPhysicalCustomXPosition(x);
                            customAxis.SetPhysicalCustomYPosition(y);
                            wxCoord xStartCoordinateOffsetted{0};
                            if (customAxis.GetPhysicalCustomXPosition() != -1 &&
                                GetBottomXAxis().GetPhysicalCoordinate(
                                    rangeXStart+customAxis.GetOffsetFromParentAxis(), xStartCoordinateOffsetted))
                                {
                                customAxis.SetPoints(
                                    wxPoint(xStartCoordinateOffsetted, customAxis.GetPhysicalCustomYPosition()),
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(), customAxis.GetPhysicalCustomYPosition()),
                                    dc);
                                }
                            }
                        }
                    }
                }
            };

        m_plotRect = GetBoundingBox(dc);
        // set the axes' points assuming the entire drawing area, then measure their overhangs
        adjustAxesPoints();

        long leftAxisOverhang(0), rightAxisOverhang(0), topAxisOverhang(0), bottomAxisOverhang(0);
        GetAxesOverhang(leftAxisOverhang, rightAxisOverhang, topAxisOverhang, bottomAxisOverhang, dc);

        m_calculatedLeftPadding = std::max<long>(leftAxisOverhang,
                                                 GetLeftYAxis().GetProtrudingBoundingBox(dc).GetWidth());
        m_calculatedRightPadding = std::max<long>(rightAxisOverhang,
                                                  GetRightYAxis().GetProtrudingBoundingBox(dc).GetWidth());
        m_calculatedBottomPadding = std::max<long>(bottomAxisOverhang,
                                                   GetBottomXAxis().GetProtrudingBoundingBox(dc).GetHeight());
        m_calculatedTopPadding = std::max<long>(topAxisOverhang,
                                                GetTopXAxis().GetProtrudingBoundingBox(dc).GetHeight());

        // shrink the plot area to fit so that the axes outer area fit in the drawing area
        m_plotRect.x += m_calculatedLeftPadding;
        m_plotRect.y += m_calculatedTopPadding;
        m_plotRect.SetWidth(m_plotRect.GetWidth()-(m_calculatedLeftPadding+m_calculatedRightPadding));
        m_plotRect.SetHeight(m_plotRect.GetHeight()-(m_calculatedTopPadding+m_calculatedBottomPadding));

        // make space for the titles
        if (GetTitle().GetText().length() && GetTitle().IsShown())
            {
            auto titleRect = GetTitle().GetBoundingBox(dc);
            // if too wide, shrink down its scaling
            if (titleRect.GetWidth() > GetBoundingBox(dc).GetWidth())
                {
                const auto rescaleFactor = safe_divide<double>(GetBoundingBox(dc).GetWidth(),
                    titleRect.GetWidth());
                GetTitle().SetScaling(GetTitle().GetScaling() * rescaleFactor);
                titleRect = GetTitle().GetBoundingBox(dc);
                }
            // if using a background color, stretch it out to the width of the graph area
            // so that it acts as a banner
            if (GetTitle().GetFontBackgroundColor().IsOk() &&
                GetTitle().GetFontBackgroundColor() != wxTransparentColor)
                {
                GetTitle().SetMinimumUserSizeDIPs(dc.ToDIP(m_rect.GetWidth()), std::nullopt);
                }

            m_plotRect.y += titleRect.GetHeight();
            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                 GetTitle().GetBoundingBox(dc).GetHeight());
            }
        if (GetSubtitle().GetText().length() && GetSubtitle().IsShown())
            {
            auto titleRect = GetSubtitle().GetBoundingBox(dc);
            // if too wide, shrink down its scaling
            if (titleRect.GetWidth() > GetBoundingBox(dc).GetWidth())
                {
                const auto rescaleFactor = safe_divide<double>(GetBoundingBox(dc).GetWidth(),
                    titleRect.GetWidth());
                GetSubtitle().SetScaling(GetSubtitle().GetScaling() * rescaleFactor);
                titleRect = GetSubtitle().GetBoundingBox(dc);
                }
            if (GetSubtitle().GetFontBackgroundColor().IsOk() &&
                GetSubtitle().GetFontBackgroundColor() != wxTransparentColor)
                {
                GetSubtitle().SetMinimumUserSizeDIPs(dc.ToDIP(m_rect.GetWidth()), std::nullopt);
                }

            m_plotRect.y += titleRect.GetHeight();
            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                 GetSubtitle().GetBoundingBox(dc).GetHeight());
            }
        // if both titles, then we need a space above and below them and one between.
        // if only one of the titles, then just a space above and below it.
        if ((GetTitle().GetText().length() && GetTitle().IsShown()) ||
            (GetSubtitle().GetText().length() && GetSubtitle().IsShown()))
            {
            const auto lineSpacing = ScaleToScreenAndCanvas(GetCaption().GetLineSpacing() *
                ((GetTitle().GetText().length() && GetSubtitle().GetText().length()) ? 3 : 2));
            m_plotRect.y += lineSpacing;
            m_plotRect.SetHeight(m_plotRect.GetHeight() - lineSpacing);
            }
        // and caption at the bottom
        if (GetCaption().GetText().length() && GetCaption().IsShown())
            {
            auto titleRect = GetCaption().GetBoundingBox(dc);
            // if too wide, shrink down its scaling
            if (titleRect.GetWidth() > GetBoundingBox(dc).GetWidth())
                {
                const auto rescaleFactor = safe_divide<double>(GetBoundingBox(dc).GetWidth(),
                    titleRect.GetWidth());
                GetCaption().SetScaling(GetCaption().GetScaling() * rescaleFactor);
                titleRect = GetCaption().GetBoundingBox(dc);
                }
            if (GetCaption().GetFontBackgroundColor().IsOk() &&
                GetCaption().GetFontBackgroundColor() != wxTransparentColor)
                {
                GetCaption().SetMinimumUserSizeDIPs(dc.ToDIP(m_rect.GetWidth()), std::nullopt);
                }

            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                  (GetCaption().GetBoundingBox(dc).GetHeight()+
                                   (ScaleToScreenAndCanvas(GetCaption().GetLineSpacing()*2))));
            }

        if (GetContentTop())
            { m_plotRect.SetTop(GetContentTop().value()); }
        if (GetContentBottom())
            { m_plotRect.SetBottom(GetContentBottom().value()); }
        if (GetContentLeft())
            { m_plotRect.SetLeft(GetContentLeft().value()); }
        if (GetContentRight())
            { m_plotRect.SetRight(GetContentRight().value()); }

        // reset the axes' points to the updated plot area
        adjustAxesPoints();
        }

    //----------------------------------------------------------------
    void Graph2D::UpdateSelectedItems()
        {
        for (auto& object : m_plotObjects)
            {
            if (GetSelectedIds().find(object->GetId()) != GetSelectedIds().cend())
                {
                // if applicable, set the object's subitems' selections from before
                auto foundItem = m_selectedItemsWithSubitems.find(object->GetId());
                if (foundItem != m_selectedItemsWithSubitems.cend())
                    { object->GetSelectedIds() = foundItem->second; }
                // and reset its previous selection state
                object->SetSelected(true);
                }
            }
        }

    //----------------------------------------------------------------
    void Graph2D::RecalcSizes(wxDC& dc)
        {
        m_currentAssignedId = 0;
        m_plotObjects.clear();

        // If bounding box hasn't been set yet, then set it to the parent
        // canvas's size. This would only happen if trying to measure the graph
        // before the window has a size event or is presented.
        // The normal case for this is when a graph is being meaured for a canvas
        // to a specific content scaling (e.g., Table).
        if (GetBoundingBox(dc).IsEmpty())
            { SetBoundingBox(GetCanvas()->GetCanvasRect(dc), dc, GetScaling()); }

        SetDPIScaleFactor(GetDPIScaleFactor());

        GetTopXAxis().SetScaling(GetScaling());
        GetBottomXAxis().SetScaling(GetScaling());
        GetRightYAxis().SetScaling(GetScaling());
        GetLeftYAxis().SetScaling(GetScaling());
        GetTopXAxis().SetAxisLabelScaling(GetScaling());
        GetBottomXAxis().SetAxisLabelScaling(GetScaling());
        GetRightYAxis().SetAxisLabelScaling(GetScaling());
        GetLeftYAxis().SetAxisLabelScaling(GetScaling());
        for (auto& customAxis : GetCustomAxes())
            {
            customAxis.SetScaling(GetScaling());
            customAxis.SetAxisLabelScaling(GetScaling());
            }
        GetTitle().SetScaling(GetScaling());
        GetSubtitle().SetScaling(GetScaling());
        GetCaption().SetScaling(GetScaling());

        // update mirrored axes
        if (IsXAxisMirrored())
            { GetTopXAxis().CopySettings(GetBottomXAxis()); }
        if (IsYAxisMirrored())
            { GetRightYAxis().CopySettings(GetLeftYAxis()); }

        AdjustPlotArea(dc);

        const bool shouldStackLeftY = GetLeftYAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetLeftYAxis().IsShown() &&
            ((GetLeftYAxis().IsStackingLabels() && !shouldStackLeftY) ||
            (!GetLeftYAxis().IsStackingLabels() && shouldStackLeftY)) )
            { GetLeftYAxis().StackLabels(shouldStackLeftY); }

        const bool shouldStackRightY = GetRightYAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetRightYAxis().IsShown() &&
            ((GetRightYAxis().IsStackingLabels() && !shouldStackRightY) ||
            (!GetRightYAxis().IsStackingLabels() && shouldStackRightY)) )
            { GetRightYAxis().StackLabels(shouldStackRightY); }

        const bool shouldStackBottomX = GetBottomXAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetBottomXAxis().IsShown() &&
            ((GetBottomXAxis().IsStackingLabels() && !shouldStackBottomX) ||
            (!GetBottomXAxis().IsStackingLabels() && shouldStackBottomX)) )
            { GetBottomXAxis().StackLabels(shouldStackBottomX); }

        const bool shouldStackTopX = GetTopXAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetTopXAxis().IsShown() &&
            ((GetTopXAxis().IsStackingLabels() && !shouldStackTopX) ||
            (!GetTopXAxis().IsStackingLabels() && shouldStackTopX)) )
            { GetTopXAxis().StackLabels(shouldStackTopX); }

        // Use a consistent font scaling for the four main axes, using the smallest one.
        // Note that the fonts will only be made smaller (not larger) across the axes, so
        // no need to readjust the plot areas again.
        const double bottomXLabelScaling = GetBottomXAxis().CalcBestScalingToFitLabels(dc);
        const double topXLabelScaling = GetTopXAxis().CalcBestScalingToFitLabels(dc);
        const double leftYLabelScaling = GetLeftYAxis().CalcBestScalingToFitLabels(dc);
        const double rightYLabelScaling = GetRightYAxis().CalcBestScalingToFitLabels(dc);

        const double smallestLabelScaling = std::min({ bottomXLabelScaling, topXLabelScaling,
                                                       leftYLabelScaling, rightYLabelScaling });
        GetBottomXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetTopXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetLeftYAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetRightYAxis().SetAxisLabelScaling(smallestLabelScaling);

        // adjust plot margins again in case stacking or common axis
        // label scaling was changed
        AdjustPlotArea(dc);

        // fill in the plot area's color (if being used, by default it is transparent)
        if (GetBackgroundColor().IsOk() &&
            GetBackgroundColor().GetAlpha() != wxALPHA_TRANSPARENT)
            {
            wxPoint boxPoints[4]{ {0, 0} };
            GraphItems::Polygon::GetRectPoints(GetPlotAreaBoundingBox(), boxPoints);
            auto box = std::make_shared<GraphItems::Polygon>(
                                GraphItems::GraphItemInfo().Pen(*wxBLACK_PEN).
                                Brush(wxColour(GetBackgroundColor())).
                                Scaling(GetScaling()),
                                boxPoints, std::size(boxPoints));
            AddObject(box);
            }

        // draw the X axis grid lines
        if (GetBottomXAxis().IsShown() &&
            GetBottomXAxis().GetGridlinePen().IsOk() &&
            GetBottomXAxis().GetAxisPointsCount() > 2)
            {
            auto xAxisLines = std::make_shared<Wisteria::GraphItems::Lines>(GetBottomXAxis().GetGridlinePen(), GetScaling());
            for (auto pos = GetBottomXAxis().GetAxisPoints().cbegin()+1;
                pos != GetBottomXAxis().GetAxisPoints().cend()-1;
                ++pos)
                {
                xAxisLines->AddLine(wxPoint(static_cast<wxCoord>(pos->GetPhysicalCoordinate()), GetPlotAreaBoundingBox().GetY()),
                    wxPoint(static_cast<wxCoord>(pos->GetPhysicalCoordinate()),
                        (GetPlotAreaBoundingBox().GetY()+GetPlotAreaBoundingBox().GetHeight())));
                }
            AddObject(xAxisLines);
            }

        // draw the Y axis grid lines
        if (GetLeftYAxis().IsShown() &&
            GetLeftYAxis().GetGridlinePen().IsOk() &&
            GetLeftYAxis().GetAxisPointsCount() > 2)
            {
            auto yAxisLines = std::make_shared<Wisteria::GraphItems::Lines>(GetLeftYAxis().GetGridlinePen(), GetScaling());
            for (auto pos = GetLeftYAxis().GetAxisPoints().cbegin()+1;
                pos != GetLeftYAxis().GetAxisPoints().cend()-1;
                ++pos)
                {
                yAxisLines->AddLine(wxPoint(GetPlotAreaBoundingBox().GetX(),
                    static_cast<wxCoord>(pos->GetPhysicalCoordinate())),
                    wxPoint((GetPlotAreaBoundingBox().GetX()+GetPlotAreaBoundingBox().GetWidth()),
                        static_cast<wxCoord>(pos->GetPhysicalCoordinate())) );
                }
            AddObject(yAxisLines);
            }

        // draw the axes on the plot area (on top of the gridlines)
        // (AdjustPlotArea() will have already set the axes' points)
        AddObject(std::make_shared<Axis>(GetBottomXAxis()));
        AddObject(std::make_shared<Axis>(GetTopXAxis()));
        AddObject(std::make_shared<Axis>(GetLeftYAxis()));
        AddObject(std::make_shared<Axis>(GetRightYAxis()));

        // draw the title
        if (GetTitle().GetText().length())
            {
            auto title = std::make_shared<GraphItems::Label>(GetTitle());
            if (title->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                title->SetAnchoring(Anchoring::TopLeftCorner);
                auto topPt = GetBoundingBox(dc).GetTopLeft();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing());
                title->SetAnchorPoint(topPt);
                }
            else if (title->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                title->SetAnchoring(Anchoring::Center);
                auto topPt = GetBoundingBox(dc).GetLeftTop();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing()) +
                            safe_divide<double>(title->GetBoundingBox(dc).GetHeight(), 2);
                topPt.x += GetBoundingBox(dc).GetWidth()/2;
                title->SetAnchorPoint(topPt);
                }
            else if (title->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                title->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox(dc).GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing());
                title->SetAnchorPoint(topPt);
                }
            AddObject(title);
            }

        // draw the subtitle
        if (GetSubtitle().GetText().length())
            {
            const auto titleSpacing = (GetTitle().GetText().length() ?
                GetTitle().GetBoundingBox(dc).GetHeight() +
                ScaleToScreenAndCanvas(GetTitle().GetLineSpacing()) :
                0);
            auto subtitle = std::make_shared<GraphItems::Label>(GetSubtitle());
            if (subtitle->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                subtitle->SetAnchoring(Anchoring::TopLeftCorner);
                auto topPt = GetBoundingBox(dc).GetTopLeft();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing()) + titleSpacing;
                subtitle->SetAnchorPoint(topPt);
                }
            else if (subtitle->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                subtitle->SetAnchoring(Anchoring::Center);
                auto topPt = GetBoundingBox(dc).GetLeftTop();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing()) +
                    safe_divide<double>(subtitle->GetBoundingBox(dc).GetHeight(), 2) + titleSpacing;
                topPt.x += GetBoundingBox(dc).GetWidth()/2;
                subtitle->SetAnchorPoint(topPt);
                }
            else if (subtitle->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                subtitle->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox(dc).GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing())+titleSpacing;
                subtitle->SetAnchorPoint(topPt);
                }
            AddObject(subtitle);
            }

        // draw the caption
        if (GetCaption().GetText().length())
            {
            auto caption = std::make_shared<GraphItems::Label>(GetCaption());
            if (caption->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                caption->SetAnchoring(Anchoring::BottomLeftCorner);
                auto bottomPt = GetBoundingBox(dc).GetLeftBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing());
                caption->SetAnchorPoint(bottomPt);
                }
            else if (caption->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                caption->SetAnchoring(Anchoring::Center);
                auto bottomPt = GetBoundingBox(dc).GetLeftBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing()) +
                    safe_divide<double>(caption->GetBoundingBox(dc).GetHeight(), 2);
                bottomPt.x += GetBoundingBox(dc).GetWidth()/2;
                caption->SetAnchorPoint(bottomPt);
                }
            else if (caption->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                caption->SetAnchoring(Anchoring::BottomRightCorner);
                auto bottomPt = GetBoundingBox(dc).GetRightBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing());
                caption->SetAnchorPoint(bottomPt);
                }
            AddObject(caption);
            }

        // custom axes
        for (const auto& customAxis : GetCustomAxes())
            { AddObject(std::make_shared<Axis>(customAxis)); }

        // reference lines
        for (const auto& refLine : GetReferenceLines())
            {
            wxCoord axisCoord{ 0 };
            auto dividerLine = std::make_shared<GraphItems::Lines>(
                wxPen(refLine.m_lineColor, 2, refLine.m_linePenStyle), GetScaling());
            if (refLine.m_axisType == AxisType::LeftYAxis ||
                refLine.m_axisType == AxisType::RightYAxis)
                {
                const auto& parentAxis = (refLine.m_axisType == AxisType::LeftYAxis ?
                                          GetLeftYAxis() : GetRightYAxis());
                if (parentAxis.GetPhysicalCoordinate(refLine.m_axisPosition, axisCoord))
                    {
                    dividerLine->AddLine(
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord));
                    AddObject(dividerLine);
                    }
                }
            else if (refLine.m_axisType == AxisType::BottomXAxis ||
                     refLine.m_axisType == AxisType::TopXAxis)
                {
                const auto& parentAxis = (refLine.m_axisType == AxisType::BottomXAxis ?
                                          GetBottomXAxis() : GetTopXAxis());
                if (parentAxis.GetPhysicalCoordinate(refLine.m_axisPosition, axisCoord))
                    {
                    dividerLine->AddLine(
                        wxPoint(axisCoord, GetLeftYAxis().GetBottomPoint().y),
                        wxPoint(axisCoord, GetLeftYAxis().GetTopPoint().y));
                    AddObject(dividerLine);
                    }
                }
            }

        // reference areas
        for (const auto& refArea : GetReferenceAreas())
            {
            wxCoord axisCoord1{ 0 }, axisCoord2{ 0 };
            auto dividerLine1 = std::make_shared<GraphItems::Lines>(
                wxPen(refArea.m_lineColor, 2, refArea.m_linePenStyle), GetScaling());
            auto dividerLine2 = std::make_shared<GraphItems::Lines>(
                wxPen(refArea.m_lineColor, 2, refArea.m_linePenStyle), GetScaling());
            if (refArea.m_axisType == AxisType::LeftYAxis ||
                refArea.m_axisType == AxisType::RightYAxis)
                {
                const auto& parentAxis = (refArea.m_axisType == AxisType::LeftYAxis ?
                                          GetLeftYAxis() : GetRightYAxis());
                if (parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition, axisCoord1) &&
                    parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition2, axisCoord2))
                    {
                    const wxPoint boxPoints[4] =
                        {
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord1),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord1),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord2),
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord2)
                        };
                    AddObject(std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo().
                        Pen(wxNullPen).Brush(ColorContrast::ChangeOpacity(
                                             refArea.m_lineColor, Settings::GetTranslucencyValue())),
                        boxPoints, std::size(boxPoints)));

                    dividerLine1->AddLine(
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord1),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord1));
                    AddObject(dividerLine1);

                    dividerLine2->AddLine(
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord2),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord2));
                    AddObject(dividerLine2);
                    }
                }
            else if (refArea.m_axisType == AxisType::BottomXAxis ||
                refArea.m_axisType == AxisType::TopXAxis)
                {
                const auto& parentAxis = (refArea.m_axisType == AxisType::BottomXAxis ?
                                          GetBottomXAxis() : GetTopXAxis());
                if (parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition, axisCoord1) &&
                    parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition2, axisCoord2))
                    {
                    const wxPoint boxPoints[4] =
                        {
                        wxPoint(axisCoord1, GetLeftYAxis().GetBottomPoint().y),
                        wxPoint(axisCoord1, GetLeftYAxis().GetTopPoint().y),
                        wxPoint(axisCoord2, GetLeftYAxis().GetTopPoint().y),
                        wxPoint(axisCoord2, GetLeftYAxis().GetBottomPoint().y)
                        };
                    AddObject(std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo().
                        Pen(wxNullPen).Brush(ColorContrast::ChangeOpacity(
                                             refArea.m_lineColor, Settings::GetTranslucencyValue())),
                        boxPoints, std::size(boxPoints)));

                    dividerLine1->AddLine(
                        wxPoint(axisCoord1, GetLeftYAxis().GetBottomPoint().y),
                        wxPoint(axisCoord1, GetLeftYAxis().GetTopPoint().y));
                    AddObject(dividerLine1);

                    dividerLine2->AddLine(
                        wxPoint(axisCoord2, GetLeftYAxis().GetBottomPoint().y),
                        wxPoint(axisCoord2, GetLeftYAxis().GetTopPoint().y));
                    AddObject(dividerLine2);
                    }
                }
            }

        // embed client object once the axes's physical coordinates have been recalculated
        for (auto& object : m_embeddedObjects)
            {
            wxCoord x{0}, y{0};
            if (GetBottomXAxis().GetPhysicalCoordinate(object.m_anchorPt.x, x) &&
                GetLeftYAxis().GetPhysicalCoordinate(object.m_anchorPt.y, y))
                { object.m_object->SetAnchorPoint({x,y}); }
            object.m_object->SetScaling(GetScaling());
            }
        }

    //----------------------------------------------------------------
    wxRect Graph2D::Draw(wxDC& dc) const
        {
        // draw the plot objects
        for (const auto& object : m_plotObjects)
            { object->Draw(dc); }
        for (const auto& object : m_embeddedObjects)
            {
            for (const auto& interestPoint : object.m_interestPts)
                {
                wxPoint anchorPt, interestPt;
                if (GetBottomXAxis().GetPhysicalCoordinate(object.m_anchorPt.x, anchorPt.x) &&
                    GetLeftYAxis().GetPhysicalCoordinate(object.m_anchorPt.y, anchorPt.y) &&
                    GetBottomXAxis().GetPhysicalCoordinate(interestPoint.x, interestPt.x) &&
                    GetLeftYAxis().GetPhysicalCoordinate(interestPoint.y, interestPt.y))
                    {
                    Lines ln(wxPen(*wxBLACK, 2, wxPenStyle::wxPENSTYLE_SHORT_DASH), GetScaling());
                    ln.AddLine(anchorPt, interestPt);
                    ln.SetLineStyle(LineStyle::Arrows);
                    ln.SetDPIScaleFactor(GetDPIScaleFactor());
                    ln.Draw(dc);
                    }
                }
            object.m_object->Draw(dc);
            }
        // draw the outline
        if (IsSelected())
            {
            // regular outline
                {
                wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                wxPoint pts[5];
                GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), pts);
                pts[4] = pts[0]; // close the square
                dc.DrawLines(std::size(pts), pts);
                }
            // with higher-level debugging enabled, show a large amount of information
            // about the plot, including its axes' physical points, scaling, a graphical
            // ruler, etc.
            if (Settings::IsDebugFlagEnabled(DebugSettings::DrawInformationOnSelection))
                {
                // highlight horizontal axes
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                    wxDCBrushChanger bc(dc, wxBrush(*wxRED, wxBRUSHSTYLE_BDIAGONAL_HATCH));
                    dc.DrawRectangle(GetTopXAxis().GetBoundingBox(dc));
                    dc.DrawRectangle(GetBottomXAxis().GetBoundingBox(dc));
                    }
                // vertical axes
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                    wxDCBrushChanger bc(dc, wxBrush(*wxRED, wxBRUSHSTYLE_FDIAGONAL_HATCH));
                    dc.DrawRectangle(GetLeftYAxis().GetBoundingBox(dc));
                    dc.DrawRectangle(GetRightYAxis().GetBoundingBox(dc));
                    }
                // ruler along the top, showing a 100-pixel legend
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxBLUE, ScaleToScreenAndCanvas(4)));
                    dc.DrawLine(GetBoundingBox(dc).GetTopLeft(), GetBoundingBox(dc).GetTopRight());
                    // left-to-right
                    for (auto i = GetBoundingBox(dc).GetTopLeft().x; i < GetBoundingBox(dc).GetTopRight().x; i += 100)
                        {
                        dc.DrawLine(wxPoint(i, GetBoundingBox(dc).GetTop()),
                            wxPoint(i, GetBoundingBox(dc).GetTop()+ScaleToScreenAndCanvas(20)) );
                        }
                    // right-to-left
                    for (auto i = GetBoundingBox(dc).GetTopRight().x; i > GetBoundingBox(dc).GetTopLeft().x; i -= 100)
                        {
                        dc.DrawLine(wxPoint(i, GetBoundingBox(dc).GetTop()+ScaleToScreenAndCanvas(20)),
                            wxPoint(i, GetBoundingBox(dc).GetTop()+ScaleToScreenAndCanvas(40)) );
                        }
                    Label rulerLabel(GraphItemInfo(L"\u21E6 100 pixels").
                        AnchorPoint(wxPoint(GetBoundingBox(dc).GetTopRight().x - ScaleToScreenAndCanvas(5),
                                            GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(25))).
                        Anchoring(Anchoring::TopRightCorner).FontColor(*wxBLUE).
                        Pen(*wxBLUE_PEN).DPIScaling(GetDPIScaleFactor()).
                        FontBackgroundColor(*wxWHITE).Padding(2,2,2,2));
                    rulerLabel.SetMinimumUserSizeDIPs(90, std::nullopt);
                    rulerLabel.Draw(dc);
                    rulerLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    rulerLabel.SetText(L"100 pixels \u21E8");
                    rulerLabel.SetAnchorPoint((wxPoint(GetBoundingBox(dc).GetTopLeft().x + ScaleToScreenAndCanvas(5),
                        GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(5))) );
                    rulerLabel.Draw(dc);
                    }
                // ruler along the left, showing a 100-pixel legend
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxBLUE, ScaleToScreenAndCanvas(4)));
                    dc.DrawLine(GetBoundingBox(dc).GetTopLeft(), GetBoundingBox(dc).GetTopRight());
                    // top-to-bottom
                    for (auto i = GetBoundingBox(dc).GetTopLeft().y; i < GetBoundingBox(dc).GetBottomLeft().y; i += 100)
                        {
                        dc.DrawLine(wxPoint(GetBoundingBox(dc).GetLeft(), i),
                            wxPoint(GetBoundingBox(dc).GetLeft()+ScaleToScreenAndCanvas(20), i) );
                        }
                    }
                const auto bBox = GetBoundingBox(dc);
                Label infoLabel(GraphItemInfo(
                        wxString::Format(L"Scaling: %s\n"
                            "Vertical Axes Top (x, y): %d, %d\n"
                            "Vertical Axes Bottom (x, y): %d, %d\n"
                            "Horizontal Axes Left (x, y): %d, %d\n"
                            "Horizontal Axes Right (x, y): %d, %d\n"
                            "Bounding Box (x,y,width,height): %d, %d, %d, %d\n"
                            "Content Area (x,y,width,height): %d, %d, %d, %d\n"
                            "Plot Decoration Padding (t,r,b,l): %ld, %ld, %ld, %ld\n"
                            "%s",
                            wxNumberFormatter::ToString(GetScaling(), 1, wxNumberFormatter::Style::Style_NoTrailingZeroes),
                            GetLeftYAxis().GetTopPoint().x, GetLeftYAxis().GetTopPoint().y,
                            GetLeftYAxis().GetBottomPoint().x, GetLeftYAxis().GetBottomPoint().y,
                            GetBottomXAxis().GetLeftPoint().x, GetBottomXAxis().GetLeftPoint().y,
                            GetBottomXAxis().GetRightPoint().x, GetBottomXAxis().GetRightPoint().y,
                            bBox.x, bBox.y, bBox.width, bBox.height,
                            GetContentRect().GetX(), GetContentRect().GetY(),
                            GetContentRect().GetWidth(), GetContentRect().GetHeight(),
                            m_calculatedTopPadding, m_calculatedRightPadding,
                            m_calculatedBottomPadding, m_calculatedLeftPadding,
                            m_debugDrawInfoLabel)).
                        AnchorPoint(GetBoundingBox(dc).GetBottomRight()).
                        Anchoring(Anchoring::BottomRightCorner).FontColor(*wxBLUE).
                        Pen(*wxBLUE_PEN).DPIScaling(GetDPIScaleFactor()).
                        FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2));
                infoLabel.Draw(dc);
                }
            }
        return GetBoundingBox(dc);
        }

    //----------------------------------------------------------------
    bool Graph2D::SelectObjectAtPoint(const wxPoint& pt, wxDC& dc)
        {
        if (!IsSelectable())
            { return false; }
        // if CTRL isn't held down, then unselect everything
        if (!wxGetMouseState().ControlDown())
            {
            GetSelectedIds().clear();
            m_selectedItemsWithSubitems.clear();
            for (auto& plotObject : m_plotObjects)
                {
                plotObject->GetSelectedIds().clear();
                plotObject->SetSelected(false);
                }
            for (auto& plotObject : m_embeddedObjects)
                {
                plotObject.m_object->GetSelectedIds().clear();
                plotObject.m_object->SetSelected(false);
                }
            }
        // items are added to a plot FILO (i.e., painter's algorithm),
        // so go backwards so that we select the items on top

        // the embedded objects, added by client, that would be sitting
        // on top of everything else
        for (auto plotObject = m_embeddedObjects.rbegin();
             plotObject != m_embeddedObjects.rend();
             ++plotObject)
            {
            if ((*plotObject).m_object->IsSelectable() && (*plotObject).m_object->HitTest(pt, dc))
                {
                (*plotObject).m_object->SetSelected(!(*plotObject).m_object->IsSelected());
                return true;
                }
            }
        // the standard graph objects (addded via AddObject())
        for (auto plotObject = m_plotObjects.rbegin();
             plotObject != m_plotObjects.rend();
             ++plotObject)
            {
            if ((*plotObject)->IsSelectable() && (*plotObject)->HitTest(pt, dc))
                {
                // toggle selection (or if it has subitems, then set it to selected
                // and let it perform its own selection logic)
                (*plotObject)->SetSelected(
                    (*plotObject)->GetSelectedIds().size() ? true :
                    !(*plotObject)->IsSelected());
                // update list of selected items
                // (based on whether this is newly selected or just unselected)
                if ((*plotObject)->IsSelected())
                    {
                    GetSelectedIds().insert((*plotObject)->GetId());
                    // if object has subitems, then record that for when we
                    // need to reselect items after recreating managed objects
                    if ((*plotObject)->GetSelectedIds().size())
                        {
                        m_selectedItemsWithSubitems.insert_or_assign(
                            (*plotObject)->GetId(), (*plotObject)->GetSelectedIds());
                        }
                    }
                else
                    {
                    // update our selection info if the object (an possibly, its subobjects)
                    // were deselected
                    auto unselectedItem = GetSelectedIds().find((*plotObject)->GetId());
                    if (unselectedItem != GetSelectedIds().end())
                        { GetSelectedIds().erase(unselectedItem); }
                    auto unselectedItemWithSubitems = m_selectedItemsWithSubitems.find((*plotObject)->GetId());
                    if (unselectedItemWithSubitems != m_selectedItemsWithSubitems.end())
                        { m_selectedItemsWithSubitems.erase(unselectedItemWithSubitems); }
                    }
                return true;
                }
            }
        // no items selected, so see if we at least clicked inside of the plot area
        if (HitTest(pt, dc))
            {
            SetSelected(true);
            return true;
            }
        return false;
        }
    }
