///////////////////////////////////////////////////////////////////////////////
// Name:        sankeydiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sankeydiagram.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::SankeyDiagram, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    SankeyDiagram::SankeyDiagram(
        Canvas * canvas,
        const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/)
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
        m_fromAxisGroups.clear();

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

        // compares (sort group, "from" group) pairs
        struct StringPairLessNoCase
            {
            [[nodiscard]]
            bool
            operator()(const std::pair<wxString, wxString>& lhv,
                       const std::pair<wxString, wxString>& rhv) const
                {
                const int sortGroupCmp{ lhv.first.CmpNoCase(rhv.first) };
                return (sortGroupCmp != 0) ? (sortGroupCmp < 0) :
                                             (lhv.second.CmpNoCase(rhv.second) < 0);
                }
            };

        // Load the combinations of labels (and weights and groups).
        // Keys are (sort group, "from" group) pairs so that the same "from" label
        // appearing under different sort groups is treated as separate groups.
        // (For example, schools with the same name, but in different counties.)
        // If no sort column is in use, then the sort group is simply blank for every key.
        multi_value_frequency_double_aggregate_map<std::pair<wxString, wxString>, wxString,
                                                   StringPairLessNoCase, Data::wxStringLessNoCase>
            fromAndToMap;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useWeightColumn && (std::isnan(fromWeightColumn->GetValue(i)) ||
                                    std::isnan(toWeightColumn->GetValue(i))))
                {
                continue;
                }

            fromAndToMap.insert(std::make_pair(fromSortColumnName.has_value() ?
                                                   fromGroupColumn->GetValueAsLabel(i) :
                                                   wxString{},
                                               fromColumn->GetValueAsLabel(i)),
                                toColumn->GetValueAsLabel(i),
                                (useWeightColumn ? fromWeightColumn->GetValue(i) : 1),
                                (useWeightColumn ? toWeightColumn->GetValue(i) : 1));
            }

        m_sankeyColumns.resize(2);
        for (const auto& [key, subValues] : fromAndToMap.get_data())
            {
            const auto& [sortGroupLabel, fromLabel] = key;

            // the map is sorted by sort group, so start a new axis bracket when
            // the sort group changes (or extend the current one)
            if (fromSortColumnName.has_value())
                {
                const size_t currentIndex{ m_sankeyColumns[0].size() };
                if (m_fromAxisGroups.empty() ||
                    m_fromAxisGroups.back().m_label.CmpNoCase(sortGroupLabel) != 0)
                    {
                    m_fromAxisGroups.push_back(
                        SankeyAxisGroup{ sortGroupLabel, currentIndex, currentIndex });
                    }
                else
                    {
                    m_fromAxisGroups.back().m_endGroup = currentIndex;
                    }
                }

            m_sankeyColumns[0].emplace_back(fromLabel, subValues.second, subValues.first);

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

        for (size_t colIndex = 0; colIndex < m_sankeyColumns.size(); ++colIndex)
            {
            auto& col = m_sankeyColumns[colIndex];
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
                // padding at the top of the first column shifts its groups' indices,
                // so keep the axis brackets pointing at the correct groups
                if (colIndex == 0)
                    {
                    for (auto& axisGroup : m_fromAxisGroups)
                        {
                        ++axisGroup.m_startGroup;
                        ++axisGroup.m_endGroup;
                        }
                    }
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
    void SankeyDiagram::CalcColumn(const size_t colIndex, const double xStart, const double xEnd,
                                   const double yRangeStart, const double yRangeEnd,
                                   const double spacePadding)
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
        }

    //----------------------------------------------------------------
    void SankeyDiagram::AlignColumns()
        {
        if (std::ranges::any_of(m_sankeyColumns, [](const auto& col) { return col.empty(); }))
            {
            return;
            }

        const auto lowestYPosition = [this]()
        {
            const auto lowestHangingColumn = std::ranges::min_element(
                std::as_const(m_sankeyColumns), [](const auto& lhv, const auto& rhv)
                { return lhv.back().m_yAxisBottomPosition < rhv.back().m_yAxisBottomPosition; });
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
        }

    //----------------------------------------------------------------
    void SankeyDiagram::DrawColumns(size_t & colorIndex)
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
                        GraphItems::GraphItemInfo{ group.m_label }
                            .Pen(wxNullPen)
                            .Brush(GetBrushScheme()->GetBrush(colorIndex))
                            .Scaling(GetScaling()),
                        pts));
                    }
                ++colorIndex;
                }
            }
        }

    //----------------------------------------------------------------
    void SankeyDiagram::DrawStreams(const size_t colIndex, const double xStart, const double xEnd)
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
                auto downstreamGroupPos = std::ranges::find(m_sankeyColumns[colIndex + 1],
                                                            SankeyGroup{ downstreamGroup.first });
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
                        GetPhysicalCoordinates(xStart, (group.m_currentYAxisPosition - streamWidth),
                                               pts[9]))
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
                        pts[6] = isBottomUpward ?
                                     GraphItems::Polygon::PairToPoint(
                                         geometry::middle_point_horizontal_upward_spline(
                                             GraphItems::Polygon::PointToPair(pts[5]),
                                             GraphItems::Polygon::PointToPair(pts[7]))) :
                                     GraphItems::Polygon::PairToPoint(
                                         geometry::middle_point_horizontal_downward_spline(
                                             GraphItems::Polygon::PointToPair(pts[5]),
                                             GraphItems::Polygon::PointToPair(pts[7])));
                        pts[8] = isBottomUpward ?
                                     GraphItems::Polygon::PairToPoint(
                                         geometry::middle_point_horizontal_downward_spline(
                                             GraphItems::Polygon::PointToPair(pts[7]),
                                             GraphItems::Polygon::PointToPair(pts[9]))) :
                                     GraphItems::Polygon::PairToPoint(
                                         geometry::middle_point_horizontal_upward_spline(
                                             GraphItems::Polygon::PointToPair(pts[7]),
                                             GraphItems::Polygon::PointToPair(pts[9])));

                        auto streamRibbon{ std::make_unique<GraphItems::Polygon>(
                            GraphItems::GraphItemInfo{
                                wxString::Format(L"%s → %s", group.m_label, downstreamGroup.first) }
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
        }

    //----------------------------------------------------------------
    void SankeyDiagram::DrawLabels(const size_t colIndex, const Wisteria::Side labelSide,
                                   const BinLabelDisplay labelDisplay, wxDC& dc)
        {
        std::array<wxPoint, 4> pts{};

        // if a column only has a single (shown) group, then that group obviously
        // consumes 100% of the column, so showing its percentage would be redundant
        const bool columnHasMultipleGroups{ std::ranges::count_if(m_sankeyColumns[colIndex],
                                                                  [](const auto& group) {
                                                                      return group.m_isShown;
                                                                  }) > 1 };

        std::vector<std::unique_ptr<GraphItems::Label>> labels;
        for (auto& group : m_sankeyColumns[colIndex])
            {
            if (group.m_isShown &&
                GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisTopPosition, pts[0]) &&
                GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisBottomPosition, pts[1]) &&
                GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisBottomPosition, pts[2]) &&
                GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisTopPosition, pts[3]))
                {
                const wxString boxLabel = [&, this]()
                {
                    return (labelDisplay == BinLabelDisplay::BinName) ?
                               group.m_label :
                           (labelDisplay == BinLabelDisplay::BinNameAndPercentage) ?
                               (columnHasMultipleGroups ?
                                    wxString::Format(
                                        // TRANSLATORS: Group label, the percentage value,
                                        // and then percent sign (%%)
                                        _(L"%s (%s%%)"), group.m_label,
                                        wxNumberFormatter::ToString(group.m_percentOfColumn * 100,
                                                                    0)) :
                                    group.m_label) :
                           (labelDisplay == BinLabelDisplay::BinNameAndValue) ?
                               wxString::Format(
                                   L"%s (%s)", group.m_label,
                                   wxNumberFormatter::ToString(
                                       group.m_frequency, 0,
                                       wxNumberFormatter::Style::Style_WithThousandsSep)) :
                           (labelDisplay == BinLabelDisplay::BinPercentage) ?
                               (columnHasMultipleGroups ?
                                    wxString::Format(
                                        // TRANSLATORS: Percent value placeholder and
                                        // percentage symbol (%%)
                                        _(L"%s%%"), wxNumberFormatter::ToString(
                                                        group.m_percentOfColumn * 100, 0)) :
                                    group.m_label) :
                           (labelDisplay == BinLabelDisplay::BinValue) ?
                               wxString::Format(
                                   L"%s", wxNumberFormatter::ToString(
                                              group.m_frequency, 0,
                                              wxNumberFormatter::Style::Style_WithThousandsSep)) :
                           (labelDisplay == BinLabelDisplay::BinValueAndPercentage) ?
                               (columnHasMultipleGroups ?
                                    wxString::Format(
                                        // TRANSLATORS: Group frequency, the percentage value,
                                        // and then percent sign (%%)
                                        _(L"%s (%s%%)"),
                                        wxNumberFormatter::ToString(
                                            group.m_frequency, 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep),
                                        wxNumberFormatter::ToString(group.m_percentOfColumn * 100,
                                                                    0)) :
                                    wxString::Format(
                                        L"%s",
                                        wxNumberFormatter::ToString(
                                            group.m_frequency, 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep))) :
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
                        GraphItems::GraphItemInfo{ boxLabel }
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Pen(wxNullPen)
                            .FontColor(
                                Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
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
                        GraphItems::GraphItemInfo{ boxLabel }
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
                    if (compare_doubles_less((*labelIter)->GetScaling(), smallestFontScale.value()))
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

        if (m_sankeyColumns.size() == 2)
            {
            CalcColumn(0, 0, 0.5, yRangeStart, yRangeEnd, spacePadding);
            CalcColumn(1, 9.5, 10, yRangeStart, yRangeEnd, spacePadding);

            AlignColumns();
            DrawColumns(colorIndex);

            DrawStreams(0, 0.5, 9.5);

            DrawLabels(0, Side::Right, GetInitialColumnLabels(), dc);
            DrawLabels(1, Side::Left, GetGroupLabelDisplay(), dc);
            }

        m_columnTotals.resize(m_sankeyColumns.size());
        std::ranges::transform(std::as_const(m_sankeyColumns), m_columnTotals.begin(),
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
                wxASSERT_MSG(aGr.m_startGroup < m_sankeyColumns[0].size(),
                             L"Axis group start out of range!");
                wxASSERT_MSG(aGr.m_endGroup < m_sankeyColumns[0].size(),
                             L"Axis group end out of range!");
                const auto groupTop = m_sankeyColumns[0].at(aGr.m_startGroup).m_yAxisTopPosition;
                const auto groupBottom =
                    m_sankeyColumns[0].at(aGr.m_endGroup).m_yAxisBottomPosition;
                GetLeftYAxis().AddBracket(GraphItems::Axis::AxisBracket(
                    groupBottom, groupTop,
                    groupBottom + ((groupTop - groupBottom) * math_constants::half), aGr.m_label));
                }
            }
        }

    //----------------------------------------------------------------
    void SankeyDiagram::SetAutoAccessibilityAttributes()
        {
        if (m_sankeyColumns.empty())
            {
            return;
            }

        wxString label = _(L"A Sankey diagram");

        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");

        // walk each column ("question") left-to-right: read the column header,
        // then read each block in that column as "<block label> (<size>)".
        // Column 0 may have axis-bracket groupings (e.g., counties grouping schools);
        // when present, the bracket label is announced before its run of blocks.
        for (size_t colIdx = 0; colIdx < m_sankeyColumns.size(); ++colIdx)
            {
            label += L". ";

            // column header (the "question"). Prefer the user-supplied header
            // (which may expand placeholders like @COLUMNNAME@); fall back to
            // the raw column name if no header is set yet.
            wxString header;
            if (colIdx < m_columnHeaders.size() && !m_columnHeaders[colIdx].empty())
                {
                header = ExpandColumnHeader(colIdx);
                }
            else if (colIdx < m_columnsNames.size())
                {
                header = m_columnsNames[colIdx];
                }
            if (!header.empty())
                {
                label += header;
                label += L": ";
                }

            // brackets only apply to column 0 in this implementation
            const bool useBrackets = (colIdx == 0 && !m_fromAxisGroups.empty());

            wxString currentBracket;
            bool firstEntry{ true };
            for (size_t blockIdx = 0; blockIdx < m_sankeyColumns[colIdx].size(); ++blockIdx)
                {
                const auto& block = m_sankeyColumns[colIdx][blockIdx];
                // skip the synthetic padding block used to balance weighted columns
                if (!block.m_isShown || block.m_label.empty())
                    {
                    continue;
                    }

                if (useBrackets)
                    {
                    wxString blockBracket;
                    for (const auto& br : m_fromAxisGroups)
                        {
                        if (blockIdx >= br.m_startGroup && blockIdx <= br.m_endGroup)
                            {
                            blockBracket = br.m_label;
                            break;
                            }
                        }
                    // start a new bracketed run when the bracket label changes
                    if (blockBracket != currentBracket)
                        {
                        currentBracket = blockBracket;
                        if (!firstEntry)
                            {
                            label += L"; ";
                            }
                        if (!currentBracket.empty())
                            {
                            label += wxString::Format(_DT(L"%s: "), currentBracket);
                            }
                        firstEntry = true;
                        }
                    }

                if (!firstEntry)
                    {
                    label += L", ";
                    }
                firstEntry = false;

                label += wxString::Format(
                    L"%s (%s)", block.m_label,
                    wxNumberFormatter::ToString(block.m_frequency, 0,
                                                wxNumberFormatter::Style::Style_WithThousandsSep));
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
