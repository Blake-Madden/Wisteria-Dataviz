/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __SIDEBARBOOK_H__
#define __SIDEBARBOOK_H__

#include <wx/control.h>
#include <wx/dynarray.h>
#include <wx/bookctrl.h>
#include <wx/imaglist.h>
#include <wx/wupdlock.h>
#include "sidebar.h"

/// @private
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGED,  wxBookCtrlEvent);
/// @private
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGING, wxBookCtrlEvent);

namespace Wisteria::UI
    {
    /// @brief Class for connecting a sidebar's content with a set of dialog pages.
    class SideBarBook final : public wxControl
        {
    public:
        // flags for DoSetSelection()
        /// @private
        static constexpr int SetSelection_SendEvent = 1;

        /// @brief Constructor.
        /// @param parent The parent window.
        /// @param id The ID of this control.
        SideBarBook(wxWindow *parent, wxWindowID id);

        /** @brief Adds a folder and its connected page.
            @param page The (dialog) page to connect.
            @param text The label for folder in the sidebar.
            @param Id The ID for the item.
            @param bSelect @c true to select the page after adding it.
            @param imageId The index into the image list to use as an icon.
            @returns @c true if the page was successfully added.*/
        bool AddPage(wxWindow *page,
                     const wxString& text,
                     const wxWindowID Id,
                     const bool bSelect = false,
                     const int imageId = wxNOT_FOUND);
        /** @brief Adds a subitem to one of the folders.
            @param page The (dialog) page to connect.
            @param text The label for item in the sidebar.
            @param Id The ID for the item.
            @param bSelect @c true to select the page after adding it.
            @param imageId The index into the image list to use as an icon.
            @returns @c true if the page was successfully added.*/
        bool AddSubPage(wxWindow* page, const wxString& text, const wxWindowID Id,
                        bool bSelect = false, int imageId = wxNOT_FOUND);
        /// @returns The panel which represents the given page.
        /// @param n The index of the page to return.
        [[nodiscard]] wxWindow* GetPage(size_t n) const
            { return m_pages.at(n); }
        /// @returns The current page or null if none.
        [[nodiscard]] wxWindow* GetCurrentPage() const
            {
            const int n = GetSelection();
            return n == wxNOT_FOUND ? nullptr : GetPage(n);
            }
        /// @returns The number of pages in the dialog.
        [[nodiscard]] size_t GetPageCount() const noexcept
            { return m_pages.size(); }

        /// @brief Selects a page.
        /// @param n The index of the page to select.
        /// @returns The index of the selected page if successful; @c wxNOT_FOUND otherwise.
        int SetSelection(size_t n)
            { return DoSetSelection(n, SetSelection_SendEvent); }
        /// @returns The currently selected page or @c wxNOT_FOUND if none.
        [[nodiscard]] int GetSelection() const noexcept
            { return m_selection; }

        /// @private
        void UpdateSelectedPage(size_t newsel);

        /// @brief Deletes all the pages connected to this control.
        /// @returns @c true if all pages were deleted successfully.
        bool DeleteAllPages();

        /// @returns The sidebar.
        [[nodiscard]] SideBar* GetSideBar() noexcept
            { return m_sidebar; }
        /// @private
        [[nodiscard]] const SideBar* GetSideBar() const noexcept
            { return m_sidebar; }

        /// @returns The image list used for the sidebar.
        [[nodiscard]] std::vector<wxBitmap>& GetImageList()
            {
            wxASSERT(m_sidebar);
            return m_sidebar->GetImageList();
            }
        /// @returns The image list used for the sidebar (constantly).
        [[nodiscard]] const std::vector<wxBitmap>& GetImageList() const
            {
            wxASSERT(m_sidebar);
            return m_sidebar->GetImageList();
            }
    protected:
        // set the selection to the given page, sending the events (which can
        // possibly prevent the page change from taking place) if SendEvent flag is
        // included
        /// @private
        int DoSetSelection(size_t nPage, int flags = 0);
        /// @brief Remove one page from the control and delete it.
        /// @param n The index of the page to delete.
        /// @returns @c true if the page was successfully deleted.
        bool DeletePage(size_t n);
        // remove the page and return a pointer to it
        /// @private
        wxWindow* DoRemovePage(size_t page);

        // This method also invalidates the size of the controller and should be
        // called instead of just InvalidateBestSize() whenever pages are added or
        // removed as this also affects the controller
        /// @private
        void DoInvalidateBestSize();
        /// @private
        void OnSize(wxSizeEvent& event);
        // Lay out controls
        /// @private
        void DoSize();
        // get the page area
        /// @private
        [[nodiscard]] wxRect GetPageRect() const;
        // return the size of the area needed to accommodate the controller
        /// @private
        [[nodiscard]] wxSize GetControllerSize() const;
        // our best size is the size which fits all our pages
        /// @private
        [[nodiscard]] wxSize DoGetBestSize() const final;
        // calculate the size of the control from the size of its page
        // by default this simply returns size enough to fit both the page and the
        // controller
        /// @private
        [[nodiscard]] wxSize CalcSizeFromPage(const wxSize& sizePage) const;
        // this should be called when we need to be relaid out
        /// @private
        void UpdateSize();

        /// @private
        bool DoInsertPage(size_t n,
                          wxWindow* page,
                          [[maybe_unused]] const wxString& text,
                          [[maybe_unused]] bool bSelect = false,
                          [[maybe_unused]] int imageId = wxNOT_FOUND);

        /// @returns @c true if we have @c wxBK_TOP or @c wxBK_BOTTOM style.
        [[nodiscard]] bool IsVertical() const noexcept
            { return HasFlag(wxBK_BOTTOM|wxBK_TOP); }
        // get/set size of area between book control area and page area
        /// @private
        [[nodiscard]] unsigned int GetInternalBorder() const noexcept
            { return wxSizerFlags::GetDefaultBorder(); }
        // choose the default border for this window
        /// @private
        [[nodiscard]] wxBorder GetDefaultBorder() const noexcept final
            { return wxBORDER_NONE; }

        /// @private
        [[nodiscard]] wxBookCtrlEvent* CreatePageChangingEvent() const;
        /// @private
        void MakeChangedEvent(wxBookCtrlEvent &event);

        // event handlers
        /// @private
        void OnListSelected([[maybe_unused]] wxCommandEvent& event);
    private:
        SideBarBook() noexcept {}

        // the array of all pages of this control
        std::vector<wxWindow*> m_pages;

        // controller buddy (navigation window on the left).
        SideBar* m_sidebar{ nullptr };

        // Whether to shrink to fit current page
        bool m_fitToCurrentPage{ false };

        // the sizer containing the control
        wxSizer* m_controlSizer{ nullptr };

        // The currently selected page (in range 0..m_pages.size()-1 inclusive) or
        // wxNOT_FOUND if none (this can normally only be the case for an empty
        // control without any pages).
        int m_selection{ wxNOT_FOUND };

        wxDECLARE_DYNAMIC_CLASS_NO_COPY(SideBarBook);
        };
    }

/** @}*/

// ----------------------------------------------------------------------------
// listbook event class and related stuff
// ----------------------------------------------------------------------------

#define EVT_SIDEBARBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGED, winid, wxBookCtrlEventHandler(fn))

#define EVT_SIDEBARBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(EVT_COMMAND_SIDEBARBOOK_PAGE_CHANGING, winid, wxBookCtrlEventHandler(fn))

#endif // __SIDEBARBOOK_H__
