///////////////////////////////////////////////////////////////////////////////
// Name:        sidebarbook.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "sidebarbook.h"
#include <algorithm>
#include <utility>
#include <wx/wupdlock.h>

namespace Wisteria::UI
    {
    wxIMPLEMENT_DYNAMIC_CLASS(SideBarBook, wxControl)

        wxDEFINE_EVENT(wxEVT_SIDEBARBOOK_PAGE_CHANGING, wxBookCtrlEvent);
    wxDEFINE_EVENT(wxEVT_SIDEBARBOOK_PAGE_CHANGED, wxBookCtrlEvent);

    //---------------------------------------------------
    SideBarBook::SideBarBook(wxWindow* parent, wxWindowID id)
        : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBK_LEFT, wxDefaultValidator,
                    L"SideBarBook")
        {
        // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
        m_sidebar = new SideBar(this);
        m_sidebar->SetMinSize(wxSize(100, -1));

        // bind events
        Bind(wxEVT_SIDEBAR_CLICK, &SideBarBook::OnListSelected, this, wxID_ANY);
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
        GetSideBar()->SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
        GetSideBar()->SelectAnyItem(newsel);
        GetSideBar()->SetExtraStyle(style);
        }

    //---------------------------------------------------
    wxBookCtrlEvent* SideBarBook::CreatePageChangingEvent() const
        {
        return new wxBookCtrlEvent(wxEVT_SIDEBARBOOK_PAGE_CHANGING, m_windowId);
        }

    //---------------------------------------------------
    void SideBarBook::MakeChangedEvent(wxBookCtrlEvent& event)
        {
        event.SetEventType(wxEVT_SIDEBARBOOK_PAGE_CHANGED);
        }

    //---------------------------------------------------
    bool SideBarBook::AddPage(wxWindow* page, const wxString& text, const wxWindowID Id,
                              const bool bSelect, const int imageId)
        {
        wxASSERT(page);
        if (page == nullptr)
            {
            return false;
            }
        const size_t position = GetPageCount();
        if (!DoInsertPage(position, page, text, bSelect, imageId))
            {
            return false;
            }

        GetSideBar()->InsertItem(GetSideBar()->GetFolderCount(), text, Id, imageId);

        // if the inserted page is before the selected one, we must update the
        // index of the selected page
        if (std::cmp_less_equal(position, m_selection))
            {
            // one extra page added
            ++m_selection;
            GetSideBar()->SelectFolder(m_selection);
            }

        // some page should be selected: either this one or the first one if there
        // is still no selection
        int selNew = -1;
        if (bSelect)
            {
            selNew = static_cast<int>(position);
            }
        else if (m_selection == -1)
            {
            selNew = 0;
            }

        if (selNew != m_selection)
            {
            page->Hide();
            }

        if (selNew != -1)
            {
            SetSelection(selNew);
            }

        UpdateSize();

        return true;
        }

    //---------------------------------------------------
    bool SideBarBook::AddSubPage(wxWindow* page, const wxString& text, const wxWindowID Id,
                                 const bool bSelect /*= false*/, int imageId /*= wxNOT_FOUND*/)
        {
        wxASSERT(page);
        if (page == nullptr)
            {
            return false;
            }
        if (GetSideBar()->GetFolderCount() == 0)
            {
            // no folder to attach to
            return false;
            }
        if (!DoInsertPage(GetPageCount(), page, text, bSelect, imageId))
            {
            return false;
            }
        GetSideBar()->InsertSubItemById(
            GetSideBar()->GetFolder(GetSideBar()->GetFolderCount() - 1).GetId(), text, Id, imageId);
        if (!bSelect)
            {
            page->Hide();
            }
        UpdateSize();

        return true;
        }

    //---------------------------------------------------
    bool SideBarBook::DeleteAllPages()
        {
        GetSideBar()->DeleteAllFolders();
        m_selection = wxNOT_FOUND;
        DoInvalidateBestSize();
        for (auto* page : m_pages)
            {
            delete page;
            }
        m_pages.clear();
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
            {
            return;
            }
        // if previous and new items are the same then don't change anything
        if (oldSel != wxNOT_FOUND && std::cmp_less(oldSel, GetPageCount()) &&
            (std::cmp_equal(oldSel, selNew.value()) || m_pages[oldSel] == m_pages[selNew.value()]))
            {
            return;
            }
        SetSelection(selNew.value());
        }

    //---------------------------------------------------
    bool SideBarBook::DeletePage(const size_t nPage)
        {
        const wxWindow* page = DoRemovePage(nPage);
        if (page == nullptr)
            {
            return false;
            }

        // delete null is harmless
        delete page;

        return true;
        }

    //---------------------------------------------------
    int SideBarBook::DoSetSelection(size_t nPage, int flags)
        {
        wxCHECK_MSG(nPage < GetPageCount(), wxNOT_FOUND, L"invalid page index in DoSetSelection()");

        const wxWindowUpdateLocker noUpdates(this);

        const int oldSel = GetSelection();

        if (std::cmp_not_equal(nPage, oldSel))
            {
            wxBookCtrlEvent* event = CreatePageChangingEvent();
            bool allowed = false;

            if ((flags & SetSelection_SendEvent) != 0)
                {
                event->SetSelection(static_cast<int>(nPage));
                event->SetOldSelection(oldSel);
                event->SetEventObject(this);

                allowed = !GetEventHandler()->ProcessEvent(*event) || event->IsAllowed();
                }

            if (((flags & SetSelection_SendEvent) == 0) || allowed)
                {
                if (oldSel != wxNOT_FOUND)
                    {
                    m_pages[oldSel]->Hide();
                    }

                wxWindow* page = m_pages[nPage];
                page->SetSize(GetPageRect());
                page->Show();

                // change selection now to ignore the selection change event
                UpdateSelectedPage(nPage);

                if ((flags & SetSelection_SendEvent) != 0)
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
        assert(nPage < m_pages.size() && L"invalid page index in SideBarBook::DoRemovePage()");

        // NOLINTNEXTLINE(misc-const-correctness)
        wxWindow* pageRemoved = m_pages[nPage];
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
        for (size_t nPage = 0; nPage < nCount; nPage++)
            {
            const wxWindow* const pPage = m_pages[nPage];
            if (pPage != nullptr)
                {
                const wxSize childBestSize(pPage->GetBestSize());

                bestSize.x = std::max(childBestSize.x, bestSize.x);

                bestSize.y = std::max(childBestSize.y, bestSize.y);
                }
            }

        if (m_fitToCurrentPage && (GetCurrentPage() != nullptr))
            {
            bestSize = GetCurrentPage()->GetBestSize();
            }

        // convert display area to window area, adding the size necessary
        // for the tabs
        const wxSize best = CalcSizeFromPage(bestSize);
        CacheBestSize(best);
        return best;
        }

    //---------------------------------------------------
    wxSize SideBarBook::CalcSizeFromPage(const wxSize& sizePage) const
        {
        // we need to add the size of the choice control and the border between
        const wxSize sizeController = GetControllerSize();

        wxSize size = sizePage;
        if (IsVertical())
            {
            if (sizeController.x > sizePage.x)
                {
                size.x = sizeController.x;
                }
            size.y += sizeController.y + GetInternalBorder();
            }
        else // left/right aligned
            {
            size.x += sizeController.x + GetInternalBorder();
            if (sizeController.y > sizePage.y)
                {
                size.y = sizeController.y;
                }
            }

        return size;
        }

    //---------------------------------------------------
    wxSize SideBarBook::GetControllerSize() const
        {
        if ((GetSideBar() == nullptr) || !GetSideBar()->IsShown())
            {
            return { 0, 0 };
            }

        const wxSize sizeClient = GetClientSize(), sizeCtrl = GetSideBar()->GetBestSize();

        wxSize size;
        if (IsVertical())
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

        wxRect rectPage{ wxPoint{}, GetClientSize() };

        switch (GetWindowStyle() & wxBK_ALIGN_MASK)
            {
        case wxBK_TOP:
            rectPage.y = size.y + GetInternalBorder();
            [[fallthrough]];
        case wxBK_BOTTOM:
            rectPage.height -= size.y + GetInternalBorder();
            rectPage.height = std::max(rectPage.height, 0);
            break;
        case wxBK_LEFT:
            rectPage.x = size.x + GetInternalBorder();
            [[fallthrough]];
        case wxBK_RIGHT:
            rectPage.width -= size.x + GetInternalBorder();
            rectPage.width = std::max(rectPage.width, 0);
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
        if (GetSideBar() == nullptr)
            {
            // we're not fully created yet or OnSize() should be hidden by derived class
            return;
            }

        if (GetSizer() != nullptr)
            {
            Layout();
            }
        else
            {
            // resize controller and the page area to fit inside our new size
            const wxSize sizeClient{ GetClientSize() },
                sizeBorder{ GetSideBar()->GetSize() - GetSideBar()->GetClientSize() },
                sizeCtrl{ GetControllerSize() };

            GetSideBar()->SetClientSize(sizeCtrl.x - sizeBorder.x, sizeCtrl.y - sizeBorder.y);
            // if this changes the visibility of the scrollbars the best size changes, re-layout
            // in this case
            const wxSize sizeCtrl2 = GetControllerSize();
            if (sizeCtrl != sizeCtrl2)
                {
                const wxSize sizeBorder2 = GetSideBar()->GetSize() - GetSideBar()->GetClientSize();
                GetSideBar()->SetClientSize(sizeCtrl2.x - sizeBorder2.x,
                                            sizeCtrl2.y - sizeBorder2.y);
                }

            const wxSize sizeNew = GetSideBar()->GetSize();
            wxPoint posCtrl;
            switch (GetWindowStyle() & wxBK_ALIGN_MASK)
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

            if (GetSideBar()->GetPosition() != posCtrl)
                {
                GetSideBar()->Move(posCtrl);
                }
            }

        // resize all pages to fit the new control size
        const wxRect pageRect = GetPageRect();
        for (auto* page : m_pages)
            {
            if (page == nullptr)
                {
                wxASSERT_MSG(false, L"Null page in a control that does not allow null pages?");
                continue;
                }

            page->SetSize(pageRect);
            }
        }

    //---------------------------------------------------
    bool SideBarBook::DoInsertPage(const size_t nPage, wxWindow* page,
                                   [[maybe_unused]] const wxString& text,
                                   [[maybe_unused]] bool bSelect, [[maybe_unused]] int imageId)
        {
        wxCHECK_MSG(page, false, L"NULL page in SideBarBook::DoInsertPage()");
        wxCHECK_MSG(nPage <= m_pages.size(), false,
                    L"invalid page index in SideBarBook::DoInsertPage()");
        m_pages.insert(m_pages.begin() + nPage, page);

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
        if (GetSideBar() != nullptr)
            {
            GetSideBar()->InvalidateBestSize();
            }
        else
            {
            InvalidateBestSize();
            }
        }
    } // namespace Wisteria::UI
