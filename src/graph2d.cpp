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
        for (const auto& refLine : GetReferenceLines())
            {
            textLines += refLine.m_label + L"\n";
            legend->GetLegendIcons().push_back(
                LegendIcon(IconShape::SquareIcon,
                           wxPen(refLine.m_lineColor, 2, refLine.m_linePenStyle),
                           ColorContrast::ChangeOpacity(refLine.m_lineColor,
                                                        Settings::GetTranslucencyValue())));
            }
        std::set<ReferenceArea> refAreas;
        // combine areas with the same color and label
        for (const auto& refArea : GetReferenceAreas())
            { refAreas.insert(refArea); }
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
                                       const LegendCanvasPlacementHint hint) const
        {
        if (hint == LegendCanvasPlacementHint::EmbeddedOnGraph)
            {
            legend->GetGraphItemInfo().Pen(*wxBLACK_PEN).
                Padding(4, 4, 4, (legend->HasLegendIcons() ? Label::GetMinLegendWidth() : 4)).
                FontBackgroundColor(*wxWHITE);
            }
        else if (hint == LegendCanvasPlacementHint::RightOrLeftOfGraph)
            {
            legend->SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(legend));
            legend->GetGraphItemInfo().Pen(wxNullPen).
                Padding(0, 0, 0, (legend->HasLegendIcons() ? Label::GetMinLegendWidth() : 0)).
                CanvasPadding(4, 4, 4, 4);;
            }
        else if (hint == LegendCanvasPlacementHint::AboveOrBeneathGraph)
            {
            legend->GetGraphItemInfo().Pen(wxNullPen).
                Padding(0, 0, 0, (legend->HasLegendIcons() ? Label::GetMinLegendWidth() : 0)).
                CanvasPadding(4, 4, 4, 4);
            }
        }

    //----------------------------------------------------------------
    Graph2D::Graph2D(Canvas* canvas)
        {
        wxASSERT_MSG(canvas, L"Cannot use a null canvas with a plot!");
        SetWindow(canvas);
        SetCanvas(canvas);
        // set axes' DPI information
        GetLeftYAxis().SetWindow(GetCanvas());
        GetRightYAxis().SetWindow(GetCanvas());
        GetBottomXAxis().SetWindow(GetCanvas());
        GetTopXAxis().SetWindow(GetCanvas());

        GetTitle().SetWindow(GetCanvas());
        GetTitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);

        GetSubtitle().SetWindow(GetCanvas());
        GetSubtitle().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetSubtitle().GetFont().MakeSmaller();

        GetCaption().SetWindow(GetCanvas());
        GetCaption().SetRelativeAlignment(RelativeAlignment::FlushLeft);
        GetCaption().GetFont().MakeSmaller();
        GetCaption().SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::DimGray));
        }

    //----------------------------------------------------------------
    void Graph2D::GetAxesOverhang(long& leftMargin, long& rightMargin, long& topMargin,
                                  long& bottomMargin, wxDC& measureDC) const
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

        addGutterDifferences(GetLeftYAxis().GetBoundingBox(measureDC));
        addGutterDifferences(GetRightYAxis().GetBoundingBox(measureDC));
        addGutterDifferences(GetBottomXAxis().GetBoundingBox(measureDC));
        addGutterDifferences(GetTopXAxis().GetBoundingBox(measureDC));

        // Adjust for any custom axes also.
        // Note that we are only interested in how much the custom axes overhang the main.
        for (const auto& customAxis : GetCustomAxes())
            { addGutterDifferences(customAxis.GetBoundingBox(measureDC)); }

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
    void Graph2D::AdjustPlotArea()
        {
        // sets the physical points for the axes
        const auto adjustAxesPoints = [this]()
            {
            GetBottomXAxis().SetPoints(GetPlotAreaBoundingBox().GetLeftBottom(),
                                       GetPlotAreaBoundingBox().GetRightBottom());
            GetTopXAxis().SetPoints(GetPlotAreaBoundingBox().GetTopLeft(),
                                    GetPlotAreaBoundingBox().GetTopRight());
            GetLeftYAxis().SetPoints(GetPlotAreaBoundingBox().GetTopLeft(),
                                     GetPlotAreaBoundingBox().GetLeftBottom());
            GetRightYAxis().SetPoints(GetPlotAreaBoundingBox().GetRightTop(),
                                      GetPlotAreaBoundingBox().GetRightBottom());
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
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(), yStartCoordinateOffsetted));
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
                                    wxPoint(customAxis.GetPhysicalCustomXPosition(), customAxis.GetPhysicalCustomYPosition()));
                                }
                            }
                        }
                    }
                }
            };

        m_plotRect = GetBoundingBox();
        // set the axes' points assuming the entire drawing area, then measure their overhangs
        adjustAxesPoints();

        long leftAxisOverhang(0), rightAxisOverhang(0), topAxisOverhang(0), bottomAxisOverhang(0);
        wxGCDC measureDC;
        GetAxesOverhang(leftAxisOverhang, rightAxisOverhang, topAxisOverhang, bottomAxisOverhang, measureDC);

        m_calculatedLeftPadding = std::max<long>(leftAxisOverhang,
                                                 GetLeftYAxis().GetProtrudingBoundingBox(measureDC).GetWidth());
        m_calculatedRightPadding = std::max<long>(rightAxisOverhang,
                                                  GetRightYAxis().GetProtrudingBoundingBox(measureDC).GetWidth());
        m_calculatedBottomPadding = std::max<long>(bottomAxisOverhang,
                                                   GetBottomXAxis().GetProtrudingBoundingBox(measureDC).GetHeight());
        m_calculatedTopPadding = std::max<long>(topAxisOverhang,
                                                GetTopXAxis().GetProtrudingBoundingBox(measureDC).GetHeight());

        // shrink the plot area to fit so that the axes outer area fit in the drawing area
        m_plotRect.x += m_calculatedLeftPadding;
        m_plotRect.y += m_calculatedTopPadding;
        m_plotRect.SetWidth(m_plotRect.GetWidth()-(m_calculatedLeftPadding+m_calculatedRightPadding));
        m_plotRect.SetHeight(m_plotRect.GetHeight()-(m_calculatedTopPadding+m_calculatedBottomPadding));

        // make space for the titles
        if (GetTitle().GetText().length())
            {
            m_plotRect.y += GetTitle().GetBoundingBox(measureDC).GetHeight();
            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                  GetTitle().GetBoundingBox(measureDC).GetHeight());
            }
        if (GetSubtitle().GetText().length())
            {
            m_plotRect.y += GetSubtitle().GetBoundingBox(measureDC).GetHeight();
            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                  GetSubtitle().GetBoundingBox(measureDC).GetHeight());
            }
        // if both titles, then we need a space above and below them and one between.
        // if only one of the titles, then just a space above and below it.
        if (GetTitle().GetText().length() || GetSubtitle().GetText().length())
            {
            const auto lineSpacing = ScaleToScreenAndCanvas(GetCaption().GetLineSpacing() *
                ((GetTitle().GetText().length() && GetSubtitle().GetText().length()) ? 3 : 2));
            m_plotRect.y += lineSpacing;
            m_plotRect.SetHeight(m_plotRect.GetHeight() - lineSpacing);
            }
        // and caption at the bottom
        if (GetCaption().GetText().length())
            {
            m_plotRect.SetHeight(m_plotRect.GetHeight() -
                                  (GetCaption().GetBoundingBox(measureDC).GetHeight()+
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
    void Graph2D::RecalcSizes()
        {
        m_plotObjects.clear();

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

        AdjustPlotArea();

        // ...but now, see if any axis needs to be stacked and adjust everything again (if needed)
        bool stackingChanged = false;
        wxGCDC measureDC;

        const bool shouldStackLeftY = GetLeftYAxis().ShouldLabelsBeStackedToFit(measureDC);
        if (GetLeftYAxis().IsShown() &&
            ((GetLeftYAxis().IsStackingLabels() && !shouldStackLeftY) ||
            (!GetLeftYAxis().IsStackingLabels() && shouldStackLeftY)) )
            {
            GetLeftYAxis().StackLabels(shouldStackLeftY);
            stackingChanged = true;
            }

        const bool shouldStackRightY = GetRightYAxis().ShouldLabelsBeStackedToFit(measureDC);
        if (GetRightYAxis().IsShown() &&
            ((GetRightYAxis().IsStackingLabels() && !shouldStackRightY) ||
            (!GetRightYAxis().IsStackingLabels() && shouldStackRightY)) )
            {
            GetRightYAxis().StackLabels(shouldStackRightY);
            stackingChanged = true;
            }

        const bool shouldStackBottomX = GetBottomXAxis().ShouldLabelsBeStackedToFit(measureDC);
        if (GetBottomXAxis().IsShown() &&
            ((GetBottomXAxis().IsStackingLabels() && !shouldStackBottomX) ||
            (!GetBottomXAxis().IsStackingLabels() && shouldStackBottomX)) )
            {
            GetBottomXAxis().StackLabels(shouldStackBottomX);
            stackingChanged = true;
            }

        const bool shouldStackTopX = GetTopXAxis().ShouldLabelsBeStackedToFit(measureDC);
        if (GetTopXAxis().IsShown() &&
            ((GetTopXAxis().IsStackingLabels() && !shouldStackTopX) ||
            (!GetTopXAxis().IsStackingLabels() && shouldStackTopX)) )
            {
            GetTopXAxis().StackLabels(shouldStackTopX);
            stackingChanged = true;
            }

        // adjust plot margins again in case stacking was changed
        if (stackingChanged)
            { AdjustPlotArea(); }

        // Use a consistent font scaling for the four main axes, using the smallest one.
        // Note that the fonts will only be made smaller (not larger) across the axes, so
        // no need to readjust the plot areas again.
        const double bottomXLabelScaling = GetBottomXAxis().CalcBestScalingToFitLabels(measureDC);
        const double topXLabelScaling = GetTopXAxis().CalcBestScalingToFitLabels(measureDC);
        const double leftYLabelScaling = GetLeftYAxis().CalcBestScalingToFitLabels(measureDC);
        const double rightYLabelScaling = GetRightYAxis().CalcBestScalingToFitLabels(measureDC);

        const double smallestLabelScaling = std::min({ bottomXLabelScaling, topXLabelScaling,
                                                       leftYLabelScaling, rightYLabelScaling });
        GetBottomXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetTopXAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetLeftYAxis().SetAxisLabelScaling(smallestLabelScaling);
        GetRightYAxis().SetAxisLabelScaling(smallestLabelScaling);

        // fill in the plot area's color
        // (don't bother if none of the axes are being drawn or the color is transparent)
        if (GetBackgroundColor().IsOk() &&
             GetBackgroundOpacity() != wxALPHA_TRANSPARENT &&
            (GetBottomXAxis().IsShown() || GetTopXAxis().IsShown() ||
             GetLeftYAxis().IsShown() || GetRightYAxis().IsShown()) )
            {
            wxPoint boxPoints[4];
            GraphItems::Polygon::GetRectPoints(GetPlotAreaBoundingBox(), boxPoints);
            auto box = std::make_shared<GraphItems::Polygon>(
                                GraphItems::GraphItemInfo().Pen(*wxBLACK_PEN).
                                Brush(wxBrush(wxColour(GetBackgroundColor().Red(), GetBackgroundColor().Green(),
                                                       GetBackgroundColor().Blue(), GetBackgroundOpacity()))).
                                Scaling(GetScaling()),
                                boxPoints, 4);
            if (HasLinearGradient())
                {
                box->SetBackgroundFill(Colors::GradientFill(
                                   wxColour(GetBackgroundColor().Red(), GetBackgroundColor().Green(),
                                            GetBackgroundColor().Blue(), GetBackgroundOpacity()),
                                   wxColour(255, 255, 255, GetBackgroundOpacity()),
                                   FillDirection::South));
                }
            AddObject(box);
            }

        // draw the X axis grid lines
        if (GetBottomXAxis().IsShown() && GetBottomXAxis().GetGridlinePen().IsOk() && GetBottomXAxis().GetAxisPointsCount() > 2)
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
        if (GetLeftYAxis().IsShown() && GetLeftYAxis().GetGridlinePen().IsOk() && GetLeftYAxis().GetAxisPointsCount() > 2)
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
                auto topPt = GetBoundingBox().GetTopLeft();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing());
                title->SetAnchorPoint(topPt);
                AddObject(title);
                }
            else if (title->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                title->SetAnchoring(Anchoring::Center);
                auto topPt = GetBoundingBox().GetLeftTop();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing()) +
                            (title->GetBoundingBox(measureDC).GetHeight()/2);
                topPt.x += GetBoundingBox().GetWidth()/2;
                title->SetAnchorPoint(topPt);
                AddObject(title);
                }
            else if (title->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                title->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox().GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(title->GetLineSpacing());
                title->SetAnchorPoint(topPt);
                AddObject(title);
                }
            }

        // draw the subtitle
        if (GetSubtitle().GetText().length())
            {
            const auto titleSpacing = (GetTitle().GetText().length() ?
                GetTitle().GetBoundingBox(measureDC).GetHeight() +
                ScaleToScreenAndCanvas(GetTitle().GetLineSpacing()) :
                0);
            auto subtitle = std::make_shared<GraphItems::Label>(GetSubtitle());
            if (subtitle->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                subtitle->SetAnchoring(Anchoring::TopLeftCorner);
                auto topPt = GetBoundingBox().GetTopLeft();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing())+titleSpacing;
                subtitle->SetAnchorPoint(topPt);
                AddObject(subtitle);
                }
            else if (subtitle->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                subtitle->SetAnchoring(Anchoring::Center);
                auto topPt = GetBoundingBox().GetLeftTop();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing()) +
                               (subtitle->GetBoundingBox(measureDC).GetHeight()/2)+titleSpacing;
                topPt.x += GetBoundingBox().GetWidth()/2;
                subtitle->SetAnchorPoint(topPt);
                AddObject(subtitle);
                }
            else if (subtitle->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                subtitle->SetAnchoring(Anchoring::TopRightCorner);
                auto topPt = GetBoundingBox().GetRightTop();
                topPt.y += ScaleToScreenAndCanvas(subtitle->GetLineSpacing())+titleSpacing;
                subtitle->SetAnchorPoint(topPt);
                AddObject(subtitle);
                }
            }

        // draw the caption
        if (GetCaption().GetText().length())
            {
            auto caption = std::make_shared<GraphItems::Label>(GetCaption());
            if (caption->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                caption->SetAnchoring(Anchoring::BottomLeftCorner);
                auto bottomPt = GetBoundingBox().GetLeftBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing());
                caption->SetAnchorPoint(bottomPt);
                AddObject(caption);
                }
            else if (caption->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                caption->SetAnchoring(Anchoring::Center);
                auto bottomPt = GetBoundingBox().GetLeftBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing()) +
                               (caption->GetBoundingBox(measureDC).GetHeight()/2);
                bottomPt.x += GetBoundingBox().GetWidth()/2;
                caption->SetAnchorPoint(bottomPt);
                AddObject(caption);
                }
            else if (caption->GetRelativeAlignment() == RelativeAlignment::FlushRight)
                {
                caption->SetAnchoring(Anchoring::BottomRightCorner);
                auto bottomPt = GetBoundingBox().GetRightBottom();
                bottomPt.y -= ScaleToScreenAndCanvas(caption->GetLineSpacing());
                caption->SetAnchorPoint(bottomPt);
                AddObject(caption);
                }
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
                    ln.SetWindow(GetCanvas());
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
                GraphItems::Polygon::GetRectPoints(GetBoundingBox(), pts);
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
                    dc.DrawLine(GetBoundingBox().GetTopLeft(), GetBoundingBox().GetTopRight());
                    // left-to-right
                    for (auto i = GetBoundingBox().GetTopLeft().x; i < GetBoundingBox().GetTopRight().x; i += 100)
                        {
                        dc.DrawLine(wxPoint(i, GetBoundingBox().GetTop()),
                            wxPoint(i, GetBoundingBox().GetTop()+ScaleToScreenAndCanvas(20)) );
                        }
                    // right-to-left
                    for (auto i = GetBoundingBox().GetTopRight().x; i > GetBoundingBox().GetTopLeft().x; i -= 100)
                        {
                        dc.DrawLine(wxPoint(i, GetBoundingBox().GetTop()+ScaleToScreenAndCanvas(20)),
                            wxPoint(i, GetBoundingBox().GetTop()+ScaleToScreenAndCanvas(40)) );
                        }
                    Label rulerLabel(GraphItemInfo(L"\u21E6 100 pixels").
                        AnchorPoint(wxPoint(GetBoundingBox().GetTopRight().x - ScaleToScreenAndCanvas(5),
                                            GetBoundingBox().GetTop() + ScaleToScreenAndCanvas(25))).
                        Anchoring(Anchoring::TopRightCorner).FontColor(*wxBLUE).
                        Pen(*wxBLUE_PEN).Window(GetWindow()).
                        FontBackgroundColor(*wxWHITE).Padding(2,2,2,2));
                    rulerLabel.SetMinimumUserSize(90, std::nullopt);
                    rulerLabel.Draw(dc);
                    rulerLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    rulerLabel.SetText(L"100 pixels \u21E8");
                    rulerLabel.SetAnchorPoint((wxPoint(GetBoundingBox().GetTopLeft().x + ScaleToScreenAndCanvas(5),
                        GetBoundingBox().GetTop() + ScaleToScreenAndCanvas(5))) );
                    rulerLabel.Draw(dc);
                    }
                // ruler along the left, showing a 100-pixel legend
                    {
                    wxDCPenChanger pc(dc, wxPen(*wxBLUE, ScaleToScreenAndCanvas(4)));
                    dc.DrawLine(GetBoundingBox().GetTopLeft(), GetBoundingBox().GetTopRight());
                    // top-to-bottom
                    for (auto i = GetBoundingBox().GetTopLeft().y; i < GetBoundingBox().GetBottomLeft().y; i += 100)
                        {
                        dc.DrawLine(wxPoint(GetBoundingBox().GetLeft(), i),
                            wxPoint(GetBoundingBox().GetLeft()+ScaleToScreenAndCanvas(20), i) );
                        }
                    }
                const auto bBox = GetBoundingBox();
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
                        AnchorPoint(GetBoundingBox().GetBottomRight()).
                        Anchoring(Anchoring::BottomRightCorner).FontColor(*wxBLUE).
                        Pen(*wxBLUE_PEN).Window(GetWindow()).
                        FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2));
                infoLabel.Draw(dc);
                }
            }
        return GetBoundingBox();
        }

    //----------------------------------------------------------------
    bool Graph2D::SelectObjectAtPoint(const wxPoint& pt)
        {
        if (!IsSelectable())
            { return false; }
        // items are added to a plot FILO (i.e., painter's algorithm),
        // so go backwards so that we select the items on top
        for (auto plotObject = m_plotObjects.rbegin();
             plotObject != m_plotObjects.rend();
             ++plotObject)
            {
            if ((*plotObject)->IsSelectable() && (*plotObject)->HitTest(pt))
                {
                (*plotObject)->SetSelected(!(*plotObject)->IsSelected());
                return true;
                }
            }
        for (auto plotObject = m_embeddedObjects.rbegin();
             plotObject != m_embeddedObjects.rend();
             ++plotObject)
            {
            if ((*plotObject).m_object->IsSelectable() && (*plotObject).m_object->HitTest(pt))
                {
                (*plotObject).m_object->SetSelected(!(*plotObject).m_object->IsSelected());
                return true;
                }
            }
        // no items selected, so see if we at least clicked inside of the plot area
        if (HitTest(pt))
            {
            SetSelected(true);
            return true;
            }
        return false;
        }
    }
