///////////////////////////////////////////////////////////////////////////////
// Name:        boxplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "boxplot.h"
#include "../math/statistics.h"
#include <ranges>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::BoxPlot, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void BoxPlot::BoxAndWhisker::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                         const wxString& continuousColumnName,
                                         const std::optional<const wxString>& groupColumnName,
                                         const Data::GroupIdType groupId, const size_t schemeIndex)
        {
        if (data == nullptr)
            {
            return;
            }

        m_data = data;
        m_schemeIndex = schemeIndex;
        // If ignoring grouping column, then set the group ID to the default 0 value.
        // If the parent plot needs to access this ID for shape and color scheme info,
        // it will then use the default 0 value.
        m_useGrouping = groupColumnName.has_value();
        m_groupId = m_useGrouping ? groupId : 0;

        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
                                           m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column not found for box plot."),
                                 groupColumnName.value())
                    .ToUTF8());
            }
        m_continuousColumn = m_data->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': continuous column not found for box plot."),
                                 continuousColumnName)
                    .ToUTF8());
            }
        m_continuousColumnName = continuousColumnName;
        m_groupColumnName = groupColumnName;

        Calculate();

        frequency_set<double> jitterPoints;
        if (m_useGrouping)
            {
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                if (!std::isfinite(m_continuousColumn->GetValue(i)))
                    {
                    continue;
                    }

                if (m_groupColumn->GetValue(i) == m_groupId)
                    {
                    jitterPoints.insert(m_continuousColumn->GetValue(i));
                    }
                }
            }
        else
            {
            for (const auto& datum : m_continuousColumn->GetValues())
                {
                if (!std::isfinite(datum))
                    {
                    continue;
                    }

                jitterPoints.insert(datum);
                }
            }
        m_jitter.CalcSpread(jitterPoints);
        }

    //----------------------------------------------------------------
    void BoxPlot::BoxAndWhisker::Calculate()
        {
        if (GetDataset() == nullptr || m_continuousColumn->GetRowCount() == 0)
            {
            return;
            }
        std::vector<double> dest;
        if (m_useGrouping)
            {
            dest.reserve(GetDataset()->GetRowCount());
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                if (m_groupColumn->GetValue(i) == m_groupId &&
                    std::isfinite(m_continuousColumn->GetValue(i)))
                    {
                    dest.push_back(m_continuousColumn->GetValue(i));
                    }
                }
            }
        else
            {
            dest.reserve(GetDataset()->GetRowCount());
            std::ranges::copy_if(m_continuousColumn->GetValues(), std::back_inserter(dest),
                                 [](const auto val) noexcept { return std::isfinite(val); });
            }

        if (dest.empty())
            {
            // reset stats to NaN; rendering will naturally short-circuit
            m_middlePoint = m_lowerControlLimit = m_upperControlLimit = m_lowerWhisker =
                m_upperWhisker = std::numeric_limits<double>::quiet_NaN();
            return;
            }

        std::ranges::sort(dest);
        statistics::quartiles_presorted(dest, m_lowerControlLimit, m_upperControlLimit);
        const double outlierRange = 1.5 * (m_upperControlLimit - m_lowerControlLimit);
        m_lowerWhisker = m_lowerControlLimit - outlierRange;
        m_upperWhisker = m_upperControlLimit + outlierRange;
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
        for (const auto& val : std::ranges::reverse_view(dest))
            {
            if (val <= m_upperWhisker)
                {
                m_upperWhisker = val;
                break;
                }
            }

        m_middlePoint = statistics::median_presorted(dest.cbegin(), dest.cend());
        }

    //----------------------------------------------------------------
    BoxPlot::BoxPlot(Canvas * canvas,
                     const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/,
                     const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
                     const std::shared_ptr<Icons::Schemes::IconScheme>& shapes /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetColorScheme(colors);
        SetBrushScheme((brushes != nullptr ? brushes :
                                             std::make_shared<Brushes::Schemes::BrushScheme>(
                                                 Settings::GetDefaultColorScheme())));
        SetShapeScheme(
            (shapes != nullptr ? shapes : std::make_shared<Icons::Schemes::StandardShapes>()));

        GetRightYAxis().Show(false);
        if (GetTopXAxis().GetAxisLinePen().IsOk())
            {
            GetTopXAxis().GetAxisLinePen().SetColour(GetLeftYAxis().GetGridlinePen().GetColour());
            }
        if (GetBottomXAxis().GetAxisLinePen().IsOk())
            {
            GetBottomXAxis().GetAxisLinePen().SetColour(
                GetLeftYAxis().GetGridlinePen().GetColour());
            }
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().GetAxisLinePen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void BoxPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                          const wxString& continuousColumnName,
                          const std::optional<const wxString>& groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);

        m_boxes.clear();
        GetSelectedIds().clear();
        GetLeftYAxis().Reset();
        GetRightYAxis().Reset();
        GetBottomXAxis().Reset();
        GetTopXAxis().Reset();

        if (GetDataset() == nullptr)
            {
            return;
            }

        // sets titles from variables
        if (groupColumnName)
            {
            GetBottomXAxis().GetTitle().SetText(groupColumnName.value());
            }
        // AddBox() will turn on label display again if we have more than one box
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        m_groupColumn =
            (groupColumnName ? GetDataset()->GetCategoricalColumn(groupColumnName.value()) :
                               GetDataset()->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == GetDataset()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column not found for box plot."),
                                 groupColumnName.value())
                    .ToUTF8());
            }
        m_continuousColumn = GetDataset()->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': continuous column not found for box plot."),
                                 continuousColumnName)
                    .ToUTF8());
            }

        std::vector<BoxAndWhisker> boxes;
        if (m_groupColumn != GetDataset()->GetCategoricalColumns().cend())
            {
            std::set<Data::GroupIdType> groups;
            for (const auto& groupId : m_groupColumn->GetValues())
                {
                groups.insert(groupId);
                }
            for (const auto& group : groups)
                {
                BoxAndWhisker box(GetBoxEffect(), GetBoxCorners(), GetOpacity());
                box.SetData(data, continuousColumnName, groupColumnName, group, 0);
                boxes.push_back(std::move(box));
                }
            }
        else
            {
            BoxAndWhisker box(GetBoxEffect(), GetBoxCorners(), GetOpacity());
            box.SetData(data, continuousColumnName, std::nullopt, 0, 0);
            boxes.push_back(std::move(box));
            }

        std::sort(boxes.begin(), boxes.end());
        for (const auto& box : boxes)
            {
            AddBox(box);
            }
        }

    //----------------------------------------------------------------
    void BoxPlot::AddBox(const BoxAndWhisker& box)
        {
        if (GetDataset() == nullptr)
            {
            return;
            }

        m_boxes.push_back(box);
        const BoxAndWhisker& currentBox = m_boxes[m_boxes.size() - 1];
        GetBottomXAxis().SetRange(0,
                                  (m_boxes.size() > 1) ?
                                      m_boxes.size() + 1 :
                                      m_boxes.size() + 3 /*couple extra gridlines around the box*/,
                                  0, 1, 1);
        if (GetBoxCount() > 1)
            {
            GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
            }
        for (auto boxPos = m_boxes.begin(); boxPos != m_boxes.end(); ++boxPos)
            {
            const size_t axisOffset = (m_boxes.size() > 1) ? 1 : 2;
            const double boxAxisPosition = (boxPos - m_boxes.begin()) + axisOffset;
            const wxString groupIdLabel = boxPos->m_useGrouping ?
                                              m_groupColumn->GetLabelFromID(boxPos->m_groupId) :
                                              wxString{};
            boxPos->SetXAxisPosition(boxAxisPosition);
            GetBottomXAxis().SetCustomLabel(boxAxisPosition, GraphItems::Label(groupIdLabel));
            }

        // see how much room is needed for the whiskers and data points
        // (outliers would go beyond the whiskers).
        const auto [minValue, maxValue] =
            currentBox.m_useGrouping ?
                currentBox.GetDataset()->GetContinuousMinMax(currentBox.m_continuousColumnName,
                                                             currentBox.m_groupColumnName,
                                                             currentBox.m_groupId) :
                currentBox.GetDataset()->GetContinuousMinMax(currentBox.m_continuousColumnName);
        if (!std::isfinite(minValue) || !std::isfinite(maxValue))
            {
            wxLogWarning(L"Box plot cannot be drawn; data contains no finite values.");
            return;
            }
        const double yMin = std::min(currentBox.GetLowerWhisker(), minValue);
        const double yMax = std::max(currentBox.GetUpperWhisker(), maxValue);

        auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();

        // adjust the range (if necessary) to accommodate the plot
        if (GetLeftYAxis().GetInterval() > 0 && std::isfinite(yMin) && std::isfinite(yMax))
            {
            while (rangeStart > yMin)
                {
                rangeStart -= GetLeftYAxis().GetInterval();
                }
            while (rangeEnd < yMax)
                {
                rangeEnd += GetLeftYAxis().GetInterval();
                }
            }

        GetLeftYAxis().SetRange(rangeStart, rangeEnd, GetLeftYAxis().GetPrecision());
        }

    //----------------------------------------------------------------
    void BoxPlot::RecalcSizes(wxDC & dc)
        {
        if (GetDataset() == nullptr)
            {
            return;
            }

        Graph2D::RecalcSizes(dc);
        // get how much space we have for all the boxes
        const wxCoord boxWidth =
            safe_divide<int>(GetPlotAreaBoundingBox().GetWidth(), (m_boxes.size() + 3)) -
            ScaleToScreenAndCanvas(10);

        // if we don't have enough collective space for the boxes to be at least 3
        // units wide then will have to fail
        if (boxWidth < 3 * GetScaling())
            {
            // show "can't be drawn" message on graph if the boxes won't fit.
            // Should never happen unless you add an absurd amount of boxes to the plot
            const wxPoint textCoordinate(
                GetPlotAreaBoundingBox().GetX() + (GetPlotAreaBoundingBox().GetWidth() / 2),
                GetPlotAreaBoundingBox().GetY() + (GetPlotAreaBoundingBox().GetHeight() / 2));
            auto invalidLabel = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo(_(L"Too many boxes. Plot cannot be drawn."))
                    .Scaling(GetScaling())
                    .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                    .Font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).MakeLarger())
                    .AnchorPoint(textCoordinate));

            invalidLabel->SetShadowType(GetShadowType());
            AddObject(std::move(invalidLabel));
            return;
            }

        // scale the common image to the plot area's size
        wxImage scaledCommonImg;

        // main box renderer
        const auto drawBox = [&](auto& box, const bool measureOnly, const size_t boxIndex)
        {
            if (box.GetDataset()->GetRowCount() == 0)
                {
                return;
                }

            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetMiddlePoint(),
                                   box.m_middleCoordinate);
            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetLowerControlLimit(),
                                   box.m_lowerQuartileCoordinate);
            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetUpperControlLimit(),
                                   box.m_upperQuartileCoordinate);
            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetLowerWhisker(),
                                   box.m_lowerOutlierRangeCoordinate);
            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetUpperWhisker(),
                                   box.m_upperOutlierRangeCoordinate);

            // calculate the box (interquartile range)
            box.m_boxRect = wxRect(
                box.m_upperQuartileCoordinate.x - ((boxWidth / 2)), box.m_upperQuartileCoordinate.y,
                boxWidth + 1,
                // in case quartile range is nothing, set the box height to one
                std::max(box.m_lowerQuartileCoordinate.y - box.m_upperQuartileCoordinate.y, 1));

            if (measureOnly)
                {
                return;
                }

            // only draw a whisker if there is more than one datum
            // (which would certainly be the case, usually)
            if (box.GetDataset()->GetRowCount() > 1)
                {
                const wxString whiskerLabel = wxString::Format(
                    _(L"Non-outlier range: %s-%s"),
                    wxNumberFormatter::ToString(box.GetLowerWhisker(), 3,
                                                Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(box.GetUpperWhisker(), 3,
                                                Settings::GetDefaultNumberFormat()));

                const wxPen linePen(
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), 2);

                std::array<wxPoint, 2> linePoints = { box.m_upperOutlierRangeCoordinate,
                                                      box.m_lowerOutlierRangeCoordinate };
                AddObject(std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo(whiskerLabel)
                        .Pen(linePen)
                        .Brush(Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                        .Scaling(GetScaling()),
                    linePoints));

                linePoints[0] = wxPoint(box.m_lowerOutlierRangeCoordinate.x - ((boxWidth / 4)),
                                        box.m_lowerOutlierRangeCoordinate.y);
                linePoints[1] =
                    wxPoint(linePoints[0].x + (boxWidth / 2), box.m_lowerOutlierRangeCoordinate.y);
                AddObject(std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo(whiskerLabel)
                        .Pen(linePen)
                        .Brush(Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                        .Scaling(GetScaling()),
                    linePoints));

                linePoints[0] = wxPoint(box.m_lowerOutlierRangeCoordinate.x - ((boxWidth / 4)),
                                        box.m_upperOutlierRangeCoordinate.y);
                linePoints[1] =
                    wxPoint(linePoints[0].x + (boxWidth / 2), box.m_upperOutlierRangeCoordinate.y);
                AddObject(std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo(whiskerLabel)
                        .Pen(linePen)
                        .Brush(Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                        .Scaling(GetScaling()),
                    linePoints));
                }

            if (box.GetDataset()->GetRowCount() > 1)
                {
                const wxString boxLabel = wxString::Format(
                    _(L"75th Percentile: %s\n"
                      "Median: %s\n" // 50th percentile
                      "25th Percentile: %s"),
                    wxNumberFormatter::ToString(box.GetUpperControlLimit(), 3,
                                                Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(box.GetMiddlePoint(), 3,
                                                Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(box.GetLowerControlLimit(), 3,
                                                Settings::GetDefaultNumberFormat()));
                // draw the box
                if (box.GetBoxEffect() == BoxEffect::CommonImage && scaledCommonImg.IsOk())
                    {
                    wxRect imgSubRect{ box.m_boxRect };
                    imgSubRect.Offset(-GetPlotAreaBoundingBox().GetX(),
                                      -GetPlotAreaBoundingBox().GetY());
                    auto boxImage = std::make_unique<GraphItems::Image>(
                        GraphItems::GraphItemInfo(boxLabel)
                            .Pen(GetImageOutlineColor())
                            .AnchorPoint(box.m_boxRect.GetLeftTop()),
                        scaledCommonImg.GetSubImage(imgSubRect));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    boxImage->SetShadowType(GetShadowType());
                    AddObject(std::move(boxImage));
                    }
                else if (box.GetBoxEffect() == BoxEffect::Image && GetImageScheme() != nullptr)
                    {
                    const auto& barScaledImage = GetImageScheme()->GetImage(boxIndex);
                    auto boxImage = std::make_unique<GraphItems::Image>(
                        GraphItems::GraphItemInfo(boxLabel)
                            .Pen(GetImageOutlineColor())
                            .AnchorPoint(box.m_boxRect.GetLeftTop()),
                        GraphItems::Image::CropImageToRect(
                            barScaledImage.GetBitmap(barScaledImage.GetDefaultSize())
                                .ConvertToImage(),
                            wxSize(box.m_boxRect.GetWidth(), box.m_boxRect.GetHeight()), true));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetShadowType((GetShadowType() != ShadowType::NoDisplay) ?
                                                ShadowType::RightSideAndBottomShadow :
                                                ShadowType::NoDisplay);
                    AddObject(std::move(boxImage));
                    }
                else if (box.GetBoxEffect() == BoxEffect::StippleImage && GetStippleBrush().IsOk())
                    {
                    auto boxImage = std::make_unique<GraphItems::Image>(
                        GraphItems::GraphItemInfo(boxLabel).Pen(wxNullPen).AnchorPoint(
                            box.m_boxRect.GetLeftTop()),
                        GraphItems::Image::CreateStippledImage(
                            GetStippleBrush()
                                .GetBitmap(GetStippleBrush().GetDefaultSize())
                                .ConvertToImage(),
                            wxSize(box.m_boxRect.GetWidth(), box.m_boxRect.GetHeight()),
                            Orientation::Vertical, (GetShadowType() != ShadowType::NoDisplay),
                            ScaleToScreenAndCanvas(4)));
                    boxImage->SetOpacity(box.GetOpacity());
                    boxImage->SetAnchoring(Anchoring::TopLeftCorner);
                    boxImage->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    // note that stipples have their own shadows (a silhouette), so turn off
                    // the Image's native shadow renderer.
                    boxImage->SetShadowType(ShadowType::NoDisplay);
                    AddObject(std::move(boxImage));
                    }
                else if (box.GetBoxEffect() == BoxEffect::StippleShape)
                    {
                    wxPoint currentYTop = box.m_boxRect.GetLeftTop();
                    while (currentYTop.y < box.m_boxRect.GetBottom())
                        {
                        const wxSize stippleImgSize(box.m_boxRect.GetWidth(),
                                                    box.m_boxRect.GetWidth());
                        auto shape = std::make_unique<GraphItems::Shape>(
                            GraphItems::GraphItemInfo{}
                                .Pen(wxNullPen)
                                .Brush(GetStippleShapeColor())
                                .AnchorPoint(currentYTop)
                                .Anchoring(Anchoring::TopLeftCorner)
                                .DPIScaling(GetDPIScaleFactor())
                                .Scaling(GetScaling()),
                            GetStippleShape(), stippleImgSize);
                        shape->SetBoundingBox(wxRect(currentYTop, wxSize(box.m_boxRect.GetWidth(),
                                                                         box.m_boxRect.GetWidth())),
                                              dc, GetScaling());
                        AddObject(std::move(shape));
                        currentYTop.y += stippleImgSize.GetHeight();
                        }
                    }
                // color-filled box
                else
                    {
                    std::array<wxPoint, 4> boxPoints;
                    GraphItems::Polygon::GetRectPoints(box.m_boxRect, boxPoints);
                    // Polygons don't support drop shadows, so need to manually add
                    // a shadow as another polygon
                    if ((GetShadowType() != ShadowType::NoDisplay))
                        {
                        const wxCoord scaledShadowOffset =
                            ScaleToScreenAndCanvas(GetShadowOffset());
                        const std::array<wxPoint, 7> shadowPts = {
                            box.m_boxRect.GetLeftBottom() + wxPoint(scaledShadowOffset, 0),
                            box.m_boxRect.GetLeftBottom() +
                                wxPoint(scaledShadowOffset, scaledShadowOffset),
                            box.m_boxRect.GetRightBottom() +
                                wxPoint(scaledShadowOffset, scaledShadowOffset),
                            box.m_boxRect.GetRightTop() +
                                wxPoint(scaledShadowOffset, scaledShadowOffset),
                            box.m_boxRect.GetRightTop() + wxPoint(0, scaledShadowOffset),
                            box.m_boxRect.GetRightBottom(),
                            box.m_boxRect.GetLeftBottom() + wxPoint(scaledShadowOffset, 0)
                        };
                        AddObject(std::make_unique<GraphItems::Polygon>(
                            GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(
                                GraphItemBase::GetShadowColor()),
                            shadowPts));
                        }
                    wxColour boxColor =
                        (GetColorScheme() ? GetColorScheme()->GetColor(box.GetSchemeIndex()) :
                                            wxNullColour);
                    if (boxColor.IsOk())
                        {
                        boxColor = Colors::ColorContrast::ChangeOpacity(boxColor, box.GetOpacity());
                        }
                    wxBrush brush{ GetBrushScheme()->GetBrush(box.GetSchemeIndex()) };
                    brush.SetColour(
                        Colors::ColorContrast::ChangeOpacity(brush.GetColour(), box.GetOpacity()));
                    auto boxPoly = std::make_unique<GraphItems::Polygon>(
                        GraphItems::GraphItemInfo(boxLabel)
                            .Pen(
                                Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                            .Outline(true, true, true, true)
                            .Scaling(GetScaling())
                            .BaseColor(boxColor)
                            .Brush(brush),
                        boxPoints);
                    constexpr uint8_t BOX_LIGHTEN_FACTOR = 160;
                    if (box.GetBoxEffect() == BoxEffect::FadeFromLeftToRight)
                        {
                        boxPoly->GetBrush() = wxNullBrush;
                        boxPoly->SetBackgroundFill(Colors::GradientFill(
                            Colors::ColorContrast::ChangeOpacity(
                                GetBrushScheme()->GetBrush(box.GetSchemeIndex()).GetColour(),
                                box.GetOpacity()),
                            Colors::ColorContrast::ChangeOpacity(
                                GetBrushScheme()
                                    ->GetBrush(box.GetSchemeIndex())
                                    .GetColour()
                                    .ChangeLightness(BOX_LIGHTEN_FACTOR),
                                box.GetOpacity()),
                            FillDirection::East));
                        }
                    else if (box.GetBoxEffect() == BoxEffect::FadeFromRightToLeft)
                        {
                        boxPoly->GetBrush() = wxNullBrush;
                        boxPoly->SetBackgroundFill(Colors::GradientFill(
                            Colors::ColorContrast::ChangeOpacity(
                                GetBrushScheme()->GetBrush(box.GetSchemeIndex()).GetColour(),
                                box.GetOpacity()),
                            Colors::ColorContrast::ChangeOpacity(
                                GetBrushScheme()
                                    ->GetBrush(box.GetSchemeIndex())
                                    .GetColour()
                                    .ChangeLightness(BOX_LIGHTEN_FACTOR),
                                box.GetOpacity()),
                            FillDirection::West));
                        }
                    else if (box.GetBoxEffect() == BoxEffect::Glassy)
                        {
                        auto blockColor = Colors::ColorContrast::ChangeOpacity(
                            GetBrushScheme()->GetBrush(box.GetSchemeIndex()).GetColour(),
                            box.GetOpacity());
                        boxPoly->GetBrush() = wxNullBrush;
                        boxPoly->SetBackgroundFill(
                            Colors::GradientFill(blockColor, blockColor, FillDirection::East));
                        }
                    boxPoly->SetShape(
                        (box.GetBoxEffect() == BoxEffect::WaterColor) ?
                            GraphItems::Polygon::PolygonShape::WaterColorRectangle :
                        (box.GetBoxEffect() == BoxEffect::ThickWaterColor) ?
                            GraphItems::Polygon::PolygonShape::ThickWaterColorRectangle :
                        (box.GetBoxEffect() == BoxEffect::Glassy) ?
                            GraphItems::Polygon::PolygonShape::GlassyRectangle :
                            GraphItems::Polygon::PolygonShape::Rectangle);
                    boxPoly->SetBoxCorners(GetBoxCorners());
                    boxPoly->SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    // along with a second coat, we will make the thick water color
                    // brush use a more opaque value than the system's default
                    if (box.GetBoxEffect() == BoxEffect::ThickWaterColor &&
                        boxPoly->GetBrush().IsOk() &&
                        boxPoly->GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE &&
                        Settings::GetTranslucencyValue() < 200)
                        {
                        boxPoly->GetBrush().SetColour(Colors::ColorContrast::ChangeOpacity(
                            boxPoly->GetBrush().GetColour(), 200));
                        }
                    // add the box to the plot item collection
                    AddObject(std::move(boxPoly));
                    }
                }

            // draw the middle point line
            GetPhysicalCoordinates(box.GetXAxisPosition(), box.GetMiddlePoint(),
                                   box.m_middleCoordinate);
            const std::array<wxPoint, 2> boxLinePts = {
                wxPoint(box.m_boxRect.GetX(), box.m_middleCoordinate.y),
                wxPoint(box.m_boxRect.GetX() + box.m_boxRect.GetWidth(), box.m_middleCoordinate.y)
            };
            AddObject(std::make_unique<GraphItems::Polygon>(
                GraphItems::GraphItemInfo()
                    .Pen(
                        wxPenInfo(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetBrushScheme()->GetBrush(box.GetSchemeIndex()).GetColour()))
                            .Cap(wxPenCap::wxCAP_BUTT))
                    .Scaling(GetScaling()),
                boxLinePts));

            // draw the points (grouped)
            box.m_jitter.SetJitterWidth(box.m_boxRect.GetWidth());

            wxPoint pt;
            auto outliers = std::make_unique<GraphItems::Points2D>(wxNullPen);
            outliers->SetScaling(GetScaling());
            outliers->SetDPIScaleFactor(GetDPIScaleFactor());
            auto dataPoints = std::make_unique<GraphItems::Points2D>(wxNullPen);
            dataPoints->SetScaling(GetScaling());
            dataPoints->SetDPIScaleFactor(GetDPIScaleFactor());
            for (size_t i = 0; i < box.GetDataset()->GetRowCount(); ++i)
                {
                if (!std::isfinite(box.m_continuousColumn->GetValue(i)))
                    {
                    continue;
                    }

                const auto pointOutline =
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor());
                // skip value if from a different group
                if (box.m_useGrouping && box.m_groupColumn->GetValue(i) != box.m_groupId)
                    {
                    continue;
                    }
                // skip non-outlier points (unless they are requested to be shown)
                if (!box.IsShowingAllPoints() &&
                    box.m_continuousColumn->GetValue(i) <= box.GetUpperWhisker() &&
                    box.m_continuousColumn->GetValue(i) >= box.GetLowerWhisker())
                    {
                    continue;
                    }
                if (GetPhysicalCoordinates(box.GetXAxisPosition(),
                                           box.m_continuousColumn->GetValue(i), pt))
                    {
                    box.m_jitter.JitterPoint(pt);
                    if (box.m_continuousColumn->GetValue(i) > box.GetUpperWhisker() ||
                        box.m_continuousColumn->GetValue(i) < box.GetLowerWhisker())
                        {
                        outliers->AddPoint(
                            GraphItems::Point2D(GraphItems::GraphItemInfo(
                                                    box.GetDataset()->GetIdColumn().GetValue(i))
                                                    .AnchorPoint(pt)
                                                    .Brush(GetPointColor())
                                                    .Pen(pointOutline),
                                                Settings::GetPointRadius(),
                                                GetShapeScheme()->GetShape(box.GetSchemeIndex())),
                            dc);
                        }
                    else
                        {
                        dataPoints->AddPoint(
                            GraphItems::Point2D(GraphItems::GraphItemInfo(
                                                    box.GetDataset()->GetIdColumn().GetValue(i))
                                                    .AnchorPoint(pt)
                                                    .Brush(GetPointColor())
                                                    .Pen(pointOutline),
                                                Settings::GetPointRadius(),
                                                GetShapeScheme()->GetShape(box.GetSchemeIndex())),
                            dc);
                        }
                    }
                }
            AddObject(std::move(dataPoints));
            AddObject(std::move(outliers));
        };

        for (auto& box : m_boxes)
            {
            drawBox(box, true, 0); // index just used for images, not irrelevant here
            }

        // scale the common image to the plot area's size
        scaledCommonImg = GetCommonBoxImage().IsOk() ?
                              GraphItems::Image::CropImageToRect(
                                  GetCommonBoxImage()
                                      .GetBitmap(GetCommonBoxImage().GetDefaultSize())
                                      .ConvertToImage(),
                                  GetPlotAreaBoundingBox().GetSize(), true) :
                              wxNullImage;

        // draw the boxes
        for (size_t i = 0; i < m_boxes.size(); ++i)
            {
            drawBox(m_boxes[i], false, i);
            }

        // draw the connection points
        if (GetBoxCount() >= 2)
            {
            const wxPen connectionPen{ Colors::ColorContrast::ShadeOrTintIfClose(
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::BondiBlue),
                GetPlotOrCanvasColor()) };
            for (size_t i = 0; i < GetBoxCount() - 1; ++i)
                {
                const std::array<wxPoint, 2> connectionPts = {
                    wxPoint{ m_boxes[i].m_middleCoordinate.x, m_boxes[i].m_middleCoordinate.y },
                    wxPoint{ m_boxes[i + 1].m_middleCoordinate.x,
                             m_boxes[i + 1].m_middleCoordinate.y }
                };
                AddObject(std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo().Pen(connectionPen).Scaling(GetScaling()),
                    connectionPts));
                }
            }

        // draw the labels
        for (auto& box : m_boxes)
            {
            if (box.IsShowingLabels())
                {
                    // draw the labels
                    {
                    auto middleLabel = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo(
                            wxNumberFormatter::ToString(box.GetMiddlePoint(), GetLabelPrecision(),
                                                        Settings::GetDefaultNumberFormat()))
                            .Scaling(GetScaling())
                            .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                            .FontBackgroundColor(
                                Colors::ColorBrewer::GetColor(Colors::Color::White))
                            .Padding(2, 2, 2, 2)
                            .AnchorPoint(box.m_middleCoordinate));
                    middleLabel->SetShadowType(GetShadowType());
                    AddObject(std::move(middleLabel));
                    }

                if (box.GetDataset()->GetRowCount() > 1)
                    {
                        // lower control limit
                        {
                        auto label = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(
                                wxNumberFormatter::ToString(box.GetLowerControlLimit(),
                                                            GetLabelPrecision(),
                                                            Settings::GetDefaultNumberFormat()))
                                .Scaling(GetScaling())
                                .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                .FontBackgroundColor(
                                    Colors::ColorBrewer::GetColor(Colors::Color::White))
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(box.m_lowerQuartileCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(std::move(label));
                        }

                        // upper control limit
                        {
                        auto label = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(
                                wxNumberFormatter::ToString(box.GetUpperControlLimit(),
                                                            GetLabelPrecision(),
                                                            Settings::GetDefaultNumberFormat()))
                                .Scaling(GetScaling())
                                .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                .FontBackgroundColor(
                                    Colors::ColorBrewer::GetColor(Colors::Color::White))
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(box.m_upperQuartileCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(std::move(label));
                        }

                        // lower whisker
                        {
                        auto label = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(
                                wxNumberFormatter::ToString(box.GetLowerWhisker(),
                                                            GetLabelPrecision(),
                                                            Settings::GetDefaultNumberFormat()))
                                .Scaling(GetScaling())
                                .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                .FontBackgroundColor(
                                    Colors::ColorBrewer::GetColor(Colors::Color::White))
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(box.m_lowerOutlierRangeCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(std::move(label));
                        }

                        // upper whisker
                        {
                        auto label = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(
                                wxNumberFormatter::ToString(box.GetUpperWhisker(),
                                                            GetLabelPrecision(),
                                                            Settings::GetDefaultNumberFormat()))
                                .Scaling(GetScaling())
                                .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                .FontBackgroundColor(
                                    Colors::ColorBrewer::GetColor(Colors::Color::White))
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(box.m_upperOutlierRangeCoordinate));
                        label->SetShadowType(GetShadowType());
                        AddObject(std::move(label));
                        }
                    }
                }
            }

        // draw the legend on top of the plot is a single-box plot and was requested
        if (GetBoxCount() == 1 && IsOverlayingLegend())
            {
            auto legend = BoxPlot::CreateLegend(
                LegendOptions().PlacementHint(LegendCanvasPlacementHint::EmbeddedOnGraph));
            legend->SetAnchorPoint(
                wxPoint(GetPlotAreaBoundingBox().GetX() + GetPlotAreaBoundingBox().GetWidth(),
                        GetPlotAreaBoundingBox().GetY() + GetPlotAreaBoundingBox().GetHeight()));
            legend->SetAnchoring(Anchoring::BottomRightCorner);
            legend->SetScaling(GetScaling());
            AddObject(std::move(legend));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> BoxPlot::CreateLegend(const LegendOptions& options)
        {
        if (GetDataset() == nullptr || GetBoxCount() != 1)
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo().Pen(wxNullPen).DPIScaling(GetDPIScaleFactor()));
        legend->GetGraphItemInfo().Text(
            wxString::Format(_(L"75th Percentile: %s\n"
                               "Median: %s\n" // 50th percentile
                               "25th Percentile: %s\n"
                               "Non-outlier Range: %s-%s"),
                             wxNumberFormatter::ToString(GetBox(0).GetUpperControlLimit(), 3,
                                                         Settings::GetDefaultNumberFormat()),
                             wxNumberFormatter::ToString(GetBox(0).GetMiddlePoint(), 3,
                                                         Settings::GetDefaultNumberFormat()),
                             wxNumberFormatter::ToString(GetBox(0).GetLowerControlLimit(), 3,
                                                         Settings::GetDefaultNumberFormat()),
                             wxNumberFormatter::ToString(GetBox(0).GetLowerWhisker(), 3,
                                                         Settings::GetDefaultNumberFormat()),
                             wxNumberFormatter::ToString(GetBox(0).GetUpperWhisker(), 3,
                                                         Settings::GetDefaultNumberFormat())));

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
