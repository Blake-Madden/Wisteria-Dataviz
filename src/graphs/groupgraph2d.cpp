#include "groupgraph2d.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Graphs;
using namespace Wisteria::Icons;

//----------------------------------------------------------------
void GroupGraph2D::BuildGroupIdMap()
    {
    m_groupIds.clear();
    if (!IsUsingGrouping())
        { return; }
    // make reverse string table, sorted by label
    std::map<wxString, Data::GroupIdType, Data::StringCmpNoCase> groups;
    if (m_groupColumn->GetStringTable().size())
        {
        for (const auto& [id, str] : m_groupColumn->GetStringTable())
            { groups.insert(std::make_pair(str, id)); }
        }
    // if no string table, then it's just discrete values;
    // make a reverse "string table" from that
    else
        {
        for (size_t i = 0; i < m_groupColumn->GetRowCount(); ++i)
            {
            groups.insert(std::make_pair(m_groupColumn->GetValueAsLabel(i),
                                         m_groupColumn->GetValue(i)));
            }
        }
    // build a list of group IDs and their respective strings' alphabetical order
    size_t currentIndex{ 0 };
    for (auto& group : groups)
        {
        m_groupIds.insert(std::make_pair(group.second, currentIndex++));
        }
    }

//----------------------------------------------------------------
std::shared_ptr<GraphItems::Label> GroupGraph2D::CreateLegend(
    const LegendOptions& options)
    {
    if (!IsUsingGrouping() || GetGroupCount() == 0)
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
            wxString();
        wxASSERT_MSG(Settings::GetMaxLegendTextLength() >= 1, L"Max legend text length is zero?!");
        if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
            Settings::GetMaxLegendTextLength() >= 1)
            {
            currentLabel.erase(Settings::GetMaxLegendTextLength()-1);
            currentLabel.append(L"\u2026");
            }
        legendText.append(currentLabel.c_str()).append(L"\n");

        wxASSERT_MSG(GetBrushScheme() || GetColorScheme(),
            L"Legend needs either a brush scheme or color scheme!");
        // Graphs usually use the brush as the primary, but some may
        // only use the color scheme; fallback to that if necessary.
        const wxBrush br = (GetBrushScheme() ?
            GetBrushScheme()->GetBrush(groupId.first) :
            GetColorScheme() ?
            wxBrush(GetColorScheme()->GetColor(groupId.first)) :
            *wxTRANSPARENT_BRUSH);
        legend->GetLegendIcons().emplace_back(
                LegendIcon(IconShape::Square, *wxBLACK,
                br,
                GetColorScheme() ?
                    std::optional<wxColour>(GetColorScheme()->GetColor(groupId.first)) :
                    std::nullopt));

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
