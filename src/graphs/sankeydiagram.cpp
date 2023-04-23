///////////////////////////////////////////////////////////////////////////////
// Name:        sankeydiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sankeydiagram.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace geometry;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void SankeyDiagram::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& fromColumnName, const wxString& toColumnName,
        const std::optional<wxString>& fromWeightColumnName,
        const std::optional<wxString>& toWeightColumnName)
        {
        if (data == nullptr)
            { return; }

        GetSelectedIds().clear();
        m_sankeyColumns.clear();

        const auto fromColumn = data->GetCategoricalColumn(fromColumnName);
        if (fromColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'from' column not found for diagram."), fromColumnName).ToUTF8());
            }
        const auto toColumn = data->GetCategoricalColumn(toColumnName);
        if (toColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'to' column not found for diagram."), toColumnName).ToUTF8());
            }

        // weight columns
        if (fromWeightColumnName.has_value() && !toWeightColumnName.has_value())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'from' weight column provided, but 'to' column was not."), fromWeightColumnName.value()).ToUTF8());
            }
        else if (toWeightColumnName.has_value() && !fromWeightColumnName.has_value())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'to' weight column provided, but 'from' column was not."), toWeightColumnName.value()).ToUTF8());
            }
        const bool useWeightColumn = fromWeightColumnName.has_value() && toWeightColumnName.has_value();
        auto fromWeightColumn = data->GetContinuousColumns().cend();
        auto toWeightColumn = data->GetContinuousColumns().cend();
        if (useWeightColumn)
            {
            fromWeightColumn = data->GetContinuousColumn(fromWeightColumnName.value());
            if (fromWeightColumn == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': 'from' weight column not found for diagram."), fromWeightColumnName.value()).ToUTF8());
                }
            toWeightColumn = data->GetContinuousColumn(toWeightColumnName.value());
            if (toWeightColumn == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': 'to' weight column not found for diagram."), toWeightColumnName.value()).ToUTF8());
                }
            }

        m_columnsNames = { fromColumnName, toColumnName };

        // load the combinations of labels (and weights)
        multi_value_frequency_double_aggregate_map<wxString, wxString,
                                            Data::wxStringLessNoCase, Data::wxStringLessNoCase> fromAndToMap;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useWeightColumn &&
                (std::isnan(fromWeightColumn->GetValue(i)) ||
                 std::isnan(toWeightColumn->GetValue(i))) )
                { continue; }

            fromAndToMap.insert(fromColumn->GetValueAsLabel(i), toColumn->GetValueAsLabel(i),
                (useWeightColumn ? fromWeightColumn->GetValue(i) : 1),
                (useWeightColumn ? toWeightColumn->GetValue(i) : 1));
            }

        m_sankeyColumns.resize(2);
        for (const auto& [key, subValues] : fromAndToMap.get_data())
            {
            m_sankeyColumns[0].push_back({ key, subValues.second, subValues.first });

            // add "from" values to second column
            for (const auto& subVal : subValues.first.get_data())
                {
                auto subColGroupPos = std::find(m_sankeyColumns[1].begin(), m_sankeyColumns[1].end(),
                    SankeyGroup{ subVal.first, subVal.second.second, SankeyGroup::DownStreamGroups{} });
                if (subColGroupPos != m_sankeyColumns[1].end())
                    { subColGroupPos->m_frequency += subVal.second.second; }
                else
                    {
                    m_sankeyColumns[1].push_back(
                        { subVal.first, subVal.second.second, SankeyGroup::DownStreamGroups{} });
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
                [](const auto initVal, const auto val) noexcept
                { return val.m_frequency + initVal; });
            maxGroupTotal = std::max(maxGroupTotal, groupTotal);
            }

        for (auto& col : m_sankeyColumns)
            {
            const auto groupTotal = std::accumulate(col.cbegin(), col.cend(), 0.0,
                [](const auto initVal, const auto val) noexcept
                { return val.m_frequency + initVal; });
            // Column lost some observations for the upstream groups, so add
            // an empty group on top of it and flag it as not having any upstream
            // groups flowing into it. (That will cause it to not be drawn.)
            if (groupTotal < maxGroupTotal)
                {
                col.insert(col.begin(),
                    { wxString{}, (maxGroupTotal - groupTotal),
                      SankeyGroup::DownStreamGroups{} })->m_hasParent = false;
                }
            }

        // set how much each group consumes of its respective column
        for (auto& col : m_sankeyColumns)
            {
            const auto groupTotal = std::accumulate(col.cbegin(), col.cend(), 0.0,
                [](const auto initVal, const auto val) noexcept
                { return val.m_frequency + initVal; });
            for (auto& group : col)
                { group.m_percentOfColumn = safe_divide(group.m_frequency, groupTotal); }
            }
        }

    //----------------------------------------------------------------
    void SankeyDiagram::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        size_t colorIndex{ 0 };
        // use 10% of the area as negative space between the groups
        double negativeSpacePercent{ 10 };
        const auto yRangeStart{ GetLeftYAxis().GetRange().first };
        const auto yRangeEnd{
            GetLeftYAxis().GetRange().second -
            (safe_divide(GetLeftYAxis().GetRange().second - yRangeStart, negativeSpacePercent)) };
        const auto spacePadding{ GetLeftYAxis().GetRange().second - yRangeEnd };

        const auto calcColumn =
            [&, this]
            (const size_t colIndex, const double xStart, const double xEnd)
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
            const auto groupSpacePadding{ safe_divide<double>(spacePadding, m_sankeyColumns[colIndex].size()) };
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
            [&, this]
            (const size_t colIndex, const double xStart, const double xEnd)
            {
            if (colIndex >= m_sankeyColumns.size())
                { return; }
            // which color are we on?
            auto currentColorIndex = [&, this]()
                {
                size_t currentColorIndex{ 0 };
                for (size_t i = 0; i < colIndex; ++i)
                    { currentColorIndex += m_sankeyColumns[i].size(); }
                return currentColorIndex;
                }();
            for (auto& group : m_sankeyColumns[colIndex])
                {
                auto currentColor{ GetBrushScheme()->GetBrush(currentColorIndex++).GetColour() };
                auto currentGroupAxisWidthRemaining{ group.m_yAxisWidth };
                for (const auto& downstreamGroup : group.m_downStreamGroups.get_data())
                    {
                    auto downstreamGroupPos =
                        std::find(m_sankeyColumns[colIndex + 1].begin(),
                                  m_sankeyColumns[colIndex + 1].end(), SankeyGroup{ downstreamGroup.first });
                    if (downstreamGroupPos != m_sankeyColumns[colIndex + 1].end())
                        {
                        const auto percentOfDownstreamGroup =
                            safe_divide<double>(downstreamGroup.second.second, downstreamGroupPos->m_frequency);
                        const auto streamWidth{ downstreamGroupPos->m_yAxisWidth * percentOfDownstreamGroup };

                        std::array<wxPoint, 10> pts;
                        if (GetPhysicalCoordinates(xStart,
                                group.m_currentYAxisPosition, pts[0]) &&
                            GetPhysicalCoordinates(xEnd,
                                downstreamGroupPos->m_currentYAxisPosition, pts[4]) &&
                            GetPhysicalCoordinates(xEnd,
                                downstreamGroupPos->m_currentYAxisPosition - streamWidth, pts[5]) &&
                            GetPhysicalCoordinates(xStart,
                                (group.m_currentYAxisPosition - streamWidth), pts[9]))
                            {
                            const auto [topMidPointX, topMidPointY, isTopUpward] =
                                middle_point_horizontal_spline(
                                    std::make_pair(pts[0].x, pts[0].y), std::make_pair(pts[4].x, pts[4].y));
                            pts[2] = Polygon::PairToPoint(std::make_pair(topMidPointX, topMidPointY));
                            pts[1] = isTopUpward ?
                                Polygon::PairToPoint(
                                    middle_point_horizontal_downward_spline(Polygon::PointToPair(pts[0]),
                                        Polygon::PointToPair(pts[2]))) :
                                Polygon::PairToPoint(
                                    middle_point_horizontal_upward_spline(Polygon::PointToPair(pts[0]),
                                                                      Polygon::PointToPair(pts[2])));
                            pts[3] = isTopUpward ?
                                Polygon::PairToPoint(
                                    middle_point_horizontal_upward_spline(Polygon::PointToPair(pts[2]),
                                        Polygon::PointToPair(pts[4]))) :
                                Polygon::PairToPoint(
                                    middle_point_horizontal_downward_spline(Polygon::PointToPair(pts[2]),
                                                                            Polygon::PointToPair(pts[4])));

                            const auto [bottomMidPointX, bottomMidPointY, isBottomUpward] =
                                middle_point_horizontal_spline(
                                    std::make_pair(pts[5].x, pts[5].y), std::make_pair(pts[9].x, pts[9].y));
                            pts[7] = Polygon::PairToPoint(std::make_pair(bottomMidPointX, bottomMidPointY));
                            pts[6] = isTopUpward ?
                                Polygon::PairToPoint(
                                    middle_point_horizontal_upward_spline(Polygon::PointToPair(pts[5]),
                                        Polygon::PointToPair(pts[7]))) :
                                Polygon::PairToPoint(
                                    middle_point_horizontal_downward_spline(Polygon::PointToPair(pts[5]),
                                        Polygon::PointToPair(pts[7])));
                            pts[8] = isTopUpward ?
                                Polygon::PairToPoint(
                                    middle_point_horizontal_downward_spline(Polygon::PointToPair(pts[7]),
                                        Polygon::PointToPair(pts[9]))) :
                                Polygon::PairToPoint(
                                    middle_point_horizontal_upward_spline(Polygon::PointToPair(pts[7]),
                                        Polygon::PointToPair(pts[9])));

                            auto streamRibbon{
                                std::make_shared<GraphItems::Polygon>(
                                GraphItemInfo(wxString::Format(L"%s \x2192 %s", group.m_label, downstreamGroup.first)).
                                Pen(wxNullPen).
                                Brush(ColorContrast::ChangeOpacity(currentColor, 100)).
                                Scaling(GetScaling()),
                                pts) };
                            streamRibbon->SetShape(GetFlowShape() == FlowShape::Curvy ?
                                Polygon::PolygonShape::CurvyRectangle :
                                Polygon::PolygonShape::Irregular);

                            AddObject(streamRibbon);
                            downstreamGroupPos->m_currentYAxisPosition -= streamWidth;
                            group.m_currentYAxisPosition -= streamWidth;
                            currentGroupAxisWidthRemaining -= streamWidth;
                            }
                        }
                    currentColor = ColorContrast::ShadeOrTint(currentColor);
                    }
                }
            };

        const auto alignColumns =
            [&, this]()
            {
            const auto lowestYPosition = [&, this]()
                {
                const auto lowestHangingColumn =
                    std::min_element(m_sankeyColumns.cbegin(), m_sankeyColumns.cend(),
                        [](const auto& lhv, const auto& rhv)
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
                    [&yAdjustment](auto& group)
                    { group.OffsetY(yAdjustment); });
                }

            // ...then push everything down so that there is even spacing above
            // and below the groups
            const auto outerOffset{ lowestYPosition * math_constants::half};
            for (auto& col : m_sankeyColumns)
                {
                std::for_each(col.begin(), col.end(),
                    [&outerOffset](auto& group)
                    { group.OffsetY(-outerOffset); });
                }
            };

        const auto drawColumns =
            [&, this]()
            {
            std::array<wxPoint, 4> pts;
            
            for (const auto& col : m_sankeyColumns)
                {
                for (const auto& group : col)
                    {
                    if (group.m_hasParent &&
                        GetPhysicalCoordinates(group.m_xAxisLeft, group.m_currentYAxisPosition, pts[0]) &&
                        GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisBottomPosition, pts[1]) &&
                        GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisBottomPosition, pts[2]) &&
                        GetPhysicalCoordinates(group.m_xAxisRight, group.m_currentYAxisPosition, pts[3]))
                        {
                        AddObject(std::make_shared<GraphItems::Polygon>(
                            GraphItemInfo(group.m_label).Pen(wxNullPen).
                            Brush(GetBrushScheme()->GetBrush(colorIndex)).
                            Scaling(GetScaling()),
                            pts));
                        }
                    ++colorIndex;
                    }
                }
            };

        const auto drawLabels =
            [&, this]
            (const size_t colIndex, Wisteria::Side labelSide)
            {
            std::array<wxPoint, 4> pts;

            for (auto& group : m_sankeyColumns[colIndex])
                {
                if (group.m_hasParent &&
                    GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisTopPosition, pts[0]) &&
                    GetPhysicalCoordinates(group.m_xAxisLeft, group.m_yAxisBottomPosition, pts[1]) &&
                    GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisBottomPosition, pts[2]) &&
                    GetPhysicalCoordinates(group.m_xAxisRight, group.m_yAxisTopPosition, pts[3]))
                    {
                    const wxString boxLabel = [&, this]()
                        {
                        return (
                            GetGroupLabelDisplay() == BinLabelDisplay::BinName) ?
                                group.m_label :
                            (GetGroupLabelDisplay() == BinLabelDisplay::BinNameAndPercentage) ?
                                wxString::Format(L"%s\n%s%%", group.m_label,
                                    wxNumberFormatter::ToString(group.m_percentOfColumn * 100, 0)) :
                            (GetGroupLabelDisplay() == BinLabelDisplay::BinNameAndValue) ?
                                wxString::Format(L"%s\n%s", group.m_label,
                                    wxNumberFormatter::ToString(group.m_frequency, 0,
                                                                wxNumberFormatter::Style::Style_WithThousandsSep)) :
                            (GetGroupLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                                wxString::Format(L"%s%%",
                                    wxNumberFormatter::ToString(group.m_percentOfColumn * 100, 0)) :
                            (GetGroupLabelDisplay() == BinLabelDisplay::BinValue) ?
                                wxString::Format(L"%s",
                                    wxNumberFormatter::ToString(group.m_frequency, 0,
                                        wxNumberFormatter::Style::Style_WithThousandsSep)) :
                            (GetGroupLabelDisplay() == BinLabelDisplay::BinValueAndPercentage) ?
                                wxString::Format(L"%s\n%s%%",
                                    wxNumberFormatter::ToString(group.m_frequency, 0,
                                        wxNumberFormatter::Style::Style_WithThousandsSep),
                                    wxNumberFormatter::ToString(group.m_percentOfColumn * 100, 0)) :
                            wxString{};
                        }();
                    if (labelSide == Side::Right &&
                        GetPhysicalCoordinates(group.m_xAxisRight,
                            group.m_yAxisTopPosition -
                                ((group.m_yAxisTopPosition - group.m_yAxisBottomPosition) *
                                    math_constants::half), pts[0]))
                        {
                        auto groupLabel = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(boxLabel).
                            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                            Pen(wxNullPen).
                            FontColor(ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor())).
                            Padding(2, 2, 2, 2).
                            AnchorPoint(pts[0]).Anchoring(Anchoring::TopLeftCorner));
                        const auto bBox{ groupLabel->GetBoundingBox(dc) };
                        groupLabel->Offset(0, -(bBox.GetHeight() * math_constants::half));
                        AddObject(groupLabel);
                        }
                    else if (labelSide == Side::Left &&
                        GetPhysicalCoordinates(group.m_xAxisLeft,
                            group.m_yAxisTopPosition -
                                ((group.m_yAxisTopPosition - group.m_yAxisBottomPosition) *
                                    math_constants::half), pts[0]))
                        {
                        auto groupLabel = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(boxLabel).
                            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                            Pen(wxNullPen).
                            Padding(2, 2, 2, 2).
                            AnchorPoint(pts[0]).Anchoring(Anchoring::TopRightCorner));
                        const auto bBox{ groupLabel->GetBoundingBox(dc) };
                        groupLabel->Offset(0, -(bBox.GetHeight() * math_constants::half));
                        AddObject(groupLabel);
                        }
                    }
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

            if (GetColumnHeaderDisplay() == GraphColumnHeader::AsHeader)
                {
                GetLeftYAxis().GetHeader().GetGraphItemInfo().
                    Text(m_columnsNames[0]).ChildAlignment(RelativeAlignment::FlushLeft);
                GetRightYAxis().GetHeader().GetGraphItemInfo().
                    Text(m_columnsNames[1]).ChildAlignment(RelativeAlignment::FlushRight);

                GetLeftYAxis().GetFooter().SetText(wxString{});
                GetRightYAxis().GetFooter().SetText(wxString{});
                }
            else if (GetColumnHeaderDisplay() == GraphColumnHeader::AsFooter)
                {
                GetLeftYAxis().GetFooter().GetGraphItemInfo().
                    Text(m_columnsNames[0]).ChildAlignment(RelativeAlignment::FlushLeft);
                GetRightYAxis().GetFooter().GetGraphItemInfo().
                    Text(m_columnsNames[1]).ChildAlignment(RelativeAlignment::FlushRight);

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
            }
        }
    }
