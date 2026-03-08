/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_PAGE_DIALOG_H
#define INSERT_PAGE_DIALOG_H

#include "dialogwithhelp.h"
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Dialog for inserting a new page (canvas) into a project.
    class InsertPageDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit InsertPageDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                               const wxString& caption = _(L"Insert Page"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER);

        /// @private
        InsertPageDlg() = delete;
        /// @private
        InsertPageDlg(const InsertPageDlg&) = delete;
        /// @private
        InsertPageDlg& operator=(const InsertPageDlg&) = delete;

        /// @returns The number of rows for the new page.
        [[nodiscard]]
        size_t GetRows() const noexcept
            {
            return static_cast<size_t>(m_rowsSpin->GetValue());
            }

        /// @returns The number of columns for the new page.
        [[nodiscard]]
        size_t GetColumns() const noexcept
            {
            return static_cast<size_t>(m_columnsSpin->GetValue());
            }

        /// @brief Sets the number of rows.
        /// @param rows The number of rows.
        void SetRows(const size_t rows) { m_rowsSpin->SetValue(static_cast<int>(rows)); }

        /// @brief Sets the number of columns.
        /// @param columns The number of columns.
        void SetColumns(const size_t columns)
            {
            m_columnsSpin->SetValue(static_cast<int>(columns));
            }

        /// @brief Sets the page name.
        /// @param name The page name.
        void SetPageName(const wxString& name) { m_nameCtrl->SetValue(name); }

        /// @returns The name for the new page (may be empty).
        [[nodiscard]]
        wxString GetPageName() const
            {
            return m_nameCtrl->GetValue();
            }

      private:
        void CreateControls();

        wxSpinCtrl* m_rowsSpin{ nullptr };
        wxSpinCtrl* m_columnsSpin{ nullptr };
        wxTextCtrl* m_nameCtrl{ nullptr };
        wxPanel* m_previewPanel{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PAGE_DIALOG_H
