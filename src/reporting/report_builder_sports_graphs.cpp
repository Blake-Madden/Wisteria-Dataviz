///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_sports_graphs.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"

namespace Wisteria
    {
    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWinLossSparkline(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for sparkline."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto postSeasonRaw = variablesNode->GetProperty(L"postseason")->AsString();
            const auto postSeason = ExpandConstants(postSeasonRaw);
            auto wlSparkline = std::make_shared<Graphs::WinLossSparkline>(canvas);
            if (!postSeasonRaw.empty())
                {
                wlSparkline->SetPropertyTemplate(L"variables.postseason", postSeasonRaw);
                }
            wlSparkline->SetData(
                foundPos->second,
                ExpandAndCache(wlSparkline.get(), L"variables.season",
                               variablesNode->GetProperty(L"season")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.won",
                               variablesNode->GetProperty(L"won")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.shutout",
                               variablesNode->GetProperty(L"shutout")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.home-game",
                               variablesNode->GetProperty(L"home-game")->AsString()),
                (postSeason.empty() ? std::nullopt : std::optional<wxString>(postSeason)));

            wlSparkline->HighlightBestRecords(
                graphNode->GetProperty(L"highlight-best-records")->AsBool(true));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, wlSparkline);
            return wlSparkline;
            }

        throw std::runtime_error(_(L"Variables not defined for sparkline.").ToUTF8());
        }
    } // namespace Wisteria
