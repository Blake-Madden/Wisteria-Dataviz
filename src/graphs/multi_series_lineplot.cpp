///////////////////////////////////////////////////////////////////////////////
// Name:        multi_series_lineplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "multi_series_lineplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::MultiSeriesLinePlot, Wisteria::Graphs::LinePlot)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void MultiSeriesLinePlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                      const std::vector<wxString>& yColumnNames,
                                      const wxString& xColumnName)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();
        m_yColumns.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        m_yColumnNames = yColumnNames;
        for (const auto& yColumnName : m_yColumnNames)
            {
            auto yColumn = GetDataset()->GetContinuousColumn(yColumnName);
            if (yColumn == GetDataset()->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': Y column not found for multi-series line plot."),
                                     yColumnName)
                        .ToUTF8());
                }
            m_yColumns.push_back(yColumn);
            }
        // set the X column, which will be access through various GetX functions later
        // (do not reference these iterators after setting them here)
        SetXColumn(xColumnName);
        GetLines().clear();
        GetLeftYAxis().Reset();
        GetRightYAxis().Reset();
        GetBottomXAxis().Reset();
        GetTopXAxis().Reset();
        GetBottomXAxis().GetTitle().SetText(xColumnName);
        wxString yLabel;
        for (const auto& yLab : yColumnNames)
            {
            yLabel.append(yLab).append(L", ");
            }
        if (yLabel.length() >= 2)
            {
            yLabel.erase(yLabel.length() - 2);
            }
        GetLeftYAxis().GetTitle().SetText(yLabel);

        for (auto i = 0; i < m_yColumnNames.size(); ++i)
            {
            Line ln;
            ln.SetGroupInfo(std::nullopt, 0, m_yColumnNames[i]);
            ln.GetPen().SetColour(GetColorScheme()->GetColor(i));
            if (IsAutoSplining() && !IsDataSingleDirection(GetDataset(), 0))
                {
                ln.GetPen().SetStyle(wxPenStyle::wxPENSTYLE_SHORT_DASH);
                ln.SetStyle(LineStyle::Spline);
                }
            else
                {
                const auto& [penStyle, lineStyle] = GetPenStyleScheme()->GetLineStyle(i);
                ln.GetPen().SetStyle(penStyle);
                ln.SetStyle(lineStyle);
                }
            AddLine(ln, m_yColumnNames[i]);
            }
        }

    //----------------------------------------------------------------
    void MultiSeriesLinePlot::AddLine(const LinePlot::Line& line, const wxString& yColumnName)
        {
        if (GetDataset() == nullptr ||
            (GetDataset()->GetContinuousColumnValidN(yColumnName, std::nullopt, std::nullopt) == 0))
            {
            return;
            }

        const auto foundYColumn = std::ranges::find_if(
            m_yColumns, [&yColumnName = std::as_const(yColumnName)](const auto& colIter)
            { return colIter->GetName() == yColumnName; });
        if (foundYColumn == m_yColumns.cend())
            {
            return;
            }

        GetLines().push_back(line);

        const auto [fullYDataMin, fullYDataMax] =
            std::ranges::minmax_element((*foundYColumn)->GetValues());
        const auto [minYValue, maxYValue] = std::make_pair(*fullYDataMin, *fullYDataMax);

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
            if (IsXCategorical() && !GetXCategoricalColumnIterator()->GetStringTable().empty())
                {
                GetBottomXAxis().ClearCustomLabels();
                GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
                // customize the X axis labels
                for (const auto& label : GetXCategoricalColumnIterator()->GetStringTable())
                    {
                    GetBottomXAxis().SetCustomLabel(label.first, GraphItems::Label(label.second));
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void MultiSeriesLinePlot::RecalcSizes(wxDC & dc)
        {
        // clear everything, update axes mirroring or whatever if requested by client
        Graph2D::RecalcSizes(dc);

        for (auto colCounter = 0; colCounter < m_yColumns.size(); ++colCounter)
            {
            wxASSERT_MSG(colCounter < GetLines().size(),
                         L"Not enough lines defined in MultiSeriesLinePlot!");
            if (colCounter >= GetLines().size())
                {
                break;
                }
            auto points = std::make_unique<GraphItems::Points2D>(GetLines()[colCounter].GetPen());
            points->SetScaling(GetScaling());
            points->SetDPIScaleFactor(GetDPIScaleFactor());
            points->SetLineStyle(GetLines()[colCounter].GetStyle());
            points->Reserve(GetDataset()->GetRowCount());
            const bool isLineGhosted =
                (!GetShowcasedLines().empty() &&
                 std::ranges::find(GetShowcasedLines(), GetLines()[colCounter].GetText()) ==
                     GetShowcasedLines().cend());
            if (isLineGhosted)
                {
                points->Ghost(true);
                points->SetGhostOpacity(GetGhostOpacity());
                }
            wxPoint pt;
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                // if explicitly missing data (i.e., NaN),
                // then add a bogus point to show a gap in the line
                if (!IsXValid(i) || std::isnan(m_yColumns[colCounter]->GetValue(i)))
                    {
                    points->AddPoint(
                        GraphItems::Point2D(GraphItems::GraphItemInfo().AnchorPoint(
                                                wxPoint(wxDefaultCoord, wxDefaultCoord)),
                                            1),
                        dc);
                    continue;
                    }
                if (!GetPhysicalCoordinates(GetXValue(i), m_yColumns[colCounter]->GetValue(i), pt))
                    {
                    continue;
                    }
                const wxColor ptColor = GetMaybeGhostedColor(
                    GetColorIf() ? GetColorIf()(GetXValue(i), m_yColumns[colCounter]->GetValue(i)) :
                                   GetLines()[colCounter].GetPen().GetColour(),
                    isLineGhosted);
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt)
                            .Pen(GetMaybeGhostedColor(GetLines()[colCounter].GetPen().GetColour(),
                                                      isLineGhosted))
                            .Brush((ptColor.IsOk() ?
                                        ptColor :
                                        GetMaybeGhostedColor(
                                            GetLines()[colCounter].GetPen().GetColour(),
                                            isLineGhosted))),
                        Settings::GetPointRadius(), GetLines()[colCounter].GetShape(),
                        &GetLines()[colCounter].GetShapeImage()),
                    dc);
                }
            AddObject(std::move(points));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> MultiSeriesLinePlot::CreateLegend(
        const LegendOptions& options)
        {
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
            wxString currentLabel = line.GetText();
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
                legend->GetLegendIcons().emplace_back(line.GetShape(), *wxBLACK,
                                                      line.GetPen().GetColour());
                }
            else
                {
                legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine,
                                                      line.GetPen(), line.GetPen().GetColour());
                }
            ++lineCount;
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
