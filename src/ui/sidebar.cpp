///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sidebar.h"

DEFINE_EVENT_TYPE(wxEVT_SIDEBAR_CLICK)
DEFINE_EVENT_TYPE(wxEVT_SIDEBAR_SHOWHIDE_CLICK)

wxBEGIN_EVENT_TABLE(wxSideBar, wxScrolledCanvas)
    EVT_PAINT(wxSideBar::OnPaint)
    EVT_MOTION(wxSideBar::OnMouseChange)
    EVT_LEAVE_WINDOW(wxSideBar::OnMouseLeave)
    EVT_LEFT_DOWN(wxSideBar::OnMouseClick)
    EVT_KEY_DOWN(wxSideBar::OnChar)
    EVT_LEFT_DCLICK(wxSideBar::OnDblClick)
    EVT_SIZE(wxSideBar::OnResize)
wxEND_EVENT_TABLE()

//---------------------------------------------------
wxSideBar::wxSideBar(wxWindow* parent, wxWindowID id /*= wxID_ANY*/) 
        : wxScrolledCanvas(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS|wxVSCROLL|wxBORDER_THEME|wxFULL_REPAINT_ON_RESIZE),
          m_highlightedItem(static_cast<size_t>(wxNOT_FOUND)), m_activeItem(static_cast<size_t>(wxNOT_FOUND)),
          m_savedActiveItem(static_cast<size_t>(wxNOT_FOUND)),
          m_effect(VisualEffect::Flat),
          m_activeColor(wxT("#FDB759")),
          m_activeFontColor(*wxBLACK),
          m_highlightColor(wxColour(253, 211, 155)),
          m_highlightFontColor(*wxBLACK),
          m_parentColor(wxColour(180, 189, 207)),
          m_includeShowHideToolbar(false),
          m_isExpanded(true)
    {
    // Start off with enough height for a usual icon and some padding around it.
    // This will be adjusted in Realize() to take into account the actual height of the
    // text and any loaded icons.
    m_itemHeight = FromDIP(wxSize(16,16)).GetHeight()+GetPaddingHeight();
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(wxColour(200, 211, 231));
    SetScrollbars(FromDIP(wxSize(30,30)).GetWidth(), FromDIP(wxSize(30,30)).GetHeight(), 0, 0);
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    SetVirtualSize(GetSize().GetWidth(), GetSize().GetHeight());
    SetMinSize(wxSize(GetPaddingWidth(), m_itemHeight));
    }

//---------------------------------------------------
void wxSideBar::DrawGlassEffect(wxDC& dc, const wxRect rect, const wxColour color)
    {
    // fill with the color
    dc.GradientFillLinear(rect, color, color.ChangeLightness(140),
        wxSOUTH);
    // create a shiny overlay
    dc.GradientFillLinear(wxRect(rect.GetX(), rect.GetY(),
        rect.GetWidth(),
        rect.GetHeight()*.25),
        color.ChangeLightness(115), color.ChangeLightness(155),
        wxSOUTH);
    }

//---------------------------------------------------
size_t wxSideBar::GetSelectedAnyItem() const
    {
    if (GetSelectedCategory() == wxNOT_FOUND)
        { return static_cast<size_t>(wxNOT_FOUND); }
    size_t selectedSubItem = 0;
    for (size_t i = 0; i < GetSelectedCategory(); ++i)
        {
        ++selectedSubItem;
        selectedSubItem += m_items[i].GetSubItemCount();
        }
    ++selectedSubItem;
    selectedSubItem += m_items[GetSelectedCategory()].m_activeItem;
    return selectedSubItem;
    }

//---------------------------------------------------
size_t wxSideBar::AdjustWidthToFitItems()
    {
    if (!IsExpanded())
        { return GetHideWidth(); }
    size_t sideBarMinimumWidth = FromDIP(wxSize(wxSizerFlags::GetDefaultBorder()*10,
                                         wxSizerFlags::GetDefaultBorder()*10)).GetWidth();
    for (size_t i = 0; i < GetCategoryCount(); ++i)
        { sideBarMinimumWidth = std::max(GetCategoryWidth(i), sideBarMinimumWidth); }
    SetMinSize(wxSize(sideBarMinimumWidth, -1));
    return sideBarMinimumWidth;
    }

//---------------------------------------------------
void wxSideBar::OnResize(wxSizeEvent& event)
    {
    CalculateSizes();
    event.Skip();
    }

//---------------------------------------------------
void wxSideBar::CollapseAll()
    {
    for (std::vector<SideBarItem>::iterator pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        { pos->Collapse(); }
    CalculateSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void wxSideBar::ExpandAll()
    {
    for (std::vector<SideBarItem>::iterator pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        { pos->Expand(); }
    CalculateSizes();
    Refresh();
    Update();
    }

//---------------------------------------------------
void wxSideBar::SaveState()
    {
    m_savedActiveItem = m_activeItem;
    m_stateInfo.clear();
    for (std::vector<SideBarItem>::const_iterator pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        { m_stateInfo.insert(SideBarStateInfo(*pos)); }
    }

//---------------------------------------------------
void wxSideBar::ResetState()
    {
    m_activeItem = m_savedActiveItem;
    for (std::vector<SideBarItem>::iterator pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        {
        std::set<SideBarStateInfo>::const_iterator statePos = m_stateInfo.find(SideBarStateInfo(*pos));
        if (statePos != m_stateInfo.end())
            {
            pos->m_activeItem = statePos->m_activeItem;
            if ((pos->m_subItems.size() && pos->m_activeItem == static_cast<size_t>(wxNOT_FOUND)) ||
                pos->m_activeItem >= pos->m_subItems.size())
                { pos->m_activeItem = 0; }
            pos->m_isExpanded = statePos->m_isExpanded;
            }
        }
    CalculateSizes();
    }

//-------------------------------------------
void wxSideBar::OnChar(wxKeyEvent& event)
    {
    ClearHighlightedItems();
    //if going down, select next item. If it has subitems, then select the first subitem
    if (event.GetKeyCode() == WXK_DOWN)
        {
        if (IsCategorySelected() && m_items[GetSelectedCategory()].GetSubItemCount() &&
            m_items[GetSelectedCategory()].m_activeItem < m_items[GetSelectedCategory()].GetSubItemCount()-1)
            { SelectSubItem(GetSelectedCategory(), m_items[GetSelectedCategory()].m_activeItem+1); }
        else if (IsCategorySelected() && GetSelectedCategory() < GetCategoryCount()-1)
            { SelectSubItem(GetSelectedCategory()+1, 0); }
        }
    //if going up, select the previous item. If it has subitems, then select the last subitem
    else if (event.GetKeyCode() == WXK_UP)
        {
        if (IsCategorySelected() && m_items[GetSelectedCategory()].GetSubItemCount() &&
            m_items[GetSelectedCategory()].m_activeItem > 0)
            { SelectSubItem(GetSelectedCategory(), m_items[GetSelectedCategory()].m_activeItem-1); }
        else if (IsCategorySelected() && GetSelectedCategory() > 0)
            {
            SelectSubItem(GetSelectedCategory()-1,
                m_items[GetSelectedCategory()-1].GetSubItemCount() ? m_items[GetSelectedCategory()-1].GetSubItemCount()-1 : 0);
            }
        }
    //going right will expanded the selected item
    else if (event.GetKeyCode() == WXK_RIGHT)
        {
        if (IsCategorySelected() && !m_items[GetSelectedCategory()].m_isExpanded)
            {
            m_items[GetSelectedCategory()].Expand();
            CalculateSizes();
            Refresh();
            }
        }
    //going left will collapse the selected item
    else if (event.GetKeyCode() == WXK_LEFT)
        {
        if (IsCategorySelected() && m_items[GetSelectedCategory()].m_isExpanded)
            {
            m_items[GetSelectedCategory()].Collapse();
            CalculateSizes();
            Refresh();
            }
        }
    else
        { event.Skip(); }
    }

//---------------------------------------------------
std::pair<wxWindowID,wxWindowID> wxSideBar::GetSelectedSubItemId() const
    {
    if (!IsCategorySelected() || m_items[GetSelectedCategory()].GetSubItemCount() == 0 ||
        !m_items[GetSelectedCategory()].IsSubItemSelected())
        { return std::pair<wxWindowID,wxWindowID>(wxNOT_FOUND,wxNOT_FOUND); }
    else
        {
        return std::pair<wxWindowID,wxWindowID>(m_items[GetSelectedCategory()].m_id,
            m_items[GetSelectedCategory()].m_subItems[m_items[GetSelectedCategory()].m_activeItem].m_id);
        }
    }

//---------------------------------------------------
wxString wxSideBar::GetSelectedLabel() const
    {
    if (!IsCategorySelected())
        { return wxEmptyString; }
    else if (m_items[GetSelectedCategory()].GetSubItemCount() == 0)
        { return GetCategoryText(GetSelectedCategory()); }
    else if (m_items[GetSelectedCategory()].IsSubItemSelected())
        { return m_items[GetSelectedCategory()].m_subItems[m_items[GetSelectedCategory()].m_activeItem].m_Label; }
    else
        { return wxEmptyString; }
    }

//---------------------------------------------------
long wxSideBar::FindCategory(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_id == Id)
            { return i; }
        }
    return wxNOT_FOUND;
    }

//---------------------------------------------------
std::pair<long,long> wxSideBar::FindSubItem(const wxWindowID Id) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
            {
            if (m_items[i].m_subItems[j].m_id == Id)
                { return std::pair<int,int>(i,j); }
            }
        }
    return std::pair<long,long>(wxNOT_FOUND,wxNOT_FOUND);
    }

//---------------------------------------------------
std::pair<long,long> wxSideBar::FindSubItem(const wxString& label) const
    {
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
            {
            if (m_items[i].m_subItems[j].m_Label.CmpNoCase(label) == 0)
                { return std::pair<int,int>(i,j); }
            }
        }
    return std::pair<long,long>(wxNOT_FOUND,wxNOT_FOUND);
    }

//---------------------------------------------------
std::pair<long,long> wxSideBar::FindSubItem(const wxWindowID parentId,
                                            const wxWindowID subItemId) const
    {
    const size_t parentIndex = FindCategory(parentId);
    if (parentIndex == wxNOT_FOUND)
        { return std::pair<int,int>(wxNOT_FOUND,wxNOT_FOUND); }
    for (size_t j = 0; j < m_items[parentIndex].m_subItems.size(); ++j)
        {
        if (m_items[parentIndex].m_subItems[j].m_id == subItemId)
            { return std::pair<int,int>(parentIndex,j); }
        }
    return std::pair<long,long>(wxNOT_FOUND,wxNOT_FOUND);
    }

//---------------------------------------------------
std::pair<long,long> wxSideBar::FindSubItem(const wxWindowID parentId,
                                            const SideBarSubItem& subItem) const
    {
    const size_t parentIndex = FindCategory(parentId);
    if (parentIndex == wxNOT_FOUND)
        { return std::pair<int,int>(wxNOT_FOUND,wxNOT_FOUND); }
    for (size_t j = 0; j < m_items[parentIndex].m_subItems.size(); ++j)
        {
        if (m_items[parentIndex].m_subItems[j].m_id == subItem.m_id &&
            (m_items[parentIndex].m_subItems[j].m_iconIndex == subItem.m_iconIndex ||
             m_items[parentIndex].m_subItems[j].m_Label == subItem.m_Label))
            { return std::pair<int,int>(parentIndex,j); }
        }
    return std::pair<long,long>(wxNOT_FOUND,wxNOT_FOUND);
    }

//---------------------------------------------------
size_t wxSideBar::GetCategoryWidth(const size_t item)
    {
    wxASSERT_LEVEL_2(item < GetCategoryCount());
    if (item >= GetCategoryCount())
        { return 0; }
    wxClientDC dc(this);
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxCoord textWidth(0), textHeight(0);
    dc.GetTextExtent(m_items[item].m_Label, &textWidth, &textHeight);
    wxCoord parentWidth = textWidth;
    parentWidth += IsValidImageId(m_items[item].m_IconIndex) ?
        GetImageList()[m_items[item].m_IconIndex].GetSize().GetWidth() +
        wxSizerFlags::GetDefaultBorder() : 0;
    parentWidth += GetPaddingWidth(); // padding around the label
    // see what the widest subitem is
    wxCoord widestSubItem = 0;
    for (const auto& subItem : m_items[item].m_subItems)
        {
        dc.GetTextExtent(subItem.m_Label, &textWidth, &textHeight);
        textWidth += IsValidImageId(subItem.m_iconIndex) ?
            GetImageList()[subItem.m_iconIndex].GetSize().GetWidth() +
            wxSizerFlags::GetDefaultBorder() : 0;
        textWidth += GetPaddingWidth() + GetSubitemIndentation(); // padding around the label
        widestSubItem = std::max(textWidth,widestSubItem);
        }
    return std::max(parentWidth+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X),
                    widestSubItem+wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
    }

//---------------------------------------------------
void wxSideBar::InsertItem(const size_t position, const wxString& label,
                           const wxWindowID Id, const long iconIndex)
    {
    SideBarItem item;
    item.m_id = Id;
    item.m_IconIndex = iconIndex;
    item.m_Label = label;
    if (position > m_items.size())
        { m_items.resize(position); }
    m_items.insert(m_items.begin()+position, item);
    }

//---------------------------------------------------
bool wxSideBar::InsertSubItemById(const wxWindowID parentItemId, const wxString& label,
                                  const wxWindowID Id, const long iconIndex)
    {
    SideBarSubItem subitem;
    subitem.m_id = Id;
    subitem.m_iconIndex = iconIndex;
    subitem.m_Label = label;

    const int parent = FindCategory(parentItemId);
    if (parent == wxNOT_FOUND)
        { return false; }
    m_items[parent].m_subItems.push_back(subitem);
    if (m_items[parent].m_highlightedItem == static_cast<size_t>(wxNOT_FOUND))
        { m_items[parent].m_highlightedItem = 0; }
    if (m_items[parent].m_activeItem == static_cast<size_t>(wxNOT_FOUND))
        { m_items[parent].m_activeItem = 0; }
    return true;
    }

//---------------------------------------------------
void wxSideBar::OnDraw(wxDC& dc)
    {
    if (GetCategoryCount() == 0)
        { return; }

    wxDCFontChanger fc(dc, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxDCTextColourChanger tc(dc, GetForegroundColour());

    if (HasShowHideToolbar())
        {
        if (GetVisualEffect() == VisualEffect::Glass)
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
        dc.DrawBitmap(wxArtProvider::GetBitmap(IsExpanded() ? wxART_GO_BACK : wxART_GO_FORWARD, wxART_BUTTON, FromDIP(wxSize(16,16))),
            IsExpanded() ?
                GetClientSize().GetWidth()-(FromDIP(wxSize(16,16)).GetWidth()+wxSizerFlags::GetDefaultBorder()) :
                (GetClientSize().GetWidth()/2)-(FromDIP(wxSize(16,16)).GetWidth()/2),
            (GetToolbarHeight()/2)-(FromDIP(wxSize(16,16)).GetHeight()/2));
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
        if (GetSelectedCategory() == i && !m_items[i].m_isExpanded)
            {
            if (GetVisualEffect() == VisualEffect::Glass)
                {
                DrawGlassEffect(dc,
                    wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                        GetSize().GetWidth(), GetItemHeight()),
                    m_activeColor); }
            else
                {
                wxDCBrushChanger bc(dc, m_activeColor);
                wxDCPenChanger pc(dc, m_activeColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                                 GetSize().GetWidth(), GetItemHeight()));
                }
            }
        else if (m_highlightedItem == i)
            {
            if (GetVisualEffect() == VisualEffect::Glass)
                {
                DrawGlassEffect(dc,
                    wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                           GetSize().GetWidth(), GetItemHeight()),
                    m_highlightColor);
                }
            else
                {
                wxDCBrushChanger bc(dc, m_highlightColor);
                wxDCPenChanger pc(dc, m_highlightColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                                        GetSize().GetWidth(), GetItemHeight()));
                }
            }
        else
            {
            if (GetVisualEffect() == VisualEffect::Glass)
                {
                DrawGlassEffect(dc, wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                    GetSize().GetWidth(), GetItemHeight()), m_parentColor);
                }
            else
                {
                wxDCBrushChanger bc(dc, m_parentColor);
                wxDCPenChanger pc(dc, m_parentColor);
                dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y, GetSize().GetWidth(), GetItemHeight()));
                }
            }
        }

    // draw the parent labels and subitems
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        // draw parent labels
            {
            wxDCTextColourChanger tcc(dc, (m_highlightedItem == i) ?
                m_highlightFontColor :
                (GetSelectedCategory() == i && !m_items[i].m_isExpanded) ?
                    m_activeFontColor :
                    GetForegroundColour());
            if (IsValidImageId(m_items[i].m_IconIndex))
                {
                dc.DrawLabel(m_items[i].m_Label,
                             GetImageList()[m_items[i].m_IconIndex],
                             wxRect(m_items[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            else
                {
                dc.DrawLabel(m_items[i].m_Label, wxNullBitmap,
                             wxRect(m_items[i].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()), wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                }
            }
        // draw subitems (if parent item is expanded)
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_highlightedItem == j)
                    {
                    const wxRect buttonBorderRect = m_items[i].m_subItems[j].m_Rect;
                    if (GetVisualEffect() == VisualEffect::Glass)
                        {
                        DrawGlassEffect(dc,
                            wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                                   GetSize().GetWidth() >= GetSubitemIndentation() ? GetSize().GetWidth()-GetSubitemIndentation() : 0, GetItemHeight()),
                            m_highlightColor);
                        }
                    else
                        {
                        wxDCBrushChanger bc(dc, m_highlightColor);
                        wxDCPenChanger pc(dc, m_highlightColor);
                        dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                                                GetSize().GetWidth() >= GetSubitemIndentation() ? GetSize().GetWidth()-GetSubitemIndentation() : 0, GetItemHeight()));
                        }
                    }
                if (GetSelectedCategory() == i &&
                    m_items[i].m_activeItem == j)
                    {
                    const wxRect buttonBorderRect = m_items[i].m_subItems[j].m_Rect;
                    if (GetVisualEffect() == VisualEffect::Glass)
                        {
                        DrawGlassEffect(dc, wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y,
                            GetSize().GetWidth() >= GetSubitemIndentation() ? GetSize().GetWidth()-GetSubitemIndentation() : 0,
                            GetItemHeight()), m_activeColor);
                        }
                    else
                        {
                        wxDCBrushChanger bc(dc, m_activeColor);
                        wxDCPenChanger pc(dc, m_activeColor);
                        dc.DrawRectangle(wxRect(buttonBorderRect.GetLeftTop().x, buttonBorderRect.GetLeftTop().y, GetSize().GetWidth() >= GetSubitemIndentation() ? GetSize().GetWidth()-GetSubitemIndentation() : 0, GetItemHeight()));
                        }
                    }
                // draw the subitem labels
                    {
                    wxDCTextColourChanger tcc(dc, (m_items[i].m_highlightedItem == j) ?
                        m_highlightFontColor :
                        (GetSelectedCategory() == i && m_items[i].m_activeItem == j) ?
                            m_activeFontColor : GetForegroundColour());
                    if (IsValidImageId(m_items[i].m_subItems[j].m_iconIndex))
                        {
                        dc.DrawLabel(m_items[i].m_subItems[j].m_Label,
                                     GetImageList()[m_items[i].m_subItems[j].m_iconIndex],
                                     wxRect(m_items[i].m_subItems[j].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    else
                        {
                        dc.DrawLabel(m_items[i].m_subItems[j].m_Label,
                                     wxNullBitmap, wxRect(m_items[i].m_subItems[j].m_Rect).Deflate(wxSizerFlags::GetDefaultBorder()),
                                     wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
                        }
                    }
                }
            }
        }
    }

//---------------------------------------------------
void wxSideBar::ClearHighlightedItems() noexcept
    {
    m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
    for (auto& item : m_items)
        { item.m_highlightedItem = static_cast<size_t>(wxNOT_FOUND); }
    }

//---------------------------------------------------
void wxSideBar::OnPaint([[maybe_unused]] wxPaintEvent& event)
    {
    //if mouse is not inside of window, then turn off any item mouse highlighting
    if (!GetScreenRect().Contains(wxGetMousePosition()))
        { ClearHighlightedItems(); }

    wxAutoBufferedPaintDC pdc(this);
    pdc.Clear();
    wxGCDC dc(pdc);
    PrepareDC(dc);
    OnDraw(dc);
    }

//-------------------------------------------
void wxSideBar::OnMouseChange(wxMouseEvent& event)
    {
    if (HasShowHideToolbar())
        {
        int x,y;
        CalcUnscrolledPosition(0,0,&x,&y);
        if (m_toolbarRect.Contains(event.GetX()+x, event.GetY()+y))
            {
            SetToolTip(IsExpanded() ?
                _("Click to hide sidebar") :
                _("Click to show sidebar"));
            }
        else
            { SetToolTip(wxEmptyString); }
        //if not shown, then don't bother handling hover events for items that aren't being displayed
        if (!IsExpanded())
            { return; }
        }

    int x,y;
    CalcUnscrolledPosition(0,0,&x,&y);

    m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            m_highlightedItem = i;
            m_items[i].m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
            break;
            }
        //if a parent item isn't being moused over, then see if its expanded subitems are being moused over.
        //otherwise, turn off the subitems' highlighting.
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_subItems[j].m_Rect.Contains(event.GetX()+x, event.GetY()+y))
                    {
                    m_items[i].m_highlightedItem = j;
                    break;
                    }
                else
                    { m_items[i].m_highlightedItem = static_cast<size_t>(wxNOT_FOUND); }
                }
            }
        }

    Refresh();
    Update();
    }

//-------------------------------------------
void wxSideBar::OnMouseLeave([[maybe_unused]] wxMouseEvent& event)
    {
    //if not shown, then don't bother handling hover events for items that aren't being displayed
    if (!IsExpanded())
        { return; }
    m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
    for (size_t i = 0; i < m_items.size(); ++i)
        { m_items[i].m_highlightedItem = static_cast<size_t>(wxNOT_FOUND); }
    Refresh();
    Update();
    }

//-------------------------------------------
void wxSideBar::Minimize()
    {
    m_isExpanded = false;
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
    SetMinSize(wxSize(GetHideWidth(), wxDefaultCoord));
    SetSize(GetHideWidth(), wxDefaultCoord);

    wxCommandEvent cevent(wxEVT_SIDEBAR_SHOWHIDE_CLICK, GetId());
    cevent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(cevent);
    }

//-------------------------------------------
void wxSideBar::Maximize()
    {
    m_isExpanded = true;
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    const size_t sideBarMinimumWidth = AdjustWidthToFitItems();
    SetMinSize(wxSize(sideBarMinimumWidth, wxDefaultCoord));
    SetSize(sideBarMinimumWidth, wxDefaultCoord);

    wxCommandEvent cevent(wxEVT_SIDEBAR_SHOWHIDE_CLICK, GetId());
    cevent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(cevent);
    }

//-------------------------------------------
void wxSideBar::OnMouseClick(wxMouseEvent& event)
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
            SelectCategory(i);
            return;
            }
        // if a parent item isn't being moused over,
        // then see if it expanded subitems being moused over
        if (m_items[i].m_isExpanded)
            {
            for (size_t j = 0; j < m_items[i].m_subItems.size(); ++j)
                {
                if (m_items[i].m_subItems[j].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
                    {
                    SelectSubItem(i,j);
                    return;
                    }
                }
            }
        }
    }

//-------------------------------------------
void wxSideBar::OnDblClick(wxMouseEvent& event)
    {
    int x{ 0 }, y{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);

    for (size_t i = 0; i < m_items.size(); ++i)
        {
        if (m_items[i].m_Rect.Contains(event.GetX()+x, event.GetY()+y) )
            {
            SelectCategory(i);
            m_items[i].m_isExpanded = !m_items[i].m_isExpanded; // flip collapsed state
            CalculateSizes();
            Refresh();
            Update();
            return;
            }
        }
    }

//-------------------------------------------
void wxSideBar::Realize()
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
        const auto sz = dc.GetMultiLineTextExtent(item.m_Label);
        if (sz.GetHeight()+GetPaddingHeight() > GetItemHeight())
            { m_itemHeight = sz.GetHeight()+GetPaddingHeight(); }
        // measure the item's subitems text heights
        for (const auto& subitem : item.m_subItems)
            {
            const auto subSz = dc.GetMultiLineTextExtent(subitem.m_Label);
            if (subSz.GetHeight()+GetPaddingHeight() > GetItemHeight())
                { m_itemHeight = subSz.GetHeight()+GetPaddingHeight(); }
            }
        }
    }

//-------------------------------------------
void wxSideBar::CalculateSizes()
    {
    // adjust (or show/hide) scrollbars and update the items' positions
    SetVirtualSize(GetSize().GetWidth(), CalculateItemRects());
    }

//-------------------------------------------
size_t wxSideBar::CalculateItemRects()
    {
    if (HasShowHideToolbar())
        { m_toolbarRect = wxRect(0, 0, GetSize().GetWidth(), GetToolbarHeight()); }
    long subItemHeight = 0;
    for (std::vector<SideBarItem>::iterator pos = m_items.begin();
        pos != m_items.end();
        ++pos)
        {
        pos->m_Rect = wxRect(0, (GetItemHeight()*(pos-m_items.begin()))+subItemHeight+GetToolbarHeight(), 
                             GetClientSize().GetWidth(), GetItemHeight());
        // if an expanded item, then factor in its subitems' heights for the rest of the items
        if (pos->m_isExpanded)
            {
            for (size_t j = 0; j < pos->m_subItems.size(); ++j)
                {
                pos->m_subItems[j].m_Rect = wxRect(0, (GetItemHeight()*(pos-m_items.begin()+1))+subItemHeight+GetToolbarHeight(), GetClientSize().GetWidth()-GetSubitemIndentation(), GetItemHeight());
                pos->m_subItems[j].m_Rect.Offset(GetSubitemIndentation(),0);
                subItemHeight += GetItemHeight();
                }
            }
        }
    return (GetItemHeight() * m_items.size()) + subItemHeight;
    }

//-------------------------------------------
void wxSideBar::EnsureCategoryVisible(const size_t category)
    {
    if (category == wxNOT_FOUND || category >= GetCategoryCount() || !IsExpanded())
        { return; }

    int x{ 0 }, y{ 0 }, xUnit{ 0 }, yUnit{ 0 };
    CalcUnscrolledPosition(0, 0, &x, &y);
    GetScrollPixelsPerUnit(&xUnit,&yUnit);
    wxRect scrolledClientRect = GetClientRect();
    scrolledClientRect.Offset(x, y);
    const wxRect lastItemRectToBeVisible =
        (m_items[category].m_isExpanded && m_items[category].GetSubItemCount() &&
         m_items[category].IsSubItemSelected()) ?
        m_items[category].m_subItems[m_items[category].m_activeItem].m_Rect :
            m_items[category].m_Rect;
    if (!scrolledClientRect.Contains(lastItemRectToBeVisible) && yUnit != 0)
        {
        // scroll to the middle of the item
        // (safe compromise when we could be scrolling up or down)
        Scroll(wxDefaultCoord, (lastItemRectToBeVisible.GetY() +
               (lastItemRectToBeVisible.GetHeight()/2))/yUnit);
        }
    }

//-------------------------------------------
void wxSideBar::SelectCategory(const size_t item, const bool setFocus /*= true*/,
                               const bool sendEvent /*= true*/)
    {
    if (item == wxNOT_FOUND || item >= GetCategoryCount())
        { return; }
    // first, see if we should fire a subitem selection event instead
    // (in case this parent has subitems)
    if (m_items[item].GetSubItemCount())
        {
        if (!m_items[item].IsSubItemSelected())
            { m_items[item].m_activeItem = 0; }
        SelectSubItem(item, m_items[item].m_activeItem, setFocus, sendEvent);
        return;
        }

    m_activeItem = item;
    m_items[GetSelectedCategory()].Expand();

    CalculateSizes();
    EnsureCategoryVisible(GetSelectedCategory());
    Refresh();
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(wxEVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_items[GetSelectedCategory()].m_Label);
        cevent.SetInt(m_items[GetSelectedCategory()].m_id);
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }
    }

//-------------------------------------------
void wxSideBar::SelectAnyItem(const size_t item, const bool setFocus /*= true*/,
                              const bool sendEvent /*= true*/)
    {
    size_t itemToSelect = 0;
    for (size_t i = 0; i < GetCategoryCount(); ++i)
        {
        if (itemToSelect == item)
            {
            SelectCategory(i,setFocus, sendEvent);
            return;
            }
        for (size_t j = 0; j < m_items[i].GetSubItemCount(); ++j)
            {
            ++itemToSelect;
            if (itemToSelect == item)
                {
                SelectSubItem(i,j,setFocus, sendEvent);
                return;
                }
            }
        ++itemToSelect;
        }
    }

//-------------------------------------------
void wxSideBar::SelectSubItem(const size_t item, const size_t subItem,
                              const bool setFocus /*= true*/,
                              const bool sendEvent /*= true*/)
    {
    if (item == wxNOT_FOUND || item >= GetCategoryCount() )
        { return; }
    //if bogus subitem, then just select the parent item
    if (subItem == wxNOT_FOUND || m_items[item].GetSubItemCount() == 0 ||
        subItem >= m_items[item].GetSubItemCount())
        {
        SelectCategory(item, setFocus, sendEvent);
        return;
        }
    m_activeItem = item;
    m_items[GetSelectedCategory()].Expand();
    m_items[GetSelectedCategory()].m_activeItem = subItem;

    CalculateSizes();
    EnsureCategoryVisible(GetSelectedCategory());
    Refresh();
    Update();
    if (setFocus)
        { SetFocus(); }

    if (sendEvent)
        {
        wxCommandEvent cevent(wxEVT_SIDEBAR_CLICK, GetId());
        cevent.SetString(m_items[GetSelectedCategory()].m_subItems[subItem].m_Label);
        cevent.SetExtraLong(m_items[GetSelectedCategory()].m_id);
        cevent.SetInt(m_items[GetSelectedCategory()].m_subItems[subItem].m_id);
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }
    }
