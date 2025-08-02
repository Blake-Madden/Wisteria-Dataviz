///////////////////////////////////////////////////////////////////////////////
// Name:        sankeydiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sankeydiagram.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::SankeyDiagram, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    SankeyDiagram::SankeyDiagram(
        Canvas * canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetBrushScheme(brushes != nullptr ? brushes :
                                            std::make_shared<Brushes::Schemes::BrushScheme>(
                                                Wisteria::Colors::Schemes::IceCream{}));

        GetLeftYAxis().SetRange(0, 100, 0, 1, 10);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().GetAxisLinePen() = wxNullPen;

        GetRightYAxis().SetRange(0, 100, 0, 1, 10);
        GetRightYAxis().GetGridlinePen() = wxNullPen;
        GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetRightYAxis().GetAxisLinePen() = wxNullPen;

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        }

    //----------------------------------------------------------------
    void SankeyDiagram::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                const wxString& fromColumnName, const wxString& toColumnName,
                                const std::optional<wxString>& fromWeightColumnName,
                                const std::optional<wxString>& toWeightColumnName,
                                const std::optional<wxString>& fromSortColumnName)
        {
        if (data == nullptr)
            {
            return;
            }

        GetSelectedIds().clear();
        m_sankeyColumns.clear();

        const auto fromColumn = data->GetCategoricalColumn(fromColumnName);
        if (fromColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': 'from' column not found for diagram."), fromColumnName)
                    .ToUTF8());
            }
        const auto toColumn = data->GetCategoricalColumn(toColumnName);
        if (toColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': 'to' column not found for diagram."), toColumnName)
                    .ToUTF8());
            }

        // weight columns
        if (fromWeightColumnName.has_value() && !toWeightColumnName.has_value())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"'%s': 'from' weight column provided, but 'to' column was not."),
                    fromWeightColumnName.value())
                    .ToUTF8());
            }
        if (toWeightColumnName.has_value() && !fromWeightColumnName.has_value())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"'%s': 'to' weight column provided, but 'from' column was not."),
                    toWeightColumnName.value())
                    .ToUTF8());
            }
        const bool useWeightColumn =
            fromWeightColumnName.has_value() && toWeightColumnName.has_value();
        auto fromWeightColumn = data->GetContinuousColumns().cend();
        auto toWeightColumn = data->GetContinuousColumns().cend();
        if (useWeightColumn)
            {
            fromWeightColumn = data->GetContinuousColumn(fromWeightColumnName.value());
            if (fromWeightColumn == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': 'from' weight column not found for diagram."),
                                     fromWeightColumnName.value())
                        .ToUTF8());
                }
            toWeightColumn = data->GetContinuousColumn(toWeightColumnName.value());
            if (toWeightColumn == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': 'to' weight column not found for diagram."),
                                     toWeightColumnName.value())
                        .ToUTF8());
                }
            }

        // grouping column
        auto fromGroupColumn = data->GetCategoricalColumns().cend();
        if (fromSortColumnName.has_value())
            {
            fromGroupColumn = data->GetCategoricalColumn(fromSortColumnName.value());
            if (fromGroupColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': 'from' sort column not found for diagram."),
                                     fromSortColumnName.value())
                        .ToUTF8());
                }
            }

        m_columnsNames = { fromColumnName, toColumnName };

        // load the combinations of labels (and weights and groups)
        multi_value_frequency_double_aggregate_map<wxString, wxString, Data::wxStringLessNoCase,
                                                   Data::wxStringLessNoCase>
            fromAndToMap;
        multi_value_aggregate_map<wxString, wxString, Data::wxStringLessNoCase,
                                  Data::wxStringLessNoCase>
            fromGrouping;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useWeightColumn && (std::isnan(fromWeightColumn->GetValue(i)) ||
                                    std::isnan(toWeightColumn->GetValue(i))))
                {
                continue;
                }

            fromAndToMap.insert(fromColumn->GetValueAsLabel(i), toColumn->GetValueAsLabel(i),
                                (useWeightColumn ? fromWeightColumn->GetValue(i) : 1),
                                (useWeightColumn ? toWeightColumn->GetValue(i) : 1));

            if (fromSortColumnName.has_value())
                {
                fromGrouping.insert(fromGroupColumn->GetValueAsLabel(i),
                                    fromColumn->GetValueAsLabel(i));
                }
            }

        m_sankeyColumns.resize(2);
        for (const auto& [key, subValues] : fromAndToMap.get_data())
            {
            m_sankeyColumns[0].emplace_back(key, subValues.second, subValues.first);

            // add "from" values to second column
            for (const auto& subVal : subValues.first.get_data())
                {
                auto subColGroupPos = std::ranges::find(
                    m_sankeyColumns[1], SankeyGroup{ subVal.first, subVal.second.second,
                                                     SankeyGroup::DownStreamGroups{} });
                if (subColGroupPos != m_sankeyColumns[1].end())
                    {
                    subColGroupPos->m_frequency += subVal.second.second;
                    }
                else
                    {
                    m_sankeyColumns[1].emplace_back(subVal.first, subVal.second.second,
                                                    SankeyGroup::DownStreamGroups{});
                    }
                }
            }

        if (!fromGrouping.get_data().empty())
            {
            SankeyColumn tempColumn;
            size_t groupOffset{ 0 };
            for (const auto& groups : fromGrouping.get_data())
                {
                assert(!groups.second.first.empty() && L"No groups in column group info!");
                m_fromAxisGroups.push_back(SankeyAxisGroup{
                    groups.first, groupOffset, groupOffset + (groups.second.first.size() - 1) });
                groupOffset += groups.second.first.size();
                for (const auto& gr : groups.second.first)
                    {
                    const auto subGroupPos =
                        std::ranges::find(std::as_const(m_sankeyColumns[0]), SankeyGroup{ gr });
                    assert(subGroupPos != m_sankeyColumns[0].cend() &&
                           L"Unable to find group in Sankey column!");
                    if (subGroupPos != m_sankeyColumns[0].cend())
                        {
                        tempColumn.push_back(*subGroupPos);
                        }
                    }
                }
            m_sankeyColumns[0] = tempColumn;
            }

        AdjustColumns();
        }

    //----------------------------------------------------------------
    void SankeyDiagram::AdjustColumns()
        {
        // if 'from' and 'to' are weighted, then ensure all columns have the same
        // total value, adding an empty group to a column is necessary
        double maxGroupTotal{ 0 };
        for (const auto& col : m_sankeyColumns)
            {
            const auto groupTotal = std::accumulate(col.cbegin(), col.cend(), 0.0,
                                                    [](const auto initVal, const auto& val) noexcept
                                                    { return val.m_frequency + initVal; });
            maxGroupTotal = std::max(maxGroupTotal, groupTotal);
            }

        for (auto& col : m_sankeyColumns)
            {
            const auto groupTotal = std::accumulate(col.cbegin(), col.cend(), 0.0,
                                                    [](const auto initVal, const auto& val) noexcept
                                                    { return val.m_frequency + initVal; });
            // Column lost some observations for the upstream groups, so add
            // an empty group on top of it and flag it as not having any upstream
            // groups flowing into it. (That will cause it to not be drawn.)
            if (groupTotal < maxGroupTotal)
                {
                col.insert(col.begin(), { wxString{}, (maxGroupTotal - groupTotal),
                                          SankeyGroup::DownStreamGroups{} })
                    ->m_isShown = false;
                }
            }

        // set how much each group consumes of its respective column
        for (auto& col : m_sankeyColumns)
            {
            const auto groupTotal = std::accumulate(col.cbegin(), col.cend(), 0.0,
                                                    [](const auto initVal, const auto& val) noexcept
                                                    { return val.m_frequency + initVal; });
            for (auto& group : col)
                {
                group.m_percentOfColumn = safe_divide(group.m_frequency, groupTotal);
                }
            }
        }

    //----------------------------------------------------------------
    void SankeyDiagram::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        GetLeftYAxis().ClearBrackets();

        size_t colorIndex{ 0 };
        // use 10% of the area as negative space between the groups
        constexpr double NEGATIVE_SPACE_PERCENT{ 10 };
        const auto yRangeStart{ GetLeftYAxis().GetRange().first };
        const auto yRangeEnd{ GetLeftYAxis().GetRange().second -
                              (safe_divide(GetLeftYAxis().GetRange().second - yRangeStart,
                                           NEGATIVE_SPACE_PERCENT)) };
        const auto spacePadding{ GetLeftYAxis().GetRange().second - yRangeEnd };

        const auto calcColumn =
            [&, this](const size_t colIndex, const double xStart, const double xEnd)
        {
            // set initial positions and sizes of group boxes
            auto startY{ yRangeEnd };
            for (auto& group : m_sankeyColumns[colIndex])
                {
                group.m_yAxisWidth = ((yRangeEnd - yRangeStart) * group.m_percentOfColumn);
                group.m_currentYAxisPosition = startY;
                group.m_yAxisTopPosition = startY;
                group.m_xAxisLeft = xStart;
                group.m_xAxisRight = xEnd;
                // prepare for next group underneath this one
                startY = std::max(group.m_currentYAxisPosition - group.m_yAxisWidth, 0.0);
                }

            // adjust group positions, inserting negative space between them
            const auto groupSpacePadding{ safe_divide<double>(spacePadding,
                                                              m_sankeyColumns[colIndex].size()) };
            size_t offsetMultiplier{ m_sankeyColumns[colIndex].size() };
            for (size_t i = 0; i < m_sankeyColumns[colIndex].size(); ++i, --offsetMultiplier)
                {
                auto& group{ m_sankeyColumns[colIndex][i] };
                group.m_currentYAxisPosition += (groupSpacePadding * offsetMultiplier);
                group.m_yAxisTopPosition = group.m_currentYAxisPosition;
                group.m_yAxisBottomPosition = group.m_currentYAxisPosition - group.m_yAxisWidth;
                }
        };

        const auto drawStreams =
            [&, this](const size_t colIndex, const double xStart, const double xEnd)
        {
            if (colIndex >= m_sankeyColumns.size())
                {
                return;
                }
            // which color are we on?
            auto currentColorIndex = [&, this]()
            {
                size_t prevColumnColorIndices{ 0 };
                for (size_t i = 0; i < colIndex; ++i)
                    {
                    prevColumnColorIndices += m_sankeyColumns[i].size();
                    }
                return prevColumnColorIndices;
            }();
            for (auto& group : m_sankeyColumns[colIndex])
                {
                auto currentColor{ GetBrushScheme()->GetBrush(currentColorIndex++).GetColour() };
                for (const auto& downstreamGroup : group.m_downStreamGroups.get_data())
                    {
                    auto downstreamGroupPos = std::ranges::find(
                        m_sankeyColumns[colIndex + 1], SankeyGroup{ downstreamGroup.first });
                    if (downstreamGroupPos != m_sankeyColumns[colIndex + 1].end())
                        {
                        const auto percentOfDownstreamGroup = safe_divide<double>(
                            downstreamGroup.second.second, downstreamGroupPos->m_frequency);
                        const auto streamWidth{ downstreamGroupPos->m_yAxisWidth *
                                                percentOfDownstreamGroup };

                        std::array<wxPoint, 10> pts{};
                        if (GetPhysicalCoordinates(xStart, group.m_currentYAxisPosition, pts[0]) &&
                            GetPhysicalCoordinates(xEnd, downstreamGroupPos->m_currentYAxisPosition,
                                                   pts[4]) &&
                            GetPhysicalCoordinates(
                                xEnd, downstreamGroupPos->m_currentYAxisPosition - streamWidth,
                                pts[5]) &&
                            GetPhysicalCoordinates(
                                xStart, (group.m_currentYAxisPosition - streamWidth), pts[9]))
                            {
                            const auto [topMidPointX, topMidPointY, isTopUpward] =
                                geometry::middle_point_horizontal_spline(
                                    std::make_pair(pts[0].x, pts[0].y),
                                    std::make_pair(pts[4].x, pts[4].y));
                            pts[2] = GraphItems::Polygon::PairToPoint(
                                std::make_pair(topMidPointX, topMidPointY));
                            pts[1] = isTopUpward ?
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_downward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[0]),
                                                 GraphItems::Polygon::PointToPair(pts[2]))) :
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_upward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[0]),
                                                 GraphItems::Polygon::PointToPair(pts[2])));
                            pts[3] = isTopUpward ?
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_upward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[2]),
                                                 GraphItems::Polygon::PointToPair(pts[4]))) :
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_downward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[2]),
                                                 GraphItems::Polygon::PointToPair(pts[4])));

                            const auto [bottomMidPointX, bottomMidPointY, isBottomUpward] =
                                geometry::middle_point_horizontal_spline(
                                    std::make_pair(pts[5].x, pts[5].y),
                                    std::make_pair(pts[9].x, pts[9].y));
                            pts[7] = GraphItems::Polygon::PairToPoint(
                                std::make_pair(bottomMidPointX, bottomMidPointY));
                            pts[6] = isTopUpward ?
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_upward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[5]),
                                                 GraphItems::Polygon::PointToPair(pts[7]))) :
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_downward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[5]),
                                                 GraphItems::Polygon::PointToPair(pts[7])));
                            pts[8] = isTopUpward ?
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_downward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[7]),
                                                 GraphItems::Polygon::PointToPair(pts[9]))) :
                                         GraphItems::Polygon::PairToPoint(
                                             geometry::middle_point_horizontal_upward_spline(
                                                 GraphItems::Polygon::PointToPair(pts[7]),
                                                 GraphItems::Polygon::PointToPair(pts[9])));

                            auto streamRibbon{ std::make_unique<GraphItems::Polygon>(
                                GraphItems::GraphItemInfo(wxString::Format(L"%s \u2192 %s",
                                                                           group.m_label,
                                                                           downstreamGroup.first))
                                    .Pen(wxNullPen)
                                    .Brush(Colors::ColorContrast::ChangeOpacity(currentColor, 100))
                                    .Scaling(GetScaling()),
                                pts) };
                            streamRibbon->SetShape(
                                GetFlowShape() == FlowShape::Curvy ?
                                    GraphItems::Polygon::PolygonShape::CurvyRectangle :
                                    GraphItems::Polygon::PolygonShape::Irregular);

                            AddObject(std::move(streamRibbon));
                            downstreamGroupPos->m_currentYAxisPosition -= streamWidth;
                            group.m_currentYAxisPosition -= streamWidth;
                            }
                        }
                    currentColor = Colors::ColorContrast::ShadeOrTint(currentColor);
                    }
                }
        };

        const auto alignColumns = [&, this]()
        {
            const auto lowestYPosition = [&, this]()
            {
                const auto lowestHangingColumn = std::ranges::min_element(
                    std::as_const(m_sankeyColumns),
                    [](const auto& lhv, const auto& rhv)
                    {
                        return lhv.back().m_yAxisBottomPosition < rhv.back().m_yAxisBottomPosition;
                    });
                return lowestHangingColumn->back().m_yAxisBottomPosition;
            }();

            // adjust spacing between groups so that the bottom of the columns line up vertically
            for (auto& col : m_sankeyColumns)
                {
                const auto yAdjustment{ lowestYPosition - col.back().m_yAxisBottomPosition };
                // leave the top group where it is, just adjust the ones beneath it
                // so that they have even spacing and then all columns will line up
                // evenly at the bottom
                std::for_each(col.begin() + 1, col.end(),
                              [&yAdjustment](auto& group) { group.OffsetY(yAdjustment); });
                }

            // ...then push everything down so that there is even spacing above
            // and below the groups
            const auto outerOffset{ lowestYPosition * math_constants::half };
            for (auto& col : m_sankeyColumns)
                {
                std::ranges::for_each(col,
                                      [&outerOffset](auto& group) { group.OffsetY(-outerOffset); });
                }
        };

        const auto drawColumns = [&, this]()
        {
            std::array<wxPoint, 4> pts{};

            for (const auto& col : m_sankeyColumns)
                {
                for (const auto& group : col)
                    {
                    if (group.m_isShown &&
                        GetPhysicalCoordinates(group.m_xAxisLeft, group.m_currentYAxisPosition,
                                               pts[0]) &&
                        GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisBottomPosition,
                                               pts[1]) &&
                        GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisBottomPosition,
                                               pts[2]) &&
                        GetPhysicalCoordinates(group.m_xAxisRight, group.m_currentYAxisPosition,
                                               pts[3]))
                        {
                        AddObject(std::make_unique<GraphItems::Polygon>(
                            GraphItems::GraphItemInfo(group.m_label)
                                .Pen(wxNullPen)
                                .Brush(GetBrushScheme()->GetBrush(colorIndex))
                                .Scaling(GetScaling()),
                            pts));
                        }
                    ++colorIndex;
                    }
                }
        };

        const auto drawLabels = [&, this](const size_t colIndex, Wisteria::Side labelSide)
        {
            std::array<wxPoint, 4> pts{};

            std::vector<std::unique_ptr<GraphItems::Label>> labels;
            for (auto& group : m_sankeyColumns[colIndex])
                {
                if (group.m_isShown &&
                    GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisTopPosition, pts[0]) &&
                    GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisBottomPosition,
                                           pts[1]) &&
                    GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisBottomPosition,
                                           pts[2]) &&
                    GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisTopPosition, pts[3]))
                    {
                    const wxString boxLabel = [&, this]()
                    {
                        return (GetGroupLabelDisplay() == BinLabelDisplay::BinName) ?
                                   group.m_label :
                               (GetGroupLabelDisplay() == BinLabelDisplay::BinNameAndPercentage) ?
                                   wxString::Format(
                                       // TRANSLATORS: Group label, the percentage value,
                                       // and then percent sign (%%)
                                       _(L"%s (%s%%)"), group.m_label,
                                       wxNumberFormatter::ToString(group.m_percentOfColumn * 100,
                                                                   0)) :
                               (GetGroupLabelDisplay() == BinLabelDisplay::BinNameAndValue) ?
                                   wxString::Format(
                                       L"%s (%s)", group.m_label,
                                       wxNumberFormatter::ToString(
                                           group.m_frequency, 0,
                                           wxNumberFormatter::Style::Style_WithThousandsSep)) :
                               (GetGroupLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                                   wxString::Format(
                                       // TRANSLATORS: Percent value placeholder and percentage
                                       // symbol (%%)
                                       _(L"%s%%"), wxNumberFormatter::ToString(
                                                       group.m_percentOfColumn * 100, 0)) :
                               (GetGroupLabelDisplay() == BinLabelDisplay::BinValue) ?
                                   wxString::Format(
                                       L"%s",
                                       wxNumberFormatter::ToString(
                                           group.m_frequency, 0,
                                           wxNumberFormatter::Style::Style_WithThousandsSep)) :
                               (GetGroupLabelDisplay() == BinLabelDisplay::BinValueAndPercentage) ?
                                   wxString::Format(
                                       // TRANSLATORS: Group frequency, the percentage value,
                                       // and then percent sign (%%)
                                       _(L"%s (%s%%)"),
                                       wxNumberFormatter::ToString(
                                           group.m_frequency, 0,
                                           wxNumberFormatter::Style::Style_WithThousandsSep),
                                       wxNumberFormatter::ToString(group.m_percentOfColumn * 100,
                                                                   0)) :
                                   wxString{};
                    }();
                    if (labelSide == Side::Right &&
                        GetPhysicalCoordinates(
                            group.m_xAxisRight,
                            group.m_yAxisTopPosition -
                                ((group.m_yAxisTopPosition - group.m_yAxisBottomPosition) *
                                 math_constants::half),
                            pts[0]))
                        {
                        auto groupLabel = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(boxLabel)
                                .Scaling(GetScaling())
                                .DPIScaling(GetDPIScaleFactor())
                                .Pen(wxNullPen)
                                .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                    GetPlotOrCanvasColor()))
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(pts[0])
                                .Anchoring(Anchoring::TopLeftCorner));
                        const auto bBox{ groupLabel->GetBoundingBox(dc) };
                        groupLabel->Offset(0, -(bBox.GetHeight() * math_constants::half));
                        labels.push_back(std::move(groupLabel));
                        }
                    else if (labelSide == Side::Left &&
                             GetPhysicalCoordinates(
                                 group.m_xAxisLeft,
                                 group.m_yAxisTopPosition -
                                     ((group.m_yAxisTopPosition - group.m_yAxisBottomPosition) *
                                      math_constants::half),
                                 pts[0]))
                        {
                        auto groupLabel = std::make_unique<GraphItems::Label>(
                            GraphItems::GraphItemInfo(boxLabel)
                                .Scaling(GetScaling())
                                .DPIScaling(GetDPIScaleFactor())
                                .Pen(wxNullPen)
                                .Padding(2, 2, 2, 2)
                                .AnchorPoint(pts[0])
                                .Anchoring(Anchoring::TopRightCorner));
                        const auto bBox{ groupLabel->GetBoundingBox(dc) };
                        groupLabel->Offset(0, -(bBox.GetHeight() * math_constants::half));
                        labels.push_back(std::move(groupLabel));
                        }
                    }
                }

            // look for overlapping labels
            if (labels.size() > 1)
                {
                std::optional<double> smallestFontScale{ std::nullopt };
                for (size_t i = 0; i < (labels.size() - 1); ++i)
                    {
                    const auto bBox = labels[i]->GetBoundingBox(dc);
                    const auto nextBBox = labels[i + 1]->GetBoundingBox(dc);
                    if (bBox.Intersects(nextBBox))
                        {
                        constexpr double MIN_FONT_SCALE{ 1.0 };
                        const auto heightEclipsed = bBox.GetBottom() - nextBBox.GetTop();
                        const auto percentEclipsed =
                            safe_divide<double>(heightEclipsed, bBox.GetHeight());
                        labels[i]->SetScaling(labels[i]->GetScaling() * (1.0 - percentEclipsed));
                        smallestFontScale =
                            std::max(MIN_FONT_SCALE,
                                     std::min(smallestFontScale.value_or(labels[i]->GetScaling()),
                                              labels[i]->GetScaling()));
                        const auto newBBox = labels[i]->GetBoundingBox(dc);
                        const auto heightDiff = bBox.GetHeight() - newBBox.GetHeight();
                        labels[i]->Offset(0, heightDiff * math_constants::half);
                        }
                    }
                // homogenize the labels' font scales (or hide the ones that are too small)
                // if there were overlaps that were adjusted
                if (smallestFontScale)
                    {
                    for (auto labelIter = labels.begin(); labelIter != labels.end(); /* in loop */)
                        {
                        if (compare_doubles_less((*labelIter)->GetScaling(),
                                                 smallestFontScale.value()))
                            {
                            labelIter = labels.erase(labelIter);
                            }
                        else
                            {
                            const auto bBox = (*labelIter)->GetBoundingBox(dc);
                            (*labelIter)->SetScaling(smallestFontScale.value());
                            const auto newBBox = (*labelIter)->GetBoundingBox(dc);
                            const auto heightDiff = bBox.GetHeight() - newBBox.GetHeight();
                            (*labelIter)->Offset(0, heightDiff * math_constants::half);
                            ++labelIter;
                            }
                        }
                    }
                }

            for (auto& label : labels)
                {
                AddObject(std::move(label));
                }
        };

        if (m_sankeyColumns.size() == 2)
            {
            calcColumn(0, 0, 0.5);
            calcColumn(1, 9.5, 10);

            alignColumns();
            drawColumns();

            drawStreams(0, 0.5, 9.5);

            drawLabels(0, Side::Right);
            drawLabels(1, Side::Left);
            }

        m_columnTotals.resize(m_sankeyColumns.size());
        std::transform(m_sankeyColumns.cbegin(), m_sankeyColumns.cend(), m_columnTotals.begin(),
                       [](const auto& group)
                       {
                           return std::accumulate(
                               group.cbegin(), group.cend(), 0.0,
                               [](const auto init, const auto& val)
                               { return (val.m_isShown ? val.m_frequency : 0) + init; });
                       });

        // if user hasn't supplied enough custom column headers (or any)
        if (m_columnHeaders.size() < m_sankeyColumns.size())
            {
            const auto originalSize{ m_columnHeaders.size() };
            m_columnHeaders.resize(m_sankeyColumns.size());
            for (size_t i = originalSize; i < m_sankeyColumns.size(); ++i)
                {
                m_columnHeaders[i] = L"@COLUMNNAME@";
                }
            }

        if (GetColumnHeaderDisplay() == GraphColumnHeader::AsHeader)
            {
            GetLeftYAxis()
                .GetHeader()
                .GetGraphItemInfo()
                .Text(ExpandColumnHeader(0))
                .ChildAlignment(RelativeAlignment::FlushLeft);
            GetRightYAxis()
                .GetHeader()
                .GetGraphItemInfo()
                .Text(ExpandColumnHeader(1))
                .ChildAlignment(RelativeAlignment::FlushRight);

            GetLeftYAxis().GetFooter().SetText(wxString{});
            GetRightYAxis().GetFooter().SetText(wxString{});
            }
        else if (GetColumnHeaderDisplay() == GraphColumnHeader::AsFooter)
            {
            GetLeftYAxis()
                .GetFooter()
                .GetGraphItemInfo()
                .Text(ExpandColumnHeader(0))
                .ChildAlignment(RelativeAlignment::FlushLeft);
            GetRightYAxis()
                .GetFooter()
                .GetGraphItemInfo()
                .Text(ExpandColumnHeader(1))
                .ChildAlignment(RelativeAlignment::FlushRight);

            GetLeftYAxis().GetHeader().SetText(wxString{});
            GetRightYAxis().GetHeader().SetText(wxString{});
            }
        else
            {
            GetLeftYAxis().GetFooter().SetText(wxString{});
            GetRightYAxis().GetFooter().SetText(wxString{});

            GetLeftYAxis().GetHeader().SetText(wxString{});
            GetRightYAxis().GetHeader().SetText(wxString{});
            }

        if (!m_fromAxisGroups.empty() && !m_sankeyColumns.empty())
            {
            for (const auto& aGr : m_fromAxisGroups)
                {
                assert(aGr.m_startGroup < m_sankeyColumns[0].size() &&
                       L"Axis group start out of range!");
                assert(aGr.m_endGroup < m_sankeyColumns[0].size() &&
                       L"Axis group start out of range!");
                const auto groupTop = m_sankeyColumns[0].at(aGr.m_startGroup).m_yAxisTopPosition;
                const auto groupBottom =
                    m_sankeyColumns[0].at(aGr.m_endGroup).m_yAxisBottomPosition;
                GetLeftYAxis().AddBracket(GraphItems::Axis::AxisBracket(
                    groupBottom, groupTop,
                    groupBottom + ((groupTop - groupBottom) * math_constants::half), aGr.m_label));
                }
            }
        }
    } // namespace Wisteria::Graphs
