///////////////////////////////////////////////////////////////////////////////
// Name:        lineplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lineplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LinePlot, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void LinePlot::SetData(std::shared_ptr<const Data::Dataset> data, const wxString& yColumnName,
                           const wxString& xColumnName,
                           std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);
        m_yColumnName = yColumnName;
        m_yColumn = GetDataset()->GetContinuousColumn(yColumnName);
        if (m_yColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': Y column not found for line plot."), yColumnName)
                    .ToUTF8());
            }
        // set the X column, which will be access through various GetX functions later
        // (do not reference these iterators after setting them here)

        // reset iterators
        m_xColumnContinuous = GetDataset()->GetContinuousColumns().cend();
        m_xColumnCategorical = GetDataset()->GetCategoricalColumns().cend();
        m_xColumnDate = GetDataset()->GetDateColumns().cend();
        // look for it as a continuous variable first
        m_xColumnContinuous = GetDataset()->GetContinuousColumn(xColumnName);
        if (m_xColumnContinuous == GetDataset()->GetContinuousColumns().cend())
            {
            // if not found, look for it as a categorical
            m_xColumnCategorical = GetDataset()->GetCategoricalColumn(xColumnName);
            if (m_xColumnCategorical == GetDataset()->GetCategoricalColumns().cend())
                {
                // try date columns
                m_xColumnDate = GetDataset()->GetDateColumn(xColumnName);
                if (m_xColumnDate == GetDataset()->GetDateColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': X column not found for line plot."), xColumnName)
                            .ToUTF8());
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

        if (IsUsingGrouping())
            {
            // create a reverse string table, with it sorted by label
            std::map<wxString, Data::GroupIdType, Data::wxStringLessNoCase> groups;
            for (const auto& [id, str] : GetGroupColumn()->GetStringTable())
                {
                groups.insert(std::make_pair(str, id));
                }
            size_t currentIndex{ 0 };
            for (const auto& group : groups)
                {
                Line ln;
                ln.SetGroupInfo(groupColumnName, group.second,
                                GetGroupColumn()->GetLabelFromID(group.second));
                ln.GetPen().SetColour(GetColorScheme()->GetColor(currentIndex));
                ln.m_shape = GetShapeScheme()->GetShape(currentIndex);
                ln.m_shapeImg = GetShapeScheme()->GetImage(currentIndex);
                // if some sort of spiral, then draw as a dashed spline
                if (IsAutoSplining() && !IsDataSingleDirection(data, group.second))
                    {
                    ln.GetPen().SetStyle(wxPenStyle::wxPENSTYLE_SHORT_DASH);
                    ln.SetStyle(LineStyle::Spline);
                    }
                else
                    {
                    const auto& [penStyle, lineStyle] =
                        GetPenStyleScheme()->GetLineStyle(currentIndex);
                    ln.GetPen().SetStyle(penStyle);
                    ln.SetStyle(lineStyle);
                    }
                AddLine(ln);
                ++currentIndex;
                }
            }
        else
            {
            Line ln;
            ln.SetGroupInfo(groupColumnName, 0, wxString{});
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
        }

    //-------------------------------------------
    bool LinePlot::IsDataSingleDirection(std::shared_ptr<const Data::Dataset> & data,
                                         const Data::GroupIdType group) const noexcept
        {
        assert(data && L"Null dataset passed to IsDataSingleDirection()");
        // this only makes sense with numeric data
        if (data == nullptr || data->GetRowCount() == 0 || IsXDates() || IsXCategorical())
            {
            return true;
            }
        double currentX{ std::numeric_limits<double>::lowest() };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!IsUsingGrouping() || group == GetGroupColumn()->GetValue(i))
                {
                if (IsXValid(i))
                    {
                    if (GetXValue(i) < currentX)
                        {
                        return false;
                        }
                    else
                        {
                        currentX = GetXValue(i);
                        }
                    }
                }
            }
        return true;
        }

    //----------------------------------------------------------------
    void LinePlot::AddLine(const LinePlot::Line& line)
        {
        if (GetDataset() == nullptr ||
            (GetDataset()->GetContinuousColumnValidN(m_yColumnName, line.m_groupColumnName,
                                                     line.m_groupId) == 0))
            {
            return;
            }

        m_lines.push_back(line);

        const auto [fullYDataMin, fullYDataMax] =
            std::minmax_element(m_yColumn->GetValues().cbegin(), m_yColumn->GetValues().cend());
        const auto [minYValue, maxYValue] =
            IsUsingGrouping() ? GetDataset()->GetContinuousMinMax(
                                    m_yColumnName, line.m_groupColumnName, line.m_groupId) :
                                std::make_pair(*fullYDataMin, *fullYDataMax);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [yStart, yEnd] = adjust_intervals(minYValue, maxYValue);

        GetLeftYAxis().SetRange((GetLineCount() > 1 ? std::min(yStart, yStartCurrent) : yStart),
                                (GetLineCount() > 1 ? std::max(yEnd, yEndCurrent) : yEnd),
                                // show precision if min or max have floating-point values
                                ((get_mantissa(yStart) == 0 && get_mantissa(yEnd) == 0) ? 0 : 1),
                                false);

        // X axis
        //-------

        if (IsXDates())
            {
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
                ((get_mantissa(minXValue) == 0 && get_mantissa(maxXValue) == 0) ? 0 : 1), false);

            // if we have a string table to work with, use that for the X axis labels
            if (IsXCategorical() && m_xColumnCategorical->GetStringTable().size() > 0)
                {
                GetBottomXAxis().ClearCustomLabels();
                GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
                // customize the X axis labels
                for (const auto& label : m_xColumnCategorical->GetStringTable())
                    {
                    GetBottomXAxis().SetCustomLabel(label.first, GraphItems::Label(label.second));
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void LinePlot::RecalcSizes(wxDC & dc)
        {
        // clear everything, update axes mirroring or whatever if requested by client
        Graph2D::RecalcSizes(dc);

        for (auto& line : m_lines)
            {
            auto points = std::make_unique<GraphItems::Points2D>(line.GetPen());
            points->SetScaling(GetScaling());
            points->SetDPIScaleFactor(GetDPIScaleFactor());
            points->SetLineStyle(line.GetStyle());
            points->Reserve(GetDataset()->GetRowCount());
            wxPoint pt;
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                // skip value if from a different group
                if (IsUsingGrouping() && GetGroupColumn()->GetValue(i) != line.m_groupId)
                    {
                    continue;
                    }
                // if explicitly missing data (i.e., NaN),
                // then add a bogus point to show a gap in the line
                if (!IsXValid(i) || std::isnan(m_yColumn->GetValue(i)))
                    {
                    points->AddPoint(
                        GraphItems::Point2D(GraphItems::GraphItemInfo().AnchorPoint(
                                                wxPoint(wxDefaultCoord, wxDefaultCoord)),
                                            1),
                        dc);
                    continue;
                    }
                if (!GetPhysicalCoordinates(GetXValue(i), m_yColumn->GetValue(i), pt))
                    {
                    continue;
                    }
                const wxColor ptColor =
                    (m_colorIf ? m_colorIf(GetXValue(i), m_yColumn->GetValue(i)) :
                                 line.GetPen().GetColour());
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt)
                            .Brush((ptColor.IsOk() ? ptColor : line.GetPen().GetColour())),
                        Settings::GetPointRadius(), line.m_shape, &line.m_shapeImg),
                    dc);
                }
            AddObject(std::move(points));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> LinePlot::CreateLegend(const LegendOptions& options)
        {
        if (!IsUsingGrouping())
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo()
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor()));

        const bool showingMarkers =
            (GetShapeScheme()->GetShapes().size() >= m_lines.size() &&
             // multiple lines or one line and it is not using a blank icon
             (m_lines.size() > 1 || GetShapeScheme()->GetShape(0) != Icons::IconShape::Blank));
        wxString legendText;
        size_t lineCount{ 0 };
        for (const auto& line : m_lines)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = GetGroupColumn()->GetLabelFromID(line.m_groupId);
            assert(Settings::GetMaxLegendTextLength() >= 1 && L"Max legend text length is zero?!");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
                Settings::GetMaxLegendTextLength() >= 1)
                {
                currentLabel.erase(Settings::GetMaxLegendTextLength() - 1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            if (showingMarkers)
                {
                legend->GetLegendIcons().push_back(
                    Icons::LegendIcon(line.m_shape, *wxBLACK, line.GetPen().GetColour()));
                }
            else
                {
                legend->GetLegendIcons().push_back(Icons::LegendIcon(
                    Icons::IconShape::HorizontalLine, line.GetPen(), line.GetPen().GetColour()));
                }
            ++lineCount;
            }
        if (options.IsIncludingHeader())
            {
            legendText.Prepend(wxString::Format(L"%s\n", GetGroupColumn()->GetName()));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
