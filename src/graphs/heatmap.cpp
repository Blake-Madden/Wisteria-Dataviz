///////////////////////////////////////////////////////////////////////////////
// Name:        heatmap.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "heatmap.h"
#include "../util/frequencymap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::HeatMap, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    HeatMap::HeatMap(Wisteria::Canvas * canvas,
                     const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
                                           std::make_unique<Colors::Schemes::ColorScheme>(
                                               Colors::Schemes::ColorScheme{ *wxWHITE, *wxBLACK }));

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        GetPen().SetColour(L"#BEBBBB");
        }

    //----------------------------------------------------------------
    void HeatMap::SetData(const std::shared_ptr<const Data::Dataset>& data,
                          const wxString& continuousColumnName,
                          const std::optional<wxString>& groupColumnName /*= std::nullopt*/,
                          const std::optional<size_t> groupColumnCount /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);
        m_continuousColumn = GetDataset()->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': continuous column not found for heatmap."),
                                 continuousColumnName)
                    .ToUTF8());
            }
        m_groupColumnCount =
            (groupColumnCount ? std::clamp<size_t>(groupColumnCount.value(), 1, 5) : 1);
        m_matrix.clear();
        m_range = { 0, 0 };

        if (GetDataset()->GetContinuousColumns().empty())
            {
            wxFAIL_MSG(L"Heatmap requires a continuous column to analyze!");
            SetDataset(nullptr);
            return;
            }

        // prepare the colors
        m_reversedColorSpectrum = GetColorScheme()->GetColors();
        std::ranges::reverse(m_reversedColorSpectrum);

        Colors::ColorBrewer cb;
        cb.SetColorScale(GetColorScheme()->GetColors().cbegin(),
                         GetColorScheme()->GetColors().cend());
        const auto cellColors = cb.BrewColors(m_continuousColumn->GetValues().cbegin(),
                                              m_continuousColumn->GetValues().cend());
        m_range = cb.GetRange();

        const wxString crossedOutSymbolForNaN{ L"\u274C" };
        if (IsUsingGrouping())
            {
            // see how many groups there are
            frequency_set<Data::GroupIdType> groups;
            for (const auto& groupId : GetGroupColumn()->GetValues())
                {
                groups.insert(groupId);
                }
            // if more columns than groups, then fix the column count
            m_groupColumnCount = std::min(m_groupColumnCount, groups.get_data().size());
            m_matrix.resize(groups.get_data().size());
            const auto maxItemByColumnCount = std::ranges::max_element(
                groups.get_data(), [](const auto& item1, const auto& item2) noexcept
                { return item1.second < item2.second; });
            for (auto& row : m_matrix)
                {
                row.resize(maxItemByColumnCount->second);
                }

            size_t currentRow{ 0 }, currentColumn{ 0 };
            auto currentGroupId = GetGroupColumn()->GetValue(0);
            for (size_t i = 0; i < cellColors.size(); ++i)
                {
                // move to next row if on another group ID
                if (GetGroupColumn()->GetValue(i) != currentGroupId)
                    {
                    ++currentRow;
                    currentColumn = 0;
                    currentGroupId = GetGroupColumn()->GetValue(i);
                    }
                assert(currentRow < m_matrix.size() &&
                       L"Invalid row when filling heatmap matrix! "
                       "Data should be sorted by group before calling SetData().!");
                assert(currentColumn < m_matrix[currentRow].size() &&
                       L"Invalid column when filling heatmap matrix!");
                // should not happen, just do this to prevent crash if data was not sorted by
                // value and then by group first. What's displayed if this happens is the data
                // won't be grouped properly, but it's showing it how the client passed it in.
                if (currentRow >= m_matrix.size())
                    {
                    std::vector<HeatCell> newRow;
                    newRow.resize(maxItemByColumnCount->second);
                    m_matrix.push_back(newRow);
                    }
                // shouldn't happen, just done as sanity check
                if (currentRow >= m_matrix.size() || currentColumn >= m_matrix[currentRow].size())
                    {
                    break;
                    }
                m_matrix[currentRow][currentColumn].m_color = cellColors[i];
                m_matrix[currentRow][currentColumn].m_valueLabel =
                    (std::isnan(m_continuousColumn->GetValue(i)) ?
                         crossedOutSymbolForNaN :
                         wxNumberFormatter::ToString(m_continuousColumn->GetValue(i), 1,
                                                     Settings::GetDefaultNumberFormat()));
                m_matrix[currentRow][currentColumn].m_selectionLabel =
                    GetDataset()->GetIdColumn().GetValue(i);
                m_matrix[currentRow][currentColumn].m_groupId = GetGroupColumn()->GetValue(i);
                ++currentColumn;
                }
            }
        else
            {
            // Prepare the (rectangular [graphs are usually viewed in landscape]) matrix of color
            // cells. If there are 10 or fewer items, then just keep them all on one row.
            const size_t cellColumnCount =
                (m_continuousColumn->GetRowCount() <= 10) ?
                    10 :
                    std::ceil(std::sqrt(m_continuousColumn->GetRowCount())) *
                        math_constants::golden_ratio;
            const size_t cellRowCount =
                std::ceil(safe_divide<double>(m_continuousColumn->GetRowCount(), cellColumnCount));

            m_matrix.resize(cellRowCount);
            for (auto& row : m_matrix)
                {
                row.resize(cellColumnCount);
                }

            size_t currentRow{ 0 }, currentColumn{ 0 };
            for (size_t i = 0; i < cellColors.size(); ++i)
                {
                // move to next row, if needed
                if (currentColumn >= cellColumnCount)
                    {
                    ++currentRow;
                    currentColumn = 0;
                    }
                assert(currentRow < m_matrix.size() && L"Invalid row when filling heatmap matrix!");
                assert(currentColumn < m_matrix[currentRow].size() &&
                       L"Invalid column when filling heatmap matrix!");
                // shouldn't happen, just done as sanity check
                if (currentRow >= m_matrix.size() || currentColumn >= m_matrix[currentRow].size())
                    {
                    break;
                    }
                m_matrix[currentRow][currentColumn].m_color = cellColors[i];
                m_matrix[currentRow][currentColumn].m_valueLabel =
                    (std::isnan(m_continuousColumn->GetValue(i)) ?
                         crossedOutSymbolForNaN :
                         wxNumberFormatter::ToString(m_continuousColumn->GetValue(i), 1,
                                                     Settings::GetDefaultNumberFormat()));
                m_matrix[currentRow][currentColumn].m_selectionLabel =
                    GetDataset()->GetIdColumn().GetValue(i);
                // ignored, just default to zero
                m_matrix[currentRow][currentColumn].m_groupId = 0;
                ++currentColumn;
                }
            }
        }

    //----------------------------------------------------------------
    void HeatMap::RecalcSizes(wxDC & dc)
        {
        // if no data then bail
        if (GetDataset() == nullptr || GetDataset()->GetRowCount() == 0 || m_matrix.empty())
            {
            return;
            }

        Graph2D::RecalcSizes(dc);

        const auto maxRowsWhenGrouping =
            std::ceil(safe_divide<double>(m_matrix.size(), m_groupColumnCount));

        constexpr wxCoord LABEL_RIGHT_PADDING{ 4 };

        // size the boxes to fit in the area available
        wxRect drawArea = GetPlotAreaBoundingBox();
        const auto padding = wxSizerFlags::GetDefaultBorder() * GetScaling();
        wxCoord groupHeaderLabelHeight{ 0 };
        wxFont groupHeaderLabelFont{ GetBottomXAxis().GetFont() };
        bool groupHeaderLabelMultiline{ false };

        // find the width of the longest group label
        GraphItems::Label measuringLabel(GraphItems::GraphItemInfo()
                                             .Scaling(GetScaling())
                                             .Pen(wxNullPen)
                                             .DPIScaling(GetDPIScaleFactor()));
        wxCoord widestLabelWidth{ 0 };
        wxString widestStr;
        if (IsUsingGrouping())
            {
            for (const auto& [id, strVal] : GetGroupColumn()->GetStringTable())
                {
                measuringLabel.SetText(strVal);
                if (widestLabelWidth < measuringLabel.GetBoundingBox(dc).GetWidth())
                    {
                    widestStr = strVal;
                    }
                widestLabelWidth =
                    std::max(measuringLabel.GetBoundingBox(dc).GetWidth(), widestLabelWidth);
                }
            }
        const bool hasGroupLabels{ IsUsingGrouping() &&
                                   !GetGroupColumn()->GetStringTable().empty() };
        const auto groupLabelWidth{ hasGroupLabels ? widestLabelWidth : 0 };
        if (IsUsingGrouping() && m_matrix.size() > 1)
            {
            // if multiple columns, set the size of the drawing area to a column
            // (minus padding around it)
            if (m_groupColumnCount > 1)
                {
                drawArea.SetWidth(
                    safe_divide<size_t>(GetPlotAreaBoundingBox().GetWidth(), m_groupColumnCount) -
                    // border between areas
                    padding);
                }
            drawArea.SetWidth(drawArea.GetWidth() - groupLabelWidth);
            // Free some space for the group labels above each column (even if one column).
            // First, label might be too long, so get best fitting font and measure again.
            GraphItems::Label groupHeaderLabelTemplate(
                GraphItems::GraphItemInfo(wxString::Format(L"%s %zu-%zu", GetGroupHeaderPrefix(),
                                                           // largest possible range
                                                           GetDataset()->GetRowCount(),
                                                           GetDataset()->GetRowCount()))
                    .Scaling(GetScaling())
                    .Pen(wxNullPen)
                    .DPIScaling(GetDPIScaleFactor())
                    .Padding(0, 0, LABEL_RIGHT_PADDING, 0)
                    .Font(groupHeaderLabelFont));
            // try to keep the axis font size, but use smaller font if necessary
            groupHeaderLabelFont.SetPointSize(
                std::min(groupHeaderLabelFont.GetPointSize(),
                         GraphItems::Label::CalcFontSizeToFitBoundingBox(
                             dc, groupHeaderLabelFont,
                             drawArea /* really just needing the width measurement */,
                             groupHeaderLabelTemplate.GetText())));

            // remeasure with adjusted font
            if (IsShowingGroupHeaders())
                {
                groupHeaderLabelTemplate.GetFont() = groupHeaderLabelFont;
                const auto measuredSize = groupHeaderLabelTemplate.GetBoundingBox(dc);
                groupHeaderLabelHeight = measuredSize.GetHeight();
                // still too wide, so make it multiline
                if (measuredSize.GetWidth() > drawArea.GetWidth())
                    {
                    groupHeaderLabelTemplate.SetText(
                        wxString::Format(L"%s\n%zu-%zu", GetGroupHeaderPrefix(),
                                         GetDataset()->GetRowCount(), GetDataset()->GetRowCount()));
                    groupHeaderLabelHeight =
                        groupHeaderLabelTemplate.GetBoundingBox(dc).GetHeight();
                    groupHeaderLabelMultiline = true;
                    // readjust font size now that it is multiline and can be larger now
                    groupHeaderLabelFont.SetPointSize(std::max(
                        GetBottomXAxis().GetFont().GetPointSize(),
                        GraphItems::Label::CalcFontSizeToFitBoundingBox(
                            dc, groupHeaderLabelFont,
                            GraphItems::Polygon::DownScaleRect(
                                wxRect{ wxSize(groupHeaderLabelHeight, drawArea.GetWidth()) },
                                GetScaling()),
                            groupHeaderLabelTemplate.GetText())));
                    }
                }

            drawArea.SetHeight(drawArea.GetHeight() - groupHeaderLabelHeight);
            drawArea.Offset(wxPoint(groupLabelWidth, groupHeaderLabelHeight));
            }

        const auto boxWidth =
            IsUsingGrouping() ?
                std::min(
                    safe_divide<wxCoord>(drawArea.GetHeight(), maxRowsWhenGrouping),
                    safe_divide<wxCoord>(drawArea.GetWidth(),
                                         ((m_groupColumnCount > 1) ? m_matrix[0].size() :
                                                                     // just one column?
                                                                     // keep boxes from being huge
                                              std::max<size_t>(m_matrix[0].size(), 5)))) :
                // If being drawn rectangularly (i.e., not grouped), prevent the boxes from
                // being larger than a fifth of the area's width;
                // having only a few cells would cause the boxes to be a
                // ridiculously huge size.
                std::min(safe_divide<wxCoord>(drawArea.GetHeight(), m_matrix.size()),
                         safe_divide<wxCoord>(drawArea.GetWidth(),
                                              std::max<size_t>(m_matrix[0].size(), 5)));

        // get the best font size to fit the row labels
        wxFont groupLabelFont{ GetBottomXAxis().GetFont() };
        groupLabelFont.SetPointSize( // fit font as best possible
            GraphItems::Label::CalcFontSizeToFitBoundingBox(
                dc, groupLabelFont,
                wxSize(widestLabelWidth - ScaleToScreenAndCanvas(LABEL_RIGHT_PADDING), boxWidth),
                widestStr));
        // and the labels on the boxes
        wxFont boxLabelFont{ GetBottomXAxis().GetFont() };
        boxLabelFont.SetPointSize( // fit font as best possible
            GraphItems::Label::CalcFontSizeToFitBoundingBox(
                dc, boxLabelFont, wxSize(boxWidth, boxWidth),
                wxNumberFormatter::ToString(m_range.second /* largest value in the range*/, 1,
                                            Settings::GetDefaultNumberFormat())));

        // draw the boxes in a grid, row x column
        wxPoint pts[4];
        size_t currentRow{ 0 }, currentColumn{ 0 };
        size_t currentGroupStart{ 0 };
        // go through the row's columns
        const wxString groupHeaderFormatString =
            (groupHeaderLabelMultiline ? L"%s\n%zu-%zu" : L"%s %zu-%zu");
        const wxString singleGroupHeaderFormatString =
            (groupHeaderLabelMultiline ? L"%s\n%zu" : L"%s %zu");
        for (const auto& row : m_matrix)
            {
            if (currentRow == 0 && IsShowingGroupHeaders() && IsUsingGrouping() &&
                m_matrix.size() > 1)
                {
                // If only one group in column, then don't show that as a range;
                // otherwise, show as a range.
                const wxString headerString =
                    (currentGroupStart + 1 ==
                     std::min<size_t>(maxRowsWhenGrouping + currentGroupStart, m_matrix.size())) ?
                        wxString::Format(singleGroupHeaderFormatString, GetGroupHeaderPrefix(),
                                         currentGroupStart + 1) :
                        wxString::Format(groupHeaderFormatString, GetGroupHeaderPrefix(),
                                         currentGroupStart + 1,
                                         std::min<size_t>(maxRowsWhenGrouping + currentGroupStart,
                                                          m_matrix.size()));

                auto columnHeader =
                    std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo(headerString)
                                                            .Scaling(GetScaling())
                                                            .Pen(wxNullPen)
                                                            .Font(groupHeaderLabelFont)
                                                            .AnchorPoint(drawArea.GetTopLeft()));
                columnHeader->SetFontColor(GetBottomXAxis().GetFontColor());
                columnHeader->Offset(0, -groupHeaderLabelHeight);
                columnHeader->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                AddObject(std::move(columnHeader));
                }
            // then the column's cells
            for (const auto& cell : row)
                {
                // if no label on cell, then that means this row is jagged and there
                // are no more cells in it, so go to next row
                if (cell.m_valueLabel.empty())
                    {
                    continue;
                    }
                // if NaN, then color will be bogus, so use plot's background color
                const wxColour cellColor = cell.m_color.IsOk() ? cell.m_color : wxTransparentColor;

                pts[0] = wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                                 drawArea.GetTopLeft().y + (currentRow * boxWidth));
                pts[1] = wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn),
                                 drawArea.GetTopLeft().y + (boxWidth) + (currentRow * boxWidth));
                pts[2] = wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                                 drawArea.GetTopLeft().y + (boxWidth) + (currentRow * boxWidth));
                pts[3] = wxPoint(drawArea.GetTopLeft().x + (boxWidth * currentColumn) + boxWidth,
                                 drawArea.GetTopLeft().y + (currentRow * boxWidth));
                // keep scaling at 1 since this is set to a specific size on the plot
                auto box = std::make_unique<GraphItems::Polygon>(
                    GraphItems::GraphItemInfo(cell.m_selectionLabel).Pen(GetPen()).Brush(cellColor),
                    pts, std::size(pts));
                const wxRect boxRect(pts[0], pts[2]);

                AddObject(std::move(box));
                // show the value of the cell, centered on it
                AddObject(std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(cell.m_valueLabel)
                        .Font(boxLabelFont)
                        .Pen(wxNullPen)
                        .Selectable(false)
                        .FontColor(
                            // contrast colors, unless this cell is NaN
                            (cell.m_color.IsOk() ?
                                 Colors::ColorContrast::BlackOrWhiteContrast(cellColor) :
                                 // if NaN, then set 'X' to red
                                 Colors::ColorContrast::ShadeOrTintIfClose(*wxRED, cellColor)))
                        .Anchoring(Anchoring::Center)
                        .AnchorPoint(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                                             boxRect.GetTop() + (boxRect.GetHeight() / 2)))));
                ++currentColumn;
                }
            if (hasGroupLabels)
                {
                // add a group label
                auto groupRowLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo(GetGroupColumn()->GetLabelFromID(currentGroupStart))
                        .Anchoring(Anchoring::TopLeftCorner)
                        .
                    // font is already scaled, so leave the label's scaling at 1.0
                    Font(groupLabelFont)
                        .AnchorPoint(wxPoint(drawArea.GetTopLeft().x - groupLabelWidth,
                                             drawArea.GetTopLeft().y + (currentRow * boxWidth)))
                        .Pen(wxNullPen)
                        .Padding(0, LABEL_RIGHT_PADDING, 0, 0)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered));
                groupRowLabel->SetMinimumUserSizeDIPs(DownscaleFromScreenAndCanvas(groupLabelWidth),
                                                      DownscaleFromScreenAndCanvas(boxWidth));
                AddObject(std::move(groupRowLabel));
                }

            ++currentGroupStart;
            ++currentRow;
            currentColumn = 0;
            if (IsUsingGrouping() && m_groupColumnCount > 1 &&
                currentRow + 1 > maxRowsWhenGrouping &&
                // don't add another column if this is the last row
                currentGroupStart != m_matrix.size())
                {
                currentRow = 0;
                drawArea.Offset(wxPoint(drawArea.GetWidth() + padding + groupLabelWidth, 0));
                }
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> HeatMap::CreateLegend(const LegendOptions& options)
        {
        if (GetDataset() == nullptr || m_continuousColumn->GetRowCount() == 0)
            {
            return nullptr;
            }

        std::vector<double> validData;
        std::copy_if(m_continuousColumn->GetValues().cbegin(),
                     m_continuousColumn->GetValues().cend(), std::back_inserter(validData),
                     [](auto x) { return std::isfinite(x); });
        const auto minValue = *std::ranges::min_element(std::as_const(validData));
        const auto maxValue = *std::ranges::max_element(std::as_const(validData));
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo(
                // add spaces on the empty lines to work around SVG exporting
                // stripping out the blank lines
                wxString::Format(
                    L"%s\n \n \n%s",
                    wxNumberFormatter::ToString(maxValue, 6, Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(minValue, 6, Settings::GetDefaultNumberFormat())))
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs() * 1.5)
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft));
        if (options.IsIncludingHeader())
            {
            legend->SetText(wxString::Format(L"%s\n", m_continuousColumn->GetName()) +
                            legend->GetText());
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->GetLegendIcons().push_back(Icons::LegendIcon(m_reversedColorSpectrum));

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
