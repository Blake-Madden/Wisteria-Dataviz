///////////////////////////////////////////////////////////////////////////////
// Name:        sidebar.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sidebar.h"

using namespace Wisteria::UI;

wxDEFINE_EVENT(EVT_SIDEBAR_CLICK, wxCommandEvent);

//---------------------------------------------------
SideBar::SideBar(wxWindow* parent, wxWindowID id /*= wxID_ANY*/)
        : wxScrolledCanvas(parent, id, wxDefaultPosition, wxDefaultSize,
                           wxWANTS_CHARS|wxVSCROLL|wxBORDER_THEME|
                           wxCLIP_CHILDREN|wxFULL_REPAINT_ON_RESIZE)
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
        selectedSubItem += m_folders[i].GetSubItemCount();
        }
    selectedSubItem += m_folders[GetSelectedFolder().value()].m_selectedItem.has_value() ?
        m_folders[GetSelectedFolder().value()].m_selectedItem.value()+1 : 0;
    return selectedSubItem;
    }

//---------------------------------------------------
size_t SideBar::AdjustWidthToFitItems()
    {
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
    for (auto& item : m_folders)
        { item.Collapse(); }
    RecalcSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void SideBar::ExpandAll()
    {
    for (auto& item : m_folders)
        { item.Expand(); }
    RecalcSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void SideBar::SaveState()
    {
    m_savedSelectedItem = m_selectedFolder;
    m_stateInfo.clear();
    for (const auto& item : m_folders)
        { m_stateInfo.insert(SideBarStateInfo(item)); }
    }

//---------------------------------------------------
void SideBar::ResetState()
    {
    m_selectedFolder = m_savedSelectedItem;
    for (auto& item : m_folders)
        {
        const auto statePos = m_stateInfo.find(SideBarStateInfo(item));
        if (statePos != m_stateInfo.cend())
            {
            item.m_selectedItem = statePos->m_selectedItem;
            if ((item.m_subItems.size() && !item.m_selectedItem.has_value()) ||
                (item.m_selectedItem.has_value() &&
                    item.m_selectedItem.value() >= item.m_subItems.size()))
                { item.m_selectedItem = 0; }
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
        if (IsFolderSelected() && m_folders[GetSelectedFolder().value()].GetSubItemCount() &&
            m_folders[GetSelectedFolder().value()].m_selectedItem.has_value() &&
            m_folders[GetSelectedFolder().value()].m_selectedItem.value() <
                m_folders[GetSelectedFolder().value()].GetSubItemCount()-1)
            {
            SelectSubItem(GetSelectedFolder().value(),
                          m_folders[GetSelectedFolder().value()].m_selectedItem.value() + 1);
            }
        else if (IsFolderSelected() && GetSelectedFolder().value() < GetFolderCount()-1)
            { SelectSubItem(GetSelectedFolder().value() + 1, 0); }
        }
    // if going up, select the previous item. If it has subitems, then select the last subitem.
    else if (event.GetKeyCode() == WXK_UP)
        {
        if (IsFolderSelected() && m_folders[GetSelectedFolder().value()].GetSubItemCount() &&
            m_folders[GetSelectedFolder().value()].m_selectedItem.has_value() &&
            m_folders[GetSelectedFolder().value()].m_selectedItem.value() > 0)
            {
            SelectSubItem(GetSelectedFolder().value(),
                          m_folders[GetSelectedFolder().value()].m_selectedItem.value() - 1);
            }
        else if (IsFolderSelected() && GetSelectedFolder().value() > 0)
            {
            SelectSubItem(GetSelectedFolder().value() - 1,
                m_folders[GetSelectedFolder().value() - 1].GetSubItemCount() ?
                    m_folders[GetSelectedFolder().value() - 1].GetSubItemCount()-1 : 0);
            }
        }
    // going right will expanded the selected item
    else if (event.GetKeyCode() == WXK_RIGHT)
        {
        if (IsFolderSelected() && !m_folders[GetSelectedFolder().value()].m_isExpanded)
            {
            m_folders[GetSelectedFolder().value()].Expand();
            RecalcSizes();
            Refresh();
            }
        }
    // going left will collapse the selected item
    else if (event.GetKeyCode() == WXK_LEFT)
        {
        if (IsFolderSelected() && m_folders[GetSelectedFolder().value()].m_isExpanded)
            {
            m_folders[GetSelectedFolder().value()].Collapse();
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
    if (!IsFolderSelected() ||
        m_folders[GetSelectedFolder().value()].GetSubItemCount() == 0 ||
        !m_folders[GetSelectedFolder().value()].IsSubItemSelected())
        {
        return std::make_pair(std::nullopt, std::nullopt);
        }
    else
        {
        return std::make_pair(m_folders[GetSelectedFolder().value()].m_id,
            m_folders[GetSelectedFolder().value()].
                m_subItems[m_folders[GetSelectedFolder().value()].m_selectedItem.value()].m_id);
        }
    }

//---------------------------------------------------
wxString SideBar::GetSelectedLabel() const
    {
    if (!IsFolderSelected())
        { return wxString{}; }
    else if (m_folders[GetSelectedFolder().value()].GetSubItemCount() == 0)
        { return GetFolderText(GetSelectedFolder().value()); }
    else if (m_folders[GetSelectedFolder().value()].IsSubItemSelected())
        {
        return m_folders[GetSelectedFolder().value()].
            m_subItems[m_folders[GetSelectedFolder().value()].m_selectedItem.value()].m_label;
        }
    else
        { return wxString{}; }
    }

//---------------------------------------------------
std::optional<size_t> SideBar::FindFolder(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        if (m_folders[i].m_id == Id)
            { return i; }
        }
    return std::nullopt;
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        for (size_t j = 0; j < m_folders[i].m_subItems.size(); ++j)
            {
            if (m_folders[i].m_subItems[j].m_id == Id)
                { return std::make_pair(i, j); }
            }
        }
    return std::make_pair(std::nullopt, std::nullopt);
    }

//---------------------------------------------------
std::pair<std::optional<size_t>, std::optional<size_t>>
    SideBar::FindSubItem(const wxString& label) const
    {
    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        for (size_t j = 0; j < m_folders[i].m_subItems.size(); ++j)
            {
            if (m_folders[i].m_subItems[j].m_label.CmpNoCase(label) == 0)
                { return std::make_pair(i, j); }
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
    for (size_t j = 0; j < m_folders[parentIndex.value()].m_subItems.size(); ++j)
        {
        if (m_folders[parentIndex.value()].m_subItems[j].m_id == subItemId)
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
    for (size_t j = 0; j < m_folders[parentIndex.value()].m_subItems.size(); ++j)
        {
        if (m_folders[parentIndex.value()].m_subItems[j].m_id == subItem.m_id &&
            (m_folders[parentIndex.value()].m_subItems[j].m_iconIndex == subItem.m_iconIndex ||
             m_folders[parentIndex.value()].m_subItems[j].m_label == subItem.m_label))
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
    dc.GetTextExtent(m_folders[item].m_label, &textWidth, &textHeight);
    wxCoord parentWidth = textWidth;
    parentWidth += IsValidImageId(m_folders[item].m_iconIndex) ?
        GetIconSize().GetWidth() +
        wxSizerFlags::GetDefaultBorder() : 0;
    parentWidth += GetPaddingWidth(); // padding around the label
    // see what the widest subitem is
    wxCoord widestSubItem = 0;
    for (const auto& subItem : m_folders[item].m_subItems)
        {
        dc.GetTextExtent(subItem.m_label, &textWidth, &textHeight);
        textWidth += IsValidImageId(subItem.m_iconIndex) ?
            GetIconSize().GetWidth() +
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
    if (position > m_folders.size())
        { m_folders.resize(position); }
    m_folders.insert(m_folders.begin()+position, item);
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
    m_folders[parent.value()].m_subItems.push_back(subitem);
    if (!m_folders[parent.value()].m_highlightedItem.has_value())
        { m_folders[parent.value()].m_highlightedItem = 0; }
    if (!m_folders[parent.value()].m_selectedItem.has_value())
        { m_folders[parent.value()].m_selectedItem = 0; }
    return true;
    }

//---------------------------------------------------
void SideBar::ClearHighlightedItems() noexcept
    {
    m_highlightedIsSelected = false;
    m_highlightedRect = std::nullopt;
    m_highlightedFolder = std::nullopt;
    m_folderWithHighlightedSubitem = std::make_pair(std::nullopt, std::nullopt);
    for (auto& item : m_folders)
        { item.m_highlightedItem = std::nullopt; }
    }

//---------------------------------------------------
void SideBar::OnDraw(wxDC& dc)
    {
    if (GetFolderCount() == 0)
        { return; }

    // update region is always in device coords, translate to logical ones
    wxRect rectUpdate = GetUpdateRegion().GetBox();
    CalcUnscrolledPosition(rectUpdate.x, rectUpdate.y,
                           &rectUpdate.x, &rectUpdate.y);

    std::optional<size_t> firstFolderToDraw{ std::nullopt };
    std::optional<size_t> lastFolderToDraw{ std::nullopt };
    std::optional<size_t> firstSubitemInFirstDrawnFolderToDraw{ std::nullopt };
    std::optional<size_t> lastSubitemInLastDrawnFolderToDraw{ std::nullopt };
    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        if (rectUpdate.Intersects(m_folders[i].m_Rect))
            {
            if (!firstFolderToDraw)
                { firstFolderToDraw = i; }
            lastFolderToDraw = i;
            lastSubitemInLastDrawnFolderToDraw = std::nullopt;
            }
        for (size_t j = 0; j < m_folders[i].m_subItems.size(); ++j)
            {
            if (rectUpdate.Intersects(m_folders[i].m_subItems[j].m_Rect))
                {
                // folder header was not in clipping rect,
                // but one of its subitems might be
                if (!firstFolderToDraw)
                    { firstFolderToDraw = i; }
                lastFolderToDraw = i;
                if (!firstSubitemInFirstDrawnFolderToDraw)
                    { firstSubitemInFirstDrawnFolderToDraw = j; }
                lastSubitemInLastDrawnFolderToDraw = j;
                }
            }
        }

    if (!firstFolderToDraw || !lastFolderToDraw)
        { return; }

    wxDCFontChanger fc(dc, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxDCTextColourChanger tc(dc, GetForegroundColour());

    // draw the borders and background for root-level folders
    const auto drawFolderBackground = [this, &dc](const size_t folderIndex)
        {
        const wxRect buttonBorderRect = m_folders[folderIndex].m_Rect;
        if (GetSelectedFolder().has_value() &&
            GetSelectedFolder().value() == folderIndex &&
            !m_folders[folderIndex].m_isExpanded)
            {
            if (GetStyle() == SidebarStyle::Glassy)
                {
                DrawGlassEffect(dc,
                    wxRect(buttonBorderRect.GetLeftTop().x,
                           buttonBorderRect.GetLeftTop().y,
                           GetSize().GetWidth(), GetItemHeight()),
                    m_selectedColor); }
            else
                {
                wxDCBrushChanger bc(dc, m_selectedColor);
                wxDCPenChanger pc(dc, m_selectedColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x,
                                 buttonBorderRect.GetLeftTop().y,
                                 GetSize().GetWidth(), GetItemHeight()));
                }
            }
        else if (m_highlightedFolder.has_value() &&
            m_highlightedFolder.value() == folderIndex)
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
        };

    // draw the folders and subitems
    wxASSERT_MSG(firstFolderToDraw.value() < m_folders.size(),
        wxString::Format(
            L"Sidebar first folder index out of range! (Folder %zu.)",
            firstFolderToDraw.value()));
    wxASSERT_MSG(lastFolderToDraw.value() < m_folders.size(),
        wxString::Format(
            L"Sidebar last folder index out of range! (Folder %zu.)",
            lastFolderToDraw.value()));
    for (size_t i = firstFolderToDraw.value();
        i <= lastFolderToDraw.value();
        ++i)
        {
        // draw the folder (i.e., parent label)
            {
            wxDCTextColourChanger tcc(dc,
                (m_highlightedFolder.has_value() && m_highlightedFolder.value() == i) ?
                    m_highlightFontColor :
                    (GetSelectedFolder().has_value() && GetSelectedFolder().value() == i &&
                     !m_folders[i].m_isExpanded) ?
                        m_selectedFontColor :
                        GetForegroundColour());
            drawFolderBackground(i);
            if (IsValidImageId(m_folders[i].m_iconIndex))
                {
                dc.DrawLabel(m_folders[i].m_label,
                             GetImageList()[m_folders[i].m_iconIndex.value()].GetBitmap(GetIconSize()),
                             wxRect(m_folders[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            else
                {
                dc.DrawLabel(m_folders[i].m_label, wxNullBitmap,
                             wxRect(m_folders[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            }
        // draw subitems (if folder is expanded)
        if (m_folders[i].m_isExpanded && m_folders[i].m_subItems.size() > 0)
            {
            const size_t startSubitem =
                (firstFolderToDraw.value() == i && firstSubitemInFirstDrawnFolderToDraw) ?
                std::clamp<size_t>(firstSubitemInFirstDrawnFolderToDraw.value(), 0,
                                   m_folders[i].m_subItems.size()-1) :
                0;
            const size_t endSubitem =
                (lastFolderToDraw.value() == i && lastSubitemInLastDrawnFolderToDraw) ?
                std::clamp<size_t>(lastSubitemInLastDrawnFolderToDraw.value(), 0,
                                   m_folders[i].m_subItems.size()-1) :
                m_folders[i].m_subItems.size()-1;
            for (size_t j = startSubitem; j <= endSubitem; ++j)
                {
                const bool subitemIsSelected =
                    (GetSelectedFolder().has_value() &&
                     GetSelectedFolder().value() == i &&
                     m_folders[i].m_selectedItem.has_value() &&
                     m_folders[i].m_selectedItem.value() == j);

                if (m_folders[i].m_highlightedItem.has_value() &&
                    m_folders[i].m_highlightedItem.value() == j &&
                    !subitemIsSelected)
                    {
                    const wxRect buttonBorderRect = m_folders[i].m_subItems[j].m_Rect;
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
                else if (subitemIsSelected)
                    {
                    const wxRect buttonBorderRect = m_folders[i].m_subItems[j].m_Rect;
                    if (GetStyle() == SidebarStyle::Glassy)
                        {
                        DrawGlassEffect(dc, wxRect(buttonBorderRect.GetLeftTop().x,
                            buttonBorderRect.GetLeftTop().y,
                            GetSize().GetWidth() >= GetSubitemIndentation() ?
                                GetSize().GetWidth()-GetSubitemIndentation() : 0,
                            GetItemHeight()), m_selectedColor);
                        }
                    else
                        {
                        wxDCBrushChanger bc(dc, m_selectedColor);
                        wxDCPenChanger pc(dc, m_selectedColor);
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
                        (m_folders[i].m_highlightedItem.has_value() &&
                         m_folders[i].m_highlightedItem.value() == j) ?
                            m_highlightFontColor :
                            (GetSelectedFolder().has_value() && GetSelectedFolder().value() == i &&
                             m_folders[i].m_selectedItem.has_value() &&
                             m_folders[i].m_selectedItem.value() == j) ?
                                m_selectedFontColor : GetForegroundColour());
                    if (IsValidImageId(m_folders[i].m_subItems[j].m_iconIndex))
                        {
                        dc.DrawLabel(m_folders[i].m_subItems[j].m_label,
                                     GetImageList()
                                        [m_folders[i].m_subItems[j].m_iconIndex.value()].
                                        GetBitmap(GetIconSize()),
                                     wxRect(m_folders[i].m_subItems[j].m_Rect).
                                        Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    else
                        {
                        dc.DrawLabel(m_folders[i].m_subItems[j].m_label,
                                     wxNullBitmap,
                                     wxRect(m_folders[i].m_subItems[j].m_Rect).
                                        Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    }
                }
            }
        }
    }

//---------------------------------------------------
void SideBar::OnPaint([[maybe_unused]] wxPaintEvent& event)
    {
    // if mouse is not inside of window, then turn off any item mouse highlighting
    if (!GetScreenRect().Contains(wxGetMousePosition()))
        { ClearHighlightedItems(); }

 #ifdef __WXMSW__
    wxAutoBufferedPaintDC pdc(this);
    pdc.Clear();
    wxGraphicsContext* context{ nullptr };
    auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
    if (renderer)
        { context = renderer->CreateContext(pdc); }

    if (context)
        {
        wxGCDC dc(context);
        PrepareDC(dc);
        OnDraw(dc);
        }
    else
        {
        wxGCDC dc(pdc);
        PrepareDC(dc);
        OnDraw(dc);
        }
#else
    wxAutoBufferedPaintDC pdc(this);
    pdc.Clear();
    wxGCDC dc(pdc);
    PrepareDC(dc);
    OnDraw(dc);
#endif
    }

//-------------------------------------------
void SideBar::OnMouseChange(wxMouseEvent& event)
    {
    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    const auto previouslyHighlightedRect = m_highlightedRect;
    const auto previouslyHighlightedFolder = m_highlightedFolder;
    const auto previouslyHighlightedSubitem = m_folderWithHighlightedSubitem;
    m_previouslyHighlightedItemsIsSelected =
        (GetSelectedFolder() &&
         GetFolder(GetSelectedFolder().value()).m_selectedItem &&
         previouslyHighlightedSubitem.first &&
         previouslyHighlightedSubitem.second &&
         GetSelectedFolder().value() ==
            previouslyHighlightedSubitem.first.value() &&
         GetFolder(GetSelectedFolder().value()).m_selectedItem ==
            previouslyHighlightedSubitem.second.value());
    ClearHighlightedItems();

    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        if (m_folders[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            m_highlightedFolder = i;
            m_highlightedIsSelected = (m_selectedFolder && m_selectedFolder.value() == i);
            m_highlightedRect = m_folders[i].m_Rect;
            m_folders[i].m_highlightedItem = std::nullopt;
            // mouse is over the same folder as before, don't bother repainting
            if (previouslyHighlightedFolder.has_value() &&
                previouslyHighlightedFolder.value() == i)
                { return; }
            break;
            }
        // If a parent item isn't being moused over, then see if its
        // expanded subitems are being moused over.
        // Otherwise, turn off the subitems' highlighting.
        if (m_folders[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_folders[i].m_subItems.size(); ++j)
                {
                if (m_folders[i].m_subItems[j].m_Rect.
                    Contains(event.GetX()+x, event.GetY()+y))
                    {
                    m_folders[i].m_highlightedItem = j;
                    m_highlightedIsSelected =
                        (m_selectedFolder &&
                         m_selectedFolder.value() == i &&
                         m_folders[i].m_selectedItem &&
                         m_folders[i].m_selectedItem.value() == j);
                    m_highlightedRect = m_folders[i].m_subItems[j].m_Rect;
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
                    { m_folders[i].m_highlightedItem = std::nullopt; }
                }
            }
        }

    // if nothing highlighted and nothing was highlighted before, then don't repaint
    if (!previouslyHighlightedRect && !m_highlightedRect)
        { return; }

    // refresh only the affected items, not the whole control
    wxRect refreshRect =
        (previouslyHighlightedRect && !m_previouslyHighlightedItemsIsSelected &&
         m_highlightedRect && !m_highlightedIsSelected) ?
            previouslyHighlightedRect.value().Union(m_highlightedRect.value()) :
        (previouslyHighlightedRect && !m_previouslyHighlightedItemsIsSelected) ?
            previouslyHighlightedRect.value() :
        (m_highlightedRect && !m_highlightedIsSelected) ?
            m_highlightedRect.value() :
        wxRect();

    if (refreshRect.IsEmpty())
        { return; }

    refreshRect.Offset(0, -std::min(y, refreshRect.y));

    Refresh(true, &refreshRect);
    Update();
    }

//-------------------------------------------
void SideBar::Minimize()
    {
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
    SetMinSize(wxSize(0, wxDefaultCoord));
    SetSize(0, wxDefaultCoord);
    }

//-------------------------------------------
void SideBar::Maximize()
    {
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    const auto sideBarMinimumWidth = AdjustWidthToFitItems();
    SetMinSize(wxSize(sideBarMinimumWidth, wxDefaultCoord));
    SetSize(sideBarMinimumWidth, wxDefaultCoord);
    }

//-------------------------------------------
void SideBar::OnMouseLeave([[maybe_unused]] wxMouseEvent& event)
    {
    // If nothing was highlighted, then don't bother repainting since there's
    // nothing to remove highlighting from.
    if (!m_highlightedRect ||
        m_previouslyHighlightedItemsIsSelected ||
        m_highlightedIsSelected)
        { return; }

    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);
    wxRect refreshRect{ m_highlightedRect.value() };
    refreshRect.Offset(0, -std::min(y, refreshRect.y));

    ClearHighlightedItems();

    Refresh(true, &refreshRect);
    Update();
    }

//-------------------------------------------
void SideBar::OnMouseClick(wxMouseEvent& event)
    {
    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    for (size_t i = 0; i < m_folders.size(); ++i)
        {
        if (m_folders[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            SelectFolder(i, true, true, true);
            return;
            }
        // if a parent item isn't being moused over,
        // then see if its expanded subitems are being moused over
        if (m_folders[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_folders[i].m_subItems.size(); ++j)
                {
                if (m_folders[i].m_subItems[j].m_Rect.
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
void SideBar::Realize()
    {
    for (const auto& bmp : GetImageList())
        {
        if (bmp.IsOk() &&
            GetIconSize().GetHeight() + GetPaddingHeight() > GetItemHeight())
            { m_itemHeight = GetIconSize().GetHeight() + GetPaddingHeight(); }
        }
    // measure the items' text heights
    wxMemoryDC dc;
    for (const auto& item : m_folders)
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
    size_t subItemHeight{ 0 };
    size_t currentFolderIndex{ 0 };
    for (auto& folder : m_folders)
        {
        folder.m_Rect = wxRect(0,
            (GetItemHeight() * currentFolderIndex) + subItemHeight,
            GetClientSize().GetWidth(), GetItemHeight());
        // update subitems' areas and factor them into the overall height
        for (auto& subitem : folder.m_subItems)
            {
            if (folder.m_isExpanded)
                {
                subitem.m_Rect = wxRect(0,
                    (GetItemHeight() * (currentFolderIndex + 1)) +
                    subItemHeight,
                    GetClientSize().GetWidth() - GetSubitemIndentation(),
                    GetItemHeight());
                subitem.m_Rect.Offset(GetSubitemIndentation(), 0);
                subItemHeight += GetItemHeight();
                }
            else
                { subitem.m_Rect = wxRect(0, 0, 0, 0); }
            }
        ++currentFolderIndex;
        }
    return (GetItemHeight() * m_folders.size()) + subItemHeight;
    }

//-------------------------------------------
void SideBar::EnsureFolderVisible(const size_t index)
    {
    if (index >= GetFolderCount())
        { return; }

    int x{ 0 }, y{ 0 }, xUnit{ 0 }, yUnit{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);
    GetScrollPixelsPerUnit(&xUnit,&yUnit);
    wxRect scrolledClientRect = GetClientRect();
    scrolledClientRect.Offset(x, y);
    const wxRect lastItemRectToBeVisible =
        (m_folders[index].m_isExpanded && m_folders[index].GetSubItemCount() &&
         m_folders[index].IsSubItemSelected()) ?
        m_folders[index].m_subItems[m_folders[index].m_selectedItem.value()].m_Rect :
            m_folders[index].m_Rect;
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
                           const bool sendEvent /*= true*/,
                           const bool collapseIfExpanded /*= false*/)
    {
    if (item >= GetFolderCount())
        { return; }

    EnsureFolderVisible(item);

    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    wxRect refreshRect{ GetClientRect() };
    refreshRect.SetTop(m_folders[item].m_Rect.GetTop());
    refreshRect.Offset(0, -std::min(y, refreshRect.y));

    // If changing from expanded to collapsed, then just collapse it;
    // nothing is being selected or de-selected, just closing the folder.
    // Because of this, we won't be firing a selection event.
    if (collapseIfExpanded && m_folders[item].m_isExpanded)
        {
        m_folders[item].Collapse();
        RecalcSizes();
        Refresh(true, &refreshRect);
        Update();
        return;
        }
    // See if we should fire a subitem selection event instead
    // (in case this parent has subitems)
    if (m_folders[item].GetSubItemCount())
        {
        if (!m_folders[item].IsSubItemSelected())
            { m_folders[item].m_selectedItem = 0; }
        SelectSubItem(item, m_folders[item].m_selectedItem.value(), setFocus, sendEvent);
        return;
        }

    m_selectedFolder = item;
    m_folders[GetSelectedFolder().value()].Expand();
    RecalcSizes();
    Refresh(true, &refreshRect);
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(EVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_folders[GetSelectedFolder().value()].m_label);
        cevent.SetInt(m_folders[GetSelectedFolder().value()].m_id);
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
        for (size_t j = 0; j < m_folders[i].GetSubItemCount(); ++j)
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
bool SideBar::SelectSubItem(const size_t item, const size_t subItem,
                            const bool setFocus /*= true*/,
                            const bool sendEvent /*= true*/)
    {
    if (item >= GetFolderCount())
        { return false; }
    // if bogus subitem, then just select the parent folder
    if (m_folders[item].GetSubItemCount() == 0 ||
        subItem >= m_folders[item].GetSubItemCount())
        {
        SelectFolder(item, setFocus, sendEvent);
        return true;
        }
    const auto previouslySelectedFolder = m_selectedFolder.value_or(item);
    const auto previouslySelectedSubItem = m_folders[previouslySelectedFolder].m_selectedItem ?
        std::optional<size_t>(m_folders[previouslySelectedFolder].m_selectedItem.value()) :
        std::nullopt;

    m_selectedFolder = item;
    const auto needsExpanding = !m_folders[GetSelectedFolder().value()].m_isExpanded;
    m_folders[GetSelectedFolder().value()].Expand();
    m_folders[GetSelectedFolder().value()].m_selectedItem = subItem;

    EnsureFolderVisible(GetSelectedFolder().value());
    RecalcSizes();

    // get the rects of the selected (prior and current) selected items
    const auto previosuslySelectedRect = previouslySelectedSubItem ?
        m_folders[previouslySelectedFolder].m_subItems[previouslySelectedSubItem.value()].m_Rect :
        m_folders[previouslySelectedFolder].m_Rect;

    const auto currentlySelectedRect =
        m_folders[GetSelectedFolder().value()].m_subItems[subItem].m_Rect;

    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    wxRect refreshRect =
        (needsExpanding ?
         GetClientRect() :
         previosuslySelectedRect.Union(currentlySelectedRect));
    refreshRect.Offset(0, -std::min(y, refreshRect.y));

    Refresh(true, &refreshRect);
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(EVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_folders[GetSelectedFolder().value()].m_subItems[subItem].m_label);
        cevent.SetExtraLong(m_folders[GetSelectedFolder().value()].m_id);
        cevent.SetInt(m_folders[GetSelectedFolder().value()].m_subItems[subItem].m_id);
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }
    return true;
    }
