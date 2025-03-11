///////////////////////////////////////////////////////////////////////////////
// Name:        categoricalbarchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "categoricalbarchart.h"
#include "../math/statistics.h"
#include "../util/frequencymap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::CategoricalBarChart, Wisteria::Graphs::BarChart)

using namespace Wisteria::GraphItems;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void CategoricalBarChart::SetData(
        std::shared_ptr<const Data::Dataset> data, const wxString& categoricalColumnName,
        const std::optional<const wxString> weightColumnName /*= std::nullopt*/,
        const std::optional<const wxString> groupColumnName /*= std::nullopt*/,
        const BinLabelDisplay blDisplay /*= BinLabelDisplay::BinValue*/)
        {
        // point to (new) data and reset
        SetDataset(data);
        ResetGrouping();
        m_useWeightColumn = weightColumnName.has_value();
        m_useIDColumnForBars = false;
        GetSelectedIds().clear();
        ClearBars();
        ClearBarGroups();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetBinLabelDisplay(blDisplay);

        m_idColumn = &GetDataset()->GetIdColumn();

        m_categoricalColumn = GetDataset()->GetCategoricalColumn(categoricalColumnName);
        if (m_categoricalColumn == GetDataset()->GetCategoricalColumns().cend())
            {
            // see if they are using the ID column for the bars
            if (m_idColumn->GetName().CmpNoCase(categoricalColumnName) == 0)
                {
                m_useIDColumnForBars = true;
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': categorical/ID column not found for categorical bar chart."),
                        categoricalColumnName)
                        .ToUTF8());
                }
            }

        // set the grouping column (or keep it as null if not in use)
        SetGroupColumn(groupColumnName);

        m_weightColumn =
            (weightColumnName ? GetDataset()->GetContinuousColumn(weightColumnName.value()) :
                                GetDataset()->GetContinuousColumns().cend());
        if (weightColumnName && m_weightColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': weight column not found for categorical bar chart."),
                                 weightColumnName.value())
                    .ToUTF8());
            }

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            }

        // if no data then just draw a blank 10x10 grid
        if (GetDataset()->GetRowCount() == 0)
            {
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        Calculate();

        // re-build the bar labels now that the bars are ready
        SetBinLabelDisplay(blDisplay);

        GetBarAxis().ShowOuterLabels(false);

        // set axis labels
        GetBarAxis().GetTitle().SetText(m_useIDColumnForBars ? m_idColumn->GetName() :
                                                               m_categoricalColumn->GetName());
        GetScalingAxis().GetTitle().SetText(_(L"Frequency"));
        }

    //----------------------------------------------------------------
    void CategoricalBarChart::Calculate()
        {
        if (GetDataset() == nullptr)
            {
            return;
            }

        // calculate how many observations are in each group
        aggregate_frequency_set<CatBarBlock> groups;
        std::map<wxString, size_t, Data::wxStringLessNoCase> m_IDsMap;
        if (m_useIDColumnForBars)
            {
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                m_IDsMap.insert(std::make_pair(m_idColumn->GetValue(i), m_IDsMap.size()));
                }
            }

        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (m_useWeightColumn && std::isnan(m_weightColumn->GetValue(i)))
                {
                continue;
                }
            const double groupTotal = (m_useWeightColumn ? m_weightColumn->GetValue(i) : 1);
            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex =
                IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) : 0;

            if (m_useIDColumnForBars)
                {
                const auto ID = m_IDsMap.find(m_idColumn->GetValue(i));
                assert((ID != m_IDsMap.cend()) && L"Error finding bar index for ID value!");
                if (ID != m_IDsMap.cend())
                    {
                    groups.insert(
                        // the current category ID (and group's color index and label,
                        // if applicable)
                        CatBarBlock{ ID->second, m_idColumn->GetValue(i), colorIndex,
                                     (IsUsingGrouping() ? GetGroupColumn()->GetLabelFromID(
                                                              GetGroupColumn()->GetValue(i)) :
                                                          wxString()) },
                        groupTotal);
                    }
                }
            else
                {
                groups.insert(
                    // the current category ID (and group's color index and label, if applicable)
                    CatBarBlock{
                        m_categoricalColumn->GetValue(i),
                        m_categoricalColumn->GetLabelFromID(m_categoricalColumn->GetValue(i)),
                        colorIndex,
                        (IsUsingGrouping() ?
                             GetGroupColumn()->GetLabelFromID(GetGroupColumn()->GetValue(i)) :
                             wxString()) },
                    groupTotal);
                }
            }

        // add the bars (block-by-block)
        for (const auto& blockTable : groups.get_data())
            {
            const wxColour blockColor =
                GetColorScheme() ? (IsUsingGrouping() ?
                                        GetColorScheme()->GetColor(blockTable.first.m_schemeIndex) :
                                        GetColorScheme()->GetColor(0)) :
                                   wxTransparentColour;
            const wxBrush blockBrush =
                (IsUsingGrouping() ? GetBrushScheme()->GetBrush(blockTable.first.m_schemeIndex) :
                                     GetBrushScheme()->GetBrush(0));

            wxString blockLabelText =
                (m_useWeightColumn ?
                     wxString::Format(
                         // TRANSLATORS: the number of items in a bin
                         wxPLURAL(L"%s item, totaling %s", L"%s items, totaling %s",
                                  blockTable.second.first),
                         wxNumberFormatter::ToString(blockTable.second.first, 0,
                                                     Settings::GetDefaultNumberFormat()),
                         wxNumberFormatter::ToString(blockTable.second.second, 2,
                                                     Settings::GetDefaultNumberFormat())) :
                     wxString::Format(
                         // TRANSLATORS: the number of items in a bin
                         wxPLURAL(L"%s item", L"%s items", blockTable.second.first),
                         wxNumberFormatter::ToString(blockTable.second.first, 0,
                                                     Settings::GetDefaultNumberFormat())));

            if (IsUsingGrouping())
                {
                blockLabelText.Prepend(blockTable.first.m_groupName + L": ");
                }
            GraphItems::Label blockLabel(blockLabelText);

            auto foundBar = std::find_if(
                GetBars().begin(), GetBars().end(),
                [&blockTable](const auto& bar) noexcept
                { return compare_doubles(bar.GetAxisPosition(), blockTable.first.m_bin); });
            if (foundBar == GetBars().end())
                {
                Bar theBar(blockTable.first.m_bin,
                           { BarBlock(BarBlockInfo(blockTable.second.second)
                                          .Brush(blockBrush)
                                          .Color(blockColor)
                                          .Tag(blockTable.first.m_groupName)
                                          .SelectionLabel(blockLabel)) },
                           wxEmptyString, GraphItems::Label(blockTable.first.m_binName),
                           GetBarEffect(), GetBarOpacity());
                AddBar(theBar);
                }
            else
                {
                BarBlock block{ BarBlock(BarBlockInfo(blockTable.second.second)
                                             .Brush(blockBrush)
                                             .Color(blockColor)
                                             .Tag(blockTable.first.m_groupName)
                                             .SelectionLabel(blockLabel)) };
                foundBar->AddBlock(block);
                UpdateScalingAxisFromBar(*foundBar);
                }
            }

        // add the bar labels now that they are built
        for (auto& bar : GetBars())
            {
            UpdateBarLabel(bar);
            }

        // if grouping, then sort by the labels alphabetically
        if (IsUsingGrouping())
            {
            SortBars(BarChart::BarSortComparison::SortByAxisLabel, SortDirection::SortAscending);
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
    } // namespace Wisteria::Graphs
