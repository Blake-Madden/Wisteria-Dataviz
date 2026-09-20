///////////////////////////////////////////////////////////////////////////////
// Name:        pictograph.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pictograph.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::Pictograph, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    Pictograph::Pictograph(Canvas * canvas, const Icons::IconShape shape,
                           const Orientation orientation /*= Orientation::Vertical*/)
        : Graph2D(canvas), m_shape(shape),
          m_orientation(orientation == Orientation::Both ? Orientation::Vertical : orientation)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        }

    //----------------------------------------------------------------
    void Pictograph::SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& valueColumn, const wxString& labelColumn)
        {
        if (data == nullptr)
            {
            m_icons.clear();
            m_valueColumnName.clear();
            m_labelColumnName.clear();
            SetDataset(data);
            return;
            }

        const auto valueCol = data->GetContinuousColumn(valueColumn);
        if (valueCol == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': value column not found for pictograph."), valueColumn)
                    .ToUTF8());
            }

        const auto labelCol = data->GetCategoricalColumn(labelColumn);
        const bool hasCategoricalLabels{ labelCol != data->GetCategoricalColumns().cend() };
        const auto& idColumn = data->GetIdColumn();
        const bool hasIdLabels{ !hasCategoricalLabels &&
                                wxString{ idColumn.GetName() }.CmpNoCase(labelColumn) == 0 };
        if (!hasCategoricalLabels && !hasIdLabels)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': label column not found for pictograph."), labelColumn)
                    .ToUTF8());
            }

        size_t truncatedCount{ 0 };
        std::vector<IconInfo> icons;
        icons.reserve(std::min(data->GetRowCount(), MAX_ICONS));
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            const auto value = valueCol->GetValue(i);
            if (!std::isfinite(value) || value <= 0)
                {
                continue;
                }
            if (icons.size() >= MAX_ICONS)
                {
                ++truncatedCount;
                continue;
                }
            wxString label;
            if (hasCategoricalLabels)
                {
                label = labelCol->GetValueAsLabel(i);
                }
            else if (idColumn.GetRowCount() > i)
                {
                label = idColumn.GetValue(i);
                }
            icons.emplace_back(value, label);
            }

        SetDataset(data);
        m_icons = std::move(icons);
        m_valueColumnName = valueColumn;
        m_labelColumnName = labelColumn;

        // the note is only written over an empty caption (or the note itself),
        // and is removed again if the data no longer needs it
        const wxString truncationNote{ wxString::Format(
            _(L"Note: only the first %zu observations are being displayed."), MAX_ICONS) };
        if (truncatedCount > 0)
            {
            wxLogWarning(L"Pictograph limited to %zu observations; "
                         "%zu observations truncated.",
                         MAX_ICONS, truncatedCount);
            if (GetCaption().GetText().empty() || GetCaption().GetText() == truncationNote)
                {
                GetCaption().SetText(truncationNote);
                }
            }
        else if (GetCaption().GetText() == truncationNote)
            {
            GetCaption().SetText(wxString{});
            }
        }

    //----------------------------------------------------------------
    wxString Pictograph::FormatValue(const double value, const bool thousandsSeparator) const
        {
        const auto decimalsFor = [](const double num)
        {
            const double magnitude = std::abs(num);
            if (magnitude == 0 || magnitude >= 1)
                {
                return 2;
                }
            return std::clamp(1 - static_cast<int>(std::floor(std::log10(magnitude))), 2, 6);
        };
        const int numberStyle = wxNumberFormatter::Style_NoTrailingZeroes |
                                (thousandsSeparator ? wxNumberFormatter::Style_WithThousandsSep :
                                                      wxNumberFormatter::Style_None);

        if (m_valueFormat == NumberDisplay::Currency)
            {
            // cents are shown (always as two digits) only if the value has any
            return wxNumberFormatter::ToString(
                value, has_fractional_part(value) ? 2 : 0,
                (thousandsSeparator ? wxNumberFormatter::Style_WithThousandsSep :
                                      wxNumberFormatter::Style_None) |
                    wxNumberFormatter::Style_Currency | wxNumberFormatter::Style_CurrencySymbol);
            }
        if (m_valueFormat == NumberDisplay::Percentage)
            {
            return wxString::Format(
                /* TRANSLATORS: Percentage value and percentage symbol (%%).
                   '%%' can be changed and/or moved within string. */
                _(L"%s%%"),
                wxNumberFormatter::ToString(value * 100, decimalsFor(value * 100), numberStyle));
            }
        return wxNumberFormatter::ToString(value, decimalsFor(value), numberStyle);
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> Pictograph::CreateLegend(
        [[maybe_unused]] const LegendOptions& options)
        {
        return nullptr;
        }

    //----------------------------------------------------------------
    void Pictograph::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_icons.empty())
            {
            return;
            }

        const wxRect drawArea = GetPlotAreaBoundingBox();
        if (drawArea.IsEmpty())
            {
            return;
            }

        constexpr double ITEM_GAP_DIPS{ 4 };
        // an icon can be overlapped by at most this proportion of the smallest icon's size
        constexpr double MAX_OVERLAP_PROPORTION{ 0.75 };
        // vertical bars have their labels to the right of the icons, so the icons
        // take up at most this proportion of the plot area's width
        constexpr double MAX_ICON_COLUMN_PROPORTION{ 0.5 };
        // no icon's side is drawn more than this many times the smallest icon's side
        constexpr double MAX_SIDE_RATIO{ 100 };
        // labels are never scaled below this proportion of their default size
        constexpr double MIN_LABEL_FACTOR{ 0.05 };
        constexpr int LABEL_FACTOR_SEARCH_STEPS{ 20 };
        constexpr double LABEL_FACTOR_BACKOFF{ 0.95 };

        const bool isVertical{ m_orientation != Orientation::Horizontal };
        const double itemGap{ ScaleToScreenAndCanvas(ITEM_GAP_DIPS) };
        const auto iconCount = static_cast<double>(m_icons.size());

        // Areas are proportional to the values, so the sides are proportional
        // to the square roots of the values (relative to the smallest one),
        // but capped so that extreme ranges can't produce unbounded icon sizes.
        const double minValue = std::ranges::min_element(m_icons, {}, &IconInfo::m_value)->m_value;
        std::vector<double> ratios;
        ratios.reserve(m_icons.size());
        for (const auto& icon : m_icons)
            {
            ratios.push_back(
                std::min(std::sqrt(safe_divide(icon.m_value, minValue)), MAX_SIDE_RATIO));
            }
        const double sumRatio = std::accumulate(ratios.cbegin(), ratios.cend(), 0.0);
        const double maxRatio = *std::ranges::max_element(ratios);

        // the main axis is the direction that the icons are stacked along
        const auto mainAvailable =
            static_cast<double>(isVertical ? drawArea.GetHeight() : drawArea.GetWidth());
        const auto crossAvailable =
            static_cast<double>(isVertical ? drawArea.GetWidth() : drawArea.GetHeight());
        const double totalGaps{ (iconCount - 1) * itemGap };

        // plain, centered label
        const wxColour labelFontColor =
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor());
        const auto makeLabel = [this, &labelFontColor](const wxString& text, const double scaling)
        {
            auto label = std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo{ text }
                                                                 .Scaling(scaling)
                                                                 .DPIScaling(GetDPIScaleFactor())
                                                                 .Pen(wxNullPen)
                                                                 .FontColor(labelFontColor)
                                                                 .Anchoring(Anchoring::Center));
            label->SetShape(LabelShape::NoShape);
            label->SetBoxCorners(BoxCorners::Straight);
            label->SetShadowType(ShadowType::NoDisplay);
            return label;
        };

        std::vector<wxString> labelTexts;
        std::vector<double> naturalWidths;
        labelTexts.reserve(m_icons.size());
        naturalWidths.reserve(m_icons.size());
        double maxNaturalWidth{ 0 };
        double maxNaturalHeight{ 0 };
        for (const auto& icon : m_icons)
            {
            const wxString valueText = FormatValue(icon.m_value);
            labelTexts.push_back(icon.m_label.empty() ?
                                     valueText :
                                     wxString::Format(_DT(L"%s (%s)"), icon.m_label, valueText));
            const wxSize measured =
                makeLabel(labelTexts.back(), GetScaling())->GetBoundingBox(dc).GetSize();
            naturalWidths.push_back(static_cast<double>(measured.GetWidth()));
            maxNaturalWidth = std::max(maxNaturalWidth, naturalWidths.back());
            maxNaturalHeight =
                std::max(maxNaturalHeight, static_cast<double>(measured.GetHeight()));
            }

        struct Layout
            {
            double m_baseSize{ 0 };
            double m_overlap{ 0 };
            double m_mainStart{ 0 };
            double m_maxSide{ 0 };
            std::vector<double> m_sides;
            };

        // Sizes the icons, leaving room on the cross axis for the given number of label rows
        // (horizontal bars only, as vertical bars have their labels beside the icons).
        // The smallest icon grows to fill the plot area, up to the maximum size,
        // and shrinks down to the hard minimum when there isn't room.
        const auto computeLayout = [&](const double labelRows, const double rowHeight)
        {
            Layout layout;
            const double mainFit = safe_divide(mainAvailable - totalGaps, sumRatio);
            const double crossFit =
                isVertical ? safe_divide(crossAvailable * MAX_ICON_COLUMN_PROPORTION, maxRatio) :
                             safe_divide(crossAvailable - (labelRows * rowHeight), maxRatio);
            layout.m_baseSize = std::clamp(std::min(mainFit, crossFit),
                                           ScaleToScreenAndCanvas(HARD_MIN_ICON_SIZE_DIPS),
                                           ScaleToScreenAndCanvas(MAX_ICON_SIZE_DIPS));

            // if the icons still don't fit, then overlap consecutive ones by an equal amount
            const double neededLength = (layout.m_baseSize * sumRatio) + totalGaps;
            if (m_icons.size() > 1)
                {
                layout.m_overlap = std::min(
                    safe_divide(std::max(0.0, neededLength - mainAvailable), iconCount - 1),
                    layout.m_baseSize * MAX_OVERLAP_PROPORTION);
                }
            const double usedLength = neededLength - (layout.m_overlap * (iconCount - 1));
            layout.m_mainStart =
                static_cast<double>(isVertical ? drawArea.GetTop() : drawArea.GetLeft()) +
                ((mainAvailable - usedLength) * math_constants::half);
            layout.m_maxSide = layout.m_baseSize * maxRatio;
            for (const double ratio : ratios)
                {
                layout.m_sides.push_back(layout.m_baseSize * ratio);
                }
            return layout;
        };

        // the center of each icon along the main axis
        const auto iconCenters = [&](const Layout& layout)
        {
            std::vector<double> centers;
            centers.reserve(layout.m_sides.size());
            double cursor{ layout.m_mainStart };
            for (const double side : layout.m_sides)
                {
                centers.push_back(cursor + (side * math_constants::half));
                cursor += side + itemGap - layout.m_overlap;
                }
            return centers;
        };

        // The x-position of the center of a horizontal label, which is centered on its icon
        // but slid inward if it would otherwise stick out of the plot area.
        const auto labelCenterX = [&](const double iconCenter, const double labelWidth)
        {
            const auto areaLeft = static_cast<double>(drawArea.GetLeft());
            const double halfWidth = labelWidth * math_constants::half;
            return std::max(areaLeft + halfWidth,
                            std::min(iconCenter, areaLeft + drawArea.GetWidth() - halfWidth));
        };

        // Whether horizontal labels at the given proportion of their default size would overlap
        // (every @c step-th label shares a side of the icons) or not fit in the plot area.
        const auto labelsOverlap =
            [&](const std::vector<double>& centers, const double factor, const size_t step)
        {
            std::vector<double> lefts;
            std::vector<double> rights;
            lefts.reserve(centers.size());
            rights.reserve(centers.size());
            for (size_t i = 0; i < centers.size(); ++i)
                {
                const double width = naturalWidths[i] * factor;
                if (width > drawArea.GetWidth())
                    {
                    return true;
                    }
                const double centerX = labelCenterX(centers[i], width);
                lefts.push_back(centerX - (width * math_constants::half));
                rights.push_back(centerX + (width * math_constants::half));
                }
            for (size_t i = step; i < centers.size(); ++i)
                {
                if (lefts[i] < rights[i - step] + itemGap)
                    {
                    return true;
                    }
                }
            return false;
        };

        // Labels start at the default size. Horizontal labels are drawn under the icons,
        // then alternate under and over the icons if they overlap, and are only
        // scaled down if they still overlap.
        double labelFactor{ 1.0 };
        bool alternateLabels{ false };
        Layout layout = computeLayout(isVertical ? 0.0 : 1.0, maxNaturalHeight);
        if (isVertical)
            {
            // the labels are beside the icons, so they must fit between the neighboring
            // icons' centers and in the space to the right of the icons
            if (maxNaturalHeight > 0)
                {
                labelFactor = std::min(
                    labelFactor,
                    safe_divide(layout.m_baseSize + itemGap - layout.m_overlap, maxNaturalHeight));
                }
            if (maxNaturalWidth > 0)
                {
                labelFactor =
                    std::min(labelFactor, safe_divide(crossAvailable - layout.m_maxSide - itemGap,
                                                      maxNaturalWidth));
                }
            labelFactor = std::max(labelFactor, MIN_LABEL_FACTOR);
            }
        else
            {
            if (m_icons.size() > 1 && labelsOverlap(iconCenters(layout), 1.0, 1))
                {
                alternateLabels = true;
                layout = computeLayout(2.0, maxNaturalHeight);
                }
            const size_t sideStep = alternateLabels ? 2 : 1;
            const double rowCount = alternateLabels ? 2.0 : 1.0;
            if (labelsOverlap(iconCenters(layout), 1.0, sideStep))
                {
                // find the largest scaling (shared by all of the labels) that doesn't overlap
                const std::vector<double> searchCenters = iconCenters(layout);
                double lowFactor{ MIN_LABEL_FACTOR };
                double highFactor{ 1.0 };
                for (int i = 0; i < LABEL_FACTOR_SEARCH_STEPS; ++i)
                    {
                    const double midFactor = (lowFactor + highFactor) * math_constants::half;
                    if (labelsOverlap(searchCenters, midFactor, sideStep))
                        {
                        highFactor = midFactor;
                        }
                    else
                        {
                        lowFactor = midFactor;
                        }
                    }
                labelFactor = lowFactor;
                // the smaller labels leave more room for the icons
                layout = computeLayout(rowCount, maxNaturalHeight * labelFactor);
                while (labelFactor > MIN_LABEL_FACTOR &&
                       labelsOverlap(iconCenters(layout), labelFactor, sideStep))
                    {
                    labelFactor *= LABEL_FACTOR_BACKOFF;
                    }
                }
            }
        const double labelScaling = GetScaling() * labelFactor;
        const double rowHeight = maxNaturalHeight * labelFactor;
        const std::vector<double> centers = iconCenters(layout);

        // position each icon and its label
        std::vector<wxRect> iconRects;
        std::vector<wxPoint> labelCenters;
        iconRects.reserve(m_icons.size());
        labelCenters.reserve(m_icons.size());
        if (isVertical)
            {
            // the icons and their labels are centered as a group,
            // with the labels flush left beside the icons
            double widestLabel{ 0 };
            std::vector<double> measuredWidths;
            measuredWidths.reserve(m_icons.size());
            for (const auto& text : labelTexts)
                {
                const auto measured = static_cast<double>(
                    makeLabel(text, labelScaling)->GetBoundingBox(dc).GetWidth());
                measuredWidths.push_back(measured);
                widestLabel = std::max(widestLabel, measured);
                }
            const double groupLeft =
                static_cast<double>(drawArea.GetLeft()) +
                ((crossAvailable - (layout.m_maxSide + itemGap + widestLabel)) *
                 math_constants::half);
            const double iconCenterX = groupLeft + (layout.m_maxSide * math_constants::half);
            const double labelLeft = groupLeft + layout.m_maxSide + itemGap;
            for (size_t i = 0; i < m_icons.size(); ++i)
                {
                const double side = layout.m_sides[i];
                iconRects.emplace_back(wxRound(iconCenterX - (side * math_constants::half)),
                                       wxRound(centers[i] - (side * math_constants::half)),
                                       wxRound(side), wxRound(side));
                labelCenters.emplace_back(
                    wxRound(labelLeft + (measuredWidths[i] * math_constants::half)),
                    wxRound(centers[i]));
                }
            }
        else
            {
            // The labels are on two baselines, just below (under labels) and just above
            // (over labels) the largest icon.
            const double rowCount = alternateLabels ? 2.0 : 1.0;
            const double bandHeight = layout.m_maxSide + (rowCount * rowHeight);
            const double bandTop = static_cast<double>(drawArea.GetTop()) +
                                   ((crossAvailable - bandHeight) * math_constants::half);
            const double iconCenterY = bandTop + (alternateLabels ? rowHeight : 0.0) +
                                       (layout.m_maxSide * math_constants::half);
            for (size_t i = 0; i < m_icons.size(); ++i)
                {
                const double side = layout.m_sides[i];
                iconRects.emplace_back(wxRound(centers[i] - (side * math_constants::half)),
                                       wxRound(iconCenterY - (side * math_constants::half)),
                                       wxRound(side), wxRound(side));
                const bool isOver{ alternateLabels && (i % 2) == 1 };
                const double labelY = isOver ?
                                          bandTop + (rowHeight * math_constants::half) :
                                          bandTop + bandHeight - (rowHeight * math_constants::half);
                labelCenters.emplace_back(
                    wxRound(labelCenterX(centers[i], naturalWidths[i] * labelFactor)),
                    wxRound(labelY));
                }
            }

        // larger icons are drawn first so that smaller ones aren't covered when overlapping
        std::vector<size_t> drawOrder(m_icons.size());
        std::iota(drawOrder.begin(), drawOrder.end(), 0);
        std::ranges::stable_sort(drawOrder, std::greater<>{},
                                 [this](const size_t idx) { return m_icons[idx].m_value; });
        for (const auto idx : drawOrder)
            {
            const wxSize iconSizeDIPs{ static_cast<int>(std::round(DownscaleFromScreenAndCanvas(
                                           iconRects[idx].GetWidth()))),
                                       static_cast<int>(std::round(DownscaleFromScreenAndCanvas(
                                           iconRects[idx].GetHeight()))) };
            AddObject(
                std::make_unique<GraphItems::Shape>(GraphItems::GraphItemInfo{}
                                                        .Pen(m_iconPen)
                                                        .Brush(m_iconBrush)
                                                        .Scaling(GetScaling())
                                                        .DPIScaling(GetDPIScaleFactor())
                                                        .Selectable(false)
                                                        .Anchoring(Anchoring::TopLeftCorner)
                                                        .AnchorPoint(iconRects[idx].GetTopLeft()),
                                                    m_shape, iconSizeDIPs));
            }

        // labels are added after all of the icons so that overlapping icons don't cover them
        for (size_t i = 0; i < labelTexts.size(); ++i)
            {
            auto label = makeLabel(labelTexts[i], labelScaling);
            label->SetAnchorPoint(labelCenters[i]);
            AddObject(std::move(label));
            }
        }

    //----------------------------------------------------------------
    void Pictograph::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A pictograph") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");

        label += L". ";
        label += wxString::Format(
            /* TRANSLATORS: Pictograph accessibility description.
               %s is the shape name (e.g., property bag). */
            _(L"Each %s icon's area is proportional to its value"),
            GraphItems::ShapeInfo::GetReadableShapeName(m_shape));

        for (const auto& icon : m_icons)
            {
            const wxString valueText = FormatValue(icon.m_value, false);
            label += L". " + (icon.m_label.empty() ?
                                  valueText :
                                  wxString::Format(_DT(L"%s: %s"), icon.m_label, valueText));
            }

        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");
        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
