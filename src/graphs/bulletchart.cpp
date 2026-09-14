///////////////////////////////////////////////////////////////////////////////
// Name:        bulletchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "bulletchart.h"
#include "../base/lines.h"
#include "../base/polygon.h"
#include "../base/settings.h"
#include "../base/shapes.h"
#include "../math/mathematics.h"
#include <array>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::BulletChart, Wisteria::Graphs::BarChart)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    BulletChart::BulletChart(Wisteria::Canvas * canvas) : BarChart(canvas)
        {
        SetBarOrientation(Orientation::Horizontal);
        ApplyAxisAppearance();
        // a row's bar length is the qualitative range span, not meaningful data to
        // display as a label
        SetBinLabelDisplay(BinLabelDisplay::NoDisplay);
        }

    //----------------------------------------------------------------
    void BulletChart::ApplyAxisAppearance()
        {
        // only the primary scaling axis (with the range brackets) is shown
        GetOppositeScalingAxis().Show(false);
        GetOppositeBarAxis().Show(false);
        // each row shows its KPI label (e.g., "Employee Satisfaction"), with no axis line
        // or tick marks of its own
        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetBarAxis().GetAxisLinePen() = wxNullPen;
        GetBarAxis().SetTickMarkDisplay(GraphItems::Axis::TickMark::DisplayType::NoDisplay);
        GetScalingAxis().GetGridlinePen() = wxNullPen;
        GetScalingAxis().SetTickMarkDisplay(GraphItems::Axis::TickMark::DisplayType::Inner);
        // tie the axis line/tick color to the label font color, rather than an
        // unrelated hardcoded pen color
        GetScalingAxis().GetAxisLinePen().SetColour(
            Colors::ColorContrast::ShadeOrTint(GetScalingAxis().GetFontColor()));
        }

    //----------------------------------------------------------------
    void BulletChart::SetActualBarColor(const wxColour& color)
        {
        m_actualBarBrush = wxBrush{ color };
        m_actualBarPen = wxPen{ Colors::ColorContrast::Shade(color), 1 };
        }

    //----------------------------------------------------------------
    void BulletChart::SetTargetTickColor(const wxColour& color)
        {
        m_targetTickPen.SetColour(color);
        }

    //----------------------------------------------------------------
    wxColour BulletChart::BlendColors(const wxColour& start, const wxColour& end, double proportion)
        {
        proportion = std::clamp(proportion, 0.0, 1.0);
        const auto blendChannel = [proportion](const unsigned char from, const unsigned char to)
        { return static_cast<unsigned char>(std::lround(from + ((to - from) * proportion))); };
        return wxColour{ blendChannel(start.Red(), end.Red()),
                         blendChannel(start.Green(), end.Green()),
                         blendChannel(start.Blue(), end.Blue()) };
        }

    //----------------------------------------------------------------
    void BulletChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                              const wxString& labelColumnName, const wxString& actualColumnName,
                              const wxString& targetColumnName)
        {
        SetDataset(data);
        m_labelColumnName = labelColumnName;
        m_actualColumnName = actualColumnName;
        m_targetColumnName = targetColumnName;
        GetSelectedIds().clear();
        m_rows.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto actualColumn = GetContinuousColumn(m_actualColumnName);
        const auto targetColumn = GetContinuousColumn(m_targetColumnName);

        // labels can come from the ID column or a categorical column
        std::vector<wxString> labels(GetDataset()->GetRowCount());
        if (GetDataset()->GetIdColumn().GetName().CmpNoCase(m_labelColumnName) == 0)
            {
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                labels[i] = GetDataset()->GetIdColumn().GetValue(i);
                }
            }
        else
            {
            const auto labelColumn = GetCategoricalColumn(m_labelColumnName);
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                labels[i] = labelColumn->GetLabelFromID(labelColumn->GetValue(i));
                }
            }

        m_rows.reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            m_rows.push_back({ labels[i], static_cast<double>(m_rows.size() + 1),
                               actualColumn->GetValue(i), targetColumn->GetValue(i) });
            }

        RebuildRows();
        }

    //----------------------------------------------------------------
    void BulletChart::SetRanges(std::vector<Range> ranges)
        {
        m_ranges = std::move(ranges);
        RebuildRows();
        }

    //----------------------------------------------------------------
    void BulletChart::RebuildRows()
        {
        ClearBars();
        // ClearBars() resets the axes, so recreate axes
        ApplyAxisAppearance();
        RebuildRangeBrackets();

        if (m_rows.empty())
            {
            return;
            }

        // full extent of a row's slot: the ranges' total span, or the largest
        // actual/target value seen if no ranges are set
        double fullExtent{ 0 };
        if (!m_ranges.empty())
            {
            fullExtent = m_ranges.back().m_end;
            }
        else
            {
            for (const auto& row : m_rows)
                {
                if (std::isfinite(row.m_actual))
                    {
                    fullExtent = std::max(fullExtent, row.m_actual);
                    }
                if (std::isfinite(row.m_target))
                    {
                    fullExtent = std::max(fullExtent, row.m_target);
                    }
                }
            }

        // Invisible placeholder bars, reserving each row's slot/position/axis label and
        // driving the scaling axis's auto-fit. The visible range bands are hand-drawn as
        // rounded RangeSegments in RecalcSizes() instead.
        for (const auto& row : m_rows)
            {
            // No custom width: that would throw off AddBar()'s bar-axis range calculation
            // and misalign the row's custom label from its integer axis position.
            //
            // The outline pen's colour (not just its style) must be alpha-transparent,
            // or BarChart substitutes a solid contrasting outline for it.
            Bar theBar{ row.m_axisPosition,
                        { BarBlock{
                            BarBlockInfo(fullExtent)
                                .OutlinePen(wxPen{ wxColour{ 0, 0, 0, wxALPHA_TRANSPARENT } }) } },
                        wxString{},
                        Wisteria::GraphItems::Label{ row.m_label },
                        BoxEffect::Solid,
                        wxALPHA_TRANSPARENT };
            AddBar(theBar);
            }
        }

    //----------------------------------------------------------------
    void BulletChart::RebuildRangeBrackets()
        {
        GetScalingAxis().ClearBrackets();
        if (IsShowingRangeLabels() && !m_ranges.empty())
            {
            double previousEnd{ GetScalingAxis().GetRange().first };
            for (const auto& range : m_ranges)
                {
                GetScalingAxis().AddBracket(Wisteria::GraphItems::Axis::AxisBracket(
                    previousEnd, range.m_end, (previousEnd + range.m_end) / 2.0, range.m_label));
                previousEnd = range.m_end;
                }
            }
        }

    //----------------------------------------------------------------
    wxRect BulletChart::RangeSegment::Draw(wxDC & dc) const
        {
        const Wisteria::GraphItems::GraphicsContextFallback gcf{ &dc, m_rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return m_rect;
            }

        const double radius{ std::min(Settings::GetBoxRoundedCornerRadius(),
                                      std::min(m_rect.GetWidth(), m_rect.GetHeight()) / 2.0) };

        const double left{ static_cast<double>(m_rect.GetLeft()) };
        const double top{ static_cast<double>(m_rect.GetTop()) };
        const double right{ static_cast<double>(m_rect.GetRight()) };
        const double bottom{ static_cast<double>(m_rect.GetBottom()) };

        const double topLeftRadius{ m_roundLeftCorners ? radius : 0.0 };
        const double bottomLeftRadius{ m_roundLeftCorners ? radius : 0.0 };
        const double topRightRadius{ m_roundRightCorners ? radius : 0.0 };
        const double bottomRightRadius{ m_roundRightCorners ? radius : 0.0 };

        // outline clockwise from the top-left, rounding only the corners this segment owns
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(left + topLeftRadius, top);
        path.AddLineToPoint(right - topRightRadius, top);
        if (topRightRadius > 0)
            {
            path.AddArc(right - topRightRadius, top + topRightRadius, topRightRadius,
                        geometry::degrees_to_radians(-90), geometry::degrees_to_radians(0), true);
            }
        path.AddLineToPoint(right, bottom - bottomRightRadius);
        if (bottomRightRadius > 0)
            {
            path.AddArc(right - bottomRightRadius, bottom - bottomRightRadius, bottomRightRadius,
                        geometry::degrees_to_radians(0), geometry::degrees_to_radians(90), true);
            }
        path.AddLineToPoint(left + bottomLeftRadius, bottom);
        if (bottomLeftRadius > 0)
            {
            path.AddArc(left + bottomLeftRadius, bottom - bottomLeftRadius, bottomLeftRadius,
                        geometry::degrees_to_radians(90), geometry::degrees_to_radians(180), true);
            }
        path.AddLineToPoint(left, top + topLeftRadius);
        if (topLeftRadius > 0)
            {
            path.AddArc(left + topLeftRadius, top + topLeftRadius, topLeftRadius,
                        geometry::degrees_to_radians(180), geometry::degrees_to_radians(270), true);
            }
        path.CloseSubpath();

        gc->SetBrush(gc->CreateBrush(GetGraphItemInfo().GetBrush()));
        if (GetPen().IsOk())
            {
            gc->SetPen(gc->CreatePen(wxGraphicsPenInfo{
                GetPen().GetColour(), static_cast<double>(GetPen().GetWidth()) }));
            }
        else
            {
            gc->SetPen(*wxTRANSPARENT_PEN);
            }
        gc->DrawPath(path);

        return m_rect;
        }

    //----------------------------------------------------------------
    const BulletChart::RowInfo& BulletChart::GetTopmostRow() const noexcept
        {
        wxASSERT_MSG(!m_rows.empty(), L"No rows available when finding topmost bullet row!");
        return GetBarAxis().IsReversed() ? m_rows.front() : m_rows.back();
        }

    //----------------------------------------------------------------
    void BulletChart::AddValueCallouts(wxDC & dc, const RowInfo& row)
        {
        const auto formatValue = [this](const double value)
        {
            wxString text{ wxNumberFormatter::ToString(
                value, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes) };
            if (GetValueDisplayFormat() == BulletChartValueFormat::Percentage)
                {
                text += L"%";
                }
            return text;
        };

        const double axisPos{ row.m_axisPosition };
        const double aboveOffset{
            (GetRangeBarWidthProportion() / 2.0) * m_targetTickOvershootFactor + 0.15
        };
        const auto [scaleStart, scaleEnd] = GetScalingAxis().GetRange();

        const bool hasActual{ std::isfinite(row.m_actual) };
        const bool hasTarget{ std::isfinite(row.m_target) };
        if (!hasActual && !hasTarget)
            {
            return;
            }

        const wxRect graphRect{ GetBoundingBox(dc) };

        std::unique_ptr<Wisteria::GraphItems::Label> actualLabel;
        std::unique_ptr<Wisteria::GraphItems::Label> targetLabel;
        wxPoint actualLeaderPt{}, targetLeaderPt{};

        if (hasActual)
            {
            wxPoint actualLabelPt{};
            GetPhysicalCoordinates(std::clamp(row.m_actual, scaleStart, scaleEnd),
                                   axisPos + aboveOffset, actualLabelPt);
            GetPhysicalCoordinates(std::clamp(row.m_actual, scaleStart, scaleEnd), axisPos,
                                   actualLeaderPt);
            actualLabel = std::make_unique<Wisteria::GraphItems::Label>(
                Wisteria::GraphItems::GraphItemInfo{
                    wxString::Format(L"<span style='color: %s;'>%s</span>: %s",
                                     GetActualCalloutLabelColor().GetAsString(wxC2S_HTML_SYNTAX),
                                     _(L"Actual"), formatValue(row.m_actual)) }
                    .Pen(wxNullPen)
                    .AnchorPoint(actualLabelPt)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .LabelAlignment(TextAlignment::Centered));
            actualLabel->EnableMarkup(true);
            }

        if (hasTarget)
            {
            // the leader line points at the bottom of the target tick's overshoot
            // (where it protrudes below the range bar), not the row's center
            const double tickHalfWidth{ (GetRangeBarWidthProportion() / 2.0) *
                                        m_targetTickOvershootFactor };

            wxPoint targetTickPt{};
            GetPhysicalCoordinates(std::clamp(row.m_target, scaleStart, scaleEnd),
                                   axisPos + aboveOffset, targetTickPt);
            GetPhysicalCoordinates(std::clamp(row.m_target, scaleStart, scaleEnd),
                                   axisPos - tickHalfWidth, targetLeaderPt);
            targetLabel = std::make_unique<Wisteria::GraphItems::Label>(
                Wisteria::GraphItems::GraphItemInfo{
                    wxString::Format(L"<span style='color: %s;'>%s</span>: %s",
                                     GetTargetCalloutLabelColor().GetAsString(wxC2S_HTML_SYNTAX),
                                     _(L"Target"), formatValue(row.m_target)) }
                    .Pen(wxNullPen)
                    .AnchorPoint(targetTickPt)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .LabelAlignment(TextAlignment::Centered));
            targetLabel->EnableMarkup(true);

            // offset to the right of the tick line by default (so the leader line
            // doesn't cut through the text), falling back to the left if there's
            // no room on the right
            const wxRect targetBox{ targetLabel->GetBoundingBox(dc) };
            const wxCoord sideGap{ targetBox.GetHeight() / 2 };
            const wxCoord rightAnchorX{ targetTickPt.x + sideGap + (targetBox.GetWidth() / 2) };
            if (rightAnchorX + (targetBox.GetWidth() / 2) <= graphRect.GetRight())
                {
                targetLabel->SetAnchorPoint({ rightAnchorX, targetTickPt.y });
                }
            else
                {
                const wxCoord leftAnchorX{ targetTickPt.x - sideGap - (targetBox.GetWidth() / 2) };
                targetLabel->SetAnchorPoint({ leftAnchorX, targetTickPt.y });
                }
            }

            // nudge each label up if its box (which includes descender space) dips
            // into the range bar below it
            {
            wxPoint barTopPt{};
            GetPhysicalCoordinates(scaleStart, axisPos + (GetRangeBarWidthProportion() / 2.0),
                                   barTopPt);
            for (auto* label : { actualLabel.get(), targetLabel.get() })
                {
                if (label == nullptr)
                    {
                    continue;
                    }
                const wxRect labelBox{ label->GetBoundingBox(dc) };
                // a quarter of the label's own line height as breathing room, so the
                // clearance scales with the font size/DPI rather than a fixed pixel count
                const wxCoord margin{ labelBox.GetHeight() / 4 };
                const wxCoord overlap{ labelBox.GetBottom() - (barTopPt.y - margin) };
                if (overlap > 0)
                    {
                    label->SetAnchorPoint(label->GetAnchorPoint() - wxPoint(0, overlap));
                    }
                }
            }

        // if both labels still overlap, the target label's position is authoritative;
        // the actual label is the one that moves to clear it
        if (actualLabel != nullptr && targetLabel != nullptr)
            {
            const wxRect actualBox{ actualLabel->GetBoundingBox(dc) };
            const wxRect targetBox{ targetLabel->GetBoundingBox(dc) };
            // use the labels' own measured text height as the minimum gap, so it
            // scales with font size, DPI, and canvas scaling
            const wxCoord gap{ (actualBox.GetHeight() + targetBox.GetHeight()) / 2 };

            if (actualBox.GetLeft() <= targetBox.GetLeft())
                {
                const wxCoord overlap{ (actualBox.GetRight() + gap) - targetBox.GetLeft() };
                if (overlap > 0)
                    {
                    actualLabel->SetAnchorPoint(actualLabel->GetAnchorPoint() -
                                                wxPoint(overlap, 0));
                    }
                }
            else
                {
                const wxCoord overlap{ (targetBox.GetRight() + gap) - actualBox.GetLeft() };
                if (overlap > 0)
                    {
                    actualLabel->SetAnchorPoint(actualLabel->GetAnchorPoint() +
                                                wxPoint(overlap, 0));
                    }
                }
            }

        // keep whichever labels are present fully inside the graph's own drawing area
        for (auto* label : { actualLabel.get(), targetLabel.get() })
            {
            if (label == nullptr)
                {
                continue;
                }
            const wxRect labelBox{ label->GetBoundingBox(dc) };
            const wxCoord rightOverflow{ labelBox.GetRight() - graphRect.GetRight() };
            if (rightOverflow > 0)
                {
                label->SetAnchorPoint(label->GetAnchorPoint() - wxPoint(rightOverflow, 0));
                }
            const wxCoord leftOverflow{ graphRect.GetLeft() - labelBox.GetLeft() };
            if (leftOverflow > 0)
                {
                label->SetAnchorPoint(label->GetAnchorPoint() + wxPoint(leftOverflow, 0));
                }
            const wxCoord topOverflow{ graphRect.GetTop() - labelBox.GetTop() };
            if (topOverflow > 0)
                {
                label->SetAnchorPoint(label->GetAnchorPoint() + wxPoint(0, topOverflow));
                }
            }

        // the leader line starts at the bottom edge of the label's box (not its
        // center anchor point), so it appears beneath the text instead of
        // cutting through it
        const auto addWithLeaderLine =
            [this, &dc](std::unique_ptr<Wisteria::GraphItems::Label> label, const wxPoint& leaderPt)
        {
            const wxRect labelBox{ label->GetBoundingBox(dc) };
            const wxPoint leaderStartPt{ labelBox.GetLeft() + (labelBox.GetWidth() / 2),
                                         labelBox.GetBottom() };
            auto leaderLine = std::make_unique<Wisteria::GraphItems::Lines>(
                wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black), 1,
                      wxPenStyle::wxPENSTYLE_SOLID),
                GetScaling());
            leaderLine->SetDPIScaleFactor(GetDPIScaleFactor());
            leaderLine->SetLineStyle(LineStyle::Arrows);
            leaderLine->SetArrowheadScale(0.5);
            leaderLine->AddLine(leaderStartPt, leaderPt);
            AddObject(std::move(leaderLine));
            AddObject(std::move(label));
        };

        if (actualLabel != nullptr)
            {
            addWithLeaderLine(std::move(actualLabel), actualLeaderPt);
            }
        if (targetLabel != nullptr)
            {
            addWithLeaderLine(std::move(targetLabel), targetLeaderPt);
            }
        }

    //----------------------------------------------------------------
    void BulletChart::RecalcSizes(wxDC & dc)
        {
        BarChart::RecalcSizes(dc);

        if (m_rows.empty())
            {
            return;
            }

        const auto [scaleStart, scaleEnd] = GetScalingAxis().GetRange();

        // extra margin so the hand-drawn pill fully covers the invisible placeholder bar
        const wxCoord thicknessBleed{ static_cast<wxCoord>(
            std::ceil(ScaleToScreenAndCanvas(2.0))) };

        for (const auto& row : m_rows)
            {
            const double axisPos{ row.m_axisPosition };
            const double rangeHalfWidth{ GetRangeBarWidthProportion() / 2.0 };

            wxPoint rangeTopPt, rangeBottomPt;
            GetPhysicalCoordinates(scaleStart, axisPos + rangeHalfWidth, rangeTopPt);
            GetPhysicalCoordinates(scaleStart, axisPos - rangeHalfWidth, rangeBottomPt);

            if (m_ranges.empty())
                {
                wxPoint rangeStartPt, rangeEndPt;
                GetPhysicalCoordinates(scaleStart, axisPos, rangeStartPt);
                GetPhysicalCoordinates(scaleEnd, axisPos, rangeEndPt);
                wxRect rangeRect(rangeStartPt.x, rangeTopPt.y, rangeEndPt.x - rangeStartPt.x,
                                 rangeBottomPt.y - rangeTopPt.y);
                rangeRect.Inflate(0, thicknessBleed);
                AddObject(std::make_unique<RangeSegment>(
                    Wisteria::GraphItems::GraphItemInfo{}
                        .Brush(Colors::ColorBrewer::GetColor(Colors::Color::LightGray))
                        .Pen(wxNullPen),
                    rangeRect, /*roundLeftCorners*/ true, /*roundRightCorners*/ true));
                }
            else
                {
                // under GoalStatus, the whole row shades from one base color (met or
                // not met), not a shared gradient across rows; falls back to TwoTone
                // if there's no target to compare against
                const bool hasGoalData{ std::isfinite(row.m_actual) &&
                                        std::isfinite(row.m_target) };
                const auto [bandStartColor, bandEndColor] =
                    [this, &row, hasGoalData]() -> std::pair<wxColour, wxColour>
                {
                    if (GetRangeColorScheme() == BulletChartRangeColorScheme::TwoTone ||
                        !hasGoalData)
                        {
                        return { GetRangeStartColor(), GetRangeEndColor() };
                        }
                    const wxColour baseColor{ (row.m_actual >= row.m_target) ?
                                                  GetGoalSuccessColor() :
                                                  GetGoalFailureColor() };
                    return { Colors::ColorContrast::Shade(baseColor),
                             Colors::ColorContrast::Tint(baseColor) };
                }();

                double previousEnd{ scaleStart };
                for (size_t rangeIdx = 0; rangeIdx < m_ranges.size(); ++rangeIdx)
                    {
                    const auto& range{ m_ranges[rangeIdx] };
                    wxPoint segStartPt, segEndPt;
                    GetPhysicalCoordinates(std::clamp(previousEnd, scaleStart, scaleEnd), axisPos,
                                           segStartPt);
                    GetPhysicalCoordinates(std::clamp(range.m_end, scaleStart, scaleEnd), axisPos,
                                           segEndPt);
                    wxRect segRect(segStartPt.x, rangeTopPt.y, segEndPt.x - segStartPt.x,
                                   rangeBottomPt.y - rangeTopPt.y);
                    segRect.Inflate(0, thicknessBleed);
                    const double bandProportion{ (m_ranges.size() == 1) ?
                                                     1.0 :
                                                     static_cast<double>(rangeIdx) /
                                                         static_cast<double>(m_ranges.size() - 1) };
                    AddObject(std::make_unique<RangeSegment>(
                        Wisteria::GraphItems::GraphItemInfo{}
                            .Brush(BlendColors(bandStartColor, bandEndColor, bandProportion))
                            .Pen(wxNullPen),
                        segRect, /*roundLeftCorners*/ rangeIdx == 0,
                        /*roundRightCorners*/ rangeIdx == m_ranges.size() - 1));
                    previousEnd = range.m_end;
                    }
                }
            }

        // target ticks are added before the actual-value bars so that a bar drawn over
        // a tick covers it, rather than the tick line cutting across the bar
        auto targetTicks =
            std::make_unique<Wisteria::GraphItems::Lines>(GetTargetTickPen(), GetScaling());
        targetTicks->SetDPIScaleFactor(GetDPIScaleFactor());
        targetTicks->Reserve(m_rows.size());

        for (const auto& row : m_rows)
            {
            if (!std::isfinite(row.m_target))
                {
                continue;
                }

            const double axisPos{ row.m_axisPosition };
            const double tickHalfWidth{ (GetRangeBarWidthProportion() / 2.0) *
                                        m_targetTickOvershootFactor };

            wxPoint tickTopPt{}, tickBottomPt{}, targetPt{};
            GetPhysicalCoordinates(scaleStart, axisPos + tickHalfWidth, tickTopPt);
            GetPhysicalCoordinates(scaleStart, axisPos - tickHalfWidth, tickBottomPt);
            GetPhysicalCoordinates(std::clamp(row.m_target, scaleStart, scaleEnd), axisPos,
                                   targetPt);
            targetTicks->AddLine(wxPoint(targetPt.x, tickTopPt.y),
                                 wxPoint(targetPt.x, tickBottomPt.y));
            }

        AddObject(std::move(targetTicks));

        for (const auto& row : m_rows)
            {
            if (!std::isfinite(row.m_actual))
                {
                continue;
                }

            const double axisPos{ row.m_axisPosition };
            const double actualHalfWidth{ GetActualBarWidthProportion() / 2.0 };

            wxPoint actualTopPt{}, actualBottomPt{}, actualStartPt{}, actualEndPt{};
            GetPhysicalCoordinates(scaleStart, axisPos + actualHalfWidth, actualTopPt);
            GetPhysicalCoordinates(scaleStart, axisPos - actualHalfWidth, actualBottomPt);
            GetPhysicalCoordinates(scaleStart, axisPos, actualStartPt);
            GetPhysicalCoordinates(std::clamp(row.m_actual, scaleStart, scaleEnd), axisPos,
                                   actualEndPt);

            const wxRect actualRect(std::min(actualStartPt.x, actualEndPt.x), actualTopPt.y,
                                    std::abs(actualEndPt.x - actualStartPt.x),
                                    actualBottomPt.y - actualTopPt.y);
            std::array<wxPoint, 4> pts{};
            Wisteria::GraphItems::Polygon::GetRectPoints(actualRect, pts);
            AddObject(std::make_unique<Wisteria::GraphItems::Polygon>(
                Wisteria::GraphItems::GraphItemInfo{ row.m_label }
                    .Brush(GetActualBarBrush())
                    .Pen(GetActualBarPen())
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .Selectable(true),
                pts));
            }

        // single callout pair for the whole chart, anchored above the visually
        // topmost row, not one per row
        if (IsShowingValueCallouts())
            {
            AddValueCallouts(dc, GetTopmostRow());
            }
        }

    //----------------------------------------------------------------
    void BulletChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A bullet chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");

        label += L". ";
        label += wxString::Format(
            /* TRANSLATORS: bullet chart accessibility: KPI row count. %zu is the count. */
            _(L"%zu KPIs"), m_rows.size());

        const auto findRangeForValue = [this](const double value)
        {
            double previousEnd{ GetScalingAxis().GetRange().first };
            for (const auto& range : m_ranges)
                {
                if (is_within(value, previousEnd, range.m_end))
                    {
                    return range.m_label;
                    }
                previousEnd = range.m_end;
                }
            return wxString{};
        };

        for (const auto& row : m_rows)
            {
            label += L". ";
            label += row.m_label;
            if (std::isfinite(row.m_actual))
                {
                label += wxString::Format(
                    /* TRANSLATORS: bullet chart accessibility: actual value for a KPI row.
                       %s is the value. */
                    _(L", actual %s"),
                    wxNumberFormatter::ToString(row.m_actual, 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                const wxString rangeLabel{ findRangeForValue(row.m_actual) };
                if (!rangeLabel.empty())
                    {
                    label += wxString::Format(_(L" (%s)"), rangeLabel);
                    }
                }
            if (std::isfinite(row.m_target))
                {
                label += wxString::Format(
                    /* TRANSLATORS: bullet chart accessibility: target value for a KPI row.
                       %s is the value. */
                    _(L", target %s"),
                    wxNumberFormatter::ToString(row.m_target, 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
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
