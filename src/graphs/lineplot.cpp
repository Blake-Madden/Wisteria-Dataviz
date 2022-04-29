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
    //-------------------------------------------
    void LinePlot::Line::SetData(std::shared_ptr<const Data::Dataset> data,
                                 const wxString& yColumnName,
                                 const wxString& xColumnName,
                                 std::optional<const wxString>& groupColumnName,
                                 const Data::GroupIdType groupId)
        {
        if (data == nullptr)
            { return; }

        m_data = data;
        m_groupId = groupId;

        m_yColumnName = yColumnName;
        m_xColumnName = xColumnName;
        m_groupColumnName = groupColumnName;

        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for line plot."), groupColumnName.value()));
            }
        m_yColumn = m_data->GetContinuousColumn(yColumnName);
        if (m_yColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': Y column not found for line plot."), yColumnName));
            }
        m_xColumn = m_data->GetContinuousColumn(xColumnName);
        if (m_yColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': X column not found for line plot."), xColumnName));
            }

        m_label = (groupColumnName.has_value() ?
            m_groupColumn->GetCategoryLabel(m_groupId) : wxString(L""));
        }

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
        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for line plot"), groupColumnName.value()));
            }
        if (m_data->GetContinuousColumn(yColumnName) == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': Y column not found for line plot"), yColumnName));
            }
        m_xColumn = m_data->GetContinuousColumn(xColumnName);
        if (m_xColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': X column not found for line plot"), xColumnName));
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
                ln.SetData(m_data, yColumnName, xColumnName, groupColumnName, group);
                ln.GetPen().SetColour(GetColorScheme()->GetColor(group));
                // if some sort of spiral, then draw as a dashed spline
                if (!IsDataSingleDirection(data, group))
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
            ln.SetData(m_data, yColumnName, xColumnName, groupColumnName, 0);
            ln.GetPen().SetColour(GetColorScheme()->GetColor(0));
            if (!IsDataSingleDirection(data, 0))
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
                    return m_groupColumn->GetCategoryLabel(first.m_groupId) <
                           m_groupColumn->GetCategoryLabel(second.m_groupId);
                    }
                );
            }
        }

    //-------------------------------------------
    bool LinePlot::IsDataSingleDirection(std::shared_ptr<const Data::Dataset>& data,
                                         const Data::GroupIdType group) const noexcept
        {
        wxASSERT_MSG(data, L"Null dataset passed to IsDataSingleDirection()");
        if (data == nullptr)
            { return false; }
        if (data->GetRowCount() == 0)
            { return true; }
        double currentX{ std::numeric_limits<double>::min() };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!m_useGrouping || group == m_groupColumn->GetValue(i))
                {
                if (!std::isnan(m_xColumn->GetValue(i)))
                    {
                    if (m_xColumn->GetValue(i) < currentX)
                        { return false; }
                    else
                        { currentX = m_xColumn->GetValue(i); }
                    }
                }
            }
        return true;
        }

    //----------------------------------------------------------------
    void LinePlot::AddLine(const LinePlot::Line& line)
        {
        if (line.GetData() == nullptr ||
            (line.GetData()->GetContinuousColumnValidN(line.m_yColumnName,
                                                    line.m_groupColumnName,
                                                    line.m_groupId) == 0) ||
            (line.GetData()->GetContinuousColumnValidN(line.m_xColumnName,
                                                    line.m_groupColumnName,
                                                    line.m_groupId) == 0))
            { return; }

        m_lines.push_back(line);

        const auto [fullYDataMin, fullYDataMax] = std::minmax_element(
            line.m_yColumn->GetValues().cbegin(),
            line.m_yColumn->GetValues().cend());
        const auto [minYValue, maxYValue] = m_useGrouping ?
            line.GetData()->GetContinuousMinMax(line.m_yColumnName,
                                                line.m_groupColumnName,
                                                line.m_groupId) :
            std::make_pair(*fullYDataMin, *fullYDataMax);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [yStart, yEnd] = adjust_intervals(minYValue, maxYValue);

        const auto [xStartCurrent, xEndCurrent] = GetBottomXAxis().GetRange();

        GetLeftYAxis().SetRange(
            (GetLineCount() > 1 ? std::min(yStart, yStartCurrent) : yStart),
            (GetLineCount() > 1 ? std::max(yEnd, yEndCurrent) : yEnd),
            // show precision if min or max have floating-point values
            ((get_mantissa(yStart) == 0 && get_mantissa(yEnd) == 0) ? 0 : 1),
            false);

        const auto [fullXDataMin, fullXDataMax] = std::minmax_element(
            line.m_xColumn->GetValues().cbegin(),
            line.m_xColumn->GetValues().cend());
        const auto [minXValue, maxXValue] = m_useGrouping ?
            line.GetData()->GetContinuousMinMax(line.m_xColumnName,
                                                line.m_groupColumnName,
                                                line.m_groupId) :
            std::make_pair(*fullXDataMin, *fullXDataMax);

        GetBottomXAxis().SetRange(
            (GetLineCount() > 1 ? std::min(minXValue, xStartCurrent) : minXValue),
            (GetLineCount() > 1 ? std::max(maxXValue, xEndCurrent) : maxXValue),
            ((get_mantissa(minXValue) == 0 && get_mantissa(maxXValue) == 0) ? 0 : 1),
            false);

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
            points->Reserve(line.GetData()->GetRowCount());
            wxPoint pt;
            for (size_t i = 0; i < line.GetData()->GetRowCount(); ++i)
                {
                // skip value if from a different group
                if (m_useGrouping && line.m_groupColumn->GetValue(i) != line.m_groupId)
                    { continue; }
                // if explicitly missing data (i.e., NaN),
                // then add a bogus point to show a gap in the line
                if (std::isnan(line.m_xColumn->GetValue(i)) ||
                    std::isnan(line.m_yColumn->GetValue(i)))
                    {
                    points->AddPoint(Point2D(
                        GraphItemInfo().AnchorPoint(wxPoint(wxDefaultCoord, wxDefaultCoord)), 1), dc);
                    continue;
                    }
                if (!GetPhyscialCoordinates(line.m_xColumn->GetValue(i),
                                        line.m_yColumn->GetValue(i),
                                        pt))
                    { continue; }
                const wxColor ptColor = (m_colorIf ?
                    m_colorIf(line.m_xColumn->GetValue(i),
                              line.m_yColumn->GetValue(i)) :
                    line.GetPen().GetColour());
                points->AddPoint(Point2D(
                                    GraphItemInfo(line.GetData()->GetIdColumn().GetValue(i)).
                                    AnchorPoint(pt).
                                    Brush((ptColor.IsOk() ? ptColor : line.GetPen().GetColour())),
                                    Settings::GetPointRadius(),
                                    GetShapeScheme()->GetShape(line.m_groupId),
                                    &GetShapeScheme()->GetImage()), dc);
                }
            AddObject(points);
            }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> LinePlot::CreateLegend(
        const LegendCanvasPlacementHint hint, const bool includeHeader)
        {
        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidth()).
            DPIScaling(GetDPIScaleFactor()));
        legend->SetBoxCorners(BoxCorners::Rounded);

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
                m_groupColumn->GetCategoryLabel(line.m_groupId) :
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
        if (includeHeader)
            {
            legendText.Prepend(wxString::Format(L"%s\n", m_groupColumn->GetTitle()));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
