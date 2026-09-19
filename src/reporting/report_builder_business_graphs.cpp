///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_business_graphs.cpp
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
    ReportBuilder::LoadFunnelChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for funnel chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(_(L"Variables not defined for funnel chart.").ToUTF8());
            }

        const auto stageVarNameRaw = variablesNode->GetProperty(L"stage")->AsString();
        const auto valueVarNameRaw = variablesNode->GetProperty(L"value")->AsString();
        const auto targetVarNameRaw = variablesNode->GetProperty(L"target")->AsString();

        if (stageVarNameRaw.empty() || valueVarNameRaw.empty())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"%s: stage and value variables must be specified for funnel chart."), dsName)
                    .ToUTF8());
            }

        auto funnelChart = std::make_shared<Graphs::FunnelChart>(canvas);
        funnelChart->SetPropertyTemplate(L"dataset", dsName);
        const auto stageVarName =
            ExpandAndCache(funnelChart.get(), L"variables.stage", stageVarNameRaw);
        const auto valueVarName =
            ExpandAndCache(funnelChart.get(), L"variables.value", valueVarNameRaw);
        std::optional<wxString> targetVarName{ std::nullopt };
        if (!targetVarNameRaw.empty())
            {
            targetVarName =
                ExpandAndCache(funnelChart.get(), L"variables.target", targetVarNameRaw);
            }

        const auto funnelStyle = graphNode->GetProperty(L"funnel-style")->AsString();
        if (funnelStyle.CmpNoCase(L"standard") == 0)
            {
            funnelChart->SetFunnelStyle(Graphs::FunnelChart::FunnelStyle::Standard);
            }
        else if (funnelStyle.CmpNoCase(L"glassy") == 0)
            {
            funnelChart->SetFunnelStyle(Graphs::FunnelChart::FunnelStyle::Glassy);
            }

        if (graphNode->HasProperty(L"show-explanations"))
            {
            funnelChart->ShowExplanations(graphNode->GetProperty(L"show-explanations")->AsBool());
            }
        if (graphNode->HasProperty(L"show-conversion-labels"))
            {
            funnelChart->ShowConversionLabels(
                graphNode->GetProperty(L"show-conversion-labels")->AsBool());
            }
        if (graphNode->HasProperty(L"target-ghost-opacity"))
            {
            funnelChart->SetTargetGhostOpacity(
                graphNode->GetProperty(L"target-ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
            }

        funnelChart->SetData(foundPos->second, stageVarName, valueVarName, targetVarName);

        LoadGraph(graphNode, canvas, currentRow, currentColumn, funnelChart);
        return funnelChart;
        }
    } // namespace Wisteria
