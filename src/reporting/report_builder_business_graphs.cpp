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
    ReportBuilder::LoadWaterfallChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                      size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for waterfall chart."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(_(L"Variables not defined for waterfall chart.").ToUTF8());
            }

        const auto labelVarNameRaw = variablesNode->GetProperty(L"label")->AsString();
        const auto valueVarNameRaw = variablesNode->GetProperty(L"value")->AsString();
        const auto totalFlagVarNameRaw = variablesNode->GetProperty(L"total-flag")->AsString();

        if (labelVarNameRaw.empty() || valueVarNameRaw.empty())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: label and value variables must be specified for "
                                   L"waterfall chart."),
                                 dsName)
                    .ToUTF8());
            }

        auto waterfallChart = std::make_shared<Graphs::WaterfallChart>(canvas);
        waterfallChart->SetPropertyTemplate(L"dataset", dsName);
        const auto labelVarName =
            ExpandAndCache(waterfallChart.get(), L"variables.label", labelVarNameRaw);
        const auto valueVarName =
            ExpandAndCache(waterfallChart.get(), L"variables.value", valueVarNameRaw);
        std::optional<wxString> totalFlagVarName{ std::nullopt };
        if (!totalFlagVarNameRaw.empty())
            {
            totalFlagVarName =
                ExpandAndCache(waterfallChart.get(), L"variables.total-flag", totalFlagVarNameRaw);
            }

        // orientation must be set before SetData(), as the bar-axis direction
        // (and axis titles) are resolved when the bars are built
        const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->AsString();
        if (bOrientation.CmpNoCase(L"horizontal") == 0)
            {
            waterfallChart->SetBarOrientation(Orientation::Horizontal);
            }
        else if (bOrientation.CmpNoCase(L"vertical") == 0)
            {
            waterfallChart->SetBarOrientation(Orientation::Vertical);
            }

        if (const auto color = ConvertColor(graphNode->GetProperty(L"increase-color"));
            color.IsOk())
            {
            waterfallChart->SetIncreaseColor(color);
            }
        if (const auto color = ConvertColor(graphNode->GetProperty(L"decrease-color"));
            color.IsOk())
            {
            waterfallChart->SetDecreaseColor(color);
            }
        if (const auto color = ConvertColor(graphNode->GetProperty(L"total-color")); color.IsOk())
            {
            waterfallChart->SetTotalColor(color);
            }

        if (const auto valueDisplay = ReportEnumConvert::ConvertNumberDisplay(
                graphNode->GetProperty(L"value-display-format")->AsString());
            valueDisplay.has_value())
            {
            waterfallChart->SetValueDisplay(valueDisplay.value());
            }

        if (graphNode->HasProperty(L"show-bar-values"))
            {
            waterfallChart->ShowBarValues(graphNode->GetProperty(L"show-bar-values")->AsBool());
            }
        if (graphNode->HasProperty(L"show-block-values"))
            {
            waterfallChart->ShowBlockValues(graphNode->GetProperty(L"show-block-values")->AsBool());
            }

        waterfallChart->SetData(foundPos->second, labelVarName, valueVarName, totalFlagVarName);

        LoadGraph(graphNode, canvas, currentRow, currentColumn, waterfallChart);
        return waterfallChart;
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

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadGanttChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Gantt chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto dateInterval = ReportEnumConvert::ConvertDateInterval(
                graphNode->GetProperty(L"date-interval")->AsString());
            const auto fyType = ReportEnumConvert::ConvertFiscalYear(
                graphNode->GetProperty(L"fy-type")->AsString());

            auto ganttChart = std::make_shared<Graphs::GanttChart>(canvas);
            ganttChart->SetData(
                foundPos->second,
                dateInterval.has_value() ? dateInterval.value() : DateInterval::FiscalQuarterly,
                fyType.has_value() ? fyType.value() : FiscalYear::USBusiness,
                ExpandAndCache(ganttChart.get(), L"variables.task",
                               variablesNode->GetProperty(L"task")->AsString()),
                ExpandAndCache(ganttChart.get(), L"variables.start-date",
                               variablesNode->GetProperty(L"start-date")->AsString()),
                ExpandAndCache(ganttChart.get(), L"variables.end-date",
                               variablesNode->GetProperty(L"end-date")->AsString()),
                (variablesNode->HasProperty(L"resource") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.resource",
                                        variablesNode->GetProperty(L"resource")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"description") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.description",
                                        variablesNode->GetProperty(L"description")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"completion") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.completion",
                                        variablesNode->GetProperty(L"completion")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"group") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.group",
                                        variablesNode->GetProperty(L"group")->AsString())) :
                     std::nullopt));

            const auto taskLabelDisplay = ReportEnumConvert::ConvertTaskLabelDisplay(
                graphNode->GetProperty(L"task-label-display")->AsString());
            if (taskLabelDisplay.has_value())
                {
                ganttChart->SetLabelDisplay(taskLabelDisplay.value());
                }

            LoadBarChart(graphNode, ganttChart);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, ganttChart);
            return ganttChart;
            }

        throw std::runtime_error(_(L"Variables not defined for Gantt chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadCandlestickPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                       size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for candlestick plot."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto candlestickPlot = std::make_shared<Graphs::CandlestickPlot>(canvas);
            candlestickPlot->SetData(
                foundPos->second,
                ExpandAndCache(candlestickPlot.get(), L"variables.date",
                               variablesNode->GetProperty(L"date")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.open",
                               variablesNode->GetProperty(L"open")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.high",
                               variablesNode->GetProperty(L"high")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.low",
                               variablesNode->GetProperty(L"low")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.close",
                               variablesNode->GetProperty(L"close")->AsString()));

            const auto plotType = ReportEnumConvert::ConvertCandlestickPlotType(
                graphNode->GetProperty(L"plot-type")->AsString());
            if (plotType.has_value())
                {
                candlestickPlot->SetPlotType(plotType.value());
                }

            LoadBrush(graphNode->GetProperty(L"gain-brush"), candlestickPlot->GetGainBrush());
            LoadBrush(graphNode->GetProperty(L"loss-brush"), candlestickPlot->GetLossBrush());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, candlestickPlot);
            return candlestickPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for candlestick plot.").ToUTF8());
        }
    } // namespace Wisteria
