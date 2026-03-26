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

#include "../../base/canvas.h"
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
            @param canvas The canvas whose grid layout is displayed.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit InsertPageDlg(Canvas* canvas, wxWindow* parent, wxWindowID id = wxID_ANY,
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
        size_t GetRows() noexcept
            {
            TransferDataFromWindow();
            return m_rowCount;
            }

        /// @returns The number of columns for the new page.
        [[nodiscard]]
        size_t GetColumns() noexcept
            {
            TransferDataFromWindow();
            return m_columnCount;
            }

        /// @brief Sets the page name.
        /// @param name The page name.
        void SetPageName(const wxString& name)
            {
            m_pageName = name;
            TransferDataToWindow();
            }

        /// @returns The name for the new page (may be empty).
        [[nodiscard]]
        wxString GetPageName()
            {
            TransferDataFromWindow();
            return m_pageName;
            }

      private:
        void CreateControls();
        void SelectCell(size_t row, size_t column);

        Canvas* m_canvas{ nullptr };
        wxPanel* m_previewPanel{ nullptr };

        int m_rowCount{ 1 };
        int m_columnCount{ 1 };
        size_t m_selectedRow{ 0 };
        size_t m_selectedColumn{ 0 };
        wxString m_pageName;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PAGE_DIALOG_H
