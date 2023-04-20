///////////////////////////////////////////////////////////////////////////////
// Name:        sankeydiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
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
        const std::optional<wxString>& weightColumnName)
        {
        if (data == nullptr)
            { return; }

        GetSelectedIds().clear();
        m_sankeyColumns.clear();

        const auto fromColumn = data->GetCategoricalColumn(fromColumnName);
        if (fromColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'from' column not found for plot."), fromColumnName).ToUTF8());
            }
        const auto toColumn = data->GetCategoricalColumn(toColumnName);
        if (toColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': 'to' column not found for plot."), toColumnName).ToUTF8());
            }
        const bool useWeightColumn = weightColumnName.has_value();
        auto weightColumn = data->GetContinuousColumns().cend();
        if (useWeightColumn)
            {
            weightColumn = data->GetContinuousColumn(weightColumnName.value());
            if (weightColumn == data->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': weight column not found for plot."), weightColumnName.value()).ToUTF8());
                }
            }

        // load the combinations of labels (and weights)
        multi_value_frequency_aggregate_map<wxString, wxString,
                                            Data::wxStringLessNoCase, Data::wxStringLessNoCase,
                                            double> fromAndToMap;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useWeightColumn &&
                std::isnan(weightColumn->GetValue(i)))
                { continue; }
            const double comboTotal = (useWeightColumn ? weightColumn->GetValue(i) : 1);
            fromAndToMap.insert(fromColumn->GetValueAsLabel(i), toColumn->GetValueAsLabel(i), comboTotal);
            }

        m_sankeyColumns.resize(2);
        for (const auto& [key, subValues] : fromAndToMap.get_data())
            {
            m_sankeyColumns[0].push_back({ key, subValues.second, subValues.first });

            // add "from" values to second column
            for (const auto& subVal : subValues.first.get_data())
                {
                auto subColGroupPos = std::find(m_sankeyColumns[1].begin(), m_sankeyColumns[1].end(),
                    SankeyGroup{ subVal.first, subVal.second, SankeyGroup::DownStreamGroups{} });
                if (subColGroupPos != m_sankeyColumns[1].end())
                    { subColGroupPos->m_frequency += subVal.second; }
                else
                    { m_sankeyColumns[1].push_back({ subVal.first, subVal.second, SankeyGroup::DownStreamGroups{} }); }
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
        const auto drawColumn =
            [&, this]
            (const size_t colIndex, const double xStart, const double xEnd)
            {
            const auto yRangeStart{ GetLeftYAxis().GetRange().first };
            const auto yRangeEnd{ GetLeftYAxis().GetRange().second };
            std::array<wxPoint, 4> pts;

            // use 10% of the area as negative space between the groups
            const auto spacePadding{
                safe_divide<double>(safe_divide((yRangeEnd - yRangeStart), 10.0), m_sankeyColumns[colIndex].size())
                };
            auto startY{ yRangeEnd - (spacePadding * math_constants::half) };
            for (auto& group : m_sankeyColumns[colIndex])
                {
                const auto boxLength = ((yRangeEnd - yRangeStart) * group.m_percentOfColumn) - spacePadding;
                const auto endY{ std::max(startY - boxLength, 0.0) };
                if (GetPhysicalCoordinates(xStart, startY, pts[0]) &&
                    GetPhysicalCoordinates(xStart, endY, pts[1]) &&
                    GetPhysicalCoordinates(xEnd, endY, pts[2]) &&
                    GetPhysicalCoordinates(xEnd, startY, pts[3]))
                    {
                    AddObject(std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo(group.m_label).Pen(wxNullPen).
                        Brush(GetBrushScheme()->GetBrush(colorIndex)).
                        Scaling(GetScaling()),
                        pts));
                    group.m_currentYAxisPosition = startY;
                    group.m_yAxisWidth = boxLength;
                    }

                // prepare for next group underneath this one
                startY = endY - spacePadding;
                ++colorIndex;
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
                for (auto downstreamGroup = group.m_downStreamGroups.get_data().cbegin();
                    downstreamGroup != group.m_downStreamGroups.get_data().cend();
                    ++downstreamGroup)
                    {
                    auto downstreamGroupPos =
                        std::find(m_sankeyColumns[colIndex + 1].begin(),
                                  m_sankeyColumns[colIndex + 1].end(), SankeyGroup{ downstreamGroup->first });
                    if (downstreamGroupPos != m_sankeyColumns[colIndex + 1].end())
                        {
                        const auto percentOfDownstreamGroup =
                            safe_divide(downstreamGroup->second, downstreamGroupPos->m_frequency);
                        const auto streamWidth{ downstreamGroupPos->m_yAxisWidth * percentOfDownstreamGroup };

                        // If last group, then adjust positions of stream to fit precisely with the edges
                        // of the boxs (these alignments may be slightly off because of rounding).
                        double groupYOffset{ 0 }, downstreamYOffset{ 0 };
                        if (auto nextGroup = downstreamGroup;
                            ++nextGroup == group.m_downStreamGroups.get_data().cend())
                            {
                            groupYOffset =
                                currentGroupAxisWidthRemaining - streamWidth;
                            }

                        std::array<wxPoint, 10> pts;
                        if (GetPhysicalCoordinates(xStart,
                                group.m_currentYAxisPosition - groupYOffset, pts[0]) &&
                            GetPhysicalCoordinates(xEnd,
                                downstreamGroupPos->m_currentYAxisPosition, pts[4]) &&
                            GetPhysicalCoordinates(xEnd,
                                downstreamGroupPos->m_currentYAxisPosition - streamWidth, pts[5]) &&
                            GetPhysicalCoordinates(xStart,
                                (group.m_currentYAxisPosition - streamWidth) - groupYOffset, pts[9]))
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
                                GraphItemInfo(wxString::Format(L"%s \x2192 %s", group.m_label, downstreamGroup->first)).
                                Pen(wxNullPen).
                                Brush(ColorContrast::ChangeOpacity(currentColor, 100)).
                                Scaling(GetScaling()),
                                pts) };
                            //streamRibbon->SetShape(Polygon::PolygonShape::Spline);

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

        const auto drawLabels =
            [&, this]
            (const size_t colIndex, const double xStart, const double xEnd,
             Wisteria::Side labelSide)
            {
            const auto yRangeStart{ GetLeftYAxis().GetRange().first };
            const auto yRangeEnd{ GetLeftYAxis().GetRange().second };
            std::array<wxPoint, 4> pts;

            // use 10% of the area as negative space between the groups
            const auto spacePadding{
                safe_divide<double>(safe_divide((yRangeEnd - yRangeStart), 10.0), m_sankeyColumns[colIndex].size())
                };
            auto startY{ yRangeEnd - (spacePadding * math_constants::half) };
            for (auto& group : m_sankeyColumns[colIndex])
                {
                const auto boxLength = ((yRangeEnd - yRangeStart) * group.m_percentOfColumn) - spacePadding;
                const auto endY{ std::max(startY - boxLength, 0.0) };
                if (GetPhysicalCoordinates(xStart, startY, pts[0]) &&
                    GetPhysicalCoordinates(xStart, endY, pts[1]) &&
                    GetPhysicalCoordinates(xEnd, endY, pts[2]) &&
                    GetPhysicalCoordinates(xEnd, startY, pts[3]))
                    {
                    if (labelSide == Side::Right &&
                        GetPhysicalCoordinates(xEnd, startY - ((startY - endY) * math_constants::half), pts[0]))
                        {
                        auto groupLabel = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(group.m_label).
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
                        GetPhysicalCoordinates(xStart, startY - ((startY - endY) * math_constants::half), pts[0]))
                        {
                        auto groupLabel = std::make_shared<GraphItems::Label>(
                            GraphItemInfo(group.m_label).
                            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                            Pen(wxNullPen).
                            Padding(2, 2, 2, 2).
                            AnchorPoint(pts[0]).Anchoring(Anchoring::TopRightCorner));
                        const auto bBox{ groupLabel->GetBoundingBox(dc) };
                        groupLabel->Offset(0, -(bBox.GetHeight() * math_constants::half));
                        AddObject(groupLabel);
                        }
                    group.m_currentYAxisPosition = startY;
                    group.m_yAxisWidth = boxLength;
                    }

                // prepare for next group underneath this one
                startY = endY - spacePadding;
                ++colorIndex;
                }
            };

        if (m_sankeyColumns.size() == 2)
            {
            drawColumn(0, 0, 0.5);
            drawColumn(1, 9.5, 10);

            drawStreams(0, 0.5, 9.5);

            drawLabels(0, 0, 0.5, Side::Right);
            drawLabels(1, 9.5, 10, Side::Left);
            }
        }
    }
