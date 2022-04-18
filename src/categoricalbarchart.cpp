///////////////////////////////////////////////////////////////////////////////
// Name:        histogram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "categoricalbarchart.h"
#include "util/frequency_set.h"
#include "math/statistics.h"

using namespace Wisteria::GraphItems;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void CategoricalBarChart::SetData(std::shared_ptr<const Data::Dataset> data,
                            const wxString& categoricalColumnName,
                            const std::optional<const wxString> groupColumnName /*= std::nullopt*/,
                            const BinLabelDisplay blDisplay /*= BinLabelDisplay::BinValue*/)
        {
        m_data = data;
        m_useGrouping = groupColumnName.has_value();
        m_groupIds.clear();
        m_binLabelDisplay = blDisplay;

        // garbage, so reset and bail
        if (m_data == nullptr)
            {
            ClearBars();
            return;
            }

        m_categoricalColumn = m_data->GetCategoricalColumn(categoricalColumnName);
        if (m_categoricalColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': categorical column not found for categorical bar chart."), categoricalColumnName));
            }
        m_groupColumn = (groupColumnName ? m_data->GetCategoricalColumn(groupColumnName.value()) :
            m_data->GetCategoricalColumns().cend());
        if (groupColumnName && m_groupColumn == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for categorical bar chart."), groupColumnName.value()));
            }

        // see if we should use grouping from the data
        if (m_useGrouping)
            {
            std::for_each(m_groupColumn->GetValues().cbegin(), m_groupColumn->GetValues().cend(),
                [this](const auto id) { m_groupIds.insert(id); });
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
        GetBarAxis().GetTitle().SetText(m_categoricalColumn->GetTitle());
        GetScalingAxis().GetTitle().SetText(_(L"Frequency"));
        }

    //----------------------------------------------------------------
    void CategoricalBarChart::Calculate()
        {
        if (m_data == nullptr)
            { return; }

        // calculate how many observations are in each group
        multi_value_frequency_map<CatBarBlock, wxString, std::less<CatBarBlock>, Data::StringCmpNoCase> groups;
        groups.set_values_list_max_size(Settings::GetMaxObservationInBin());

        for (size_t i = 0; i < m_data->GetRowCount(); ++i)
            {
            groups.insert(CatBarBlock{
                m_categoricalColumn->GetValue(i),
                (m_useGrouping ?
                    m_groupColumn->GetValue(i) : static_cast<Data::GroupIdType>(0)) },
                m_data->GetIdColumn().GetValue(i).wc_str() );
            }

        // add the bars (block-by-block)
        for (const auto& blockTable : groups.get_data())
            {
            const wxColour blockColor = (m_useGrouping ?
                GetColorScheme()->GetColor(blockTable.first.m_block) : GetColorScheme()->GetColor(0));

            wxString blockLabelText{ wxString::Format(_("%s item(s)\n"),
                wxNumberFormatter::ToString(blockTable.second.second, 0,
                    Settings::GetDefaultNumberFormat())) };
            // piece the first few observations together as a display label for the block
            for (const auto& obsLabel : blockTable.second.first)
                { blockLabelText.append(obsLabel).append(L"\n"); }
            blockLabelText.Trim();
            // if observations are added to the selection label, but not all of them, then add ellipsis
            if (blockTable.second.first.size() < blockTable.second.second &&
                blockTable.second.first.size() > 1)
                { blockLabelText += L"..."; }

            if (m_useGrouping)
                {
                blockLabelText.Prepend(
                    m_groupColumn->GetCategoryLabel(blockTable.first.m_block) + L": ");
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
                        Brush(blockColor).SelectionLabel(blockLabel))
                    },
                    wxEmptyString,
                    GraphItems::Label(m_categoricalColumn->GetCategoryLabel(blockTable.first.m_bin)),
                    GetBarEffect(), GetBarOpacity());
                // if observations added to the selection label, then show it as a report
                if (blockTable.second.first.size() > 1)
                    {
                    theBar.GetBlocks()[0].GetSelectionLabel().
                        GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::Centered);
                    theBar.GetBlocks()[0].GetSelectionLabel().
                        SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    }
                AddBar(theBar);
                }
            else
                {
                BarBlock block{ BarBlock(BarBlockInfo(blockTable.second.second).
                    Brush(blockColor).SelectionLabel(blockLabel)) };
                if (blockTable.second.first.size() > 1)
                    {
                    block.GetSelectionLabel().SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    block.GetSelectionLabel().GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::Centered);
                    }
                foundBar->AddBlock(block);
                UpdateScalingAxisFromBar(*foundBar);
                }
            }
        // add the bar labels now that they are built
        for (auto& bar : GetBars())
            {
            const double percentage = safe_divide<double>(bar.GetLength(),m_categoricalColumn->GetRowCount())*100;
            const wxString labelStr = (bar.GetLength() == 0 || GetBinLabelDisplay() == BinLabelDisplay::NoDisplay) ?
                wxString(wxEmptyString) :
                (GetBinLabelDisplay() == BinLabelDisplay::BinValue) ?
                wxNumberFormatter::ToString(bar.GetLength(), 0, Settings::GetDefaultNumberFormat()) :
                (GetBinLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                wxNumberFormatter::ToString(percentage, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                wxNumberFormatter::ToString(bar.GetLength(), 0, Settings::GetDefaultNumberFormat()) +
                    L" (" + wxNumberFormatter::ToString(percentage, 0, wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%)";
            bar.GetLabel().SetText(labelStr);
            }

        // if grouping, then sort by the labels alphabetically
        if (m_useGrouping)
            { SortBars(BarChart::BarSortComparison::SortByAxisLabel, SortDirection::SortAscending); }
        // if no grouping within the bars, then sort the largest counts to the top
        else
            { SortBars(BarChart::BarSortComparison::SortByBarLength, SortDirection::SortDescending); }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> CategoricalBarChart::CreateLegend(
        const LegendCanvasPlacementHint hint, const bool includeHeader) const
        {
        if (m_data == nullptr || GetGroupCount() == 0)
            { return nullptr; }

        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidth()).
            Window(GetWindow()));
        legend->SetBoxCorners(BoxCorners::Rounded);

        wxString legendText;
        size_t lineCount{ 0 };
        for (const auto& groupId : m_groupIds)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = m_useGrouping ?
                m_groupColumn->GetCategoryLabel(groupId) :
                wxString(L"");
            if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                {
                currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::SquareIcon, *wxBLACK,
                        GetColorScheme()->GetColor(groupId)));
            }
        if (includeHeader)
            {
            legendText.Prepend(
                wxString::Format(L"%s\n", m_groupColumn->GetTitle()));
            legend->GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::FlushLeft);
            }
        legend->SetText(legendText.Trim());

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
