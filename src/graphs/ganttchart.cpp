///////////////////////////////////////////////////////////////////////////////
// Name:        ganttchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ganttchart.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    GanttChart::GanttChart(Wisteria::Canvas* canvas,
        std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/) :
        BarChart(canvas),
        m_colorScheme(colors != nullptr ? colors :
            Settings::GetDefaultColorScheme())
        {
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
                             const DateInterval interval,
                             const FiscalYear FYType,
                             const wxString& taskColumnName,
                             const wxString& startDateColumnName,
                             const wxString& endDateColumnName,
                             std::optional<const wxString> resourceColumnName /*= std::nullopt*/,
                             std::optional<const wxString> descriptionColumnName /*= std::nullopt*/,
                             std::optional<const wxString> completionColumnName /*= std::nullopt*/,
                             std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        if (data == nullptr)
            { return; }

        ClearBars();
        GetSelectedIds().clear();
        m_legendLines.clear();
        m_legendTitle.clear();

        m_dateDisplayInterval = interval;
        m_fyType = FYType;

        auto taskColumn = data->GetCategoricalColumn(taskColumnName);
        if (taskColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': task name column not found for gantt chart."),
                taskColumnName).ToUTF8());
            }
        auto startColumn = data->GetDateColumn(startDateColumnName);
        if (startColumn == data->GetDateColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': start date column not found for gantt chart."),
                startDateColumnName).ToUTF8());
            }
        auto endColumn = data->GetDateColumn(endDateColumnName);
        if (endColumn == data->GetDateColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': end date column not found for gantt chart."),
                endDateColumnName).ToUTF8());
            }
        // these columns are optional
        auto resourceColumn = data->GetCategoricalColumn(resourceColumnName.value_or(wxString(L"")));
        auto completionColumn = data->GetContinuousColumn(completionColumnName.value_or(wxString(L"")));
        auto groupColumn = data->GetCategoricalColumn(groupColumnName.value_or(wxString(L"")));
        auto descriptionColumn = data->GetCategoricalColumn(descriptionColumnName.value_or(wxString(L"")));

        std::set<Data::GroupIdType> groupIds;

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            AddTask(
                GanttChart::TaskInfo(taskColumn->GetCategoryLabelFromID(taskColumn->GetValue(i))).
                Resource(
                    (resourceColumn != data->GetCategoricalColumns().cend()) ?
                     resourceColumn->GetCategoryLabelFromID(resourceColumn->GetValue(i)) :
                     wxString(L"")).
                Description(
                    (descriptionColumn != data->GetCategoricalColumns().cend()) ?
                     descriptionColumn->GetCategoryLabelFromID(descriptionColumn->GetValue(i)) :
                     wxString(L"")).
                StartDate(startColumn->GetValue(i)).
                EndDate(endColumn->GetValue(i)).
                Color(
                    (groupColumn != data->GetCategoricalColumns().cend() ?
                     GetColorScheme()->GetColor(groupColumn->GetValue(i)) :
                     GetColorScheme()->GetColor(0))).
                PercentFinished(
                    (completionColumn != data->GetContinuousColumns().cend() ?
                     zero_if_nan(completionColumn->GetValue(i)) : 0)).
                LabelDisplay(GetLabelDisplay()));
            // build a list of used group IDs (used for the legend later)
            if (groupColumn != data->GetCategoricalColumns().cend())
                { groupIds.insert(groupColumn->GetValue(i)); }
            }

        if (groupColumn != data->GetCategoricalColumns().cend())
            {
            m_legendTitle = groupColumn->GetTitle();
            for (const auto& groupId : groupIds)
                {
                m_legendLines.emplace(
                    std::make_pair(groupColumn->GetCategoryLabelFromID(groupId),
                                   GetColorScheme()->GetColor(groupId)));
                }
            }
        }

    //----------------------------------------------------------------
    void GanttChart::Calculate()
        {
        if (!m_tasks.size())
            { return; }

        wxDateTime firstDay = std::min_element(m_tasks.cbegin(), m_tasks.cend(),
            [](const auto& task1, const auto& task2)
                {
                return (!task1.m_start.IsValid() && !task2.m_start.IsValid()) ? false :
                    (!task1.m_start.IsValid() && task2.m_start.IsValid()) ? false :
                    (task1.m_start.IsValid() && !task2.m_start.IsValid()) ? true :
                    task1.m_start < task2.m_start;
                })->m_start;
        wxDateTime lastDay = std::max_element(m_tasks.cbegin(), m_tasks.cend(),
            [](const auto& task1, const auto& task2)
                {
                return (!task1.m_end.IsValid() && !task2.m_end.IsValid()) ? false :
                    (!task1.m_end.IsValid() && task2.m_end.IsValid()) ? true :
                    (task1.m_end.IsValid() && !task2.m_end.IsValid()) ? false :
                    task1.m_end < task2.m_end;
                })->m_end;

        if (firstDay.IsValid() && lastDay.IsValid())
            {
            GetScalingAxis().SetRange(firstDay, lastDay,
                                      GetDateDisplayInterval(), GetFiscalYearType());
            }

        GetTopXAxis().CopySettings(GetScalingAxis());
        if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly &&
            GetTopXAxis().GetRangeDates().first.IsValid() &&
            GetTopXAxis().GetRangeDates().second.IsValid())
            { GetTopXAxis().AddBrackets(BracketType::FiscalQuarterly); }

        // reverse so that bars appear in the order that the client constructed them
        GetBarAxis().ReverseScale(true);

        if (GetScalingAxis().GetRangeDates().first.IsValid() &&
            GetScalingAxis().GetRangeDates().second.IsValid())
            {
            m_debugDrawInfoLabel = wxString::Format(_DT(L"Date range: %s-%s"),
                GetScalingAxis().GetRangeDates().first.FormatDate(),
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

                const auto startPt = GetScalingAxis().GetPointFromDate(taskInfo.m_start);
                const auto endPt = GetScalingAxis().GetPointFromDate(taskInfo.m_end);
                wxASSERT_MSG(startPt.has_value() && endPt.has_value(),
                    L"Valid dates not found on axis in Gantt chart?!");
                if (!startPt.has_value() || !endPt.has_value())
                    { continue; }

                const auto daysInTask = static_cast<int>(endPt.value() - startPt.value());
                const double daysFinished = safe_divide<double>(taskInfo.m_percentFinished, 100) * daysInTask;
                const double daysRemaining = daysInTask-daysFinished;
                Bar br(GetBars().size(),
                    {
                        { BarBlock(BarBlockInfo(daysFinished).
                            Brush(wxBrush(ColorContrast::BlackOrWhiteContrast(taskInfo.m_color),
                                          wxBrushStyle::wxBRUSHSTYLE_FDIAGONAL_HATCH)).
                            Color(taskInfo.m_color).
                            SelectionLabel(GraphItems::Label(
                                wxString(wxString::Format(_(L"%s\n%d days\n(%s through %s)"),
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(), daysInTask,
                                taskInfo.m_start.FormatDate(), taskInfo.m_end.FormatDate())).
                                Trim(true).Trim(false)))) },
                        { BarBlock(BarBlockInfo(daysRemaining).
                            Brush(taskInfo.m_color).
                            SelectionLabel(GraphItems::Label(
                                wxString(wxString::Format(_(L"%s\n%d days\n(%s through %s)"),
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(), daysInTask,
                                taskInfo.m_start.FormatDate(), taskInfo.m_end.FormatDate())).
                                Trim(true).Trim(false)))) }
                    },
                    wxEmptyString,
                    axisLabel, GetBarEffect(), GetBarOpacity());
                // remove "completed" bar block if nothing is actually completed
                if (taskInfo.m_percentFinished == 0)
                    { br.GetBlocks().erase(br.GetBlocks().begin()); }
                else
                    {
                    br.GetLabel().SetText(taskInfo.m_percentFinished == 100 ?
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
                    decalStr = wxString::Format(_(L"%s\n%d days"), taskInfo.m_description, daysInTask);
                    break;
                case TaskLabelDisplay::ResourceAndDescription:
                    decalStr = taskInfo.m_resource + L"\n" + taskInfo.m_description;
                    break;
                case TaskLabelDisplay::ResourceDescriptionAndDays:
                    decalStr = wxString::Format(_(L"%s\n%d days"),
                                   wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(), daysInTask);
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

                br.GetBlocks().front().SetDecal(GraphItems::Label(GraphItemInfo(decalStr).
                    ChildAlignment(RelativeAlignment::FlushLeft).
                    FontColor(ColorContrast::BlackOrWhiteContrast(
                        br.GetBlocks().front().GetBrush().GetColour()))) );
                br.GetBlocks().front().GetSelectionLabel().SplitTextToFitLength(m_maxDescriptionLength);
                if (taskInfo.m_img.IsOk() && taskInfo.m_name.length())
                    {
                    // see how tall the name label is and scale the image to that size
                    br.GetAxisLabel().SetScaling(GetScaling());
                    const auto labelHeight = br.GetAxisLabel().GetBoundingBox(dc).GetHeight();

                    const auto scaledSize = wxSize(geometry::calculate_rescale_width(
                        std::make_pair<double, double>(taskInfo.m_img.GetSize().GetWidth(),
                                                       taskInfo.m_img.GetSize().GetHeight()),
                        labelHeight), labelHeight);
                    wxImage img{ taskInfo.m_img };
                    img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

                    // Set the axis labels' padding to fit the widest image so far
                    // (or at least the min legend size).
                    // Labels that are taller than others (because they have new lines in them)
                    // will have larger images next to them.
                    GetBarAxis().SetLeftPadding(std::max<double>(GetBarAxis().GetLeftPadding(),
                        std::max<double>(DownscaleFromScreenAndCanvas(img.GetWidth())+5,
                        Label::GetMinLegendWidthDIPs())) );
                    br.GetAxisLabel().GetLegendIcons().emplace_back(
                        LegendIcon(IconShape::ImageWholeLegend, img));
                    }
                AddBar(br, false);
                }
            else
                {
                const GraphItems::Label axisLabel(taskInfo.m_name);

                const auto startPoint = GetScalingAxis().GetPointFromDate(taskInfo.m_start);
                const auto endPoint = GetScalingAxis().GetPointFromDate(taskInfo.m_end);
                const auto daysDiff =
                    (endPoint.has_value() ? endPoint.value() : GetScalingAxis().GetRange().second) -
                    (startPoint.has_value() ? startPoint.value() : GetScalingAxis().GetRange().first);

                Bar arrowBar(GetBars().size(),
                    {
                        {
                        BarBlock(BarBlockInfo(daysDiff).
                            Brush(taskInfo.m_color).
                            SelectionLabel(GraphItems::Label(
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).
                                Trim(true).Trim(false))))
                        }
                    },
                    wxEmptyString,
                    GraphItems::Label(axisLabel), GetBarEffect(), GetBarOpacity());
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
                    decalStr = wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim();
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

                arrowBar.GetBlocks().front().SetDecal(GraphItems::Label(GraphItemInfo(decalStr).
                    FontColor(ColorContrast::BlackOrWhiteContrast(
                        taskInfo.m_color))) );
                arrowBar.GetBlocks().front().GetSelectionLabel().SplitTextToFitLength(m_maxDescriptionLength);
                if (taskInfo.m_img.IsOk() && taskInfo.m_name.length())
                    {
                    // see how tall the name label is and scale the image to that size
                    arrowBar.GetAxisLabel().SetScaling(GetScaling());
                    const auto labelHeight = arrowBar.GetAxisLabel().GetBoundingBox(dc).GetHeight();

                    const auto scaledSize = wxSize(geometry::calculate_rescale_width(
                        std::make_pair<double, double>(taskInfo.m_img.GetSize().GetWidth(),
                                                       taskInfo.m_img.GetSize().GetHeight()),
                        labelHeight), labelHeight);
                    wxImage img{ taskInfo.m_img };
                    img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

                    // Set the axis labels' padding to fit the widest image so far
                    // (or at least the min legend size).
                    // Labels that are taller than others (because they have new lines in them)
                    // will have larger images next to them.
                    GetBarAxis().SetLeftPadding(std::max<double>(GetBarAxis().GetLeftPadding(),
                        std::max<double>(DownscaleFromScreenAndCanvas(img.GetWidth())+5,
                        Label::GetMinLegendWidthDIPs())) );
                    arrowBar.GetAxisLabel().GetLegendIcons().emplace_back(
                        LegendIcon(IconShape::ImageWholeLegend, img));
                    }
                AddBar(arrowBar, false);
                }
            }

        BarChart::RecalcSizes(dc);
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> GanttChart::CreateLegend(
        const LegendCanvasPlacementHint hint, const bool includeHeader)
        {
        if (m_legendLines.empty())
            { return nullptr; }

        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));
        legend->SetBoxCorners(BoxCorners::Rounded);
        legend->GetFont().MakeSmaller();
        legend->GetHeaderInfo().GetFont().MakeSmaller();

        wxString legendText;
        size_t lineCount{ 0 };
        for (const auto& legendLine : m_legendLines)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = legendLine.first;
            if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                {
                currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::SquareIcon,
                        legendLine.second,
                        legendLine.second));
            ++lineCount;
            }
        if (includeHeader)
            {
            legendText.Prepend(wxString::Format(L"%s\n", m_legendTitle));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
