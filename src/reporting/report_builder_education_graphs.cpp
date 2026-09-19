///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_education_graphs.cpp
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
    ReportBuilder::LoadScaleChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for scale chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto scoreVarNameRaw = variablesNode->GetProperty(L"score")->AsString();
            const auto scoreVarName = ExpandConstants(scoreVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);

            auto chart = std::make_shared<Graphs::ScaleChart>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
            if (!scoreVarNameRaw.empty())
                {
                chart->SetPropertyTemplate(L"variables.score", scoreVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                chart->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }

            if (graphNode->GetProperty(L"showcase-score")->AsBool(false))
                {
                chart->ShowcaseScore(true);
                }

            // load scales
            const auto scalesNode = graphNode->GetProperty(L"scales");
            if (scalesNode->IsOk())
                {
                const auto scales = scalesNode->AsNodes();
                for (const auto& scaleNode : scales)
                    {
                    std::vector<Graphs::BarChart::BarBlock> blocks;
                    const auto blocksNode = scaleNode->GetProperty(L"blocks");
                    if (blocksNode->IsOk())
                        {
                        const auto blockNodes = blocksNode->AsNodes();
                        blocks.reserve(blockNodes.size());
                        for (const auto& blockNode : blockNodes)
                            {
                            const auto length = blockNode->GetProperty(L"length")->AsDouble(10);
                            const auto label = blockNode->GetProperty(L"label")->AsString();
                            const auto color = ConvertColor(blockNode->GetProperty(L"color"));
                            blocks.emplace_back(
                                Graphs::BarChart::BarBlockInfo(length)
                                    .Brush(color.IsOk() ? wxBrush(color) : *wxGREEN_BRUSH)
                                    .Decal(GraphItems::Label(
                                        GraphItems::GraphItemInfo{ label }.LabelFitting(
                                            LabelFit::ScaleFontToFit))));
                            }
                        }
                    const auto header =
                        ExpandConstants(scaleNode->GetProperty(L"header")->AsString());
                    const auto startNode = scaleNode->GetProperty(L"start");
                    const std::optional<double> startPos =
                        startNode->IsOk() ? std::optional<double>(startNode->AsDouble(0)) :
                                            std::nullopt;
                    chart->AddScale(blocks, startPos, header);
                    }
                }

            // main scale values and precision
            const auto mainValsNode = graphNode->GetProperty(L"main-scale-values");
            if (mainValsNode->IsOk())
                {
                const auto mainVals = mainValsNode->AsDoubles();
                const auto precision = static_cast<uint8_t>(
                    graphNode->GetProperty(L"main-scale-precision")->AsDouble(0));
                chart->SetMainScaleValues(mainVals, precision);
                }

            chart->SetData(
                foundPos->second, scoreVarName,
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt));

            // column headers (must be set after AddScale/SetData)
            chart->SetMainScaleColumnHeader(
                ExpandConstants(graphNode->GetProperty(L"main-scale-header")->AsString()));
            chart->SetDataColumnHeader(
                ExpandConstants(graphNode->GetProperty(L"data-column-header")->AsString()));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, chart);
            return chart;
            }

        throw std::runtime_error(_(L"Variables not defined for scale chart.").ToUTF8());
        }
    } // namespace Wisteria
