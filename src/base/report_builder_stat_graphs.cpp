///////////////////////////////////////////////////////////////////////////////
// Name:        reportbuilder_stat_graphs.cpp
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
    ReportBuilder::LoadBubblePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bubble plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(_DT(L"group"))->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto bubblePlot = std::make_shared<Graphs::BubblePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"regression-line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                bubblePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            bubblePlot->SetData(
                foundPos->second,
                ExpandAndCache(bubblePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(bubblePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                ExpandAndCache(bubblePlot.get(), L"variables.size",
                               variablesNode->GetProperty(_DT(L"size"))->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            // bubble size options
            if (graphNode->HasProperty(L"min-bubble-radius"))
                {
                bubblePlot->SetMinBubbleRadius(
                    static_cast<size_t>(graphNode->GetProperty(L"min-bubble-radius")->AsDouble(4)));
                }
            if (graphNode->HasProperty(L"max-bubble-radius"))
                {
                bubblePlot->SetMaxBubbleRadius(static_cast<size_t>(
                    graphNode->GetProperty(L"max-bubble-radius")->AsDouble(30)));
                }

            // regression line options (inherited from ScatterPlot)
            if (graphNode->HasProperty(L"show-regression-lines"))
                {
                bubblePlot->ShowRegressionLines(
                    graphNode->GetProperty(L"show-regression-lines")->AsBool(true));
                }
            if (graphNode->HasProperty(L"show-confidence-bands"))
                {
                bubblePlot->ShowConfidenceBands(
                    graphNode->GetProperty(L"show-confidence-bands")->AsBool(true));
                }
            if (graphNode->HasProperty(L"confidence-level"))
                {
                bubblePlot->SetConfidenceLevel(
                    graphNode->GetProperty(L"confidence-level")->AsDouble(0.95));
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, bubblePlot);
            return bubblePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for bubble plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadChernoffFaces(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Chernoff faces."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            // chernoffPlot created below; lambda captures pointer after creation
            std::shared_ptr<Graphs::ChernoffFacesPlot> chernoffPlot;
            const auto getOptionalColumn = [this, &variablesNode, &chernoffPlot](
                                               const wxString& propName) -> std::optional<wxString>
            {
                const wxString rawValue = variablesNode->GetProperty(propName)->AsString();
                const wxString colName = ExpandConstants(rawValue);
                if (!rawValue.empty() && chernoffPlot)
                    {
                    chernoffPlot->SetPropertyTemplate(L"variables." + propName, rawValue);
                    }
                return (!colName.empty() ? std::optional<wxString>(colName) : std::nullopt);
            };

            // face color (optional)
            wxColour faceColor{ 255, 224, 189 }; // default flesh tone
            if (graphNode->HasProperty(L"face-color"))
                {
                faceColor = ConvertColor(graphNode->GetProperty(L"face-color"));
                }

            chernoffPlot = std::make_shared<Graphs::ChernoffFacesPlot>(canvas, faceColor);

            chernoffPlot->SetData(
                foundPos->second,
                ExpandAndCache(chernoffPlot.get(), L"variables.face-width",
                               variablesNode->GetProperty(L"face-width")->AsString()),
                getOptionalColumn(L"face-height"), getOptionalColumn(L"eye-size"),
                getOptionalColumn(L"eye-position"), getOptionalColumn(L"eyebrow-slant"),
                getOptionalColumn(L"pupil-position"), getOptionalColumn(L"nose-size"),
                getOptionalColumn(L"mouth-width"), getOptionalColumn(L"mouth-curvature"),
                getOptionalColumn(L"face-saturation"), getOptionalColumn(L"ear-size"),
                getOptionalColumn(L"hair-style"), getOptionalColumn(L"hair-addition"));

            // appearance options
            if (graphNode->HasProperty(L"show-labels"))
                {
                chernoffPlot->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool(true));
                }
            if (graphNode->HasProperty(L"outline-color"))
                {
                chernoffPlot->SetOutlineColor(
                    ConvertColor(graphNode->GetProperty(L"outline-color")));
                }
            if (graphNode->HasProperty(L"gender"))
                {
                const auto gender =
                    ReportEnumConvert::ConvertGender(graphNode->GetProperty(L"gender")->AsString());
                if (gender.has_value())
                    {
                    chernoffPlot->SetGender(gender.value());
                    }
                }
            if (graphNode->HasProperty(L"eye-color"))
                {
                chernoffPlot->SetEyeColor(ConvertColor(graphNode->GetProperty(L"eye-color")));
                }
            if (graphNode->HasProperty(L"hair-color"))
                {
                chernoffPlot->SetHairColor(ConvertColor(graphNode->GetProperty(L"hair-color")));
                }
            if (graphNode->HasProperty(L"lipstick-color"))
                {
                chernoffPlot->SetLipstickColor(
                    ConvertColor(graphNode->GetProperty(L"lipstick-color")));
                }
            if (graphNode->HasProperty(L"hair-style"))
                {
                const auto hairStyleStr = graphNode->GetProperty(L"hair-style")->AsString();
                if (chernoffPlot->GetGender() == Gender::Male)
                    {
                    const auto hairStyle = ReportEnumConvert::ConvertHairStyleMale(hairStyleStr);
                    if (hairStyle.has_value())
                        {
                        chernoffPlot->SetHairStyle(hairStyle.value());
                        }
                    }
                else
                    {
                    const auto hairStyle = ReportEnumConvert::ConvertHairStyleFemale(hairStyleStr);
                    if (hairStyle.has_value())
                        {
                        chernoffPlot->SetHairStyle(hairStyle.value());
                        }
                    }
                }
            chernoffPlot->SetPropertyTemplate(
                L"enhanced-legend",
                graphNode->GetProperty(L"enhanced-legend")->AsBool(true) ? L"true" : L"false");

            LoadGraph(graphNode, canvas, currentRow, currentColumn, chernoffPlot);
            return chernoffPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for Chernoff faces.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadScatterPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for scatter plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(_DT(L"group"))->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto scatterPlot = std::make_shared<Graphs::ScatterPlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"regression-line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                scatterPlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            scatterPlot->SetData(
                foundPos->second,
                ExpandAndCache(scatterPlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(scatterPlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            // regression line options
            if (graphNode->HasProperty(L"show-regression-lines"))
                {
                scatterPlot->ShowRegressionLines(
                    graphNode->GetProperty(L"show-regression-lines")->AsBool(true));
                }
            if (graphNode->HasProperty(L"show-confidence-bands"))
                {
                scatterPlot->ShowConfidenceBands(
                    graphNode->GetProperty(L"show-confidence-bands")->AsBool(true));
                }
            if (graphNode->HasProperty(L"confidence-level"))
                {
                scatterPlot->SetConfidenceLevel(
                    graphNode->GetProperty(L"confidence-level")->AsDouble(0.95));
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, scatterPlot);
            return scatterPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for scatter plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadHeatMap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                               size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for heatmap."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto heatmap =
                std::make_shared<Graphs::HeatMap>(canvas, LoadGraphColorScheme(graphNode));
            if (!groupVarNameRaw.empty())
                {
                heatmap->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            heatmap->SetData(
                foundPos->second,
                ExpandAndCache(heatmap.get(), L"variables.continuous",
                               variablesNode->GetProperty(L"continuous")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt),
                graphNode->GetProperty(L"group-column-count")->AsDouble(5));

            heatmap->ShowGroupHeaders(graphNode->GetProperty(L"show-group-header")->AsBool(true));
            heatmap->SetGroupHeaderPrefix(
                graphNode->GetProperty(L"group-header-prefix")->AsString());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, heatmap);
            return heatmap;
            }

        throw std::runtime_error(_(L"Variables not defined for heatmap.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadBoxPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                               size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for box plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupVar1NameRaw = variablesNode->GetProperty(L"group-1")->AsString();
            const auto groupVar1Name = ExpandConstants(groupVar1NameRaw);

            auto boxPlot = std::make_shared<Graphs::BoxPlot>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
            if (!aggVarNameRaw.empty())
                {
                boxPlot->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupVar1NameRaw.empty())
                {
                boxPlot->SetPropertyTemplate(L"variables.group-1", groupVar1NameRaw);
                }
            boxPlot->SetData(
                foundPos->second, aggVarName,
                (!groupVar1Name.empty() ? std::optional<wxString>(groupVar1Name) : std::nullopt));

            const auto boxEffect = ReportEnumConvert::ConvertBoxEffect(
                graphNode->GetProperty(L"box-effect")->AsString());
            if (boxEffect)
                {
                boxPlot->SetBoxEffect(boxEffect.value());
                }

            boxPlot->ShowAllPoints(graphNode->GetProperty(L"show-all-points")->AsBool());
            boxPlot->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool());
            const auto showMidpointNode = graphNode->GetProperty(L"show-midpoint-connection");
            if (showMidpointNode->IsOk())
                {
                boxPlot->ShowMidpointConnection(showMidpointNode->AsBool());
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, boxPlot);
            return boxPlot;
            }
        throw std::runtime_error(_(L"Variables not defined for box plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadHistogram(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for histogram."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto contVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto contVarName = ExpandConstants(contVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);

            const auto binMethod = ReportEnumConvert::ConvertBinningMethod(
                graphNode->GetProperty(L"binning-method")->AsString());

            const auto binIntervalDisplay = ReportEnumConvert::ConvertIntervalDisplay(
                graphNode->GetProperty(L"interval-display")->AsString());

            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->AsString());

            const auto rounding = ReportEnumConvert::ConvertRoundingMethod(
                graphNode->GetProperty(L"rounding")->AsString());

            const std::optional<double> startBinsValue =
                graphNode->HasProperty(L"bins-start") ?
                    std::optional<double>(graphNode->GetProperty(L"bins-start")->AsDouble()) :
                    std::nullopt;
            const std::optional<size_t> suggestedBinCount =
                graphNode->HasProperty(L"suggested-bin-count") ?
                    std::optional<double>(
                        graphNode->GetProperty(L"suggested-bin-count")->AsDouble()) :
                    std::nullopt;
            const std::optional<size_t> maxBinCount =
                graphNode->HasProperty(L"max-bin-count") ?
                    std::optional<double>(graphNode->GetProperty(L"max-bin-count")->AsDouble()) :
                    std::nullopt;

            auto histo = std::make_shared<Graphs::Histogram>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!contVarNameRaw.empty())
                {
                histo->SetPropertyTemplate(L"variables.aggregate", contVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                histo->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->AsString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                {
                histo->SetBarOrientation(Orientation::Horizontal);
                }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                {
                histo->SetBarOrientation(Orientation::Vertical);
                }

            histo->SetData(
                foundPos->second, contVarName,
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt),
                binMethod.value_or(Graphs::Histogram::BinningMethod::BinByIntegerRange),
                rounding.value_or(RoundingMethod::NoRounding),
                binIntervalDisplay.value_or(Graphs::Histogram::IntervalDisplay::Cutpoints),
                binLabel.value_or(BinLabelDisplay::BinValue),
                graphNode->GetProperty(L"show-full-range")->AsBool(true), startBinsValue,
                std::make_pair(suggestedBinCount, maxBinCount));

            LoadBarChart(graphNode, histo);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, histo);
            return histo;
            }

        throw std::runtime_error(_(L"Variables not defined for histogram.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWilmarthBridgePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                          size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Wilmarth bridge plot."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(
                _(L"Variables not defined for Wilmarth bridge plot.").ToUTF8());
            }

        auto bridgePlot = std::make_shared<Graphs::WilmarthBridgePlot>(canvas);

        bridgePlot->SetData(foundPos->second,
                            ExpandAndCache(bridgePlot.get(), L"variables.label",
                                           variablesNode->GetProperty(L"label")->AsString()),
                            ExpandAndCache(bridgePlot.get(), L"variables.exit",
                                           variablesNode->GetProperty(L"exit")->AsString()),
                            (variablesNode->HasProperty(L"entered") ?
                                 std::optional<wxString>(ExpandAndCache(
                                     bridgePlot.get(), L"variables.entered",
                                     variablesNode->GetProperty(L"entered")->AsString())) :
                                 std::nullopt),
                            (variablesNode->HasProperty(L"status") ?
                                 std::optional<wxString>(ExpandAndCache(
                                     bridgePlot.get(), L"variables.status",
                                     variablesNode->GetProperty(L"status")->AsString())) :
                                 std::nullopt));

        if (const auto fadeEffect = ReportEnumConvert::ConvertWilmarthBridgeFadeEffect(
                graphNode->GetProperty(L"fade-effect")->AsString());
            fadeEffect.has_value())
            {
            bridgePlot->SetFadeEffect(fadeEffect.value());
            }

        if (const auto survivalDisplay = ReportEnumConvert::ConvertWilmarthBridgeSurvivalDisplay(
                graphNode->GetProperty(L"survival-display")->AsString());
            survivalDisplay.has_value())
            {
            bridgePlot->SetSurvivalDisplay(survivalDisplay.value());
            }

        if (graphNode->HasProperty(L"show-censored-markers"))
            {
            bridgePlot->ShowCensoredMarkers(
                graphNode->GetProperty(L"show-censored-markers")->AsBool());
            }

        if (graphNode->HasProperty(L"terminal-row-label"))
            {
            bridgePlot->ShowTerminalRow(
                ExpandAndCache(bridgePlot.get(), L"terminal-row-label",
                               graphNode->GetProperty(L"terminal-row-label")->AsString()));
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, bridgePlot);
        return bridgePlot;
        }
    } // namespace Wisteria
