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
        /// @brief Whether this dialog is being used to insert a new item or edit an existing one.
        enum class EditMode
            {
            /// @brief A new item is being created and inserted into a canvas.
            Insert,
            /// @brief An existing item is being edited.
            Edit
            };

        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param pageNames The names of the pages currently in the project.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).
            @param editMode Whether the page is being inserted or edited.*/
        explicit InsertPageDlg(Canvas* canvas, const wxArrayString& pageNames, wxWindow* parent,
                               wxWindowID id = wxID_ANY,
                               const wxString& caption = _(L"Insert Page"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER,
                               EditMode editMode = EditMode::Insert);

        /// @private
        InsertPageDlg() = delete;
        /// @private
        InsertPageDlg(const InsertPageDlg&) = delete;
        /// @private
        InsertPageDlg& operator=(const InsertPageDlg&) = delete;

        /// @returns The insertion position (0 for before, 1 for after).
        [[nodiscard]]
        int GetInsertPosition()
            {
            TransferDataFromWindow();
            return m_insertAfter ? 1 : 0;
            }

        /// @returns The index of the page to insert before/after.
        [[nodiscard]]
        int GetRelativePageIndex()
            {
            TransferDataFromWindow();
            return m_relativePageIndex;
            }

        /// @returns The number of rows for the new page.
        [[nodiscard]]
        size_t GetRows()
            {
            TransferDataFromWindow();
            return m_rowCount;
            }

        /// @returns The number of columns for the new page.
        [[nodiscard]]
        size_t GetColumns()
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

        /// @returns The edit mode.
        [[nodiscard]]
        EditMode GetEditMode() const noexcept
            {
            return m_editMode;
            }

      private:
        void CreateControls();
        void SelectCell(size_t row, size_t column);

        Canvas* m_canvas{ nullptr };
        wxPanel* m_previewPanel{ nullptr };

        EditMode m_editMode{ EditMode::Insert };

        int m_rowCount{ 1 };
        int m_columnCount{ 1 };
        size_t m_selectedRow{ 0 };
        size_t m_selectedColumn{ 0 };
        bool m_insertAfter{ true };
        int m_relativePageIndex{ wxNOT_FOUND };
        wxArrayString m_pageNames;
        wxString m_pageName;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PAGE_DIALOG_H
