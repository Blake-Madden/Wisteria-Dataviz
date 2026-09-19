///////////////////////////////////////////////////////////////////////////////
// Name:        funnelchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "funnelchart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::FunnelChart, Wisteria::Graphs::BarChart)

    namespace Wisteria::Graphs
    {
    //-----------------------------------
    FunnelChart::FunnelChart(Canvas * canvas,
                             const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes,
                             const std::shared_ptr<Colors::Schemes::ColorScheme>& colors)
        : BarChart{ canvas }
        {
        SetBrushScheme(brushes != nullptr ? brushes :
                                            std::make_shared<Brushes::Schemes::BrushScheme>(
                                                Settings::GetDefaultColorScheme()));
        SetColorScheme(colors);
        SetBarOrientation(Orientation::Horizontal);
        // row 0 is top of funnel, so reverse the bar axis so 0 maps to the top
        GetLeftYAxis().Reverse(true);
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetLeftYAxis().ShowOuterLabels(false);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetBottomXAxis().Show(false);
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetRightYAxis().Show(false);
        GetTopXAxis().Show(false);
        SetSortable(false);
        IncludeSpacesBetweenBars(false);
        SetBarEffect(BoxEffect::Solid);
        SetBarOpacity(wxALPHA_OPAQUE);
        SetBinLabelDisplay(BinLabelDisplay::NoDisplay);
        }

    //-----------------------------------
    auto FunnelChart::GetStageColumn(const wxString& stageColumnName) const
        {
        if (GetDataset() != nullptr &&
            GetDataset()->GetIdColumn().GetName().CmpNoCase(stageColumnName) == 0)
            {
            return GetDataset()->GetCategoricalColumns().cend();
            }
        auto catCol = GetDataset()->GetCategoricalColumn(stageColumnName);
        if (catCol == GetDataset()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': stage column not found for funnel chart."),
                                 stageColumnName)
                    .ToUTF8());
            }
        return catCol;
        }

    //-----------------------------------
    auto FunnelChart::GetValueColumn(const wxString& valueColumnName) const
        {
        auto contCol = GetDataset()->GetContinuousColumn(valueColumnName);
        if (contCol == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': value column not found for funnel chart."),
                                 valueColumnName)
                    .ToUTF8());
            }
        return contCol;
        }

    //-----------------------------------
    auto FunnelChart::GetTargetColumn(const std::optional<wxString>& targetColumnName) const
        {
        if (!targetColumnName)
            {
            return GetDataset()->GetContinuousColumns().cend();
            }
        auto contCol = GetDataset()->GetContinuousColumn(targetColumnName.value());
        if (contCol == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': target column not found for funnel chart."),
                                 targetColumnName.value())
                    .ToUTF8());
            }
        return contCol;
        }

    //-----------------------------------
    void FunnelChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                              const wxString& stageColumnName, const wxString& valueColumnName,
                              const std::optional<wxString>& targetColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        m_stageColumnName = stageColumnName;
        m_valueColumnName = valueColumnName;
        m_targetColumnName = targetColumnName;
        m_targetValues.clear();
        m_useIdColumnForStage = false;
        ClearBars();
        ClearBarGroups();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        if (GetDataset()->GetIdColumn().GetName().CmpNoCase(stageColumnName) == 0)
            {
            m_useIdColumnForStage = true;
            }
        const auto stageCol = GetStageColumn(stageColumnName);
        const auto valueCol = GetValueColumn(valueColumnName);
        const auto targetCol = GetTargetColumn(targetColumnName);

        if (GetDataset()->GetRowCount() == 0)
            {
            GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
            GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        // collect row-ordered data
        struct RowEntry
            {
            wxString m_label;
            double m_value{ 0 };
            double m_target{ std::numeric_limits<double>::quiet_NaN() };
            };

        std::vector<RowEntry> rows;
        rows.reserve(GetDataset()->GetRowCount());
        double longest{ 0 };
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            wxString label;
            if (m_useIdColumnForStage)
                {
                label = GetDataset()->GetIdColumn().GetValue(i);
                }
            else
                {
                const auto code = stageCol->GetValue(i);
                // if categorical missing, keep label empty but preserve order
                if (stageCol->IsMissingData(i))
                    {
                    label.clear();
                    }
                else
                    {
                    label = stageCol->GetLabelFromID(code);
                    }
                }
            double val = valueCol->GetValue(i);
            if (!std::isfinite(val) || val < 0)
                {
                val = 0;
                }
            double tgt = std::numeric_limits<double>::quiet_NaN();
            if (targetCol != GetDataset()->GetContinuousColumns().cend())
                {
                tgt = targetCol->GetValue(i);
                if (!std::isfinite(tgt) || tgt < 0)
                    {
                    tgt = std::numeric_limits<double>::quiet_NaN();
                    }
                else
                    {
                    longest = std::max(longest, tgt);
                    }
                }
            longest = std::max(longest, val);
            rows.push_back({ label, val, tgt });
            }

        // handle all-zero case
        if (longest == 0)
            {
            longest = 10;
            }

        GetScalingAxis().SetRange(0, longest, 0,
                                  (GetBinLabelDisplay() != BinLabelDisplay::NoDisplay));
        AdjustScalingAxisFromBarLength(longest);

        // build bars in row order, axis position = index
        m_targetValues.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i)
            {
            const auto& row = rows[i];
            m_targetValues.push_back(row.m_target);

            const wxColour brushColor =
                GetColorScheme() ?
                    GetColorScheme()->GetColor(i % (GetColorScheme()->GetColors().empty() ?
                                                        1 :
                                                        GetColorScheme()->GetColors().size())) :
                    wxTransparentColour;
            const wxBrush brush =
                GetBrushScheme() ?
                    GetBrushScheme()->GetBrush(i % (GetBrushScheme()->GetBrushes().empty() ?
                                                        1 :
                                                        GetBrushScheme()->GetBrushes().size())) :
                    wxNullBrush;

            // per-stage color/brush in row order, wrapping scheme
            // selection label shows value and conversion from previous
            wxString selLabelText;
            if (i > 0 && rows[i - 1].m_value != 0)
                {
                const double conv = safe_divide<double>(row.m_value, rows[i - 1].m_value) * 100.0;
                const double drop = rows[i - 1].m_value - row.m_value;
                const wxString convStr = wxString::Format(
                    _DT(L"<span style='font-weight:bold;'>%s%%</span>"),
                    wxNumberFormatter::ToString(conv, 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                selLabelText = wxString::Format(
                    // TRANSLATORS: %s are row label + raw value,
                    // conversion rate and drop values in a funnel chart.
                    _(L"%s (%s conversion, %s drop)"),
                    wxString::Format(_DT(L"%s: %s"), row.m_label,
                                     wxNumberFormatter::ToString(
                                         row.m_value, 0, Settings::GetDefaultNumberFormat())),
                    convStr,
                    wxNumberFormatter::ToString(drop, 0, Settings::GetDefaultNumberFormat()));
                }
            else
                {
                selLabelText =
                    wxString::Format(_DT(L"%s: %s"), row.m_label,
                                     wxNumberFormatter::ToString(
                                         row.m_value, 0, Settings::GetDefaultNumberFormat()));
                }
            GraphItems::Label selLabel{ selLabelText };
            selLabel.EnableMarkup(true);

            Bar bar{ static_cast<double>(i),
                     { BarBlock{ BarBlockInfo{ row.m_value }
                                     .Brush(brush)
                                     .Color(brushColor)
                                     .Tag(row.m_label)
                                     .SelectionLabel(selLabel) } },
                     wxString{},
                     GraphItems::Label{ row.m_label },
                     GetBarEffect(),
                     GetBarOpacity() };
            // No bar label, value is drawn inside the trapezoid.
            bar.GetLabel().SetText(wxString{});
            // do not adjust scaling per-bar, we set range once
            AddBar(bar, false);
            }

        GetLeftYAxis().GetTitle().SetText(
            m_useIdColumnForStage ? GetDataset()->GetIdColumn().GetName() : stageCol->GetName());
        GetBottomXAxis().GetTitle().SetText(wxString{});
        }

    //-----------------------------------
    void FunnelChart::RecalcSizes(wxDC & dc)
        {
        // base draws axes, titles and gridlines
        // (Call Graph2D directly, BarChart draws standard bars and would duplicate trapezoids)
        Graph2D::RecalcSizes(dc); // NOLINT(bugprone-parent-virtual-call)

        if (GetBars().empty())
            {
            GetRightYAxis().Reset();
            GetBarAxis().Reset();
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            GetTopXAxis().Reset();
            GetScalingAxis().Reset();
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        // hide bottom axis numbers. values are on the funnel
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(true);

        const wxRect plotRect{ GetPlotAreaBoundingBox() };
        const size_t n{ GetBars().size() };
        if (n == 0 || plotRect.GetWidth() <= 0 || plotRect.GetHeight() <= 0)
            {
            return;
            }

        const double maxVal =
            GetScalingAxis().GetRange().second > 0 ? GetScalingAxis().GetRange().second : 1.0;
        const double centerDataX{ safe_divide<double>(maxVal, 2.0) };

        // interval between bar axis positions
        const double intervalPhys{ GetBarAxis().GetIntervalPhysicalLength() };
        // stage thickness fills the interval
        const double stageHeightPhys{ std::max<double>(2.0, intervalPhys) };
        const wxCoord stageHeight{ static_cast<wxCoord>(std::round(stageHeightPhys)) };

        constexpr double bottleneckOutlinePenWidthDIPs{ 2.0 };

        auto getLeft = [&](const double val, const double stageIdx) -> wxCoord
        {
            const double leftData{ centerDataX - safe_divide<double>(val, 2.0) };
            wxPoint lp{ -1, -1 };
            GetPhysicalCoordinates(leftData, stageIdx, lp);
            if (lp.x != -1)
                {
                return lp.x;
                }
            return plotRect.GetLeft() +
                   static_cast<wxCoord>(std::round(safe_divide<double>(leftData, maxVal) *
                                                   static_cast<double>(plotRect.GetWidth())));
        };

        auto getRight = [&](const double val, const double stageIdx) -> wxCoord
        {
            const double rightData{ centerDataX + safe_divide<double>(val, 2.0) };
            wxPoint rp{ -1, -1 };
            GetPhysicalCoordinates(rightData, stageIdx, rp);
            if (rp.x != -1)
                {
                return rp.x;
                }
            return plotRect.GetLeft() +
                   static_cast<wxCoord>(std::round(safe_divide<double>(rightData, maxVal) *
                                                   static_cast<double>(plotRect.GetWidth())));
        };

        // physical center Y for a (possibly fractional) stage index, degrading gracefully
        // if physical coordinates can't be resolved for it
        auto getCenterY = [&](const double stageIdx) -> wxCoord
        {
            wxPoint pt{ -1, -1 };
            if (GetPhysicalCoordinates(centerDataX, stageIdx, pt))
                {
                return pt.y;
                }
            const double frac{ safe_divide<double>(stageIdx + 0.5, static_cast<double>(n)) };
            return plotRect.GetTop() +
                   static_cast<wxCoord>(std::round(frac * plotRect.GetHeight()));
        };

        // boxed black-on-white annotation label (overall/actual/target/bottleneck callouts)
        auto makeAnnotationLabel = [&](const wxString& text) -> std::unique_ptr<GraphItems::Label>
        {
            auto label = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ text }
                    .Pen(wxNullPen)
                    .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                    .FontBackgroundColor(Colors::ColorBrewer::GetColor(Colors::Color::White))
                    .Padding(2, 4, 2, 4)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor()));
            label->EnableMarkup(true);
            label->GetFont().MakeSmaller();
            label->SetTextAlignment(TextAlignment::Centered);
            return label;
        };

        // check if any target is wider than actual
        bool anyTargetWider{ false };
        size_t targetCount{ 0 };
        for (size_t i = 0; i < n; ++i)
            {
            if (i < m_targetValues.size() && std::isfinite(m_targetValues[i]))
                {
                ++targetCount;
                if (m_targetValues[i] > GetBars()[i].GetLength())
                    {
                    anyTargetWider = true;
                    }
                }
            }
        const bool noTargetsWider{ targetCount > 0 && !anyTargetWider };

        // find bottleneck: stage with biggest drop from previous. only when positive
        size_t bottleneckIdx{ n };
        double maxDropPct{ 0.0 };
        for (size_t i = 1; i < n; ++i)
            {
            const double prev{ GetBars()[i - 1].GetLength() };
            const double cur{ GetBars()[i].GetLength() };
            if (prev > 0 && std::isfinite(prev) && std::isfinite(cur))
                {
                const double dropPct{ 1.0 - safe_divide<double>(cur, prev) };
                if (dropPct > maxDropPct)
                    {
                    maxDropPct = dropPct;
                    bottleneckIdx = i;
                    }
                }
            }

        // draw each stage as centered trapezoid
        for (size_t i = 0; i < n; ++i)
            {
            const double curVal{ GetBars()[i].GetLength() };
            const double nextVal{ (i + 1 < n) ? GetBars()[i + 1].GetLength() : curVal };
            const double curTarget{ (i < m_targetValues.size() &&
                                     std::isfinite(m_targetValues[i])) ?
                                        m_targetValues[i] :
                                        std::numeric_limits<double>::quiet_NaN() };
            const double nextTarget{
                (i + 1 < m_targetValues.size() && std::isfinite(m_targetValues[i + 1])) ?
                    m_targetValues[i + 1] :
                    (!std::isfinite(curTarget) ? std::numeric_limits<double>::quiet_NaN() :
                                                 curTarget)
            };

            const wxCoord yCenter{ getCenterY(static_cast<double>(i)) };
            const wxCoord yTop{ yCenter - (stageHeight / 2) };
            const wxCoord yBottom{ yCenter + (stageHeight / 2) };

            // helper to build ghost polygon for this stage
            auto buildGhostPoly = [&](const double topT,
                                      const double botT) -> std::unique_ptr<GraphItems::Polygon>
            {
                wxCoord tLeftTop{ getLeft(topT, static_cast<double>(i)) };
                wxCoord tRightTop{ getRight(topT, static_cast<double>(i)) };
                wxCoord tLeftBot{ getLeft(botT, static_cast<double>(i)) };
                wxCoord tRightBot{ getRight(botT, static_cast<double>(i)) };
                if (tLeftTop == tRightTop)
                    {
                    tLeftTop -= 1;
                    tRightTop += 1;
                    }
                if (tLeftBot == tRightBot)
                    {
                    tLeftBot -= 1;
                    tRightBot += 1;
                    }
                const std::array<wxPoint, 4> targetPts{ wxPoint{ tLeftTop, yTop },
                                                        wxPoint{ tRightTop, yTop },
                                                        wxPoint{ tRightBot, yBottom },
                                                        wxPoint{ tLeftBot, yBottom } };
                const wxBrush& srcBrush{ GetBars()[i].GetBlocks().front().GetBrush() };
                const wxColour ghostCol{ Colors::ColorContrast::ChangeOpacity(
                    srcBrush.GetColour().IsOk() ?
                        srcBrush.GetColour() :
                        Colors::ColorBrewer::GetColor(Colors::Color::LightGray),
                    m_targetGhostOpacity) };
                wxBrush ghostBrush{ srcBrush };
                ghostBrush.SetColour(ghostCol);
                auto ghostPoly =
                    std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{ wxString{} }
                                                              .Pen(*wxTRANSPARENT_PEN)
                                                              .Brush(ghostBrush)
                                                              .Scaling(GetScaling())
                                                              .ShowLabelWhenSelected(false),
                                                          targetPts);
                ghostPoly->SetShape(GraphItems::Polygon::PolygonShape::Irregular);
                ghostPoly->SetClippingRect(plotRect);
                // faint outline only when no target is wider. ghost is smaller and hidden.
                if (noTargetsWider)
                    {
                    ghostPoly->GetPen() =
                        wxPen{ Colors::ColorContrast::ChangeOpacity(
                                   Colors::ColorBrewer::GetColor(Colors::Color::Black), 40),
                               1, wxPENSTYLE_DOT };
                    }
                return ghostPoly;
            };

            // helper to build actual polygon for this stage
            auto buildActualPoly = [&]() -> std::unique_ptr<GraphItems::Polygon>
            {
                wxCoord leftTop{ getLeft(curVal, static_cast<double>(i)) };
                wxCoord rightTop{ getRight(curVal, static_cast<double>(i)) };
                wxCoord leftBot{ getLeft(nextVal, static_cast<double>(i)) };
                wxCoord rightBot{ getRight(nextVal, static_cast<double>(i)) };
                if (leftTop == rightTop)
                    {
                    leftTop -= 1;
                    rightTop += 1;
                    }
                if (leftBot == rightBot)
                    {
                    leftBot -= 1;
                    rightBot += 1;
                    }
                const std::array<wxPoint, 4> pts{ wxPoint{ leftTop, yTop },
                                                  wxPoint{ rightTop, yTop },
                                                  wxPoint{ rightBot, yBottom },
                                                  wxPoint{ leftBot, yBottom } };
                const auto& bar{ GetBars()[i] };
                const auto& block{ bar.GetBlocks().front() };
                wxBrush blockBrush{ block.GetBrush() };
                if (blockBrush.GetColour().IsOk() && blockBrush.GetColour().IsOpaque())
                    {
                    blockBrush.SetColour(Colors::ColorContrast::ChangeOpacity(
                        blockBrush.GetColour(), bar.GetOpacity()));
                    }
                const wxColour blockCol{ blockBrush.GetColour() };
                const wxColour lightCol{ block.GetLightenedColor() };
                auto poly = std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo{ block.GetSelectionLabel().GetText() }
                        .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                        .Brush(blockBrush)
                        .Scaling(GetScaling())
                        .ShowLabelWhenSelected(true),
                    pts);
                poly->GetPen() = Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                     *wxTRANSPARENT_PEN :
                                     wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
                if (block.GetOutlinePen().IsOk())
                    {
                    poly->GetPen() = block.GetOutlinePen();
                    }
                if (AreExplanationsShown() && i == bottleneckIdx)
                    {
                    poly->GetPen() =
                        wxPen{ Colors::ColorContrast::ChangeOpacity(
                                   Colors::ColorBrewer::GetColor(Colors::Color::Red), 140),
                               static_cast<int>(bottleneckOutlinePenWidthDIPs), wxPENSTYLE_DOT };
                    }
                if (bar.GetEffect() == BoxEffect::FadeFromBottomToTop)
                    {
                    poly->GetBrush() = wxNullBrush;
                    poly->SetBackgroundFill(
                        Colors::GradientFill(blockCol, lightCol, FillDirection::East));
                    }
                else if (bar.GetEffect() == BoxEffect::FadeFromTopToBottom)
                    {
                    poly->GetBrush() = wxNullBrush;
                    poly->SetBackgroundFill(
                        Colors::GradientFill(blockCol, lightCol, FillDirection::West));
                    }
                else if (bar.GetEffect() == BoxEffect::Glassy)
                    {
                    poly->GetBrush() = wxNullBrush;
                    poly->SetBackgroundFill(
                        Colors::GradientFill(blockCol, blockCol, FillDirection::South));
                    }
                // funnel style glassy overrides with 3D sheen
                if (GetFunnelStyle() == FunnelStyle::Glassy)
                    {
                    poly->GetBrush() = wxNullBrush;
                    const wxColour topCol{ blockCol.ChangeLightness(135) };
                    const wxColour bottomCol{ blockCol.ChangeLightness(90) };
                    poly->SetBackgroundFill(
                        Colors::GradientFill(topCol, bottomCol, FillDirection::South));
                    // subtle inner stroke for 3D edge
                    if (!poly->GetPen().IsOk() || poly->GetPen().GetColour() == *wxBLACK)
                        {
                        poly->GetPen() = wxPen{ blockCol.ChangeLightness(70), 1 };
                        }
                    }
                poly->SetShape(GraphItems::Polygon::PolygonShape::Irregular);
                poly->SetClippingRect(plotRect);
                return poly;
            };

            // glossy sheen highlight at top of trapezoid
            auto buildGlossySheen = [&]() -> std::unique_ptr<GraphItems::Polygon>
            {
                const wxCoord leftTopH{ getLeft(curVal, static_cast<double>(i)) };
                const wxCoord rightTopH{ getRight(curVal, static_cast<double>(i)) };
                const wxCoord leftBotH{ getLeft(nextVal, static_cast<double>(i)) };
                const wxCoord rightBotH{ getRight(nextVal, static_cast<double>(i)) };
                constexpr double highlightT{ 0.35 };
                const wxCoord leftMidH{ static_cast<wxCoord>(leftTopH +
                                                             (leftBotH - leftTopH) * highlightT) };
                const wxCoord rightMidH{ static_cast<wxCoord>(rightTopH + (rightBotH - rightTopH) *
                                                                              highlightT) };
                const wxCoord yMidH{ yTop + static_cast<wxCoord>(stageHeight * highlightT) };
                const std::array<wxPoint, 4> hlPts{ wxPoint{ leftTopH, yTop },
                                                    wxPoint{ rightTopH, yTop },
                                                    wxPoint{ rightMidH, yMidH },
                                                    wxPoint{ leftMidH, yMidH } };
                auto hlPoly =
                    std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{ wxString{} }
                                                              .Pen(*wxTRANSPARENT_PEN)
                                                              .Brush(wxNullBrush)
                                                              .Scaling(GetScaling())
                                                              .ShowLabelWhenSelected(false),
                                                          hlPts);
                hlPoly->SetShape(GraphItems::Polygon::PolygonShape::Irregular);
                hlPoly->SetBackgroundFill(Colors::GradientFill(
                    Colors::ColorContrast::ChangeOpacity(
                        Colors::ColorBrewer::GetColor(Colors::Color::White), 28),
                    Colors::ColorContrast::ChangeOpacity(
                        Colors::ColorBrewer::GetColor(Colors::Color::White), 0),
                    FillDirection::South));
                hlPoly->SetClippingRect(plotRect);
                return hlPoly;
            };

            // value decal centered inside trapezoid
            auto buildValueDecal = [&]() -> std::unique_ptr<GraphItems::Label>
            {
                const wxString valStr{ wxNumberFormatter::ToString(
                    curVal, 0, Settings::GetDefaultNumberFormat()) };
                auto decal = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo{ valStr }
                        .Pen(wxNullPen)
                        .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                            GetBars()[i].GetBlocks().front().GetBrush().GetColour()))
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Padding(2, 2, 2, 2));
                decal->GetFont().MakeSmaller();
                decal->SetTextAlignment(TextAlignment::Centered);
                decal->SetAnchoring(Anchoring::Center);
                const wxCoord leftTop{ getLeft(curVal, static_cast<double>(i)) };
                const wxCoord rightTop{ getRight(curVal, static_cast<double>(i)) };
                const wxCoord leftBot{ getLeft(nextVal, static_cast<double>(i)) };
                const wxCoord rightBot{ getRight(nextVal, static_cast<double>(i)) };
                const wxCoord midX{ (leftTop + rightTop + leftBot + rightBot) / 4 };
                decal->SetAnchorPoint(wxPoint{ midX, yCenter });
                decal->SetShadowType(ShadowType::NoDisplay);
                decal->SetSelectable(false);
                const wxRect decalBox{ decal->GetBoundingBox(dc) };
                const wxCoord decalGap{ static_cast<wxCoord>(
                    std::round(decalBox.GetHeight() * 0.2)) };
                const wxCoord funnelWidthAtCenter{
                    ((leftTop + leftBot) / 2 < (rightTop + rightBot) / 2) ?
                        ((rightTop + rightBot) / 2 - (leftTop + leftBot) / 2) :
                        2
                };
                if (decalBox.GetWidth() > funnelWidthAtCenter - decalGap)
                    {
                    decal->SetAnchoring(Anchoring::Center);
                    const wxCoord rightMid{ (rightTop + rightBot) / 2 };
                    decal->SetAnchorPoint(
                        wxPoint{ static_cast<int>(rightMid + decalGap), yCenter });
                    decal->SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black));
                    decal->SetFontBackgroundColor(
                        Colors::ColorBrewer::GetColor(Colors::Color::White));
                    decal->GetGraphItemInfo()
                        .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                        .Outline(true, true, true, true);
                    }
                return decal;
            };

            const bool hasGhost{ std::isfinite(curTarget) };
            if (!noTargetsWider)
                {
                // ghost behind
                if (hasGhost)
                    {
                    const double topT{ curTarget };
                    const double botT{ !std::isfinite(nextTarget) ? curTarget : nextTarget };
                    AddObject(buildGhostPoly(topT, botT));
                    }
                AddObject(buildActualPoly());
                if (GetFunnelStyle() == FunnelStyle::Glassy)
                    {
                    AddObject(buildGlossySheen());
                    }
                AddObject(buildValueDecal());
                }
            else
                {
                // no targets wider. ghost is smaller, draw on top with faint outline.
                auto actualPoly{ buildActualPoly() };
                // need ghost values
                const double topT{ curTarget };
                const double botT{ !std::isfinite(nextTarget) ? curTarget : nextTarget };
                AddObject(std::move(actualPoly));
                if (GetFunnelStyle() == FunnelStyle::Glassy)
                    {
                    AddObject(buildGlossySheen());
                    }
                if (hasGhost)
                    {
                    AddObject(buildGhostPoly(topT, botT));
                    }
                // decal on top of ghost
                AddObject(buildValueDecal());
                }
            }

        // Conversion labels between stages.
        // Build all labels first to homogenize scaling.
        struct ConvLabelInfo
            {
            std::unique_ptr<GraphItems::Label> label;
            wxCoord midY{ 0 };
            wxCoord rightW{ 0 };
            bool nearBottleneck{ false };
            };

        std::vector<ConvLabelInfo> conversionLabels;
        double commonLabelScale{ 1.0 };
        if (m_showConversionLabels)
            {
            conversionLabels.reserve(n > 0 ? n - 1 : 0);
            for (size_t i = 1; i < n; ++i)
                {
                const double prevVal{ GetBars()[i - 1].GetLength() };
                const double curVal{ GetBars()[i].GetLength() };
                if (prevVal == 0)
                    {
                    continue;
                    }
                const double conv{ safe_divide<double>(curVal, prevVal) * 100.0 };
                const double drop{ prevVal - curVal };
                const wxString convStr{ wxString::Format(
                    _DT(L"<span style='font-weight:bold;'>%s%%</span>"),
                    wxNumberFormatter::ToString(
                        conv, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes)) };
                const wxString dropStr{ wxNumberFormatter::ToString(
                    drop, 0, Settings::GetDefaultNumberFormat()) };
                /* TRANSLATORS: Conversion label: %s is the conversion percent,
                   %s is the drop count. */
                const wxString labelText{ wxString::Format(_(L"Converted %s (%s drop)"), convStr,
                                                           dropStr) };

                // Y midway between stages
                wxPoint midPrev{ -1, -1 }, midCur{ -1, -1 };
                GetPhysicalCoordinates(centerDataX, static_cast<double>(i - 1), midPrev);
                GetPhysicalCoordinates(centerDataX, static_cast<double>(i), midCur);
                wxCoord midY{ 0 };
                if (midPrev.y != -1 && midCur.y != -1)
                    {
                    midY = (midPrev.y + midCur.y) / 2;
                    }
                else
                    {
                    const wxCoord interval{ static_cast<wxCoord>(intervalPhys) };
                    wxPoint midCurFallback{ -1, -1 };
                    GetPhysicalCoordinates(centerDataX, static_cast<double>(i), midCurFallback);
                    if (midCurFallback.y != -1)
                        {
                        midY = midCurFallback.y - (interval / 2);
                        }
                    else
                        {
                        midY = plotRect.GetTop() + static_cast<wxCoord>(std::round(
                                                       safe_divide<double>(static_cast<double>(i),
                                                                           static_cast<double>(n)) *
                                                       static_cast<double>(plotRect.GetHeight())));
                        }
                    }

                const double belowVal{ (i + 1 < n) ? GetBars()[i + 1].GetLength() : curVal };

                const bool nearBottleneck{ AreExplanationsShown() &&
                                           (i == bottleneckIdx || i - 1 == bottleneckIdx) };

                auto convLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo{ labelText }
                        .Pen(wxNullPen)
                        .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Padding(1, 2, 1, 2));
                convLabel->EnableMarkup(true);
                convLabel->GetFont().MakeSmaller().MakeSmaller();
                const wxRect measuredBox{ convLabel->GetBoundingBox(dc) };

                // the label straddles the boundary, so widen it toward however far the
                // stage above and below actually taper within the label's own height
                const double halfLabelFrac{ std::min(
                    1.0, safe_divide<double>(measuredBox.GetHeight() / 2.0, stageHeightPhys)) };
                double widthAtBoundary{ curVal };
                widthAtBoundary =
                    std::max(widthAtBoundary, curVal + ((prevVal - curVal) * halfLabelFrac));
                widthAtBoundary =
                    std::max(widthAtBoundary, curVal + ((belowVal - curVal) * halfLabelFrac));
                if (i < m_targetValues.size() && std::isfinite(m_targetValues[i]))
                    {
                    widthAtBoundary = std::max(widthAtBoundary, m_targetValues[i]);
                    }
                else if ((i - 1) < m_targetValues.size() && std::isfinite(m_targetValues[i - 1]))
                    {
                    widthAtBoundary = std::max(widthAtBoundary, m_targetValues[i - 1]);
                    }
                const wxCoord rightW{ getRight(widthAtBoundary, static_cast<double>(i)) };

                wxCoord padding{ static_cast<wxCoord>(std::round(measuredBox.GetHeight() * 0.3)) };
                if (nearBottleneck)
                    {
                    padding += static_cast<wxCoord>(
                        std::ceil(ScaleToScreenAndCanvas(bottleneckOutlinePenWidthDIPs)));
                    }
                const wxCoord availableWidthRight{ plotRect.GetRight() +
                                                   static_cast<wxCoord>(
                                                       std::round(plotRect.GetWidth() * 0.05)) -
                                                   (rightW + padding) };
                double neededScale{ 1.0 };
                if (measuredBox.GetWidth() > 0 && availableWidthRight > 0 &&
                    measuredBox.GetWidth() > availableWidthRight)
                    {
                    neededScale = safe_divide<double>(availableWidthRight, measuredBox.GetWidth());
                    }
                commonLabelScale = std::min(commonLabelScale, neededScale);
                conversionLabels.push_back({ std::move(convLabel), midY, rightW, nearBottleneck });
                }
            }

        // overall conversion rate at bottom
        std::unique_ptr<GraphItems::Label> overallLabel{ nullptr };
        wxCoord overallY{ 0 };
        wxCoord overallMidX{ 0 };
        if (n > 1)
            {
            const double topValOverall{ GetBars()[0].GetLength() };
            const double bottomValOverall{ GetBars()[n - 1].GetLength() };
            if (topValOverall > 0 && std::isfinite(topValOverall) &&
                std::isfinite(bottomValOverall))
                {
                const double overallConv{ safe_divide<double>(bottomValOverall, topValOverall) *
                                          100.0 };
                const double overallDrop{ topValOverall - bottomValOverall };
                const wxString overallConvStr{ wxString::Format(
                    _DT(L"<span style='font-weight:bold;'>%s%%</span>"),
                    wxNumberFormatter::ToString(
                        overallConv, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes)) };
                const wxString overallDropStr{ wxNumberFormatter::ToString(
                    overallDrop, 0, Settings::GetDefaultNumberFormat()) };
                /* TRANSLATORS: Overall conversion label: %s is the overall conversion percent, %s
                   is the total drop. */
                const wxString overallText{ wxString::Format(
                    _(L"Overall Conversion Rate: %s (%s drop)"), overallConvStr, overallDropStr) };
                overallLabel = makeAnnotationLabel(overallText);
                const wxRect overallBox{ overallLabel->GetBoundingBox(dc) };
                // geometry for bottom of funnel
                const wxCoord yCenterLast{ getCenterY(static_cast<double>(n - 1)) };
                const wxCoord yBottomLast{ yCenterLast + (stageHeight / 2) };
                overallMidX = plotRect.GetLeft() + (plotRect.GetWidth() / 2);
                const wxCoord overallVerticalGap{ static_cast<wxCoord>(
                    std::round(overallBox.GetHeight() * 0.5)) };
                overallY = yBottomLast + overallVerticalGap;
                // width available is plot width, less a margin sized off the label's own height
                const wxCoord availableWidthOverall{ plotRect.GetWidth() - overallVerticalGap };
                double neededScaleOverall{ 1.0 };
                if (overallBox.GetWidth() > availableWidthOverall && availableWidthOverall > 0)
                    {
                    neededScaleOverall =
                        safe_divide<double>(availableWidthOverall, overallBox.GetWidth());
                    }
                if (overallBox.GetHeight() > stageHeight)
                    {
                    neededScaleOverall =
                        std::min(neededScaleOverall,
                                 safe_divide<double>(stageHeight, overallBox.GetHeight()));
                    }
                commonLabelScale = std::min(commonLabelScale, neededScaleOverall);
                }
            }

        // Explanations for top stage and target and bottleneck.
        // Gather left labels to homogenize with right.
        std::unique_ptr<GraphItems::Label> actualLabel{ nullptr };
        std::unique_ptr<GraphItems::Label> targetLabel{ nullptr };
        std::unique_ptr<GraphItems::Label> bottleneckLabel{ nullptr };
        wxCoord yCenterTop{ 0 };
        wxCoord leftActualTop{ 0 };
        size_t targetPointIdx{ n };
        double targetPointVal{ std::numeric_limits<double>::quiet_NaN() };
        wxCoord targetPointY{ 0 };
        wxCoord targetPointLeft{ 0 };
        bool hasTargetPoint{ false };
        wxCoord yCenterBottleneck{ 0 };
        wxCoord leftBottleneckCenter{ 0 };
        wxCoord leftAxisRight{ 0 };
        wxCoord funnelLeftActual{ 0 };
        if (AreExplanationsShown() && n > 0)
            {
            const double topVal{ GetBars()[0].GetLength() };
            const double topTargetVal{ (!m_targetValues.empty() &&
                                        std::isfinite(m_targetValues[0])) ?
                                           m_targetValues[0] :
                                           std::numeric_limits<double>::quiet_NaN() };
            const bool hasTopTarget{ std::isfinite(topTargetVal) };

            // geometry for top stage
            yCenterTop = getCenterY(0.0);

            leftActualTop = getLeft(topVal, 0.0);

            // find first ghost below top that is wider than actual
            targetPointY = yCenterTop;
            targetPointLeft = leftActualTop;
            auto findWiderTargetPoint = [&](const bool avoidBottleneck) -> bool
            {
                for (size_t i = 1; i < n; ++i)
                    {
                    if (avoidBottleneck && i == bottleneckIdx)
                        {
                        continue;
                        }
                    if (i < m_targetValues.size() && std::isfinite(m_targetValues[i]) &&
                        std::isfinite(GetBars()[i].GetLength()) &&
                        m_targetValues[i] > GetBars()[i].GetLength())
                        {
                        targetPointIdx = i;
                        targetPointVal = m_targetValues[i];
                        targetPointY = getCenterY(static_cast<double>(i));
                        targetPointLeft = getLeft(targetPointVal, static_cast<double>(i));
                        return true;
                        }
                    }
                return false;
            };
            // prefer a wider ghost that isn't the bottleneck stage, so labels do not overlap;
            // if the only wider ghost was the bottleneck, allow it
            hasTargetPoint = findWiderTargetPoint(true);
            if (!hasTargetPoint)
                {
                hasTargetPoint = findWiderTargetPoint(false);
                }
            if (!hasTargetPoint && hasTopTarget)
                {
                targetPointIdx = 0;
                targetPointVal = topTargetVal;
                targetPointY = yCenterTop;
                targetPointLeft = getLeft(targetPointVal, 0.0);
                hasTargetPoint = true;
                }

            // create left labels at base scale to measure
            actualLabel = makeAnnotationLabel(_(L"Actual"));

            if (hasTargetPoint)
                {
                targetLabel = makeAnnotationLabel(_(L"Target"));
                }

            if (bottleneckIdx < n)
                {
                const wxString bottleneckText{ _(L"Bottleneck (worst conversion rate)") };
                const double curVal{ GetBars()[bottleneckIdx].GetLength() };
                bottleneckLabel = makeAnnotationLabel(bottleneckText);

                // geometry for bottleneck stage
                yCenterBottleneck = getCenterY(static_cast<double>(bottleneckIdx));
                const double bottleneckNextVal{ (bottleneckIdx + 1 < n) ?
                                                    GetBars()[bottleneckIdx + 1].GetLength() :
                                                    curVal };
                leftBottleneckCenter =
                    (getLeft(curVal, static_cast<double>(bottleneckIdx)) +
                     getLeft(bottleneckNextVal, static_cast<double>(bottleneckIdx))) /
                    2;
                }

            // Measure left labels and update common scale.
            // Use same scale for left, right and overall.
            leftAxisRight = GetLeftYAxis().GetBoundingBox(dc).GetRight();
            funnelLeftActual = leftActualTop;
            auto updateCommonScaleForLabel =
                [&](GraphItems::Label* label, const wxCoord availableWidth, const wxCoord maxHeight)
            {
                if (label == nullptr)
                    {
                    return;
                    }
                const wxRect box{ label->GetBoundingBox(dc) };
                double neededScale{ 1.0 };
                if (box.GetHeight() > maxHeight && maxHeight > 0)
                    {
                    neededScale =
                        std::min(neededScale, safe_divide<double>(maxHeight, box.GetHeight()));
                    }
                wxCoord availableWidthForLabel{ availableWidth };
                if (availableWidthForLabel > 0 && box.GetWidth() > availableWidthForLabel)
                    {
                    const double wScale{ safe_divide<double>(availableWidthForLabel,
                                                             box.GetWidth()) };
                    neededScale = std::min(neededScale, wScale);
                    }
                commonLabelScale = std::min(commonLabelScale, neededScale);
            };

            const wxCoord actualAxisGap{ static_cast<wxCoord>(
                std::round(actualLabel->GetBoundingBox(dc).GetHeight() * 0.5)) };
            wxCoord availableWidthActual{ funnelLeftActual - leftAxisRight - actualAxisGap };
            if (availableWidthActual < 0)
                {
                availableWidthActual = 0;
                }
            updateCommonScaleForLabel(actualLabel.get(), availableWidthActual, stageHeight);

            if (targetLabel)
                {
                const wxCoord funnelLeftTarget{ targetPointLeft };
                const wxCoord targetAxisGap{ static_cast<wxCoord>(
                    std::round(targetLabel->GetBoundingBox(dc).GetHeight() * 0.5)) };
                wxCoord availableWidthTarget{ funnelLeftTarget - leftAxisRight - targetAxisGap };
                if (availableWidthTarget < 0)
                    {
                    availableWidthTarget = 0;
                    }
                updateCommonScaleForLabel(targetLabel.get(), availableWidthTarget, stageHeight);
                }

            if (bottleneckLabel)
                {
                const wxCoord bottleneckAxisGap{ static_cast<wxCoord>(
                    std::round(bottleneckLabel->GetBoundingBox(dc).GetHeight() * 0.5)) };
                wxCoord availableWidthBottleneck{ leftBottleneckCenter - leftAxisRight -
                                                  bottleneckAxisGap };
                if (availableWidthBottleneck < 0)
                    {
                    availableWidthBottleneck = 0;
                    }
                updateCommonScaleForLabel(bottleneckLabel.get(), availableWidthBottleneck,
                                          stageHeight);
                }
            }

        // apply common scale to all labels
        if (commonLabelScale < 1.0)
            {
            for (auto& convInfo : conversionLabels)
                {
                convInfo.label->SetScaling(GetScaling() * commonLabelScale);
                }
            if (overallLabel)
                {
                overallLabel->SetScaling(GetScaling() * commonLabelScale);
                }
            if (actualLabel)
                {
                actualLabel->SetScaling(GetScaling() * commonLabelScale);
                }
            if (targetLabel)
                {
                targetLabel->SetScaling(GetScaling() * commonLabelScale);
                }
            if (bottleneckLabel)
                {
                bottleneckLabel->SetScaling(GetScaling() * commonLabelScale);
                }
            }

        // place conversion labels with final scale
        for (auto& convInfo : conversionLabels)
            {
            const wxRect box{ convInfo.label->GetBoundingBox(dc) };
            wxCoord padding{ static_cast<wxCoord>(std::round(box.GetHeight() * 0.3)) };
            if (convInfo.nearBottleneck)
                {
                padding += static_cast<wxCoord>(
                    std::ceil(ScaleToScreenAndCanvas(bottleneckOutlinePenWidthDIPs)));
                }
            convInfo.label->SetTextAlignment(TextAlignment::FlushLeft);
            convInfo.label->SetAnchoring(Anchoring::TopLeftCorner);
            convInfo.label->SetAnchorPoint(
                wxPoint{ convInfo.rightW + padding, convInfo.midY - (box.GetHeight() / 2) });
            convInfo.label->SetShadowType(ShadowType::NoDisplay);
            convInfo.label->SetSelectable(false);
            AddObject(std::move(convInfo.label));
            }

        // place overall conversion rate centered below funnel
        if (overallLabel)
            {
            const wxRect overallBox{ overallLabel->GetBoundingBox(dc) };
            overallLabel->SetTextAlignment(TextAlignment::Centered);
            overallLabel->SetAnchoring(Anchoring::Center);
            const wxCoord yAnchor{ overallY + (overallBox.GetHeight() / 2) };
            overallLabel->SetAnchorPoint(wxPoint{ overallMidX, yAnchor });
            overallLabel->SetShadowType(ShadowType::NoDisplay);
            overallLabel->SetSelectable(false);
            AddObject(std::move(overallLabel));
            }

        // place left explanation labels and their leader lines if enabled
        if (AreExplanationsShown() && n > 0 && actualLabel)
            {
            // dotted arrow from an explanation label to the point on the funnel it describes
            auto addLeaderLine = [&](const wxPoint& from, const wxPoint& to)
            {
                auto line = std::make_unique<GraphItems::Lines>(
                    wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black), 1, wxPENSTYLE_DOT },
                    GetScaling());
                line->SetLineStyle(LineStyle::Arrows);
                line->SetArrowheadScale(0.5);
                line->AddLine(from, to);
                AddObject(std::move(line));
            };

            wxRect actualBox{ actualLabel->GetBoundingBox(dc) };
            const wxCoord labelXActual{ leftAxisRight + static_cast<wxCoord>(std::round(
                                                            actualBox.GetHeight() * 0.3)) };
            const wxCoord startYActual{ yCenterTop - (actualBox.GetHeight() / 2) };
            actualLabel->SetAnchoring(Anchoring::TopLeftCorner);
            actualLabel->SetAnchorPoint(wxPoint{ labelXActual, startYActual });
            actualLabel->SetShadowType(ShadowType::NoDisplay);
            actualLabel->SetSelectable(false);
            const wxCoord actualCenterY{ startYActual + (actualBox.GetHeight() / 2) };

            wxRect targetBox{ targetLabel ? targetLabel->GetBoundingBox(dc) : wxRect{} };
            wxCoord targetLabelX{ labelXActual };
            wxCoord targetLabelY{ 0 };
            wxCoord targetCenterY{ 0 };
            if (targetLabel)
                {
                const wxCoord plotMidY{ plotRect.GetTop() + (plotRect.GetHeight() / 2) };
                if (targetPointIdx == 0)
                    {
                    targetLabelY = plotMidY - (targetBox.GetHeight() / 2);
                    }
                else
                    {
                    const wxCoord targetEdgeGap{ static_cast<wxCoord>(
                        std::round(targetBox.GetHeight() * 0.3)) };
                    targetLabelY = plotMidY - (targetBox.GetHeight() / 2);
                    if (targetLabelY + targetBox.GetHeight() > plotRect.GetBottom())
                        {
                        targetLabelY = static_cast<wxCoord>(plotRect.GetBottom() -
                                                            targetBox.GetHeight() - targetEdgeGap);
                        }
                    if (targetLabelY < plotRect.GetTop())
                        {
                        targetLabelY = plotRect.GetTop() + targetEdgeGap;
                        }
                    }
                targetLabel->SetAnchoring(Anchoring::TopLeftCorner);
                targetLabel->SetAnchorPoint(wxPoint{ targetLabelX, targetLabelY });
                targetLabel->SetShadowType(ShadowType::NoDisplay);
                targetLabel->SetSelectable(false);
                targetCenterY = targetLabelY + (targetBox.GetHeight() / 2);
                }

                // add lines after all shapes so they are on top actual line
                {
                const wxPoint labelRight{ actualLabel->GetBoundingBox(dc).GetRight(),
                                          actualCenterY };
                const wxCoord leftActualCenter{
                    (getLeft(GetBars()[0].GetLength(), 0.0) +
                     getLeft((n > 1 ? GetBars()[1].GetLength() : GetBars()[0].GetLength()), 0.0)) /
                    2
                };
                addLeaderLine(labelRight, wxPoint{ leftActualCenter, actualCenterY });
                }
            AddObject(std::move(actualLabel));

            if (targetLabel)
                {
                const wxPoint labelRight2{ targetLabel->GetBoundingBox(dc).GetRight(),
                                           targetCenterY };
                wxCoord leftTargetCenter{ targetPointLeft };
                if (targetPointIdx < n)
                    {
                    const double botTForCenter{ (targetPointIdx + 1 < m_targetValues.size() &&
                                                 std::isfinite(
                                                     m_targetValues[targetPointIdx + 1])) ?
                                                    m_targetValues[targetPointIdx + 1] :
                                                    targetPointVal };
                    const wxCoord leftBotCenter{ getLeft(
                        std::isfinite(botTForCenter) ? botTForCenter : targetPointVal,
                        static_cast<double>(targetPointIdx)) };
                    leftTargetCenter = (targetPointLeft + leftBotCenter) / 2;
                    }
                addLeaderLine(labelRight2, wxPoint{ leftTargetCenter, targetPointY });
                AddObject(std::move(targetLabel));
                }

            // bottleneck explanation pointing at stage with biggest drop
            if (bottleneckLabel)
                {
                wxRect bottleneckBox{ bottleneckLabel->GetBoundingBox(dc) };
                const wxCoord bottleneckEdgeGap{ static_cast<wxCoord>(
                    std::round(bottleneckBox.GetHeight() * 0.3)) };
                wxCoord bottleneckLabelX{ leftAxisRight + bottleneckEdgeGap };
                wxCoord bottleneckLabelY{ yCenterBottleneck - (bottleneckBox.GetHeight() / 2) };
                if (bottleneckLabelY < plotRect.GetTop())
                    {
                    bottleneckLabelY = plotRect.GetTop() + bottleneckEdgeGap;
                    }
                if (bottleneckLabelY + bottleneckBox.GetHeight() > plotRect.GetBottom())
                    {
                    bottleneckLabelY = static_cast<wxCoord>(
                        plotRect.GetBottom() - bottleneckBox.GetHeight() - bottleneckEdgeGap);
                    }
                bottleneckLabel->SetAnchoring(Anchoring::TopLeftCorner);
                bottleneckLabel->SetAnchorPoint(wxPoint{ bottleneckLabelX, bottleneckLabelY });
                bottleneckLabel->SetShadowType(ShadowType::NoDisplay);
                bottleneckLabel->SetSelectable(false);
                const wxCoord bottleneckCenterY{ bottleneckLabelY +
                                                 (bottleneckBox.GetHeight() / 2) };

                const wxPoint bottleneckLabelRight{ bottleneckLabel->GetBoundingBox(dc).GetRight(),
                                                    bottleneckCenterY };
                addLeaderLine(bottleneckLabelRight,
                              wxPoint{ leftBottleneckCenter, yCenterBottleneck });
                AddObject(std::move(bottleneckLabel));
                }
            }
        }

    //-----------------------------------
    void FunnelChart::SetAutoAccessibilityAttributes()
        {
        if (GetDataset() == nullptr || GetDataset()->GetRowCount() == 0 || GetBars().empty())
            {
            return;
            }
        wxString label{ _(L"A funnel chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            const auto& bar{ GetBars()[i] };
            const wxString stage{ bar.GetAxisLabel().GetText() };
            const wxString valStr{ wxNumberFormatter::ToString(
                bar.GetLength(), 0, Settings::GetDefaultNumberFormat()) };
            if (i == 0)
                {
                label += L". " + wxString::Format(_DT(L"%s: %s"), stage, valStr);
                }
            else
                {
                const double prev{ GetBars()[i - 1].GetLength() };
                const double conv{ prev != 0 ? safe_divide<double>(bar.GetLength(), prev) * 100.0 :
                                               0.0 };
                const wxString convStr{ wxNumberFormatter::ToString(
                    conv, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes) };
                /* TRANSLATORS: Funnel stage accessibility description: first %s is the stage
                   name, second %s is its value, third %s is its conversion percent from the
                   previous stage. */
                label += L". " +
                         wxString::Format(_(L"%s: %s (%s%% of previous)"), stage, valStr, convStr);
                }
            if (i < m_targetValues.size() && std::isfinite(m_targetValues[i]))
                {
                const wxString tgtStr{ wxNumberFormatter::ToString(
                    m_targetValues[i], 0, Settings::GetDefaultNumberFormat()) };
                label += wxString::Format(_(L", target %s"), tgtStr);
                }
            }
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");
        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
