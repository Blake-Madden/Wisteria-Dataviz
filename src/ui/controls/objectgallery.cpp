///////////////////////////////////////////////////////////////////////////////
// Name:        objectgallery.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "objectgallery.h"
#include "../../app/wisteriaapp.h"
#include "../../import/html_encode.h"
#include "../../util/donttranslate.h"
#include <wx/base64.h>
#include <wx/mstream.h>
#include <wx/statbmp.h>

namespace Wisteria::UI
    {
    /// @cond DOXYGEN_IGNORE
    wxDEFINE_EVENT(wxEVT_OBJECTGALLERY_ITEM_DROPPED, ObjectGalleryItemDroppedEvent);

    /// @endcond

    //-------------------------------------------
    static wxString EscapeForHtml(const wxString& text)
        {
        const lily_of_the_valley::html_encode_text encode;
        return encode({ text.wc_str(), text.length() }, false);
        }

    //-------------------------------------------
    static wxString BitmapToPngDataUri(const wxBitmapBundle& bmpBundle, const wxSize& size)
        {
        if (!bmpBundle.IsOk())
            {
            return {};
            }
        const wxImage img = bmpBundle.GetBitmap(size).ConvertToImage();
        wxMemoryOutputStream stream;
        if (!img.IsOk() || !img.SaveFile(stream, wxBITMAP_TYPE_PNG))
            {
            return {};
            }
        const auto* buffer = stream.GetOutputStreamBuffer();
        const auto length = stream.GetLength();
        if (buffer == nullptr || buffer->GetBufferStart() == nullptr || length <= 0)
            {
            return {};
            }
        return _DT(L"data:image/png;base64,") +
               wxBase64Encode(buffer->GetBufferStart(), static_cast<size_t>(length));
        }

    //-------------------------------------------
    ObjectGalleryCtrl::ObjectGalleryCtrl(wxWindow* parent, wxWindow* dropTarget,
                                         const wxWindowID id, const wxPoint& pos,
                                         const wxSize& size)
        : wxPanel(parent, id, pos, size), m_dropTarget(dropTarget),
          m_collapsedGroups(wxGetApp().GetAppSettings()->GetCollapsedGalleryGroups())
        {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        m_webView = wxWebView::New(this, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize);
        if (m_webView == nullptr)
            {
            wxLogError(L"Failed to create wxWebView. No backend available.");
            }
        else
            {
            m_webView->EnableContextMenu(false);
            m_webView->AddScriptMessageHandler(wxString{ ScriptMessageHandlerName });
            m_webView->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                            &ObjectGalleryCtrl::OnScriptMessage, this);
            sizer->Add(m_webView, wxSizerFlags{ 1 }.Expand());
            RefreshPage();
            }

        SetSizer(sizer);

        m_dragTimer.SetOwner(this);
        Bind(wxEVT_TIMER, &ObjectGalleryCtrl::OnDragTimer, this, m_dragTimer.GetId());
        Bind(wxEVT_SYS_COLOUR_CHANGED, &ObjectGalleryCtrl::OnSysColourChanged, this);
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::RefreshPage()
        {
        if (m_webView != nullptr)
            {
            m_webView->SetPage(BuildHtml(), wxString{});
            }
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::OnSysColourChanged([[maybe_unused]] wxSysColourChangedEvent& event)
        {
        RefreshPage();
        }

    //-------------------------------------------
    wxString ObjectGalleryCtrl::GetGroupDisplayName(const GalleryGroup group)
        {
        switch (group)
            {
        case GalleryGroup::Objects:
            return _(L"Objects");
        case GalleryGroup::Basic:
            return _(L"Basic");
        case GalleryGroup::Business:
            return _(L"Business");
        case GalleryGroup::Statistical:
            return _(L"Statistical");
        case GalleryGroup::Survey:
            return _(L"Survey");
        case GalleryGroup::Education:
            return _(L"Education");
        case GalleryGroup::Social:
            return _(L"Social Sciences");
        case GalleryGroup::Sports:
            return _(L"Sports");
            }
        return {};
        }

    //-------------------------------------------
    wxString ObjectGalleryCtrl::GetGroupIconName(const GalleryGroup group)
        {
        switch (group)
            {
        case GalleryGroup::Objects:
            return {};
        case GalleryGroup::Basic:
            return L"chart-basic.svg";
        case GalleryGroup::Business:
            return L"chart-business.svg";
        case GalleryGroup::Statistical:
            return L"chart-statistical.svg";
        case GalleryGroup::Survey:
            return L"chart-survey.svg";
        case GalleryGroup::Education:
            return L"chart-education.svg";
        case GalleryGroup::Social:
            return L"chart-social.svg";
        case GalleryGroup::Sports:
            return L"chart-sports.svg";
            }
        return {};
        }

    //-------------------------------------------
    wxString ObjectGalleryCtrl::GalleryItemTypeToId(const GalleryItemType type)
        {
        switch (type)
            {
        case GalleryItemType::Label:
            return L"label";
        case GalleryItemType::KpiCard:
            return L"kpi-card";
        case GalleryItemType::Image:
            return L"image";
        case GalleryItemType::Shape:
            return L"shape";
        case GalleryItemType::Axis:
            return L"axis";
        case GalleryItemType::Spacer:
            return L"spacer";
        case GalleryItemType::DividerHorizontalSingle:
            return L"divider-h-single";
        case GalleryItemType::DividerHorizontalDouble:
            return L"divider-h-double";
        case GalleryItemType::DividerVerticalSingle:
            return L"divider-v-single";
        case GalleryItemType::DividerVerticalDouble:
            return L"divider-v-double";
        case GalleryItemType::BarChart:
            return L"barchart";
        case GalleryItemType::PieChart:
            return L"piechart";
        case GalleryItemType::LinePlot:
            return L"lineplot";
        case GalleryItemType::MultiSeriesLinePlot:
            return L"multiseries-lineplot";
        case GalleryItemType::Table:
            return L"table";
        case GalleryItemType::SankeyDiagram:
            return L"sankey";
        case GalleryItemType::WaffleChart:
            return L"waffle";
        case GalleryItemType::RaceTrackChart:
            return L"racetrack";
        case GalleryItemType::NightingaleRoseChart:
            return L"nightingale-rose";
        case GalleryItemType::BulletChart:
            return L"bullet-chart";
        case GalleryItemType::ChoroplethMap:
            return L"choropleth";
        case GalleryItemType::GanttChart:
            return L"gantt";
        case GalleryItemType::CandlestickPlot:
            return L"candlestick";
        case GalleryItemType::Histogram:
            return L"histogram";
        case GalleryItemType::BoxPlot:
            return L"boxplot";
        case GalleryItemType::StemAndLeafPlot:
            return L"stem-leaf";
        case GalleryItemType::HeatMap:
            return L"heatmap";
        case GalleryItemType::ScatterPlot:
            return L"scatterplot";
        case GalleryItemType::BubblePlot:
            return L"bubbleplot";
        case GalleryItemType::ChernoffFacesPlot:
            return L"chernoff-faces";
        case GalleryItemType::WilmarthBridgePlot:
            return L"wilmarth-bridge";
        case GalleryItemType::LikertChart:
            return L"likert";
        case GalleryItemType::WordCloud:
            return L"wordcloud";
        case GalleryItemType::ProConRoadmap:
            return L"procon-roadmap";
        case GalleryItemType::ScaleChart:
            return L"scale-chart";
        case GalleryItemType::WCurvePlot:
            return L"wcurve";
        case GalleryItemType::LRRoadmap:
            return L"lr-roadmap";
        case GalleryItemType::WinLossSparkline:
            return L"winloss-sparkline";
        case GalleryItemType::WaterfallChart:
            return L"waterfall-chart";
        case GalleryItemType::FunnelChart:
            return L"funnel-chart";
            }
        return {};
        }

    //-------------------------------------------
    std::optional<GalleryItemType> ObjectGalleryCtrl::IdToGalleryItemType(const wxString& id)
        {
        static const auto idMap = []()
        {
            std::map<wxString, GalleryItemType> catalogIds;
            for (const auto& item : WisteriaApp::GetGalleryItemCatalog())
                {
                catalogIds[GalleryItemTypeToId(item.m_id)] = item.m_id;
                }
            return catalogIds;
        }();

        const auto pos = idMap.find(id);
        return (pos == idMap.cend()) ? std::nullopt : std::optional<GalleryItemType>{ pos->second };
        }

    //-------------------------------------------
    wxString ObjectGalleryCtrl::BuildHtml() const
        {
        const wxString bgColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString fgColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString tileColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString borderColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW).GetAsString(wxC2S_HTML_SYNTAX);

        wxString html = _DT(L"<!DOCTYPE html><html><head><meta charset='utf-8'>"
                            L"<meta name='color-scheme' content='light dark'><style>");
        html += wxString::Format(L":root { --bg:%s; --fg:%s; --tile-bg:%s; --border:%s; }", bgColor,
                                 fgColor, tileColor, borderColor);
        html += _DT(LR"raw(
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
    font-family: -apple-system, "Segoe UI", sans-serif; font-size: 13px;
    -webkit-user-select: none; user-select: none; }
.gallery { padding: 12px; }
section.group { margin-bottom: 16px; }
section.group header { display: flex; align-items: center; gap: 6px;
    margin-bottom: 8px; font-weight: 600; opacity: 0.85; cursor: pointer; }
section.group header img { width: 16px; height: 16px; }
section.group header .chevron { margin-left: auto; opacity: 0.6; font-size: 10px;
    transition: transform 0.15s ease; }
section.group.collapsed header { margin-bottom: 0; }
section.group.collapsed header .chevron { transform: rotate(-90deg); }
section.group.collapsed .tiles { display: none; }
.tiles { display: grid; grid-template-columns: repeat(auto-fill, minmax(76px, 1fr)); gap: 8px; }
.tile { display: flex; flex-direction: column; align-items: center; gap: 4px;
    padding: 8px 4px; border-radius: 8px; background: var(--tile-bg);
    border: 1px solid var(--border); cursor: grab; text-align: center;
    transition: transform 0.08s ease, box-shadow 0.08s ease; }
.tile:hover { transform: translateY(-2px); box-shadow: 0 3px 8px rgba(0, 0, 0, 0.18); }
.tile.picked-up { opacity: 0.35; }
.tile.disabled { opacity: 0.35; cursor: not-allowed; }
.tile .tile-icon { width: 32px; height: 32px; display: flex; align-items: center;
    justify-content: center; }
.tile .tile-icon img { width: 32px; height: 32px; }
.tile .tile-label { font-size: 11px; line-height: 1.2; }
img { -webkit-user-drag: none; user-drag: none; }
)raw");
        html += L"</style></head><body><div class='gallery'>";

        bool inSection{ false };
        GalleryGroup currentGroup{ GalleryGroup::Objects };
        for (const auto& item : WisteriaApp::GetGalleryItemCatalog())
            {
            if (!inSection || item.m_group != currentGroup)
                {
                if (inSection)
                    {
                    html += L"</div></section>";
                    }
                currentGroup = item.m_group;
                inSection = true;
                html +=
                    wxString::Format(L"<section class='group%s' data-group='%d'><header>",
                                     m_collapsedGroups.contains(currentGroup) ? L" collapsed" : L"",
                                     static_cast<int>(currentGroup));
                const auto groupIconName = GetGroupIconName(currentGroup);
                if (!groupIconName.empty())
                    {
                    const auto dataUri = BitmapToPngDataUri(
                        wxGetApp().GetResourceManager().GetSVG(groupIconName), wxSize{ 32, 32 });
                    if (!dataUri.empty())
                        {
                        html +=
                            wxString::Format(L"<img src='%s' alt='' draggable='false'>", dataUri);
                        }
                    }
                html += L"<span>" + EscapeForHtml(GetGroupDisplayName(currentGroup)) +
                        L"</span><span class='chevron'>&#9660;</span></header>";
                html += L"<div class='tiles'>";
                }

            const auto disabledPos = m_disabledItemTooltips.find(item.m_id);
            const bool isDisabled{ disabledPos != m_disabledItemTooltips.cend() };
            const wxString tooltipText = isDisabled ? disabledPos->second : item.m_displayName;
            const auto iconDataUri = BitmapToPngDataUri(
                wxGetApp().GetResourceManager().GetSVG(item.m_svgName), wxSize{ 64, 64 });

            html += wxString::Format(L"<div class='tile%s' data-type='%s' title='%s' tabindex='0'>"
                                     L"<div class='tile-icon'><img src='%s' alt='' "
                                     L"draggable='false'></div>"
                                     L"<div class='tile-label'>%s</div></div>",
                                     isDisabled ? L" disabled" : L"",
                                     GalleryItemTypeToId(item.m_id), EscapeForHtml(tooltipText),
                                     iconDataUri, EscapeForHtml(item.m_displayName));
            }
        if (inSection)
            {
            html += L"</div></section>";
            }
        html += L"</div>";

        html += wxString::Format(_DT(LR"raw(<script>
(function(){
    var active = null;
    function post(msg) {
        try { window.%s.postMessage(msg); } catch(err) { /* backend without messaging */ }
    }
    document.querySelectorAll('.tile').forEach(function(tile){
        tile.addEventListener('dragstart', function(e){ e.preventDefault(); });
        tile.addEventListener('pointerdown', function(e){
            if (tile.classList.contains('disabled')) { return; }
            e.preventDefault();
            try { tile.setPointerCapture(e.pointerId); } catch(err) {}
            tile.classList.add('picked-up');
            active = tile;
            post('dragstart|' + tile.getAttribute('data-type'));
        });
        var endDrag = function(){
            if (active === tile) {
                tile.classList.remove('picked-up');
                active = null;
            }
            post('dragend');
        };
        tile.addEventListener('pointerup', endDrag);
        tile.addEventListener('pointercancel', endDrag);
        tile.addEventListener('lostpointercapture', endDrag);
    });
    document.querySelectorAll('section.group > header').forEach(function(header){
        header.addEventListener('click', function(){
            var section = header.parentElement;
            var collapsed = section.classList.toggle('collapsed');
            post((collapsed ? 'collapse|' : 'expand|') + section.getAttribute('data-group'));
        });
    });
})();
</script>)raw"),
                                 wxString{ ScriptMessageHandlerName });

        html += L"</body></html>";
        return html;
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::OnScriptMessage(wxWebViewEvent& event)
        {
        const wxString msg = event.GetString();
        static const wxString dragStartPrefix{ L"dragstart|" };
        static const wxString collapsePrefix{ L"collapse|" };
        static const wxString expandPrefix{ L"expand|" };
        if (msg.StartsWith(dragStartPrefix))
            {
            if (const auto itemType = IdToGalleryItemType(msg.substr(dragStartPrefix.length()));
                itemType.has_value())
                {
                StartDrag(itemType.value());
                }
            }
        else if (msg == L"dragend")
            {
            EndDrag(true);
            }
        else if (msg.StartsWith(collapsePrefix))
            {
            long groupId{ 0 };
            if (msg.substr(collapsePrefix.length()).ToLong(&groupId))
                {
                m_collapsedGroups.insert(static_cast<GalleryGroup>(groupId));
                PersistCollapsedGroups();
                }
            }
        else if (msg.StartsWith(expandPrefix))
            {
            long groupId{ 0 };
            if (msg.substr(expandPrefix.length()).ToLong(&groupId))
                {
                m_collapsedGroups.erase(static_cast<GalleryGroup>(groupId));
                PersistCollapsedGroups();
                }
            }
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::PersistCollapsedGroups() const
        {
        auto& settings = wxGetApp().GetAppSettings();
        settings->SetCollapsedGalleryGroups(m_collapsedGroups);
        settings->SaveSettingsFile();
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::StartDrag(const GalleryItemType itemType)
        {
        if (m_disabledItemTooltips.contains(itemType))
            {
            return;
            }

        m_isDragging = true;
        m_draggedItemType = itemType;

        const auto ghostSize = FromDIP(wxSize{ 40, 40 });
        if (m_ghostWindow == nullptr)
            {
            m_ghostWindow = new wxFrame(this, wxID_ANY, wxString{}, wxDefaultPosition, ghostSize,
                                        wxFRAME_TOOL_WINDOW | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP |
                                            wxBORDER_NONE);
            m_ghostBitmapCtrl = new wxStaticBitmap(m_ghostWindow, wxID_ANY, wxNullBitmap,
                                                   wxDefaultPosition, ghostSize);
            }

        for (const auto& item : WisteriaApp::GetGalleryItemCatalog())
            {
            if (item.m_id == itemType)
                {
                const auto bmpBundle = wxGetApp().GetResourceManager().GetSVG(item.m_svgName);
                if (bmpBundle.IsOk() && m_ghostBitmapCtrl != nullptr)
                    {
                    m_ghostBitmapCtrl->SetBitmap(bmpBundle.GetBitmap(ghostSize));
                    }
                break;
                }
            }

        const auto mousePos = wxGetMouseState().GetPosition();
        m_ghostWindow->SetPosition(mousePos -
                                   wxPoint{ ghostSize.GetWidth() / 2, ghostSize.GetHeight() / 2 });
        m_ghostWindow->Show();

        m_dragTimer.Start(16);
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::EndDrag([[maybe_unused]] const bool viaMessage)
        {
        if (!m_isDragging)
            {
            return;
            }
        m_isDragging = false;
        m_dragTimer.Stop();
        if (m_ghostWindow != nullptr)
            {
            m_ghostWindow->Hide();
            }

        const wxPoint screenPos = wxGetMouseState().GetPosition();
        if (m_dropTarget != nullptr)
            {
            const wxRect targetScreenRect{ m_dropTarget->GetScreenPosition(),
                                           m_dropTarget->GetSize() };
            if (targetScreenRect.Contains(screenPos))
                {
                ObjectGalleryItemDroppedEvent evt{ wxEVT_OBJECTGALLERY_ITEM_DROPPED,
                                                   m_dropTarget->GetId(), m_draggedItemType,
                                                   screenPos };
                evt.SetEventObject(this);
                m_dropTarget->GetEventHandler()->ProcessEvent(evt);
                }
            }
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::OnDragTimer([[maybe_unused]] wxTimerEvent& event)
        {
        if (!m_isDragging)
            {
            return;
            }

        const auto mouseState = wxGetMouseState();
        if (m_ghostWindow != nullptr)
            {
            const auto ghostSize = m_ghostWindow->GetSize();
            m_ghostWindow->SetPosition(
                mouseState.GetPosition() -
                wxPoint{ ghostSize.GetWidth() / 2, ghostSize.GetHeight() / 2 });
            }

        // safety net in case the webview's content never delivers "dragend"
        // (e.g., focus was stolen mid-drag)
        if (!mouseState.LeftIsDown())
            {
            EndDrag(false);
            }
        }

    //-------------------------------------------
    void ObjectGalleryCtrl::EnableItem(const GalleryItemType itemType, const bool enable,
                                       const wxString& disabledTooltip)
        {
        if (enable)
            {
            m_disabledItemTooltips.erase(itemType);
            }
        else
            {
            m_disabledItemTooltips[itemType] = disabledTooltip;
            }

        if (m_webView == nullptr)
            {
            return;
            }

        wxString displayName;
        for (const auto& item : WisteriaApp::GetGalleryItemCatalog())
            {
            if (item.m_id == itemType)
                {
                displayName = item.m_displayName;
                break;
                }
            }

        const wxString titleText = enable ? displayName : disabledTooltip;
        m_webView->RunScriptAsync(wxString::Format(
            _DT(L"(function(){ var t = document.querySelector('[data-type=\"%s\"]'); "
                "if (t) { t.classList.toggle('disabled', %s); t.title = '%s'; } })();"),
            GalleryItemTypeToId(itemType), enable ? L"false" : L"true", EscapeForHtml(titleText)));
        }
    } // namespace Wisteria::UI
