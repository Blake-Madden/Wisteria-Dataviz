///////////////////////////////////////////////////////////////////////////////
// Name:        sidebarbook.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sidebarbook.h"

using namespace Wisteria::UI;

wxIMPLEMENT_DYNAMIC_CLASS(SideBarBook, wxControl)

wxDEFINE_EVENT(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGING, wxBookCtrlEvent);
wxDEFINE_EVENT(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGED, wxBookCtrlEvent);

//---------------------------------------------------
SideBarBook::SideBarBook(wxWindow *parent, wxWindowID id) :
            wxControl(parent, id, wxDefaultPosition, wxDefaultSize,
                      wxBK_LEFT, wxDefaultValidator, L"SideBarBook")
    {
    m_sidebar = new SideBar(this);
    m_sidebar->SetMinSize(wxSize(100, -1));

    // bind events
    Bind(EVT_SIDEBAR_CLICK, &SideBarBook::OnListSelected, this, wxID_ANY);
    Bind(wxEVT_SIZE, &SideBarBook::OnSize, this);
    }

//---------------------------------------------------
void SideBarBook::UpdateSize()
    {
    GetSideBar()->AdjustWidthToFitItems();
    Layout();
    }

//---------------------------------------------------
void SideBarBook::UpdateSelectedPage(size_t newsel)
    {
    m_selection = static_cast<int>(newsel);
    // block events temporarily. We just want to select an item in the sidebar
    // without actually calling its selection event, which would be redundant.
    const long style = GetSideBar()->GetExtraStyle();
    GetSideBar()->SetExtraStyle(style|wxWS_EX_BLOCK_EVENTS);
    GetSideBar()->SelectAnyItem(newsel);
    GetSideBar()->SetExtraStyle(style);
    }

//---------------------------------------------------
wxBookCtrlEvent* SideBarBook::CreatePageChangingEvent() const
    { return new wxBookCtrlEvent(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGING, m_windowId); }

//---------------------------------------------------
void SideBarBook::MakeChangedEvent(wxBookCtrlEvent &event)
    { event.SetEventType(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGED); }

//---------------------------------------------------
bool SideBarBook::AddPage(wxWindow *page,
                       const wxString& text,
                       const wxWindowID Id,
                       bool bSelect,
                       const int imageId)
    {
    wxASSERT(page);
    const size_t position = GetPageCount();
    if (!DoInsertPage(position, page, text, bSelect, imageId) )
        return false;

    GetSideBar()->InsertItem(GetSideBar()->GetFolderCount(), text, Id, imageId);

    // if the inserted page is before the selected one, we must update the
    // index of the selected page
    if (position <= static_cast<size_t>(m_selection))
    {
        // one extra page added
        ++m_selection;
        GetSideBar()->SelectFolder(m_selection);
    }

    // some page should be selected: either this one or the first one if there
    // is still no selection
    int selNew = -1;
    if ( bSelect )
        selNew = static_cast<int>(position);
    else if ( m_selection == -1 )
        selNew = 0;

    if (page && selNew != m_selection )
        page->Hide();

    if ( selNew != -1 )
        SetSelection(selNew);

    UpdateSize();

    return true;
    }

//---------------------------------------------------
bool SideBarBook::AddSubPage(wxWindow* page, const wxString& text, const wxWindowID Id,
        bool bSelect /*= false*/, int imageId /*= wxNOT_FOUND*/)
    {
    wxASSERT(page);
    if (!DoInsertPage(GetPageCount(), page, text, bSelect, imageId) )
        return false;
    GetSideBar()->InsertSubItemById(GetSideBar()->GetFolder(
        GetSideBar()->GetFolderCount()-1).GetId(), text, Id, imageId);
    if (!bSelect && page)
        { page->Hide(); }
    UpdateSize();

    return true;
    }

//---------------------------------------------------
bool SideBarBook::DeleteAllPages()
    {
    GetSideBar()->DeleteAllFolders();
    m_selection = wxNOT_FOUND;
    DoInvalidateBestSize();
    WX_CLEAR_ARRAY(m_pages);
    UpdateSize();

    return true;
    }

//---------------------------------------------------
// SideBarBook events
//---------------------------------------------------
void SideBarBook::OnListSelected([[maybe_unused]] wxCommandEvent& event)
    {
    const auto selNew = GetSideBar()->GetSelectedAnyItem();
    const int oldSel = GetSelection();
    if (!selNew.has_value() || selNew.value() >= GetPageCount())
        { return; }
    // if previous and new items are the same then don't change anything
    else if (oldSel != wxNOT_FOUND && oldSel < static_cast<int>(GetPageCount()) &&
        (static_cast<size_t>(oldSel) == selNew.value() || m_pages[oldSel] == m_pages[selNew.value()]) )
        { return; }
    else
        { SetSelection(selNew.value()); }
    }

//---------------------------------------------------
bool SideBarBook::DeletePage(size_t nPage)
    {
    wxWindow *page = DoRemovePage(nPage);
    if (!page)
        return false;

    // delete null is harmless
    delete page;

    return true;
    }

//---------------------------------------------------
int SideBarBook::DoSetSelection(size_t n, int flags)
{
    wxCHECK_MSG( n < GetPageCount(), wxNOT_FOUND,
                 L"invalid page index in DoSetSelection()");

    wxWindowUpdateLocker noUpdates(this);

    const int oldSel = GetSelection();

    if ( n != static_cast<size_t>(oldSel) )
    {
        wxBookCtrlEvent *event = CreatePageChangingEvent();
        bool allowed = false;

        if ( flags & SetSelection_SendEvent )
        {
            event->SetSelection(static_cast<int>(n));
            event->SetOldSelection(oldSel);
            event->SetEventObject(this);

            allowed = !GetEventHandler()->ProcessEvent(*event) || event->IsAllowed();
        }

        if ( !(flags & SetSelection_SendEvent) || allowed)
        {
            if ( oldSel != wxNOT_FOUND )
                m_pages[oldSel]->Hide();

            wxWindow *page = m_pages[n];
            page->SetSize(GetPageRect());
            page->Show();

            // change selection now to ignore the selection change event
            UpdateSelectedPage(n);

            if ( flags & SetSelection_SendEvent )
            {
                // program allows the page change
                MakeChangedEvent(*event);
                GetEventHandler()->ProcessEvent(*event);
            }
        }

        delete event;
    }

    return oldSel;
}

//---------------------------------------------------
wxWindow* SideBarBook::DoRemovePage(size_t nPage)
    {
    wxASSERT_MSG(nPage < m_pages.size(),
                 L"invalid page index in SideBarBook::DoRemovePage()");

    wxWindow *pageRemoved = m_pages[nPage];
    m_pages.erase(m_pages.begin() + nPage);
    DoInvalidateBestSize();

    return pageRemoved;
    }

//---------------------------------------------------
wxSize SideBarBook::DoGetBestSize() const
{
    wxSize bestSize;

    // iterate over all pages, get the largest width and height
    const size_t nCount = m_pages.size();
    for ( size_t nPage = 0; nPage < nCount; nPage++ )
    {
        const wxWindow * const pPage = m_pages[nPage];
        if( pPage )
        {
            const wxSize childBestSize(pPage->GetBestSize());

            if ( childBestSize.x > bestSize.x )
                bestSize.x = childBestSize.x;

            if ( childBestSize.y > bestSize.y )
                bestSize.y = childBestSize.y;
        }
    }

    if (m_fitToCurrentPage && GetCurrentPage())
        bestSize = GetCurrentPage()->GetBestSize();

    // convert display area to window area, adding the size necessary for the
    // tabs
    wxSize best = CalcSizeFromPage(bestSize);
    CacheBestSize(best);
    return best;
}

//---------------------------------------------------
wxSize SideBarBook::CalcSizeFromPage(const wxSize& sizePage) const
{
    // we need to add the size of the choice control and the border between
    const wxSize sizeController = GetControllerSize();

    wxSize size = sizePage;
    if ( IsVertical() )
    {
        if ( sizeController.x > sizePage.x )
            size.x = sizeController.x;
        size.y += sizeController.y + GetInternalBorder();
    }
    else // left/right aligned
    {
        size.x += sizeController.x + GetInternalBorder();
        if ( sizeController.y > sizePage.y )
            size.y = sizeController.y;
    }

    return size;
}

//---------------------------------------------------
wxSize SideBarBook::GetControllerSize() const
{
    if ( !GetSideBar() || !GetSideBar()->IsShown() )
        return wxSize(0, 0);

    const wxSize sizeClient = GetClientSize(),
                 sizeCtrl = GetSideBar()->GetBestSize();

    wxSize size;

    if ( IsVertical() )
    {
        size.x = sizeClient.x;
        size.y = sizeCtrl.y;
    }
    else // left/right aligned
    {
        size.x = sizeCtrl.x;
        size.y = sizeClient.y;
    }

    return size;
}

//---------------------------------------------------
wxRect SideBarBook::GetPageRect() const
{
    const wxSize size = GetControllerSize();

    wxRect rectPage(wxPoint(), GetClientSize());

    switch ( GetWindowStyle() & wxBK_ALIGN_MASK )
        {
        case wxBK_TOP:
            rectPage.y = size.y + GetInternalBorder();
            [[fallthrough]];
        case wxBK_BOTTOM:
            rectPage.height -= size.y + GetInternalBorder();
            if (rectPage.height < 0)
                rectPage.height = 0;
            break;
        case wxBK_LEFT:
            rectPage.x = size.x + GetInternalBorder();
            [[fallthrough]];
        case wxBK_RIGHT:
            rectPage.width -= size.x + GetInternalBorder();
            if (rectPage.width < 0)
                rectPage.width = 0;
            break;
        default:
            wxFAIL_MSG(L"unexpected alignment");
        }

    return rectPage;
}

//---------------------------------------------------
void SideBarBook::OnSize(wxSizeEvent& event)
{
    event.Skip();

    DoSize();
}

//---------------------------------------------------
void SideBarBook::DoSize()
{
    if ( !GetSideBar() )
    {
        // we're not fully created yet or OnSize() should be hidden by derived class
        return;
    }

    if (GetSizer())
        Layout();
    else
    {
        // resize controller and the page area to fit inside our new size
        const wxSize sizeClient( GetClientSize() ),
                    sizeBorder( GetSideBar()->GetSize() - GetSideBar()->GetClientSize() ),
                    sizeCtrl( GetControllerSize() );

        GetSideBar()->SetClientSize( sizeCtrl.x - sizeBorder.x, sizeCtrl.y - sizeBorder.y );
        // if this changes the visibility of the scrollbars the best size changes, re-layout in this case
        const wxSize sizeCtrl2 = GetControllerSize();
        if ( sizeCtrl != sizeCtrl2 )
        {
            const wxSize sizeBorder2 = GetSideBar()->GetSize() - GetSideBar()->GetClientSize();
            GetSideBar()->SetClientSize( sizeCtrl2.x - sizeBorder2.x, sizeCtrl2.y - sizeBorder2.y );
        }

        const wxSize sizeNew = GetSideBar()->GetSize();
        wxPoint posCtrl;
        switch ( GetWindowStyle() & wxBK_ALIGN_MASK )
        {
            case wxBK_TOP:
                [[fallthrough]];
            case wxBK_LEFT:
                // posCtrl is already ok
                break;
            case wxBK_BOTTOM:
                posCtrl.y = sizeClient.y - sizeNew.y;
                break;
            case wxBK_RIGHT:
                posCtrl.x = sizeClient.x - sizeNew.x;
                break;
            default:
                wxFAIL_MSG(L"unexpected alignment");
        }

        if ( GetSideBar()->GetPosition() != posCtrl )
            GetSideBar()->Move(posCtrl);
    }

    // resize all pages to fit the new control size
    const wxRect pageRect = GetPageRect();
    for (unsigned int i = 0; i < m_pages.size(); ++i)
    {
        wxWindow * const page = m_pages[i];
        if ( !page )
        {
            wxASSERT_MSG(false,
                L"Null page in a control that does not allow null pages?");
            continue;
        }

        page->SetSize(pageRect);
    }
}

//---------------------------------------------------
bool SideBarBook::DoInsertPage(size_t nPage,
                           wxWindow *page,
                           [[maybe_unused]] const wxString& text,
                           [[maybe_unused]] bool bSelect,
                           [[maybe_unused]] int imageId)
{
    wxCHECK_MSG( page, false,
                 L"NULL page in SideBarBook::DoInsertPage()");
    wxCHECK_MSG( nPage <= m_pages.size(), false,
                 L"invalid page index in SideBarBook::DoInsertPage()");

    m_pages.insert(m_pages.begin() + nPage, page);
    if ( page )
        page->SetSize(GetPageRect());

    DoInvalidateBestSize();

    return true;
}

//---------------------------------------------------
void SideBarBook::DoInvalidateBestSize()
{
    // notice that it is not necessary to invalidate our own best size
    // explicitly if we have sidebar as it will already invalidate the best
    // size of its parent when its own size is invalidated and its parent is
    // this control
    if ( GetSideBar() )
        GetSideBar()->InvalidateBestSize();
    else
        wxControl::InvalidateBestSize();
}
