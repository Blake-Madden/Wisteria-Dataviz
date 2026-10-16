/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PAGE_SELECTION_DIALOG_H
#define PAGE_SELECTION_DIALOG_H

#include "dialogwithhelp.h"
#include <vector>
#include <wx/bannerwindow.h>
#include <wx/checklst.h>
#include <wx/dialog.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief A dialog with a checklist of a project's pages, for choosing which
    ///     pages to include in an export.
    class PageSelectionDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param caption The title of the dialog.
            @param pageLabels The display names of the pages.
            @param pageGuids The GUIDs of the pages (in the same order as @c pageLabels).
            @param excludedGuids The GUIDs of the pages that should start out unchecked.
            @param id The window ID.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        PageSelectionDlg(wxWindow* parent, const wxString& caption, wxArrayString pageLabels,
                         std::vector<wxString> pageGuids,
                         const std::vector<wxString>& excludedGuids, const wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         const long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        /// @private
        PageSelectionDlg() = delete;
        /// @private
        PageSelectionDlg(const PageSelectionDlg&) = delete;
        /// @private
        PageSelectionDlg& operator=(const PageSelectionDlg&) = delete;

        /// @returns The GUIDs of the pages that were left unchecked.
        [[nodiscard]]
        std::vector<wxString> GetExcludedGuids() const;

      private:
        /// Creates the controls and sizers.
        void CreateControls();
        /// Checks (or unchecks) every page.
        void CheckAll(const bool check);
        /// Disables the OK button if no pages are checked.
        void UpdateOkButton();

        wxArrayString m_pageLabels;
        std::vector<wxString> m_pageGuids;
        wxArrayInt m_checkedItems;

        wxCheckListBox* m_pageList{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // PAGE_SELECTION_DIALOG_H
