///////////////////////////////////////////////////////////////////////////////
// Name:        lineplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lineplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LinePlot, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void LinePlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& yColumnName, const wxString& xColumnName,
                           const std::optional<wxString>& groupColumnName)
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
                wxString::Format(_(L"'%s': y column not found for line plot."), yColumnName)
                    .ToUTF8());
            }
        // set the x column, which will be accessed through various GetX functions later
        // (do not reference these iterators after setting them here)
        SetXColumn(xColumnName);
        GetLines().clear();
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
                ln.SetShape(GetShapeScheme()->GetShape(currentIndex));
                ln.SetShapeImage(GetShapeScheme()->GetImage(currentIndex));
                // if some sort of spiral, then draw as a dashed spline
                if (IsAutoSplining() && !IsDataSingleDirection(GetDataset(), group.second))
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
            if (IsAutoSplining() && !IsDataSingleDirection(GetDataset(), 0))
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
    bool LinePlot::IsDataSingleDirection(const std::shared_ptr<const Data::Dataset>& data,
                                         const Data::GroupIdType group) const
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
                    currentX = GetXValue(i);
                    }
                }
            }
        return true;
        }

    //----------------------------------------------------------------
    void LinePlot::AddLine(const LinePlot::Line& line)
        {
        if (GetDataset() == nullptr ||
            (GetDataset()->GetContinuousColumnValidN(m_yColumnName, line.GetGroupColumnName(),
                                                     line.GetGroupId()) == 0))
            {
            return;
            }

        GetLines().push_back(line);

        const auto [fullYDataMin, fullYDataMax] =
            std::minmax_element(m_yColumn->GetValues().cbegin(), m_yColumn->GetValues().cend());
        const auto [minYValue, maxYValue] =
            IsUsingGrouping() ? GetDataset()->GetContinuousMinMax(
                                    m_yColumnName, line.GetGroupColumnName(), line.GetGroupId()) :
                                std::make_pair(*fullYDataMin, *fullYDataMax);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [yStart, yEnd] = adjust_intervals(minYValue, maxYValue);

        GetLeftYAxis().SetRange((GetLineCount() > 1 ? std::min(yStart, yStartCurrent) : yStart),
                                (GetLineCount() > 1 ? std::max(yEnd, yEndCurrent) : yEnd),
                                // show precision if min or max have floating-point values
                                ((get_mantissa(yStart) == 0 && get_mantissa(yEnd) == 0) ? 0 : 1),
                                false);

        // x-axis
        //-------

        if (IsXDates())
            {
            const auto [minXValue, maxXValue] = GetXMinMaxDates();

            GetBottomXAxis().SetDateRange(minXValue, maxXValue);
            }
        else
            {
            const auto [xStartCurrent, xEndCurrent] = GetBottomXAxis().GetRange();

            const auto [minXValue, maxXValue] = GetXMinMax();

            GetBottomXAxis().SetRange(
                (GetLineCount() > 1 ? std::min(minXValue, xStartCurrent) : minXValue),
                (GetLineCount() > 1 ? std::max(maxXValue, xEndCurrent) : maxXValue),
                ((get_mantissa(minXValue) == 0 && get_mantissa(maxXValue) == 0) ? 0 : 1), false);

            // if we have a string table to work with, use that for the x-axis labels
            if (IsXCategorical() && !GetXCategoricalColumnIterator()->GetStringTable().empty())
                {
                GetBottomXAxis().ClearCustomLabels();
                GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
                // customize the x-axis labels
                for (const auto& label : GetXCategoricalColumnIterator()->GetStringTable())
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

        for (auto& line : GetLines())
            {
            auto points = std::make_unique<GraphItems::Points2D>(line.GetPen());
            points->SetScaling(GetScaling());
            points->SetDPIScaleFactor(GetDPIScaleFactor());
            points->SetLineStyle(line.GetStyle());
            points->Reserve(GetDataset()->GetRowCount());
            const bool isLineGhosted = (IsUsingGrouping() && !GetShowcasedLines().empty() &&
                                        std::ranges::find(GetShowcasedLines(), line.GetText()) ==
                                            GetShowcasedLines().cend());
            if (isLineGhosted)
                {
                points->Ghost(true);
                points->SetGhostOpacity(GetGhostOpacity());
                }
            wxPoint pt;
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                // skip value if from a different group
                if (IsUsingGrouping() && GetGroupColumn()->GetValue(i) != line.GetGroupId())
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
                const wxColor ptColor = GetMaybeGhostedColor(
                    m_colorIf ? m_colorIf(GetXValue(i), m_yColumn->GetValue(i)) :
                                line.GetPen().GetColour(),
                    isLineGhosted);
                points->AddPoint(
                    GraphItems::Point2D{
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt)
                            .Pen(GetMaybeGhostedColor(line.GetPen().GetColour(), isLineGhosted))
                            .Brush((ptColor.IsOk() ? ptColor :
                                                     GetMaybeGhostedColor(line.GetPen().GetColour(),
                                                                          isLineGhosted))),
                        Settings::GetPointRadius(), line.GetShape(),
                        std::make_unique<wxBitmapBundle>(line.GetShapeImage()) },
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
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        const bool showingMarkers =
            (GetShapeScheme()->GetShapes().size() >= GetLines().size() &&
             // multiple lines (or one line) and it is not using a blank icon
             (GetLines().size() > 1 || GetShapeScheme()->GetShape(0) != Icons::IconShape::Blank));
        wxString legendText;
        size_t lineCount{ 0 };
        for (const auto& line : GetLines())
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = GetGroupColumn()->GetLabelFromID(line.GetGroupId());
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
                legend->GetLegendIcons().emplace_back(
                    line.GetShape(), Colors::ColorBrewer::GetColor(Colors::Color::Black),
                    line.GetPen().GetColour());
                }
            else
                {
                legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine,
                                                      line.GetPen(), line.GetPen().GetColour());
                }
            ++lineCount;
            }
        if (options.IsIncludingHeader())
            {
            legendText.Prepend(wxString::Format(L"%s\n", GetGroupColumn()->GetName()));
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    void LinePlot::SetXColumn(const wxString& xColumnName)
        {
        ResetXColumns();
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
                        wxString::Format(_(L"'%s': x column not found for line plot."), xColumnName)
                            .ToUTF8());
                    }
                }
            }
        }
    } // namespace Wisteria::Graphs
