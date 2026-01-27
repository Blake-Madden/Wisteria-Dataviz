///////////////////////////////////////////////////////////////////////////////
// Name:        piechart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "piechart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::PieChart, Wisteria::Graphs::Graph2D)

    namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------------
    std::unique_ptr<Label> PieSlice::CreateMiddleLabel(
        wxDC & dc, const double pieProportion, const BinLabelDisplay labelDisplay,
        const std::shared_ptr<const TextReplace>& abbreviate /*= nullptr*/)
        {
        const auto arcMiddle = GetMiddleOfArc(pieProportion);
        auto pieLabel = std::make_unique<Label>(GetGraphItemInfo());
        // if less than 1%, then use higher precision so that it doesn't just show as "0%"
        const auto percStr =
            wxNumberFormatter::ToString(m_percent * 100, ((m_percent * 100) < 1) ? 2 : 0,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes);
        switch (labelDisplay)
            {
        case BinLabelDisplay::BinValue:
            pieLabel->SetText(
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat()));
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            pieLabel->SetText(wxString::Format(
                /* TRANSLATORS: Percentage value (%s), % symbol (%%), and value.
                   '%%' can be changed and/or moved elsewhere in the string.*/
                _(L"%s%%\n(%s)"), percStr,
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat())));
            break;
        case BinLabelDisplay::BinPercentage:
            pieLabel->SetText(wxString::Format(
                /* TRANSLATORS: Percentage value (%s) and % symbol (%%).
                   '%%' can be changed and/or moved elsewhere in the string.*/
                _(L"%s%%"), percStr));
            break;
        case BinLabelDisplay::NoDisplay:
            pieLabel->SetText(wxEmptyString);
            break;
        case BinLabelDisplay::BinNameAndValue:
            pieLabel->SetText(wxString::Format(
                L"%s\n(%s)", pieLabel->GetText(),
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat())));
            break;
        case BinLabelDisplay::BinNameAndPercentage:
            pieLabel->SetText(wxString::Format(
                // TRANSLATORS: Pie chart label and then a percentage value in parentheses
                _(L"%s\n(%s%%)"), pieLabel->GetText(), percStr));
            break;
        case BinLabelDisplay::BinName:
            [[fallthrough]];
        default:
            // leave as the name of the slice
            break;
            }

        pieLabel->GetGraphItemInfo()
            .Pen(wxNullPen)
            .Scaling(GetScaling())
            .LabelAlignment(TextAlignment::Centered)
            .Selectable(true)
            .Anchoring(Anchoring::Center)
            .AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second))
            .FontColor((GetBrush().IsOk() && GetBrush().GetColour().IsOk()) ?
                           Colors::ColorContrast::BlackOrWhiteContrast(GetBrush().GetColour()) :
                           Colors::ColorBrewer::GetColor(Colors::Color::Black));
        pieLabel->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
        pieLabel->GetHeaderInfo().Enable(false);

        const auto originalFontSize{ pieLabel->GetFont().GetFractionalPointSize() };
        const auto originalText{ pieLabel->GetText() };

        // make it fit in the slice and return it (or null if too small)
        const auto fitLabelToSlice = [this, &dc](auto& pieSliceLabel)
        {
            const auto points = GetPolygon();
            bool middleLabelIsTooSmall{ false };
            for (;;)
                {
                if (auto labelBox = pieSliceLabel->GetBoundingBox(dc);
                    geometry::is_inside_polygon(labelBox.GetTopLeft(), points) &&
                    geometry::is_inside_polygon(labelBox.GetBottomLeft(), points) &&
                    geometry::is_inside_polygon(labelBox.GetTopRight(), points) &&
                    geometry::is_inside_polygon(labelBox.GetBottomRight(), points))
                    {
                    break;
                    }
                const auto currentFontSize = pieSliceLabel->GetFont().GetFractionalPointSize();
                pieSliceLabel->GetFont().Scale(.95F);
                // either too small for our taste or couldn't be scaled down anymore
                if ((pieSliceLabel->GetFont().GetFractionalPointSize() * GetScaling()) <= 6 ||
                    compare_doubles(pieSliceLabel->GetFont().GetFractionalPointSize(),
                                    currentFontSize))
                    {
                    middleLabelIsTooSmall = true;
                    break;
                    }
                }
            return middleLabelIsTooSmall ? nullptr : std::move(pieSliceLabel);
        };

        // If scaledPieLabel is null, then pieLabel wasn't moved.
        // Suppress false positive warnings.
        // NOLINTBEGIN(clang-analyzer-cplusplus.Move)
        auto scaledPieLabel = fitLabelToSlice(pieLabel);
        // if it doesn't fit, try to split it into smaller lines
        // and possibly abbreviate it, then try again
        if (scaledPieLabel == nullptr)
            {
            pieLabel->GetFont().SetFractionalPointSize(originalFontSize);
            pieLabel->SetText(originalText);
            if (abbreviate && labelDisplay == BinLabelDisplay::BinName)
                {
                pieLabel->SetText((*abbreviate)(pieLabel->GetText()));
                }
            pieLabel->SplitTextAuto();

            scaledPieLabel = fitLabelToSlice(pieLabel);
            // if auto splitting still wasn't enough to fit, then try
            // splitting into multiple lines (if a comma-separated list)
            if (scaledPieLabel == nullptr)
                {
                pieLabel->GetFont().SetFractionalPointSize(originalFontSize);
                pieLabel->SetText(originalText);
                if (abbreviate && labelDisplay == BinLabelDisplay::BinName)
                    {
                    pieLabel->SetText((*abbreviate)(pieLabel->GetText()));
                    }
                pieLabel->SplitTextByListItems();

                return fitLabelToSlice(pieLabel);
                }
            return scaledPieLabel;
            }
        // NOLINTEND(clang-analyzer-cplusplus.Move)
        return scaledPieLabel;
        }

    //----------------------------------------------------------------
    std::unique_ptr<Label> PieSlice::CreateOuterLabel(const BinLabelDisplay labelDisplay)
        {
        return CreateOuterLabel(m_pieArea, labelDisplay);
        }

    //----------------------------------------------------------------
    std::unique_ptr<Label> PieSlice::CreateOuterLabel(const wxRect& pieArea,
                                                      const BinLabelDisplay labelDisplay)
        {
        const auto angle = m_startAngle + ((m_endAngle - m_startAngle) / 2);
        const auto arcMiddle = GetMiddleOfArc(1.0, pieArea);
        auto pieLabel = std::make_unique<Label>(GetGraphItemInfo());
        pieLabel->GetGraphItemInfo()
            .Pen(wxNullPen)
            .Scaling(GetScaling())
            .Padding(0, 4, 0, 4)
            .Selectable(true)
            .Anchoring(
                is_within<double>(std::make_pair(0, 90), angle)    ? Anchoring::BottomLeftCorner :
                is_within<double>(std::make_pair(90, 180), angle)  ? Anchoring::BottomRightCorner :
                is_within<double>(std::make_pair(180, 270), angle) ? Anchoring::TopRightCorner :
                                                                     Anchoring::TopLeftCorner)
            .AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second))
            .LabelAlignment((is_within<double>(std::make_pair(0, 90), angle) ||
                             is_within<double>(std::make_pair(270, 360), angle)) ?
                                TextAlignment::FlushLeft :
                                TextAlignment::FlushRight);
        // if less than 1%, then use higher precision so that it doesn't just show as "0%"
        const auto percStr =
            wxNumberFormatter::ToString(m_percent * 100, ((m_percent * 100) < 1) ? 2 : 0,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes);
        switch (labelDisplay)
            {
        case BinLabelDisplay::BinValue:
            pieLabel->SetText(
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat()));
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            pieLabel->SetText(wxString::Format(
                /* TRANSLATORS: Percentage value (%s), % symbol (%%), and value.
                   '%%' can be changed and/or moved elsewhere in the string.*/
                _(L"%s%% (%s)"), percStr,
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat())));
            break;
        case BinLabelDisplay::BinPercentage:
            pieLabel->SetText(wxString::Format(
                /* TRANSLATORS: Percentage value (%s) and % symbol (%%).
                   '%%' can be changed and/or moved elsewhere in the string.*/
                _(L"%s%%"), percStr));
            break;
        case BinLabelDisplay::NoDisplay:
            pieLabel->SetText(wxEmptyString);
            break;
        case BinLabelDisplay::BinNameAndValue:
            pieLabel->SetText(wxString::Format(
                L"%s (%s)", pieLabel->GetText(),
                wxNumberFormatter::ToString(m_value, 0, Settings::GetDefaultNumberFormat())));
            break;
        case BinLabelDisplay::BinNameAndPercentage:
            pieLabel->SetText(wxString::Format(
                // TRANSLATORS: Pie chart label and then a percentage value in parentheses
                _(L"%s (%s%%)"), pieLabel->GetText(), percStr));
            break;
        case BinLabelDisplay::BinName:
            [[fallthrough]];
        default:
            // leave as the name of the slice
            break;
            }
        // outer labels can have headers
        pieLabel->GetHeaderInfo()
            .LabelAlignment((is_within<double>(std::make_pair(0, 90), angle) ||
                             is_within<double>(std::make_pair(270, 360), angle)) ?
                                TextAlignment::FlushLeft :
                                TextAlignment::FlushRight)
            .FontColor(GetHeaderInfo().GetFontColor())
            .GetFont()
            .MakeBold();

        return pieLabel;
        }

    //----------------------------------------------------------------
    std::pair<double, double> PieSlice::GetMiddleOfArc(const double pieProportion,
                                                       const wxRect pieArea) const noexcept
        {
        const auto shrinkProportion{ 1 - pieProportion };
        wxRect outerRect{ pieArea };
        outerRect.SetWidth(outerRect.GetWidth() - (outerRect.GetWidth() * shrinkProportion));
        outerRect.SetHeight(outerRect.GetHeight() - (outerRect.GetHeight() * shrinkProportion));
        outerRect.Offset(wxPoint(safe_divide(pieArea.GetWidth() - outerRect.GetWidth(), 2),
                                 safe_divide(pieArea.GetHeight() - outerRect.GetHeight(), 2)));
        auto midPt =
            geometry::arc_vertex(std::make_pair(outerRect.GetWidth(), outerRect.GetHeight()),
                                 m_startAngle + ((m_endAngle - m_startAngle) / 2));
        // in case the rect doesn't start at (0, 0), offset the point
        midPt.first += outerRect.GetTopLeft().x;
        midPt.second += outerRect.GetTopLeft().y;
        return midPt;
        }

    //----------------------------------------------------------------
    std::pair<double, double> PieSlice::GetMiddleOfArc(const double pieProportion) const noexcept
        {
        return GetMiddleOfArc(pieProportion, m_pieArea);
        }

    //----------------------------------------------------------------
    std::vector<wxPoint> PieSlice::GetPolygon() const noexcept
        {
        std::vector<wxPoint> points;

        auto startSweep = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_startAngle);
        // in case the rect doesn't start at (0, 0), offset the point
        startSweep.first += m_pieArea.GetTopLeft().x;
        startSweep.second += m_pieArea.GetTopLeft().y;
        points.push_back(Polygon::PairToPoint(startSweep));

        auto middleSweep1 = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle - m_startAngle) * math_constants::quarter) + m_startAngle);
        middleSweep1.first += m_pieArea.GetTopLeft().x;
        middleSweep1.second += m_pieArea.GetTopLeft().y;
        points.push_back(Polygon::PairToPoint(middleSweep1));

        auto middleSweep2 = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle - m_startAngle) * math_constants::half) + m_startAngle);
        middleSweep2.first += m_pieArea.GetTopLeft().x;
        middleSweep2.second += m_pieArea.GetTopLeft().y;
        points.push_back(Polygon::PairToPoint(middleSweep2));

        auto middleSweep3 = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle - m_startAngle) * math_constants::three_quarters) + m_startAngle);
        middleSweep3.first += m_pieArea.GetTopLeft().x;
        middleSweep3.second += m_pieArea.GetTopLeft().y;
        points.push_back(Polygon::PairToPoint(middleSweep3));

        auto endSweep = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_endAngle);
        endSweep.first += m_pieArea.GetTopLeft().x;
        endSweep.second += m_pieArea.GetTopLeft().y;
        points.push_back(Polygon::PairToPoint(endSweep));

        // center of pie
        points.emplace_back(m_pieArea.GetLeft() + (m_pieArea.GetWidth() / 2),
                            m_pieArea.GetTop() + (m_pieArea.GetHeight() / 2));

        return points;
        }

    //----------------------------------------------------------------
    wxRect PieSlice::Draw(wxDC & dc) const
        {
        wxPen scaledPen(GetPen().IsOk()   ? GetPen() :
                        GetBrush().IsOk() ? GetBrush().GetColour() :
                                            wxColour{ 0, 0, 0, 0 });
        scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));

        const wxPoint centerPoint{ m_pieArea.GetWidth() / 2 + m_pieArea.GetLeft(),
                                   m_pieArea.GetHeight() / 2 + m_pieArea.GetTop() };

        // Outer arc
        // Note that if the start and end angles are the same, then an entire pie
        // is drawn with the current brush (per wxWidgets's docs), which is not what we want;
        // in that case, don't draw the outer arc (which would be zero length anyway).
        if (!compare_doubles(m_startAngle, m_endAngle))
            {
            wxPen scaledArcPen((GetArcPen().has_value() && GetArcPen().value().IsOk()) ?
                                   GetArcPen().value() :
                                   scaledPen);
            scaledArcPen.SetWidth(ScaleToScreenAndCanvas(scaledArcPen.GetWidth()));

            const wxDCPenChanger pc{ dc, scaledArcPen };

            // if a base color is in use, draw under a (possibly) hatched brush
            if (GetGraphItemInfo().GetBaseColor().has_value())
                {
                const wxDCBrushChanger bCh{ dc, GetGraphItemInfo().GetBaseColor().value() };
                dc.DrawEllipticArc(m_pieArea.GetTopLeft(), m_pieArea.GetSize(), m_startAngle,
                                   m_endAngle);
                }

            const wxDCBrushChanger bCh{ dc, GetBrush().IsOk() ? GetBrush() : dc.GetBrush() };
            dc.DrawEllipticArc(m_pieArea.GetTopLeft(), m_pieArea.GetSize(), m_startAngle,
                               m_endAngle);
            }
        // line from the pie center to the start of the arc
        auto arcStart = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_startAngle);
        arcStart.first += m_pieArea.GetTopLeft().x;
        arcStart.second += m_pieArea.GetTopLeft().y;
            {
            const wxDCPenChanger pc{ dc, scaledPen };
            dc.DrawLine(centerPoint, wxPoint(arcStart.first, arcStart.second));
            }
        // line from the pie center to the end of the arc
        auto arcEnd = geometry::arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_endAngle);
        arcEnd.first += m_pieArea.GetTopLeft().x;
        arcEnd.second += m_pieArea.GetTopLeft().y;
            {
            const wxDCPenChanger pc{ dc, scaledPen };
            dc.DrawLine(centerPoint, wxPoint(arcEnd.first, arcEnd.second));
            }

        if (IsSelected())
            {
            auto points = GetPolygon();
            const wxDCPenChanger pc{ dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                               ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT) };
            dc.DrawLines(points.size(), points.data());
            // highlight the selected protruding bounding box in debug mode
            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                std::array<wxPoint, 5> debugOutline;
                GraphItems::Polygon::GetRectPoints(m_pieArea, debugOutline);
                const wxDCPenChanger pcDebug{
                    dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Green),
                              ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH)
                };
                dc.DrawLines(debugOutline.size(), debugOutline.data());
                }
            }

        return GetBoundingBox(dc);
        }
    } // namespace Wisteria::GraphItems

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    PieChart::PieChart(Canvas* canvas,
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

        GetDonutHoleLabel().GetGraphItemInfo().LabelAlignment(TextAlignment::Justified);
        }

    //----------------------------------------------------------------
    void PieChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                           std::optional<const wxString> weightColumnName,
                           const wxString& groupColumn1Name,
                           std::optional<const wxString> groupColumn2Name /*= std::nullopt*/)
        {
        if (data == nullptr)
            {
            return;
            }

        GetSelectedIds().clear();
        auto groupColumnContinuous1{ data->GetContinuousColumns().cend() };
        bool useContinuousGroup1Column{ false };
        const auto& groupColumn1 = data->GetCategoricalColumn(groupColumn1Name);
        if (groupColumn1 == data->GetCategoricalColumns().cend())
            {
            groupColumnContinuous1 = data->GetContinuousColumn(groupColumn1Name);
            if (groupColumnContinuous1 == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': group column not found for pie chart.").ToUTF8(), groupColumn1Name));
                }
            useContinuousGroup1Column = true;
            }

        auto groupColumnContinuous2{ data->GetContinuousColumns().cend() };
        bool useContinuousGroup2Column{ false };
        const auto& groupColumn2 =
            (groupColumn2Name ? data->GetCategoricalColumn(groupColumn2Name.value()) :
                                data->GetCategoricalColumns().cend());
        if (groupColumn2Name.has_value() && groupColumn2 == data->GetCategoricalColumns().cend())
            {
            groupColumnContinuous2 = data->GetContinuousColumn(groupColumn2Name.value());
            if (groupColumnContinuous2 == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': inner group column not found for pie chart.").ToUTF8(),
                    groupColumn2Name.value()));
                }
            useContinuousGroup2Column = true;
            }
        const bool useSubgrouping = groupColumn2Name.has_value();

        const auto& weightColumn =
            (weightColumnName.has_value() ? data->GetContinuousColumn(weightColumnName.value()) :
                                            data->GetContinuousColumns().cend());
        if (weightColumnName.has_value() && weightColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': aggregate column not found for pie chart.").ToUTF8(),
                                 weightColumnName.value()));
            }
        const bool useAggregateColumn = (weightColumn != data->GetContinuousColumns().cend());

        GetInnerPie().clear();
        GetOuterPie().clear();

        // Note that the frequencies from the aggregate column could be a
        // double value (e.g., 3.5), so that's why the counter value
        // for this type is a double.
        using SliceAndCounts = std::map<Data::GroupIdType, double>;
        // the outer pie (or only pie, if a single series)
        SliceAndCounts outerGroups;

        double totalValue{ 0.0 };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (useAggregateColumn && std::isnan(weightColumn->GetValue(i)))
                {
                continue;
                }

            auto [iterator, inserted] = outerGroups.insert(std::make_pair(
                (useContinuousGroup1Column ?
                     static_cast<Data::GroupIdType>(groupColumnContinuous1->GetValue(i)) :
                     groupColumn1->GetValue(i)),
                (useAggregateColumn ? weightColumn->GetValue(i) : 1)));
            // increment counts for group
            if (!inserted)
                {
                iterator->second += (useAggregateColumn ? weightColumn->GetValue(i) : 1);
                }
            totalValue += (useAggregateColumn ? weightColumn->GetValue(i) : 1);
            }

        // create slices with their percentages of the overall total
        for (const auto& group : outerGroups)
            {
            GetOuterPie().emplace_back((useContinuousGroup1Column ?
                                            std::to_wstring(group.first) :
                                            groupColumn1->GetLabelFromID(group.first)),
                                       group.second, safe_divide(group.second, totalValue));
            }
        std::sort(GetOuterPie().begin(), GetOuterPie().end());

        // if more grouping columns, then add an inner pie (which is a subgrouping
        // of the main group)
        if (useSubgrouping)
            {
            std::map<Data::GroupIdType, SliceAndCounts> innerGroups;
            totalValue = 0;
            auto searchValue = std::pair<Data::GroupIdType, SliceAndCounts>(0, SliceAndCounts{});
            for (size_t i = 0; i < data->GetRowCount(); ++i)
                {
                if (useAggregateColumn && std::isnan(weightColumn->GetValue(i)))
                    {
                    continue;
                    }

                searchValue.first =
                    (useContinuousGroup1Column ?
                         static_cast<Data::GroupIdType>(groupColumnContinuous1->GetValue(i)) :
                         groupColumn1->GetValue(i));
                auto [iterator, inserted] = innerGroups.insert(searchValue);
                if (inserted)
                    {
                    iterator->second.insert(std::make_pair(
                        (useContinuousGroup2Column ?
                             static_cast<Data::GroupIdType>(groupColumnContinuous2->GetValue(i)) :
                             groupColumn2->GetValue(i)),
                        (useAggregateColumn ? weightColumn->GetValue(i) : 1)));
                    }
                else
                    {
                    auto [subIterator, subInserted] = iterator->second.insert(std::make_pair(
                        (useContinuousGroup2Column ?
                             static_cast<Data::GroupIdType>(groupColumnContinuous2->GetValue(i)) :
                             groupColumn2->GetValue(i)),
                        (useAggregateColumn ? weightColumn->GetValue(i) : 1)));
                    // increment counts for group
                    if (!subInserted)
                        {
                        subIterator->second += (useAggregateColumn ? weightColumn->GetValue(i) : 1);
                        }
                    }
                totalValue += (useAggregateColumn ? weightColumn->GetValue(i) : 1);
                }

            std::map<wxString, PieInfo, Data::wxStringLessNoCase> innerPie;
            // the outer ring (main group) for the inner group slices
            for (const auto& innerGroupOuterRing : innerGroups)
                {
                PieInfo currentOuterSliceSlices;
                // the slices with the current outer ring group
                for (const auto& innerGroup : innerGroupOuterRing.second)
                    {
                    currentOuterSliceSlices.emplace_back(
                        (useContinuousGroup2Column ?
                             std::to_wstring(innerGroup.first) :
                             groupColumn2->GetLabelFromID(innerGroup.first)),
                        innerGroup.second, safe_divide(innerGroup.second, totalValue));
                    }
                std::sort(currentOuterSliceSlices.begin(), currentOuterSliceSlices.end());
                innerPie.insert(
                    std::make_pair((useContinuousGroup1Column ?
                                        std::to_wstring(innerGroupOuterRing.first) :
                                        groupColumn1->GetLabelFromID(innerGroupOuterRing.first)),
                                   currentOuterSliceSlices));
                }
            // unroll the grouped slices into one large pie
            Data::GroupIdType parentGroupIndex{ 0 };
            for (auto& innerPieSliceGroup : innerPie)
                {
                std::ranges::for_each(innerPieSliceGroup.second,
                                      [&parentGroupIndex](auto& slice) noexcept
                                      { slice.m_parentSliceIndex = parentGroupIndex; });
                GetInnerPie().insert(GetInnerPie().end(), innerPieSliceGroup.second.cbegin(),
                                     innerPieSliceGroup.second.cend());
                ++parentGroupIndex;
                }

            // Don't show their labels on the outside by default though,
            // may overlap the outer pie's labels and be too cluttered;
            // client can turn these on after setting the data.
            ShowInnerPieLabels(false);
            }
        }

    //----------------------------------------------------------------
    void PieChart::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        std::vector<std::unique_ptr<GraphItemBase>> addedObjects;
        const auto queueObjectForOffsetting = [&addedObjects](auto obj)
        { addedObjects.push_back(std::move(obj)); };

        DrawAreas drawAreas;

        // get a square inside the drawing area for the pie
        drawAreas.m_pieDrawArea = GetPlotAreaBoundingBox();
        // get 75% of the area width and height for the pie (adding space for any labels),
        // and use the smaller of the two for the pie's area
        const auto pieHeight =
            (drawAreas.m_pieDrawArea.GetHeight() * math_constants::three_quarters);
        const auto pieWidth = (drawAreas.m_pieDrawArea.GetWidth() * math_constants::three_quarters);
        const auto pieDimension = std::min(pieHeight, pieWidth);
        const auto widthDifference = (drawAreas.m_pieDrawArea.GetWidth() - pieDimension);
        const auto heightDifference = (drawAreas.m_pieDrawArea.GetHeight() - pieDimension);
        drawAreas.m_pieDrawArea.SetWidth(pieDimension);
        drawAreas.m_pieDrawArea.SetX(drawAreas.m_pieDrawArea.GetX() + (widthDifference / 2));
        drawAreas.m_pieDrawArea.SetHeight(pieDimension);
        drawAreas.m_pieDrawArea.SetY(drawAreas.m_pieDrawArea.GetY() + (heightDifference / 2));

        // make label drawing area square or "golden ratioed,"
        // so that labels don't go up too high or too far over
        drawAreas.m_fullDrawArea = GetPlotAreaBoundingBox();
            {
            const auto widthDiff =
                GetPlotAreaBoundingBox().GetWidth() - drawAreas.m_pieDrawArea.GetWidth();
            const auto heightDiff =
                GetPlotAreaBoundingBox().GetHeight() - drawAreas.m_pieDrawArea.GetHeight();
            if (heightDiff > widthDiff)
                {
                const auto sizeDiff = heightDiff - widthDiff;
                drawAreas.m_fullDrawArea.SetHeight(drawAreas.m_fullDrawArea.GetHeight() - sizeDiff);
                drawAreas.m_fullDrawArea.SetY(drawAreas.m_fullDrawArea.GetY() + (sizeDiff / 2));
                }
            else if (widthDiff > heightDiff)
                {
                // use the golden ratio for the width if we have enough space for it;
                // otherwise, use whatever width we have, making it more of a square
                const auto goldenRatioWidth =
                    drawAreas.m_fullDrawArea.GetHeight() * math_constants::golden_ratio;
                const auto newWidth =
                    std::min<double>(goldenRatioWidth, drawAreas.m_fullDrawArea.GetWidth());
                const auto newWidthDiff = drawAreas.m_fullDrawArea.GetWidth() - newWidth;
                drawAreas.m_fullDrawArea.SetWidth(newWidth);
                drawAreas.m_fullDrawArea.SetX(drawAreas.m_fullDrawArea.GetX() + (newWidthDiff / 2));
                }
            }

        // make the connection line for inner slices and their labels
        // poke out a little from the pie
        drawAreas.m_outerPieDrawArea = drawAreas.m_pieDrawArea;
        drawAreas.m_outerPieDrawArea.width *= 1.1;
        drawAreas.m_outerPieDrawArea.height *= 1.1;
        drawAreas.m_outerPieDrawArea.Offset(
            wxPoint((drawAreas.m_pieDrawArea.width - drawAreas.m_outerPieDrawArea.width) / 2,
                    (drawAreas.m_pieDrawArea.height - drawAreas.m_outerPieDrawArea.height) / 2));

        double smallestOuterLabelFontSize{ GetBottomXAxis().GetFont().GetFractionalPointSize() };

        // shrinks an outer label to fit within the plotting area
        // and also draws a connection line from the label to the pie slice
        GutterLabels gutterLabels;

        // outer (main) pie
        DrawOuterPie(dc, gutterLabels, drawAreas, smallestOuterLabelFontSize, addedObjects);

        // inner pie
        DrawInnerPie(dc, gutterLabels, drawAreas, smallestOuterLabelFontSize, addedObjects);

        // sort top quadrant labels (top-to-bottom)
        std::ranges::sort(gutterLabels.m_outerTopLeftLabelAndLines,
                          [](const auto& lhv, const auto& rhv) noexcept
                          {
                              wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
                              wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
                              return lhv.first->GetAnchorPoint().y < rhv.first->GetAnchorPoint().y;
                          });
        // reverse bottom quadrant sort labels (bottom-to-top)
        std::ranges::sort(gutterLabels.m_outerBottomLeftLabelAndLines,
                          [](const auto& lhv, const auto& rhv) noexcept
                          {
                              wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
                              wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
                              return rhv.first->GetAnchorPoint().y < lhv.first->GetAnchorPoint().y;
                          });
        // Make the left-side outer labels (for both rings) have a common font size.
        // Also, adjust their positioning and connection lines (if necessary).
        wxRect previousLabelBoundingBox;
        // left-side labels, top quadrant
        for (size_t i = 0; i < gutterLabels.m_outerTopLeftLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = gutterLabels.m_outerTopLeftLabelAndLines[i];
            if (outerLabel == nullptr)
                {
                continue;
                }
            outerLabel->GetHeaderInfo().GetFont().SetFractionalPointSize(
                smallestOuterLabelFontSize);
            outerLabel->GetFont().SetFractionalPointSize(smallestOuterLabelFontSize);
            if (!IsUsingColorLabels())
                {
                outerLabel->SetFontColor(
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                }

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                const GraphItems::Label* nextLabel =
                    (i + 1 < gutterLabels.m_outerTopLeftLabelAndLines.size() ?
                         gutterLabels.m_outerTopLeftLabelAndLines[i + 1].first.get() :
                         nullptr);
                // push label to the left and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(drawAreas.m_fullDrawArea.GetLeft(),
                            outerLabel->GetAnchorPoint().y +
                                (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                // Does the top label overlap the one below it?
                // If so, push it all the way up to the top.
                if (i == 0 && nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(drawAreas.m_fullDrawArea.GetTopLeft());
                        outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                        }
                    }
                else if (nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetBottomLeft());
                        outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);

            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    // flush always has three points, just a sanity test
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::min(labelBox.GetRight(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight() / 2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside the pie's bounding box
                    if (!drawAreas.m_pieDrawArea.Contains(calculatedMiddlePt))
                        {
                        middlePt.SetAnchorPoint(calculatedMiddlePt);
                        }
                    }
                queueObjectForOffsetting(std::move(outerLine));
                }
            queueObjectForOffsetting(std::move(outerLabel));
            }
        // left-side labels, bottom quadrant
        for (size_t i = 0; i < gutterLabels.m_outerBottomLeftLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = gutterLabels.m_outerBottomLeftLabelAndLines[i];
            if (outerLabel == nullptr)
                {
                continue;
                }
            outerLabel->GetHeaderInfo().GetFont().SetFractionalPointSize(
                smallestOuterLabelFontSize);
            outerLabel->GetFont().SetFractionalPointSize(smallestOuterLabelFontSize);
            if (!IsUsingColorLabels())
                {
                outerLabel->SetFontColor(
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                }

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                const GraphItems::Label* nextLabel =
                    (i + 1 < gutterLabels.m_outerBottomLeftLabelAndLines.size() ?
                         gutterLabels.m_outerBottomLeftLabelAndLines[i + 1].first.get() :
                         nullptr);
                // push label to the left and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(drawAreas.m_fullDrawArea.GetLeft(),
                            outerLabel->GetAnchorPoint().y -
                                (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                // Does the bottom label overlap the one above it?
                // If so, push it all the way down to the bottom.
                if (i == 0 && nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(drawAreas.m_fullDrawArea.GetBottomLeft());
                        outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                        }
                    else
                        {
                        nextLabelBox = nextLabel->GetBoundingBox(dc);
                        nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetLeft());
                        nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight() / 2));
                        if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                            {
                            outerLabel->SetAnchorPoint(
                                // there is already padding on the labels, OK to
                                // not have space between them
                                previousLabelBoundingBox.GetTopLeft());
                            outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                            }
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);

            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // Connect last point to middle of label's right side;
                    // unless the label is oddly wide, then align with the middle point.
                    lastPt.SetAnchorPoint(
                        wxPoint(std::min(labelBox.GetRight(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight() / 2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside the pie's bounding box
                    if (!drawAreas.m_pieDrawArea.Contains(calculatedMiddlePt))
                        {
                        middlePt.SetAnchorPoint(calculatedMiddlePt);
                        }
                    }
                queueObjectForOffsetting(std::move(outerLine));
                }
            queueObjectForOffsetting(std::move(outerLabel));
            }

        // do the same for the right-side labels
        std::ranges::sort(gutterLabels.m_outerTopRightLabelAndLines,
                          [](const auto& lhv, const auto& rhv) noexcept
                          {
                              wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
                              wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
                              return lhv.first->GetAnchorPoint().y < rhv.first->GetAnchorPoint().y;
                          });
        std::ranges::sort(gutterLabels.m_outerBottomRightLabelAndLines,
                          [](const auto& lhv, const auto& rhv) noexcept
                          {
                              wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
                              wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
                              return rhv.first->GetAnchorPoint().y < lhv.first->GetAnchorPoint().y;
                          });

        // center hole, if a donut
        if (IsIncludingDonutHole())
            {
            const wxPoint centerPt(
                drawAreas.m_pieDrawArea.GetLeft() + (drawAreas.m_pieDrawArea.GetWidth() / 2),
                drawAreas.m_pieDrawArea.GetTop() + (drawAreas.m_pieDrawArea.GetHeight() / 2));
            auto donutHole =
                std::make_unique<GraphItems::Point2D>(GraphItems::GraphItemInfo()
                                                          .Brush(GetDonutHoleColor())
                                                          .DPIScaling(GetDPIScaleFactor())
                                                          .Scaling(GetScaling())
                                                          .Selectable(false)
                                                          .Pen(GetPen())
                                                          .Anchoring(Anchoring::Center)
                                                          .AnchorPoint(centerPt),
                                                      0);
            const double holeRadius{
                (drawAreas.m_pieDrawArea.GetWidth() * GetDonutHoleProportion()) / 2
            };
            donutHole->SetRadius(donutHole->DownscaleFromScreenAndCanvas(holeRadius));

            queueObjectForOffsetting(std::move(donutHole));
            if (!GetDonutHoleLabel().GetText().empty())
                {
                auto donutHoleLabel = std::make_unique<GraphItems::Label>(GetDonutHoleLabel());
                donutHoleLabel->GetGraphItemInfo()
                    .Pen(wxNullPen)
                    .DPIScaling(GetDPIScaleFactor())
                    .Scaling(GetScaling())
                    .LabelPageVerticalAlignment(PageVerticalAlignment::Centered)
                    .LabelPageHorizontalAlignment(PageHorizontalAlignment::Centered)
                    .Anchoring(Anchoring::Center)
                    .AnchorPoint(centerPt);

                wxPoint donutHoleLabelCorner{ centerPt };
                auto rectWithinCircleWidth = geometry::radius_to_inner_rect_width(holeRadius);
                donutHoleLabelCorner.x -= rectWithinCircleWidth / 2;
                donutHoleLabelCorner.y -= rectWithinCircleWidth / 2;
                donutHoleLabel->SetBoundingBox(
                    wxRect(donutHoleLabelCorner,
                           wxSize(rectWithinCircleWidth, rectWithinCircleWidth)),
                    dc, GetScaling());
                queueObjectForOffsetting(std::move(donutHoleLabel));
                }
            }

        // right-side labels, top quadrant (drawn clockwise)
        for (size_t i = 0; i < gutterLabels.m_outerTopRightLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = gutterLabels.m_outerTopRightLabelAndLines[i];
            if (outerLabel == nullptr)
                {
                continue;
                }
            outerLabel->GetHeaderInfo().GetFont().SetFractionalPointSize(
                smallestOuterLabelFontSize);
            outerLabel->GetFont().SetFractionalPointSize(smallestOuterLabelFontSize);
            if (!IsUsingColorLabels())
                {
                outerLabel->SetFontColor(
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                }

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                const GraphItems::Label* nextLabel =
                    (i + 1 < gutterLabels.m_outerTopRightLabelAndLines.size() ?
                         gutterLabels.m_outerTopRightLabelAndLines[i + 1].first.get() :
                         nullptr);
                // push label to the right and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(drawAreas.m_fullDrawArea.GetRight(),
                            outerLabel->GetAnchorPoint().y +
                                (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                // Does the top label overlap the one below it?
                // If so, push it all the way up to the top.
                if (i == 0 && nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(drawAreas.m_fullDrawArea.GetTopRight());
                        outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                        }
                    }
                else if (nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetBottomRight());
                        outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);
            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::max(labelBox.GetLeft(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight() / 2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside the pie's bounding box
                    if (!drawAreas.m_pieDrawArea.Contains(calculatedMiddlePt))
                        {
                        middlePt.SetAnchorPoint(calculatedMiddlePt);
                        }
                    }
                queueObjectForOffsetting(std::move(outerLine));
                }
            queueObjectForOffsetting(std::move(outerLabel));
            }
        // right-side labels, bottom quadrant (drawn counter clockwise)
        for (size_t i = 0; i < gutterLabels.m_outerBottomRightLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = gutterLabels.m_outerBottomRightLabelAndLines[i];
            if (outerLabel == nullptr)
                {
                continue;
                }
            outerLabel->GetHeaderInfo().GetFont().SetFractionalPointSize(
                smallestOuterLabelFontSize);
            outerLabel->GetFont().SetFractionalPointSize(smallestOuterLabelFontSize);
            if (!IsUsingColorLabels())
                {
                outerLabel->SetFontColor(
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                }

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                const GraphItems::Label* nextLabel =
                    (i + 1 < gutterLabels.m_outerBottomRightLabelAndLines.size() ?
                         gutterLabels.m_outerBottomRightLabelAndLines[i + 1].first.get() :
                         nullptr);
                // push label to the right and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(drawAreas.m_fullDrawArea.GetRight(),
                            outerLabel->GetAnchorPoint().y -
                                (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                // Does the bottom label overlap the one above it?
                // If so, push it all the way down to the bottom.
                if (i == 0 && nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(drawAreas.m_fullDrawArea.GetBottomRight());
                        outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                        }
                    }
                else if (nextLabel != nullptr)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(drawAreas.m_fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetTopRight());
                        outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);

            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::max(labelBox.GetLeft(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight() / 2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside the pie's bounding box
                    if (!drawAreas.m_pieDrawArea.Contains(calculatedMiddlePt))
                        {
                        middlePt.SetAnchorPoint(calculatedMiddlePt);
                        }
                    }
                queueObjectForOffsetting(std::move(outerLine));
                }
            queueObjectForOffsetting(std::move(outerLabel));
            }

        // Re-splits labels that may be been split. This is meant to take advantage
        // of having more real estate available; newlines will be removed, and
        // then will be re-split, hopefully having no (or at least less) newlines now.
        const auto refitLabelAndLine = [&dc, this](auto& labelAndLine, const Side side)
        {
            if (labelAndLine.first->GetLineCount() < 2)
                {
                return;
                }
            wxString text = labelAndLine.first->GetText();
            text.Replace(L"\n", " ");
            labelAndLine.first->SetText(text);
            auto labelBox = labelAndLine.first->GetBoundingBox(dc);
            if (!GraphItems::Polygon::IsRectInsideRect(labelBox, GetPlotAreaBoundingBox()))
                {
                if ((GetOuterLabelDisplay() == BinLabelDisplay::BinNameAndPercentage ||
                     GetOuterLabelDisplay() == BinLabelDisplay::BinNameAndValue ||
                     GetOuterLabelDisplay() == BinLabelDisplay::BinValueAndPercentage) ||
                    !labelAndLine.first->SplitTextAuto())
                    {
                    labelAndLine.first->SplitTextToFitLength(
                        labelAndLine.first->GetText().length() * math_constants::third);
                    }
                }
            // reconnect to its line
            labelBox = labelAndLine.first->GetBoundingBox(dc);
            if (labelAndLine.second != nullptr && labelAndLine.second->GetPoints().size())
                {
                if (side == Side::Right)
                    {
                    wxPoint connectionPt = labelAndLine.second->GetPoints().back().GetAnchorPoint();
                    connectionPt.y += labelBox.GetHeight() / 2;
                    labelAndLine.first->SetAnchorPoint(connectionPt);
                    labelAndLine.first->SetAnchoring(Anchoring::BottomLeftCorner);
                    }
                else
                    {
                    wxPoint connectionPt = labelAndLine.second->GetPoints().back().GetAnchorPoint();
                    connectionPt.y += labelBox.GetHeight() / 2;
                    labelAndLine.first->SetAnchorPoint(connectionPt);
                    labelAndLine.first->SetAnchoring(Anchoring::BottomRightCorner);
                    }
                }
        };

        // If we have an empty gutter, then shift everything over and give that real estate
        // to the other gutter (if the client is requesting that behavior and there
        // aren't any margin notes).
        if (HasDynamicMargins() &&
            (!GetLeftMarginNote().IsShown() || GetLeftMarginNote().GetText().empty()) &&
            (!GetRightMarginNote().IsShown() || GetRightMarginNote().GetText().empty()))
            {
            // if both gutters are empty, then no point in moving the chart around
            // (just keep it centered)
            if (gutterLabels.m_outerTopLeftLabelAndLines.empty() &&
                gutterLabels.m_outerBottomLeftLabelAndLines.empty() &&
                gutterLabels.m_outerTopRightLabelAndLines.empty() &&
                gutterLabels.m_outerBottomRightLabelAndLines.empty())
                { /*no-op*/
                }
            // empty left gutter
            if (gutterLabels.m_outerTopLeftLabelAndLines.empty() &&
                gutterLabels.m_outerBottomLeftLabelAndLines.empty())
                {
                const auto xDiff = drawAreas.m_pieDrawArea.GetX() - GetPlotAreaBoundingBox().GetX();
                // move everything over to the left
                std::ranges::for_each(addedObjects,
                                      [&](auto& obj)
                                      {
                                          if (obj != nullptr)
                                              {
                                              obj->Offset(-xDiff, 0);
                                              }
                                      });
                // refit outer right labels now that there is more real estate for them
                std::ranges::for_each(gutterLabels.m_outerTopRightLabelAndLines,
                                      [&](auto& labelAndLine)
                                      {
                                          if (labelAndLine.first != nullptr)
                                              {
                                              refitLabelAndLine(labelAndLine, Side::Right);
                                              }
                                      });
                std::ranges::for_each(gutterLabels.m_outerBottomRightLabelAndLines,
                                      [&](auto& labelAndLine)
                                      {
                                          if (labelAndLine.first != nullptr)
                                              {
                                              refitLabelAndLine(labelAndLine, Side::Right);
                                              }
                                      });
                }
            // empty right gutter
            else if (gutterLabels.m_outerTopRightLabelAndLines.empty() &&
                     gutterLabels.m_outerBottomRightLabelAndLines.empty())
                {
                const auto xDiff = drawAreas.m_pieDrawArea.GetX() - GetPlotAreaBoundingBox().GetX();
                // move everything over to the right
                std::ranges::for_each(addedObjects,
                                      [&](auto& obj)
                                      {
                                          if (obj != nullptr)
                                              {
                                              obj->Offset(xDiff, 0);
                                              }
                                      });
                // refit outer left labels now that there is more real estate for them
                std::ranges::for_each(gutterLabels.m_outerTopLeftLabelAndLines,
                                      [&](auto& labelAndLine)
                                      {
                                          if (labelAndLine.first != nullptr)
                                              {
                                              refitLabelAndLine(labelAndLine, Side::Left);
                                              }
                                      });
                std::ranges::for_each(gutterLabels.m_outerBottomLeftLabelAndLines,
                                      [&](auto& labelAndLine)
                                      {
                                          if (labelAndLine.first != nullptr)
                                              {
                                              refitLabelAndLine(labelAndLine, Side::Left);
                                              }
                                      });
                }
            }

        for (auto& addedObject : addedObjects)
            {
            AddObject(std::move(addedObject));
            }

        // see if there is a note to show in an empty gutter (if there is one)
        if (!GetLeftMarginNote().GetText().empty())
            {
            const auto xDiff = drawAreas.m_pieDrawArea.GetX() - GetPlotAreaBoundingBox().GetX();
            const auto marginRect = wxRect(GetPlotAreaBoundingBox().GetTopLeft(),
                                           wxSize(xDiff, GetPlotAreaBoundingBox().GetHeight()));

            auto gutterLabel = std::make_unique<GraphItems::Label>(GetLeftMarginNote());
            gutterLabel->GetGraphItemInfo()
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Padding(4, 4, 4, 4)
                .Selectable(true)
                .Anchoring(Anchoring::TopLeftCorner)
                .AnchorPoint(GetPlotAreaBoundingBox().GetTopLeft());
            gutterLabel->SplitTextToFitBoundingBox(dc, marginRect.GetSize());
            gutterLabel->SetBoundingBox(marginRect, dc, GetScaling());

            AddObject(std::move(gutterLabel));
            }
        if (!GetRightMarginNote().GetText().empty())
            {
            const auto xDiff = drawAreas.m_pieDrawArea.GetX() - GetPlotAreaBoundingBox().GetX();
            const auto marginRect = wxRect{ wxPoint{ drawAreas.m_pieDrawArea.GetTopRight().x,
                                                     GetPlotAreaBoundingBox().GetTop() },
                                            wxSize{ xDiff, GetPlotAreaBoundingBox().GetHeight() } };

            auto gutterLabel = std::make_unique<GraphItems::Label>(GetRightMarginNote());
            gutterLabel->GetGraphItemInfo()
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Padding(4, 4, 4, 4)
                .Selectable(true)
                .Anchoring(Anchoring::TopRightCorner)
                .AnchorPoint(GetPlotAreaBoundingBox().GetTopRight());
            gutterLabel->SplitTextToFitBoundingBox(dc, marginRect.GetSize());
            gutterLabel->SetBoundingBox(marginRect, dc, GetScaling());

            AddObject(std::move(gutterLabel));
            }

        if (GetPieStyle() == PieStyle::Clockface)
            {
            AddClockTicks(drawAreas);
            AddClockHands(drawAreas);
            }
        else if (GetPieStyle() == PieStyle::CheezePizza)
            {
            AddCrustRing(drawAreas);
            AddToastedCheeseSpots(drawAreas);
            }
        else if (GetPieStyle() == PieStyle::PepperoniCheezePizza)
            {
            AddCrustRing(drawAreas);
            AddToastedCheeseSpots(drawAreas);
            AddPepperoni(drawAreas);
            }
        else if (GetPieStyle() == PieStyle::CoffeeRing)
            {
            AddCrustRing(drawAreas);
            AddPartialCoffeeRingStain(drawAreas);
            AddCoffeeInnerStains(drawAreas);
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddCoffeeInnerStains(const DrawAreas& drawAreas)
        {
        const wxRect pieRect = drawAreas.m_pieDrawArea;
        const wxRect fullRect = drawAreas.m_fullDrawArea;

        // expand the scatter area beyond the pie but within the full draw area
        wxRect scatterRect = pieRect;
        scatterRect.Inflate(wxRound((fullRect.GetWidth() - pieRect.GetWidth()) * 0.4),
                            wxRound((fullRect.GetHeight() - pieRect.GetHeight()) * 0.4));

        constexpr uint32_t COFFEE_SEED{ 0xC0FFEE77 };

        const auto mixSeed = [](uint32_t x) -> uint32_t
        {
            x ^= x >> 16;
            x *= 0x7feb352d;
            x ^= x >> 15;
            x *= 0x846ca68b;
            x ^= x >> 16;
            return x;
        };

        // color palette with varying tones and opacities
        const std::array<wxColour, 4> stainColors{
            wxColour{ 195, 175, 145, 50 }, // warm medium
            wxColour{ 175, 150, 115, 55 }, // darker brown
            wxColour{ 210, 185, 150, 45 }, // soft tan
            wxColour{ 185, 160, 125, 50 }  // earthy mid-tone
        };

            // 1 large stain on the left, pushed halfway outside the ring
            {
            const uint32_t seed = mixSeed(COFFEE_SEED + 1117U);
            const double sizeScale = 1.0 + 0.3 * HashToUnitInterval(seed ^ 0x4444U);
            const wxColour& color = stainColors[seed % stainColors.size()];
            // 180 degrees = left side, high distance to push outside ring
            DrawSingleCoffeeStain(scatterRect, seed, color, sizeScale, 0.9, 1.05, 180.0);
            }

        // 6 small stains scattered around (avoiding the left where the large one is)
        constexpr int smallStainCount{ 6 };
        for (int i = 0; i < smallStainCount; ++i)
            {
            const uint32_t seed = mixSeed(COFFEE_SEED + static_cast<uint32_t>((i + 30) * 557));
            const double sizeScale = 0.2 + 0.25 * HashToUnitInterval(seed ^ 0x6666U);
            wxColour color = stainColors[seed % stainColors.size()];
            // small stains slightly more opaque
            color = wxColour(color.Red(), color.Green(), color.Blue(),
                             std::min(255, static_cast<int>(color.Alpha() * 1.3)));
            // spread around the right side and top/bottom (avoiding left where large stain is)
            // angles: ~280, 335, 30, 85, 140, 195 (wrapping around, mostly right side)
            const double angleOffset = i * 55.0 + 280.0;
            DrawSingleCoffeeStain(scatterRect, seed, color, sizeScale, 0.2, 0.9, angleOffset);
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddPartialCoffeeRingStain(const DrawAreas& drawAreas)
        {
        // This draws a partial coffee ring stain, like a previous cup was placed
        // slightly offset from the current one. The arc spans about 1/4 of the
        // circumference and is offset to the right.

        constexpr int sampleCount{ 60 };
        constexpr double arcSpanDegrees{ 90.0 };
        constexpr double arcStartDegrees{ 135.0 }; // upper-left quadrant

        // offset to the left and slightly up
        const int xOffset = static_cast<int>(ScaleToScreenAndCanvas(-25));
        const int yOffset = static_cast<int>(ScaleToScreenAndCanvas(-8));

        const double baseRingThickness{ ScaleToScreenAndCanvas(6) };
        const double ringInflation{ ScaleToScreenAndCanvas(8) };

        constexpr uint32_t partialRingSeed{ 0xCAFE1234 };

        // coffee stain color - more translucent than the main ring
        const wxColour stainColor{ 180, 150, 110, 60 };
        const wxColour stainColorFaded{ 180, 150, 110, 25 };

        // create offset pie rect
        wxRect offsetPieRect = drawAreas.m_pieDrawArea;
        offsetPieRect.Offset(xOffset, yOffset);
        offsetPieRect.Inflate(wxRound(ringInflation));

        // taper factor: 1.0 at center, fading to 0 at ends
        // Uses a smooth cosine-based falloff for organic appearance
        const auto getTaperFactor = [](double t) -> double
        {
            // t is 0 at start, 1 at end of arc
            // taper the first and last 20% of the arc
            constexpr double taperZone{ 0.20 };
            if (t < taperZone)
                {
                // fade in from 0 to 1 over the first taperZone
                const double localT = t / taperZone;
                return 0.5 * (1.0 - std::cos(localT * std::numbers::pi));
                }
            if (t > (1.0 - taperZone))
                {
                // fade out from 1 to 0 over the last taperZone
                const double localT = (t - (1.0 - taperZone)) / taperZone;
                return 0.5 * (1.0 + std::cos(localT * std::numbers::pi));
                }
            return 1.0;
        };

        // draw multiple overlapping passes for organic look
        constexpr int layerCount{ 3 };
        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
            {
            const uint32_t layerSeed = partialRingSeed + static_cast<uint32_t>(layerIndex * 777);

            // each layer has slight random offset for natural variation
            const int layerXJitter = wxRound((HashToUnitInterval(layerSeed ^ 0xAAAAU) - 0.5) *
                                             ScaleToScreenAndCanvas(3));
            const int layerYJitter = wxRound((HashToUnitInterval(layerSeed ^ 0xBBBBU) - 0.5) *
                                             ScaleToScreenAndCanvas(3));

            wxRect layerRect = offsetPieRect;
            layerRect.Offset(layerXJitter, layerYJitter);

            const double layerThickness =
                baseRingThickness * (0.7 + 0.6 * HashToUnitInterval(layerSeed ^ 0xCCCCU));

            wxRect innerRect = layerRect;
            innerRect.Inflate(-wxRound(layerThickness));

            std::vector<wxPoint> outerPoints;
            std::vector<wxPoint> innerPoints;
            outerPoints.reserve(sampleCount + 1);
            innerPoints.reserve(sampleCount + 1);

            for (int sampleIndex = 0; sampleIndex <= sampleCount; ++sampleIndex)
                {
                const auto tap = safe_divide<double>(sampleIndex, sampleCount);
                const double angleDegrees = arcStartDegrees + tap * arcSpanDegrees;

                // add some irregularity like the main crust ring
                const double noise = RingIrregularity(angleDegrees, layerSeed);
                const double irregularity = ScaleToScreenAndCanvas(2) * noise;

                wxRect adjustedOuter = layerRect;
                wxRect adjustedInner = innerRect;
                adjustedOuter.Inflate(wxRound(irregularity));
                adjustedInner.Inflate(wxRound(irregularity * 0.4));

                // apply taper to the thickness at the ends
                const double taper = getTaperFactor(tap);

                // when taper < 1, bring inner points closer to outer (reducing thickness)
                if (taper < 1.0)
                    {
                    const wxPoint outerPt = GetEllipsePointFromRect(adjustedOuter, angleDegrees);
                    const wxPoint innerPtFull =
                        GetEllipsePointFromRect(adjustedInner, angleDegrees);

                    // interpolate inner point toward outer based on taper
                    const double invTaper = 1.0 - taper;
                    const wxPoint innerPtTapered{
                        wxRound(innerPtFull.x + (outerPt.x - innerPtFull.x) * invTaper),
                        wxRound(innerPtFull.y + (outerPt.y - innerPtFull.y) * invTaper)
                    };

                    outerPoints.push_back(outerPt);
                    innerPoints.push_back(innerPtTapered);
                    }
                else
                    {
                    outerPoints.push_back(GetEllipsePointFromRect(adjustedOuter, angleDegrees));
                    innerPoints.push_back(GetEllipsePointFromRect(adjustedInner, angleDegrees));
                    }
                }

            // build polygon from outer + reversed inner
            std::vector<wxPoint> ringPolygon;
            ringPolygon.reserve(outerPoints.size() + innerPoints.size());
            ringPolygon.insert(ringPolygon.end(), outerPoints.begin(), outerPoints.end());
            ringPolygon.insert(ringPolygon.end(), innerPoints.rbegin(), innerPoints.rend());

            // use faded color for outer layers, stronger for inner
            const wxColour layerColor = (layerIndex == 0) ? stainColor : stainColorFaded;

            auto ringPoly =
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                          .Brush(wxBrush(layerColor))
                                                          .Pen(wxNullPen)
                                                          .Scaling(GetScaling())
                                                          .DPIScaling(GetDPIScaleFactor())
                                                          .Selectable(false),
                                                      ringPolygon);

            AddObject(std::move(ringPoly));
            }

        // Second partial ring: outside right of the main ring
        // This one is more subtle and covers a different arc segment
        constexpr double arcSpanDegrees2{ 70.0 };
        constexpr double arcStartDegrees2{ 320.0 }; // lower-right quadrant

        // offset to the right and slightly down
        const int xOffset2 = static_cast<int>(ScaleToScreenAndCanvas(30));
        const int yOffset2 = static_cast<int>(ScaleToScreenAndCanvas(12));

        const double baseRingThickness2{ ScaleToScreenAndCanvas(4) };
        const double ringInflation2{ ScaleToScreenAndCanvas(12) }; // further out

        constexpr uint32_t partialRingSeed2{ 0xBEAD5678 };

        // even more translucent for subtlety
        const wxColour stainColor2{ 165, 135, 95, 45 };
        const wxColour stainColorFaded2{ 165, 135, 95, 18 };

        wxRect offsetPieRect2 = drawAreas.m_pieDrawArea;
        offsetPieRect2.Offset(xOffset2, yOffset2);
        offsetPieRect2.Inflate(wxRound(ringInflation2));

        constexpr int layerCount2{ 2 };
        for (int layerIndex = 0; layerIndex < layerCount2; ++layerIndex)
            {
            const uint32_t layerSeed = partialRingSeed2 + static_cast<uint32_t>(layerIndex * 919);

            const int layerXJitter = wxRound((HashToUnitInterval(layerSeed ^ 0xDDDDU) - 0.5) *
                                             ScaleToScreenAndCanvas(2));
            const int layerYJitter = wxRound((HashToUnitInterval(layerSeed ^ 0xEEEEU) - 0.5) *
                                             ScaleToScreenAndCanvas(2));

            wxRect layerRect = offsetPieRect2;
            layerRect.Offset(layerXJitter, layerYJitter);

            const double layerThickness =
                baseRingThickness2 * (0.8 + 0.4 * HashToUnitInterval(layerSeed ^ 0xFFFFU));

            wxRect innerRect = layerRect;
            innerRect.Inflate(-wxRound(layerThickness));

            std::vector<wxPoint> outerPoints;
            std::vector<wxPoint> innerPoints;
            outerPoints.reserve(sampleCount + 1);
            innerPoints.reserve(sampleCount + 1);

            for (int sampleIndex = 0; sampleIndex <= sampleCount; ++sampleIndex)
                {
                const auto tap = safe_divide<double>(sampleIndex, sampleCount);
                const double angleDegrees = arcStartDegrees2 + tap * arcSpanDegrees2;

                const double noise = RingIrregularity(angleDegrees, layerSeed);
                const double irregularity = ScaleToScreenAndCanvas(1.5) * noise;

                wxRect adjustedOuter = layerRect;
                wxRect adjustedInner = innerRect;
                adjustedOuter.Inflate(wxRound(irregularity));
                adjustedInner.Inflate(wxRound(irregularity * 0.3));

                const double taper = getTaperFactor(tap);

                if (taper < 1.0)
                    {
                    const wxPoint outerPt = GetEllipsePointFromRect(adjustedOuter, angleDegrees);
                    const wxPoint innerPtFull =
                        GetEllipsePointFromRect(adjustedInner, angleDegrees);

                    const double invTaper = 1.0 - taper;
                    const wxPoint innerPtTapered{
                        wxRound(innerPtFull.x + (outerPt.x - innerPtFull.x) * invTaper),
                        wxRound(innerPtFull.y + (outerPt.y - innerPtFull.y) * invTaper)
                    };

                    outerPoints.push_back(outerPt);
                    innerPoints.push_back(innerPtTapered);
                    }
                else
                    {
                    outerPoints.push_back(GetEllipsePointFromRect(adjustedOuter, angleDegrees));
                    innerPoints.push_back(GetEllipsePointFromRect(adjustedInner, angleDegrees));
                    }
                }

            std::vector<wxPoint> ringPolygon;
            ringPolygon.reserve(outerPoints.size() + innerPoints.size());
            ringPolygon.insert(ringPolygon.end(), outerPoints.begin(), outerPoints.end());
            ringPolygon.insert(ringPolygon.end(), innerPoints.rbegin(), innerPoints.rend());

            const wxColour layerColor = (layerIndex == 0) ? stainColor2 : stainColorFaded2;

            auto ringPoly =
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                          .Brush(wxBrush(layerColor))
                                                          .Pen(wxNullPen)
                                                          .Scaling(GetScaling())
                                                          .DPIScaling(GetDPIScaleFactor())
                                                          .Selectable(false),
                                                      ringPolygon);

            AddObject(std::move(ringPoly));
            }
        }

    //----------------------------------------------------------------
    void PieChart::DrawSingleCoffeeStain(const wxRect& pieRect, uint32_t seed,
                                         const wxColour& color, double sizeScale,
                                         double minDistancePct, double maxDistancePct,
                                         double angleOffset)
        {
        // more samples for smoother curves
        constexpr int STAIN_SAMPLES{ 48 };

        const wxPoint center(pieRect.GetX() + pieRect.GetWidth() / 2,
                             pieRect.GetY() + pieRect.GetHeight() / 2);

        const double pieRadius = std::min(pieRect.GetWidth(), pieRect.GetHeight()) / 2.0;

        const double minRadius = ScaleToScreenAndCanvas(32);
        const double maxRadius = ScaleToScreenAndCanvas(85);

        const double distanceRange = maxDistancePct - minDistancePct;
        const double distance =
            pieRadius * (minDistancePct + distanceRange * HashToUnitInterval(seed ^ 0x1111U));

        const double angle =
            std::fmod(HashToUnitInterval(seed ^ 0x2222U) * 360.0 + angleOffset, 360.0);

        const wxPoint stainCenter(
            wxRound(center.x + std::cos(geometry::degrees_to_radians(angle)) * distance),
            wxRound(center.y + std::sin(geometry::degrees_to_radians(angle)) * distance));

        const double baseRadius =
            (minRadius + HashToUnitInterval(seed ^ 0x3333U) * (maxRadius - minRadius)) * sizeScale;

        // use low-frequency sine waves for smooth organic wobble instead of per-sample random
        const double wavePhase1 = HashToUnitInterval(seed ^ 0x4444U) * 360.0;
        const double wavePhase2 = HashToUnitInterval(seed ^ 0x5555U) * 360.0;
        const double wavePhase3 = HashToUnitInterval(seed ^ 0x6666U) * 360.0;
        const double waveAmp1 = 0.06 + 0.06 * HashToUnitInterval(seed ^ 0x7777U);
        const double waveAmp2 = 0.03 + 0.04 * HashToUnitInterval(seed ^ 0x8888U);
        const double waveAmp3 = 0.02 + 0.02 * HashToUnitInterval(seed ^ 0x9999U);

        std::vector<wxPoint> points;
        points.reserve(STAIN_SAMPLES);

        for (int sample = 0; sample < STAIN_SAMPLES; ++sample)
            {
            const double angleDeg = 360.0 * safe_divide<double>(sample, STAIN_SAMPLES);

            // smooth wobble using layered sine waves (like simplex noise)
            const double wobble =
                1.0 +
                waveAmp1 * std::sin(geometry::degrees_to_radians(angleDeg * 2.0 + wavePhase1)) +
                waveAmp2 * std::sin(geometry::degrees_to_radians(angleDeg * 3.0 + wavePhase2)) +
                waveAmp3 * std::sin(geometry::degrees_to_radians(angleDeg * 5.0 + wavePhase3));

            // reduced gravity effect to avoid overly elliptical shapes
            const double gravity =
                1.0 + 0.08 * std::sin(geometry::degrees_to_radians(angleDeg - angle));

            const double radius = baseRadius * wobble * gravity;

            points.emplace_back(
                wxRound(stainCenter.x + std::cos(geometry::degrees_to_radians(angleDeg)) * radius),
                wxRound(stainCenter.y + std::sin(geometry::degrees_to_radians(angleDeg)) * radius));
            }

        AddObject(std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                            .Brush(wxBrush(color))
                                                            .Pen(wxNullPen)
                                                            .Scaling(GetScaling())
                                                            .DPIScaling(GetDPIScaleFactor())
                                                            .Selectable(false),
                                                        points));
        }

    //----------------------------------------------------------------
    void PieChart::AddClockTicks(const DrawAreas& drawAreas)
        {
        const double diameter =
            std::min(drawAreas.m_pieDrawArea.GetWidth(), drawAreas.m_pieDrawArea.GetHeight());
        const double radius = diameter * math_constants::half;
        const double cx = drawAreas.m_pieDrawArea.GetLeft() +
                          (drawAreas.m_pieDrawArea.GetWidth() * math_constants::half);
        const double cy = drawAreas.m_pieDrawArea.GetTop() +
                          (drawAreas.m_pieDrawArea.GetHeight() * math_constants::half);

        const double longInnerRadius = radius * 0.91;
        const double shortInnerRadius = radius * 0.95;
        const double outerRadius = radius - ScaleToScreenAndCanvas(1);

        const double longTickHalfWidth = ScaleToScreenAndCanvas(2);
        const double shortTickHalfWidth = ScaleToScreenAndCanvas(1);

        const auto addTick = [this, cx, cy, longInnerRadius, shortInnerRadius, outerRadius,
                              longTickHalfWidth, shortTickHalfWidth](double angleDeg, bool longTick)
        {
            const double angleRad = geometry::degrees_to_radians(angleDeg);

            const double innerRadius = longTick ? longInnerRadius : shortInnerRadius;
            const double halfWidth = longTick ? longTickHalfWidth : shortTickHalfWidth;

            // endpoints along the radial angle
            const wxPoint inner{ static_cast<int>(cx + innerRadius * std::cos(angleRad)),
                                 static_cast<int>(cy + innerRadius * std::sin(angleRad)) };
            const wxPoint outer{ static_cast<int>(cx + outerRadius * std::cos(angleRad)),
                                 static_cast<int>(cy + outerRadius * std::sin(angleRad)) };

            // direction of tick
            wxPoint2DDouble dir(static_cast<double>(outer.x - inner.x),
                                static_cast<double>(outer.y - inner.y));

            const double mag = std::sqrt(dir.m_x * dir.m_x + dir.m_y * dir.m_y);
            if (mag < 1.0)
                {
                return;
                }

            dir.m_x /= mag;
            dir.m_y /= mag;

            // perpendicular
            const wxPoint2DDouble perp(dir.m_y, -dir.m_x);

            // build 4 corners of the pill rectangle
            const wxPoint p1(inner.x + perp.m_x * halfWidth, inner.y + perp.m_y * halfWidth);
            const wxPoint p2(inner.x - perp.m_x * halfWidth, inner.y - perp.m_y * halfWidth);
            const wxPoint p3(outer.x - perp.m_x * halfWidth, outer.y - perp.m_y * halfWidth);
            const wxPoint p4(outer.x + perp.m_x * halfWidth, outer.y + perp.m_y * halfWidth);

            const std::array<wxPoint, 4> pts{ p1, p2, p3, p4 };

            auto tickPoly = std::make_unique<GraphItems::Polygon>(
                GraphItems::GraphItemInfo()
                    .Brush(longTick ? Colors::ColorBrewer::GetColor(Colors::Color::Black, 75) :
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black, 75))
                    .Pen(wxNullPen)
                    .Scaling(GetScaling()),
                pts);

            AddObject(std::move(tickPoly));
        };

        // long ticks: 12, 3, 6, 9
        for (const double deg : { 0.0, 90.0, 180.0, 270.0 })
            {
            addTick(deg, true);
            }

        // short ticks: 5-min increments
        for (const int m : { 5, 10, 20, 25, 35, 40, 50, 55 })
            {
            addTick(m * 6.0, false);
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddPepperoni(const DrawAreas& drawAreas)
        {
        const wxRect pieRect = drawAreas.m_pieDrawArea;

        constexpr int TARGET_PEPPERONI_COUNT{ 32 };
        constexpr uint32_t PEPPERONI_SEED{ 0xBEEF1234 };

        const double minRadius = ScaleToScreenAndCanvas(14);
        const double maxRadius = ScaleToScreenAndCanvas(22);

        const wxColour pepperoniFillBase(170, 45, 45);
        const wxColour pepperoniEdgeBase(110, 20, 20);
        const wxColour pepperoniBlisterColor(120, 35, 25, 90);

        const wxPoint pieCenter(pieRect.GetX() + pieRect.GetWidth() / 2,
                                pieRect.GetY() + pieRect.GetHeight() / 2);

        const double crustMargin = maxRadius + ScaleToScreenAndCanvas(14);

        const double maxDistance =
            (std::min(pieRect.GetWidth(), pieRect.GetHeight()) / 2.0) - crustMargin;

        std::vector<wxPoint> acceptedCenters;
        acceptedCenters.reserve(TARGET_PEPPERONI_COUNT);

        const auto angleStepDegrees = safe_divide<double>(360.0, TARGET_PEPPERONI_COUNT);

        for (int slotIndex = 0; slotIndex < TARGET_PEPPERONI_COUNT; ++slotIndex)
            {
            const uint32_t seed = PEPPERONI_SEED + static_cast<uint32_t>(slotIndex * 911);

            // stratified angle with jitter
            const double baseAngleDegrees = slotIndex * angleStepDegrees;

            const double angleJitterDegrees =
                (HashToUnitInterval(seed ^ 0x1111U) - 0.5) * angleStepDegrees * 0.8;

            const double angleDegrees = baseAngleDegrees + angleJitterDegrees;

            const double radialUnit = std::sqrt(HashToUnitInterval(seed ^ 0x2222U));

            const double distance = radialUnit * maxDistance;

            acceptedCenters.emplace_back(
                wxRound(pieCenter.x +
                        std::cos(geometry::degrees_to_radians(angleDegrees)) * distance),
                wxRound(pieCenter.y +
                        std::sin(geometry::degrees_to_radians(angleDegrees)) * distance));
            }

        // pepperoni discs
        for (size_t index = 0; index < acceptedCenters.size(); ++index)
            {
            const uint32_t seed = PEPPERONI_SEED + static_cast<uint32_t>(index * 733);

            const double radius =
                minRadius + HashToUnitInterval(seed ^ 0x3333U) * (maxRadius - minRadius);

            const int discSamples{ 28 };

            std::vector<wxPoint> discPoints;
            discPoints.reserve(discSamples);

            for (int sample = 0; sample < discSamples; ++sample)
                {
                const auto angleDegrees = safe_divide<double>(360.0 * sample, discSamples);

                const double wobble =
                    0.95 + 0.1 * HashToUnitInterval(seed ^ static_cast<uint32_t>(sample * 97));

                const double adjustedRadius = radius * wobble;

                discPoints.emplace_back(
                    wxRound(acceptedCenters[index].x +
                            std::cos(geometry::degrees_to_radians(angleDegrees)) * adjustedRadius),
                    wxRound(acceptedCenters[index].y +
                            std::sin(geometry::degrees_to_radians(angleDegrees)) * adjustedRadius));
                }

            const double shade = 0.9 + 0.15 * HashToUnitInterval(seed ^ 0x4444U);

            const wxColour fillColor(static_cast<unsigned char>(pepperoniFillBase.Red() * shade),
                                     static_cast<unsigned char>(pepperoniFillBase.Green() * shade),
                                     static_cast<unsigned char>(pepperoniFillBase.Blue() * shade),
                                     pepperoniFillBase.Alpha());

            const wxColour edgeColor(static_cast<unsigned char>(pepperoniEdgeBase.Red() * shade),
                                     static_cast<unsigned char>(pepperoniEdgeBase.Green() * shade),
                                     static_cast<unsigned char>(pepperoniEdgeBase.Blue() * shade),
                                     pepperoniEdgeBase.Alpha());

            auto pepperoni = std::make_unique<GraphItems::Polygon>(
                GraphItems::GraphItemInfo()
                    .Brush(wxBrush(fillColor))
                    .Pen(wxPen(edgeColor, ScaleToScreenAndCanvas(1)))
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .Selectable(false),
                discPoints);

            AddObject(std::move(pepperoni));

            constexpr int BLISTER_COUNT{ 3 };
            constexpr int BLISTER_SAMPLES{ 16 };

            // pepperoni browned blisters
            for (int blisterIndex = 0; blisterIndex < BLISTER_COUNT; ++blisterIndex)
                {
                const uint32_t blisterSeed =
                    seed ^ static_cast<uint32_t>(0x9000 + blisterIndex * 131);

                const double blisterRadius =
                    radius * (0.18 + 0.12 * HashToUnitInterval(blisterSeed ^ 0xAAAAU));

                const auto angleStepInDegrees = safe_divide<double>(360.0, BLISTER_COUNT);

                const double baseAngleDegrees = blisterIndex * angleStepInDegrees;

                const double angleJitterDegrees =
                    (HashToUnitInterval(blisterSeed ^ 0xBBBBU) - 0.5) * angleStepInDegrees * 0.6;

                const double angleDegrees = baseAngleDegrees + angleJitterDegrees;

                const double blisterBandMin{ 0.15 };
                const double blisterBandMax{ 0.70 };

                const double bandStep =
                    safe_divide<double>(blisterBandMax - blisterBandMin, BLISTER_COUNT);

                const double bandCenter = blisterBandMin + (blisterIndex + 0.5) * bandStep;

                const double bandJitter =
                    (HashToUnitInterval(blisterSeed ^ 0xCCCCU) - 0.5) * bandStep * 0.6;

                const double offsetDistance =
                    radius * std::clamp(bandCenter + bandJitter, blisterBandMin, blisterBandMax);

                const wxPoint blisterCenter(
                    wxRound(acceptedCenters[index].x +
                            std::cos(geometry::degrees_to_radians(angleDegrees)) * offsetDistance),
                    wxRound(acceptedCenters[index].y +
                            std::sin(geometry::degrees_to_radians(angleDegrees)) * offsetDistance));

                std::vector<wxPoint> blisterPoints;
                blisterPoints.reserve(BLISTER_SAMPLES);

                for (int sample = 0; sample < BLISTER_SAMPLES; ++sample)
                    {
                    const auto sampleAngle = safe_divide<double>(360.0 * sample, BLISTER_SAMPLES);

                    const double wobble =
                        0.8 +
                        0.4 * HashToUnitInterval(blisterSeed ^ static_cast<uint32_t>(sample * 17));

                    blisterPoints.emplace_back(
                        wxRound(blisterCenter.x +
                                std::cos(geometry::degrees_to_radians(sampleAngle)) *
                                    blisterRadius * wobble),
                        wxRound(blisterCenter.y +
                                std::sin(geometry::degrees_to_radians(sampleAngle)) *
                                    blisterRadius * wobble));
                    }

                AddObject(
                    std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                              .Brush(wxBrush(pepperoniBlisterColor))
                                                              .Pen(wxNullPen)
                                                              .Scaling(GetScaling())
                                                              .DPIScaling(GetDPIScaleFactor())
                                                              .Selectable(false),
                                                          blisterPoints));
                }

            const wxColour oreganoColor(80, 95, 60, 110);
            constexpr int SEASONING_COUNT{ 20 };
            const double seasoningRadius = ScaleToScreenAndCanvas(1.3);
            // Oregano specks
            for (int seasoningIndex = 0; seasoningIndex < SEASONING_COUNT; ++seasoningIndex)
                {
                const uint32_t spiceSeed =
                    seed ^ static_cast<uint32_t>(0x7000 + seasoningIndex * 47);

                const double angleDegrees = HashToUnitInterval(spiceSeed ^ 0x1111U) * 360.0;

                const double distance = radius * std::sqrt(HashToUnitInterval(spiceSeed ^ 0x2222U));

                const wxPoint spiceCenter(
                    wxRound(acceptedCenters[index].x +
                            std::cos(geometry::degrees_to_radians(angleDegrees)) * distance),
                    wxRound(acceptedCenters[index].y +
                            std::sin(geometry::degrees_to_radians(angleDegrees)) * distance));

                const wxRect spiceRect(spiceCenter.x - seasoningRadius,
                                       spiceCenter.y - seasoningRadius, seasoningRadius * 2,
                                       seasoningRadius * 2);

                std::array<wxPoint, 8> spicePoints;
                for (int sample = 0; std::cmp_less(sample, spicePoints.size()); ++sample)
                    {
                    spicePoints[sample] = GetEllipsePointFromRect(
                        spiceRect, safe_divide<double>(360.0 * sample, spicePoints.size()));
                    }

                AddObject(std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                                    .Brush(wxBrush(oreganoColor))
                                                                    .Pen(wxNullPen)
                                                                    .Scaling(GetScaling())
                                                                    .DPIScaling(GetDPIScaleFactor())
                                                                    .Selectable(false),
                                                                spicePoints));
                }
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddToastedCheeseSpots(const DrawAreas& drawAreas)
        {
        const wxRect pieRect = drawAreas.m_pieDrawArea;

        constexpr int TARGET_SPOT_COUNT{ 14 };

        const double minRadius = ScaleToScreenAndCanvas(18);
        const double maxRadius = ScaleToScreenAndCanvas(36);

        const double minSpotSpacing = ScaleToScreenAndCanvas(40);

        const wxColour toastedSpotColor(215, 185, 120, 80);
        const wxColour toastedHaloColor(235, 215, 160, 60);

        constexpr uint32_t SPOT_SEED{ 0xBEEFCAFE };

        const wxPoint center(pieRect.GetX() + pieRect.GetWidth() / 2,
                             pieRect.GetY() + pieRect.GetHeight() / 2);

        const double maxHaloRadius = maxRadius * 1.35;
        const double crustSafetyMargin = maxHaloRadius + ScaleToScreenAndCanvas(4);

        const double maxDistance =
            (std::min(pieRect.GetWidth(), pieRect.GetHeight()) / 2.0) - crustSafetyMargin;
        const double maxDistanceEffective = maxDistance * 0.98;

        std::vector<wxPoint> acceptedCenters;
        acceptedCenters.reserve(TARGET_SPOT_COUNT);

        const double angleStep = 360.0 / static_cast<double>(TARGET_SPOT_COUNT);

        for (int slotIndex = 0; slotIndex < TARGET_SPOT_COUNT; ++slotIndex)
            {
            const uint32_t seed = SPOT_SEED + static_cast<uint32_t>(slotIndex * 733);

            const double baseAngle = slotIndex * angleStep;
            const double jitter = (HashToUnitInterval(seed ^ 0x1111U) - 0.5) * angleStep * 0.6;

            const double angle = baseAngle + jitter;

            const double radialT = HashToUnitInterval(seed ^ 0x2222U);
            const double mixT = HashToUnitInterval(seed ^ 0x9999U);

            // 75% outer-cheese bias, 25% center-friendly
            const bool useOuterBias = (mixT < math_constants::half);

            double distance{ 0.0 };

            if (useOuterBias)
                {
                // outward-biased (avoids center dominance)
                const double biasedT = 0.35 + 0.65 * std::pow(radialT, 0.55);
                distance = biasedT * maxDistanceEffective;
                }
            else
                {
                // center-friendly (but still area-correct)
                distance = std::sqrt(radialT) * maxDistanceEffective;
                }

            const wxPoint candidate(
                wxRound(center.x + std::cos(geometry::degrees_to_radians(angle)) * distance),
                wxRound(center.y + std::sin(geometry::degrees_to_radians(angle)) * distance));

            for (const wxPoint& existing : acceptedCenters)
                {
                if (geometry::segment_length(GraphItems::Polygon::PointToPair(candidate),
                                             GraphItems::Polygon::PointToPair(existing)) <
                    minSpotSpacing)
                    {
                    break;
                    }
                }

            acceptedCenters.push_back(candidate);
            }

        // irregular toasted blobs
        constexpr int BLOB_SAMPLES{ 40 };

        for (size_t index = 0; index < acceptedCenters.size(); ++index)
            {
            const uint32_t seed = SPOT_SEED + static_cast<uint32_t>(index * 911);

            const double baseRadius =
                minRadius + HashToUnitInterval(seed ^ 0x3333U) * (maxRadius - minRadius);

                {
                std::vector<wxPoint> haloPoints;
                haloPoints.reserve(BLOB_SAMPLES);

                for (int sample = 0; sample < BLOB_SAMPLES; ++sample)
                    {
                    const double angleDeg = (360.0 * sample) / static_cast<double>(BLOB_SAMPLES);

                    const double wobble =
                        0.85 + 0.4 * HashToUnitInterval(seed ^ static_cast<uint32_t>(sample * 311));

                    const double radius = baseRadius * 1.35 * wobble;

                    haloPoints.emplace_back(
                        static_cast<int>(
                            std::round(acceptedCenters[index].x +
                                       std::cos(geometry::degrees_to_radians(angleDeg)) * radius)),
                        static_cast<int>(
                            std::round(acceptedCenters[index].y +
                                       std::sin(geometry::degrees_to_radians(angleDeg)) * radius)));
                    }

                AddObject(
                    std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                              .Brush(wxBrush(toastedHaloColor))
                                                              .Pen(wxNullPen)
                                                              .Scaling(GetScaling())
                                                              .DPIScaling(GetDPIScaleFactor())
                                                              .Selectable(false),
                                                          haloPoints));
                }

                {
                std::vector<wxPoint> blobPoints;
                blobPoints.reserve(BLOB_SAMPLES);

                for (int sample = 0; sample < BLOB_SAMPLES; ++sample)
                    {
                    const auto angleDeg = safe_divide<double>(360.0 * sample, BLOB_SAMPLES);

                    // gentle radial wobble
                    const double wobble =
                        0.75 + 0.5 * HashToUnitInterval(seed ^ static_cast<uint32_t>(sample * 131));

                    const double radius = baseRadius * wobble;

                    const wxPoint point(
                        static_cast<int>(
                            std::round(acceptedCenters[index].x +
                                       std::cos(geometry::degrees_to_radians(angleDeg)) * radius)),
                        static_cast<int>(
                            std::round(acceptedCenters[index].y +
                                       std::sin(geometry::degrees_to_radians(angleDeg)) * radius)));

                    blobPoints.push_back(point);
                    }

                auto toastedSpot =
                    std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                              .Brush(wxBrush(toastedSpotColor))
                                                              .Pen(wxNullPen)
                                                              .Scaling(GetScaling())
                                                              .DPIScaling(GetDPIScaleFactor())
                                                              .Selectable(false),
                                                          blobPoints);

                AddObject(std::move(toastedSpot));
                }
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddCrustRing(const DrawAreas& drawAreas)
        {
        // resolution around the circle
        constexpr int SAMPLE_COUNT{ 180 };
        const double baseCrustThickness{ ScaleToScreenAndCanvas(10) };
        const double baseCrustInflation{ ScaleToScreenAndCanvas(6) };
        const double irregularityAmplitude{ ScaleToScreenAndCanvas(3) };
        constexpr int CRUST_LAYER_COUNT{ 4 };

        constexpr uint32_t CRUST_SEED{ 0xC0FFEE };

        const wxColour doughColor{ 232, 205, 160, 220 };
        const wxColour toastedColor{ 176, 120, 70, 90 };

        // layered crust passes
        for (int crustLayerIndex = 0; crustLayerIndex < CRUST_LAYER_COUNT; ++crustLayerIndex)
            {
            const uint32_t layerSeed = CRUST_SEED + static_cast<uint32_t>(crustLayerIndex * 1337);

            const int horizontalOffset =
                wxRound((HashToUnitInterval(layerSeed ^ 0xA5A5U) - math_constants::half) *
                        ScaleToScreenAndCanvas(3));
            const int verticalOffset =
                wxRound((HashToUnitInterval(layerSeed ^ 0x5A5AU) - math_constants::half) *
                        ScaleToScreenAndCanvas(3));

            const double layerThickness =
                baseCrustThickness * (0.85 + 0.15 * HashToUnitInterval(layerSeed ^ 0x1234U));

            const double layerIrregularity =
                irregularityAmplitude * (0.70 + 0.60 * HashToUnitInterval(layerSeed ^ 0x5678U));

            wxRect outerEllipseRect = drawAreas.m_pieDrawArea;
            outerEllipseRect.Inflate(wxRound(baseCrustInflation));
            outerEllipseRect.Offset(horizontalOffset, verticalOffset);

            wxRect innerEllipseRect = drawAreas.m_pieDrawArea;
            innerEllipseRect.Inflate(wxRound(baseCrustInflation - layerThickness));
            innerEllipseRect.Offset(horizontalOffset, verticalOffset);

            std::vector<wxPoint> outerRingPoints;
            std::vector<wxPoint> innerRingPoints;

            outerRingPoints.reserve(SAMPLE_COUNT);
            innerRingPoints.reserve(SAMPLE_COUNT);

            for (int sampleIndex = 0; sampleIndex <= SAMPLE_COUNT; ++sampleIndex)
                {
                const auto angleDegrees = safe_divide<double>(360.0 * sampleIndex, SAMPLE_COUNT);

                const double doughNoise = RingIrregularity(angleDegrees, layerSeed);

                const int outerInflationOffset = wxRound(doughNoise * layerIrregularity);
                const int innerInflationOffset = wxRound(doughNoise * layerIrregularity * 0.35);

                wxRect adjustedOuterRect = outerEllipseRect;
                wxRect adjustedInnerRect = innerEllipseRect;

                adjustedOuterRect.Inflate(outerInflationOffset, outerInflationOffset);
                adjustedInnerRect.Inflate(innerInflationOffset, innerInflationOffset);

                outerRingPoints.push_back(GetEllipsePointFromRect(adjustedOuterRect, angleDegrees));
                innerRingPoints.push_back(GetEllipsePointFromRect(adjustedInnerRect, angleDegrees));
                }

            std::vector<wxPoint> crustPolygonPoints;
            crustPolygonPoints.reserve(outerRingPoints.size() + innerRingPoints.size());
            crustPolygonPoints.insert(crustPolygonPoints.end(), outerRingPoints.begin(),
                                      outerRingPoints.end());
            crustPolygonPoints.insert(crustPolygonPoints.end(), innerRingPoints.rbegin(),
                                      innerRingPoints.rend());

            const bool isToastedLayer = (crustLayerIndex >= 1);

            const wxBrush crustBrush =
                isToastedLayer ? wxBrush{ toastedColor } : wxBrush{ doughColor };
            const wxPen crustPen(wxColour(150, 110, 70, isToastedLayer ? 40 : 90),
                                 ScaleToScreenAndCanvas(1), wxPENSTYLE_SOLID);

            auto crustPolygon =
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                          .Brush(crustBrush)
                                                          .Pen(crustPen)
                                                          .Scaling(GetScaling())
                                                          .DPIScaling(GetDPIScaleFactor())
                                                          .Selectable(false),
                                                      crustPolygonPoints);

            AddObject(std::move(crustPolygon));
            }
        }

    //----------------------------------------------------------------
    void PieChart::AddClockHands(const DrawAreas& drawAreas)
        {
        if (m_outerPie.size() <= 1)
            {
            return;
            }

        // recompute slice angles from m_outerPie
        std::vector<std::pair<double, double>> sliceAngles;
        sliceAngles.reserve(m_outerPie.size());

        double cumulativeAngle{ 0.0 };
        for (const auto& sliceInfo : m_outerPie)
            {
            const double startAngle{ cumulativeAngle };
            const double endAngle{ cumulativeAngle + sliceInfo.m_percent * 360.0 };
            sliceAngles.emplace_back(startAngle, endAngle);
            cumulativeAngle = endAngle;
            }

        // pie center
        const double pieCenterX = drawAreas.m_pieDrawArea.GetLeft() +
                                  (drawAreas.m_pieDrawArea.GetWidth() * math_constants::half);
        const double pieCenterY = drawAreas.m_pieDrawArea.GetTop() +
                                  (drawAreas.m_pieDrawArea.GetHeight() * math_constants::half);

        const wxPoint pieCenterPoint{ static_cast<int>(pieCenterX), static_cast<int>(pieCenterY) };

        const auto computeBoundaryPoint = [&drawAreas](const double angleDegrees)
        {
            const auto pt = geometry::arc_vertex(
                { drawAreas.m_pieDrawArea.GetWidth(), drawAreas.m_pieDrawArea.GetHeight() },
                angleDegrees);

            return wxPoint{ static_cast<int>(pt.first + drawAreas.m_pieDrawArea.GetLeft()),
                            static_cast<int>(pt.second + drawAreas.m_pieDrawArea.GetTop()) };
        };

        const auto interpolate =
            [pieCenterX, pieCenterY](const wxPoint boundaryPoint, const double scalar)
        {
            const double dx{ boundaryPoint.x - pieCenterX };
            const double dy{ boundaryPoint.y - pieCenterY };

            return wxPoint{ static_cast<int>(pieCenterX + dx * scalar),
                            static_cast<int>(pieCenterY + dy * scalar) };
        };

        const auto extendBehind =
            [pieCenterX, pieCenterY](const wxPoint boundaryPoint, const double scalar)
        {
            const double dx{ boundaryPoint.x - pieCenterX };
            const double dy{ boundaryPoint.y - pieCenterY };

            return wxPoint{ static_cast<int>(pieCenterX - dx * scalar),
                            static_cast<int>(pieCenterY - dy * scalar) };
        };

        // highlight sheen (uses parallel translucent lines)
        const auto addHandHighlight =
            [this](const wxPoint& backPoint, const wxPoint& tipPoint, double highlightOffset)
        {
            const wxPoint2DDouble direction{ static_cast<double>(tipPoint.x - backPoint.x),
                                             static_cast<double>(tipPoint.y - backPoint.y) };

            const double magnitude =
                std::sqrt((direction.m_x * direction.m_x) + (direction.m_y * direction.m_y));
            if (magnitude < 1.0)
                {
                return;
                }

            const double nx{ direction.m_x / magnitude };
            const double ny{ direction.m_y / magnitude };

            // perpendicular (CW)
            const double px{ ny };
            const double py{ -nx };

            const std::array<int, 3> alphaLevels{ 120, 75, 30 };
            for (const auto alpha : alphaLevels)
                {
                const wxPoint offsetBack{ static_cast<int>(backPoint.x + px * highlightOffset),
                                          static_cast<int>(backPoint.y + py * highlightOffset) };
                const wxPoint offsetTip{ static_cast<int>(tipPoint.x + px * highlightOffset),
                                         static_cast<int>(tipPoint.y + py * highlightOffset) };

                auto line = std::make_unique<GraphItems::Lines>(
                    wxPen(wxColour(255, 255, 255, alpha), ScaleToScreenAndCanvas(1.5),
                          wxPENSTYLE_SOLID),
                    GetScaling());
                line->GetPen().SetCap(wxCAP_ROUND);
                line->AddLine(offsetBack, offsetTip);

                AddObject(std::move(line));

                highlightOffset *= 2.0;
                }
        };

        // tapered hand
        const auto makeTaperedHand =
            [this, &pieCenterPoint, &interpolate, &extendBehind](
                const wxColour& color, const wxPoint& boundaryPoint, const double lengthScalar,
                const double backScalar, const double tipWidth,
                const double baseWidth) -> std::unique_ptr<GraphItems::Polygon>
        {
            const wxPoint2DDouble direction{
                static_cast<double>(boundaryPoint.x - pieCenterPoint.x),
                static_cast<double>(boundaryPoint.y - pieCenterPoint.y)
            };

            const double magnitude =
                std::sqrt((direction.m_x * direction.m_x) + (direction.m_y * direction.m_y));
            if (magnitude < 1.0)
                {
                return nullptr;
                }

            const double nx{ direction.m_x / magnitude };
            const double ny{ direction.m_y / magnitude };

            const double px{ ny };
            const double py{ -nx };

            const wxPoint tip{ interpolate(boundaryPoint, lengthScalar) };
            const wxPoint base{ interpolate(boundaryPoint, 0.15) };
            const wxPoint back{ extendBehind(boundaryPoint, backScalar) };

            const double tipHalf{ tipWidth * math_constants::half };
            const double baseHalf{ baseWidth * math_constants::half };

            const std::array<wxPoint, 6> pts{ wxPoint{ static_cast<int>(tip.x + px * tipHalf),
                                                       static_cast<int>(tip.y + py * tipHalf) },
                                              wxPoint{ static_cast<int>(base.x + px * baseHalf),
                                                       static_cast<int>(base.y + py * baseHalf) },
                                              wxPoint{ static_cast<int>(back.x + px * baseHalf),
                                                       static_cast<int>(back.y + py * baseHalf) },
                                              wxPoint{ static_cast<int>(back.x - px * baseHalf),
                                                       static_cast<int>(back.y - py * baseHalf) },
                                              wxPoint{ static_cast<int>(base.x - px * baseHalf),
                                                       static_cast<int>(base.y - py * baseHalf) },
                                              wxPoint{ static_cast<int>(tip.x - px * tipHalf),
                                                       static_cast<int>(tip.y - py * tipHalf) } };

            auto poly = std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo()
                                                                  .Brush(color)
                                                                  .Pen(wxPen(color))
                                                                  .Scaling(GetScaling())
                                                                  .DPIScaling(GetDPIScaleFactor()),
                                                              pts);

            return poly;
        };

            // hour hand
            {
            const double angle{ sliceAngles.front().first };
            const wxPoint boundary{ computeBoundaryPoint(angle) };

            const wxPoint hourTip{ interpolate(boundary, 0.55) };
            const wxPoint hourBack{ extendBehind(boundary, 0.10) };

            auto hourHand = makeTaperedHand(*wxBLACK, boundary, 0.55, 0.10,
                                            ScaleToScreenAndCanvas(3), ScaleToScreenAndCanvas(6));

            if (hourHand)
                {
                AddObject(std::move(hourHand));
                }

            addHandHighlight(hourBack, hourTip, ScaleToScreenAndCanvas(0.6));
            }

            // minute hand
            {
            const double angle{ sliceAngles.front().second };
            const wxPoint boundary{ computeBoundaryPoint(angle) };

            const wxPoint minuteTip{ interpolate(boundary, 0.80) };
            const wxPoint minuteBack{ extendBehind(boundary, 0.10) };

            auto minuteHand = makeTaperedHand(*wxBLACK, boundary, 0.80, 0.10,
                                              ScaleToScreenAndCanvas(3), ScaleToScreenAndCanvas(8));

            if (minuteHand)
                {
                AddObject(std::move(minuteHand));
                }

            addHandHighlight(minuteBack, minuteTip, ScaleToScreenAndCanvas(0.6));
            }

        // seconds hand (if 3 or more slices)
        if (sliceAngles.size() >= 3)
            {
            const double angle{ sliceAngles[1].second };
            const wxPoint boundary{ computeBoundaryPoint(angle) };

            const wxPoint secondsTip{ interpolate(boundary, 0.92) };
            const wxPoint secondsBack{ extendBehind(boundary, 0.15) };

            auto secondsHand = makeTaperedHand(
                *wxRED, boundary, 0.92, 0.15, ScaleToScreenAndCanvas(2), ScaleToScreenAndCanvas(5));

            if (secondsHand)
                {
                AddObject(std::move(secondsHand));
                }

            addHandHighlight(secondsBack, secondsTip, ScaleToScreenAndCanvas(0.3));
            }

            // center hub
            {
            AddObject(std::make_unique<GraphItems::Point2D>(GraphItems::GraphItemInfo()
                                                                .AnchorPoint(pieCenterPoint)
                                                                .Brush(*wxBLACK_BRUSH)
                                                                .DPIScaling(GetDPIScaleFactor())
                                                                .Pen(*wxBLACK_PEN)
                                                                .Scaling(GetScaling()),
                                                            ScaleToScreenAndCanvas(6),
                                                            Icons::IconShape::Circle));
            AddObject(std::make_unique<GraphItems::Point2D>(GraphItems::GraphItemInfo()
                                                                .AnchorPoint(pieCenterPoint)
                                                                .DPIScaling(GetDPIScaleFactor())
                                                                .Brush(*wxWHITE_BRUSH)
                                                                .Pen(*wxWHITE_PEN)
                                                                .Scaling(GetScaling()),
                                                            ScaleToScreenAndCanvas(3),
                                                            Icons::IconShape::Circle));
            }
        }

    //----------------------------------------------------------------
    void PieChart::DrawInnerPie(wxDC& dc, GutterLabels& gutterLabels, DrawAreas drawAreas,
                                double& smallestOuterLabelFontSize,
                                std::vector<std::unique_ptr<GraphItemBase>>& addedObjects)
        {
        const auto queueObjectForOffsetting = [&addedObjects](auto obj)
        { addedObjects.push_back(std::move(obj)); };

        double smallestMiddleLabelFontSize{ GetBottomXAxis().GetFont().GetFractionalPointSize() };
        std::vector<std::unique_ptr<GraphItems::Label>> middleLabels;
        double startAngle{ 0.0 };
        size_t currentParentSliceIndex{ 0 };
        std::optional<wxColour> sliceColor{
            GetColorScheme() ? std::optional<wxColour>(GetColorScheme()->GetColor(0)) : std::nullopt
        };
        auto sliceBrush{ GetBrushScheme()->GetBrush(0) };

        const double sliceProportion =
            safe_divide<double>(1 - (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), 2) +
            (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
        auto innerDrawArea = drawAreas.m_pieDrawArea;
        innerDrawArea.width *= sliceProportion;
        innerDrawArea.height *= sliceProportion;
        innerDrawArea.Offset(wxPoint((drawAreas.m_pieDrawArea.width - innerDrawArea.width) / 2,
                                     (drawAreas.m_pieDrawArea.height - innerDrawArea.height) / 2));

        // how much (percentage) of the inner ring area the donut hole consumes
        const auto donutHoleInnerProportion = safe_divide<double>(
            (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), sliceProportion);

        // outline of inner slices' sides, which will be half as thick as the
        // outer ring's slice sides
        auto sliceOutlinePen{ GetPen() };
        sliceOutlinePen.SetWidth(
            std::max(1, (sliceOutlinePen.IsOk() ? sliceOutlinePen.GetWidth() : 2) / 2));

        // note that we do NOT clear outerLabels or its smallest font size,
        // both rings use these
        for (auto& innerPie : GetInnerPie())
            {
            std::optional<wxColour> sliceColorToUse{ sliceColor };
            if (sliceColor && GetColorScheme())
                {
                sliceColor = (currentParentSliceIndex == innerPie.m_parentSliceIndex) ?
                                 Colors::ColorContrast::ShadeOrTint(sliceColor.value(), .1) :
                                 Colors::ColorContrast::ShadeOrTint(
                                     GetColorScheme()->GetColor(innerPie.m_parentSliceIndex), 0.1);
                sliceColorToUse =
                    (innerPie.IsGhosted() ?
                         // inner slices should be twice as translucent as outer slices since
                         // the outer slices will slightly show through it
                         Colors::ColorContrast::ChangeOpacity(sliceColor.value(),
                                                              GetGhostOpacity() / 2) :
                         sliceColor);
                }
            sliceBrush = (currentParentSliceIndex == innerPie.m_parentSliceIndex) ?
                             sliceBrush :
                             GetBrushScheme()->GetBrush(innerPie.m_parentSliceIndex);
            sliceBrush.SetColour(
                (currentParentSliceIndex == innerPie.m_parentSliceIndex) ?
                    Colors::ColorContrast::ShadeOrTint(sliceBrush.GetColour(), 0.1) :
                    Colors::ColorContrast::ShadeOrTint(
                        GetBrushScheme()->GetBrush(innerPie.m_parentSliceIndex).GetColour(), 0.1));
            wxBrush sliceBrushToUse{ sliceBrush };
            sliceBrushToUse.SetColour(
                innerPie.IsGhosted() ?
                    // inner slices should be twice as translucent as outer slices since
                    // the outer slices will slightly show through it
                    Colors::ColorContrast::ChangeOpacity(sliceBrush.GetColour(),
                                                         GetGhostOpacity() / 2) :
                    sliceBrush.GetColour());

            if (GetPieStyle() == PieStyle::CheezePizza ||
                GetPieStyle() == PieStyle::PepperoniCheezePizza)
                {
                sliceBrushToUse.SetColour(GetCheeseColor());
                sliceOutlinePen.SetColour(*wxBLACK);
                }
            else if (GetPieStyle() == PieStyle::CoffeeRing)
                {
                sliceBrushToUse.SetColour(GetPlotOrCanvasColor());
                sliceOutlinePen.SetColour(*wxBLACK);
                }

            currentParentSliceIndex = innerPie.m_parentSliceIndex;

            auto pSlice = std::make_unique<GraphItems::PieSlice>(
                GraphItems::GraphItemInfo(innerPie.GetGroupLabel())
                    .Brush(sliceBrushToUse)
                    .BaseColor(sliceColorToUse)
                    .DPIScaling(GetDPIScaleFactor())
                    .Scaling(GetScaling())
                    .Pen(sliceOutlinePen),
                innerDrawArea, startAngle, startAngle + (innerPie.m_percent * 360),
                innerPie.m_value, innerPie.m_percent);
            pSlice->SetMidPointLabelDisplay(innerPie.GetMidPointLabelDisplay());
            pSlice->GetArcPen() = GetPen();
            if (!innerPie.GetDescription().empty())
                {
                pSlice->SetText(innerPie.GetGroupLabel() + L"\n" + innerPie.GetDescription());
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                // use the parent slice color for the header, font color for the body
                if (IsUsingColorLabels())
                    {
                    pSlice->GetHeaderInfo().FontColor(
                        GetBrushScheme()->GetBrush(innerPie.m_parentSliceIndex).GetColour());
                    pSlice->SetFontColor(
                        Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                    }
                else
                    {
                    pSlice->SetFontColor(
                        Colors::ColorContrast::ShadeOrTint(pSlice->GetFontColor(), 0.4));
                    }
                }
            else
                {
                if (IsUsingColorLabels())
                    {
                    // parent color if using color labels, the inner slice's color
                    // may be too washed out to be legible
                    pSlice->SetFontColor(
                        GetBrushScheme()->GetBrush(innerPie.m_parentSliceIndex).GetColour());
                    }
                pSlice->GetFont().MakeBold();
                }
            // Make inner slices transparent, so that its parent slice's image shows through.
            // Note that if the parent is ghosted and this inner slice is NOT ghosted
            // (i.e., it is being showcased), then keep as a solid color. Trying to have
            // part of an image translucent and other parts of it opaque will not be obvious
            // and would also be difficult to do technically.
            const bool parentIsGhosted = GetOuterPie().at(innerPie.m_parentSliceIndex).IsGhosted();
            if (GetPieSliceEffect() == PieSliceEffect::Image && GetImageScheme() &&
                (!parentIsGhosted || innerPie.IsGhosted()))
                {
                pSlice->GetBrush() = wxColour{ 0, 0, 0, 0 };
                }

            if (innerPie.m_showText)
                {
                CreateLabelAndConnectionLine(dc, gutterLabels, drawAreas, pSlice,
                                             smallestOuterLabelFontSize, true);
                }

            const auto labelDisplay =
                pSlice->GetMidPointLabelDisplay().value_or(GetInnerPieMidPointLabelDisplay());
            if (labelDisplay != BinLabelDisplay::NoDisplay)
                {
                auto middleLabel = pSlice->CreateMiddleLabel(
                    dc,
                    // take into account the hole consuming a larger % of the inner
                    // area compared to the full pie area
                    safe_divide(1.0 - donutHoleInnerProportion, 2.0) + donutHoleInnerProportion,
                    labelDisplay, m_abbreviate);
                if (middleLabel != nullptr)
                    {
                    middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                    smallestMiddleLabelFontSize =
                        std::min(smallestMiddleLabelFontSize,
                                 middleLabel->GetFont().GetFractionalPointSize());
                    if (innerPie.IsGhosted())
                        {
                        middleLabel->SetFontColor(Colors::ColorContrast::ChangeOpacity(
                            middleLabel->GetFontColor(), GetGhostOpacity()));
                        }
                    middleLabels.push_back(std::move(middleLabel));
                    }
                }

            queueObjectForOffsetting(std::move(pSlice));
            startAngle += innerPie.m_percent * 360;
            }

        // make the inner ring center labels have a common font size
        for (auto& middleLabel : middleLabels)
            {
            if (middleLabel != nullptr)
                {
                middleLabel->GetFont().SetFractionalPointSize(smallestMiddleLabelFontSize);
                queueObjectForOffsetting(std::move(middleLabel));
                }
            }
        }

    //----------------------------------------------------------------
    void PieChart::DrawOuterPie(wxDC& dc, GutterLabels& gutterLabels, DrawAreas drawAreas,
                                double& smallestOuterLabelFontSize,
                                std::vector<std::unique_ptr<GraphItemBase>>& addedObjects)
        {
        const auto queueObjectForOffsetting = [&addedObjects](auto obj)
        { addedObjects.push_back(std::move(obj)); };

        double smallestMiddleLabelFontSize{ GetBottomXAxis().GetFont().GetFractionalPointSize() };
        std::vector<std::unique_ptr<GraphItems::Label>> middleLabels;
        double startAngle{ 0.0 };
        wxPen sliceOutlinePen{ GetPen() };
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            const std::optional<wxColour> sliceColor =
                GetColorScheme() ? std::optional<wxColour>(
                                       GetOuterPie().at(i).IsGhosted() ?
                                           Colors::ColorContrast::ChangeOpacity(
                                               GetColorScheme()->GetColor(i), GetGhostOpacity()) :
                                           GetColorScheme()->GetColor(i)) :
                                   std::nullopt;
            wxBrush sliceBrush = GetBrushScheme()->GetBrush(i);
            sliceBrush.SetColour(
                GetOuterPie().at(i).IsGhosted() ?
                    Colors::ColorContrast::ChangeOpacity(GetBrushScheme()->GetBrush(i).GetColour(),
                                                         GetGhostOpacity()) :
                    GetBrushScheme()->GetBrush(i).GetColour());
            if (GetPieStyle() == PieStyle::CheezePizza ||
                GetPieStyle() == PieStyle::PepperoniCheezePizza)
                {
                sliceBrush.SetColour(GetCheeseColor());
                sliceOutlinePen.SetColour(*wxBLACK);
                }
            else if (GetPieStyle() == PieStyle::CoffeeRing)
                {
                sliceBrush.SetColour(GetPlotOrCanvasColor());
                sliceOutlinePen.SetColour(*wxBLACK);
                }
            auto pSlice = std::make_unique<GraphItems::PieSlice>(
                GraphItems::GraphItemInfo(GetOuterPie().at(i).GetGroupLabel())
                    .Brush(sliceBrush)
                    .BaseColor(sliceColor)
                    .DPIScaling(GetDPIScaleFactor())
                    .Scaling(GetScaling())
                    .Pen(sliceOutlinePen),
                drawAreas.m_pieDrawArea, startAngle,
                startAngle + (GetOuterPie().at(i).m_percent * 360), GetOuterPie().at(i).m_value,
                GetOuterPie().at(i).m_percent);
            pSlice->SetMidPointLabelDisplay(GetOuterPie().at(i).GetMidPointLabelDisplay());
            if (!GetOuterPie().at(i).GetDescription().empty())
                {
                pSlice->SetText(GetOuterPie().at(i).GetGroupLabel() + L"\n" +
                                GetOuterPie().at(i).GetDescription());
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                if (IsUsingColorLabels())
                    {
                    pSlice->GetHeaderInfo().FontColor(GetBrushScheme()->GetBrush(i).GetColour());
                    pSlice->SetFontColor(
                        Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                    }
                else
                    {
                    pSlice->SetFontColor(
                        Colors::ColorContrast::ShadeOrTint(pSlice->GetFontColor(), .4));
                    }
                }
            else
                {
                if (IsUsingColorLabels())
                    {
                    pSlice->SetFontColor(GetBrushScheme()->GetBrush(i).GetColour());
                    }
                pSlice->GetFont().MakeBold();
                }
            // if showing an image under the slice, then set its brush's stipple to that image
            if (GetPieSliceEffect() == PieSliceEffect::Image && GetImageScheme() &&
                GetImageScheme()->GetImage(i).IsOk())
                {
                const auto sliceBBox = pSlice->GetBoundingBox(dc);
                const auto& bmp = GetImageScheme()->GetImage(i);
                const auto bmpSize = geometry::downscaled_size(
                    std::make_pair(bmp.GetDefaultSize().GetWidth(),
                                   bmp.GetDefaultSize().GetHeight()),
                    std::make_pair(sliceBBox.GetWidth(), sliceBBox.GetHeight()));
                auto sliceBmp = bmp.GetBitmap(wxSize(bmpSize.first, bmpSize.second));
                if (GetOuterPie().at(i).IsGhosted())
                    {
                    GraphItems::Image::SetOpacity(sliceBmp, GetGhostOpacity(), false);
                    }
                wxASSERT_MSG(sliceBmp.IsOk(), L"Unable to create pie slice image!");
                if (sliceBmp.IsOk())
                    {
                    pSlice->GetBrush() = wxBrush{ sliceBmp };
                    }
                }

            if (GetOuterPie().at(i).m_showText)
                {
                CreateLabelAndConnectionLine(dc, gutterLabels, drawAreas, pSlice,
                                             smallestOuterLabelFontSize, false);
                }

            double sliceProportion = 1 - (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
            if (!GetInnerPie().empty())
                {
                sliceProportion /= 2;
                }

            sliceProportion = (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0) +
                              safe_divide<double>(sliceProportion, 2) +
                              (!GetInnerPie().empty() ? sliceProportion : 0);
            const auto labelDisplay =
                pSlice->GetMidPointLabelDisplay().value_or(GetOuterPieMidPointLabelDisplay());
            if (labelDisplay != BinLabelDisplay::NoDisplay)
                {
                auto middleLabel =
                    pSlice->CreateMiddleLabel(dc, sliceProportion, labelDisplay, m_abbreviate);
                if (middleLabel != nullptr)
                    {
                    middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                    smallestMiddleLabelFontSize =
                        std::min(smallestMiddleLabelFontSize,
                                 middleLabel->GetFont().GetFractionalPointSize());
                    if (GetOuterPie().at(i).IsGhosted())
                        {
                        middleLabel->SetFontColor(Colors::ColorContrast::ChangeOpacity(
                            middleLabel->GetFontColor(), GetGhostOpacity()));
                        }
                    middleLabels.push_back(std::move(middleLabel));
                    }
                }

            queueObjectForOffsetting(std::move(pSlice));
            startAngle += GetOuterPie().at(i).m_percent * 360;
            }
        // make the outer ring middle labels have a common font size
        for (auto& middleLabel : middleLabels)
            {
            if (middleLabel != nullptr)
                {
                middleLabel->GetFont().SetFractionalPointSize(smallestMiddleLabelFontSize);
                queueObjectForOffsetting(std::move(middleLabel));
                }
            }
        }

    //----------------------------------------------------------------
    void PieChart::CreateLabelAndConnectionLine(wxDC& dc, GutterLabels& gutterLabels,
                                                const DrawAreas drawAreas, auto& pSlice,
                                                double& smallestOuterLabelFontSize,
                                                bool isInnerSlice)

        {
        auto outerLabel = pSlice->CreateOuterLabel(
            (isInnerSlice ? drawAreas.m_outerPieDrawArea : drawAreas.m_pieDrawArea),
            GetOuterLabelDisplay());
        if (!IsUsingColorLabels())
            {
            outerLabel->SetFontColor(
                Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
            }
        outerLabel->SetDPIScaleFactor(GetDPIScaleFactor());
        if (outerLabel != nullptr)
            {
            // lambda to adjust label to fit in pie's gutters
            const auto measureAndFitLabel = [&dc, &drawAreas](auto& label)
            {
                const auto labelBox = label->GetBoundingBox(dc);
                if (!GraphItems::Polygon::IsRectInsideRect(labelBox, drawAreas.m_fullDrawArea))
                    {
                    const auto currentFontSize = label->GetFont().GetFractionalPointSize();
                    const auto& [widthInside, heightInside] =
                        GraphItems::Polygon::GetPercentInsideRect(labelBox,
                                                                  drawAreas.m_fullDrawArea);
                    const auto smallerScale = std::min(widthInside, heightInside);
                    label->GetFont().SetFractionalPointSize(currentFontSize * smallerScale);
                    return smallerScale;
                    }
                return 1.0;
            };

            // adjust label to fit
            const auto currentFontSize = outerLabel->GetFont().GetFractionalPointSize();
            const auto textScale = measureAndFitLabel(outerLabel);
            // ...but if it's a little too small and doesn't have a header,
            // then try to split it into multiple lines and resize it again.
            // Note that we don't do this if it has a header because the header
            // implies that the first line break is meaningful, so we can't
            // arbitrarily split this text up.
            if (compare_doubles_less(textScale, math_constants::three_quarters) &&
                !outerLabel->GetHeaderInfo().IsEnabled())
                {
                outerLabel->GetFont().SetFractionalPointSize(currentFontSize);
                // try to auto split if we aren't appending something in parentheses;
                // otherwise, split into three lines
                if ((GetOuterLabelDisplay() == BinLabelDisplay::BinNameAndPercentage ||
                     GetOuterLabelDisplay() == BinLabelDisplay::BinNameAndValue ||
                     GetOuterLabelDisplay() == BinLabelDisplay::BinValueAndPercentage) ||
                    !outerLabel->SplitTextAuto())
                    {
                    outerLabel->SplitTextToFitLength(outerLabel->GetText().length() *
                                                     math_constants::third);
                    }
                measureAndFitLabel(outerLabel);
                }

            smallestOuterLabelFontSize = std::min(smallestOuterLabelFontSize,
                                                  outerLabel->GetFont().GetFractionalPointSize());

            std::unique_ptr<GraphItems::Points2D> connectionLine{ nullptr };
            const bool isTopLeft = outerLabel->GetAnchoring() == Anchoring::BottomRightCorner;
            const bool isBottomLeft = outerLabel->GetAnchoring() == Anchoring::TopRightCorner;
            const bool isLeft = (isTopLeft || isBottomLeft);
            const bool isTopRight = outerLabel->GetAnchoring() == Anchoring::BottomLeftCorner;
            const bool isBottomRight = outerLabel->GetAnchoring() == Anchoring::TopLeftCorner;
            if (isInnerSlice)
                {
                // a line connecting the inner slice to its outside label
                auto arcMiddle = pSlice->GetMiddleOfArc(1);
                connectionLine =
                    std::make_unique<GraphItems::Points2D>(GetInnerPieConnectionLinePen());
                connectionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                connectionLine->SetScaling(GetScaling());
                connectionLine->SetSelectable(false);
                connectionLine->AddPoint(
                    GraphItems::Point2D(GraphItems::GraphItemInfo()
                                            .AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second))
                                            .Show(false),
                                        0),
                    dc);
                connectionLine->AddPoint(
                    GraphItems::Point2D(GraphItems::GraphItemInfo()
                                            .AnchorPoint(outerLabel->GetAnchorPoint())
                                            .Show(false),
                                        0),
                    dc);
                if (GetLabelPlacement() == LabelPlacement::Flush)
                    {
                    connectionLine->AddPoint(
                        GraphItems::Point2D(
                            GraphItems::GraphItemInfo()
                                .AnchorPoint(wxPoint(isLeft ? drawAreas.m_fullDrawArea.GetLeft() :
                                                              drawAreas.m_fullDrawArea.GetRight(),
                                                     outerLabel->GetAnchorPoint().y))
                                .Show(false),
                            0),
                        dc);
                    // force using lines (instead of arrows) since this will be two lines
                    connectionLine->SetLineStyle(LineStyle::Lines);
                    }
                else
                    {
                    connectionLine->SetLineStyle(GetInnerPieConnectionLineStyle());
                    }
                }
            else
                {
                // a line connecting the outer slice to its outside label
                // (only if pushed over to the side)
                if (GetLabelPlacement() == LabelPlacement::Flush)
                    {
                    auto arcMiddle = pSlice->GetMiddleOfArc(1);
                    connectionLine =
                        std::make_unique<GraphItems::Points2D>(GetInnerPieConnectionLinePen());
                    connectionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                    connectionLine->SetScaling(GetScaling());
                    connectionLine->SetSelectable(false);
                    connectionLine->AddPoint(
                        GraphItems::Point2D(
                            GraphItems::GraphItemInfo()
                                .AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second))
                                .Show(false),
                            0),
                        dc);
                    connectionLine->AddPoint(
                        GraphItems::Point2D(GraphItems::GraphItemInfo()
                                                .AnchorPoint(outerLabel->GetAnchorPoint())
                                                .Show(false),
                                            0),
                        dc);
                    connectionLine->AddPoint(
                        GraphItems::Point2D(
                            GraphItems::GraphItemInfo()
                                .AnchorPoint(wxPoint(isLeft ? drawAreas.m_fullDrawArea.GetLeft() :
                                                              drawAreas.m_fullDrawArea.GetRight(),
                                                     outerLabel->GetAnchorPoint().y))
                                .Show(false),
                            0),
                        dc);
                    connectionLine->SetLineStyle(LineStyle::Lines);
                    }
                }
            if (isTopLeft)
                {
                gutterLabels.m_outerTopLeftLabelAndLines.push_back(
                    std::make_pair(std::move(outerLabel), std::move(connectionLine)));
                }
            else if (isBottomLeft)
                {
                gutterLabels.m_outerBottomLeftLabelAndLines.push_back(
                    std::make_pair(std::move(outerLabel), std::move(connectionLine)));
                }
            else if (isTopRight)
                {
                gutterLabels.m_outerTopRightLabelAndLines.push_back(
                    std::make_pair(std::move(outerLabel), std::move(connectionLine)));
                }
            else if (isBottomRight)
                {
                gutterLabels.m_outerBottomRightLabelAndLines.push_back(
                    std::make_pair(std::move(outerLabel), std::move(connectionLine)));
                }
            }
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestOuterPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetOuterPie().empty())
            {
            return pieLabels;
            }

        // find largest percentage
        const auto& maxPie = *std::ranges::max_element(
            GetOuterPie(), [](const auto& lhv, const auto& rhv) noexcept
            { return compare_doubles_less(lhv.m_percent, rhv.m_percent); });

        // in case of ties, grab all pie slices with same percentage as the largest one
        std::ranges::for_each(GetOuterPie(),
                              [&](const auto& slice) noexcept
                              {
                                  if (compare_doubles(slice.m_percent, maxPie.m_percent))
                                      {
                                      pieLabels.push_back(slice.m_groupLabel);
                                      }
                              });

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestOuterPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetOuterPie().empty())
            {
            return pieLabels;
            }

        // find smallest percentage
        const auto& minPie = *std::ranges::min_element(
            GetOuterPie(), [](const auto& lhv, const auto& rhv) noexcept
            { return compare_doubles_less(lhv.m_percent, rhv.m_percent); });

        // in case of ties, grab all pie slices with same percentage as the smallest one
        std::ranges::for_each(GetOuterPie(),
                              [&](const auto& slice) noexcept
                              {
                                  if (compare_doubles(slice.m_percent, minPie.m_percent))
                                      {
                                      pieLabels.push_back(slice.m_groupLabel);
                                      }
                              });

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestInnerPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetInnerPie().empty())
            {
            return pieLabels;
            }

        // find largest percentage
        const auto& maxPie = *std::ranges::max_element(
            GetInnerPie(), [](const auto& lhv, const auto& rhv) noexcept
            { return compare_doubles_less(lhv.m_percent, rhv.m_percent); });

        // in case of ties, grab all pie slices with same percentage as the largest one
        std::ranges::for_each(GetInnerPie(),
                              [&](const auto& slice) noexcept
                              {
                                  if (compare_doubles(slice.m_percent, maxPie.m_percent))
                                      {
                                      pieLabels.push_back(slice.m_groupLabel);
                                      }
                              });

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestInnerPieSlicesByGroup() const
        {
        std::vector<wxString> pieLabels;
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            // get the inner slices within the current parent slice
            std::vector<const SliceInfo*> innerSlicesForCurrentGroup;
            std::ranges::for_each(std::as_const(GetInnerPie()),
                                  [&](const auto& slice)
                                  {
                                      if (slice.m_parentSliceIndex == i)
                                          {
                                          innerSlicesForCurrentGroup.push_back(&slice);
                                          }
                                  });
            if (innerSlicesForCurrentGroup.empty())
                {
                continue;
                }
            // find the largest percentage within the subgroup of slices
            const auto* const maxPie = *std::ranges::max_element(
                std::as_const(innerSlicesForCurrentGroup),
                [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv->m_percent, rhv->m_percent); });

            // in case of ties, grab all pie slices with same percentage as the largest one
            std::ranges::for_each(std::as_const(innerSlicesForCurrentGroup),
                                  [&](const auto& slice) noexcept
                                  {
                                      if (compare_doubles(slice->m_percent, maxPie->m_percent))
                                          {
                                          pieLabels.push_back(slice->m_groupLabel);
                                          }
                                  });
            }

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestInnerPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetInnerPie().empty())
            {
            return pieLabels;
            }

        // find smallest percentage
        const auto& minPie = *std::ranges::min_element(
            GetInnerPie(), [](const auto& lhv, const auto& rhv) noexcept
            { return compare_doubles_less(lhv.m_percent, rhv.m_percent); });

        // in case of ties, grab all pie slices with same percentage as the smallest one
        std::ranges::for_each(std::as_const(GetInnerPie()),
                              [&](const auto& slice) noexcept
                              {
                                  if (compare_doubles(slice.m_percent, minPie.m_percent))
                                      {
                                      pieLabels.push_back(slice.m_groupLabel);
                                      }
                              });

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestInnerPieSlicesByGroup() const
        {
        std::vector<wxString> pieLabels;
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            // get the inner slices within the current parent slice
            std::vector<const SliceInfo*> innerSlicesForCurrentGroup;
            std::ranges::for_each(std::as_const(GetInnerPie()),
                                  [&](const auto& slice)
                                  {
                                      if (slice.m_parentSliceIndex == i)
                                          {
                                          innerSlicesForCurrentGroup.push_back(&slice);
                                          }
                                  });
            if (innerSlicesForCurrentGroup.empty())
                {
                continue;
                }
            // find the smallest percentage within the subgroup of slices
            const auto* const minPie = *std::ranges::min_element(
                std::as_const(innerSlicesForCurrentGroup),
                [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv->m_percent, rhv->m_percent); });

            // in case of ties, grab all pie slices with same percentage as the smallest one
            std::ranges::for_each(std::as_const(innerSlicesForCurrentGroup),
                                  [&](const auto& slice) noexcept
                                  {
                                      if (compare_doubles(slice->m_percent, minPie->m_percent))
                                          {
                                          pieLabels.push_back(slice->m_groupLabel);
                                          }
                                  });
            }

        return pieLabels;
        }

    //----------------------------------------------------------------
    void PieChart::GhostOuterPieSlices(const bool ghost)
        {
        std::ranges::for_each(GetOuterPie(), [&](auto& slice) noexcept { slice.Ghost(ghost); });
        }

    //----------------------------------------------------------------
    void PieChart::GhostOuterPieSlices(const bool ghost, const std::vector<wxString>& slicesToGhost)
        {
        std::ranges::for_each(
            GetOuterPie(),
            [&](auto& slice) noexcept
            {
                const bool inList = (std::find_if(slicesToGhost.cbegin(), slicesToGhost.cend(),
                                                  [&slice](const auto& label)
                                                  {
                                                      return Data::CmpNoCaseIgnoreControlChars(
                                                                 label, slice.GetGroupLabel()) == 0;
                                                  }) != slicesToGhost.cend());
                slice.Ghost(inList ? ghost : !ghost);
            });
        }

    //----------------------------------------------------------------
    void PieChart::GhostInnerPieSlices(const bool ghost)
        {
        std::ranges::for_each(GetInnerPie(), [&](auto& slice) noexcept { slice.Ghost(ghost); });
        }

    //----------------------------------------------------------------
    void PieChart::GhostInnerPieSlices(const bool ghost, const std::vector<wxString>& slicesToGhost)
        {
        std::ranges::for_each(
            GetInnerPie(),
            [&](auto& slice) noexcept
            {
                const bool inList = (std::find_if(slicesToGhost.cbegin(), slicesToGhost.cend(),
                                                  [&slice](const auto& label)
                                                  {
                                                      return Data::CmpNoCaseIgnoreControlChars(
                                                                 label, slice.GetGroupLabel()) == 0;
                                                  }) != slicesToGhost.cend());
                slice.Ghost(inList ? ghost : !ghost);
            });
        }

    //----------------------------------------------------------------
    std::set<size_t> PieChart::GetOuterPieIndices(const std::vector<wxString>& labels)
        {
        std::set<size_t> indices;
        for (const auto& label : labels)
            {
            for (size_t i = 0; i < GetOuterPie().size(); ++i)
                {
                if (Data::CmpNoCaseIgnoreControlChars(GetOuterPie()[i].GetGroupLabel(), label) == 0)
                    {
                    indices.insert(i);
                    break;
                    }
                }
            }
        return indices;
        }

    //----------------------------------------------------------------
    void PieChart::ShowcaseLargestOuterPieSlices(
        const Perimeter outerLabelRingToShow /*= Perimeter::Outer*/)
        {
        const std::vector<wxString> highlightSlices = GetLargestOuterPieSlices();
        if (outerLabelRingToShow == Perimeter::Outer)
            {
            ShowOuterPieLabels(true, highlightSlices);
            }
        else
            {
            ShowOuterPieLabels(false);
            }
        ShowOuterPieMidPointLabels(true, highlightSlices);
        GhostOuterPieSlices(false, highlightSlices);

        // do the same for the inner slices
        const auto showcasedOuterIndices = GetOuterPieIndices(highlightSlices);
        std::vector<wxString> innerLabelsForGroups;
        for (const auto& innerSlice : GetInnerPie())
            {
            if (showcasedOuterIndices.contains(innerSlice.m_parentSliceIndex))
                {
                innerLabelsForGroups.push_back(innerSlice.GetGroupLabel());
                }
            }

        if (outerLabelRingToShow == Perimeter::Inner)
            {
            ShowInnerPieLabels(true, innerLabelsForGroups);
            }
        else
            {
            ShowInnerPieLabels(false);
            }
        ShowInnerPieMidPointLabels(true, innerLabelsForGroups);
        GhostInnerPieSlices(false, innerLabelsForGroups);
        }

    //----------------------------------------------------------------
    void PieChart::ShowcaseSmallestOuterPieSlices(
        const Perimeter outerLabelRingToShow /*= Perimeter::Outer*/)
        {
        const std::vector<wxString> highlightSlices = GetSmallestOuterPieSlices();
        if (outerLabelRingToShow == Perimeter::Outer)
            {
            ShowOuterPieLabels(true, highlightSlices);
            }
        else
            {
            ShowOuterPieLabels(false);
            }
        ShowOuterPieMidPointLabels(true, highlightSlices);
        GhostOuterPieSlices(false, highlightSlices);

        // do the same for the inner slices
        const auto showcasedOuterIndices = GetOuterPieIndices(highlightSlices);
        std::vector<wxString> innerLabelsForGroups;
        for (const auto& innerSlice : GetInnerPie())
            {
            if (showcasedOuterIndices.contains(innerSlice.m_parentSliceIndex))
                {
                innerLabelsForGroups.push_back(innerSlice.GetGroupLabel());
                }
            }

        if (outerLabelRingToShow == Perimeter::Inner)
            {
            ShowInnerPieLabels(true, innerLabelsForGroups);
            }
        else
            {
            ShowInnerPieLabels(false);
            }
        ShowInnerPieMidPointLabels(true, innerLabelsForGroups);
        GhostInnerPieSlices(false, innerLabelsForGroups);
        }

    //----------------------------------------------------------------
    void
    PieChart::ShowcaseOuterPieSlices(const std::vector<wxString>& pieSlices,
                                     const Perimeter outerLabelRingToShow /*= Perimeter::Outer*/)
        {
        if (outerLabelRingToShow == Perimeter::Outer)
            {
            ShowOuterPieLabels(true, pieSlices);
            }
        else
            {
            ShowOuterPieLabels(false);
            }
        ShowOuterPieMidPointLabels(true, pieSlices);
        GhostOuterPieSlices(false, pieSlices);

        // get positions of outer slices being showcased
        std::set<size_t> showcasedOuterIndices;
        for (const auto& pieSliceLabel : pieSlices)
            {
            const auto foundSlice =
                std::ranges::find_if(std::as_const(GetOuterPie()),
                                     [&pieSliceLabel](const auto& slice)
                                     {
                                         return (Data::CmpNoCaseIgnoreControlChars(
                                                     slice.GetGroupLabel(), pieSliceLabel) == 0);
                                     });
            if (foundSlice != GetOuterPie().cend())
                {
                showcasedOuterIndices.insert(std::distance(GetOuterPie().cbegin(), foundSlice));
                }
            }

        std::vector<wxString> innerLabelsForGroups;
        for (const auto& innerSlice : GetInnerPie())
            {
            if (showcasedOuterIndices.contains(innerSlice.m_parentSliceIndex))
                {
                innerLabelsForGroups.push_back(innerSlice.GetGroupLabel());
                }
            }

        if (outerLabelRingToShow == Perimeter::Inner)
            {
            ShowInnerPieLabels(true, innerLabelsForGroups);
            }
        else
            {
            ShowInnerPieLabels(false);
            }
        ShowInnerPieMidPointLabels(true, innerLabelsForGroups);
        GhostInnerPieSlices(false, innerLabelsForGroups);
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieLabels(const bool show)
        {
        std::ranges::for_each(GetOuterPie(),
                              [&](auto& slice) noexcept { slice.ShowGroupLabel(show); });
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::ranges::for_each(
            GetOuterPie(),
            [&](auto& slice) noexcept
            {
                const bool inList =
                    (std::find_if(labelsToShow.cbegin(), labelsToShow.cend(),
                                  [&slice](const auto& label)
                                  { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) !=
                     labelsToShow.cend());
                slice.ShowGroupLabel(inList ? show : !show);
            });
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieMidPointLabels(const bool show)
        {
        std::ranges::for_each(
            GetOuterPie(),
            [&show, this](auto& slice) noexcept
            {
                slice.SetMidPointLabelDisplay(
                    show ? std::optional<BinLabelDisplay>(GetOuterPieMidPointLabelDisplay()) :
                           std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
            });
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieMidPointLabels(const bool show,
                                              const std::vector<wxString>& labelsToShow)
        {
        std::ranges::for_each(
            GetOuterPie(),
            [&show, &labelsToShow, this](auto& slice) noexcept
            {
                const bool inList =
                    (std::find_if(labelsToShow.cbegin(), labelsToShow.cend(),
                                  [&slice](const auto& label)
                                  { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) !=
                     labelsToShow.cend());
                if (inList)
                    {
                    slice.SetMidPointLabelDisplay(
                        show ? std::optional<BinLabelDisplay>(GetOuterPieMidPointLabelDisplay()) :
                               std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
                    }
                // do the opposite for labels not in the user-provided list
                else
                    {
                    slice.SetMidPointLabelDisplay(
                        show ? std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay) :
                               std::optional<BinLabelDisplay>(GetOuterPieMidPointLabelDisplay()));
                    }
            });
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieLabels(const bool show)
        {
        std::ranges::for_each(GetInnerPie(),
                              [&show](auto& slice) noexcept { slice.ShowGroupLabel(show); });
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::ranges::for_each(
            GetInnerPie(),
            [&show, &labelsToShow](auto& slice) noexcept
            {
                const bool inList =
                    (std::find_if(labelsToShow.cbegin(), labelsToShow.cend(),
                                  [&slice](const auto& label)
                                  { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) !=
                     labelsToShow.cend());
                slice.ShowGroupLabel(inList ? show : !show);
            });
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieMidPointLabels(const bool show)
        {
        std::ranges::for_each(
            GetInnerPie(),
            [&show, this](auto& slice) noexcept
            {
                slice.SetMidPointLabelDisplay(
                    show ? std::optional<BinLabelDisplay>(GetInnerPieMidPointLabelDisplay()) :
                           std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
            });
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieMidPointLabels(const bool show,
                                              const std::vector<wxString>& labelsToShow)
        {
        std::ranges::for_each(
            GetInnerPie(),
            [&show, &labelsToShow, this](auto& slice) noexcept
            {
                const bool inList =
                    (std::find_if(labelsToShow.cbegin(), labelsToShow.cend(),
                                  [&slice](const auto& label)
                                  { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) !=
                     labelsToShow.cend());
                if (inList)
                    {
                    slice.SetMidPointLabelDisplay(
                        show ? std::optional<BinLabelDisplay>(GetInnerPieMidPointLabelDisplay()) :
                               std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
                    }
                // do the opposite for labels not in the user-provided list
                else
                    {
                    slice.SetMidPointLabelDisplay(
                        show ? std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay) :
                               std::optional<BinLabelDisplay>(GetInnerPieMidPointLabelDisplay()));
                    }
            });
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label>
    PieChart::CreateInnerPieLegend(const LegendCanvasPlacementHint hint)
        {
        assert(GetInnerPie().size() > 1 && L"Inner ring of pie chart empty, cannot create legend!");
        if (GetInnerPie().empty())
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo()
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        size_t currentLine{ 0 };

        // space in line is needed for SVG exporting; otherwise, the blank line gets removed
        wxString legendText{ GetOuterPie().at(0).GetGroupLabel() + L"\n \n" };
        legend->GetLinesIgnoringLeftMargin().insert(currentLine);
        currentLine += 2;
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::HorizontalLine,
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
            GetBrushScheme()->GetBrush(0),
            GetColorScheme() ? std::optional<wxColour>(GetColorScheme()->GetColor(0)) :
                               std::nullopt);
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::HorizontalSeparator,
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));

        size_t currentParentSliceIndex{ 0 };
        std::optional<wxColour> sliceColor{
            GetColorScheme() ? std::optional<wxColour>(GetColorScheme()->GetColor(0)) : std::nullopt
        };
        auto sliceBrush{ GetBrushScheme()->GetBrush(0) };
        for (size_t i = 0; i < GetInnerPie().size(); ++i)
            {
            if (Settings::GetMaxLegendItemCount() == i)
                {
                legendText.append(L"\u2026");
                // cppcheck-suppress unreadVariable
                ++currentLine;
                break;
                }
            wxString currentLabel = GetInnerPie().at(i).GetGroupLabel();
            assert(Settings::GetMaxLegendTextLength() >= 1 && L"Max legend text length is zero?!");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
                Settings::GetMaxLegendTextLength() >= 1)
                {
                currentLabel.erase(Settings::GetMaxLegendTextLength() - 1);
                currentLabel.append(L"\u2026");
                ++currentLine;
                }

            // get the color
            // slightly adjusted color based on the parent slice color
            if (sliceColor && GetColorScheme())
                {
                sliceColor =
                    (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceIndex) ?
                        Colors::ColorContrast::ShadeOrTint(sliceColor.value(), .1) :
                        Colors::ColorContrast::ShadeOrTint(
                            GetColorScheme()->GetColor(GetInnerPie().at(i).m_parentSliceIndex), .1);
                }
            sliceBrush = (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceIndex) ?
                             sliceBrush :
                             GetBrushScheme()->GetBrush(GetInnerPie().at(i).m_parentSliceIndex);
            sliceBrush.SetColour(
                (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceIndex) ?
                    Colors::ColorContrast::ShadeOrTint(sliceBrush.GetColour(), .1) :
                    Colors::ColorContrast::ShadeOrTint(
                        GetBrushScheme()
                            ->GetBrush(GetInnerPie().at(i).m_parentSliceIndex)
                            .GetColour(),
                        .1));
            // starting a new group
            if (currentParentSliceIndex != GetInnerPie().at(i).m_parentSliceIndex)
                {
                currentParentSliceIndex = GetInnerPie().at(i).m_parentSliceIndex;
                legendText.append(GetOuterPie().at(currentParentSliceIndex).GetGroupLabel())
                    .append(L"\n \n");
                legend->GetLinesIgnoringLeftMargin().insert(currentLine);
                currentLine += 2;
                legend->GetLegendIcons().emplace_back(
                    Icons::IconShape::HorizontalLine,
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                legend->GetLegendIcons().emplace_back(
                    Icons::IconShape::HorizontalSeparator,
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                    Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
                }

            // add icon and text (after group separator, if needed)
            legendText.append(currentLabel.c_str()).append(L"\n");
            ++currentLine;
            legend->GetLegendIcons().emplace_back(
                Icons::IconShape::TriangleRight,
                Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), sliceBrush,
                sliceColor);
            }
        legend->SetText(legendText.Trim());
        // show lines to make sure text is aligned as expected
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            legend->SetLabelStyle(LabelStyle::LinedPaper);
            }

        AdjustLegendSettings(*legend, hint);
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label>
    PieChart::CreateOuterPieLegend(const LegendCanvasPlacementHint hint)
        {
        assert(GetOuterPie().size() > 1 && L"Outer ring of pie chart empty, cannot create legend!");
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo()
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        wxString legendText;
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            if (Settings::GetMaxLegendItemCount() == i)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = GetOuterPie().at(i).GetGroupLabel();
            assert(Settings::GetMaxLegendTextLength() >= 1 && L"Max legend text length is zero?!");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
                Settings::GetMaxLegendTextLength() >= 1)
                {
                currentLabel.erase(Settings::GetMaxLegendTextLength() - 1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            legend->GetLegendIcons().emplace_back(
                Icons::IconShape::TriangleRight,
                Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                GetBrushScheme()->GetBrush(i),
                GetColorScheme() ? std::optional<wxColour>(GetColorScheme()->GetColor(i)) :
                                   std::nullopt);
            }
        legend->SetText(legendText.Trim());

        AdjustLegendSettings(*legend, hint);
        return legend;
        }
    } // namespace Wisteria::Graphs
