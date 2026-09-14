///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_basic_graphs.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <wx/log.h>

namespace Wisteria
    {
    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for line plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto linePlot = std::make_shared<Graphs::LinePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                linePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            linePlot->SetData(
                foundPos->second,
                ExpandAndCache(linePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(linePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            LoadLinePlotBaseOptions(graphNode, linePlot.get());

            return linePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for line plot.").ToUTF8());
        }

    //---------------------------------------------------
    void ReportBuilder::LoadLinePlotBaseOptions(const wxSimpleJSON::Ptr_t& graphNode,
                                                Graphs::LinePlot* linePlot) const
        {
        // showcasing
        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            linePlot->SetGhostOpacity(
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
            }

        if (const auto showcaseNode = graphNode->GetProperty(L"showcase-lines");
            showcaseNode->IsOk() && showcaseNode->IsValueArray())
            {
            linePlot->ShowcaseLines(
                ExpandAndCache(linePlot, L"showcase-lines", showcaseNode->AsStrings()));
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadMultiSeriesLinePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                           size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for multi-series line plot."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto linePlot = std::make_shared<Graphs::MultiSeriesLinePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(foundPos->second,
                              ExpandAndCache(linePlot.get(), L"variables.y",
                                             variablesNode->GetProperty(L"y")->AsStrings()),
                              ExpandAndCache(linePlot.get(), L"variables.x",
                                             variablesNode->GetProperty(L"x")->AsString()));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            LoadLinePlotBaseOptions(graphNode, linePlot.get());

            return linePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for multi-series line plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWaffleChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        std::vector<GraphItems::ShapeInfo> shapes;

        if (const auto shapesNode = graphNode->GetProperty(L"shapes"); shapesNode->IsOk())
            {
            if (shapesNode->IsValueArray())
                {
                auto nodes = shapesNode->AsNodes();
                for (const auto& shpNode : nodes)
                    {
                    shapes.push_back(LoadShapeInfo(shpNode));
                    }
                }
            }
        else
            {
            throw std::runtime_error(_(L"No shapes provided for waffle chart.").ToUTF8());
            }

        std::optional<Graphs::WaffleChart::GridRounding> gridRound{ std::nullopt };
        if (graphNode->HasProperty(L"grid-round"))
            {
            if (graphNode->GetProperty(L"grid-round")->HasProperty(L"cell-count") &&
                graphNode->GetProperty(L"grid-round")->HasProperty(L"shape-index"))
                {
                gridRound = Graphs::WaffleChart::GridRounding{
                    static_cast<size_t>(graphNode->GetProperty(L"grid-round")
                                            ->GetProperty(L"cell-count")
                                            ->AsDouble(100)),
                    static_cast<size_t>(graphNode->GetProperty(L"grid-round")
                                            ->GetProperty(L"shape-index")
                                            ->AsDouble(100))
                };
                }
            }

        auto waffleChart = std::make_shared<Graphs::WaffleChart>(
            canvas, shapes, gridRound,
            graphNode->HasProperty(L"row-count") ?
                std::optional<size_t>(graphNode->GetProperty(L"row-count")->AsDouble()) :
                std::nullopt);

        LoadGraph(graphNode, canvas, currentRow, currentColumn, waffleChart);
        return waffleChart;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadRaceTrackChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                      size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for race track chart."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(_(L"Variables not defined for race track chart.").ToUTF8());
            }

        const auto valueVarNameRaw = variablesNode->GetProperty(L"value")->AsString();
        const auto valueVarName = ExpandConstants(valueVarNameRaw);
        const auto labelVarNameRaw = variablesNode->GetProperty(L"label")->AsString();
        const auto labelVarName = ExpandConstants(labelVarNameRaw);

        auto raceTrackChart = std::make_shared<Graphs::RaceTrackChart>(
            canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
            LoadGraphColorScheme(graphNode));
        if (!valueVarNameRaw.empty())
            {
            raceTrackChart->SetPropertyTemplate(L"variables.value", valueVarNameRaw);
            }
        if (!labelVarNameRaw.empty())
            {
            raceTrackChart->SetPropertyTemplate(L"variables.label", labelVarNameRaw);
            }

        raceTrackChart->SetData(foundPos->second, valueVarName, labelVarName);

        if (const auto trackCount = ReportEnumConvert::ConvertRaceTrackCount(
                graphNode->GetProperty(L"track-count")->AsString());
            trackCount.has_value())
            {
            raceTrackChart->SetTrackCount(trackCount.value());
            }

        if (graphNode->HasProperty(L"track-proportion"))
            {
            raceTrackChart->SetTrackProportion(
                graphNode->GetProperty(L"track-proportion")->AsDouble(0.65));
            }

        if (graphNode->HasProperty(L"start-angle"))
            {
            raceTrackChart->SetStartAngle(graphNode->GetProperty(L"start-angle")->AsDouble(270.0));
            }

        if (graphNode->HasProperty(L"show-labels"))
            {
            raceTrackChart->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool());
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, raceTrackChart);
        return raceTrackChart;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
                                     const std::shared_ptr<Graphs::BarChart>& barChart) const
        {
        if (barChart == nullptr)
            {
            return;
            }
        const auto boxEffect =
            ReportEnumConvert::ConvertBoxEffect(graphNode->GetProperty(L"box-effect")->AsString());
        if (boxEffect)
            {
            barChart->SetBarEffect(boxEffect.value());
            }

        // folding of bars that dwarf the others
        const auto serpentineMode = ReportEnumConvert::ConvertSerpentineMode(
            graphNode->GetProperty(L"serpentine")->AsString());
        if (serpentineMode)
            {
            barChart->SetSerpentineMode(serpentineMode.value());
            }
        if (graphNode->HasProperty(L"serpentine-threshold"))
            {
            barChart->SetSerpentineThreshold(
                graphNode->GetProperty(L"serpentine-threshold")->AsDouble(3.0));
            }
        if (graphNode->HasProperty(L"serpentine-fold-arrows"))
            {
            barChart->ShowSerpentineFoldArrows(
                graphNode->GetProperty(L"serpentine-fold-arrows")->AsBool());
            }

        // sorting
        const auto sortNode = graphNode->GetProperty(L"bar-sort");
        if (sortNode->IsOk())
            {
            // cache that bar-sort was explicitly specified
            barChart->SetPropertyTemplate(L"bar-sort", L"true");
            const auto sortDirection =
                sortNode->GetProperty(L"direction")->AsString().CmpNoCase(_DT(L"ascending")) == 0 ?
                    SortDirection::SortAscending :
                    SortDirection::SortDescending;
            const auto byNode = sortNode->GetProperty(L"by");
            if (byNode->IsOk())
                {
                const auto sortBy =
                    (byNode->AsString().CmpNoCase(L"length") == 0 ?
                         std::optional(Graphs::BarChart::BarSortComparison::SortByBarLength) :
                     byNode->AsString().CmpNoCase(L"label") == 0 ?
                         std::optional(Graphs::BarChart::BarSortComparison::SortByAxisLabel) :
                         std::nullopt);
                if (!sortBy.has_value())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': invalid bar sorting 'by' method."),
                                         byNode->AsString())
                            .ToUTF8());
                    }
                barChart->SortBars(sortBy.value(), sortDirection);
                }
            // or is sorting by a list of labels with a custom order
            else if (const auto labelsNode = sortNode->GetProperty(L"labels");
                     labelsNode->IsOk() && labelsNode->IsValueArray())
                {
                barChart->SortBars(labelsNode->AsStrings(), sortDirection);
                }
            else
                {
                throw std::runtime_error(_(L"Sorting method not defined for bar sort.").ToUTF8());
                }
            }

        const auto numDisplay = ReportEnumConvert::ConvertNumberDisplay(
            graphNode->GetProperty(_DT(L"number-display"))->AsString());
        if (numDisplay)
            {
            barChart->SetNumberDisplay(numDisplay.value());
            }

        // bar shapes
        const auto barShapes = graphNode->GetProperty(L"bar-shapes");
        // if applying one shape to all bars
        if (barShapes->IsOk() && !barShapes->IsValueArray())
            {
            const auto barShape = ReportEnumConvert::ConvertBarShape(barShapes->AsString());
            if (barShape.has_value())
                {
                for (auto& bar : barChart->GetBars())
                    {
                    bar.SetShape(barShape.value());
                    }
                }
            }
        // or assigning shapes to individual bars by axis label
        // (bars not listed keep the default BarShape::Rectangle)
        else if (barShapes->IsOk() && barShapes->IsValueArray())
            {
            for (const auto& entry : barShapes->AsNodes())
                {
                if (!entry->IsOk())
                    {
                    continue;
                    }
                const auto barShape =
                    ReportEnumConvert::ConvertBarShape(entry->GetProperty(L"shape")->AsString());
                if (!barShape.has_value())
                    {
                    continue;
                    }
                const auto barPos =
                    barChart->FindBar(entry->GetProperty(L"axis-label")->AsString());
                if (barPos.has_value())
                    {
                    barChart->GetBars().at(barPos.value()).SetShape(barShape.value());
                    }
                }
            }

        // showcasing
        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            barChart->SetGhostOpacity(
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
            }
        if (const auto showcaseNode = graphNode->GetProperty(L"showcase-bars");
            showcaseNode->IsOk() && showcaseNode->IsValueArray())
            {
            const bool hideGhostedLabels =
                graphNode->GetProperty(L"hide-ghosted-labels")->AsBool(true);
            barChart->ShowcaseBars(
                ExpandAndCache(barChart.get(), L"showcase-bars", showcaseNode->AsStrings()),
                hideGhostedLabels);
            }

        // decals to add to the bars
        const auto decalsNode = graphNode->GetProperty(L"decals");
        if (decalsNode->IsOk() && decalsNode->IsValueArray())
            {
            const auto decals = decalsNode->AsNodes();
            for (const auto& decal : decals)
                {
                const auto barPos = barChart->FindBar(decal->GetProperty(L"bar")->AsString());
                if (barPos.has_value())
                    {
                    const auto blockIndex = decal->GetProperty(L"block")->AsDouble(0);
                    const auto decalLabel =
                        LoadLabel(decal->GetProperty(L"decal"), GraphItems::Label{});
                    if (decalLabel != nullptr &&
                        blockIndex < barChart->GetBars().at(barPos.value()).GetBlocks().size())
                        {
                        barChart->GetBars()
                            .at(barPos.value())
                            .GetBlocks()
                            .at(blockIndex)
                            .SetDecal(*decalLabel);
                        }
                    }
                }
            }

        // bar groups
        const auto barGroupPlacement = ReportEnumConvert::ConvertLabelPlacement(
            graphNode->GetProperty(L"bar-group-placement")->AsString());
        if (barGroupPlacement.has_value())
            {
            barChart->SetBarGroupPlacement(barGroupPlacement.value());
            }

        const auto barGroupsNode = graphNode->GetProperty(L"bar-groups");
        if (barGroupsNode->IsOk() && barGroupsNode->IsValueArray())
            {
            const auto barGroups = barGroupsNode->AsNodes();
            for (const auto& barGroup : barGroups)
                {
                if (barGroup->IsOk())
                    {
                    Graphs::BarChart::BarGroup bGroup;
                    bGroup.m_barColor = (ConvertColor(barGroup->GetProperty(L"color")));
                    if (!bGroup.m_barColor.IsOk() && barChart->GetColorScheme() != nullptr)
                        {
                        bGroup.m_barColor = barChart->GetColorScheme()->GetColor(0);
                        }
                    LoadBrush(barGroup->GetProperty(L"brush"), bGroup.m_barBrush);
                    if (!bGroup.m_barBrush.IsOk() && barChart->GetBrushScheme() != nullptr)
                        {
                        bGroup.m_barBrush = barChart->GetBrushScheme()->GetBrush(0);
                        }
                    bGroup.m_barDecal = barGroup->GetProperty(L"decal")->AsString();

                    if (barGroup->GetProperty(L"start")->IsValueNumber())
                        {
                        bGroup.m_barPositions.first = barGroup->GetProperty(L"start")->AsDouble();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"start")->AsString());
                        if (foundBar.has_value())
                            {
                            bGroup.m_barPositions.first = foundBar.value();
                            }
                        else
                            {
                            throw std::runtime_error(
                                wxString::Format(
                                    _(L"'%s': bar label not found when adding bar group."),
                                    barGroup->GetProperty(L"start")->AsString())
                                    .ToUTF8());
                            }
                        }
                    if (barGroup->GetProperty(L"end")->IsValueNumber())
                        {
                        bGroup.m_barPositions.second = barGroup->GetProperty(L"end")->AsDouble();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"end")->AsString());
                        if (foundBar.has_value())
                            {
                            bGroup.m_barPositions.second = foundBar.value();
                            }
                        else
                            {
                            throw std::runtime_error(
                                wxString::Format(
                                    _(L"'%s': bar label not found when adding bar group."),
                                    barGroup->GetProperty(L"end")->AsString())
                                    .ToUTF8());
                            }
                        }

                    barChart->AddBarGroup(bGroup);
                    }
                }
            }

        // bar brackets
        if (const auto barBracketsNode = graphNode->GetProperty(L"first-bar-brackets");
            barBracketsNode->IsOk() && barBracketsNode->IsValueArray())
            {
            const auto barBrackets = barBracketsNode->AsNodes();
            for (size_t bbi = 0; bbi < barBrackets.size(); ++bbi)
                {
                const auto& barBracket = barBrackets[bbi];
                const auto idx = std::to_wstring(bbi);
                // just log any missing bracket requests and then skip over them
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        const auto startVal =
                            barBracket->GetProperty(L"start-block-re")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block-re")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddFirstBarBracketRE(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].start-block-re", startVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].end-block-re", endVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    else
                        {
                        const auto startVal = barBracket->GetProperty(L"start-block")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddFirstBarBracket(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].start-block", startVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block",
                                                      endVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    }
                catch (const std::exception& err)
                    {
                    wxLogWarning(L"%s", wxString::FromUTF8(err.what()));
                    }
                }
            }
        if (const auto barBracketsNode = graphNode->GetProperty(L"last-bar-brackets");
            barBracketsNode->IsOk() && barBracketsNode->IsValueArray())
            {
            const auto barBrackets = barBracketsNode->AsNodes();
            for (size_t bbi = 0; bbi < barBrackets.size(); ++bbi)
                {
                const auto& barBracket = barBrackets[bbi];
                const auto idx = std::to_wstring(bbi);
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        const auto startVal =
                            barBracket->GetProperty(L"start-block-re")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block-re")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddLastBarBracketRE(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].start-block-re", startVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].end-block-re", endVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    else
                        {
                        const auto startVal = barBracket->GetProperty(L"start-block")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddLastBarBracket(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].start-block", startVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block",
                                                      endVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    }
                catch (const std::exception& err)
                    {
                    wxLogWarning(L"%s", wxString::FromUTF8(err.what()));
                    }
                }
            }

        const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
            graphNode->GetProperty(L"bar-label-display")->AsString());
        if (binLabel.has_value())
            {
            barChart->SetBinLabelDisplay(binLabel.value());
            }

        barChart->SetBinLabelSuffix(graphNode->GetProperty(L"bar-label-suffix")->AsString());

        // bar icons
        const auto barIconsNode = graphNode->GetProperty(L"bar-icons");
        if (barIconsNode->IsOk() && barIconsNode->IsValueArray())
            {
            const auto barIcons = barIconsNode->AsNodes();
            for (const auto& barIcon : barIcons)
                {
                if (barIcon->IsOk())
                    {
                    auto path = barIcon->GetProperty(L"image")->AsString();
                    if (!path.empty())
                        {
                        if (!wxFileName::FileExists(path))
                            {
                            path = wxFileName{ m_configFilePath }.GetPathWithSep() + path;
                            if (!wxFileName::FileExists(path))
                                {
                                throw std::runtime_error(
                                    wxString::Format(_(L"%s: image not found."), path).ToUTF8());
                                }
                            }
                        }

                    barChart->AddBarIcon(barIcon->GetProperty("label")->AsString(),
                                         GraphItems::Image::LoadFile(path));
                    }
                }
            }

        barChart->IncludeSpacesBetweenBars(
            graphNode->GetProperty(L"include-spaces-between-bars")->AsBool(true));

        if (graphNode->HasProperty(L"constrain-scaling-axis-to-bars") &&
            graphNode->GetProperty(L"constrain-scaling-axis-to-bars")->AsBool())
            {
            barChart->ConstrainScalingAxisToBars();
            }

        if (graphNode->GetProperty(L"apply-brushes-to-ungrouped-bars")->AsBool() &&
            !barChart->IsUsingGrouping() && barChart->GetBrushScheme() &&
            !barChart->GetBrushScheme()->GetBrushes().empty())
            {
            barChart->SetApplyBrushesToUngroupedBars(true);
            if (barChart->GetBarOrientation() == Orientation::Vertical)
                {
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (!blocks.empty())
                        {
                        blocks.front().GetBrush() = barChart->GetBrushScheme()->GetBrush(i);
                        }
                    }
                }
            else
                {
                // apply brush in reverse because the origin in going upward, but the client
                // sees the bars as going downward
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (!blocks.empty())
                        {
                        wxASSERT_MSG(barChart->GetBrushScheme()->GetBrushes().size() >= (1 + i),
                                     L"Bad brush mapping for bar chart!");
                        blocks.front().GetBrush() = barChart->GetBrushScheme()->GetBrush(
                            barChart->GetBrushScheme()->GetBrushes().size() - 1 - i);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadCategoricalBarChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                           size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bar chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);
            const auto categoryNameRaw = variablesNode->GetProperty(L"category")->AsString();
            const auto categoryName = ExpandConstants(categoryNameRaw);
            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->AsString());

            auto barChart = std::make_shared<Graphs::CategoricalBarChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!aggVarNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }
            if (!categoryNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.category", categoryNameRaw);
                }

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->AsString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                {
                barChart->SetBarOrientation(Orientation::Horizontal);
                }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                {
                barChart->SetBarOrientation(Orientation::Vertical);
                }

            barChart->SetData(
                foundPos->second, categoryName,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt),
                binLabel.has_value() ? binLabel.value() : BinLabelDisplay::BinValue);

            LoadBarChart(graphNode, barChart);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, barChart);
            return barChart;
            }
        throw std::runtime_error(_(L"Variables not defined for bar chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadPieChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for pie chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupVar1NameRaw = variablesNode->GetProperty(L"group-1")->AsString();
            const auto groupVar1Name = ExpandConstants(groupVar1NameRaw);
            const auto groupVar2NameRaw = variablesNode->GetProperty(L"group-2")->AsString();
            const auto groupVar2Name = ExpandConstants(groupVar2NameRaw);

            auto pieChart = std::make_shared<Graphs::PieChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!aggVarNameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupVar1NameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.group-1", groupVar1NameRaw);
                }
            if (!groupVar2NameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.group-2", groupVar2NameRaw);
                }
            if (groupVar1Name.empty())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: group variable not specified for pie chart."), dsName)
                        .ToUTF8());
                }
            pieChart->SetData(
                foundPos->second,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
                groupVar1Name,
                (!groupVar2Name.empty() ? std::optional<wxString>(groupVar2Name) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"inner-pie-line-pen"),
                    pieChart->GetInnerPieConnectionLinePen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                pieChart->SetLabelPlacement(labelPlacement.value());
                }

            const auto outerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-pie-midpoint-label-display")->AsString());
            if (outerPieMidLabel.has_value())
                {
                pieChart->SetOuterPieMidPointLabelDisplay(outerPieMidLabel.value());
                }

            const auto innerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"inner-pie-midpoint-label-display")->AsString());
            if (innerPieMidLabel.has_value())
                {
                pieChart->SetInnerPieMidPointLabelDisplay(innerPieMidLabel.value());
                }

            const auto outerLabelDisplay = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-label-display")->AsString());
            if (outerLabelDisplay.has_value())
                {
                pieChart->SetOuterLabelDisplay(outerLabelDisplay.value());
                }

            if (graphNode->HasProperty(L"include-inner-pie-labels"))
                {
                pieChart->ShowInnerPieLabels(
                    graphNode->GetProperty(L"include-inner-pie-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"include-outer-pie-labels"))
                {
                pieChart->ShowOuterPieLabels(
                    graphNode->GetProperty(L"include-outer-pie-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"color-labels"))
                {
                pieChart->UseColorLabels(graphNode->GetProperty(L"color-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"ghost-opacity"))
                {
                pieChart->SetGhostOpacity(
                    graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
                }

            // margin notes
            if (graphNode->HasProperty(L"left-margin-note"))
                {
                auto marginLabel =
                    LoadLabel(graphNode->GetProperty(L"left-margin-note"), GraphItems::Label{});
                if (marginLabel != nullptr)
                    {
                    pieChart->GetLeftMarginNote() = *marginLabel;
                    }
                }
            if (graphNode->HasProperty(L"right-margin-note"))
                {
                auto marginLabel =
                    LoadLabel(graphNode->GetProperty(L"right-margin-note"), GraphItems::Label{});
                if (marginLabel != nullptr)
                    {
                    pieChart->GetRightMarginNote() = *marginLabel;
                    }
                }

            if (const auto pieEffect = ReportEnumConvert::ConvertPieSliceEffect(
                    graphNode->GetProperty(L"pie-slice-effect")->AsString());
                pieEffect.has_value())
                {
                pieChart->SetPieSliceEffect(pieEffect.value());
                }

            if (const auto pieStyle = ReportEnumConvert::ConvertPieStyle(
                    graphNode->GetProperty(L"pie-style")->AsString());
                pieStyle.has_value())
                {
                pieChart->SetPieStyle(pieStyle.value());
                }

            pieChart->SetDynamicMargins(graphNode->GetProperty(L"dynamic-margins")->AsBool());

            // showcase the slices
            const auto showcaseNode = graphNode->GetProperty(L"showcase-slices");
            if (showcaseNode->IsValueArray())
                {
                const auto peri = ReportEnumConvert::ConvertPerimeter(
                    graphNode->GetProperty(L"showcased-ring-labels")->AsString());
                pieChart->ShowcaseOuterPieSlices(
                    ExpandAndCache(pieChart.get(), L"showcase-slices", showcaseNode->AsStrings()),
                    peri.has_value() ? peri.value() : Perimeter::Outer);
                }
            else if (showcaseNode->IsOk())
                {
                const auto pieType = showcaseNode->GetProperty(L"pie")->AsString();
                const auto categoryType = showcaseNode->GetProperty(L"category")->AsString();
                const auto peri = ReportEnumConvert::ConvertPerimeter(
                    graphNode->GetProperty(L"showcased-ring-labels")->AsString());
                if (pieType.CmpNoCase(L"inner") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->AsBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->AsBool());
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->AsBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->AsBool());
                        }
                    }
                if (pieType.CmpNoCase(L"outer") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestOuterPieSlices(
                            peri.has_value() ? peri.value() : Perimeter::Outer);
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestOuterPieSlices(
                            peri.has_value() ? peri.value() : Perimeter::Outer);
                        }
                    }
                }

            // donut hole info
            const auto donutHoleNode = graphNode->GetProperty(L"donut-hole");
            if (donutHoleNode->IsOk())
                {
                pieChart->IncludeDonutHole(true);
                const auto labelProperty = donutHoleNode->GetProperty(L"label");
                if (labelProperty->IsOk())
                    {
                    auto holeLabel = LoadLabel(labelProperty, pieChart->GetDonutHoleLabel());
                    if (holeLabel != nullptr)
                        {
                        pieChart->GetDonutHoleLabel() = *holeLabel;
                        }
                    }
                const auto propNode = donutHoleNode->GetProperty(L"proportion");
                if (propNode->IsOk())
                    {
                    pieChart->SetDonutHoleProportion(propNode->AsDouble());
                    }
                const wxColour color(ConvertColor(donutHoleNode->GetProperty(L"color")));
                if (color.IsOk())
                    {
                    pieChart->SetDonutHoleColor(color);
                    }
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pieChart);
            return pieChart;
            }
        throw std::runtime_error(_(L"Variables not defined for pie chart.").ToUTF8());
        }

    //---------------------------------------------------
    void ReportBuilder::LoadGraph(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn,
                                  const std::shared_ptr<Graphs::Graph2D>& graph)
        {
        LoadItem(graphNode, *graph);

        // cache dataset name for round-tripping
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        if (!dsName.empty())
            {
            graph->SetPropertyTemplate(L"dataset", dsName);
            }

        // title information
        const auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            const auto titleLabel = LoadLabel(titleProperty, graph->GetTitle());
            if (titleLabel != nullptr)
                {
                graph->GetTitle() = *titleLabel;
                }
            }

        // subtitle information
        const auto subtitleProperty = graphNode->GetProperty(L"sub-title");
        if (subtitleProperty->IsOk())
            {
            const auto subtitleLabel = LoadLabel(subtitleProperty, graph->GetSubtitle());
            if (subtitleLabel != nullptr)
                {
                graph->GetSubtitle() = *subtitleLabel;
                }
            }

        // caption information
        const auto captionProperty = graphNode->GetProperty(L"caption");
        if (captionProperty->IsOk())
            {
            const auto captionLabel = LoadLabel(captionProperty, graph->GetCaption());
            if (captionLabel != nullptr)
                {
                graph->GetCaption() = *captionLabel;
                }
            }

        // background color
        const auto bgColor = ConvertColor(graphNode->GetProperty(L"background-color"));
        if (bgColor.IsOk())
            {
            graph->SetPlotBackgroundColor(bgColor);
            }

        // background image
        if (const auto bgImgNode = graphNode->GetProperty(L"background-image"); bgImgNode->IsOk())
            {
            const auto importNode = bgImgNode->GetProperty(L"image-import");
            const auto bmp = LoadImageFile(importNode);
            if (bmp.IsOk())
                {
                const auto opacity = static_cast<uint8_t>(
                    bgImgNode->GetProperty(L"opacity")->AsDouble(wxALPHA_OPAQUE));
                graph->SetPlotBackgroundImage(wxBitmapBundle(bmp), opacity);

                // image fit
                const auto fitStr = bgImgNode->GetProperty(L"image-fit")->AsString();
                if (!fitStr.empty())
                    {
                    const auto fit = ReportEnumConvert::ConvertImageFit(fitStr);
                    if (fit.has_value())
                        {
                        graph->SetPlotBackgroundImageFit(fit.value());
                        }
                    }

                // cache property templates for round-tripping
                if (importNode->IsValueString())
                    {
                    graph->SetPropertyTemplate(L"image-import.path", importNode->AsString());
                    }
                else if (importNode->IsOk())
                    {
                    const auto pathStr = importNode->GetProperty(L"path")->AsString();
                    if (!pathStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.path", pathStr);
                        }
                    const auto pathsArr = importNode->GetProperty(L"paths")->AsStrings();
                    if (!pathsArr.empty())
                        {
                        wxString joined;
                        for (size_t i = 0; i < pathsArr.size(); ++i)
                            {
                            if (i > 0)
                                {
                                joined += L"\t";
                                }
                            joined += pathsArr[i];
                            }
                        graph->SetPropertyTemplate(L"image-import.paths", joined);
                        }
                    const auto stitchStr = importNode->GetProperty(L"stitch")->AsString();
                    if (!stitchStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.stitch", stitchStr);
                        }
                    const auto effectStr = importNode->GetProperty(L"effect")->AsString();
                    if (!effectStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.effect", effectStr);
                        }
                    }
                // cache size
                const auto sizeNode = bgImgNode->GetProperty(L"size");
                if (sizeNode->IsOk())
                    {
                    const auto widthVal = sizeNode->GetProperty(L"width")->AsString();
                    if (!widthVal.empty())
                        {
                        graph->SetPropertyTemplate(L"size.width", widthVal);
                        }
                    const auto heightVal = sizeNode->GetProperty(L"height")->AsString();
                    if (!heightVal.empty())
                        {
                        graph->SetPropertyTemplate(L"size.height", heightVal);
                        }
                    }
                }
            }

        // image scheme
        const auto imageSchemeNode = graphNode->GetProperty(L"image-scheme");
        if (imageSchemeNode->IsOk() && imageSchemeNode->IsValueArray())
            {
            std::vector<wxBitmapBundle> images;
            const auto imgNodes = imageSchemeNode->AsNodes();
            images.reserve(imgNodes.size());
            wxString pathsJoined;
            wxString effectStr;
            for (size_t imgIdx = 0; imgIdx < imgNodes.size(); ++imgIdx)
                {
                const auto& imgNode = imgNodes[imgIdx];
                images.emplace_back(LoadImageFile(imgNode));
                // cache file paths for round-tripping; the separator is keyed off
                // the index so leading/all-blank entries survive (an empty entry
                // means "use the brush" for graphs that support null images)
                if (imgIdx > 0)
                    {
                    pathsJoined += L"\t";
                    }
                if (imgNode->IsValueString())
                    {
                    pathsJoined += imgNode->AsString();
                    }
                else
                    {
                    pathsJoined += imgNode->GetProperty(L"path")->AsString();
                    if (effectStr.empty())
                        {
                        effectStr = imgNode->GetProperty(L"effect")->AsString();
                        }
                    }
                }
            graph->SetImageScheme(
                std::make_shared<Images::Schemes::ImageScheme>(std::move(images)));
            if (!pathsJoined.empty())
                {
                graph->SetPropertyTemplate(L"image-paths", pathsJoined);
                }
            if (!effectStr.empty())
                {
                graph->SetPropertyTemplate(L"image-effect", effectStr);
                }
            }

        // common image outline used for bar charts/box plots
        if (graphNode->HasProperty(L"common-box-image-outline"))
            {
            graph->SetCommonBoxImageOutlineColor(
                ConvertColor(graphNode->GetProperty(L"common-box-image-outline")));
            }

        // stipple brush used for bar charts/box plots
        if (const auto stippleImgNode = graphNode->GetProperty(L"stipple-image");
            stippleImgNode->IsOk())
            {
            graph->SetStippleBrush(LoadImageFile(stippleImgNode));
            // cache file path and effect for round-tripping
            wxString stipplePath;
            wxString stippleEffect;
            if (stippleImgNode->IsValueString())
                {
                stipplePath = stippleImgNode->AsString();
                }
            else
                {
                stipplePath = stippleImgNode->GetProperty(L"path")->AsString();
                stippleEffect = stippleImgNode->GetProperty(L"effect")->AsString();
                }
            if (!stipplePath.empty())
                {
                graph->SetPropertyTemplate(L"image-paths", stipplePath);
                }
            if (!stippleEffect.empty())
                {
                graph->SetPropertyTemplate(L"image-effect", stippleEffect);
                }
            }

        if (const auto stippleShapeNode = graphNode->GetProperty(L"stipple-shape");
            stippleShapeNode->IsOk())
            {
            if (stippleShapeNode->IsValueString())
                {
                const auto iconValue = ReportEnumConvert::ConvertIcon(stippleShapeNode->AsString());
                if (iconValue.has_value())
                    {
                    graph->SetStippleShape(iconValue.value());
                    graph->SetPropertyTemplate(L"stipple-shape", stippleShapeNode->AsString());
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for graph stipple shape."),
                                         stippleShapeNode->AsString())
                            .ToUTF8());
                    }
                }
            else
                {
                const auto iconStr = stippleShapeNode->GetProperty(L"icon")->AsString();
                const auto iconValue = ReportEnumConvert::ConvertIcon(iconStr);
                if (iconValue.has_value())
                    {
                    graph->SetStippleShape(iconValue.value());
                    graph->SetPropertyTemplate(L"stipple-shape", iconStr);
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for graph stipple shape."), iconStr)
                            .ToUTF8());
                    }
                if (const auto stippleShapeColor =
                        ConvertColor(stippleShapeNode->GetProperty(L"color"));
                    stippleShapeColor.IsOk())
                    {
                    graph->SetStippleShapeColor(stippleShapeColor);
                    graph->SetPropertyTemplate(L"stipple-shape-color",
                                               stippleShapeColor.GetAsString(wxC2S_HTML_SYNTAX));
                    }
                }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->AsNodes();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    axisNode->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    if (axisType.value() == AxisType::LeftYAxis)
                        {
                        LoadAxis(axisNode, graph->GetLeftYAxis());
                        }
                    else if (axisType.value() == AxisType::RightYAxis)
                        {
                        LoadAxis(axisNode, graph->GetRightYAxis());
                        }
                    else if (axisType.value() == AxisType::BottomXAxis)
                        {
                        LoadAxis(axisNode, graph->GetBottomXAxis());
                        }
                    else if (axisType.value() == AxisType::TopXAxis)
                        {
                        LoadAxis(axisNode, graph->GetTopXAxis());
                        }
                    }
                }
            }

        // annotations embedded on the plot
        const auto annotationNode = graphNode->GetProperty(L"annotations");
        if (annotationNode->IsOk())
            {
            const auto annotations = annotationNode->AsNodes();
            for (const auto& annotation : annotations)
                {
                auto label = LoadLabel(annotation->GetProperty(L"label"), GraphItems::Label{});
                if (!label)
                    {
                    continue;
                    }
                // add outline and background color if not provided in config file
                if (!label->GetPen().IsOk())
                    {
                    label->GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
                    }
                if (!label->GetFontBackgroundColor().IsOk())
                    {
                    label->SetFontBackgroundColor(
                        Colors::ColorContrast::BlackOrWhiteContrast(label->GetFontColor()));
                    }
                label->SetPadding(5, 5, 5, 5);

                const auto interestPointsNode = annotation->GetProperty(L"interest-points");
                if (interestPointsNode->IsOk())
                    {
                    // get all the points on the plot that the note is pointing at
                    std::vector<wxPoint2DDouble> interestPointPositions;
                    const auto interestPoints = interestPointsNode->AsNodes();
                    for (const auto& interestPoint : interestPoints)
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           interestPoint->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           interestPoint->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            interestPointPositions.emplace_back(xPos.value(), yPos.value());
                            }
                        }
                    wxPoint2DDouble anchorPt;
                    const auto anchorNode = annotation->GetProperty(L"anchor");
                    if (anchorNode->IsOk())
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           anchorNode->GetProperty(L"x"));
                        const auto yPos =
                            FindAxisPosition(graph->GetLeftYAxis(), anchorNode->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            anchorPt.m_x = xPos.value();
                            anchorPt.m_y = yPos.value();
                            }
                        }
                    // if no anchor point specified, then use the middle point
                    // of the interest points
                    /// @todo try to add even better logic in here, like how ggrepel works
                    else if (!interestPointPositions.empty())
                        {
                        const auto [minX, maxX] = std::minmax_element(
                            interestPointPositions.cbegin(), interestPointPositions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.m_x < rhv.m_x; });
                        const auto [minY, maxY] = std::minmax_element(
                            interestPointPositions.cbegin(), interestPointPositions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.m_y < rhv.m_y; });
                        anchorPt.m_x = safe_divide(maxX->m_x - minX->m_x, 2.0) + minX->m_x;
                        anchorPt.m_y = safe_divide(maxY->m_y - minY->m_y, 2.0) + minY->m_y;
                        }
                    graph->AddAnnotation(label, anchorPt, interestPointPositions);
                    }
                }
            }

        // reference lines
        const auto referenceLinesNode = graphNode->GetProperty(L"reference-lines");
        if (referenceLinesNode->IsOk())
            {
            const auto refLines = referenceLinesNode->AsNodes();
            for (const auto& refLine : refLines)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refLine->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    wxPen pen{ Colors::ColorBrewer::GetColor(Colors::Color::LightGray), 1,
                               wxPenStyle::wxPENSTYLE_LONG_DASH };
                    LoadPen(refLine->GetProperty(L"pen"), pen);

                    const auto axisPos =
                        FindAxisPosition(graph->GetAxis(axisType.value_or(AxisType::BottomXAxis)),
                                         refLine->GetProperty(L"position"));

                    if (axisPos.has_value())
                        {
                        const auto labelPlacement =
                            ReportEnumConvert::ConvertReferenceLabelPlacement(
                                refLine->GetProperty(L"reference-label-placement")->AsString());
                        graph->AddReferenceLine(GraphItems::ReferenceLine(
                            axisType.value(), axisPos.value(),
                            refLine->GetProperty(L"label")->AsString(), pen,
                            labelPlacement.value_or(ReferenceLabelPlacement::Legend)));
                        }
                    }
                }
            }

        // reference areas
        const auto referenceAreasNode = graphNode->GetProperty(L"reference-areas");
        if (referenceAreasNode->IsOk())
            {
            const auto refAreas = referenceAreasNode->AsNodes();
            for (const auto& refArea : refAreas)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refArea->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    wxPen pen{ Colors::ColorBrewer::GetColor(Colors::Color::LightGray), 1,
                               wxPenStyle::wxPENSTYLE_LONG_DASH };
                    LoadPen(refArea->GetProperty(L"pen"), pen);

                    auto& axis = graph->GetAxis(axisType.value_or(AxisType::BottomXAxis));

                    const ReferenceAreaStyle areaStyle =
                        ReportEnumConvert::ConvertReferenceAreaStyle(
                            refArea->GetProperty(L"style")->AsString())
                            .value_or(ReferenceAreaStyle::Solid);

                    const auto axisPos1 = FindAxisPosition(axis, refArea->GetProperty(L"start"));

                    const auto axisPos2 = FindAxisPosition(axis, refArea->GetProperty(L"end"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        graph->AddReferenceArea(GraphItems::ReferenceArea(
                            axisType.value(), axisPos1.value(), axisPos2.value(),
                            refArea->GetProperty(L"label")->AsString(), pen, areaStyle));
                        }
                    }
                }
            }

        // is there a legend?
        const auto legendNode = graphNode->GetProperty(L"legend");
        if (legendNode->IsOk())
            {
            const bool useEnhancedChernoffLegend =
                (graph->IsKindOf(wxCLASSINFO(Graphs::ChernoffFacesPlot)) &&
                 graph->GetPropertyTemplate(L"enhanced-legend") == L"true");
            auto* chernoffPlot = useEnhancedChernoffLegend ?
                                     dynamic_cast<Graphs::ChernoffFacesPlot*>(graph.get()) :
                                     nullptr;
            const auto ringPerimeterStr = legendNode->GetProperty(L"ring")->AsString();
            const auto ringPerimeter =
                (ringPerimeterStr.CmpNoCase(L"inner") == 0 ? Perimeter::Inner : Perimeter::Outer);
            const auto includeHeader = legendNode->GetProperty(L"include-header")->AsBool(true);
            const auto headerLabel = legendNode->GetProperty(L"title")->AsString();
            const auto placement = legendNode->GetProperty(L"placement")->AsString();

            // a choropleth map has a specialized legend
            auto* choroplethMap = dynamic_cast<Graphs::ChoroplethMap*>(graph.get());
            const bool useChoroplethLegend =
                (choroplethMap != nullptr &&
                 !choroplethMap->GetProportionalSymbolColumnName().empty());

            // places the graph and a non-Label legend object per the placement string
            const auto placeLegendObject = [&](std::unique_ptr<GraphItems::GraphItemBase> legendObj)
            {
                if (legendObj == nullptr)
                    {
                    canvas->SetFixedObject(currentRow, currentColumn, graph);
                    return;
                    }
                if (placement.CmpNoCase(L"left") == 0)
                    {
                    canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                    canvas->SetFixedObject(currentRow, currentColumn++, std::move(legendObj));
                    }
                else if (placement.CmpNoCase(L"bottom") == 0)
                    {
                    canvas->SetFixedObject(currentRow, currentColumn, graph);
                    canvas->SetFixedObject(++currentRow, currentColumn, std::move(legendObj));
                    }
                else if (placement.CmpNoCase(L"top") == 0)
                    {
                    canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                    canvas->SetFixedObject(currentRow++, currentColumn, std::move(legendObj));
                    }
                else
                    {
                    canvas->SetFixedObject(currentRow, currentColumn, graph);
                    canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legendObj));
                    }
            };

            if (useEnhancedChernoffLegend && chernoffPlot != nullptr)
                {
                if (placement.CmpNoCase(L"left") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Left)
                            .PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                        canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"bottom") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Bottom)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"top") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}.Placement(Side::Top).PlacementHint(
                            LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                        canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else // right, the default
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Right)
                            .PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                }
            else if (useChoroplethLegend)
                {
                const auto [choroSide, choroHint] =
                    (placement.CmpNoCase(L"left") == 0) ?
                        std::pair{ Side::Left, LegendCanvasPlacementHint::LeftOfGraph } :
                    (placement.CmpNoCase(L"bottom") == 0) ?
                        std::pair{ Side::Bottom, LegendCanvasPlacementHint::AboveOrBeneathGraph } :
                    (placement.CmpNoCase(L"top") == 0) ?
                        std::pair{ Side::Top, LegendCanvasPlacementHint::AboveOrBeneathGraph } :
                        std::pair{ Side::Right, LegendCanvasPlacementHint::RightOfGraph };
                placeLegendObject(
                    choroplethMap->CreateChoroplethLegend(Graphs::LegendOptions{}
                                                              .RingPerimeter(ringPerimeter)
                                                              .IncludeHeader(includeHeader)
                                                              .Title(headerLabel)
                                                              .Placement(choroSide)
                                                              .PlacementHint(choroHint)));
                }
            else
                {
                if (placement.CmpNoCase(L"left") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Left)
                            .PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                        canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"bottom") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Bottom)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"top") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Top)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                        canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else // right, the default
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Right)
                            .PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                }
            }
        // no legend, so just add the graph
        else
            {
            canvas->SetFixedObject(currentRow, currentColumn, graph);
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadChoroplethMap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        // "region-file" is the key this writes. "kml-file" is the older key, read for
        // projects saved before GeoJSON regions were supported.
        wxString regionFileRaw = graphNode->GetProperty(_DT(L"region-file"))->AsString();
        if (regionFileRaw.empty())
            {
            regionFileRaw = graphNode->GetProperty(_DT(L"kml-file"))->AsString();
            }
        if (regionFileRaw.empty())
            {
            throw std::runtime_error(
                _(L"A region file must be specified for a choropleth map.").ToUTF8());
            }
        // don't allow trying to load external paths
        if (regionFileRaw.StartsWith(L"\\\\") || regionFileRaw.StartsWith(L"//"))
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': network paths are not allowed for region files."),
                                 regionFileRaw)
                    .ToUTF8());
            }
        const wxString regionFile = NormalizeFilePath(regionFileRaw);
        wxString regionIdField = graphNode->GetProperty(_DT(L"region-id-field"))->AsString();
        if (regionIdField.empty())
            {
            regionIdField = graphNode->GetProperty(_DT(L"kml-id-field"))->AsString();
            }

        auto geoData = std::make_shared<Data::GeoDataset>();
        if (!geoData->ImportRegionFile(regionFile, Data::GeoImportInfo().IdField(regionIdField)))
            {
            throw std::runtime_error(
                wxString::Format(L"'%s': %s", regionFile, geoData->GetLastError()).ToUTF8());
            }

        // optional dataset merged in for shading
        wxString dataSourceName;
        wxString dataSourceKeyColumn;
        wxString valueColumn;
        wxString categoryColumn;
        wxString symbolColumn;
        auto dataAggregation = Data::GeoColumnAggregation::Sum;
        if (const auto dataSourceNode = graphNode->GetProperty(_DT(L"data-source"));
            dataSourceNode->IsOk())
            {
            dataSourceName = dataSourceNode->GetProperty(_DT(L"dataset"))->AsString();
            dataSourceKeyColumn = dataSourceNode->GetProperty(_DT(L"key-column"))->AsString();
            valueColumn = dataSourceNode->GetProperty(_DT(L"value-column"))->AsString();
            categoryColumn = dataSourceNode->GetProperty(_DT(L"category-column"))->AsString();
            symbolColumn = dataSourceNode->GetProperty(_DT(L"symbol-column"))->AsString();
            if (const auto aggregation = ReportEnumConvert::ConvertGeoColumnAggregation(
                    dataSourceNode->GetProperty(_DT(L"aggregation"))->AsString());
                aggregation.has_value())
                {
                dataAggregation = aggregation.value();
                }

            const auto foundSource = m_datasets.find(dataSourceName);
            if (foundSource == m_datasets.cend() || foundSource->second == nullptr)
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: dataset not found for choropleth map."),
                                     dataSourceName)
                        .ToUTF8());
                }
            if (!categoryColumn.empty())
                {
                geoData->CopyCategoricalColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                   categoryColumn, categoryColumn);
                }
            else if (!valueColumn.empty())
                {
                geoData->CopyContinuousColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                  valueColumn, valueColumn, dataAggregation);
                }
            if (!symbolColumn.empty() && symbolColumn != valueColumn &&
                symbolColumn != categoryColumn)
                {
                geoData->CopyContinuousColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                  symbolColumn, symbolColumn, dataAggregation);
                }
            }

        const wxString shadingColumn = !categoryColumn.empty() ? categoryColumn : valueColumn;

        auto choroplethMap =
            std::make_shared<Graphs::ChoroplethMap>(canvas, LoadGraphColorScheme(graphNode));

        // classification of the value column, applied before SetData() computes the class colors
        if (const auto classMethodNode = graphNode->GetProperty(_DT(L"classification-method"));
            classMethodNode->IsOk() &&
            classMethodNode->AsString().CmpNoCase(L"jenks-natural-breaks") == 0)
            {
            choroplethMap->SetClassificationMethod(
                Graphs::ChoroplethMap::ClassificationMethod::JenksNaturalBreaks);
            }
        if (const auto classCountNode = graphNode->GetProperty(_DT(L"classification-count"));
            classCountNode->IsOk())
            {
            // screen the JSON value first to avoid a wild or non-finite number
            if (const double rawClassCount = classCountNode->AsDouble(5);
                std::isfinite(rawClassCount) && rawClassCount >= 2.0 && rawClassCount <= 12.0)
                {
                choroplethMap->SetClassCount(static_cast<size_t>(rawClassCount));
                }
            }

        choroplethMap->SetData(
            geoData, shadingColumn.empty() ? std::nullopt : std::optional<wxString>(shadingColumn));
        choroplethMap->SetSourceInfo(regionFile, regionIdField, dataSourceName,
                                     dataSourceKeyColumn);
        choroplethMap->SetDataAggregation(dataAggregation);

        // optional backdrop layer drawn under the data regions
        if (wxString backgroundFileRaw =
                graphNode->GetProperty(_DT(L"background-file"))->AsString();
            !backgroundFileRaw.empty())
            {
            if (backgroundFileRaw.StartsWith(L"\\\\") || backgroundFileRaw.StartsWith(L"//"))
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': network paths are not allowed for region files."),
                                     backgroundFileRaw)
                        .ToUTF8());
                }
            const wxString backgroundFile = NormalizeFilePath(backgroundFileRaw);
            auto backgroundData = std::make_shared<Data::GeoDataset>();
            if (backgroundData->ImportRegionFile(backgroundFile))
                {
                choroplethMap->SetBackgroundLayer(backgroundData);
                }
            else
                {
                wxLogWarning(L"'%s': %s", backgroundFile, backgroundData->GetLastError());
                }
            choroplethMap->SetBackgroundFilePath(backgroundFile);
            }

        if (!symbolColumn.empty())
            {
            choroplethMap->SetProportionalSymbolColumn(symbolColumn);
            }

        if (const auto labelsNode = graphNode->GetProperty(_DT(L"show-region-labels"));
            labelsNode->IsOk())
            {
            choroplethMap->ShowRegionLabels(labelsNode->AsBool());
            }

        if (const auto graticuleNode = graphNode->GetProperty(_DT(L"show-graticule"));
            graticuleNode->IsOk())
            {
            choroplethMap->ShowGraticule(graticuleNode->AsBool());
            }

        if (const auto valuesOnlyNode =
                graphNode->GetProperty(_DT(L"show-only-regions-with-values"));
            valuesOnlyNode->IsOk())
            {
            choroplethMap->ShowOnlyRegionsWithValues(valuesOnlyNode->AsBool());
            }

        if (const auto regionLabelDisplayNode =
                graphNode->GetProperty(_DT(L"region-label-display"));
            regionLabelDisplayNode->IsOk())
            {
            if (const auto labelDisplay =
                    ReportEnumConvert::ConvertBinLabelDisplay(regionLabelDisplayNode->AsString());
                labelDisplay.has_value())
                {
                choroplethMap->SetLabelDisplay(labelDisplay.value());
                }
            }

        const wxString projectionStr = graphNode->GetProperty(_DT(L"projection"))->AsString();
        if (projectionStr.CmpNoCase(L"equirectangular") == 0)
            {
            choroplethMap->SetMapProjection(Graphs::ChoroplethMap::MapProjection::Equirectangular);
            }
        else if (projectionStr.CmpNoCase(L"albers-equal-area-conic") == 0)
            {
            choroplethMap->SetMapProjection(
                Graphs::ChoroplethMap::MapProjection::AlbersEqualAreaConic);
            }
        else if (projectionStr.CmpNoCase(L"equal-earth") == 0)
            {
            choroplethMap->SetMapProjection(Graphs::ChoroplethMap::MapProjection::EqualEarth);
            }

        if (const wxColour noDataColor(ConvertColor(graphNode->GetProperty(_DT(L"no-data-color"))));
            noDataColor.IsOk())
            {
            choroplethMap->SetNoDataColor(noDataColor);
            }

        if (const wxColour symbolColor(ConvertColor(graphNode->GetProperty(_DT(L"symbol-color"))));
            symbolColor.IsOk())
            {
            choroplethMap->SetProportionalSymbolColor(symbolColor);
            }

        if (const auto noDataFillStyleNode = graphNode->GetProperty(_DT(L"no-data-fill-style"));
            noDataFillStyleNode->IsOk())
            {
            if (const auto fillStyle =
                    ReportEnumConvert::ConvertBrushStyle(noDataFillStyleNode->AsString());
                fillStyle.has_value())
                {
                choroplethMap->SetNoDataFillStyle(fillStyle.value());
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, choroplethMap);
        return choroplethMap;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadNightingaleRoseChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                            size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Nightingale rose chart."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(
                _(L"Variables not defined for Nightingale rose chart.").ToUTF8());
            }

        const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
        const auto aggVarName = ExpandConstants(aggVarNameRaw);
        const auto categoryVarNameRaw = variablesNode->GetProperty(L"category")->AsString();
        const auto categoryVarName = ExpandConstants(categoryVarNameRaw);
        const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
        const auto groupVarName = ExpandConstants(groupVarNameRaw);

        if (categoryVarName.empty())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"%s: category variable not specified for Nightingale rose chart."), dsName)
                    .ToUTF8());
            }

        auto roseChart = std::make_shared<Graphs::NightingaleRoseChart>(
            canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
            LoadGraphColorScheme(graphNode));
        if (!aggVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
            }
        if (!categoryVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.category", categoryVarNameRaw);
            }
        if (!groupVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
            }

        roseChart->SetData(
            foundPos->second,
            (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
            categoryVarName,
            (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

        if (const auto radialScaling = ReportEnumConvert::ConvertNightingaleRoseRadialScaling(
                graphNode->GetProperty(L"radial-scaling")->AsString());
            radialScaling.has_value())
            {
            roseChart->SetRadialScaling(radialScaling.value());
            }

        if (const auto seriesDisplay = ReportEnumConvert::ConvertNightingaleRoseSeriesDisplay(
                graphNode->GetProperty(L"series-display")->AsString());
            seriesDisplay.has_value())
            {
            roseChart->SetSeriesDisplay(seriesDisplay.value());
            }

        if (graphNode->HasProperty(L"start-angle"))
            {
            roseChart->SetStartAngle(graphNode->GetProperty(L"start-angle")->AsDouble(90.0));
            }

        if (graphNode->HasProperty(L"show-labels"))
            {
            roseChart->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool());
            }

        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            const double rawGhostOpacity =
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY);
            roseChart->SetGhostOpacity(static_cast<uint8_t>(std::clamp(
                std::isfinite(rawGhostOpacity) ? rawGhostOpacity :
                                                 static_cast<double>(Settings::GHOST_OPACITY),
                0.0, 255.0)));
            }

        if (const auto ghostedWedgesNode = graphNode->GetProperty(L"ghosted-wedges");
            ghostedWedgesNode->IsOk() && ghostedWedgesNode->IsValueArray())
            {
            for (const auto& ghostNode : ghostedWedgesNode->AsNodes())
                {
                const auto groupLabel =
                    ExpandConstants(ghostNode->GetProperty(L"group")->AsString());
                if (groupLabel.empty())
                    {
                    continue;
                    }
                roseChart->GhostWedge(
                    groupLabel, ExpandConstants(ghostNode->GetProperty(L"category")->AsString()));
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, roseChart);
        return roseChart;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadBulletChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bullet chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(_(L"Variables not defined for bullet chart.").ToUTF8());
            }

        const auto labelVarNameRaw = variablesNode->GetProperty(L"label")->AsString();
        const auto labelVarName = ExpandConstants(labelVarNameRaw);
        const auto actualVarNameRaw = variablesNode->GetProperty(L"actual")->AsString();
        const auto actualVarName = ExpandConstants(actualVarNameRaw);
        const auto targetVarNameRaw = variablesNode->GetProperty(L"target")->AsString();
        const auto targetVarName = ExpandConstants(targetVarNameRaw);

        if (labelVarName.empty() || actualVarName.empty() || targetVarName.empty())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: label, actual, and target variables must be specified for "
                                   "bullet chart."),
                                 dsName)
                    .ToUTF8());
            }

        auto bulletChart = std::make_shared<Graphs::BulletChart>(canvas);
        bulletChart->SetPropertyTemplate(L"dataset", dsName);
        bulletChart->SetPropertyTemplate(L"variables.label", labelVarNameRaw);
        bulletChart->SetPropertyTemplate(L"variables.actual", actualVarNameRaw);
        bulletChart->SetPropertyTemplate(L"variables.target", targetVarNameRaw);

        if (const auto colorScheme = ReportEnumConvert::ConvertBulletChartRangeColorScheme(
                graphNode->GetProperty(L"range-color-scheme")->AsString());
            colorScheme.has_value())
            {
            bulletChart->SetRangeColorScheme(colorScheme.value());
            }

        if (const auto color = ConvertColor(graphNode->GetProperty(L"range-start-color"));
            color.IsOk())
            {
            bulletChart->SetRangeStartColor(color);
            }
        if (const auto color = ConvertColor(graphNode->GetProperty(L"range-end-color"));
            color.IsOk())
            {
            bulletChart->SetRangeEndColor(color);
            }
        if (const auto color = ConvertColor(graphNode->GetProperty(L"goal-success-color"));
            color.IsOk())
            {
            bulletChart->SetGoalSuccessColor(color);
            }
        if (const auto color = ConvertColor(graphNode->GetProperty(L"goal-failure-color"));
            color.IsOk())
            {
            bulletChart->SetGoalFailureColor(color);
            }

        if (const auto valueFormat = ReportEnumConvert::ConvertBulletChartValueFormat(
                graphNode->GetProperty(L"value-display-format")->AsString());
            valueFormat.has_value())
            {
            bulletChart->SetValueDisplayFormat(valueFormat.value());
            }

        if (graphNode->HasProperty(L"show-value-callouts"))
            {
            bulletChart->ShowValueCallouts(
                graphNode->GetProperty(L"show-value-callouts")->AsBool());
            }
        if (graphNode->HasProperty(L"show-range-labels"))
            {
            bulletChart->ShowRangeLabels(graphNode->GetProperty(L"show-range-labels")->AsBool());
            }

        // ranges
        if (const auto rangesNode = graphNode->GetProperty(L"ranges"); rangesNode->IsOk())
            {
            std::vector<Graphs::BulletChart::Range> ranges;
            const auto rangeNodes = rangesNode->AsNodes();
            ranges.reserve(rangeNodes.size());
            for (const auto& rangeNode : rangeNodes)
                {
                ranges.emplace_back(Graphs::BulletChart::Range{
                    rangeNode->GetProperty(L"end")->AsDouble(0),
                    ExpandConstants(rangeNode->GetProperty(L"label")->AsString()) });
                }
            bulletChart->SetRanges(std::move(ranges));
            }

        bulletChart->SetData(foundPos->second, labelVarName, actualVarName, targetVarName);

        LoadGraph(graphNode, canvas, currentRow, currentColumn, bulletChart);
        return bulletChart;
        }
    } // namespace Wisteria
