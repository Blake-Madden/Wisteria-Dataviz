///////////////////////////////////////////////////////////////////////////////
// Name:        win_loss_sparkline.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "win_loss_sparkline.h"
#include "../math/safe_math.h"
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
        m_postseasonColor = Colors::ColorContrast::ChangeOpacity(
            Colors::ColorBrewer::GetColor(Colors::Color::BondiBlue), 75);
        m_highlightColor = Colors::ColorContrast::ChangeOpacity(
            Colors::ColorBrewer::GetColor(Colors::Color::ForestGreen), 75);
        }

    //----------------------------------------------------------------
    void WinLossSparkline::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& seasonColumnName,
        const wxString& wonColumnName, const wxString& shutoutColumnName,
        const wxString& homeGameColumnName,
        const std::optional<wxString>& postSeasonColumnName /*= std::nullopt*/)
        {
        GetSelectedIds().clear();
        m_longestWinningStreak = 0;
        m_hadShutoutWins = m_hadShutoutLosses = false;

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

        const auto postseasonColIter = [this, &data, &postSeasonColumnName]()
        {
            if (postSeasonColumnName)
                {
                const auto colIter = data->GetContinuousColumn(postSeasonColumnName.value());
                if (colIter == data->GetContinuousColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': column not found for graph."),
                                         postSeasonColumnName.value())
                            .ToUTF8());
                    }
                m_hasPostseasonData = true;
                return colIter;
                }
            return data->GetContinuousColumns().cend();
        }();

        const auto seasonColIter = data->GetCategoricalColumn(seasonColumnName);
        if (seasonColIter == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': season column not found for graph."), seasonColumnName)
                    .ToUTF8());
            }

        m_matrix.clear();

        // see how many seasons there are
        frequency_set<Data::GroupIdType> seasons;
        for (const auto& groupId : seasonColIter->GetValues())
            {
            seasons.insert(groupId);
            }
        // if more columns than seasons, then fix the column count
        m_matrix.resize(seasons.get_data().size());
        const auto maxItemByColumnCount = std::ranges::max_element(
            seasons.get_data(), [](const auto& item1, const auto& item2) noexcept
            { return item1.second < item2.second; });
        for (auto& row : m_matrix)
            {
            row.second.resize(maxItemByColumnCount->second);
            }

        size_t currentRow{ 0 }, currentColumn{ 0 };
        size_t currentRowWins{ 0 }, currentRowLosses{ 0 };
        size_t currentRowHomeWins{ 0 }, currentRowHomeLosses{ 0 };
        size_t currentRowRoadWins{ 0 }, currentRowRoadLosses{ 0 };
        auto currentGroupId = seasonColIter->GetValue(0);
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // move to next row if on another group ID
            if (seasonColIter->GetValue(i) != currentGroupId)
                {
                ++currentRow;
                currentColumn = currentRowWins = currentRowLosses = currentRowHomeWins =
                    currentRowHomeLosses = currentRowRoadWins = currentRowRoadLosses = 0;
                currentGroupId = seasonColIter->GetValue(i);
                }
            wxASSERT_MSG(currentRow < m_matrix.size(),
                         L"Invalid row when filling heatmap matrix! "
                         "Data should be sorted by season before calling SetData().!");
            wxASSERT_MSG(currentColumn < m_matrix[currentRow].second.size(),
                         L"Invalid column when filling heatmap matrix!");
            // should not happen, just do this to prevent crash if data was not sorted by
            // value and then by season first. What's displayed if this happens is the data
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
            m_matrix[currentRow].first.m_seasonLabel = seasonColIter->GetValueAsLabel(i);
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
            if (postSeasonColumnName && postseasonColIter != data->GetContinuousColumns().cend())
                {
                const auto postSeasonVal = postseasonColIter->GetValue(i);
                if (std::isfinite(postSeasonVal))
                    {
                    m_matrix[currentRow].second[currentColumn].m_postseason =
                        static_cast<bool>(postSeasonVal);
                    }
                }
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

        if (m_highlightBestRecords)
            {
            CalculateRecords();
            }
        }

    //----------------------------------------------------------------
    void WinLossSparkline::CalculateRecords()
        {
        // get the team/season with the best record
        double highestPct{ 0 };
        for (auto& row : m_matrix)
            {
            double val{ 0 };
            if (row.first.m_pctLabel.ToDouble(&val))
                {
                highestPct = std::max(highestPct, val);
                }
            }
        for (auto& row : m_matrix)
            {
            double val{ 0 };
            if (row.first.m_pctLabel.ToDouble(&val) && compare_doubles(val, highestPct))
                {
                row.first.m_highlightPctLabel = true;
                }
            }

        // get the longest winning streak
        std::vector<size_t> rowWinningStreaks;
        rowWinningStreaks.reserve(m_matrix.size());
        for (auto& row : m_matrix)
            {
            size_t longestWinningStreak{ 0 };
            size_t consecutiveWins{ 0 };
            for (const auto& game : row.second)
                {
                // skip over canceled games
                if (game.m_valid)
                    {
                    if (game.m_won)
                        {
                        ++consecutiveWins;
                        if (game.m_shutout)
                            {
                            m_hadShutoutWins = true;
                            }
                        }
                    else
                        {
                        longestWinningStreak = std::max(longestWinningStreak, consecutiveWins);
                        consecutiveWins = 0;
                        if (game.m_shutout)
                            {
                            m_hadShutoutLosses = true;
                            }
                        }
                    }
                }
            rowWinningStreaks.push_back(longestWinningStreak);
            }
        m_longestWinningStreak = std::ranges::max(rowWinningStreaks);
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
        double seasonHeaderLabelHeight{ 0 };
        wxFont seasonHeaderLabelFont{ GetBottomXAxis().GetFont() };

        // find the width of the longest season label
        GraphItems::Label measuringLabel(GraphItems::GraphItemInfo()
                                             .Scaling(GetScaling())
                                             .Pen(wxNullPen)
                                             .DPIScaling(GetDPIScaleFactor()));

        wxCoord seasonLabelWidth{ 0 };
        for (const auto& [strVal, games] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_seasonLabel);
            seasonLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), seasonLabelWidth);
            }
        wxCoord overallRecordLabelWidth{ 0 };
        for (const auto& [strVal, games] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_overallRecordLabel);
            overallRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), overallRecordLabelWidth);
            }
        // measure the HOME column, including the header string
        wxCoord homeRecordLabelWidth{ 0 };
        for (const auto& [strVal, games] : m_matrix)
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
        for (const auto& [strVal, games] : m_matrix)
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
        for (const auto& [strVal, games] : m_matrix)
            {
            measuringLabel.SetText(strVal.m_pctLabel);
            pctRecordLabelWidth =
                std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), pctRecordLabelWidth);
            }
        measuringLabel.SetText(_(L"pct"));
        pctRecordLabelWidth =
            std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), pctRecordLabelWidth);
        const wxCoord allLabelsWidth{ seasonLabelWidth + overallRecordLabelWidth +
                                      homeRecordLabelWidth + roadRecordLabelWidth +
                                      pctRecordLabelWidth + (PADDING_BETWEEN_LABELS * 4) };

        drawArea.SetWidth(drawArea.GetWidth() - allLabelsWidth);
        // Free some space for the season labels above each column (even if one column).
        GraphItems::Label headerLabelTemplate(
            GraphItems::GraphItemInfo(_(L"home"))
                .Scaling(GetScaling())
                .Pen(wxNullPen)
                .DPIScaling(GetDPIScaleFactor())
                .Padding(LABEL_PADDING, LABEL_PADDING, LABEL_PADDING, LABEL_PADDING)
                .Font(GetBottomXAxis().GetFont()));
        seasonHeaderLabelHeight = headerLabelTemplate.GetBoundingBox(dc).GetHeight();

        // leave space for the headers and for even spacing between each row
        drawArea.SetHeight(drawArea.GetHeight() - seasonHeaderLabelHeight -
                           ((m_matrix.size() - 1) * PADDING_BETWEEN_LABELS));
        drawArea.Offset(wxPoint(allLabelsWidth, seasonHeaderLabelHeight));

        const double boxWidth =
            std::min<double>(safe_divide<size_t>(drawArea.GetHeight(), m_matrix.size()),
                             safe_divide<wxCoord>(drawArea.GetWidth(),
                                                  std::max<size_t>(m_matrix[0].second.size(), 5)));

        std::vector<std::unique_ptr<GraphItems::Label>> labels;

        // draw the boxes in a grid, row x column
        int currentRow{ 0 }, currentColumn{ 0 };

        auto homeHeader = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo(
                /* TRANSLATORS: Sports game, as played at the team's home stadium. */ _(L"home"))
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Pen(wxNullPen)
                .Font(seasonHeaderLabelFont)
                .Padding(0, 0, 0, LABEL_PADDING)
                .AnchorPoint(drawArea.GetTopLeft()));
        homeHeader->Offset(-(homeRecordLabelWidth + roadRecordLabelWidth + pctRecordLabelWidth +
                             (PADDING_BETWEEN_LABELS * 2)),
                           -seasonHeaderLabelHeight);
        homeHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(homeHeader));

        auto roadHeader = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo(
                /* TRANSLATORS: Sports game, where a team
                   travels away from their home stadium to play. */
                _(L"road"))
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Pen(wxNullPen)
                .Font(seasonHeaderLabelFont)
                .Padding(0, 0, 0, LABEL_PADDING)
                .AnchorPoint(drawArea.GetTopLeft()));
        roadHeader->Offset(-(roadRecordLabelWidth + pctRecordLabelWidth + PADDING_BETWEEN_LABELS),
                           -seasonHeaderLabelHeight);
        roadHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(roadHeader));

        auto pctHeader =
            std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo(
                                                    /* TRANSLATORS: Percentage, as in the percent of
                                                       games a team won during a season. */
                                                    _(L"pct"))
                                                    .Scaling(GetScaling())
                                                    .DPIScaling(GetDPIScaleFactor())
                                                    .Pen(wxNullPen)
                                                    .Font(seasonHeaderLabelFont)
                                                    .Padding(0, 0, 0, LABEL_PADDING)
                                                    .AnchorPoint(drawArea.GetTopLeft()));
        pctHeader->Offset(-pctRecordLabelWidth, -seasonHeaderLabelHeight);
        pctHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        labels.push_back(std::move(pctHeader));

        for (const auto& row : m_matrix)
            {
            size_t seasonGames{ 0 };
            bool inWinningStreak{ false };
            for (size_t gameCounter = 0; gameCounter < row.second.size(); ++gameCounter)
                {
                const auto& game = row.second[gameCounter];
                const auto xOffset{ (boxWidth * currentColumn) };
                const auto yOffset{ (currentRow * boxWidth) +
                                    (currentRow * PADDING_BETWEEN_LABELS) };
                const std::array<wxPoint, 4> pts = {
                    wxPoint(drawArea.GetTopLeft().x + xOffset, drawArea.GetTopLeft().y + yOffset),
                    wxPoint(drawArea.GetTopLeft().x + xOffset,
                            drawArea.GetTopLeft().y + boxWidth + yOffset),
                    wxPoint(drawArea.GetTopLeft().x + xOffset + boxWidth,
                            drawArea.GetTopLeft().y + boxWidth + yOffset),
                    wxPoint(drawArea.GetTopLeft().x + xOffset + boxWidth,
                            drawArea.GetTopLeft().y + yOffset)
                };

                wxRect boxRect{ pts[0], pts[2] };
                boxRect.Deflate(ScaleToScreenAndCanvas(1));

                // for missing data, just place a blank placeholder where the game should be
                if (!game.m_valid)
                    {
                    // If there are valid games after this one, then this must have been a
                    // cancellation. Otherwise, it could just be a shorter season than the others
                    // and these aren't really games.
                    bool moreValidGames{ false };
                    for (auto scanAheadCounter = gameCounter + 1;
                         scanAheadCounter < row.second.size(); ++scanAheadCounter)
                        {
                        if (row.second[scanAheadCounter].m_valid)
                            {
                            moreValidGames = true;
                            break;
                            }
                        }
                    // if there are valid games after one or the entire season was canceled,
                    // then show the game as crossed out
                    if (moreValidGames || seasonGames == 0)
                        {
                        auto smallerBox{ boxRect };
                        smallerBox.Deflate(smallerBox.GetWidth() * math_constants::tenth);
                        auto shp = std::make_unique<GraphItems::Shape>(
                            GraphItems::GraphItemInfo()
                                .Pen(wxPenInfo{
                                    Colors::ColorBrewer::GetColor(Colors::Color::PastelGray), 2 })
                                .Brush(wxNullBrush)
                                .Anchoring(Anchoring::TopLeftCorner)
                                .Scaling(GetScaling())
                                .DPIScaling(GetDPIScaleFactor()),
                            Icons::IconShape::CrossedOut, smallerBox.GetSize());
                        shp->SetBoundingBox(smallerBox, dc, GetScaling());
                        AddObject(std::move(shp));
                        }
                    else
                        {
                        AddObject(std::make_unique<GraphItems::Polygon>(
                            GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(wxNullBrush), pts));
                        }
                    ++currentColumn;
                    continue;
                    }

                ++seasonGames;

                if (game.m_postseason)
                    {
                    AddObject(std::make_unique<GraphItems::Polygon>(
                        GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(m_postseasonColor), pts));
                    }

                auto homeGameLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ *wxBLACK, 2 }.Cap(wxCAP_BUTT), GetScaling());
                auto winLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ m_winColor, 2 }.Cap(wxCAP_BUTT), GetScaling());
                auto lossLine = std::make_unique<GraphItems::Lines>(
                    wxPenInfo{ m_lossColor, 2 }.Cap(wxCAP_BUTT), GetScaling());

                if (game.m_homeGame)
                    {
                    homeGameLine->AddLine(
                        { boxRect.GetLeft(), boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                        { boxRect.GetRight(), boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                    }

                if (game.m_won)
                    {
                    // if not already known to be in a longest winning streak,
                    // scan ahead and see if we are in the start of one
                    if (!inWinningStreak)
                        {
                        size_t consecutiveWins{ 1 };
                        for (auto scanAheadCounter = gameCounter + 1;
                             scanAheadCounter < row.second.size(); ++scanAheadCounter)
                            {
                            if (row.second[scanAheadCounter].m_valid)
                                {
                                if (row.second[scanAheadCounter].m_won)
                                    {
                                    ++consecutiveWins;
                                    }
                                else
                                    {
                                    break;
                                    }
                                }
                            }
                        if (consecutiveWins == m_longestWinningStreak)
                            {
                            inWinningStreak = true;
                            }
                        }

                    if (inWinningStreak)
                        {
                        AddObject(std::make_unique<GraphItems::Polygon>(
                            GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(m_highlightColor),
                            pts));
                        }
                    winLine->AddLine(
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetTop() },
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                          boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                    if (game.m_shutout)
                        {
                        winLine->GetPen().SetWidth(4);
                        }
                    }
                else
                    {
                    inWinningStreak = false;
                    lossLine->AddLine(
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                          boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                        { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetBottom() });
                    if (game.m_shutout)
                        {
                        lossLine->GetPen().SetWidth(4);
                        }
                    }

                AddObject(std::move(lossLine));
                AddObject(std::move(winLine));
                AddObject(std::move(homeGameLine));

                ++currentColumn;
                }

                // add the season label (e.g., team name or season)
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - allLabelsWidth,
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) +
                                                    (currentRow * PADDING_BETWEEN_LABELS) };
                auto seasonRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(row.first.m_seasonLabel)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .DPIScaling(GetDPIScaleFactor())
                        .Font(GetBottomXAxis().GetFont())
                        .AnchorPoint(labelAnchorPoint)
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_PADDING, 0, 0)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                seasonRowLabel->SetBoundingBox(wxRect{ labelAnchorPoint.x, labelAnchorPoint.y,
                                                       seasonLabelWidth,
                                                       static_cast<wxCoord>(boxWidth) },
                                               dc, GetScaling());
                labels.push_back(std::move(seasonRowLabel));
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
                        .FontBackgroundColor(row.first.m_highlightPctLabel ? m_highlightColor :
                                                                             wxNullColour)
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
            if (!currentLabel->GetText().empty())
                {
                smallestTextScaling = std::min(currentLabel->GetScaling(), smallestTextScaling);
                }
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
                _(L"Won") + (m_hadShutoutWins ? _(L"\nWon in a shutout") : wxString{}) +
                _(L"\nLost") + (m_hadShutoutLosses ? _(L"\nLost in a shutout") : wxString{}) +
                _(L"\nHome game\nCanceled game / scrimmage ") +
                (m_hasPostseasonData ? _(L"\nPostseason") : wxString{}) +
                (m_highlightBestRecords ? _(L"\nBest record / longest winning streak") :
                                          wxString{}))
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft)
                .Font(GetLeftYAxis().GetFont())
                .FontColor(GetLeftYAxis().GetFontColor()));
        legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine, m_winColor,
                                              wxNullBrush);
        if (m_hadShutoutWins)
            {
            legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine,
                                                  wxPenInfo(m_winColor, 4).Cap(wxCAP_BUTT),
                                                  wxNullBrush);
            }
        legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine, m_lossColor,
                                              wxNullBrush);
        if (m_hadShutoutLosses)
            {
            legend->GetLegendIcons().emplace_back(Icons::IconShape::VerticalLine,
                                                  wxPenInfo(m_lossColor, 4).Cap(wxCAP_BUTT),
                                                  wxNullBrush);
            }
        legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine, *wxBLACK_PEN,
                                              wxNullBrush);
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::CrossedOut, Colors::ColorBrewer::GetColor(Colors::Color::PastelGray),
            wxNullBrush);
        if (m_hasPostseasonData)
            {
            legend->GetLegendIcons().emplace_back(Icons::IconShape::Square, m_postseasonColor,
                                                  m_postseasonColor);
            }
        if (m_highlightBestRecords)
            {
            legend->GetLegendIcons().emplace_back(Icons::IconShape::Square, m_highlightColor,
                                                  m_highlightColor);
            }

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
