///////////////////////////////////////////////////////////////////////////////
// Name:        ganttchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ganttchart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::GanttChart, Wisteria::Graphs::BarChart)

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    GanttChart::GanttChart(Wisteria::Canvas* canvas,
                           std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/)
        : BarChart(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetBarOrientation(Orientation::Horizontal);
        GetRightYAxis().Show(false);
        GetScalingAxis().Show(false);
        IncludeSpacesBetweenBars();
        SetSortable(true);

        GetBarAxis().SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::AlignWithBoundary);
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetScalingAxis().GetGridlinePen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void GanttChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const DateInterval interval, const FiscalYear FYType,
                             const wxString& taskColumnName, const wxString& startDateColumnName,
                             const wxString& endDateColumnName,
                             std::optional<const wxString> resourceColumnName /*= std::nullopt*/,
                             std::optional<const wxString> descriptionColumnName /*= std::nullopt*/,
                             std::optional<const wxString> completionColumnName /*= std::nullopt*/,
                             std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        // point to (new) data and reset
        SetDataset(data);
        ResetGrouping();
        ClearBars();
        ClearBarGroups();
        GetSelectedIds().clear();
        m_legendTitle.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        m_dateDisplayInterval = interval;
        m_fyType = FYType;

        auto taskColumn = GetDataset()->GetCategoricalColumn(taskColumnName);
        if (taskColumn == GetDataset()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': task name column not found for Gantt chart."),
                                 taskColumnName)
                    .ToUTF8());
            }
        auto startColumn = GetDataset()->GetDateColumn(startDateColumnName);
        if (startColumn == GetDataset()->GetDateColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': start date column not found for Gantt chart."),
                                 startDateColumnName)
                    .ToUTF8());
            }
        auto endColumn = GetDataset()->GetDateColumn(endDateColumnName);
        if (endColumn == GetDataset()->GetDateColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': end date column not found for Gantt chart."),
                                 endDateColumnName)
                    .ToUTF8());
            }
        // these columns are optional
        auto resourceColumn =
            GetDataset()->GetCategoricalColumn(resourceColumnName.value_or(wxString()));
        auto completionColumn =
            GetDataset()->GetContinuousColumn(completionColumnName.value_or(wxString()));
        // set the grouping column (or keep it as null if not in use)
        SetGroupColumn(groupColumnName);
        auto descriptionColumn =
            GetDataset()->GetCategoricalColumn(descriptionColumnName.value_or(wxString()));

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            }

        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            const size_t colorIndex =
                IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) : 0;

            AddTask(GanttChart::TaskInfo(taskColumn->GetLabelFromID(taskColumn->GetValue(i)))
                        .Resource((resourceColumn != GetDataset()->GetCategoricalColumns().cend()) ?
                                      resourceColumn->GetLabelFromID(resourceColumn->GetValue(i)) :
                                      wxString())
                        .Description(
                            (descriptionColumn != GetDataset()->GetCategoricalColumns().cend()) ?
                                descriptionColumn->GetLabelFromID(descriptionColumn->GetValue(i)) :
                                wxString())
                        .StartDate(startColumn->GetValue(i))
                        .EndDate(endColumn->GetValue(i))
                        .Color(GetColorScheme()->GetColor(colorIndex))
                        .PercentFinished(
                            (completionColumn != GetDataset()->GetContinuousColumns().cend() ?
                                 zero_if_nan(completionColumn->GetValue(i)) :
                                 0))
                        .LabelDisplay(GetLabelDisplay()));
            }
        }

    //----------------------------------------------------------------
    void GanttChart::Calculate()
        {
        if (!m_tasks.size())
            {
            return;
            }

        wxDateTime firstDay =
            std::min_element(m_tasks.cbegin(), m_tasks.cend(),
                             [](const auto& task1, const auto& task2)
                             {
                                 return (!task1.m_start.IsValid() && !task2.m_start.IsValid()) ?
                                            false :
                                        (!task1.m_start.IsValid() && task2.m_start.IsValid()) ?
                                            false :
                                        (task1.m_start.IsValid() && !task2.m_start.IsValid()) ?
                                            true :
                                            task1.m_start < task2.m_start;
                             })
                ->m_start;
        wxDateTime lastDay =
            std::max_element(m_tasks.cbegin(), m_tasks.cend(),
                             [](const auto& task1, const auto& task2)
                             {
                                 return (!task1.m_end.IsValid() && !task2.m_end.IsValid()) ?
                                            false :
                                        (!task1.m_end.IsValid() && task2.m_end.IsValid()) ?
                                            true :
                                        (task1.m_end.IsValid() && !task2.m_end.IsValid()) ?
                                            false :
                                            task1.m_end < task2.m_end;
                             })
                ->m_end;

        if (firstDay.IsValid() && lastDay.IsValid())
            {
            GetScalingAxis().SetRange(firstDay, lastDay, GetDateDisplayInterval(),
                                      GetFiscalYearType());
            }

        GetTopXAxis().CopySettings(GetScalingAxis());
        if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly &&
            GetTopXAxis().GetRangeDates().first.IsValid() &&
            GetTopXAxis().GetRangeDates().second.IsValid())
            {
            GetTopXAxis().AddBrackets(BracketType::FiscalQuarterly);
            }

        // reverse so that bars appear in the order that the client constructed them
        GetBarAxis().ReverseScale(true);

        if (GetScalingAxis().GetRangeDates().first.IsValid() &&
            GetScalingAxis().GetRangeDates().second.IsValid())
            {
            m_debugDrawInfoLabel = wxString::Format(
                _DT(L"Date range: %s-%s"), GetScalingAxis().GetRangeDates().first.FormatDate(),
                GetScalingAxis().GetRangeDates().second.FormatDate());
            }
        }

    //----------------------------------------------------------------
    void GanttChart::RecalcSizes(wxDC& dc)
        {
        ClearBars(false);

        for (const auto& taskInfo : m_tasks)
            {
            if (taskInfo.m_start.IsValid() && taskInfo.m_end.IsValid())
                {
                const GraphItems::Label axisLabel(taskInfo.m_name);

                const auto startPt = GetScalingAxis().FindDatePosition(taskInfo.m_start);
                const auto endPt = GetScalingAxis().FindDatePosition(taskInfo.m_end);
                assert(startPt.has_value() && endPt.has_value() &&
                       L"Valid dates not found on axis in Gantt chart?!");
                if (!startPt.has_value() || !endPt.has_value())
                    {
                    continue;
                    }

                const auto daysInTask = static_cast<int>(endPt.value() - startPt.value());
                const double daysFinished =
                    safe_divide<double>(taskInfo.m_percentFinished, 100) * daysInTask;
                const double daysRemaining = daysInTask - daysFinished;
                Bar br(GetBars().size(),
                       { { BarBlock(BarBlockInfo(daysFinished)
                                        .Brush(wxBrush(
                                            ColorContrast::BlackOrWhiteContrast(taskInfo.m_color),
                                            wxBrushStyle::wxBRUSHSTYLE_FDIAGONAL_HATCH))
                                        .Color(taskInfo.m_color)
                                        .SelectionLabel(GraphItems::Label(
                                            wxString(wxString::Format(
                                                         _(L"%s\n%d days\n(%s through %s)"),
                                                         wxString(taskInfo.m_resource + L"\n" +
                                                                  taskInfo.m_description)
                                                             .Trim(),
                                                         daysInTask, taskInfo.m_start.FormatDate(),
                                                         taskInfo.m_end.FormatDate()))
                                                .Trim(true)
                                                .Trim(false)))) },
                         { BarBlock(BarBlockInfo(daysRemaining)
                                        .Brush(taskInfo.m_color)
                                        .SelectionLabel(GraphItems::Label(
                                            wxString(wxString::Format(
                                                         _(L"%s\n%d days\n(%s through %s)"),
                                                         wxString(taskInfo.m_resource + L"\n" +
                                                                  taskInfo.m_description)
                                                             .Trim(),
                                                         daysInTask, taskInfo.m_start.FormatDate(),
                                                         taskInfo.m_end.FormatDate()))
                                                .Trim(true)
                                                .Trim(false)))) } },
                       wxEmptyString, axisLabel, GetBarEffect(), GetBarOpacity());
                // remove "completed" bar block if nothing is actually completed
                if (taskInfo.m_percentFinished == 0)
                    {
                    br.GetBlocks().erase(br.GetBlocks().begin());
                    }
                else
                    {
                    br.GetLabel().SetText(
                        taskInfo.m_percentFinished == 100 ?
                            _(L"\x2713 Complete") :
                            wxString::Format(_(L"%d%% complete"), taskInfo.m_percentFinished));
                    }
                // move bar to actual starting date
                br.SetCustomScalingAxisStartPosition(startPt.value());

                // format the decal on the bar
                wxString decalStr;
                switch (taskInfo.m_labelDisplay)
                    {
                case TaskLabelDisplay::Resource:
                    decalStr = taskInfo.m_resource;
                    break;
                case TaskLabelDisplay::ResourceAndDays:
                    decalStr = wxString::Format(_(L"%s\n%d days"), taskInfo.m_resource, daysInTask);
                    break;
                case TaskLabelDisplay::Description:
                    decalStr = taskInfo.m_description;
                    break;
                case TaskLabelDisplay::DescriptionAndDays:
                    decalStr =
                        wxString::Format(_(L"%s\n%d days"), taskInfo.m_description, daysInTask);
                    break;
                case TaskLabelDisplay::ResourceAndDescription:
                    decalStr = taskInfo.m_resource + L"\n" + taskInfo.m_description;
                    break;
                case TaskLabelDisplay::ResourceDescriptionAndDays:
                    decalStr = wxString::Format(
                        _(L"%s\n%d days"),
                        wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(),
                        daysInTask);
                    break;
                case TaskLabelDisplay::Days:
                    decalStr = wxString::Format(_(L"%d days"), daysInTask);
                    break;
                case TaskLabelDisplay::NoDisplay:
                    [[fallthrough]];
                default:
                    decalStr.clear();
                    }
                decalStr.Trim();

                br.GetBlocks().front().SetDecal(
                    GraphItems::Label(GraphItemInfo(decalStr)
                                          .ChildAlignment(RelativeAlignment::FlushLeft)
                                          .FontColor(ColorContrast::BlackOrWhiteContrast(
                                              br.GetBlocks().front().GetBrush().GetColour()))));
                br.GetBlocks().front().GetSelectionLabel().SplitTextToFitLength(
                    m_maxDescriptionLength);
                if (taskInfo.m_img.IsOk() && taskInfo.m_name.length())
                    {
                    // see how tall the name label is and scale the image to that size
                    br.GetAxisLabel().SetScaling(GetScaling());
                    br.GetAxisLabel().SetLeftImage(taskInfo.m_img);
                    }
                AddBar(br, false);
                }
            else
                {
                const GraphItems::Label axisLabel(taskInfo.m_name);

                const auto startPoint = GetScalingAxis().FindDatePosition(taskInfo.m_start);
                const auto endPoint = GetScalingAxis().FindDatePosition(taskInfo.m_end);
                const auto daysDiff =
                    (endPoint.has_value() ? endPoint.value() : GetScalingAxis().GetRange().second) -
                    (startPoint.has_value() ? startPoint.value() :
                                              GetScalingAxis().GetRange().first);

                Bar arrowBar(
                    GetBars().size(),
                    { { BarBlock(
                        BarBlockInfo(daysDiff)
                            .Brush(taskInfo.m_color)
                            .SelectionLabel(GraphItems::Label(
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description)
                                    .Trim(true)
                                    .Trim(false)))) } },
                    wxEmptyString, GraphItems::Label(axisLabel), GetBarEffect(), GetBarOpacity());
                arrowBar.SetCustomScalingAxisStartPosition(startPoint);
                arrowBar.SetShape(BarShape::Arrow);

                wxString decalStr;
                switch (taskInfo.m_labelDisplay)
                    {
                case TaskLabelDisplay::Resource:
                    [[fallthrough]];
                case TaskLabelDisplay::ResourceAndDays:
                    decalStr = taskInfo.m_resource;
                    break;
                case TaskLabelDisplay::Description:
                    [[fallthrough]];
                case TaskLabelDisplay::DescriptionAndDays:
                    decalStr = taskInfo.m_description;
                    break;
                case TaskLabelDisplay::ResourceAndDescription:
                    [[fallthrough]];
                case TaskLabelDisplay::ResourceDescriptionAndDays:
                    decalStr =
                        wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim();
                    break;
                // Days makes no sense for a non-ending bar, so ignore it
                case TaskLabelDisplay::Days:
                    [[fallthrough]];
                case TaskLabelDisplay::NoDisplay:
                    [[fallthrough]];
                default:
                    decalStr.clear();
                    }
                decalStr.Trim();

                arrowBar.GetBlocks().front().SetDecal(
                    GraphItems::Label(GraphItemInfo(decalStr).FontColor(
                        ColorContrast::BlackOrWhiteContrast(taskInfo.m_color))));
                arrowBar.GetBlocks().front().GetSelectionLabel().SplitTextToFitLength(
                    m_maxDescriptionLength);
                if (taskInfo.m_img.IsOk() && taskInfo.m_name.length())
                    {
                    // see how tall the name label is and scale the image to that size
                    arrowBar.GetAxisLabel().SetScaling(GetScaling());
                    arrowBar.GetAxisLabel().SetLeftImage(taskInfo.m_img);
                    }
                AddBar(arrowBar, false);
                }
            }

        BarChart::RecalcSizes(dc);
        }
    } // namespace Wisteria::Graphs
