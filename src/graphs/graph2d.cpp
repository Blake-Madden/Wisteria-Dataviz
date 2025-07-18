///////////////////////////////////////////////////////////////////////////////
// Name:        plot2d.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "graph2d.h"

wxIMPLEMENT_ABSTRACT_CLASS(Wisteria::Graphs::Graph2D, Wisteria::GraphItems::GraphItemBase);

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    // random number generator that can be used by the graph
    std::mt19937 Graph2D::m_mt{ std::random_device{}() };

    //----------------------------------------------------------------
    void Graph2D::ContrastColors()
        {
        GetLeftYAxis().ContrastAgainstColor(GetPlotOrCanvasColor());
        GetBottomXAxis().ContrastAgainstColor(GetPlotOrCanvasColor());
        GetRightYAxis().ContrastAgainstColor(GetPlotOrCanvasColor());
        GetTopXAxis().ContrastAgainstColor(GetPlotOrCanvasColor());

        const wxColour contrastingColor{ Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
            GetPlotOrCanvasColor()) };

        const auto adjustLabel = [this, &contrastingColor](Label& label)
        {
            // contrast a label if its font color (or background color, if in use)
            // is the same as the background
            if (label.GetFontBackgroundColor().IsOk() &&
                !label.GetFontBackgroundColor().IsTransparent())
                {
                if (label.GetFontBackgroundColor() == GetPlotOrCanvasColor())
                    {
                    label.SetFontBackgroundColor(contrastingColor);
                    }
                }
            else if (label.GetFontColor().IsOk() && !label.GetFontColor().IsTransparent() &&
                     label.GetFontColor() == GetPlotOrCanvasColor())
                {
                label.SetFontColor(contrastingColor);
                }

            if (label.GetHeaderInfo().IsEnabled() && label.GetHeaderInfo().GetFontColor().IsOk() &&
                !label.GetHeaderInfo().GetFontColor().IsTransparent() &&
                label.GetHeaderInfo().GetFontColor() == GetPlotOrCanvasColor() &&
                // if a font background color is valid, then don't adjust the font color
                !(label.GetFontBackgroundColor().IsOk() &&
                  !label.GetFontBackgroundColor().IsTransparent()))
                {
                label.GetHeaderInfo().FontColor(contrastingColor);
                }
        };

        adjustLabel(GetTitle());
        adjustLabel(GetSubtitle());
        adjustLabel(GetCaption());

        for (auto& customAxis : GetCustomAxes())
            {
            customAxis.ContrastAgainstColor(GetPlotOrCanvasColor());
            }
        }

    //----------------------------------------------------------------
    void Graph2D::AddReferenceLinesAndAreasToLegend(GraphItems::Label& legend) const
        {
        if (GetReferenceLines().empty() && GetReferenceAreas().empty())
            {
            return;
            }

        legend.GetLegendIcons().push_back(
            LegendIcon(IconShape::HorizontalSeparator, wxPen(*wxBLACK, 2), *wxTRANSPARENT_BRUSH));
        wxString textLines;

        // combine lines with the same color and label
        std::vector<ReferenceLine> refLines{ GetReferenceLines() };
        std::ranges::sort(refLines, [](const auto& left, const auto& right) noexcept
                          { return (left.m_compKey.CmpNoCase(right.m_compKey) < 0); });
        refLines.erase(std::ranges::unique(refLines,
                                           [](const auto& left, const auto& right) noexcept
                                           {
                                               return (left.m_label.CmpNoCase(right.m_label) == 0 &&
                                                       left.m_pen.GetColour() ==
                                                           right.m_pen.GetColour());
                                           })
                           .begin(),
                       refLines.end());
        // resort by axis position and add to the legend
        std::sort(refLines.begin(), refLines.end());
        for (const auto& refLine : refLines)
            {
            textLines += refLine.m_label + L"\n";
            legend.GetLegendIcons().push_back(
                LegendIcon(IconShape::HorizontalLine,
                           wxPen(refLine.m_pen.GetColour(), 2, refLine.m_pen.GetStyle()),
                           ColorContrast::ChangeOpacity(refLine.m_pen.GetColour(),
                                                        Settings::GetTranslucencyValue())));
            }

        // combine areas with the same color and label
        std::vector<ReferenceArea> refAreas{ GetReferenceAreas() };
        std::ranges::sort(refAreas, [](const auto& left, const auto& right) noexcept
                          { return (left.m_compKey.CmpNoCase(right.m_compKey) < 0); });
        refAreas.erase(std::ranges::unique(refAreas,
                                           [](const auto& left, const auto& right) noexcept
                                           {
                                               return (left.m_label.CmpNoCase(right.m_label) == 0 &&
                                                       left.m_pen.GetColour() ==
                                                           right.m_pen.GetColour());
                                           })
                           .begin(),
                       refAreas.end());
        // resort by axis position and add to the legend
        std::sort(refAreas.begin(), refAreas.end());
        for (const auto& refArea : refAreas)
            {
            textLines += refArea.m_label + L"\n";
            legend.GetLegendIcons().push_back(LegendIcon(
                IconShape::Square, wxPen{ refArea.m_pen.GetColour(), 2, refArea.m_pen.GetStyle() },
                wxBrush{ ColorContrast::ChangeOpacity(refArea.m_pen.GetColour(),
                                                      Settings::GetTranslucencyValue()) }));
            }
        legend.SetText(legend.GetText() + L"\n \n" + textLines.Trim());
        }

    //----------------------------------------------------------------
    void Graph2D::AdjustLegendSettings(GraphItems::Label& legend,
                                       const LegendCanvasPlacementHint hint)
        {
        if (GetCanvas() == nullptr)
            {
            wxLogWarning(L"Canvas for graph is null; legend will not be sized correctly.");
            return;
            }

        legend.SetBoxCorners(BoxCorners::Rounded);
        if (hint == LegendCanvasPlacementHint::EmbeddedOnGraph)
            {
            legend.GetGraphItemInfo()
                .Pen(*wxBLACK_PEN)
                .Padding(4, 4, 4, (legend.HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 4))
                .FontBackgroundColor(*wxWHITE);
            legend.GetFont().MakeSmaller();
            legend.GetHeaderInfo().GetFont().MakeSmaller();
            }
        else if (hint == LegendCanvasPlacementHint::LeftOfGraph)
            {
            legend.SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(legend));
            legend.SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
            legend.SetBoundingBoxToContentAdjustment(
                LabelBoundingBoxContentAdjustment::ContentAdjustWidth);
            legend.GetGraphItemInfo()
                .Pen(wxNullPen)
                .Padding(0, 0, 0, (legend.HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0))
                .CanvasPadding(4, 4, 4, 4)
                .FixedWidthOnCanvas(true);
            legend.GetFont().MakeSmaller();
            legend.GetHeaderInfo().GetFont().MakeSmaller();
            }
        else if (hint == LegendCanvasPlacementHint::RightOfGraph)
            {
            legend.SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(legend));
            legend.SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
            legend.SetBoundingBoxToContentAdjustment(
                LabelBoundingBoxContentAdjustment::ContentAdjustWidth);
            legend.GetGraphItemInfo()
                .Pen(wxNullPen)
                .Padding(0, 0, 0, (legend.HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0))
                .CanvasPadding(4, 4, 4, 4)
                .FixedWidthOnCanvas(true);
            legend.GetFont().MakeSmaller();
            legend.GetHeaderInfo().GetFont().MakeSmaller();
            }
        // don't make font smaller since canvases' aspect ratio makes it so that making it
        // taller won't increase the height of the area as much as the width if the legend
        // was off to the right of the graph
        else if (hint == LegendCanvasPlacementHint::AboveOrBeneathGraph)
            {
            legend.SetBoundingBoxToContentAdjustment(
                LabelBoundingBoxContentAdjustment::ContentAdjustWidth);
            legend.SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
            legend.GetGraphItemInfo()
                .Pen(wxNullPen)
                .Padding(0, 0, 0, (legend.HasLegendIcons() ? Label::GetMinLegendWidthDIPs() : 0))
                .CanvasPadding(4, 4, 4, 4)
                .FitCanvasHeightToContent(true);
            }
        }

    //----------------------------------------------------------------
    Graph2D::Graph2D(Canvas* canvas)
        {
        Graph2D::SetDPIScaleFactor(canvas != nullptr ? canvas->GetDPIScaleFactor() : 1);
        SetCanvas(canvas);

        GetTitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);

        // Use smaller fonts for the subtitle and caption by default.
        // Normally, scaling is what controls the font sizes, but these objects
        // have their scaling set to the parents on RecalcAllSizes().
        // This way, the client can change the font sizes of these items
        // if they want without having to deal with scaling.
        GetSubtitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetSubtitle().GetFont().SetFractionalPointSize(
            GetTitle().GetFont().GetFractionalPointSize() * math_constants::three_quarters);

        GetCaption().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetCaption().GetFont().SetFractionalPointSize(
            GetTitle().GetFont().GetFractionalPointSize() * math_constants::three_quarters);
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
            {
            customAxis.SetDPIScaleFactor(scaling);
            }

        GetTitle().SetDPIScaleFactor(scaling);
        GetSubtitle().SetDPIScaleFactor(scaling);
        GetCaption().SetDPIScaleFactor(scaling);

        for (auto& object : m_plotObjects)
            {
            object->SetDPIScaleFactor(scaling);
            }
        for (auto& object : m_embeddedObjects)
            {
            if (object.GetObject() != nullptr)
                {
                object.GetObject()->SetDPIScaleFactor(scaling);
                }
            }
        }

    //----------------------------------------------------------------
    void Graph2D::GetAxesOverhang(long& leftMargin, long& rightMargin, long& topMargin,
                                  long& bottomMargin, wxDC& dc) const
        {
        leftMargin = rightMargin = topMargin = bottomMargin = 0;
        std::vector<long> topMarginVals, bottomMarginVals, leftMarginVals, rightMarginVals;

        const auto addGutterDifferences = [this, &topMarginVals, &bottomMarginVals, &leftMarginVals,
                                           &rightMarginVals](const wxRect gutter)
        {
            topMarginVals.push_back(GetLeftYAxis().GetTopPoint().y - gutter.GetTop());
            bottomMarginVals.push_back(gutter.GetBottom() - GetLeftYAxis().GetBottomPoint().y);

            leftMarginVals.push_back(GetBottomXAxis().GetLeftPoint().x - gutter.GetLeft());
            rightMarginVals.push_back(gutter.GetRight() - GetBottomXAxis().GetRightPoint().x);
        };

        addGutterDifferences(GetLeftYAxis().GetBoundingBox(dc));
        addGutterDifferences(GetRightYAxis().GetBoundingBox(dc));
        addGutterDifferences(GetBottomXAxis().GetBoundingBox(dc));
        addGutterDifferences(GetTopXAxis().GetBoundingBox(dc));

        // Adjust for any custom axes also.
        // Note that we are only interested in how much the custom axes overhang the main axes.
        for (const auto& customAxis : GetCustomAxes())
            {
            addGutterDifferences(customAxis.GetBoundingBox(dc));
            }

        topMargin = *std::ranges::max_element(std::as_const(topMarginVals));
        bottomMargin = *std::ranges::max_element(std::as_const(bottomMarginVals));
        leftMargin = *std::ranges::max_element(std::as_const(leftMarginVals));
        rightMargin = *std::ranges::max_element(std::as_const(rightMarginVals));
        }

    //----------------------------------------------------------------
    void Graph2D::DrawSelectionLabel(wxDC& dc, [[maybe_unused]] const double scaling,
                                     [[maybe_unused]] const wxRect boundingBox) const
        {
        for (const auto& object : m_plotObjects)
            {
            object->DrawSelectionLabel(dc, GetScaling(), GetPlotAreaBoundingBox());
            }
        for (const auto& object : m_embeddedObjects)
            {
            object.GetObject()->DrawSelectionLabel(dc, GetScaling(), GetPlotAreaBoundingBox());
            }
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
            wxCoord yStartCoordinate{ 0 }, yEndCoordinate{ 0 }, xStartCoordinate{ 0 },
                xEndCoordinate{ 0 };
            const auto [rangeYStart, rangeYEnd] = GetLeftYAxis().GetRange();
            const auto [rangeXStart, rangeXEnd] = GetBottomXAxis().GetRange();
            if (GetLeftYAxis().GetPhysicalCoordinate(rangeYStart, yStartCoordinate) &&
                GetLeftYAxis().GetPhysicalCoordinate(rangeYEnd, yEndCoordinate) &&
                GetBottomXAxis().GetPhysicalCoordinate(rangeXStart, xStartCoordinate) &&
                GetBottomXAxis().GetPhysicalCoordinate(rangeXEnd, xEndCoordinate))
                {
                for (auto& customAxis : GetCustomAxes())
                    {
                    wxCoord x{ 0 }, y{ 0 };
                    if (GetBottomXAxis().GetPhysicalCoordinate(customAxis.GetCustomXPosition(),
                                                               x) &&
                        GetLeftYAxis().GetPhysicalCoordinate(customAxis.GetCustomYPosition(), y))
                        {
                        if (customAxis.IsVertical())
                            {
                            customAxis.SetPhysicalCustomXPosition(x);
                            customAxis.SetPhysicalCustomYPosition(y);
                            wxCoord yStartCoordinateOffset{ 0 };
                            if (customAxis.GetPhysicalCustomYPosition() != -1 &&
                                GetLeftYAxis().GetPhysicalCoordinate(
                                    rangeYStart + customAxis.GetOffsetFromParentAxis(),
                                    yStartCoordinateOffset))
                                {
                                customAxis.SetPoints(
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(),
                                            customAxis.GetPhysicalCustomYPosition()),
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(),
                                            yStartCoordinateOffset),
                                    dc);
                                }
                            }
                        else
                            {
                            customAxis.SetPhysicalCustomXPosition(x);
                            customAxis.SetPhysicalCustomYPosition(y);
                            wxCoord xStartCoordinateOffset{ 0 };
                            if (customAxis.GetPhysicalCustomXPosition() != -1 &&
                                GetBottomXAxis().GetPhysicalCoordinate(
                                    rangeXStart + customAxis.GetOffsetFromParentAxis(),
                                    xStartCoordinateOffset))
                                {
                                customAxis.SetPoints(
                                    wxPoint(xStartCoordinateOffset,
                                            customAxis.GetPhysicalCustomYPosition()),
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(),
                                            customAxis.GetPhysicalCustomYPosition()),
                                    dc);
                                }
                            }
                        }
                    }
                }
        };

        m_plotRect = GetBoundingBox(dc);
        // constrain to zero origin in case it goes outside that by a pixel or two
        m_plotRect.x = std::max(m_plotRect.x, 0);
        // set the axes' points assuming the entire drawing area, then measure their overhangs
        adjustAxesPoints();

        long leftAxisOverhang{ 0 }, rightAxisOverhang{ 0 }, topAxisOverhang{ 0 },
            bottomAxisOverhang{ 0 };
        GetAxesOverhang(leftAxisOverhang, rightAxisOverhang, topAxisOverhang, bottomAxisOverhang,
                        dc);

        m_calculatedLeftPadding = std::max<long>(
            leftAxisOverhang, GetLeftYAxis().GetProtrudingBoundingBox(dc).GetWidth());
        m_calculatedRightPadding = std::max<long>(
            rightAxisOverhang, GetRightYAxis().GetProtrudingBoundingBox(dc).GetWidth());
        m_calculatedBottomPadding = std::max<long>(
            bottomAxisOverhang, GetBottomXAxis().GetProtrudingBoundingBox(dc).GetHeight());
        m_calculatedTopPadding =
            std::max<long>(topAxisOverhang, GetTopXAxis().GetProtrudingBoundingBox(dc).GetHeight());

        // shrink the plot area to fit so that the axes outer area fit in the drawing area
        m_plotRect.x += m_calculatedLeftPadding;
        m_plotRect.y += m_calculatedTopPadding;
        m_plotRect.SetWidth(m_plotRect.GetWidth() -
                            (m_calculatedLeftPadding + m_calculatedRightPadding));
        m_plotRect.SetHeight(m_plotRect.GetHeight() -
                             (m_calculatedTopPadding + m_calculatedBottomPadding));

        if (m_plotRect.GetWidth() < 0)
            {
            wxLogMessage(L"Graph window too small; plot area width will be adjusted.");
            m_plotRect.SetWidth(1);
            }
        if (m_plotRect.GetHeight() < 0)
            {
            wxLogMessage(L"Graph window too small; plot area height will be adjusted.");
            m_plotRect.SetHeight(1);
            }

        // make space for the titles
        if (!GetTitle().GetText().empty() && GetTitle().IsShown())
            {
            auto titleRect = GetTitle().GetBoundingBox(dc);
            // if too wide, shrink down its scaling
            if (titleRect.GetWidth() > GetBoundingBox(dc).GetWidth())
                {
                const auto rescaleFactor =
                    safe_divide<double>(GetBoundingBox(dc).GetWidth(), titleRect.GetWidth());
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
        if (!GetSubtitle().GetText().empty() && GetSubtitle().IsShown())
            {
            auto titleRect = GetSubtitle().GetBoundingBox(dc);
            // if too wide, shrink down its scaling
            if (titleRect.GetWidth() > GetBoundingBox(dc).GetWidth())
                {
                const auto rescaleFactor =
                    safe_divide<double>(GetBoundingBox(dc).GetWidth(), titleRect.GetWidth());
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
        if ((!GetTitle().GetText().empty() && GetTitle().IsShown()) ||
            (!GetSubtitle().GetText().empty() && GetSubtitle().IsShown()))
            {
            const auto lineSpacing = ScaleToScreenAndCanvas(
                GetCaption().GetLineSpacing() *
                ((!GetTitle().GetText().empty() && !GetSubtitle().GetText().empty()) ? 3 : 2));
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
                const auto rescaleFactor =
                    safe_divide<double>(GetBoundingBox(dc).GetWidth(), titleRect.GetWidth());
                GetCaption().SetScaling(GetCaption().GetScaling() * rescaleFactor);
                titleRect = GetCaption().GetBoundingBox(dc);
                }
            if (GetCaption().GetFontBackgroundColor().IsOk() &&
                GetCaption().GetFontBackgroundColor() != wxTransparentColor)
                {
                GetCaption().SetMinimumUserSizeDIPs(dc.ToDIP(m_rect.GetWidth()), std::nullopt);
                }

            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                 (GetCaption().GetBoundingBox(dc).GetHeight() +
                                  (ScaleToScreenAndCanvas(GetCaption().GetLineSpacing() * 2))));
            }

        // adjust axes and do one more pass to ensure nothing like
        // custom axis brackets are going outside the area
        adjustAxesPoints();
        GetAxesOverhang(leftAxisOverhang, rightAxisOverhang, topAxisOverhang, bottomAxisOverhang,
                        dc);

        if (m_calculatedRightPadding < rightAxisOverhang)
            {
            m_plotRect.SetWidth(m_plotRect.GetWidth() -
                                (rightAxisOverhang - m_calculatedRightPadding));
            m_calculatedRightPadding = rightAxisOverhang;
            }

        // if axes from this graph are being adjusted to align with something else
        // (e.g., another graph), then adjust them now
        const auto originalPlotArea = GetPlotAreaBoundingBox();
        if (GetContentTop())
            {
            m_plotRect.SetTop(GetContentTop().value());
            }
        if (GetContentBottom())
            {
            m_plotRect.SetBottom(GetContentBottom().value());
            }
        if (GetContentLeft())
            {
            m_plotRect.SetLeft(GetContentLeft().value());
            }
        if (GetContentRight())
            {
            m_plotRect.SetRight(GetContentRight().value());
            }
        const auto adjustedPlotArea = GetPlotAreaBoundingBox();

        // ...and shrink the graph (draw) area to the smaller plot area
        auto drawArea = GetBoundingBox(dc);
        if (adjustedPlotArea.GetWidth() < originalPlotArea.GetWidth())
            {
            drawArea.SetLeft(drawArea.GetLeft() +
                             (adjustedPlotArea.GetLeft() - originalPlotArea.GetLeft()));
            drawArea.SetWidth(drawArea.GetWidth() -
                              (originalPlotArea.GetWidth() - adjustedPlotArea.GetWidth()));
            }
        if (adjustedPlotArea.GetHeight() < originalPlotArea.GetHeight())
            {
            drawArea.SetTop(drawArea.GetTop() +
                            (adjustedPlotArea.GetTop() - originalPlotArea.GetTop()));
            drawArea.SetHeight(drawArea.GetHeight() -
                               (originalPlotArea.GetHeight() - adjustedPlotArea.GetHeight()));
            }
        if (drawArea != GetBoundingBox(dc))
            {
            SetBoundingBox(drawArea, dc, GetScaling());
            }

        // reset the axes' points to the updated plot area
        adjustAxesPoints();
        }

    //----------------------------------------------------------------
    void Graph2D::UpdateSelectedItems()
        {
        for (auto& object : m_plotObjects)
            {
            if (GetSelectedIds().contains(object->GetId()))
                {
                // if applicable, set the object's subitems' selections from before
                auto foundItem = m_selectedItemsWithSubitems.find(object->GetId());
                if (foundItem != m_selectedItemsWithSubitems.cend())
                    {
                    object->GetSelectedIds() = foundItem->second;
                    }
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

        ContrastColors();

        // If bounding box hasn't been set yet, then set it to the parent
        // canvas's size. This would only happen if trying to measure the graph
        // before the window has a size event or is presented.
        // The normal case for this is when a graph is being measured for a canvas
        // to a specific content scaling (e.g., Table).
        if (GetBoundingBox(dc).IsEmpty())
            {
            if (GetCanvas() != nullptr)
                {
                SetBoundingBox(GetCanvas()->GetCanvasRect(dc), dc, GetScaling());
                }
            else
                {
                SetBoundingBox(wxRect(wxSize(Canvas::GetDefaultCanvasWidthDIPs(),
                                             Canvas::GetDefaultCanvasHeightDIPs())),
                               dc, 1.0);
                }
            }

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
            for (auto& bracket : customAxis.GetBrackets())
                {
                bracket.GetLabel().SetScaling(GetScaling());
                }
            }
        GetTitle().SetScaling(GetScaling());
        GetSubtitle().SetScaling(GetScaling());
        GetCaption().SetScaling(GetScaling());

        // update mirrored axes
        if (IsXAxisMirrored())
            {
            GetTopXAxis().CopySettings(GetBottomXAxis());
            }
        if (IsYAxisMirrored())
            {
            GetRightYAxis().CopySettings(GetLeftYAxis());
            }

        AdjustPlotArea(dc);

        const bool shouldStackLeftY = GetLeftYAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetLeftYAxis().IsShown() && ((GetLeftYAxis().IsStackingLabels() && !shouldStackLeftY) ||
                                         (!GetLeftYAxis().IsStackingLabels() && shouldStackLeftY)))
            {
            GetLeftYAxis().StackLabels(shouldStackLeftY);
            }

        const bool shouldStackRightY = GetRightYAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetRightYAxis().IsShown() &&
            ((GetRightYAxis().IsStackingLabels() && !shouldStackRightY) ||
             (!GetRightYAxis().IsStackingLabels() && shouldStackRightY)))
            {
            GetRightYAxis().StackLabels(shouldStackRightY);
            }

        const bool shouldStackBottomX = GetBottomXAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetBottomXAxis().IsShown() &&
            ((GetBottomXAxis().IsStackingLabels() && !shouldStackBottomX) ||
             (!GetBottomXAxis().IsStackingLabels() && shouldStackBottomX)))
            {
            GetBottomXAxis().StackLabels(shouldStackBottomX);
            }

        const bool shouldStackTopX = GetTopXAxis().ShouldLabelsBeStackedToFit(dc);
        if (GetTopXAxis().IsShown() && ((GetTopXAxis().IsStackingLabels() && !shouldStackTopX) ||
                                        (!GetTopXAxis().IsStackingLabels() && shouldStackTopX)))
            {
            GetTopXAxis().StackLabels(shouldStackTopX);
            }

        // Use a consistent font scaling for the four main axes, using the smallest one.
        // Note that the fonts will only be made smaller (not larger) across the axes, so
        // no need to readjust the plot areas again.
        const double bottomXLabelScaling = GetBottomXAxis().CalcBestScalingToFitLabels(dc);
        const double topXLabelScaling = GetTopXAxis().CalcBestScalingToFitLabels(dc);
        const double leftYLabelScaling = GetLeftYAxis().CalcBestScalingToFitLabels(dc);
        const double rightYLabelScaling = GetRightYAxis().CalcBestScalingToFitLabels(dc);

        const double smallestMainAxesLabelScaling = std::min(
            { bottomXLabelScaling, topXLabelScaling, leftYLabelScaling, rightYLabelScaling });
        double smallestCustomAxisLabelScaling = smallestMainAxesLabelScaling;
        if (GetCustomAxes().size())
            {
            const auto& smallestScaledCustomAxis = *std::ranges::min_element(
                std::as_const(GetCustomAxes()),
                [](const auto& lhv, const auto& rhv) noexcept
                {
                    return compare_doubles_less(lhv.GetAxisLabelScaling(),
                                                rhv.GetAxisLabelScaling());
                });
            smallestCustomAxisLabelScaling = smallestScaledCustomAxis.GetAxisLabelScaling();
            }
        const double smallestLabelScaling =
            std::min(smallestMainAxesLabelScaling, smallestCustomAxisLabelScaling);
        GetBottomXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetTopXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetLeftYAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetRightYAxis().SetAxisLabelScaling(smallestLabelScaling);

        for (auto& customAxis : GetCustomAxes())
            {
            customAxis.SetAxisLabelScaling(smallestLabelScaling);
            }

        // adjust plot margins again in case stacking or common axis
        // label scaling was changed
        AdjustPlotArea(dc);

        // fix overlapping custom axis bracket labels
        constexpr double MIN_BRACKET_FONT_SCALE{ math_constants::half };
        std::optional<double> smallestBracketFontScale{ std::nullopt };
        for (auto& customAxis : GetCustomAxes())
            {
            wxCoord labelPosition{ 0 }, nextLabelPosition{ 0 };
            if (customAxis.GetBrackets().size() > 1 && customAxis.IsVertical())
                {
                for (size_t i = 0; i < (customAxis.GetBrackets().size() - 1); ++i)
                    {
                    if (customAxis.GetPhysicalCoordinate(
                            customAxis.GetBrackets()[i].GetStartPosition(), labelPosition) &&
                        customAxis.GetPhysicalCoordinate(
                            customAxis.GetBrackets()[i + 1].GetLabelPosition(), nextLabelPosition))
                        {
                        auto bracketLabel{ customAxis.GetBrackets()[i].GetLabel() };
                        bracketLabel.SetAnchorPoint(wxPoint(0, labelPosition));
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                        auto bBox = bracketLabel.GetBoundingBox(dc);

                        auto nextBracketLabel{ customAxis.GetBrackets()[i + 1].GetLabel() };
                        nextBracketLabel.SetAnchorPoint(wxPoint(0, nextLabelPosition));
                        nextBracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                        auto nextBBox = nextBracketLabel.GetBoundingBox(dc);
                        if (bBox.Intersects(nextBBox))
                            {
                            const auto heightEclipsed = bBox.GetBottom() - nextBBox.GetTop();
                            const auto percentEclipsed =
                                safe_divide<double>(heightEclipsed, bBox.GetHeight());
                            customAxis.GetBrackets()[i].GetLabel().SetScaling(
                                std::max(MIN_BRACKET_FONT_SCALE,
                                         customAxis.GetBrackets()[i].GetLabel().GetScaling() *
                                             (1.0 - percentEclipsed)));
                            smallestBracketFontScale = std::max(
                                MIN_BRACKET_FONT_SCALE,
                                std::min(smallestBracketFontScale.value_or(
                                             customAxis.GetBrackets()[i].GetLabel().GetScaling()),
                                         customAxis.GetBrackets()[i].GetLabel().GetScaling()));
                            }
                        }
                    }
                }
            else if (customAxis.GetBrackets().size() > 1 && customAxis.IsHorizontal())
                {
                for (size_t i = 0; i < (customAxis.GetBrackets().size() - 1); ++i)
                    {
                    if (customAxis.GetPhysicalCoordinate(
                            customAxis.GetBrackets()[i].GetStartPosition(), labelPosition) &&
                        customAxis.GetPhysicalCoordinate(
                            customAxis.GetBrackets()[i + 1].GetLabelPosition(), nextLabelPosition))
                        {
                        auto bracketLabel{ customAxis.GetBrackets()[i].GetLabel() };
                        bracketLabel.SetAnchorPoint(wxPoint(labelPosition, 0));
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::BottomLeftCorner);
                        auto bBox = bracketLabel.GetBoundingBox(dc);

                        auto nextBracketLabel{ customAxis.GetBrackets()[i + 1].GetLabel() };
                        nextBracketLabel.SetAnchorPoint(wxPoint(nextLabelPosition, 0));
                        nextBracketLabel.SetAnchoring(Wisteria::Anchoring::BottomLeftCorner);
                        auto nextBBox = nextBracketLabel.GetBoundingBox(dc);
                        if (bBox.Intersects(nextBBox))
                            {
                            const auto widthEclipsed = bBox.GetRight() - nextBBox.GetLeft();
                            const auto percentEclipsed =
                                safe_divide<double>(widthEclipsed, bBox.GetWidth());
                            customAxis.GetBrackets()[i].GetLabel().SetScaling(
                                std::max(MIN_BRACKET_FONT_SCALE,
                                         customAxis.GetBrackets()[i].GetLabel().GetScaling() *
                                             (1.0 - percentEclipsed)));
                            smallestBracketFontScale = std::max(
                                MIN_BRACKET_FONT_SCALE,
                                std::min(smallestBracketFontScale.value_or(
                                             customAxis.GetBrackets()[i].GetLabel().GetScaling()),
                                         customAxis.GetBrackets()[i].GetLabel().GetScaling()));
                            }
                        }
                    }
                }
            }
        // homogenize the custom axes' bracket font scales
        // if there were overlaps that were adjusted
        if (smallestBracketFontScale)
            {
            for (auto& customAxis : GetCustomAxes())
                {
                for (auto& bracket : customAxis.GetBrackets())
                    {
                    bracket.GetLabel().SetScaling(smallestBracketFontScale.value());
                    }
                }
            }

        // adjust again
        AdjustPlotArea(dc);

        // fill in the plot area's color (if being used, by default it is transparent)
        if (GetPlotBackgroundColor().IsOk() && !GetPlotBackgroundColor().IsTransparent())
            {
            wxPoint boxPoints[4]{ { 0, 0 } };
            GraphItems::Polygon::GetRectPoints(GetPlotAreaBoundingBox(), boxPoints);
            AddObject(
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                          .Pen(*wxBLACK_PEN)
                                                          .Brush(wxColour(GetPlotBackgroundColor()))
                                                          .Scaling(GetScaling()),
                                                      boxPoints, std::size(boxPoints)));
            }

        // fill in the plot background image
        if (m_plotAreaBgImage.IsOk() && m_bgImageOpacity != wxALPHA_TRANSPARENT)
            {
            auto img = std::make_unique<Image>(
                (m_plotAreaImageFit == ImageFit::Shrink ?
                     Image::ShrinkImageToRect(
                         m_plotAreaBgImage.GetBitmap(m_plotAreaBgImage.GetDefaultSize())
                             .ConvertToImage(),
                         GetPlotAreaBoundingBox().GetSize()) :
                     Image::CropImageToRect(
                         m_plotAreaBgImage.GetBitmap(m_plotAreaBgImage.GetDefaultSize())
                             .ConvertToImage(),
                         GetPlotAreaBoundingBox().GetSize(), true)));
            img->SetDPIScaleFactor(dc.FromDIP(1));
            img->SetAnchoring(Anchoring::TopLeftCorner);
            img->SetAnchorPoint(m_plotAreaImageFit == ImageFit::Shrink ?
                                    wxPoint(GetPlotAreaBoundingBox().GetLeft() +
                                                (((GetPlotAreaBoundingBox().GetWidth() -
                                                   img->GetImageSize().GetWidth()) /
                                                  2)),
                                            GetPlotAreaBoundingBox().GetTop() +
                                                (((GetPlotAreaBoundingBox().GetHeight() -
                                                   img->GetImageSize().GetHeight()) /
                                                  2))) :
                                    GetPlotAreaBoundingBox().GetTopLeft());
            img->SetOpacity(m_bgImageOpacity);
            AddObject(std::move(img));
            }

        // draw the X axis grid lines
        if (GetBottomXAxis().IsShown() && GetBottomXAxis().GetGridlinePen().IsOk() &&
            GetBottomXAxis().GetAxisPointsCount() > 2)
            {
            auto xAxisLines = std::make_unique<Wisteria::GraphItems::Lines>(
                GetBottomXAxis().GetGridlinePen(), GetScaling());
            for (auto pos = GetBottomXAxis().GetAxisPoints().cbegin() + 1;
                 pos != GetBottomXAxis().GetAxisPoints().cend() - 1; ++pos)
                {
                xAxisLines->AddLine(wxPoint(static_cast<wxCoord>(pos->GetPhysicalCoordinate()),
                                            GetPlotAreaBoundingBox().GetY()),
                                    wxPoint(static_cast<wxCoord>(pos->GetPhysicalCoordinate()),
                                            (GetPlotAreaBoundingBox().GetY() +
                                             GetPlotAreaBoundingBox().GetHeight())));
                }
            AddObject(std::move(xAxisLines));
            }

        // draw the Y axis grid lines
        if (GetLeftYAxis().IsShown() && GetLeftYAxis().GetGridlinePen().IsOk() &&
            GetLeftYAxis().GetAxisPointsCount() > 2)
            {
            auto yAxisLines = std::make_unique<Wisteria::GraphItems::Lines>(
                GetLeftYAxis().GetGridlinePen(), GetScaling());
            for (auto pos = GetLeftYAxis().GetAxisPoints().cbegin() + 1;
                 pos != GetLeftYAxis().GetAxisPoints().cend() - 1; ++pos)
                {
                yAxisLines->AddLine(
                    wxPoint(GetPlotAreaBoundingBox().GetX(),
                            static_cast<wxCoord>(pos->GetPhysicalCoordinate())),
                    wxPoint((GetPlotAreaBoundingBox().GetX() + GetPlotAreaBoundingBox().GetWidth()),
                            static_cast<wxCoord>(pos->GetPhysicalCoordinate())));
                }
            AddObject(std::move(yAxisLines));
            }

        // draw the axes on the plot area (on top of the gridlines)
        // (AdjustPlotArea() will have already set the axes' points)
        AddObject(std::make_unique<Axis>(GetBottomXAxis()));
        AddObject(std::make_unique<Axis>(GetTopXAxis()));
        AddObject(std::make_unique<Axis>(GetLeftYAxis()));
        AddObject(std::make_unique<Axis>(GetRightYAxis()));

        // draw the title
        if (!GetTitle().GetText().empty())
            {
            auto title = std::make_unique<GraphItems::Label>(GetTitle());
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
                topPt.x += GetBoundingBox(dc).GetWidth() / 2;
                title->SetAnchorPoint(topPt);
                }
            else if (title->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                title->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox(dc).GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing());
                title->SetAnchorPoint(topPt);
                }
            AddObject(std::move(title));
            }

        // draw the subtitle
        if (!GetSubtitle().GetText().empty())
            {
            const auto titleSpacing = (!GetTitle().GetText().empty() ?
                                           GetTitle().GetBoundingBox(dc).GetHeight() +
                                               ScaleToScreenAndCanvas(GetTitle().GetLineSpacing()) :
                                           0);
            auto subtitle = std::make_unique<GraphItems::Label>(GetSubtitle());
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
                           safe_divide<double>(subtitle->GetBoundingBox(dc).GetHeight(), 2) +
                           titleSpacing;
                topPt.x += GetBoundingBox(dc).GetWidth() / 2;
                subtitle->SetAnchorPoint(topPt);
                }
            else if (subtitle->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                subtitle->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox(dc).GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing()) + titleSpacing;
                subtitle->SetAnchorPoint(topPt);
                }
            AddObject(std::move(subtitle));
            }

        // draw the caption
        if (GetCaption().GetText().length())
            {
            auto caption = std::make_unique<GraphItems::Label>(GetCaption());
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
                bottomPt.x += GetBoundingBox(dc).GetWidth() / 2;
                caption->SetAnchorPoint(bottomPt);
                }
            else if (caption->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                caption->SetAnchoring(Anchoring::BottomRightCorner);
                auto bottomPt = GetBoundingBox(dc).GetRightBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing());
                caption->SetAnchorPoint(bottomPt);
                }
            AddObject(std::move(caption));
            }

        // custom axes
        for (const auto& customAxis : GetCustomAxes())
            {
            AddObject(std::make_unique<Axis>(customAxis));
            }

        // reference lines
        for (const auto& refLine : GetReferenceLines())
            {
            wxCoord axisCoord{ 0 };
            auto dividerLine = std::make_unique<GraphItems::Lines>(
                wxPen(refLine.m_pen.GetColour(), 2, refLine.m_pen.GetStyle()), GetScaling());
            if (refLine.m_axisType == AxisType::LeftYAxis ||
                refLine.m_axisType == AxisType::RightYAxis)
                {
                const auto& parentAxis =
                    (refLine.m_axisType == AxisType::LeftYAxis ? GetLeftYAxis() : GetRightYAxis());
                if (parentAxis.GetPhysicalCoordinate(refLine.m_axisPosition, axisCoord))
                    {
                    dividerLine->AddLine(wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord),
                                         wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord));
                    AddObject(std::move(dividerLine));
                    }
                }
            else if (refLine.m_axisType == AxisType::BottomXAxis ||
                     refLine.m_axisType == AxisType::TopXAxis)
                {
                const auto& parentAxis =
                    (refLine.m_axisType == AxisType::BottomXAxis ? GetBottomXAxis() :
                                                                   GetTopXAxis());
                if (parentAxis.GetPhysicalCoordinate(refLine.m_axisPosition, axisCoord))
                    {
                    dividerLine->AddLine(wxPoint(axisCoord, GetLeftYAxis().GetBottomPoint().y),
                                         wxPoint(axisCoord, GetLeftYAxis().GetTopPoint().y));
                    AddObject(std::move(dividerLine));
                    }
                }
            }

        // reference areas
        for (const auto& refArea : GetReferenceAreas())
            {
            wxCoord axisCoord1{ 0 }, axisCoord2{ 0 };
            auto dividerLine1 = std::make_unique<GraphItems::Lines>(
                wxPen(refArea.m_pen.GetColour(), 1, refArea.m_pen.GetStyle()), GetScaling());
            auto dividerLine2 = std::make_unique<GraphItems::Lines>(
                wxPen(refArea.m_pen.GetColour(), 1, refArea.m_pen.GetStyle()), GetScaling());
            if (refArea.m_axisType == AxisType::LeftYAxis ||
                refArea.m_axisType == AxisType::RightYAxis)
                {
                const auto& parentAxis =
                    (refArea.m_axisType == AxisType::LeftYAxis ? GetLeftYAxis() : GetRightYAxis());
                if (parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition, axisCoord1) &&
                    parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition2, axisCoord2))
                    {
                    const wxPoint boxPoints[4] = {
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord1),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord1),
                        wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord2),
                        wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord2)
                    };
                    auto area = std::make_unique<GraphItems::Polygon>(
                        GraphItemInfo().Pen(wxNullPen), boxPoints, std::size(boxPoints));
                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid)
                        {
                        area->GetBrush().SetColour(ColorContrast::ChangeOpacity(
                            refArea.m_pen.GetColour(), Settings::GetTranslucencyValue()));
                        }
                    else if (refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromTopToBottom)
                        {
                        area->GetBrush() = wxTransparentColour;
                        area->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(refArea.m_pen.GetColour(),
                                                         Settings::GetTranslucencyValue()),
                            wxTransparentColour, FillDirection::South));
                        }
                    else if (refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromBottomToTop)
                        {
                        area->GetBrush() = wxTransparentColour;
                        area->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(refArea.m_pen.GetColour(),
                                                         Settings::GetTranslucencyValue()),
                            wxTransparentColour, FillDirection::North));
                        }
                    AddObject(std::move(area));

                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid ||
                        refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromTopToBottom)
                        {
                        dividerLine1->AddLine(
                            wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord1),
                            wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord1));
                        AddObject(std::move(dividerLine1));
                        }

                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid ||
                        refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromBottomToTop)
                        {
                        dividerLine2->AddLine(
                            wxPoint(GetBottomXAxis().GetLeftPoint().x, axisCoord2),
                            wxPoint(GetBottomXAxis().GetRightPoint().x, axisCoord2));
                        AddObject(std::move(dividerLine2));
                        }
                    }
                }
            else if (refArea.m_axisType == AxisType::BottomXAxis ||
                     refArea.m_axisType == AxisType::TopXAxis)
                {
                const auto& parentAxis =
                    (refArea.m_axisType == AxisType::BottomXAxis ? GetBottomXAxis() :
                                                                   GetTopXAxis());
                if (parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition, axisCoord1) &&
                    parentAxis.GetPhysicalCoordinate(refArea.m_axisPosition2, axisCoord2))
                    {
                    const wxPoint boxPoints[4] = {
                        wxPoint(axisCoord1, GetLeftYAxis().GetBottomPoint().y),
                        wxPoint(axisCoord1, GetLeftYAxis().GetTopPoint().y),
                        wxPoint(axisCoord2, GetLeftYAxis().GetTopPoint().y),
                        wxPoint(axisCoord2, GetLeftYAxis().GetBottomPoint().y)
                    };
                    auto area = std::make_unique<GraphItems::Polygon>(
                        GraphItemInfo().Pen(wxNullPen), boxPoints, std::size(boxPoints));
                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid)
                        {
                        area->GetBrush().SetColour(ColorContrast::ChangeOpacity(
                            refArea.m_pen.GetColour(), Settings::GetTranslucencyValue()));
                        }
                    else if (refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromLeftToRight)
                        {
                        area->GetBrush() = wxTransparentColour;
                        area->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(refArea.m_pen.GetColour(),
                                                         Settings::GetTranslucencyValue()),
                            wxTransparentColour, FillDirection::Right));
                        }
                    else if (refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromRightToLeft)
                        {
                        area->GetBrush() = wxTransparentColour;
                        area->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(refArea.m_pen.GetColour(),
                                                         Settings::GetTranslucencyValue()),
                            wxTransparentColour, FillDirection::Left));
                        }
                    AddObject(std::move(area));

                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid ||
                        refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromLeftToRight)
                        {
                        dividerLine1->AddLine(
                            wxPoint(axisCoord1, GetLeftYAxis().GetBottomPoint().y),
                            wxPoint(axisCoord1, GetLeftYAxis().GetTopPoint().y));
                        AddObject(std::move(dividerLine1));
                        }

                    if (refArea.m_refAreaStyle == ReferenceAreaStyle::Solid ||
                        refArea.m_refAreaStyle == ReferenceAreaStyle::FadeFromRightToLeft)
                        {
                        dividerLine2->AddLine(
                            wxPoint(axisCoord2, GetLeftYAxis().GetBottomPoint().y),
                            wxPoint(axisCoord2, GetLeftYAxis().GetTopPoint().y));
                        AddObject(std::move(dividerLine2));
                        }
                    }
                }
            }

        // embed client object once the axes' physical coordinates have been recalculated
        for (auto& object : m_embeddedObjects)
            {
            wxCoord x{ 0 }, y{ 0 };
            if (GetBottomXAxis().GetPhysicalCoordinate(object.GetAnchorPoint().x, x) &&
                GetLeftYAxis().GetPhysicalCoordinate(object.GetAnchorPoint().y, y))
                {
                object.GetObject()->SetAnchorPoint({ x, y });
                }
            // client may have used a custom scaling for the annotation,
            // so maintain that ratio
            object.GetObject()->SetScaling(GetScaling() * object.GetOriginalScaling());
            }
        }

    //----------------------------------------------------------------
    wxRect Graph2D::Draw(wxDC& dc) const
        {
        // draw the plot objects
        for (const auto& object : m_plotObjects)
            {
            object->Draw(dc);
            }
        for (const auto& object : m_embeddedObjects)
            {
            for (const auto& interestPoint : object.GetInterestPoints())
                {
                wxPoint anchorPt, interestPt;
                if (GetBottomXAxis().GetPhysicalCoordinate(object.GetAnchorPoint().x, anchorPt.x) &&
                    GetLeftYAxis().GetPhysicalCoordinate(object.GetAnchorPoint().y, anchorPt.y) &&
                    GetBottomXAxis().GetPhysicalCoordinate(interestPoint.x, interestPt.x) &&
                    GetLeftYAxis().GetPhysicalCoordinate(interestPoint.y, interestPt.y))
                    {
                    Lines ln(wxPen(*wxBLACK, 1, wxPenStyle::wxPENSTYLE_SHORT_DASH), GetScaling());
                    ln.AddLine(anchorPt, interestPt);
                    ln.SetLineStyle(LineStyle::Arrows);
                    ln.SetDPIScaleFactor(GetDPIScaleFactor());
                    ln.Draw(dc);
                    }
                }
            object.GetObject()->Draw(dc);
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
            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawInformationOnSelection))
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
                    for (auto i = GetBoundingBox(dc).GetTopLeft().x;
                         i < GetBoundingBox(dc).GetTopRight().x; i += 100)
                        {
                        dc.DrawLine(
                            wxPoint(i, GetBoundingBox(dc).GetTop()),
                            wxPoint(i, GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(20)));
                        }
                    // right-to-left
                    for (int i = GetBoundingBox(dc).GetTopRight().x;
                         i > GetBoundingBox(dc).GetTopLeft().x; i -= 100)
                        {
                        dc.DrawLine(
                            wxPoint(i, GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(20)),
                            wxPoint(i, GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(40)));
                        }
                    Label rulerLabel(
                        GraphItemInfo(_DT(L"\u21E6 100 pixels"))
                            .AnchorPoint(wxPoint(
                                GetBoundingBox(dc).GetTopRight().x - ScaleToScreenAndCanvas(5),
                                GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(25)))
                            .Anchoring(Anchoring::TopRightCorner)
                            .FontColor(*wxBLUE)
                            .Pen(*wxBLUE_PEN)
                            .DPIScaling(GetDPIScaleFactor())
                            .FontBackgroundColor(*wxWHITE)
                            .Padding(2, 2, 2, 2));
                    rulerLabel.SetMinimumUserSizeDIPs(90, std::nullopt);
                    rulerLabel.Draw(dc);
                    rulerLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    rulerLabel.SetText(_DT(L"100 pixels \u21E8"));
                    rulerLabel.SetAnchorPoint(
                        (wxPoint(GetBoundingBox(dc).GetTopLeft().x + ScaleToScreenAndCanvas(5),
                                 GetBoundingBox(dc).GetTop() + ScaleToScreenAndCanvas(5))));
                    rulerLabel.Draw(dc);
                    }
                    // ruler along the left, showing a 100-pixel legend
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxBLUE, ScaleToScreenAndCanvas(4)));
                    dc.DrawLine(GetBoundingBox(dc).GetTopLeft(), GetBoundingBox(dc).GetTopRight());
                    // top-to-bottom
                    for (auto i = GetBoundingBox(dc).GetTopLeft().y;
                         i < GetBoundingBox(dc).GetBottomLeft().y; i += 100)
                        {
                        dc.DrawLine(
                            wxPoint(GetBoundingBox(dc).GetLeft(), i),
                            wxPoint(GetBoundingBox(dc).GetLeft() + ScaleToScreenAndCanvas(20), i));
                        }
                    }
                const auto bBox = GetBoundingBox(dc);
                Label infoLabel(
                    GraphItemInfo(
                        wxString::Format(
                            _DT(L"Scaling: %s\n"
                                "Vertical Axes Top (x, y): %d, %d\n"
                                "Vertical Axes Bottom (x, y): %d, %d\n"
                                "Horizontal Axes Left (x, y): %d, %d\n"
                                "Horizontal Axes Right (x, y): %d, %d\n"
                                "Bounding Box (x,y,width,height): %d, %d, %d, %d\n"
                                "Content Area (x,y,width,height): %d, %d, %d, %d\n"
                                "Plot Decoration Padding (t,r,b,l): %ld, %ld, %ld, %ld\n"
                                "%s"),
                            wxNumberFormatter::ToString(
                                GetScaling(), 1, wxNumberFormatter::Style::Style_NoTrailingZeroes),
                            GetLeftYAxis().GetTopPoint().x, GetLeftYAxis().GetTopPoint().y,
                            GetLeftYAxis().GetBottomPoint().x, GetLeftYAxis().GetBottomPoint().y,
                            GetBottomXAxis().GetLeftPoint().x, GetBottomXAxis().GetLeftPoint().y,
                            GetBottomXAxis().GetRightPoint().x, GetBottomXAxis().GetRightPoint().y,
                            bBox.x, bBox.y, bBox.width, bBox.height, GetContentRect().GetX(),
                            GetContentRect().GetY(), GetContentRect().GetWidth(),
                            GetContentRect().GetHeight(), m_calculatedTopPadding,
                            m_calculatedRightPadding, m_calculatedBottomPadding,
                            m_calculatedLeftPadding, m_debugDrawInfoLabel))
                        .AnchorPoint(GetBoundingBox(dc).GetBottomRight())
                        .Anchoring(Anchoring::BottomRightCorner)
                        .FontColor(*wxBLUE)
                        .Pen(*wxBLUE_PEN)
                        .DPIScaling(GetDPIScaleFactor())
                        .FontBackgroundColor(*wxWHITE)
                        .Padding(2, 2, 2, 2));
                infoLabel.Draw(dc);
                }
            }
        return GetBoundingBox(dc);
        }

    //----------------------------------------------------------------
    bool Graph2D::SelectObjectAtPoint(const wxPoint& pt, wxDC& dc)
        {
        if (!IsSelectable())
            {
            return false;
            }
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
                plotObject.GetObject()->GetSelectedIds().clear();
                plotObject.GetObject()->SetSelected(false);
                }
            }
        // items are added to a plot FILO (i.e., painter's algorithm),
        // so go backwards so that we select the items on top

        // the embedded objects, added by client, that would be sitting
        // on top of everything else
        for (auto plotObject = m_embeddedObjects.rbegin(); plotObject != m_embeddedObjects.rend();
             ++plotObject)
            {
            if (plotObject->GetObject()->IsSelectable() && plotObject->GetObject()->HitTest(pt, dc))
                {
                plotObject->GetObject()->SetSelected(!plotObject->GetObject()->IsSelected());
                return true;
                }
            }
        // the standard graph objects (added via AddObject())
        for (auto plotObject = m_plotObjects.rbegin(); plotObject != m_plotObjects.rend();
             ++plotObject)
            {
            if ((*plotObject)->IsSelectable() && (*plotObject)->HitTest(pt, dc))
                {
                // toggle selection (or if it has subitems, then set it to selected
                // and let it perform its own selection logic)
                (*plotObject)
                    ->SetSelected((*plotObject)->GetSelectedIds().size() ?
                                      true :
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
                    // update our selection info if the object (and possibly, its sub-objects)
                    // were deselected
                    const auto unselectedItem = GetSelectedIds().find((*plotObject)->GetId());
                    if (unselectedItem != GetSelectedIds().end())
                        {
                        GetSelectedIds().erase(unselectedItem);
                        }
                    const auto unselectedItemWithSubitems =
                        m_selectedItemsWithSubitems.find((*plotObject)->GetId());
                    if (unselectedItemWithSubitems != m_selectedItemsWithSubitems.end())
                        {
                        m_selectedItemsWithSubitems.erase(unselectedItemWithSubitems);
                        }
                    }
                return true;
                }
            }
        // no items selected, so see if we at least clicked inside the plot area
        if (HitTest(pt, dc))
            {
            SetSelected(true);
            return true;
            }
        return false;
        }
    } // namespace Wisteria::Graphs
