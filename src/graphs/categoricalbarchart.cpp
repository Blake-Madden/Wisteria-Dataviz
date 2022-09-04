///////////////////////////////////////////////////////////////////////////////
// Name:        histogram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "categoricalbarchart.h"
#include "../util/frequencymap.h"
#include "../math/statistics.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void CategoricalBarChart::SetData(std::shared_ptr<const Data::Dataset> data,
                            const wxString& categoricalColumnName,
                            const std::optional<const wxString> valueColumnName /*= std::nullopt*/,
                            const std::optional<const wxString> groupColumnName /*= std::nullopt*/,
                            const BinLabelDisplay blDisplay /*= BinLabelDisplay::BinValue*/)
        {
        if (data == nullptr)
            { return; }

        m_data = data;
        m_useGrouping = groupColumnName.has_value();
        m_useValueColumn = valueColumnName.has_value();
        m_groupIds.clear();
        GetSelectedIds().clear();
        SetBinLabelDisplay(blDisplay);

        m_categoricalColumn = m_data->GetCategoricalColumn(categoricalColumnName);
        if (m_categoricalColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': categorical column not found for categorical bar chart."),
                categoricalColumnName).ToUTF8());
            }
        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for categorical bar chart."),
                groupColumnName.value()).ToUTF8());
            }
        m_continuousColumn = (valueColumnName ? m_data->GetContinuousColumn(valueColumnName.value()) :
            m_data->GetContinuousColumns().cend());
        if (valueColumnName && m_continuousColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for categorical bar chart."),
                valueColumnName.value()).ToUTF8());
            }

        // see if we should use grouping from the data
        if (m_useGrouping)
            {
            // make reverse string table, sorted by label
            std::map<wxString, Data::GroupIdType, Data::StringCmpNoCase> groups;
            for (const auto& [id, str] : m_groupColumn->GetStringTable())
                { groups.insert(std::make_pair(str, id)); }
            // build a list of group IDs and their respective strings' alphabetical order
            size_t currentIndex{ 0 };
            for (auto& group : groups)
                {
                m_groupIds.insert(std::make_pair(group.second, currentIndex++));
                }
            }

        // reset everything first
        ClearBars();

        // if no data then just draw a blank 10x10 grid
        if (m_data->GetRowCount() == 0)
            {
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        Calculate();

        GetBarAxis().ShowOuterLabels(false);

        // set axis labels
        GetBarAxis().GetTitle().SetText(m_categoricalColumn->GetName());
        GetScalingAxis().GetTitle().SetText(_(L"Frequency"));
        }

    //----------------------------------------------------------------
    void CategoricalBarChart::Calculate()
        {
        if (m_data == nullptr)
            { return; }

        // calculate how many observations are in each group
        aggregate_frequency_set<CatBarBlock> groups;

        double grandTotal{ 0 };
        for (size_t i = 0; i < m_data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (m_useValueColumn &&
                std::isnan(m_continuousColumn->GetValue(i)))
                { continue; }
            const double groupTotal = (m_useValueColumn ? m_continuousColumn->GetValue(i) : 1);
            grandTotal += groupTotal;
            // convert group ID into color scheme index
            // (index is ordered by labels alphabetically)
            size_t colorIndex{ 0 };
            if (m_useGrouping)
                {
                const auto pos = m_groupIds.find(m_groupColumn->GetValue(i));
                wxASSERT_MSG((pos != m_groupIds.end()),
                             L"Error finding color scheme index for group!");
                if (pos != m_groupIds.end())
                    { colorIndex = pos->second; }
                }
            groups.insert(
                // the current category ID (and group's color index and label, if applicable)
                CatBarBlock{
                    m_categoricalColumn->GetValue(i),
                    (m_useGrouping ? colorIndex : 0),
                    (m_useGrouping ?
                        m_groupColumn->GetLabelFromID(m_groupColumn->GetValue(i)) :
                        wxString(L"")) },
                groupTotal);
            }

        // add the bars (block-by-block)
        for (const auto& blockTable : groups.get_data())
            {
            const wxColour blockColor = GetColorScheme() ?
                (m_useGrouping ?
                 GetColorScheme()->GetColor(blockTable.first.m_schemeIndex) : GetColorScheme()->GetColor(0)) :
                wxTransparentColour;
            const wxBrush blockBrush = (m_useGrouping ?
                GetBrushScheme()->GetBrush(blockTable.first.m_schemeIndex) : GetBrushScheme()->GetBrush(0));

            wxString blockLabelText = (m_useValueColumn ?
                wxString::Format(_(L"%s item(s), totalling %s"),
                    wxNumberFormatter::ToString(blockTable.second.first, 0,
                        Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(blockTable.second.second, 2,
                        Settings::GetDefaultNumberFormat())) :
                wxString::Format(_(L"%s item(s)"),
                    wxNumberFormatter::ToString(blockTable.second.first, 0,
                        Settings::GetDefaultNumberFormat())) );

            if (m_useGrouping)
                {
                blockLabelText.Prepend(blockTable.first.m_groupName + L": ");
                }
            GraphItems::Label blockLabel(blockLabelText);

            auto foundBar = std::find_if(GetBars().begin(), GetBars().end(),
                [&blockTable](const auto& bar)
                    { return compare_doubles(bar.GetAxisPosition(), blockTable.first.m_bin); });
            if (foundBar == GetBars().end())
                {
                Bar theBar(blockTable.first.m_bin,
                    {
                    BarBlock(BarBlockInfo(blockTable.second.second).
                        Brush(blockBrush).Color(blockColor).SelectionLabel(blockLabel))
                    },
                    wxEmptyString,
                    GraphItems::Label(
                        m_categoricalColumn->GetLabelFromID(blockTable.first.m_bin)),
                    GetBarEffect(), GetBarOpacity());
                AddBar(theBar);
                }
            else
                {
                BarBlock block{ BarBlock(BarBlockInfo(blockTable.second.second).
                    Brush(blockBrush).Color(blockColor).SelectionLabel(blockLabel)) };
                foundBar->AddBlock(block);
                UpdateScalingAxisFromBar(*foundBar);
                }
            }

        // add the bar labels now that they are built
        for (auto& bar : GetBars())
            { UpdateBarLabel(bar); }

        // if grouping, then sort by the labels alphabetically
        if (m_useGrouping)
            {
            SortBars(BarChart::BarSortComparison::SortByAxisLabel,
                     SortDirection::SortAscending);
            }
        // if no grouping within the bars, then sort by bar size
        else
            {
            SortBars(BarChart::BarSortComparison::SortByBarLength,
                     // largest bars to the top or to the left
                     (GetBarOrientation() == Orientation::Horizontal ?
                        SortDirection::SortDescending :
                        SortDirection::SortAscending));
            }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> CategoricalBarChart::CreateLegend(
        const LegendOptions& options)
        {
        if (m_data == nullptr || GetGroupCount() == 0)
            { return nullptr; }

        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        wxString legendText;
        size_t lineCount{ 0 };
        // color index and then group ID
        // (do this so that the items are in alphabetically order)
        std::map<size_t, Data::GroupIdType> reverseGroupIds;
        for (const auto& groupId : m_groupIds)
            { reverseGroupIds.insert(std::make_pair(groupId.second, groupId.first)); }
        for (const auto& groupId : reverseGroupIds)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = m_useGrouping ?
                m_groupColumn->GetLabelFromID(groupId.second) :
                wxString(L"");
            wxASSERT_MSG(Settings::GetMaxLegendTextLength() >= 1, L"Max legend text length is zero?!");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
                Settings::GetMaxLegendTextLength() >= 1)
                {
                currentLabel.erase(Settings::GetMaxLegendTextLength()-1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::Square, *wxBLACK,
                        GetBrushScheme()->GetBrush(groupId.first).GetColour()));
            ++lineCount;
            }

        if (options.IsIncludingHeader())
            {
            legendText.Prepend(
                wxString::Format(L"%s\n", m_groupColumn->GetName()));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, options.GetPlacementHint());
        return legend;
        }
    }
