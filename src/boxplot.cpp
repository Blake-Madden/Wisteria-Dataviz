///////////////////////////////////////////////////////////////////////////////
// Name:        boxplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "boxplot.h"
#include "math/statistics.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void BoxPlot::BoxAndWhisker::SetData(std::shared_ptr<const Data::Dataset> data,
                                         const wxString& continuousColumnName,
                                         std::optional<const wxString> groupColumnName,
                                         const Data::GroupIdType groupId,
                                         uint8_t percentileCoefficient)
        {
        if (percentileCoefficient <= 1 ||
            percentileCoefficient >= 49)
            { percentileCoefficient = 25; }
        m_percentileCoefficient = safe_divide<double>(percentileCoefficient, 100);
        m_data = data;
        // If ignoring grouping column, then set the group ID to the default 0 value.
        // If the parent plot needs to access this ID for shape and color scheme info,
        // it will then use the default 0 value.
        m_useGrouping = groupColumnName.has_value();
        m_groupId = m_useGrouping ? groupId : 0;

        if (m_data == nullptr)
            { return; }

        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for box plot."), groupColumnName.value()));
            }
        m_continuousColumn = m_data->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for box plot."), continuousColumnName));
            }
        m_continuousColumnName = continuousColumnName;
        m_groupColumnName = groupColumnName;

        Calculate();

        frequency_set<double> jitterPoints;
        if (m_useGrouping)
            {
            for (size_t i = 0; i < GetData()->GetRowCount(); ++i)
                {
                if (m_groupColumn->GetValue(i) == m_groupId)
                    { jitterPoints.insert(m_continuousColumn->GetValue(i)); }
                }
            }
        else
            {
            for (const auto& datum : m_continuousColumn->GetValues())
                { jitterPoints.insert(datum); }
            }
        m_jitter.CalcSpread(jitterPoints);
        }

    //----------------------------------------------------------------
    void BoxPlot::BoxAndWhisker::Calculate()
        {
        if (GetData() == nullptr || m_continuousColumn->GetRowCount() == 0)
            { return; }
        std::vector<double> dest;
        if (m_useGrouping)
            {
            dest.reserve(GetData()->GetRowCount());
            for (size_t i = 0; i < GetData()->GetRowCount(); ++i)
                {
                if (m_groupColumn->GetValue(i) == m_groupId)
                    { dest.push_back(m_continuousColumn->GetValue(i)); }
                }
            }
        else
            {
            dest.resize(GetData()->GetRowCount());
            std::copy(m_continuousColumn->GetValues().cbegin(),
                      m_continuousColumn->GetValues().cend(),
                      dest.begin());
            }

        std::sort(std::execution::par, dest.begin(), dest.end());
        statistics::percentiles_presorted(dest.begin(), dest.end(), GetPercentileCoefficient(),
                                          1.0f-GetPercentileCoefficient(),
            m_lowerControlLimit, m_upperControlLimit);
        const double outlierRange = 1.5*(m_upperControlLimit-m_lowerControlLimit);
        m_lowerWhisker = m_lowerControlLimit-outlierRange;
        m_upperWhisker = m_upperControlLimit+outlierRange;
        // find the first (lower) non-outlier point
        for (const auto& val : dest)
            {
            if (val >= m_lowerWhisker)
                {
                m_lowerWhisker = val;
                break;
                }
            }
        // find the first (upper) non-outlier point
        for (auto val = dest.rbegin(); val != dest.rend(); ++val)
            {
            if (*val <= m_upperWhisker)
                {
                m_upperWhisker = *val;
                break;
                }
            }

        m_middlePoint = statistics::median_presorted<double>(dest.cbegin(), dest.cend());
        }

    //----------------------------------------------------------------
    BoxPlot::BoxPlot(Canvas* canvas,
                     std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/,
                     std::shared_ptr<IconShapeScheme> shapes /*= nullptr*/) :
        Graph2D(canvas), m_labelPrecision(1),
        m_colorScheme(colors != nullptr ? colors :
            std::make_shared<Colors::Schemes::ColorScheme>
                 (Colors::Schemes::ColorScheme{
                     ColorBrewer::GetColor(Colors::Color::CarolinaBlue) })),
        m_shapeScheme(shapes != nullptr ? shapes : std::make_shared<IconShapeScheme>(StandardShapes()))
        {
        GetRightYAxis().Show(false);
        if (GetTopXAxis().GetAxisLinePen().IsOk())
            { GetTopXAxis().GetAxisLinePen().SetColour(GetLeftYAxis().GetGridlinePen().GetColour()); }
        if (GetBottomXAxis().GetAxisLinePen().IsOk())
            { GetBottomXAxis().GetAxisLinePen().SetColour(GetLeftYAxis().GetGridlinePen().GetColour()); }
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().GetAxisLinePen() = wxNullPen;
        // turn off connection line pen
        GetPen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void BoxPlot::SetData(std::shared_ptr<const Data::Dataset> data,
                         const wxString& continuousColumnName,
                         std::optional<const wxString> groupColumnName /*= std::nullopt*/,
                         uint8_t percentileCoefficient /*= 25*/)
        {
        m_data = data;
        m_boxes.clear();
        GetLeftYAxis().Reset();
        GetRightYAxis().Reset();
        GetBottomXAxis().Reset();
        GetTopXAxis().Reset();
        // sets titles from variables
        if (groupColumnName)
            { GetBottomXAxis().GetTitle().SetText(groupColumnName.value()); }
        GetLeftYAxis().GetTitle().SetText(continuousColumnName);
        // AddBox() will turn on label display again if we have more than one box
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        // garbage, so bail
        if (m_data == nullptr)
            { return; }

        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for box plot."), groupColumnName.value()));
            }
        m_continuousColumn = m_data->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for box plot."), continuousColumnName));
            }

        std::vector<BoxAndWhisker> boxes;
        if (m_groupColumn != m_data->GetCategoricalColumns().cend())
            {
            std::set<Data::GroupIdType> groups;
            for (const auto& groupId : m_groupColumn->GetValues())
                { groups.insert(groupId); }
            for (const auto& group : groups)
                {
                BoxAndWhisker box(GetBoxColor(), GetBoxEffect(),
                                  GetBoxCorners(), GetOpacity());
                box.SetData(data, continuousColumnName, groupColumnName, group, percentileCoefficient);
                boxes.push_back(box);
                }
            }
        else
            {
            BoxAndWhisker box(GetBoxColor(), GetBoxEffect(),
                              GetBoxCorners(), GetOpacity());
            box.SetData(data, continuousColumnName, std::nullopt, 0, percentileCoefficient);
            boxes.push_back(box);
            }

        std::sort(boxes.begin(), boxes.end());
        for (const auto& box : boxes)
            { AddBox(box); }
        }

    //----------------------------------------------------------------
    void BoxPlot::AddBox(const BoxAndWhisker& box)
        {
        if (m_data == nullptr)
            { return; }

        m_boxes.push_back(box);
        const BoxAndWhisker& currentBox = m_boxes[m_boxes.size()-1];
        GetBottomXAxis().SetRange(0, (m_boxes.size() > 1) ? m_boxes.size()+1 :
                                      m_boxes.size()+3/*couple extra gridlines around the box*/,
            0, 1, 1);
        if (GetBoxCount() > 1)
            { GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels); }
        for (auto boxPos = m_boxes.begin();
             boxPos != m_boxes.end();
             ++boxPos)
            {
            const size_t axisOffset = (m_boxes.size() > 1) ? 1 : 2;
            const double boxAxisPosition = (boxPos-m_boxes.begin())+axisOffset;
            const wxString groupIdLabel = boxPos->m_useGrouping ?
                m_groupColumn->GetCategoryLabel(boxPos->m_groupId) :
                wxString(L"");
            boxPos->SetXAxisPosition(boxAxisPosition);
            GetBottomXAxis().SetCustomLabel(boxAxisPosition,
                Label(groupIdLabel));
            }

        // see how much room is needed for the whiskers and data points
        // (outliers would go beyond the whiskers).
        const auto [fullDataMin, fullDataMax] = std::minmax_element(
            currentBox.m_continuousColumn->GetValues().cbegin(),
            currentBox.m_continuousColumn->GetValues().cend());
        const auto [minValue, maxValue] = currentBox.m_useGrouping ?
            currentBox.GetData()->GetContinuousMinMax(
                currentBox.m_continuousColumnName, currentBox.m_groupColumnName, currentBox.m_groupId) :
            std::make_pair(*fullDataMin, *fullDataMax);
        const double yMin = std::min(currentBox.GetLowerWhisker(), minValue);
        const double yMax = std::max(currentBox.GetUpperWhisker(), maxValue);

        auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();

        // adjust the range (if necessary) to accommodate the plot
        while (rangeStart > yMin)
            { rangeStart -= GetLeftYAxis().GetInterval(); }
        while (rangeEnd < yMax)
            { rangeEnd += GetLeftYAxis().GetInterval(); }

        GetLeftYAxis().SetRange(rangeStart, rangeEnd, GetLeftYAxis().GetPrecision());
        }

    //----------------------------------------------------------------
    void BoxPlot::RecalcSizes()
        {
        if (m_data == nullptr)
            { return; }

        Graph2D::RecalcSizes();
        // get how much space we have for all the boxes
        const wxCoord boxWidth = (GetPlotAreaBoundingBox().GetWidth()/(m_boxes.size()+3)) - ScaleToScreenAndCanvas(10);

        // if we don't have enough collective space for the boxes to be at least 3
        // units wide then will have to fail
        if (boxWidth < 3*GetScaling())
            {
            // show "can't be drawn" message on graph if the boxes won't fit.
            // Should never happen unless you add an absurd amount of boxes to the plot
            const wxPoint textCoordinate(GetPlotAreaBoundingBox().GetX()+(GetPlotAreaBoundingBox().GetWidth()/2),
                                         GetPlotAreaBoundingBox().GetY()+(GetPlotAreaBoundingBox().GetHeight()/2));
            auto invalidLabel = std::make_shared<GraphItems::Label>(
                GraphItemInfo(_("Too many boxes. Plot cannot be drawn.")).
                Scaling(GetScaling()).Pen(*wxBLACK_PEN).
                Font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).MakeLarger()).
                AnchorPoint(textCoordinate));

            invalidLabel->SetShadowType(GetShadowType());
            AddObject(invalidLabel);
            return;
            }

        // draw the boxes
        for (auto& box : m_boxes)
            {
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetMiddlePoint(), box.m_middleCoordinate);
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetLowerControlLimit(),box. m_lowerQuartileCoordinate);
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetUpperControlLimit(), box.m_upperQuartileCoordinate);
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetLowerWhisker(), box.m_lowerOutlierRangeCoordinate);
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetUpperWhisker(), box.m_upperOutlierRangeCoordinate);

            if (box.GetData()->GetRowCount() == 0)
                { continue; }

            // only draw a whisker if there is more than one datum
            // (which would certainly be the case, usually)
            if (box.GetData()->GetRowCount() > 1)
                {
                const wxString whiskerLabel = wxString::Format(_("Non-outlier range: %.3f-%.3f"),
                        box.GetLowerWhisker(), box.GetUpperWhisker());

                const wxPen linePen(*wxBLACK, 2);

                wxPoint linePoints[2] = { box.m_upperOutlierRangeCoordinate,
                                          box.m_lowerOutlierRangeCoordinate };
                AddObject(std::make_shared<GraphItems::Polygon>(
                    GraphItemInfo(whiskerLabel).Pen(linePen).Brush(*wxBLACK_BRUSH).Scaling(GetScaling()),
                    linePoints, std::size(linePoints)));

                linePoints[0] = wxPoint(box.m_lowerOutlierRangeCoordinate.x-((boxWidth/4)),
                                        box.m_lowerOutlierRangeCoordinate.y);
                linePoints[1] = wxPoint(linePoints[0].x+(boxWidth/2),
                                        box.m_lowerOutlierRangeCoordinate.y);
                AddObject(std::make_shared<GraphItems::Polygon>(
                    GraphItemInfo(whiskerLabel).Pen(linePen).Brush(*wxBLACK_BRUSH).Scaling(GetScaling()),
                    linePoints, std::size(linePoints)));

                linePoints[0] = wxPoint(box.m_lowerOutlierRangeCoordinate.x-((boxWidth/4)),
                                        box.m_upperOutlierRangeCoordinate.y);
                linePoints[1] = wxPoint(linePoints[0].x+(boxWidth/2),
                                        box.m_upperOutlierRangeCoordinate.y);
                AddObject(std::make_shared<GraphItems::Polygon>(
                    GraphItemInfo(whiskerLabel).Pen(linePen).Brush(*wxBLACK_BRUSH).Scaling(GetScaling()),
                    linePoints, std::size(linePoints)));
                }

            // calculate the box (interquartile range)
            box.m_boxRect = wxRect(box.m_upperQuartileCoordinate.x-((boxWidth/2)),
                box.m_upperQuartileCoordinate.y, boxWidth+1,
                // in case quartile range is nothing, set the box height to one
                std::max(box.m_lowerQuartileCoordinate.y-box.m_upperQuartileCoordinate.y, 1));
            if (box.GetData()->GetRowCount() > 1)
                {
                const wxString boxLabel =
                    wxString::Format(_("%dth Percentile: %.3f\n%dth Percentile: %.3f\nMedian: %.3f"),
                            static_cast<int>(100-(box.GetPercentileCoefficient()*100)),
                            box.GetUpperControlLimit(),
                            static_cast<int>(box.GetPercentileCoefficient()*100),
                            box.GetLowerControlLimit(), box.GetMiddlePoint());
                // draw the box
                if (box.GetBoxEffect() == BoxEffect::CommonImage && GetCommonBoxImage())
                    {
                    auto boxImage = std::make_shared<Image>(
                        GraphItemInfo(boxLabel).Pen(m_imageOutlineColor).
                        AnchorPoint(box.m_boxRect.GetLeftTop()),
                        GetCommonBoxImage()->GetSubImage(box.m_boxRect));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    boxImage->SetShadowType(GetShadowType());
                    AddObject(boxImage);
                    }
                else if (box.GetBoxEffect() == BoxEffect::Stipple &&
                    GetStippleBrush() && GetStippleBrush()->IsOk() )
                    {
                    auto boxImage = std::make_shared<Image>(
                        GraphItemInfo(boxLabel).Pen(wxNullPen).AnchorPoint(box.m_boxRect.GetLeftTop()),
                        Image::CreateStippledImage(wxImage(*GetStippleBrush()),
                            wxSize(box.m_boxRect.GetWidth(), box.m_boxRect.GetHeight()),
                            Orientation::Vertical, (GetShadowType() != ShadowType::NoShadow),
                            ScaleToScreenAndCanvas(4)));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    // note that stipples have their own shadows (a silhouette), so turn off the
                    // Image's native shadow renderer.
                    boxImage->SetShadowType(ShadowType::NoShadow);
                    AddObject(boxImage);
                    }
                else if (box.GetBoxEffect() == BoxEffect::Glassy)
                    {
                    auto boxImage = std::make_shared<Image>(
                        GraphItemInfo(boxLabel).Pen(ColorContrast::BlackOrWhiteContrast(box.GetBoxColor())).
                        AnchorPoint(box.m_boxRect.GetLeftTop()),
                        Image::CreateGlassEffect(
                            wxSize(box.m_boxRect.GetWidth(), box.m_boxRect.GetHeight()),
                            ColorContrast::ChangeOpacity(box.GetBoxColor(), box.GetOpacity()),
                            Orientation::Horizontal));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    boxImage->SetShadowType(GetShadowType());
                    AddObject(boxImage);
                    }
                // color-filled box
                else
                    {
                    wxPoint boxPoints[4];
                    GraphItems::Polygon::GetRectPoints(box.m_boxRect, boxPoints);
                    // Polygons don't support drop shadows, so need to manually add a shadow as another polygon
                    if ((GetShadowType() != ShadowType::NoShadow))
                        {
                        const wxCoord scaledShadowOffset = ScaleToScreenAndCanvas(GetShadowOffset());
                        wxPoint shadowPts[7];
                        shadowPts[0] = box.m_boxRect.GetLeftBottom()+wxPoint(scaledShadowOffset,0);
                        shadowPts[1] = box.m_boxRect.GetLeftBottom()+wxPoint(scaledShadowOffset,scaledShadowOffset);
                        shadowPts[2] = box.m_boxRect.GetRightBottom()+wxPoint(scaledShadowOffset,scaledShadowOffset);
                        shadowPts[3] = box.m_boxRect.GetRightTop()+wxPoint(scaledShadowOffset,scaledShadowOffset);
                        shadowPts[4] = box.m_boxRect.GetRightTop()+wxPoint(0,scaledShadowOffset);
                        shadowPts[5] = box.m_boxRect.GetRightBottom();
                        shadowPts[6] = shadowPts[0]; // close polygon
                        AddObject(std::make_shared<GraphItems::Polygon>(
                            GraphItemInfo().Pen(wxNullPen).Brush(GraphItemBase::GetShadowColour()),
                            shadowPts, std::size(shadowPts)));
                        }
                    auto boxPoly = std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo(boxLabel).Pen(ColorContrast::BlackOrWhiteContrast(box.GetBoxColor())).
                        Scaling(GetScaling()).
                        Brush(ColorContrast::ChangeOpacity(box.GetBoxColor(), box.GetOpacity())),
                        boxPoints, std::size(boxPoints));
                    const uint8_t boxLightenFactor = 160;
                    if (box.GetBoxEffect() == BoxEffect::FadeFromLeftToRight)
                        {
                        boxPoly->GetBrush() = wxNullBrush;
                        boxPoly->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(box.GetBoxColor(), box.GetOpacity()),
                            ColorContrast::ChangeOpacity(
                                box.GetBoxColor().ChangeLightness(boxLightenFactor), box.GetOpacity()),
                            FillDirection::East));
                        }
                    else if (box.GetBoxEffect() == BoxEffect::FadeFromRightToLeft)
                        {
                        boxPoly->GetBrush() = wxNullBrush;
                        boxPoly->SetBackgroundFill(Colors::GradientFill(
                            ColorContrast::ChangeOpacity(box.GetBoxColor(), box.GetOpacity()),
                            ColorContrast::ChangeOpacity(
                                box.GetBoxColor().ChangeLightness(boxLightenFactor), box.GetOpacity()),
                            FillDirection::West));
                        }
                    boxPoly->SetShape(GraphItems::Polygon::PolygonShape::Rectangle);
                    boxPoly->SetBoxCorners(GetBoxCorners());
                    boxPoly->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    // add the box to the plot item collection
                    AddObject(boxPoly);
                    }
                }

            // draw the middle point line
            GetPhyscialCoordinates(box.GetXAxisPosition(), box.GetMiddlePoint(), box.m_middleCoordinate);
            wxPoint boxLinePts[2] = { wxPoint(box.m_boxRect.GetX(), box.m_middleCoordinate.y),
                                      wxPoint(box.m_boxRect.GetX() + box.m_boxRect.GetWidth(),
                                              box.m_middleCoordinate.y) };
            AddObject(std::make_shared<GraphItems::Polygon>(
                GraphItemInfo().Pen(*wxBLACK_PEN).Brush(*wxBLACK_BRUSH).Scaling(GetScaling()),
                boxLinePts, 2));

            // draw the points (grouped)
            box.m_jitter.SetJitterWidth(box.m_boxRect.GetWidth());

            wxPoint pt;
            auto outliers = std::make_shared<GraphItems::Points2D>(wxNullPen);
            outliers->SetScaling(GetScaling());
            outliers->SetWindow(GetWindow());
            auto dataPoints = std::make_shared<GraphItems::Points2D>(wxNullPen);
            dataPoints->SetScaling(GetScaling());
            dataPoints->SetWindow(GetWindow());
            for (size_t i = 0; i < box.GetData()->GetRowCount(); ++i)
                {
                const auto pointOutline =
                    wxColour(ColorContrast::BlackOrWhiteContrast(GetColorScheme()->GetColor(0)));
                // skip value if from a different group
                if (box.m_useGrouping &&
                    box.m_groupColumn->GetValue(i) != box.m_groupId)
                    { continue; }
                // skip non-outlier points (unless they are requested to be shown)
                if (!box.IsShowingAllPoints() &&
                    box.m_continuousColumn->GetValue(i) <= box.GetUpperWhisker() &&
                    box.m_continuousColumn->GetValue(i) >= box.GetLowerWhisker())
                    { continue; }
                if (GetPhyscialCoordinates(box.GetXAxisPosition(), box.m_continuousColumn->GetValue(i), pt))
                    {
                    box.m_jitter.JitterPoint(pt);
                    if (box.m_continuousColumn->GetValue(i) > box.GetUpperWhisker() ||
                        box.m_continuousColumn->GetValue(i) < box.GetLowerWhisker())
                        {
                        outliers->AddPoint(Point2D(
                            GraphItemInfo(box.GetData()->GetIdColumn().GetValue(i)).AnchorPoint(pt).
                            Brush(GetColorScheme()->GetColor(0)).Pen(pointOutline),
                            Settings::GetPointRadius(),
                            GetShapeScheme()->GetShape(0)));
                        }
                    else
                        {
                        dataPoints->AddPoint(Point2D(
                            GraphItemInfo(box.GetData()->GetIdColumn().GetValue(i)).AnchorPoint(pt).
                            Brush(GetColorScheme()->GetColor(0)).Pen(pointOutline),
                            Settings::GetPointRadius(),
                            GetShapeScheme()->GetShape(0)));
                        }
                    }
                }
            AddObject(dataPoints);
            AddObject(outliers);
            }

        // draw the connection points
        if (GetPen().IsOk() && GetBoxCount() >= 2)
            {
            for (size_t i = 0; i < GetBoxCount()-1; ++i)
                {
                wxPoint connectionPts[2] = { wxPoint(m_boxes[i].m_middleCoordinate.x, m_boxes[i].m_middleCoordinate.y),
                                             wxPoint( m_boxes[i+1].m_middleCoordinate.x, m_boxes[i+1].m_middleCoordinate.y) };
                AddObject(std::make_shared<GraphItems::Polygon>(
                    GraphItemInfo().Pen(GetPen()).Brush(*wxBLUE_BRUSH).Scaling(GetScaling()),
                    connectionPts, 2));
                }
            }

        // draw the labels
        for (auto& box : m_boxes)
            {
            if (box.IsShowingLabels())
                {
                // draw the labels
                auto middleLabel = std::make_shared<GraphItems::Label>(
                    GraphItemInfo(wxNumberFormatter::ToString(box.GetMiddlePoint(), GetLabelPrecision(),
                        Settings::GetDefaultNumberFormat())).
                    Scaling(GetScaling()).Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2).
                    AnchorPoint(box.m_middleCoordinate));
                middleLabel->SetShadowType(GetShadowType());
                AddObject(middleLabel);

                if (box.GetData()->GetRowCount() > 1)
                    {
                    // lower control limit
                        {
                        auto label = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(wxNumberFormatter::ToString(box.GetLowerControlLimit(), GetLabelPrecision(),
                                Settings::GetDefaultNumberFormat())).
                            Scaling(GetScaling()).Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2).
                            AnchorPoint(box.m_lowerQuartileCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(label);
                        }

                    // upper control limit
                        {
                        auto label = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(wxNumberFormatter::ToString(box.GetUpperControlLimit(), GetLabelPrecision(),
                                    Settings::GetDefaultNumberFormat())).
                                Scaling(GetScaling()).Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2).
                                AnchorPoint(box.m_upperQuartileCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(label);
                        }

                    // lower whisker
                        {
                        auto label = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(wxNumberFormatter::ToString(box.GetLowerWhisker(), GetLabelPrecision(),
                                Settings::GetDefaultNumberFormat())).
                            Scaling(GetScaling()).Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2).
                            AnchorPoint(box.m_lowerOutlierRangeCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(label);
                        }

                    // upper whisker
                        {
                        auto label = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(wxNumberFormatter::ToString(box.GetUpperWhisker(), GetLabelPrecision(),
                                Settings::GetDefaultNumberFormat())).
                            Scaling(GetScaling()).Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2).
                            AnchorPoint(box.m_upperOutlierRangeCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(label);
                        }
                    }
                }
            }

        // draw the legend on top of the plot is a single-box plot and was requested
        if (GetBoxCount() == 1 && IsOverlayingLegend())
            {
            auto legend = BoxPlot::CreateLegend(LegendCanvasPlacementHint::EmbeddedOnGraph, false);
            legend->SetAnchorPoint(wxPoint(GetPlotAreaBoundingBox().GetX()+GetPlotAreaBoundingBox().GetWidth(),
                                           GetPlotAreaBoundingBox().GetY()+GetPlotAreaBoundingBox().GetHeight()));
            legend->SetAnchoring(Anchoring::BottomRightCorner);
            AddObject(legend);
            }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> BoxPlot::CreateLegend(
        const LegendCanvasPlacementHint hint,
        const bool includeHeader) const
        {
        if (m_data == nullptr)
            { return nullptr; }

        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Pen(wxNullPen).Window(GetWindow()));
        legend->SetBoxCorners(BoxCorners::Rounded);

        if (GetBoxCount() == 1)
            {
            legend->GetGraphItemInfo().Text(
                    wxString::Format(_(L"Median: %.3f\n%dth Percentile: %.3f\n"
                                        "%dth Percentile: %.3f\nNon-outlier Range: %.3f-%.3f"),
                        GetBox(0).GetMiddlePoint(),
                        static_cast<int>(100-(GetBox(0).GetPercentileCoefficient()*100)),
                        GetBox(0).GetUpperControlLimit(),
                        static_cast<int>((GetBox(0).GetPercentileCoefficient()*100)),
                        GetBox(0).GetLowerControlLimit(),
                        GetBox(0).GetLowerWhisker(), GetBox(0).GetUpperWhisker()));
            }
        else
            {
            legend->GetGraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidth());
            wxString legendText;
            size_t lineCount{ 0 };
            for (const auto& box : m_boxes)
                {
                if (Settings::GetMaxLegendItemCount() == lineCount)
                    {
                    legendText.append(L"\u2026");
                    break;
                    }
                wxString currentLabel = (box.m_useGrouping ?
                    box.m_groupColumn->GetCategoryLabel(box.m_groupId) :
                    wxString(L""));
                if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                    {
                    currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                    currentLabel.append(L"\u2026");
                    }
                legendText.append(currentLabel.c_str()).append(L"\n");
                if (GetColorScheme()->GetColors().size() > 1)
                    {
                    legend->GetLegendIcons().emplace_back(
                        LegendIcon(IconShape::BoxPlotIcon, *wxBLACK,
                            GetColorScheme()->GetColor(box.m_groupId)));
                    }
                else
                    {
                    legend->GetLegendIcons().emplace_back(
                        LegendIcon(GetShapeScheme()->GetShape(box.m_groupId),
                                   *wxBLACK, *wxBLACK));
                    }
                }
            if (includeHeader)
                {
                legendText.Prepend(wxString::Format(L"%s\n", m_groupColumn->GetTitle()));
                legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
                }
            legend->SetText(legendText.Trim());
            }

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
