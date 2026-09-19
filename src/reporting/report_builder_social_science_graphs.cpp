///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_social_science_graphs.cpp
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
    ReportBuilder::LoadWCurvePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for W-curve plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto wcurvePlot = std::make_shared<Graphs::WCurvePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                wcurvePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            wcurvePlot->SetData(
                foundPos->second,
                ExpandAndCache(wcurvePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(wcurvePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));
            if (graphNode->HasProperty(L"time-interval-label"))
                {
                wcurvePlot->SetTimeIntervalLabel(
                    graphNode->GetProperty(L"time-interval-label")->AsString());
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, wcurvePlot);
            LoadLinePlotBaseOptions(graphNode, wcurvePlot.get());

            return wcurvePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for W-curve plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadProConRoadmap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Pro & Con Roadmap."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto pcRoadmap = std::make_shared<Graphs::ProConRoadmap>(canvas);
            pcRoadmap->SetData(
                foundPos->second, variablesNode->GetProperty(L"positive")->AsString(),
                (variablesNode->HasProperty(L"positive-aggregate") ?
                     std::optional<wxString>(ExpandAndCache(
                         pcRoadmap.get(), L"variables.positive-aggregate",
                         variablesNode->GetProperty(L"positive-aggregate")->AsString())) :
                     std::nullopt),
                variablesNode->GetProperty(L"negative")->AsString(),
                (variablesNode->HasProperty(L"negative-aggregate") ?
                     std::optional<wxString>(ExpandAndCache(
                         pcRoadmap.get(), L"variables.negative-aggregate",
                         variablesNode->GetProperty(L"negative-aggregate")->AsString())) :
                     std::nullopt),
                (graphNode->GetProperty(L"minimum-count")->IsValueNumber() ?
                     std::optional<double>(graphNode->GetProperty(L"minimum-count")->AsDouble(1)) :
                     std::nullopt));

            if (graphNode->GetProperty(L"positive-legend-label")->IsValueString())
                {
                pcRoadmap->SetPositiveLegendLabel(
                    graphNode->GetProperty(L"positive-legend-label")->AsString());
                }
            if (graphNode->GetProperty(L"negative-legend-label")->IsValueString())
                {
                pcRoadmap->SetNegativeLegendLabel(
                    graphNode->GetProperty(L"negative-legend-label")->AsString());
                }

            LoadPen(graphNode->GetProperty(L"road-pen"), pcRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"),
                    pcRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                pcRoadmap->SetLabelPlacement(labelPlacement.value());
                }

            const auto laneSepStyle = ReportEnumConvert::ConvertLaneSeparatorStyle(
                graphNode->GetProperty(L"lane-separator-style")->AsString());
            if (laneSepStyle.has_value())
                {
                pcRoadmap->SetLaneSeparatorStyle(laneSepStyle.value());
                }

            const auto roadStopTheme = ReportEnumConvert::ConvertRoadStopTheme(
                graphNode->GetProperty(L"road-stop-theme")->AsString());
            if (roadStopTheme.has_value())
                {
                pcRoadmap->SetRoadStopTheme(roadStopTheme.value());
                }

            const auto markerLabelDisplay = ReportEnumConvert::ConvertMarkerLabelDisplay(
                graphNode->GetProperty(L"marker-label-display")->AsString());
            if (markerLabelDisplay.has_value())
                {
                pcRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value());
                }

            if (graphNode->GetProperty(L"default-caption")->AsBool())
                {
                pcRoadmap->AddDefaultCaption();
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pcRoadmap);
            return pcRoadmap;
            }

        throw std::runtime_error(_(L"Variables not defined for Pro & Con Roadmap.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLRRoadmap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Linear Regression Roadmap."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            // lrRoadmap created below; cache after creation
            const auto pValueColumnRaw = variablesNode->GetProperty(L"p-value")->AsString();
            const auto pValueColumn = ExpandConstants(pValueColumnRaw);
            const auto dvNameRaw =
                variablesNode->GetProperty(L"dependent-variable-name")->AsString();
            const auto dvName = ExpandConstants(dvNameRaw);

            int lrPredictors{ 0 };
            if (graphNode->HasProperty(L"predictors-to-include"))
                {
                const auto preds = graphNode->GetProperty(L"predictors-to-include")->AsStrings();
                for (const auto& pred : preds)
                    {
                    if (pred.CmpNoCase(_DT(L"positive")) == 0)
                        {
                        lrPredictors |= Influence::InfluencePositive;
                        }
                    else if (pred.CmpNoCase(_DT(L"negative")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceNegative;
                        }
                    else if (pred.CmpNoCase(_DT(L"neutral")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceNeutral;
                        }
                    else if (pred.CmpNoCase(_DT(L"all")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceAll;
                        }
                    }
                }

            auto lrRoadmap = std::make_shared<Graphs::LRRoadmap>(canvas);
            // cache templates for variables expanded before lrRoadmap was created
            if (!pValueColumnRaw.empty())
                {
                lrRoadmap->SetPropertyTemplate(L"variables.p-value", pValueColumnRaw);
                }
            if (!dvNameRaw.empty())
                {
                lrRoadmap->SetPropertyTemplate(L"variables.dependent-variable-name", dvNameRaw);
                }
            lrRoadmap->SetData(
                foundPos->second,
                ExpandAndCache(lrRoadmap.get(), L"variables.predictor",
                               variablesNode->GetProperty(L"predictor")->AsString()),
                ExpandAndCache(lrRoadmap.get(), L"variables.coefficient",
                               variablesNode->GetProperty(L"coefficient")->AsString()),
                (!pValueColumn.empty() ? std::optional<wxString>(pValueColumn) : std::nullopt),
                (graphNode->GetProperty(L"p-value-threshold")->IsValueNumber() ?
                     std::optional<double>(
                         graphNode->GetProperty(L"p-value-threshold")->AsDouble(0.05)) :
                     std::nullopt),
                (lrPredictors == 0 ?
                     std::nullopt :
                     std::optional<Influence>(static_cast<Influence>(lrPredictors))),
                (!dvName.empty() ? std::optional<wxString>(dvName) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"road-pen"), lrRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"),
                    lrRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                lrRoadmap->SetLabelPlacement(labelPlacement.value());
                }

            const auto laneSepStyle = ReportEnumConvert::ConvertLaneSeparatorStyle(
                graphNode->GetProperty(L"lane-separator-style")->AsString());
            if (laneSepStyle.has_value())
                {
                lrRoadmap->SetLaneSeparatorStyle(laneSepStyle.value());
                }

            const auto roadStopTheme = ReportEnumConvert::ConvertRoadStopTheme(
                graphNode->GetProperty(L"road-stop-theme")->AsString());
            if (roadStopTheme.has_value())
                {
                lrRoadmap->SetRoadStopTheme(roadStopTheme.value());
                }

            const auto markerLabelDisplay = ReportEnumConvert::ConvertMarkerLabelDisplay(
                graphNode->GetProperty(L"marker-label-display")->AsString());
            if (markerLabelDisplay.has_value())
                {
                lrRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value());
                }

            if (graphNode->GetProperty(L"default-caption")->AsBool())
                {
                lrRoadmap->AddDefaultCaption();
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, lrRoadmap);
            return lrRoadmap;
            }

        throw std::runtime_error(
            _(L"Variables not defined for Linear Regression Roadmap.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLikertChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Likert chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            std::vector<wxString> questions;
            const auto questionVarsRaw = variablesNode->GetProperty(L"questions")->AsStrings();
            const auto questionVars = ExpandConstants(questionVarsRaw);
            for (const auto& questionVar : questionVars)
                {
                if (auto convertedVars = ExpandColumnSelections(questionVar, foundPos->second))
                    {
                    questions.insert(questions.cend(), convertedVars.value().cbegin(),
                                     convertedVars.value().cend());
                    }
                else
                    {
                    questions.push_back(questionVar);
                    }
                }
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            // get the survey format
            auto surveyFormat = ReportEnumConvert::ConvertLikertSurveyQuestionFormat(
                graphNode->GetProperty(L"survey-format")->AsString());
            if (!surveyFormat.has_value())
                {
                surveyFormat =
                    Graphs::LikertChart::DeduceScale(foundPos->second, questions, groupVarName);
                }

            const bool doSimplify = graphNode->GetProperty(L"simplify")->AsBool();
            if (doSimplify)
                {
                surveyFormat = Graphs::LikertChart::Simplify(foundPos->second, questions,
                                                             surveyFormat.value());
                }

            const bool doApplyLabels = graphNode->GetProperty(L"apply-default-labels")->AsBool();
            if (doApplyLabels)
                {
                Graphs::LikertChart::SetLabels(
                    foundPos->second, questions,
                    Graphs::LikertChart::CreateLabels(surveyFormat.value()));
                }

            auto likertChart = std::make_shared<Graphs::LikertChart>(
                canvas, surveyFormat.value(),
                ConvertColor(graphNode->GetProperty(L"negative-color")),
                ConvertColor(graphNode->GetProperty(L"positive-color")),
                ConvertColor(graphNode->GetProperty(L"neutral-color")),
                ConvertColor(graphNode->GetProperty(L"no-response-color")));

            // cache templates for variables expanded before likertChart was created
            for (size_t i = 0; i < questionVarsRaw.size(); ++i)
                {
                if (!questionVarsRaw[i].empty())
                    {
                    likertChart->SetPropertyTemplate(
                        L"variables.questions[" + std::to_wstring(i) + L"]", questionVarsRaw[i]);
                    }
                }
            if (!groupVarNameRaw.empty())
                {
                likertChart->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            if (doSimplify)
                {
                likertChart->SetPropertyTemplate(L"simplify", L"true");
                }
            if (doApplyLabels)
                {
                likertChart->SetPropertyTemplate(L"apply-default-labels", L"true");
                }

            likertChart->SetData(
                foundPos->second, questions,
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            likertChart->ShowResponseCounts(
                graphNode->GetProperty(L"show-response-counts")->AsBool(false));
            likertChart->ShowPercentages(graphNode->GetProperty(L"show-percentages")->AsBool(true));
            likertChart->ShowSectionHeaders(
                graphNode->GetProperty(L"show-section-headers")->AsBool(true));
            likertChart->SetBarSizesToRespondentSize(
                graphNode->GetProperty(L"adjust-bar-widths-to-respondent-size")->AsBool(false));

            if (graphNode->HasProperty(L"positive-label"))
                {
                likertChart->SetPositiveHeader(
                    graphNode->GetProperty(L"positive-label")->AsString());
                }
            if (graphNode->HasProperty(L"negative-label"))
                {
                likertChart->SetNegativeHeader(
                    graphNode->GetProperty(L"negative-label")->AsString());
                }
            if (graphNode->HasProperty(L"no-response-label"))
                {
                likertChart->SetNoResponseHeader(
                    graphNode->GetProperty(L"no-response-label")->AsString());
                }

            const auto questionBracketNodes =
                graphNode->GetProperty(L"question-brackets")->AsNodes();
            for (const auto& questionBracketNode : questionBracketNodes)
                {
                likertChart->AddQuestionsBracket(
                    { questionBracketNode->GetProperty(L"start")->AsString(),
                      questionBracketNode->GetProperty(L"end")->AsString(),
                      questionBracketNode->GetProperty(L"title")->AsString() });
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, likertChart);
            return likertChart;
            }

        throw std::runtime_error(_(L"Variables not defined for Likert chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWordCloud(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for word cloud."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto wordColNameRaw = variablesNode->GetProperty(L"words")->AsString();
            const auto wordColName = ExpandConstants(wordColNameRaw);

            auto wordCloud =
                std::make_shared<Graphs::WordCloud>(canvas, LoadGraphColorScheme(graphNode));
            if (!aggVarNameRaw.empty())
                {
                wordCloud->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!wordColNameRaw.empty())
                {
                wordCloud->SetPropertyTemplate(L"variables.words", wordColNameRaw);
                }

            wordCloud->SetData(
                foundPos->second, wordColName,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, wordCloud);
            return wordCloud;
            }
        throw std::runtime_error(_(L"Variables not defined for word cloud.").ToUTF8());
        }
    } // namespace Wisteria
