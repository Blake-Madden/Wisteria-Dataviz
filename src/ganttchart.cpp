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
            std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::EarthTones()))
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

        GetScalingAxis().SetRange(firstDay, lastDay,
                                  GetDateDisplayInterval(), GetFiscalYearType());

        GetTopXAxis().CopySettings(GetScalingAxis());
        if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly)
            { GetTopXAxis().AddBrackets(BracketType::FiscalQuarterly); }

        // reverse so that bars appear in the order that the client constructed them
        GetBarAxis().ReverseScale(true);

        m_debugDrawInfoLabel = wxString::Format(_DT(L"Date range: %s-%s"),
            GetScalingAxis().GetRangeDates().first.FormatDate(),
            GetScalingAxis().GetRangeDates().second.FormatDate());
        }

    //----------------------------------------------------------------
    void GanttChart::RecalcSizes()
        {
        ClearBars(false);

        for (const auto& taskInfo : m_tasks)
            {
            if (taskInfo.m_start.IsValid() && taskInfo.m_end.IsValid())
                {
                wxGCDC measureDC;
                const GraphItems::Label axisLabel(taskInfo.m_name);

                const auto dateOffset = taskInfo.m_start.GetDateOnly().Subtract(
                    GetScalingAxis().GetRangeDates().first.GetDateOnly()).GetDays();
                const auto daysInTask = taskInfo.m_end.GetDateOnly().Subtract(taskInfo.m_start.GetDateOnly()).GetDays() + 1;
                const double daysFinished = safe_divide<double>(taskInfo.m_percentFinished, 100) * daysInTask;
                const double daysRemaining = daysInTask-daysFinished;
                Bar br(GetBars().size(),
                    {
                        { BarBlock(BarBlockInfo(daysFinished).
                            Brush({ColorContrast::ShadeOrTint(GetColorScheme()->GetColor(GetBars().size()))}).
                            SelectionLabel(GraphItems::Label(
                                wxString::Format(_(L"%s\n%d days\n(%s through %s)"),
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(), daysInTask,
                                taskInfo.m_start.FormatDate(), taskInfo.m_end.FormatDate())))) },
                        { BarBlock(BarBlockInfo(daysRemaining).
                            Brush(GetColorScheme()->GetColor(GetBars().size())).
                            SelectionLabel(GraphItems::Label(
                                wxString::Format(_(L"%s\n%d days\n(%s through %s)"),
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim(), daysInTask,
                                taskInfo.m_start.FormatDate(), taskInfo.m_end.FormatDate())))) }
                    },
                    wxEmptyString,
                    axisLabel, BoxEffect::Solid);
                // remove "completed" bar block if nothing is actually completed
                if (taskInfo.m_percentFinished == 0)
                    { br.GetBlocks().erase(br.GetBlocks().begin()); }
                else
                    {
                    br.GetLabel().SetText(taskInfo.m_percentFinished == 100 ?
                        _(L"Complete") :
                        wxString::Format(_(L"%d%% complete"), taskInfo.m_percentFinished));
                    }
                // move bar to actual starting date
                br.SetCustomScalingAxisStartPosition(dateOffset);

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
                    const auto labelHeight = br.GetAxisLabel().GetBoundingBox(measureDC).GetHeight();

                    const auto scaledSize = wxSize(geometry::calculate_rescale_width(
                        std::make_pair<double, double>(taskInfo.m_img.GetSize().GetWidth(),
                                                       taskInfo.m_img.GetSize().GetHeight()), labelHeight), labelHeight);
                    wxImage img{ taskInfo.m_img };
                    img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

                    // Set the axis labels' padding to fit the widest image so far
                    // (or at least the min legend size).
                    // Labels that are taller than others (because they have new lines in them)
                    // will have larger images next to them.
                    GetBarAxis().SetLeftPadding(std::max<double>(GetBarAxis().GetLeftPadding(),
                        std::max<double>(DownscaleFromScreenAndCanvas(img.GetWidth())+5,
                        Label::GetMinLegendWidth())) );
                    br.GetAxisLabel().GetLegendIcons().emplace_back(
                        LegendIcon(IconShape::ImageWholeLegend, img));
                    }
                AddBar(br, false);
                }
            else
                {
                const GraphItems::Label axisLabel(taskInfo.m_name);
                wxGCDC measureDC;

                const auto startPoint = GetScalingAxis().GetPointFromDate(taskInfo.m_start);
                const auto endPoint = GetScalingAxis().GetPointFromDate(taskInfo.m_end);
                const auto daysDiff =
                    (endPoint.has_value() ? endPoint.value() : GetScalingAxis().GetRange().second) -
                    (startPoint.has_value() ? startPoint.value() : GetScalingAxis().GetRange().first);

                Bar arrowBar(GetBars().size(),
                    {
                        {
                        BarBlock(BarBlockInfo(daysDiff).
                            Brush(GetColorScheme()->GetColor(GetBars().size())).
                            SelectionLabel(GraphItems::Label(
                                wxString(taskInfo.m_resource + L"\n" + taskInfo.m_description).Trim())))
                        }
                    },
                    wxEmptyString,
                    GraphItems::Label(axisLabel), BoxEffect::Solid);
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
                        GetColorScheme()->GetColor(GetBars().size())))) );
                arrowBar.GetBlocks().front().GetSelectionLabel().SplitTextToFitLength(m_maxDescriptionLength);
                if (taskInfo.m_img.IsOk() && taskInfo.m_name.length())
                    {
                    // see how tall the name label is and scale the image to that size
                    arrowBar.GetAxisLabel().SetScaling(GetScaling());
                    const auto labelHeight = arrowBar.GetAxisLabel().GetBoundingBox(measureDC).GetHeight();

                    const auto scaledSize = wxSize(geometry::calculate_rescale_width(
                        std::make_pair<double, double>(taskInfo.m_img.GetSize().GetWidth(),
                                                       taskInfo.m_img.GetSize().GetHeight()),
                        labelHeight), labelHeight);
                    wxImage img{ taskInfo.m_img };
                    img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

                    // Set the axis labels' padding to fit the widest image so far
                    // (or at least the min legend size).
                    // Labels that are taller than other (because they have new lines in them)
                    // will have larger images next to them.
                    GetBarAxis().SetLeftPadding(std::max<double>(GetBarAxis().GetLeftPadding(),
                        std::max<double>(DownscaleFromScreenAndCanvas(img.GetWidth())+5,
                        Label::GetMinLegendWidth())) );
                    arrowBar.GetAxisLabel().GetLegendIcons().emplace_back(
                        LegendIcon(IconShape::ImageWholeLegend, img));
                    }
                AddBar(arrowBar, false);
                }
            }

        BarChart::RecalcSizes();
        }
    }
