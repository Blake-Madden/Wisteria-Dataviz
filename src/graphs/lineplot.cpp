///////////////////////////////////////////////////////////////////////////////
// Name:        lineplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lineplot.h"

using namespace Wisteria::GraphItems;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void LinePlot::SetData(std::shared_ptr<const Data::Dataset> data,
                           const wxString& yColumnName,
                           const wxString& xColumnName,
                           std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        if (data == nullptr)
            { return; }

        m_data = data;
        GetSelectedIds().clear();

        m_useGrouping = groupColumnName.has_value();
        m_groupColumn = (groupColumnName ? GetData()->GetCategoricalColumn(groupColumnName.value()) :
            GetData()->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == GetData()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for line plot."), groupColumnName.value()).ToUTF8());
            }
        m_yColumnName = yColumnName;
        m_yColumn = GetData()->GetContinuousColumn(yColumnName);
        if (m_yColumn == GetData()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': Y column not found for line plot."), yColumnName).ToUTF8());
            }
        // set the X column, which will be access through various GetX functions later
        // (do not reference these iterators after setting them here)
        m_xColumnCategorical = GetData()->GetCategoricalColumns().cend(); // reset
        // look for it as a continuous variable first
        m_xColumnContinuous = GetData()->GetContinuousColumn(xColumnName);
        if (m_xColumnContinuous == GetData()->GetContinuousColumns().cend())
            {
            // if not found, look for it as a categorical
            m_xColumnCategorical = GetData()->GetCategoricalColumn(xColumnName);
            if (m_xColumnCategorical == GetData()->GetCategoricalColumns().cend())
                {
                // try date columns
                m_xColumnDate = GetData()->GetDateColumn(xColumnName);
                if (m_xColumnDate == GetData()->GetDateColumns().cend())
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"'%s': X column not found for line plot."), xColumnName).ToUTF8());
                    }
                }
            }
        m_lines.clear();
        GetLeftYAxis().Reset();
        GetRightYAxis().Reset();
        GetBottomXAxis().Reset();
        GetTopXAxis().Reset();
        GetBottomXAxis().GetTitle().SetText(xColumnName);
        GetLeftYAxis().GetTitle().SetText(yColumnName);

        if (m_useGrouping)
            {
            std::set<Data::GroupIdType> groups;
            for (const auto& groupId : m_groupColumn->GetValues())
                { groups.insert(groupId); }
            for (const auto& group : groups)
                {
                Line ln;
                ln.SetGroupInfo(groupColumnName, group,
                                m_groupColumn->GetCategoryLabelFromID(group));
                ln.GetPen().SetColour(GetColorScheme()->GetColor(group));
                // if some sort of spiral, then draw as a dashed spline
                if (IsAutoSplining() && !IsDataSingleDirection(data, group))
                    {
                    ln.GetPen().SetStyle(wxPenStyle::wxPENSTYLE_SHORT_DASH);
                    ln.SetStyle(LineStyle::Spline);
                    }
                else
                    {
                    const auto& [penStyle, lineStyle] = GetPenStyleScheme()->GetLineStyle(group);
                    ln.GetPen().SetStyle(penStyle);
                    ln.SetStyle(lineStyle);
                    }
                AddLine(ln);
                }
            }
        else
            {
            Line ln;
            ln.SetGroupInfo(groupColumnName, 0, L"");
            ln.GetPen().SetColour(GetColorScheme()->GetColor(0));
            if (IsAutoSplining() && !IsDataSingleDirection(data, 0))
                {
                ln.GetPen().SetStyle(wxPenStyle::wxPENSTYLE_SHORT_DASH);
                ln.SetStyle(LineStyle::Spline);
                }
            else
                {
                const auto& [penStyle, lineStyle] = GetPenStyleScheme()->GetLineStyle(0);
                ln.GetPen().SetStyle(penStyle);
                ln.SetStyle(lineStyle);
                }
            AddLine(ln);
            }

        // sort the lines by their group label
        if (m_useGrouping)
            {
            std::sort(m_lines.begin(), m_lines.end(),
                [this](const auto& first, const auto& second) noexcept
                    {
                    return m_groupColumn->GetCategoryLabelFromID(first.m_groupId) <
                           m_groupColumn->GetCategoryLabelFromID(second.m_groupId);
                    }
                );
            }
        }

    //-------------------------------------------
    bool LinePlot::IsDataSingleDirection(std::shared_ptr<const Data::Dataset>& data,
                                         const Data::GroupIdType group) const noexcept
        {
        wxASSERT_MSG(data, L"Null dataset passed to IsDataSingleDirection()");
        // this only makes sense with numeric data
        if (data == nullptr ||
            data->GetRowCount() == 0 ||
            IsXDates() ||
            IsXCategorical())
            { return true; }
        double currentX{ std::numeric_limits<double>::lowest() };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!m_useGrouping || group == m_groupColumn->GetValue(i))
                {
                if (IsXValid(i))
                    {
                    if (GetXValue(i) < currentX)
                        { return false; }
                    else
                        { currentX = GetXValue(i); }
                    }
                }
            }
        return true;
        }

    //----------------------------------------------------------------
    void LinePlot::AddLine(const LinePlot::Line& line)
        {
        if (GetData() == nullptr ||
            (GetData()->GetContinuousColumnValidN(m_yColumnName,
                                                  line.m_groupColumnName,
                                                  line.m_groupId) == 0))
            { return; }

        m_lines.push_back(line);

        const auto [fullYDataMin, fullYDataMax] = std::minmax_element(
            m_yColumn->GetValues().cbegin(),
            m_yColumn->GetValues().cend());
        const auto [minYValue, maxYValue] = m_useGrouping ?
            GetData()->GetContinuousMinMax(m_yColumnName,
                                           line.m_groupColumnName,
                                           line.m_groupId) :
            std::make_pair(*fullYDataMin, *fullYDataMax);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [yStart, yEnd] = adjust_intervals(minYValue, maxYValue);

        GetLeftYAxis().SetRange(
            (GetLineCount() > 1 ? std::min(yStart, yStartCurrent) : yStart),
            (GetLineCount() > 1 ? std::max(yEnd, yEndCurrent) : yEnd),
            // show precision if min or max have floating-point values
            ((get_mantissa(yStart) == 0 && get_mantissa(yEnd) == 0) ? 0 : 1),
            false);

        // X axis
        //-------

        if (IsXDates())
            {
            const auto [xStartCurrent, xEndCurrent] = GetBottomXAxis().GetRangeDates();

            const auto [minXValue, maxXValue] = GetXMinMaxDates();

            GetBottomXAxis().SetRange(minXValue, maxXValue);
            }
        else
            {
            const auto [xStartCurrent, xEndCurrent] = GetBottomXAxis().GetRange();

            const auto [minXValue, maxXValue] = GetXMinMax();

            GetBottomXAxis().SetRange(
                (GetLineCount() > 1 ? std::min(minXValue, xStartCurrent) : minXValue),
                (GetLineCount() > 1 ? std::max(maxXValue, xEndCurrent) : maxXValue),
                ((get_mantissa(minXValue) == 0 && get_mantissa(maxXValue) == 0) ? 0 : 1),
                false);

            // if we have a string table to work with, use that for the X axis labels
            if (IsXCategorical() &&
                m_xColumnCategorical->GetStringTable().size() > 0)
                {
                GetBottomXAxis().ClearCustomLabels();
                GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
                // customize the X axis labels
                for (const auto& label : m_xColumnCategorical->GetStringTable())
                    {
                    GetBottomXAxis().SetCustomLabel(label.first,
                        Label(label.second));
                    }
                }
            }

        UpdateCanvasForPoints();
        }

    //----------------------------------------------------------------
    void LinePlot::RecalcSizes(wxDC& dc)
        {
        // clear everything, update axes mirroring or whatever if requested by client
        Graph2D::RecalcSizes(dc);

        for (auto& line : m_lines)
            {
            auto points = std::make_shared<GraphItems::Points2D>(line.GetPen());
            points->SetScaling(GetScaling());
            points->SetDPIScaleFactor(GetDPIScaleFactor());
            points->SetLineStyle(line.GetStyle());
            points->Reserve(GetData()->GetRowCount());
            wxPoint pt;
            for (size_t i = 0; i < GetData()->GetRowCount(); ++i)
                {
                // skip value if from a different group
                if (m_useGrouping && m_groupColumn->GetValue(i) != line.m_groupId)
                    { continue; }
                // if explicitly missing data (i.e., NaN),
                // then add a bogus point to show a gap in the line
                if (!IsXValid(i) ||
                    std::isnan(m_yColumn->GetValue(i)))
                    {
                    points->AddPoint(Point2D(
                        GraphItemInfo().AnchorPoint(wxPoint(wxDefaultCoord, wxDefaultCoord)), 1), dc);
                    continue;
                    }
                if (!GetPhyscialCoordinates(GetXValue(i),
                                        m_yColumn->GetValue(i),
                                        pt))
                    { continue; }
                const wxColor ptColor = (m_colorIf ?
                    m_colorIf(GetXValue(i),
                              m_yColumn->GetValue(i)) :
                    line.GetPen().GetColour());
                points->AddPoint(Point2D(
                                    GraphItemInfo(GetData()->GetIdColumn().GetValue(i)).
                                    AnchorPoint(pt).
                                    Brush((ptColor.IsOk() ? ptColor : line.GetPen().GetColour())),
                                    Settings::GetPointRadius(),
                                    GetShapeScheme()->GetShape(line.m_groupId),
                                    &GetShapeScheme()->GetImage(line.m_groupId)), dc);
                }
            AddObject(points);
            }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> LinePlot::CreateLegend(const LegendOptions& options)
        {
        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        const bool showingMarkers = (GetShapeScheme()->GetShapes().size() >= m_lines.size() &&
                                    // multiple lines or one line and it is not using a blank icon
                                    (m_lines.size() > 1 ||
                                     GetShapeScheme()->GetShape(0) != IconShape::BlankIcon));
        wxString legendText;
        size_t lineCount{ 0 };
        for (const auto& line : m_lines)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = m_useGrouping ?
                m_groupColumn->GetCategoryLabelFromID(line.m_groupId) :
                wxString(L"");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                {
                currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            if (showingMarkers)
                {
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(GetShapeScheme()->GetShape(line.m_groupId), *wxBLACK,
                        line.GetPen().GetColour()));
                }
            else
                {
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::HorizontalLineIcon,
                        line.GetPen(),
                        line.GetPen().GetColour()));
                }
            ++lineCount;
            }
        if (options.IsIncludingHeader())
            {
            legendText.Prepend(wxString::Format(L"%s\n", m_groupColumn->GetTitle()));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, options.GetPlacementHint());
        return legend;
        }
    }
