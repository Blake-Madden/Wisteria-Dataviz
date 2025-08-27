///////////////////////////////////////////////////////////////////////////////
// Name:        win_loss_sparkline.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "win_loss_sparkline.h"
#include "../util/frequencymap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WinLossSparkline, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WinLossSparkline::WinLossSparkline(Wisteria::Canvas * canvas) : Graph2D(canvas)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        GetPen().SetColour(L"#BEBBBB");
        m_winColor = Colors::ColorBrewer::GetColor(Colors::Color::ForestGreen);
        m_lossColor = Colors::ColorBrewer::GetColor(Colors::Color::RedTomato);
        }

    //----------------------------------------------------------------
    void WinLossSparkline::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                   const wxString& wonColumnName, const wxString& shutoutColumnName,
                                   const wxString& homeGameColumnName,
                                   const wxString& groupColumnName)
        {
        GetSelectedIds().clear();

        const auto wonColIter = data->GetContinuousColumn(wonColumnName);
        if (wonColIter == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': column not found for graph."), wonColumnName).ToUTF8());
            }

        const auto shutoutColIter = data->GetContinuousColumn(shutoutColumnName);
        if (shutoutColIter == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': column not found for graph."), shutoutColumnName)
                    .ToUTF8());
            }

        const auto homeGameColIter = data->GetContinuousColumn(homeGameColumnName);
        if (homeGameColIter == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': column not found for graph."), homeGameColumnName)
                    .ToUTF8());
            }

        const auto groupColIter = data->GetCategoricalColumn(groupColumnName);
        if (groupColIter == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column not found for graph."), groupColumnName)
                    .ToUTF8());
            }

        m_matrix.clear();

        // see how many groups there are
        frequency_set<Data::GroupIdType> groups;
        for (const auto& groupId : groupColIter->GetValues())
            {
            groups.insert(groupId);
            }
        // if more columns than groups, then fix the column count
        m_matrix.resize(groups.get_data().size());
        const auto maxItemByColumnCount = std::ranges::max_element(
            groups.get_data(), [](const auto& item1, const auto& item2) noexcept
            { return item1.second < item2.second; });
        for (auto& row : m_matrix)
            {
            row.second.resize(maxItemByColumnCount->second);
            }

        size_t currentRow{ 0 }, currentColumn{ 0 };
        size_t currentRowWins{ 0 }, currentRowLosses{ 0 };
        size_t currentRowHomeWins{ 0 }, currentRowHomeLosses{ 0 };
        size_t currentRowRoadWins{ 0 }, currentRowRoadLosses{ 0 };
        auto currentGroupId = groupColIter->GetValue(0);
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // move to next row if on another group ID
            if (groupColIter->GetValue(i) != currentGroupId)
                {
                ++currentRow;
                currentColumn = currentRowWins = currentRowLosses = currentRowHomeWins =
                    currentRowHomeLosses = currentRowRoadWins = currentRowRoadLosses = 0;
                currentGroupId = groupColIter->GetValue(i);
                }
            wxASSERT_MSG(currentRow < m_matrix.size(),
                         L"Invalid row when filling heatmap matrix! "
                         "Data should be sorted by group before calling SetData().!");
            wxASSERT_MSG(currentColumn < m_matrix[currentRow].second.size(),
                         L"Invalid column when filling heatmap matrix!");
            // should not happen, just do this to prevent crash if data was not sorted by
            // value and then by group first. What's displayed if this happens is the data
            // won't be grouped properly, but it's showing it how the client passed it in.
            if (currentRow >= m_matrix.size())
                {
                std::vector<WinLossCell> newRow;
                newRow.resize(maxItemByColumnCount->second);
                m_matrix.emplace_back(WinLossRow{}, std::move(newRow));
                }
            // shouldn't happen, just done as sanity check
            if (currentRow >= m_matrix.size() ||
                currentColumn >= m_matrix[currentRow].second.size())
                {
                break;
                }
            m_matrix[currentRow].first.m_groupLabel = groupColIter->GetValueAsLabel(i);
            const auto wonVal = wonColIter->GetValue(i);
            const auto shutoutVal = shutoutColIter->GetValue(i);
            const auto homeGameVal = homeGameColIter->GetValue(i);
            if (!std::isfinite(wonVal) || !std::isfinite(shutoutVal) || !std::isfinite(homeGameVal))
                {
                ++currentColumn;
                continue;
                }
            m_matrix[currentRow].second[currentColumn].m_won = static_cast<bool>(wonVal);
            m_matrix[currentRow].second[currentColumn].m_shutout = static_cast<bool>(shutoutVal);
            m_matrix[currentRow].second[currentColumn].m_homeGame = static_cast<bool>(homeGameVal);
            m_matrix[currentRow].second[currentColumn].m_valid = true;

            if (m_matrix[currentRow].second[currentColumn].m_won)
                {
                ++currentRowWins;
                if (m_matrix[currentRow].second[currentColumn].m_homeGame)
                    {
                    ++currentRowHomeWins;
                    }
                else
                    {
                    ++currentRowRoadWins;
                    }
                }
            else
                {
                ++currentRowLosses;
                if (m_matrix[currentRow].second[currentColumn].m_homeGame)
                    {
                    ++currentRowHomeLosses;
                    }
                else
                    {
                    ++currentRowRoadLosses;
                    }
                }
            m_matrix[currentRow].first.m_overallRecordLabel =
                wxNumberFormatter::ToString(currentRowWins, 0, Settings::GetDefaultNumberFormat()) +
                L'–' +
                wxNumberFormatter::ToString(currentRowLosses, 0,
                                            Settings::GetDefaultNumberFormat());

            m_matrix[currentRow].first.m_homeRecordLabel =
                wxNumberFormatter::ToString(currentRowHomeWins, 0,
                                            Settings::GetDefaultNumberFormat()) +
                L'–' +
                wxNumberFormatter::ToString(currentRowHomeLosses, 0,
                                            Settings::GetDefaultNumberFormat());

            m_matrix[currentRow].first.m_roadRecordLabel =
                wxNumberFormatter::ToString(currentRowRoadWins, 0,
                                            Settings::GetDefaultNumberFormat()) +
                L'–' +
                wxNumberFormatter::ToString(currentRowRoadLosses, 0,
                                            Settings::GetDefaultNumberFormat());

            m_matrix[currentRow].first.m_pctLabel = wxNumberFormatter::ToString(
                safe_divide<double>(currentRowWins, currentRowWins + currentRowLosses), 3,
                wxNumberFormatter::Style::Style_None);

            ++currentColumn;
            }
        }

    //----------------------------------------------------------------
    void WinLossSparkline::RecalcSizes(wxDC & dc)
        {
        // if no data, then bail
        if (m_matrix.empty())
            {
            return;
            }

        Graph2D::RecalcSizes(dc);

        constexpr wxCoord LABEL_PADDING{ 4 };
        const wxCoord PADDING_BETWEEN_LABELS{ static_cast<wxCoord>(ScaleToScreenAndCanvas(10)) };

        // size the boxes to fit in the area available
        wxRect drawArea = GetPlotAreaBoundingBox();
        double groupHeaderLabelHeight{ 0 };
        wxFont groupHeaderLabelFont{ GetBottomXAxis().GetFont() };

        // find the width of the longest group label
        GraphItems::Label measuringLabel(GraphItems::GraphItemInfo()
                                             .Scaling(GetScaling())
                                             .Pen(wxNullPen)
                                             .DPIScaling(GetDPIScaleFactor()));

        wxCoord groupLabelWidth{ 0 };
        for (const auto& [strVal, cells] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_groupLabel);
            groupLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), groupLabelWidth);
            }
        wxCoord overallRecordLabelWidth{ 0 };
        for (const auto& [strVal, cells] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_overallRecordLabel);
            overallRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), overallRecordLabelWidth);
            }
        // measure the HOME column, including the header string
        wxCoord homeRecordLabelWidth{ 0 };
        for (const auto& [strVal, cells] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_homeRecordLabel);
            homeRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), homeRecordLabelWidth);
            }
        measuringLabel.SetText(_(L"home"));
        homeRecordLabelWidth =
            std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), homeRecordLabelWidth);
        // measure the ROAD column, including the header string
        wxCoord roadRecordLabelWidth{ 0 };
        for (const auto& [strVal, cells] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_roadRecordLabel);
            roadRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), roadRecordLabelWidth);
            }
        measuringLabel.SetText(_(L"road"));
        roadRecordLabelWidth =
            std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), roadRecordLabelWidth);
        // measure the PCT column, including the header string
        wxCoord pctRecordLabelWidth{ 0 };
        for (const auto& [strVal, cells] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_pctLabel);
            pctRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), pctRecordLabelWidth);
            }
        measuringLabel.SetText(_(L"pct"));
        pctRecordLabelWidth =
            std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), pctRecordLabelWidth);
        const wxCoord allLabelsWidth{ groupLabelWidth + overallRecordLabelWidth +
                                      homeRecordLabelWidth + roadRecordLabelWidth +
                                      pctRecordLabelWidth + (PADDING_BETWEEN_LABELS * 4) };

        drawArea.SetWidth(drawArea.GetWidth() - allLabelsWidth);
        // Free some space for the group labels above each column (even if one column).
        GraphItems::Label headerLabelTemplate(
            GraphItems::GraphItemInfo(_(L"home"))
                .Scaling(GetScaling())
                .Pen(wxNullPen)
                .DPIScaling(GetDPIScaleFactor())
                .Padding(LABEL_PADDING, LABEL_PADDING, LABEL_PADDING, LABEL_PADDING)
                .Font(GetBottomXAxis().GetFont()));
        groupHeaderLabelHeight = headerLabelTemplate.GetBoundingBox(dc).GetHeight();

        // leave space for the headers and for even spacing between each row
        drawArea.SetHeight(drawArea.GetHeight() - groupHeaderLabelHeight -
                           ((m_matrix.size() - 1) * PADDING_BETWEEN_LABELS));
        drawArea.Offset(wxPoint(allLabelsWidth, groupHeaderLabelHeight));

        const double boxWidth =
            std::min<double>(safe_divide<size_t>(drawArea.GetHeight(), m_matrix.size()),
                             safe_divide<wxCoord>(drawArea.GetWidth(),
                                                  std::max<size_t>(m_matrix[0].second.size(), 5)));

        std::vector<std::unique_ptr<GraphItems::Label>> labels;

        // draw the boxes in a grid, row x column
        int currentRow{ 0 }, currentColumn{ 0 };

        auto homeHeader =
            std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo(_(L"home"))
                                                    .Scaling(GetScaling())
                                                    .DPIScaling(GetDPIScaleFactor())
                                                    .Pen(wxNullPen)
                                                    .Font(groupHeaderLabelFont)
                                                    .Padding(0, 0, 0, LABEL_PADDING)
                                                    .AnchorPoint(drawArea.GetTopLeft()));
        homeHeader->Offset(-(homeRecordLabelWidth + roadRecordLabelWidth + pctRecordLabelWidth +
                             (PADDING_BETWEEN_LABELS * 2)),
                           -groupHeaderLabelHeight);
        homeHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(homeHeader));

        auto roadHeader =
            std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo(_(L"road"))
                                                    .Scaling(GetScaling())
                                                    .DPIScaling(GetDPIScaleFactor())
                                                    .Pen(wxNullPen)
                                                    .Font(groupHeaderLabelFont)
                                                    .Padding(0, 0, 0, LABEL_PADDING)
                                                    .AnchorPoint(drawArea.GetTopLeft()));
        roadHeader->Offset(-(roadRecordLabelWidth + pctRecordLabelWidth + PADDING_BETWEEN_LABELS),
                           -groupHeaderLabelHeight);
        roadHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(roadHeader));

        auto pctHeader =
            std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo(_(L"pct"))
                                                    .Scaling(GetScaling())
                                                    .DPIScaling(GetDPIScaleFactor())
                                                    .Pen(wxNullPen)
                                                    .Font(groupHeaderLabelFont)
                                                    .Padding(0, 0, 0, LABEL_PADDING)
                                                    .AnchorPoint(drawArea.GetTopLeft()));
        pctHeader->Offset(-pctRecordLabelWidth, -groupHeaderLabelHeight);
        pctHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(pctHeader));

        for (const auto& row : m_matrix)
            {
            for (size_t cellCounter = 0; cellCounter < row.second.size(); ++cellCounter)
                {
                const auto& cell = row.second[cellCounter];
                const std::array<wxPoint, 4> pts = {
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                            drawArea.GetTopLeft().y + (currentRow * boxWidth) +
                                (currentRow * PADDING_BETWEEN_LABELS)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                            drawArea.GetTopLeft().y + boxWidth + (currentRow * boxWidth) +
                                (currentRow * PADDING_BETWEEN_LABELS)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                            drawArea.GetTopLeft().y + boxWidth + (currentRow * boxWidth) +
                                (currentRow * PADDING_BETWEEN_LABELS)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                            drawArea.GetTopLeft().y + (currentRow * boxWidth) +
                                (currentRow * PADDING_BETWEEN_LABELS))
                };

                /// @todo Show canceled game as X'ed out, not gray.
                // for missing data, just place a blank placeholder where the game should be
                if (!cell.m_valid)
                    {
                    // If there are valid games after this one, then this must have been a
                    // cancelation. Otherwise, it could just be a shorter season than the others
                    // and these aren't really games.
                    bool moreValidGames{ false };
                    for (auto scanAheadCounter = cellCounter + 1;
                         scanAheadCounter < row.second.size(); ++scanAheadCounter)
                        {
                        if (row.second[scanAheadCounter].m_valid)
                            {
                            moreValidGames = true;
                            break;
                            }
                        }
                    AddObject(std::make_unique<GraphItems::Polygon>(
                        GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(
                            moreValidGames ?
                                Colors::ColorBrewer::GetColor(Colors::Color::PastelGray) :
                                wxNullBrush),
                        pts));
                    ++currentColumn;
                    continue;
                    }

                wxRect boxRect{ pts[0], pts[2] };
                boxRect.Deflate(ScaleToScreenAndCanvas(1));

                auto homeGameLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ *wxBLACK, 2 }.Cap(wxCAP_BUTT), GetScaling());
                auto winLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ m_winColor, 2 }.Cap(wxCAP_BUTT), GetScaling());
                auto lossLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ m_lossColor, 2 }.Cap(wxCAP_BUTT), GetScaling());

                if (cell.m_homeGame)
                    {
                    homeGameLine->AddLine(
                        { boxRect.GetLeft(), boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                        { boxRect.GetRight(), boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                    }

                if (cell.m_won)
                    {
                    winLine->AddLine(
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetTop() },
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                          boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                    if (cell.m_shutout)
                        {
                        winLine->GetPen().SetWidth(4);
                        }
                    }
                else
                    {
                    lossLine->AddLine(
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                          boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetBottom() });
                    if (cell.m_shutout)
                        {
                        lossLine->GetPen().SetWidth(4);
                        }
                    }

                AddObject(std::move(lossLine));
                AddObject(std::move(winLine));
                AddObject(std::move(homeGameLine));
                ++currentColumn;
                }

                // add the group label (e.g., team name)
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - allLabelsWidth,
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) +
                                                    (currentRow * PADDING_BETWEEN_LABELS) };
                auto groupRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_groupLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .DPIScaling(GetDPIScaleFactor())
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, 0)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                groupRowLabel->SetBoundingBox(wxRect{ labelAnchorPoint.x, labelAnchorPoint.y,
                                                      groupLabelWidth,
                                                      static_cast<wxCoord>(boxWidth) },
                                              dc, GetScaling());
                labels.push_back(std::move(groupRowLabel));
                }

                // overall record
                {
                const wxPoint labelAnchorPoint{
                    drawArea.GetTopLeft().x - overallRecordLabelWidth - homeRecordLabelWidth -
                        roadRecordLabelWidth - pctRecordLabelWidth - (PADDING_BETWEEN_LABELS * 3),
                    drawArea.GetTopLeft().y + static_cast<wxCoord>(currentRow * boxWidth) +
                        (currentRow * PADDING_BETWEEN_LABELS)
                };
                auto overallRecordRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_overallRecordLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .DPIScaling(GetDPIScaleFactor())
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, LABEL_PADDING)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                overallRecordRowLabel->SetBoundingBox(
                    wxRect{ labelAnchorPoint.x, labelAnchorPoint.y, overallRecordLabelWidth,
                            static_cast<wxCoord>(boxWidth) },
                    dc, GetScaling());
                labels.push_back(std::move(overallRecordRowLabel));
                }

                // home record
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - homeRecordLabelWidth -
                                                    roadRecordLabelWidth - pctRecordLabelWidth -
                                                    (PADDING_BETWEEN_LABELS * 2),
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) +
                                                    (currentRow * PADDING_BETWEEN_LABELS) };
                auto homeRecordRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_homeRecordLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .DPIScaling(GetDPIScaleFactor())
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, LABEL_PADDING)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                homeRecordRowLabel->SetBoundingBox(wxRect{ labelAnchorPoint.x, labelAnchorPoint.y,
                                                           homeRecordLabelWidth,
                                                           static_cast<wxCoord>(boxWidth) },
                                                   dc, GetScaling());
                labels.push_back(std::move(homeRecordRowLabel));
                }

                // road record
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - roadRecordLabelWidth -
                                                    pctRecordLabelWidth - PADDING_BETWEEN_LABELS,
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) +
                                                    (currentRow * PADDING_BETWEEN_LABELS) };
                auto roadRecordRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_roadRecordLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .DPIScaling(GetDPIScaleFactor())
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, LABEL_PADDING)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                roadRecordRowLabel->SetBoundingBox(wxRect{ labelAnchorPoint.x, labelAnchorPoint.y,
                                                           roadRecordLabelWidth,
                                                           static_cast<wxCoord>(boxWidth) },
                                                   dc, GetScaling());
                labels.push_back(std::move(roadRecordRowLabel));
                }

                // pct record
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - pctRecordLabelWidth,
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) +
                                                    (currentRow * PADDING_BETWEEN_LABELS) };
                auto pctRecordRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_pctLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .DPIScaling(GetDPIScaleFactor())
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, LABEL_PADDING)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                pctRecordRowLabel->SetBoundingBox(wxRect{ labelAnchorPoint.x, labelAnchorPoint.y,
                                                          pctRecordLabelWidth,
                                                          static_cast<wxCoord>(boxWidth) },
                                                  dc, GetScaling());
                labels.push_back(std::move(pctRecordRowLabel));
                }

            ++currentRow;
            currentColumn = 0;
            }

        // make the labels have a uniform font size
        double smallestTextScaling{ std::numeric_limits<double>::max() };
        for (auto& currentLabel : labels)
            {
            smallestTextScaling = std::min(currentLabel->GetScaling(), smallestTextScaling);
            }

        for (auto& currentLabel : labels)
            {
            const wxRect bBox = currentLabel->GetBoundingBox(dc);
            currentLabel->SetScaling(smallestTextScaling);
            currentLabel->LockBoundingBoxScaling();
            currentLabel->SetBoundingBox(bBox, dc, 1.0);
            currentLabel->UnlockBoundingBoxScaling();
            AddObject(std::move(currentLabel));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> WinLossSparkline::CreateLegend(const LegendOptions& options)
        {
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo(
                _(L"Won\nWon in a Shutout\nLost\nLost in a Shutout\nHome Game\nCanceled Game"))
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft)
                .Font(GetLeftYAxis().GetFont())
                .FontColor(GetLeftYAxis().GetFontColor()));
        legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine, m_winColor,
                                              wxNullBrush);
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::VerticalLine, wxPenInfo(m_winColor, 4).Cap(wxCAP_BUTT), wxNullBrush);
        legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine, m_lossColor,
                                              wxNullBrush);
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::VerticalLine, wxPenInfo(m_lossColor, 4).Cap(wxCAP_BUTT), wxNullBrush);
        legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine, *wxBLACK_PEN,
                                              wxNullBrush);
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::Square, Colors::ColorBrewer::GetColor(Colors::Color::PastelGray),
            Colors::ColorBrewer::GetColor(Colors::Color::PastelGray));

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
