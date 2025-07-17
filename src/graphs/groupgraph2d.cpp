///////////////////////////////////////////////////////////////////////////////
// Name:        groupgraph2d.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "groupgraph2d.h"
#include <ranges>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::GroupGraph2D, Wisteria::Graphs::Graph2D)

    using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Graphs;
using namespace Wisteria::Icons;

//----------------------------------------------------------------
void GroupGraph2D::SetGroupColumn(
    const std::optional<const wxString>& groupColumnName /*= std::nullopt*/)
    {
    assert(GetDataset() && L"You must call SetDataset() before calling SetGroupColumn()!");
    if (GetDataset() == nullptr)
        {
        throw std::runtime_error(
            wxString(_(L"Dataset not set before calling SetGroupColumn().")).ToUTF8());
        }
    const auto groupColIter = groupColumnName ?
                                  GetDataset()->GetCategoricalColumn(groupColumnName.value()) :
                                  GetDataset()->GetCategoricalColumns().cend();
    if (groupColumnName && groupColIter == GetDataset()->GetCategoricalColumns().cend())
        {
        throw std::runtime_error(
            wxString::Format(_(L"'%s': group column not found for graph."), groupColumnName.value())
                .ToUTF8());
        }
    SetGroupColumn(groupColumnName ? &(*groupColIter) : nullptr);
    }

//----------------------------------------------------------------
void GroupGraph2D::BuildGroupIdMap()
    {
    m_groupIds.clear();
    if (!IsUsingGrouping())
        {
        return;
        }
    // make reverse string table, sorted by label
    std::map<wxString, Data::GroupIdType, Data::wxStringLessNoCase> groups;
    if (!GetGroupColumn()->GetStringTable().empty())
        {
        for (const auto& [id, str] : GetGroupColumn()->GetStringTable())
            {
            groups.insert(std::make_pair(str, id));
            }
        }
    // if no string table, then it's just discrete values;
    // make a reverse "string table" from that
    else
        {
        for (size_t i = 0; i < GetGroupColumn()->GetRowCount(); ++i)
            {
            groups.insert(std::make_pair(GetGroupColumn()->GetValueAsLabel(i),
                                         GetGroupColumn()->GetValue(i)));
            }
        }
    // build a list of group IDs and their respective strings' alphabetical order
    size_t currentIndex{ 0 };
    for (const auto& group : groups)
        {
        m_groupIds.insert(std::make_pair(group.second, currentIndex++));
        }
    }

//----------------------------------------------------------------
std::unique_ptr<GraphItems::Label> GroupGraph2D::CreateLegend(const LegendOptions& options)
    {
    if (!IsUsingGrouping() || GetGroupCount() == 0)
        {
        return nullptr;
        }

    auto legend =
        std::make_unique<GraphItems::Label>(GraphItemInfo()
                                                .Padding(0, 0, 0, Label::GetMinLegendWidthDIPs())
                                                .DPIScaling(GetDPIScaleFactor())
                                                .FontColor(GetLeftYAxis().GetFontColor()));

    constexpr std::wstring_view ELLIPSIS{ L"\u2026" };

    wxString legendText;
    size_t lineCount{ 0 };

    const auto mdCode = GetGroupColumn()->FindMissingDataCode();
    size_t mdSchemeIndex{ 0 };
    // scheme index and then group ID
    // (do this so that the items are in alphabetically order)
    std::map<size_t, Data::GroupIdType> reverseGroupIds;
    for (const auto& groupId : GetGroupIds())
        {
        reverseGroupIds.insert(std::make_pair(groupId.second, groupId.first));
        }
    for (const auto& [schemeIndex, groupId] : reverseGroupIds)
        {
        // we'll put the missing data group at the bottom of the labels
        if (mdCode.has_value() && groupId == mdCode.value())
            {
            mdSchemeIndex = schemeIndex;
            continue;
            }
        if (Settings::GetMaxLegendItemCount() == lineCount)
            {
            legendText.append(ELLIPSIS.data());
            break;
            }
        wxString currentLabel = GetGroupColumn()->GetLabelFromID(groupId);
        assert(Settings::GetMaxLegendTextLength() >= 1 && L"Max legend text length is zero?!");
        if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
            Settings::GetMaxLegendTextLength() >= 1)
            {
            currentLabel.erase(Settings::GetMaxLegendTextLength() - 1);
            currentLabel.append(ELLIPSIS.data());
            }
        legendText.append(currentLabel.c_str()).append(L"\n");

        assert((GetBrushScheme() || GetColorScheme()) &&
               L"Legend needs either a brush scheme or color scheme!");
        // Graphs usually use the brush as the primary, but some may
        // only use the color scheme; fallback to that if necessary.
        const wxBrush br = (GetBrushScheme() ? GetBrushScheme()->GetBrush(schemeIndex) :
                            GetColorScheme() ? wxBrush(GetColorScheme()->GetColor(schemeIndex)) :
                                               *wxTRANSPARENT_BRUSH);
        legend->GetLegendIcons().emplace_back(
            (GetShapeScheme() ? GetShapeScheme()->GetShape(schemeIndex) : m_defaultLegendShape),
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), br,
            GetColorScheme() ? std::optional<wxColour>(GetColorScheme()->GetColor(schemeIndex)) :
                               std::nullopt);

        ++lineCount;
        }

    // add MD label at the bottom if there are missing data
    if (GetGroupColumn()->ContainsMissingData())
        {
        assert(mdCode.has_value() && L"Cat. column has MD, but string table has no MD code?!");
        if (mdCode.has_value())
            {
            legendText.append(_(L"[NO GROUP]")).append(L"\n");

            // Graphs usually use the brush as the primary, but some may
            // only use the color scheme; fallback to that if necessary.
            const wxBrush brush =
                (GetBrushScheme() ? GetBrushScheme()->GetBrush(mdSchemeIndex) :
                 GetColorScheme() ? wxBrush(GetColorScheme()->GetColor(mdSchemeIndex)) :
                                    *wxTRANSPARENT_BRUSH);
            legend->GetLegendIcons().emplace_back(
                (GetShapeScheme() ? GetShapeScheme()->GetShape(mdSchemeIndex) :
                                    m_defaultLegendShape),
                Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                brush,
                GetColorScheme() ?
                    std::optional<wxColour>(GetColorScheme()->GetColor(mdSchemeIndex)) :
                    std::nullopt);
            }
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
