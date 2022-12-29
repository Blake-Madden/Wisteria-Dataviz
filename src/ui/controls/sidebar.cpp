///////////////////////////////////////////////////////////////////////////////
// Name:        sidebar.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sidebar.h"

using namespace Wisteria::UI;

wxDEFINE_EVENT(EVT_SIDEBAR_CLICK, wxCommandEvent);
wxDEFINE_EVENT(EVT_SIDEBAR_SHOWHIDE_CLICK, wxCommandEvent);

//---------------------------------------------------
SideBar::SideBar(wxWindow* parent, wxWindowID id /*= wxID_ANY*/)
        : wxScrolledCanvas(parent, id, wxDefaultPosition, wxDefaultSize,
                           wxWANTS_CHARS|wxVSCROLL|wxBORDER_THEME|wxFULL_REPAINT_ON_RESIZE),
          m_activeColor(wxT("#FDB759")),
          m_activeFontColor(*wxBLACK),
          m_highlightColor(wxColour(253, 211, 155)),
          m_highlightFontColor(*wxBLACK),
          m_parentColor(wxColour(180, 189, 207))
    {
    // Start off with enough height for a usual icon and some padding around it.
    // This will be adjusted in Realize() to take into account the actual height of the
    // text and any loaded icons.
    m_itemHeight = FromDIP(wxSize(16, 16)).GetHeight()+GetPaddingHeight();
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(wxColour(200, 211, 231));
    SetScrollbars(FromDIP(wxSize(30, 30)).GetWidth(),
                  FromDIP(wxSize(30, 30)).GetHeight(), 0, 0);
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    SetVirtualSize(GetSize().GetWidth(), GetSize().GetHeight());
    SetMinSize(wxSize(GetPaddingWidth(), m_itemHeight));

    // bind events
    Bind(wxEVT_KEY_DOWN, &SideBar::OnChar, this);
    Bind(wxEVT_PAINT, &SideBar::OnPaint, this);
    Bind(wxEVT_MOTION, &SideBar::OnMouseChange, this);
    Bind(wxEVT_LEAVE_WINDOW, &SideBar::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN, &SideBar::OnMouseClick, this);
    Bind(wxEVT_LEFT_DCLICK, &SideBar::OnDblClick, this);
    Bind(wxEVT_SIZE, &SideBar::OnResize, this);
    }

//---------------------------------------------------
void SideBar::DrawGlassEffect(wxDC& dc, const wxRect rect, const wxColour color)
    {
    // fill with the color
    dc.GradientFillLinear(rect, color, color.ChangeLightness(140),
        wxSOUTH);
    // create a shiny overlay
    dc.GradientFillLinear(wxRect(rect.GetX(), rect.GetY(),
        rect.GetWidth(),
        rect.GetHeight() * .25),
        color.ChangeLightness(115), color.ChangeLightness(155),
        wxSOUTH);
    }

//---------------------------------------------------
std::optional<size_t> SideBar::GetSelectedAnyItem() const
    {
    if (!GetSelectedFolder().has_value())
        { return std::nullopt; }
    size_t selectedSubItem = 0;
    for (size_t i = 0; i < GetSelectedFolder(); ++i)
        {
        ++selectedSubItem;
        selectedSubItem += m_items[i].GetSubItemCount();
        }
    selectedSubItem += m_items[GetSelectedFolder().value()].m_activeItem.has_value() ?
        m_items[GetSelectedFolder().value()].m_activeItem.value()+1 : 0;
    return selectedSubItem;
    }

//---------------------------------------------------
size_t SideBar::AdjustWidthToFitItems()
    {
    if (!IsExpanded())
        { return GetHideWidth(); }
    size_t sideBarMinimumWidth = FromDIP(wxSize(wxSizerFlags::GetDefaultBorder() * 10,
                                         wxSizerFlags::GetDefaultBorder() * 10)).GetWidth();
    for (size_t i = 0; i < GetFolderCount(); ++i)
        { sideBarMinimumWidth = std::max(GetFolderWidth(i), sideBarMinimumWidth); }
    SetMinSize(wxSize(sideBarMinimumWidth, -1));
    return sideBarMinimumWidth;
    }

//---------------------------------------------------
void SideBar::OnResize(wxSizeEvent& event)
    {
    RecalcSizes();
    event.Skip();
    }

//---------------------------------------------------
void SideBar::CollapseAll()
    {
    for (auto& item : m_items)
        { item.Collapse(); }
    RecalcSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void SideBar::ExpandAll()
    {
    for (auto& item : m_items)
        { item.Expand(); }
    RecalcSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void SideBar::SaveState()
    {
    m_savedActiveItem = m_activeItem;
    m_stateInfo.clear();
    for (const auto& item : m_items)
        { m_stateInfo.insert(SideBarStateInfo(item)); }
    }

//---------------------------------------------------
void SideBar::ResetState()
    {
    m_activeItem = m_savedActiveItem;
    for (auto& item : m_items)
        {
        const auto statePos = m_stateInfo.find(SideBarStateInfo(item));
        if (statePos != m_stateInfo.cend())
            {
            item.m_activeItem = statePos->m_activeItem;
            if ((item.m_subItems.size() && !item.m_activeItem.has_value()) ||
                (item.m_activeItem.has_value() &&
                    item.m_activeItem.value() >= item.m_subItems.size()))
                { item.m_activeItem = 0; }
            item.m_isExpanded = statePos->m_isExpanded;
            }
        }
    RecalcSizes();
    }

//-------------------------------------------
void SideBar::OnChar(wxKeyEvent& event)
    {
    ClearHighlightedItems();
    // if going down, select next item. If it has subitems, then select the first subitem.
    if (event.GetKeyCode() == WXK_DOWN)
        {
        if (IsFolderSelected() && m_items[GetSelectedFolder().value()].GetSubItemCount() &&
            m_items[GetSelectedFolder().value()].m_activeItem.has_value() &&
            m_items[GetSelectedFolder().value()].m_activeItem.value() <
                m_items[GetSelectedFolder().value()].GetSubItemCount()-1)
            {
            SelectSubItem(GetSelectedFolder().value(),
                          m_items[GetSelectedFolder().value()].m_activeItem.value() + 1);
            }
        else if (IsFolderSelected() && GetSelectedFolder().value() < GetFolderCount()-1)
            { SelectSubItem(GetSelectedFolder().value() + 1, 0); }
        }
    // if going up, select the previous item. If it has subitems, then select the last subitem.
    else if (event.GetKeyCode() == WXK_UP)
        {
        if (IsFolderSelected() && m_items[GetSelectedFolder().value()].GetSubItemCount() &&
            m_items[GetSelectedFolder().value()].m_activeItem.has_value() &&
            m_items[GetSelectedFolder().value()].m_activeItem.value() > 0)
            {
            SelectSubItem(GetSelectedFolder().value(),
                          m_items[GetSelectedFolder().value()].m_activeItem.value() - 1);
            }
        else if (IsFolderSelected() && GetSelectedFolder().value() > 0)
            {
            SelectSubItem(GetSelectedFolder().value() - 1,
                m_items[GetSelectedFolder().value() - 1].GetSubItemCount() ?
                    m_items[GetSelectedFolder().value() - 1].GetSubItemCount()-1 : 0);
            }
        }
    // going right will expanded the selected item
    else if (event.GetKeyCode() == WXK_RIGHT)
        {
        if (IsFolderSelected() && !m_items[GetSelectedFolder().value()].m_isExpanded)
            {
            m_items[GetSelectedFolder().value()].Expand();
            RecalcSizes();
            Refresh();
            }
        }
    // going left will collapse the selected item
    else if (event.GetKeyCode() == WXK_LEFT)
        {
        if (IsFolderSelected() && m_items[GetSelectedFolder().value()].m_isExpanded)
            {
            m_items[GetSelectedFolder().value()].Collapse();
            RecalcSizes();
            Refresh();
            }
        }
    else
        { event.Skip(); }
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<wxWindowID>> SideBar::GetSelectedSubItemId() const
    {
    if (!IsFolderSelected() || m_items[GetSelectedFolder().value()].GetSubItemCount() == 0 ||
        !m_items[GetSelectedFolder().value()].IsSubItemSelected())
        {
        return std::pair<std::optional<size_t>,
                         std::optional<wxWindowID>>(std::nullopt, std::nullopt);
        }
    else
        {
        return std::pair<wxWindowID, wxWindowID>(m_items[GetSelectedFolder().value()].m_id,
            m_items[GetSelectedFolder().value()].
                m_subItems[m_items[GetSelectedFolder().value()].m_activeItem.value()].m_id);
        }
    }

//---------------------------------------------------
wxString SideBar::GetSelectedLabel() const
    {
    if (!IsFolderSelected())
        { return wxEmptyString; }
    else if (m_items[GetSelectedFolder().value()].GetSubItemCount() == 0)
        { return GetFolderText(GetSelectedFolder().value()); }
    else if (m_items[GetSelectedFolder().value()].IsSubItemSelected())
        {
        return m_items[GetSelectedFolder().value()].
            m_subItems[m_items[GetSelectedFolder().value()].m_activeItem.value()].m_label;
        }
    else
        { return wxEmptyString; }
    }

//---------------------------------------------------
std::optional<size_t> SideBar::FindFolder(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_id == Id)
            { return i; }
        }
    return std::nullopt;
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
            {
            if (m_items[i].m_subItems[j].m_id == Id)
                { return std::pair<int,int>(i,j); }
            }
        }
    return std::make_pair(std::nullopt, std::nullopt);
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxString& label) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
            {
            if (m_items[i].m_subItems[j].m_label.CmpNoCase(label) == 0)
                { return std::pair<int,int>(i,j); }
            }
        }
    return std::make_pair(std::nullopt, std::nullopt);
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxWindowID parentId,
                         const wxWindowID subItemId) const
    {
    const auto parentIndex = FindFolder(parentId);
    if (!parentIndex.has_value())
        { return std::make_pair(std::nullopt, std::nullopt); }
    for (size_t j = 0; j < m_items[parentIndex.value()].m_subItems.size(); ++j)
        {
        if (m_items[parentIndex.value()].m_subItems[j].m_id == subItemId)
            { return std::make_pair(parentIndex.value(), j); }
        }
    return std::make_pair(std::nullopt, std::nullopt);
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxWindowID parentId,
                         const SideBarSubItem& subItem) const
    {
    const auto parentIndex = FindFolder(parentId);
    if (!parentIndex.has_value())
        { return std::make_pair(std::nullopt, std::nullopt); }
    for (size_t j = 0; j < m_items[parentIndex.value()].m_subItems.size(); ++j)
        {
        if (m_items[parentIndex.value()].m_subItems[j].m_id == subItem.m_id &&
            (m_items[parentIndex.value()].m_subItems[j].m_iconIndex == subItem.m_iconIndex ||
             m_items[parentIndex.value()].m_subItems[j].m_label == subItem.m_label))
            { return std::make_pair(parentIndex, j); }
        }
    return std::make_pair(std::nullopt, std::nullopt);
    }

//---------------------------------------------------
size_t SideBar::GetFolderWidth(const size_t item)
    {
    wxASSERT_LEVEL_2(item < GetFolderCount());
    if (item >= GetFolderCount())
        { return 0; }
    wxClientDC dc(this);
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxCoord textWidth(0), textHeight(0);
    dc.GetTextExtent(m_items[item].m_label, &textWidth, &textHeight);
    wxCoord parentWidth = textWidth;
    parentWidth += IsValidImageId(m_items[item].m_iconIndex) ?
        GetImageList()[m_items[item].m_iconIndex.value()].GetSize().GetWidth() +
        wxSizerFlags::GetDefaultBorder() : 0;
    parentWidth += GetPaddingWidth(); // padding around the label
    // see what the widest subitem is
    wxCoord widestSubItem = 0;
    for (const auto& subItem : m_items[item].m_subItems)
        {
        dc.GetTextExtent(subItem.m_label, &textWidth, &textHeight);
        textWidth += IsValidImageId(subItem.m_iconIndex) ?
            GetImageList()[subItem.m_iconIndex.value()].GetSize().GetWidth() +
            wxSizerFlags::GetDefaultBorder() : 0;
        textWidth += GetPaddingWidth() + GetSubitemIndentation(); // padding around the label
        widestSubItem = std::max(textWidth,widestSubItem);
        }
    return std::max(parentWidth+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X),
                    widestSubItem+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
    }

//---------------------------------------------------
void SideBar::InsertItem(const size_t position, const wxString& label,
                           const wxWindowID Id, std::optional<size_t> iconIndex)
    {
    SideBarItem item;
    item.m_id = Id;
    item.m_iconIndex = iconIndex;
    item.m_label = label;
    if (position > m_items.size())
        { m_items.resize(position); }
    m_items.insert(m_items.begin()+position, item);
    }

//---------------------------------------------------
bool SideBar::InsertSubItemById(const wxWindowID parentItemId, const wxString& label,
                                const wxWindowID Id, std::optional<size_t> iconIndex)
    {
    SideBarSubItem subitem;
    subitem.m_id = Id;
    subitem.m_iconIndex = iconIndex;
    subitem.m_label = label;

    const auto parent = FindFolder(parentItemId);
    if (!parent.has_value())
        { return false; }
    m_items[parent.value()].m_subItems.push_back(subitem);
    if (!m_items[parent.value()].m_highlightedItem.has_value())
        { m_items[parent.value()].m_highlightedItem = 0; }
    if (!m_items[parent.value()].m_activeItem.has_value())
        { m_items[parent.value()].m_activeItem = 0; }
    return true;
    }

//---------------------------------------------------
void SideBar::ClearHighlightedItems() noexcept
    {
    m_highlightedRect = std::nullopt;
    m_highlightedFolder = std::nullopt;
    m_folderWithHighlightedSubitem = std::make_pair(std::nullopt, std::nullopt);
    for (auto& item : m_items)
        { item.m_highlightedItem = std::nullopt; }
    }

//---------------------------------------------------
void SideBar::OnPaint([[maybe_unused]] wxPaintEvent& event)
    {
    if (GetFolderCount() == 0)
        { return; }

    // if mouse is not inside of window, then turn off any item mouse highlighting
    if (!GetScreenRect().Contains(wxGetMousePosition()))
        { ClearHighlightedItems(); }

    wxAutoBufferedPaintDC adc(this);
    adc.Clear();
    wxGCDC dc(adc);

    wxDCFontChanger fc(dc, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxDCTextColourChanger tc(dc, GetForegroundColour());

    if (HasShowHideToolbar())
        {
        if (GetStyle() == SidebarStyle::Glassy)
            {
            DrawGlassEffect(dc, wxRect(0, 0, GetSize().GetWidth(), GetToolbarHeight()),
                            m_parentColor);
            }
        else
            {
            wxDCBrushChanger bc(dc, m_parentColor);
            wxDCPenChanger pc(dc, m_parentColor);
            dc.DrawRectangle(wxRect(0, 0, GetSize().GetWidth(), GetToolbarHeight()));
            }
        dc.DrawBitmap(wxArtProvider::GetBitmap(IsExpanded() ?
                wxART_GO_BACK : wxART_GO_FORWARD, wxART_BUTTON, FromDIP(wxSize(16, 16))),
            IsExpanded() ?
                GetClientSize().GetWidth() - (FromDIP(wxSize(16,16)).GetWidth() +
                    wxSizerFlags::GetDefaultBorder()) :
                (GetClientSize().GetWidth()/2) - (FromDIP(wxSize(16, 16)).GetWidth()/2),
            (GetToolbarHeight()/2) - (FromDIP(wxSize(16, 16)).GetHeight()/2));
        wxDCPenChanger pc(dc, wxColour(m_parentColor).ChangeLightness(50));
        dc.DrawLine(wxPoint(0,GetToolbarHeight()-1), 
                    wxPoint(GetSize().GetWidth(), GetToolbarHeight()-1));
        // if the control is hidden (i.e., collapsed horizontally),
        // then just display the "show" button
        if (!IsExpanded())
            { return; }
        }

    // draw the borders for the parent items
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        const wxRect buttonBorderRect = m_items[i].m_Rect;
        if (GetSelectedFolder().has_value() &&
            GetSelectedFolder().value() == i && !m_items[i].m_isExpanded)
            {
            if (GetStyle() == SidebarStyle::Glassy)
                {
                DrawGlassEffect(dc,
                    wxRect(buttonBorderRect.GetLeftTop().x,
                           buttonBorderRect.GetLeftTop().y,
                           GetSize().GetWidth(), GetItemHeight()),
                    m_activeColor); }
            else
                {
                wxDCBrushChanger bc(dc, m_activeColor);
                wxDCPenChanger pc(dc, m_activeColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                                buttonBorderRect.GetLeftTop().y,
                                 GetSize().GetWidth(), GetItemHeight()));
                }
            }
        else if (m_highlightedFolder.has_value() && m_highlightedFolder.value() == i)
            {
            if (GetStyle() == SidebarStyle::Glassy)
                {
                DrawGlassEffect(dc,
                    wxRect(buttonBorderRect.GetLeftTop().x,
                           buttonBorderRect.GetLeftTop().y,
                           GetSize().GetWidth(), GetItemHeight()),
                    m_highlightColor);
                }
            else
                {
                wxDCBrushChanger bc(dc, m_highlightColor);
                wxDCPenChanger pc(dc, m_highlightColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                                        buttonBorderRect.GetLeftTop().y,
                                        GetSize().GetWidth(), GetItemHeight()));
                }
            }
        else
            {
            if (GetStyle() == SidebarStyle::Glassy)
                {
                DrawGlassEffect(dc, wxRect(buttonBorderRect.GetLeftTop().x,
                                           buttonBorderRect.GetLeftTop().y,
                    GetSize().GetWidth(), GetItemHeight()), m_parentColor);
                }
            else
                {
                wxDCBrushChanger bc(dc, m_parentColor);
                wxDCPenChanger pc(dc, m_parentColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                                        buttonBorderRect.GetLeftTop().y,
                                        GetSize().GetWidth(), GetItemHeight()));
                }
            }
        }

    // draw the parent labels and subitems
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        // draw parent labels
            {
            wxDCTextColourChanger tcc(dc,
                (m_highlightedFolder.has_value() && m_highlightedFolder.value() == i) ?
                    m_highlightFontColor :
                    (GetSelectedFolder().has_value() && GetSelectedFolder().value() == i &&
                     !m_items[i].m_isExpanded) ?
                        m_activeFontColor :
                        GetForegroundColour());
            if (IsValidImageId(m_items[i].m_iconIndex))
                {
                dc.DrawLabel(m_items[i].m_label,
                             GetImageList()[m_items[i].m_iconIndex.value()],
                             wxRect(m_items[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            else
                {
                dc.DrawLabel(m_items[i].m_label, wxNullBitmap,
                             wxRect(m_items[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            }
        // draw subitems (if parent item is expanded)
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_highlightedItem.has_value() &&
                    m_items[i].m_highlightedItem.value() == j)
                    {
                    const wxRect buttonBorderRect = m_items[i].m_subItems[j].m_Rect;
                    if (GetStyle() == SidebarStyle::Glassy)
                        {
                        DrawGlassEffect(dc,
                            wxRect(buttonBorderRect.GetLeftTop().x,
                                   buttonBorderRect.GetLeftTop().y,
                                   GetSize().GetWidth() >= GetSubitemIndentation() ?
                                       GetSize().GetWidth()-GetSubitemIndentation() : 0,
                                   GetItemHeight()),
                            m_highlightColor);
                        }
                    else
                        {
                        wxDCBrushChanger bc(dc, m_highlightColor);
                        wxDCPenChanger pc(dc, m_highlightColor);
                        dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                                         buttonBorderRect.GetLeftTop().y,
                                         GetSize().GetWidth() >= GetSubitemIndentation() ?
                                            GetSize().GetWidth()-GetSubitemIndentation() : 0,
                                         GetItemHeight()));
                        }
                    }
                if (GetSelectedFolder().has_value() &&
                    GetSelectedFolder().value() == i &&
                    m_items[i].m_activeItem.has_value() &&
                    m_items[i].m_activeItem.value() == j)
                    {
                    const wxRect buttonBorderRect = m_items[i].m_subItems[j].m_Rect;
                    if (GetStyle() == SidebarStyle::Glassy)
                        {
                        DrawGlassEffect(dc, wxRect(buttonBorderRect.GetLeftTop().x,
                            buttonBorderRect.GetLeftTop().y,
                            GetSize().GetWidth() >= GetSubitemIndentation() ?
                                GetSize().GetWidth()-GetSubitemIndentation() : 0,
                            GetItemHeight()), m_activeColor);
                        }
                    else
                        {
                        wxDCBrushChanger bc(dc, m_activeColor);
                        wxDCPenChanger pc(dc, m_activeColor);
                        dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                            buttonBorderRect.GetLeftTop().y,
                            GetSize().GetWidth() >= GetSubitemIndentation() ?
                                GetSize().GetWidth()-GetSubitemIndentation() : 0,
                            GetItemHeight()));
                        }
                    }
                // draw the subitem labels
                    {
                    wxDCTextColourChanger tcc(dc,
                        (m_items[i].m_highlightedItem.has_value() &&
                         m_items[i].m_highlightedItem.value() == j) ?
                            m_highlightFontColor :
                            (GetSelectedFolder().has_value() && GetSelectedFolder().value() == i &&
                             m_items[i].m_activeItem.has_value() &&
                             m_items[i].m_activeItem.value() == j) ?
                                m_activeFontColor : GetForegroundColour());
                    if (IsValidImageId(m_items[i].m_subItems[j].m_iconIndex))
                        {
                        dc.DrawLabel(m_items[i].m_subItems[j].m_label,
                                     GetImageList()[m_items[i].m_subItems[j].m_iconIndex.value()],
                                     wxRect(m_items[i].m_subItems[j].m_Rect).
                                        Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    else
                        {
                        dc.DrawLabel(m_items[i].m_subItems[j].m_label,
                                     wxNullBitmap,
                                     wxRect(m_items[i].m_subItems[j].m_Rect).
                                        Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    }
                }
            }
        }
    }

//-------------------------------------------
void SideBar::OnMouseChange(wxMouseEvent& event)
    {
    if (HasShowHideToolbar())
        {
        int x{ 0 }, y{ 0 };
        CalcUnscrolledPosition(0, 0, &x, &y);
        if (m_toolbarRect.Contains(event.GetX()+x, event.GetY()+y))
            {
            SetToolTip(IsExpanded() ?
                _("Click to hide sidebar") :
                _("Click to show sidebar"));
            }
        else
            { SetToolTip(wxEmptyString); }
        // if not shown, then don't bother handling hover
        // events for items that aren't being displayed
        if (!IsExpanded())
            { return; }
        }

    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    const auto previouslyHighlightedRect = m_highlightedRect;
    const auto previouslyHighlightedFolder = m_highlightedFolder;
    const auto previouslyHighlightedSubitem = m_folderWithHighlightedSubitem;
    ClearHighlightedItems();

    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            m_highlightedFolder = i;
            m_highlightedRect = m_items[i].m_Rect;
            m_items[i].m_highlightedItem = std::nullopt;
            // mouse is over the same folder as before, don't bother repainting
            if (previouslyHighlightedFolder.has_value() &&
                previouslyHighlightedFolder.value() == i)
                { return; }
            break;
            }
        // If a parent item isn't being moused over, then see if its
        // expanded subitems are being moused over.
        // Otherwise, turn off the subitems' highlighting.
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_subItems[j].m_Rect.
                    Contains(event.GetX()+x, event.GetY()+y))
                    {
                    m_items[i].m_highlightedItem = j;
                    m_highlightedRect = m_items[i].m_subItems[j].m_Rect;
                    m_folderWithHighlightedSubitem = std::make_pair(i, j);
                    // mouse is over the same subitem as before, don't bother repainting
                    if (previouslyHighlightedSubitem.first.has_value() &&
                        previouslyHighlightedSubitem.second.has_value() &&
                        previouslyHighlightedSubitem.first.value() == i &&
                        previouslyHighlightedSubitem.second.value() == j)
                        { return; }
                    break;
                    }
                else
                    { m_items[i].m_highlightedItem = std::nullopt; }
                }
            }
        }

    // if nothing highlighted and nothing was highlighed before, then don't repaint
    if (!previouslyHighlightedRect && !m_highlightedRect)
        { return; }

    Refresh();
    Update();
    }

//-------------------------------------------
void SideBar::OnMouseLeave([[maybe_unused]] wxMouseEvent& event)
    {
    // if not shown, then don't bother handling hover events for
    // items that aren't being displayed
    if (!IsExpanded())
        { return; }
    ClearHighlightedItems();
    Refresh();
    Update();
    }

//-------------------------------------------
void SideBar::Minimize()
    {
    m_isExpanded = false;
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
    SetMinSize(wxSize(GetHideWidth(), wxDefaultCoord));
    SetSize(GetHideWidth(), wxDefaultCoord);

    wxCommandEvent cevent(EVT_SIDEBAR_SHOWHIDE_CLICK, GetId());
    cevent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(cevent);
    }

//-------------------------------------------
void SideBar::Maximize()
    {
    m_isExpanded = true;
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    const size_t sideBarMinimumWidth = AdjustWidthToFitItems();
    SetMinSize(wxSize(sideBarMinimumWidth, wxDefaultCoord));
    SetSize(sideBarMinimumWidth, wxDefaultCoord);

    wxCommandEvent cevent(EVT_SIDEBAR_SHOWHIDE_CLICK, GetId());
    cevent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(cevent);
    }

//-------------------------------------------
void SideBar::OnMouseClick(wxMouseEvent& event)
    {
    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    if (HasShowHideToolbar())
        {
        // if clicking on the show/hide toolbar, then adjust the size of the control
        // and inform the parent control in case it needs to handle this event too
        if (m_toolbarRect.Contains(event.GetX()+x, event.GetY()+y))
            {
            m_isExpanded = !m_isExpanded;
            IsExpanded() ? Maximize() : Minimize();
            return;
            }
        // if not shown, then don't bother handling click events for items
        // that aren't being displayed
        if (!IsExpanded())
            { return; }
        }

    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            SelectFolder(i);
            return;
            }
        // if a parent item isn't being moused over,
        // then see if it expanded subitems being moused over
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_subItems[j].m_Rect.
                    Contains(event.GetX()+x, event.GetY()+y) )
                    {
                    SelectSubItem(i, j);
                    return;
                    }
                }
            }
        }
    }

//-------------------------------------------
void SideBar::OnDblClick(wxMouseEvent& event)
    {
    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            SelectFolder(i);
            // flip collapsed state
            m_items[i].m_isExpanded = !m_items[i].m_isExpanded;
            RecalcSizes();
            Refresh();
            Update();
            return;
            }
        }
    }

//-------------------------------------------
void SideBar::Realize()
    {
    for (const auto& bmp : GetImageList())
        {
        if (bmp.IsOk() && bmp.GetHeight()+GetPaddingHeight() > GetItemHeight())
            { m_itemHeight = bmp.GetHeight()+GetPaddingHeight(); }
        }
    // measure the items' text heights
    wxMemoryDC dc;
    for (const auto& item : m_items)
        {
        const auto sz = dc.GetMultiLineTextExtent(item.m_label);
        if (sz.GetHeight()+GetPaddingHeight() > GetItemHeight())
            { m_itemHeight = sz.GetHeight()+GetPaddingHeight(); }
        // measure the item's subitems text heights
        for (const auto& subitem : item.m_subItems)
            {
            const auto subSz = dc.GetMultiLineTextExtent(subitem.m_label);
            if (subSz.GetHeight()+GetPaddingHeight() > GetItemHeight())
                { m_itemHeight = subSz.GetHeight()+GetPaddingHeight(); }
            }
        }
    }

//-------------------------------------------
void SideBar::RecalcSizes()
    {
    // adjust (or show/hide) scrollbars and update the items' positions
    SetVirtualSize(GetSize().GetWidth(), CalculateItemRects());
    }

//-------------------------------------------
size_t SideBar::CalculateItemRects()
    {
    if (HasShowHideToolbar())
        { m_toolbarRect = wxRect(0, 0, GetSize().GetWidth(), GetToolbarHeight()); }
    size_t subItemHeight{ 0 };
    for (auto pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        {
        pos->m_Rect = wxRect(0,
            (GetItemHeight()*(pos - m_items.begin())) + subItemHeight+GetToolbarHeight(), 
            GetClientSize().GetWidth(), GetItemHeight());
        // if an expanded item, then factor in its subitems' heights for the rest of the items
        if (pos->m_isExpanded)
            {
            for (size_t j = 0; j < pos->m_subItems.size(); ++j)
                {
                pos->m_subItems[j].m_Rect = wxRect(0,
                    (GetItemHeight() * (pos - m_items.begin()+1)) +
                        subItemHeight + GetToolbarHeight(),
                    GetClientSize().GetWidth() - GetSubitemIndentation(),
                    GetItemHeight());
                pos->m_subItems[j].m_Rect.Offset(GetSubitemIndentation(), 0);
                subItemHeight += GetItemHeight();
                }
            }
        }
    return (GetItemHeight() * m_items.size()) + subItemHeight;
    }

//-------------------------------------------
void SideBar::EnsureFolderVisible(const size_t index)
    {
    if (index >= GetFolderCount() || !IsExpanded())
        { return; }

    int x{ 0 }, y{ 0 }, xUnit{ 0 }, yUnit{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);
    GetScrollPixelsPerUnit(&xUnit,&yUnit);
    wxRect scrolledClientRect = GetClientRect();
    scrolledClientRect.Offset(x, y);
    const wxRect lastItemRectToBeVisible =
        (m_items[index].m_isExpanded && m_items[index].GetSubItemCount() &&
         m_items[index].IsSubItemSelected()) ?
        m_items[index].m_subItems[m_items[index].m_activeItem.value()].m_Rect :
            m_items[index].m_Rect;
    if (!scrolledClientRect.Contains(lastItemRectToBeVisible) && yUnit != 0)
        {
        // scroll to the middle of the item
        // (safe compromise when we could be scrolling up or down)
        Scroll(wxDefaultCoord, (lastItemRectToBeVisible.GetY() +
               (lastItemRectToBeVisible.GetHeight()/2))/yUnit);
        }
    }

//-------------------------------------------
void SideBar::SelectFolder(const size_t item, const bool setFocus /*= true*/,
                           const bool sendEvent /*= true*/)
    {
    if (item >= GetFolderCount())
        { return; }
    // first, see if we should fire a subitem selection event instead
    // (in case this parent has subitems)
    if (m_items[item].GetSubItemCount())
        {
        if (!m_items[item].IsSubItemSelected())
            { m_items[item].m_activeItem = 0; }
        SelectSubItem(item, m_items[item].m_activeItem.value(), setFocus, sendEvent);
        return;
        }

    m_activeItem = item;
    m_items[GetSelectedFolder().value()].Expand();

    RecalcSizes();
    EnsureFolderVisible(GetSelectedFolder().value());
    Refresh();
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(EVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_items[GetSelectedFolder().value()].m_label);
        cevent.SetInt(m_items[GetSelectedFolder().value()].m_id);
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }
    }

//-------------------------------------------
void SideBar::SelectAnyItem(const size_t item, const bool setFocus /*= true*/,
                            const bool sendEvent /*= true*/)
    {
    size_t itemToSelect = 0;
    for (size_t i = 0; i < GetFolderCount(); ++i)
        {
        if (itemToSelect == item)
            {
            SelectFolder(i, setFocus, sendEvent);
            return;
            }
        for (size_t j = 0; j < m_items[i].GetSubItemCount(); ++j)
            {
            ++itemToSelect;
            if (itemToSelect == item)
                {
                SelectSubItem(i, j, setFocus, sendEvent);
                return;
                }
            }
        ++itemToSelect;
        }
    }

//-------------------------------------------
void SideBar::SelectSubItem(const size_t item, const size_t subItem,
                            const bool setFocus /*= true*/,
                            const bool sendEvent /*= true*/)
    {
    if (item >= GetFolderCount())
        { return; }
    // if bogus subitem, then just select the parent item
    if (m_items[item].GetSubItemCount() == 0 ||
        subItem >= m_items[item].GetSubItemCount())
        {
        SelectFolder(item, setFocus, sendEvent);
        return;
        }
    m_activeItem = item;
    m_items[GetSelectedFolder().value()].Expand();
    m_items[GetSelectedFolder().value()].m_activeItem = subItem;

    RecalcSizes();
    EnsureFolderVisible(GetSelectedFolder().value());
    Refresh();
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(EVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_items[GetSelectedFolder().value()].m_subItems[subItem].m_label);
        cevent.SetExtraLong(m_items[GetSelectedFolder().value()].m_id);
        cevent.SetInt(m_items[GetSelectedFolder().value()].m_subItems[subItem].m_id);
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }
    }
