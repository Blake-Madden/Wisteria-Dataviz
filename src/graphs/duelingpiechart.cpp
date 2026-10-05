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
#include <numeric>
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
        GetSelectedIds().clear();
        m_topPie.clear();
        m_bottomPie.clear();
        m_topGroupLabel.clear();
        m_bottomGroupLabel.clear();

        SetDataset(data);

        m_valueColumnName = valueColumnName;
        m_categoryColumnName = categoryColumnName;
        m_groupColumnName = groupColumnName;

        const auto valueCol = GetContinuousColumn(m_valueColumnName);
        const auto categoryCol = GetCategoricalColumn(m_categoryColumnName);
        const auto groupCol = GetCategoricalColumn(m_groupColumnName);

        const auto isRowUsable = [&valueCol](const size_t row)
        {
            const auto val = valueCol->GetValue(row);
            return std::isfinite(val) && val > 0;
        };

        // categories and groups, in the order that they first appear
        std::vector<Data::GroupIdType> categoryIds;
        std::vector<Data::GroupIdType> groupIds;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!isRowUsable(i))
                {
                continue;
                }
            if (std::ranges::find(categoryIds, categoryCol->GetValue(i)) == categoryIds.cend())
                {
                categoryIds.push_back(categoryCol->GetValue(i));
                }
            if (std::ranges::find(groupIds, groupCol->GetValue(i)) == groupIds.cend())
                {
                groupIds.push_back(groupCol->GetValue(i));
                }
            }

        if (groupIds.size() != 2)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column must contain exactly two distinct values "
                                   "for a dueling pie chart.")
                                     .ToUTF8(),
                                 m_groupColumnName));
            }

        // totals for each category, per group
        std::array<std::vector<double>, 2> groupSums{ std::vector<double>(categoryIds.size(), 0.0),
                                                      std::vector<double>(categoryIds.size(),
                                                                          0.0) };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!isRowUsable(i))
                {
                continue;
                }
            const auto groupPos = static_cast<size_t>(std::distance(
                groupIds.begin(), std::ranges::find(groupIds, groupCol->GetValue(i))));
            const auto categoryPos = static_cast<size_t>(std::distance(
                categoryIds.begin(), std::ranges::find(categoryIds, categoryCol->GetValue(i))));
            groupSums[groupPos][categoryPos] += valueCol->GetValue(i);
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
        buildPie(0, m_topPie);
        buildPie(1, m_bottomPie);

        m_topGroupLabel = groupCol->GetLabelFromID(groupIds[0]);
        m_bottomGroupLabel = groupCol->GetLabelFromID(groupIds[1]);
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
        const double radius = std::min(plotArea.GetWidth(), plotArea.GetHeight()) * 0.4;
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
        double smallestLabelFontSize{ GetBottomXAxis().GetFont().GetFractionalPointSize() };
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

        // group names, beyond the outer edge of each fan
        const auto addGroupLabel = [&](const wxString& text, const bool isTopFan)
        {
            auto label = newLabel(text, Anchoring::Center, TextAlignment::Centered);
            label->GetFont().MakeBold();
            const double offset = radius + edgeGap + (label->GetBoundingBox(dc).GetHeight() / 2.0);
            label->SetAnchorPoint(
                wxPoint{ center.x, wxRound(center.y + (isTopFan ? -offset : offset)) });
            AddObject(std::move(label));
        };
        addGroupLabel(m_topGroupLabel, true);
        addGroupLabel(m_bottomGroupLabel, false);

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
        // Shrink the legend text if the block would otherwise run past the plot area.
        const double legendScale = std::clamp(
            safe_divide((plotArea.GetWidth() / 2.0) - (edgeGap * 2), (listHeight / 2) + blockWidth),
            math_constants::tenth, 1.0);
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
                    wxNumberFormatter::ToString(slice.GetValue(), 0,
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
