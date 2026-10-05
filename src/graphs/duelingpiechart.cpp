///////////////////////////////////////////////////////////////////////////////
// Name:        duelingpiechart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "duelingpiechart.h"
#include "../math/safe_math.h"
#include <array>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::DuelingPieChart, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    DuelingPieChart::DuelingPieChart(
        Canvas * canvas,
        const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/,
        const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetBrushScheme(brushes != nullptr ? brushes :
                                            std::make_shared<Brushes::Schemes::BrushScheme>(
                                                Settings::GetDefaultColorScheme()));
        SetColorScheme(colors);

        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);

        GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::White) };
        }

    //----------------------------------------------------------------
    void DuelingPieChart::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumnName,
        const wxString& categoryColumnName, const wxString& groupColumnName)
        {
        if (data == nullptr)
            {
            return;
            }

        const auto valueCol = data->GetContinuousColumn(valueColumnName);
        if (valueCol == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': value column not found for dueling pie chart.").ToUTF8(),
                                 valueColumnName));
            }
        const auto categoryCol = data->GetCategoricalColumn(categoryColumnName);
        if (categoryCol == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': category column not found for dueling pie chart.").ToUTF8(),
                categoryColumnName));
            }
        const auto groupCol = data->GetCategoricalColumn(groupColumnName);
        if (groupCol == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column not found for dueling pie chart.").ToUTF8(),
                                 groupColumnName));
            }

        const auto categoryMissingCode = categoryCol->FindMissingDataCode().value_or(
            Data::ColumnWithStringTable::MISSING_DATA_CODE);
        const auto groupMissingCode = groupCol->FindMissingDataCode().value_or(
            Data::ColumnWithStringTable::MISSING_DATA_CODE);

        const auto hasGroup = [&](const size_t row)
        { return groupCol->GetValue(row) != groupMissingCode; };
        const auto hasCategory = [&](const size_t row)
        { return categoryCol->GetValue(row) != categoryMissingCode; };
        const auto hasUsableValue = [&valueCol](const size_t row)
        {
            const auto val = valueCol->GetValue(row);
            return std::isfinite(val) && val > 0;
        };

        // Categories and groups, in the order that they first appear. Groups are taken from
        // every row that has one, so which fan a group lands in doesn't depend on whether that
        // row's category or value can be used. (A group with nothing usable gets an empty fan.)
        std::vector<Data::GroupIdType> categoryIds;
        std::vector<Data::GroupIdType> groupIds;
        // each ID's position in the vectors above, for adding up the values below
        std::unordered_map<Data::GroupIdType, size_t> categoryPositions;
        std::unordered_map<Data::GroupIdType, size_t> groupPositions;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!hasGroup(i))
                {
                continue;
                }
            if (groupPositions.try_emplace(groupCol->GetValue(i), groupIds.size()).second)
                {
                groupIds.push_back(groupCol->GetValue(i));
                }
            if (hasCategory(i) && hasUsableValue(i) &&
                categoryPositions.try_emplace(categoryCol->GetValue(i), categoryIds.size()).second)
                {
                categoryIds.push_back(categoryCol->GetValue(i));
                }
            }

        if (groupIds.size() != 2)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column must contain exactly two distinct values "
                                   "for a dueling pie chart.")
                                     .ToUTF8(),
                                 groupColumnName));
            }

        // totals for each category, per group
        std::array<std::vector<double>, 2> groupSums{ std::vector<double>(categoryIds.size(), 0.0),
                                                      std::vector<double>(categoryIds.size(),
                                                                          0.0) };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!hasGroup(i) || !hasCategory(i) || !hasUsableValue(i))
                {
                continue;
                }
            groupSums[groupPositions.at(groupCol->GetValue(i))]
                     [categoryPositions.at(categoryCol->GetValue(i))] += valueCol->GetValue(i);
            }

        // Categories without a value in a group stay in that group's pie with a zero value.
        // This keeps each category at the same position (and color) in both fans.
        const auto buildPie = [&](const size_t groupPos, PieChart::PieInfo& pie)
        {
            const auto& sums = groupSums[groupPos];
            const double total = std::accumulate(sums.cbegin(), sums.cend(), 0.0);
            pie.reserve(categoryIds.size());
            for (size_t i = 0; i < categoryIds.size(); ++i)
                {
                pie.emplace_back(categoryCol->GetLabelFromID(categoryIds[i]), sums[i],
                                 safe_divide(sums[i], total));
                }
        };
        PieChart::PieInfo topPie;
        PieChart::PieInfo bottomPie;
        buildPie(0, topPie);
        buildPie(1, bottomPie);
        wxString topGroupLabel{ groupCol->GetLabelFromID(groupIds[0]) };
        wxString bottomGroupLabel{ groupCol->GetLabelFromID(groupIds[1]) };

        // the chart is only changed now that the data is known to be good
        GetSelectedIds().clear();
        SetDataset(data);

        m_valueColumnName = valueColumnName;
        m_categoryColumnName = categoryColumnName;
        m_groupColumnName = groupColumnName;

        m_topPie = std::move(topPie);
        m_bottomPie = std::move(bottomPie);
        m_topGroupLabel = std::move(topGroupLabel);
        m_bottomGroupLabel = std::move(bottomGroupLabel);
        }

    //----------------------------------------------------------------
    void DuelingPieChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_topPie.empty())
            {
            return;
            }

        const auto plotArea = GetPlotAreaBoundingBox();
        const wxPoint center{ plotArea.GetLeft() + plotArea.GetWidth() / 2,
                              plotArea.GetTop() + plotArea.GetHeight() / 2 };

        const wxColour textColor =
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor());
        const double edgeGap = ScaleToScreenAndCanvas(4);

        const auto newLabel =
            [&](const wxString& text, const Anchoring anchor, const TextAlignment alignment)
        {
            auto label = std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo{ text }
                                                                 .Scaling(GetScaling())
                                                                 .DPIScaling(GetDPIScaleFactor())
                                                                 .Pen(wxNullPen)
                                                                 .FontColor(textColor)
                                                                 .LabelAlignment(alignment)
                                                                 .Anchoring(anchor)
                                                                 .AnchorPoint(center));
            label->SetShape(LabelShape::NoShape);
            label->SetBoxCorners(BoxCorners::Straight);
            label->SetShadowType(ShadowType::NoDisplay);
            return label;
        };

        // group names, which sit beyond the outer edge of each fan
        auto topGroupLabel = newLabel(m_topGroupLabel, Anchoring::Center, TextAlignment::Centered);
        auto bottomGroupLabel =
            newLabel(m_bottomGroupLabel, Anchoring::Center, TextAlignment::Centered);
        topGroupLabel->GetFont().MakeBold();
        bottomGroupLabel->GetFont().MakeBold();
        const double groupLabelHeight =
            std::max<double>(topGroupLabel->GetBoundingBox(dc).GetHeight(),
                             bottomGroupLabel->GetBoundingBox(dc).GetHeight());

        // fans take up most of the plot, but leave room above and below for the group names
        const double smallestPlotSide = std::min(plotArea.GetWidth(), plotArea.GetHeight());
        const double halfPlotHeight = plotArea.GetHeight() / 2.0;
        const double minRadius = smallestPlotSide * 0.2;
        double radius{ std::min(smallestPlotSide * 0.4,
                                halfPlotHeight - edgeGap - groupLabelHeight) };
        if (radius < minRadius)
            {
            // not enough room for the names at their full size, so shrink them instead
            radius = minRadius;
            const double groupLabelScale =
                std::clamp(safe_divide(halfPlotHeight - radius - edgeGap, groupLabelHeight),
                           math_constants::tenth, 1.0);
            topGroupLabel->SetScaling(topGroupLabel->GetScaling() * groupLabelScale);
            bottomGroupLabel->SetScaling(bottomGroupLabel->GetScaling() * groupLabelScale);
            }
        const wxRect pieArea{ wxPoint{ wxRound(center.x - radius), wxRound(center.y - radius) },
                              wxSize{ wxRound(radius * 2), wxRound(radius * 2) } };

        // The color scheme (if there is one) colors the categories, otherwise the brush scheme
        // does. Using the same brush for the slices and the legend keeps them matching.
        const auto categoryBrush = [this](const size_t index)
        {
            wxBrush brush{ GetBrushScheme()->GetBrush(index) };
            if (GetColorScheme())
                {
                brush.SetColour(GetColorScheme()->GetRecycledColor(index));
                }
            return brush;
        };

        constexpr double middleLabelProportion{ 0.6 };
        // the smallest size that any label needed to fit its slice
        double smallestLabelFontSize{ std::numeric_limits<double>::max() };
        std::vector<std::unique_ptr<GraphItems::Label>> middleLabels;

        // Slices run clockwise from the fan's first edge, so their angles decrease
        // (angles are counter-clockwise from 3 o'clock).
        const auto addFan = [&](const PieChart::PieInfo& pie, const double firstEdgeAngle)
        {
            double currentAngle{ firstEdgeAngle };
            for (size_t i = 0; i < pie.size(); ++i)
                {
                const double sweepAngle = pie[i].GetPercent() * m_fanSweepAngle;
                if (sweepAngle <= 0)
                    {
                    continue;
                    }
                auto pieSlice = std::make_unique<GraphItems::PieSlice>(
                    GraphItems::GraphItemInfo{ pie[i].GetGroupLabel() }
                        .Brush(categoryBrush(i))
                        .DPIScaling(GetDPIScaleFactor())
                        .Scaling(GetScaling())
                        .Pen(GetPen()),
                    pieArea, currentAngle - sweepAngle, currentAngle, pie[i].GetValue(),
                    pie[i].GetPercent());
                // an explicit arc pen is scaled once, matching the slice's straight edges
                pieSlice->GetArcPen() = GetPen();

                auto middleLabel =
                    pieSlice->CreateMiddleLabel(dc, middleLabelProportion, m_midPointLabelDisplay);
                if (middleLabel != nullptr)
                    {
                    middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                    smallestLabelFontSize = std::min(
                        smallestLabelFontSize, middleLabel->GetFont().GetFractionalPointSize());
                    middleLabels.push_back(std::move(middleLabel));
                    }

                AddObject(std::move(pieSlice));
                currentAngle -= sweepAngle;
                }
        };
        // the bottom fan is the top fan rotated 180 degrees
        addFan(m_topPie, 90.0 + (m_fanSweepAngle / 2));
        addFan(m_bottomPie, 270.0 + (m_fanSweepAngle / 2));

        // a common font size for all of the labels inside the slices
        for (auto& middleLabel : middleLabels)
            {
            middleLabel->GetFont().SetFractionalPointSize(smallestLabelFontSize);
            AddObject(std::move(middleLabel));
            }

        // place the group names just beyond the outer edge of each fan
        const auto addGroupLabel =
            [&](std::unique_ptr<GraphItems::Label> label, const bool isTopFan)
        {
            const double offset = radius + edgeGap + (label->GetBoundingBox(dc).GetHeight() / 2.0);
            label->SetAnchorPoint(
                wxPoint{ center.x, wxRound(center.y + (isTopFan ? -offset : offset)) });
            AddObject(std::move(label));
        };
        addGroupLabel(std::move(topGroupLabel), true);
        addGroupLabel(std::move(bottomGroupLabel), false);

        // The legend is split in two, filling the empty wedges to the left and right of
        // the center point. Both lists are centered vertically on the center point.
        const size_t categoryCount = m_topPie.size();
        const size_t leftCount = (categoryCount + 1) / 2;
        std::vector<GraphItems::Label*> leftLabels;
        std::vector<GraphItems::Label*> rightLabels;
        for (size_t i = 0; i < categoryCount; ++i)
            {
            const bool isLeft = (i < leftCount);
            auto label = newLabel(m_topPie[i].GetGroupLabel(),
                                  isLeft ? Anchoring::TopLeftCorner : Anchoring::TopRightCorner,
                                  isLeft ? TextAlignment::FlushLeft : TextAlignment::FlushRight);
            (isLeft ? leftLabels : rightLabels).push_back(label.get());
            AddObject(std::move(label));
            }

        // sizes at full scale
        constexpr double rowSpacing{ 1.4 };
        std::vector<GraphItems::Label*> legendLabels{ leftLabels };
        legendLabels.insert(legendLabels.end(), rightLabels.cbegin(), rightLabels.cend());
        double widestText{ 0 };
        double tallestText{ 0 };
        for (const auto* label : legendLabels)
            {
            const auto box = label->GetBoundingBox(dc);
            widestText = std::max<double>(widestText, box.GetWidth());
            tallestText = std::max<double>(tallestText, box.GetHeight());
            }
        const double listHeight = static_cast<double>(leftLabels.size()) * tallestText * rowSpacing;
        // each entry has a dot (as wide as the text is tall) next to its text
        const double blockWidth = tallestText + widestText;

        // A list's inner edge needs to be at least half of its height from the center point to
        // stay clear of the fans (the empty wedges have 45 degree edges).
        // Shrink the legend text if the block would otherwise run past the plot area's sides,
        // or if the list would be taller than the fans (where the group names begin).
        const double widthScale =
            safe_divide((plotArea.GetWidth() / 2.0) - (edgeGap * 2), (listHeight / 2) + blockWidth);
        const double heightScale = safe_divide(radius * 2, listHeight);
        const double legendScale =
            std::clamp(std::min(widthScale, heightScale), math_constants::tenth, 1.0);
        for (auto* label : legendLabels)
            {
            label->SetScaling(label->GetScaling() * legendScale);
            }

        const double rowHeight = tallestText * rowSpacing * legendScale;
        const double dotSpace = tallestText * legendScale;
        const double innerEdgeDistance = (listHeight * legendScale / 2) + edgeGap;
        const double scaledBlockWidth = blockWidth * legendScale;
        // the circle's radius is unscaled, since the point collection applies the scaling
        const auto dotRadius = static_cast<size_t>(std::max(
            1, wxRound(safe_divide(dotSpace * math_constants::third, ScaleToScreenAndCanvas(1)))));

        auto dots = std::make_unique<GraphItems::Points2D>(wxNullPen);
        dots->SetDPIScaleFactor(GetDPIScaleFactor());
        dots->SetScaling(GetScaling());
        dots->SetSelectable(false);

        const auto placeList = [&](const std::vector<GraphItems::Label*>& labels,
                                   const size_t firstCategory, const bool isLeft)
        {
            const double listLeft = isLeft ? (center.x - innerEdgeDistance - scaledBlockWidth) :
                                             (center.x + innerEdgeDistance);
            const double listRight = listLeft + scaledBlockWidth;
            double rowCenterY =
                center.y - ((static_cast<double>(labels.size()) * rowHeight) / 2) + (rowHeight / 2);
            for (size_t i = 0; i < labels.size(); ++i)
                {
                auto* label = labels[i];
                const double textX = isLeft ? (listLeft + dotSpace) : (listRight - dotSpace);
                label->SetAnchorPoint(
                    wxPoint{ wxRound(textX),
                             wxRound(rowCenterY - (label->GetBoundingBox(dc).GetHeight() / 2.0)) });

                const wxBrush dotBrush = categoryBrush(firstCategory + i);
                const double dotX =
                    isLeft ? (listLeft + (dotSpace / 2)) : (listRight - (dotSpace / 2));
                dots->AddPoint(GraphItems::Point2D(
                                   GraphItems::GraphItemInfo{}
                                       .Brush(dotBrush)
                                       .Pen(wxPen{ dotBrush.GetColour() })
                                       .AnchorPoint(wxPoint{ wxRound(dotX), wxRound(rowCenterY) }),
                                   dotRadius),
                               dc);
                rowCenterY += rowHeight;
                }
        };
        placeList(leftLabels, 0, true);
        placeList(rightLabels, leftCount, false);
        AddObject(std::move(dots));
        }

    //----------------------------------------------------------------
    void DuelingPieChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A dueling pie chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");

        const auto describeFan = [&label](const wxString& groupLabel, const PieChart::PieInfo& pie)
        {
            wxString entries;
            for (const auto& slice : pie)
                {
                if (slice.GetValue() <= 0)
                    {
                    continue;
                    }
                if (!entries.empty())
                    {
                    entries += L", ";
                    }
                entries += wxString::Format(
                    _DT(L"%s: %s"), slice.GetGroupLabel(),
                    wxNumberFormatter::ToString(slice.GetValue(), 6,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            label += L". " + groupLabel + L" - " + entries;
        };
        describeFan(m_topGroupLabel, m_topPie);
        describeFan(m_bottomGroupLabel, m_bottomPie);

        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
