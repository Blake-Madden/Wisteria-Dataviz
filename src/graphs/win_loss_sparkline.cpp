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
    WinLossSparkline::WinLossSparkline(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
                                           std::make_unique<Colors::Schemes::ColorScheme>(
                                               Colors::Schemes::ColorScheme{ *wxWHITE, *wxBLACK }));

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        GetPen().SetColour(L"#BEBBBB");
        }

    //----------------------------------------------------------------
    void WinLossSparkline::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                   const wxString& wonColumnName, const wxString& shutoutColumnName,
                                   const wxString& homeGameColumnName,
                                   const wxString& groupColumnName)
        {
        GetSelectedIds().clear();

        const auto wonColIter = data->GetCategoricalColumn(wonColumnName);
        if (wonColIter == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': column not found for graph."), wonColumnName).ToUTF8());
            }

        const auto shutoutColIter = data->GetCategoricalColumn(shutoutColumnName);
        if (shutoutColIter == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': column not found for graph."), shutoutColumnName)
                    .ToUTF8());
            }

        const auto homeGameColIter = data->GetCategoricalColumn(homeGameColumnName);
        if (homeGameColIter == data->GetCategoricalColumns().cend())
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
                m_matrix.emplace_back(wxString{}, std::move(newRow));
                }
            // shouldn't happen, just done as sanity check
            if (currentRow >= m_matrix.size() ||
                currentColumn >= m_matrix[currentRow].second.size())
                {
                break;
                }
            m_matrix[currentRow].second[currentColumn].m_won =
                static_cast<bool>(wonColIter->GetValue(i));
            m_matrix[currentRow].second[currentColumn].m_shutout =
                static_cast<bool>(shutoutColIter->GetValue(i));
            m_matrix[currentRow].second[currentColumn].m_homeGame =
                static_cast<bool>(homeGameColIter->GetValue(i));
            m_matrix[currentRow].first.m_groupLabel = groupColIter->GetValueAsLabel(i);
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
        const auto padding = static_cast<int>(wxSizerFlags::GetDefaultBorder() * GetScaling());
        double groupHeaderLabelHeight{ 0 };
        wxFont groupHeaderLabelFont{ GetBottomXAxis().GetFont() };
        bool groupHeaderLabelMultiline{ false };

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
        if (m_matrix.size() > 1)
            {
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

            drawArea.SetHeight(drawArea.GetHeight() - groupHeaderLabelHeight);
            drawArea.Offset(wxPoint(allLabelsWidth, groupHeaderLabelHeight));
            }

        const double boxWidth =
            std::min<double>(safe_divide<size_t>(drawArea.GetHeight(), m_matrix.size()),
                             safe_divide<wxCoord>(drawArea.GetWidth(),
                                                  std::max<size_t>(m_matrix[0].second.size(), 5)));

        std::vector<std::unique_ptr<GraphItems::Label>> labels;

        // draw the boxes in a grid, row x column
        size_t currentRow{ 0 }, currentColumn{ 0 };

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
            for (const auto& cell : row.second)
                {
                // if no label on cell, then that means this row is jagged and there
                // are no more cells in it, so go to next row
                if (!cell.m_valid)
                    {
                    continue;
                    }

                const std::array<wxPoint, 4> pts = {
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                            drawArea.GetTopLeft().y + (currentRow * boxWidth)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                            drawArea.GetTopLeft().y + boxWidth + (currentRow * boxWidth)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                            drawArea.GetTopLeft().y + boxWidth + (currentRow * boxWidth)),
                    wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                            drawArea.GetTopLeft().y + (currentRow * boxWidth))
                };

                wxRect boxRect{ pts[0], pts[2] };
                boxRect.Deflate(ScaleToScreenAndCanvas(2));

                auto blackLines =
                    std::make_unique<GraphItems::Lines>(wxPenInfo{ *wxBLACK, 2 }, GetScaling());
                auto redLines =
                    std::make_unique<GraphItems::Lines>(wxPenInfo{ *wxRED, 2 }, GetScaling());

                if (cell.m_homeGame)
                    {
                    blackLines->AddLine(
                        { boxRect.GetLeft(), boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                        { boxRect.GetRight(), boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                    }
                if (cell.m_shutout)
                    {
                    if (cell.m_won)
                        {
                        redLines->AddLine(
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetTop() },
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                              boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                        }
                    else
                        {
                        redLines->AddLine(
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                              boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetBottom() });
                        }
                    }
                else
                    {
                    if (cell.m_won)
                        {
                        blackLines->AddLine(
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetTop() },
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                              boxRect.GetTop() + (boxRect.GetHeight() / 2) });
                        }
                    else
                        {
                        blackLines->AddLine(
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                              boxRect.GetTop() + (boxRect.GetHeight() / 2) },
                            { boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetBottom() });
                        }
                    }
                AddObject(std::move(blackLines));
                AddObject(std::move(redLines));
                ++currentColumn;
                }

                // add a group label
                {
                const wxPoint labelAnchorPoint{ drawArea.GetTopLeft().x - allLabelsWidth,
                                                drawArea.GetTopLeft().y +
                                                    static_cast<wxCoord>(currentRow * boxWidth) };
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
                    drawArea.GetTopLeft().y + static_cast<wxCoord>(currentRow * boxWidth)
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
                const wxPoint labelAnchorPoint{
                    drawArea.GetTopLeft().x - homeRecordLabelWidth - roadRecordLabelWidth -
                        pctRecordLabelWidth - (PADDING_BETWEEN_LABELS * 2),
                    drawArea.GetTopLeft().y + static_cast<wxCoord>(currentRow * boxWidth)
                };
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
                                                    static_cast<wxCoord>(currentRow * boxWidth) };
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
                                                    static_cast<wxCoord>(currentRow * boxWidth) };
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
            // if (GetDataset() == nullptr)
            {
            return nullptr;
            }

        // std::vector<double> validData;
        // std::ranges::copy_if(m_continuousColumn->GetValues(), std::back_inserter(validData),
        //                      [](auto x) { return std::isfinite(x); });
        // const auto minValue = *std::ranges::min_element(std::as_const(validData));
        // const auto maxValue = *std::ranges::max_element(std::as_const(validData));
        // auto legend = std::make_unique<GraphItems::Label>(
        //     GraphItems::GraphItemInfo(
        //         // add spaces on the empty lines to work around SVG exporting
        //         // stripping out the blank lines
        //         wxString::Format(
        //             L"%s\n \n \n%s",
        //             wxNumberFormatter::ToString(maxValue, 6, Settings::GetDefaultNumberFormat()),
        //             wxNumberFormatter::ToString(minValue, 6,
        //             Settings::GetDefaultNumberFormat())))
        //         .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs() * 1.5)
        //         .DPIScaling(GetDPIScaleFactor())
        //         .Anchoring(Anchoring::TopLeftCorner)
        //         .LabelAlignment(TextAlignment::FlushLeft)
        //         .FontColor(GetLeftYAxis().GetFontColor()));
        // if (options.IsIncludingHeader())
        //     {
        //     legend->SetText(wxString::Format(L"%s\n", m_continuousColumn->GetName()) +
        //                     legend->GetText());
        //     legend->GetHeaderInfo()
        //         .Enable(true)
        //         .LabelAlignment(TextAlignment::FlushLeft)
        //         .FontColor(GetLeftYAxis().GetFontColor());
        //     }
        // legend->GetLegendIcons().emplace_back(m_reversedColorSpectrum);

        // AddReferenceLinesAndAreasToLegend(*legend);
        // AdjustLegendSettings(*legend, options.GetPlacementHint());
        // return legend;
        }
    } // namespace Wisteria::Graphs
