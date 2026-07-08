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
#include "../../controls/thumbnail.h"
#include "../dialogwithhelp.h"
#include <utility>
#include <wx/clrpicker.h>
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

        /// @returns The watermark label text (may be empty).
        [[nodiscard]]
        wxString GetWatermarkLabel()
            {
            TransferDataFromWindow();
            return m_watermarkLabel;
            }

        /// @returns The watermark color.
        [[nodiscard]]
        wxColour GetWatermarkColor() const
            {
            return (m_watermarkColorPicker != nullptr) ? m_watermarkColorPicker->GetColour() :
                                                         m_watermarkColor;
            }

        /// @returns The page background color.
        [[nodiscard]]
        wxColour GetPageBackgroundColor() const
            {
            return (m_bgColorPicker != nullptr) ? m_bgColorPicker->GetColour() : m_backgroundColor;
            }

        /// @returns The background image file path (may be empty).
        [[nodiscard]]
        wxString GetBackgroundImagePath() const
            {
            return (m_bgThumbnail != nullptr) ? m_bgThumbnail->GetFilePath() : wxString{};
            }

        /// @returns The background image opacity (needs to be clamped to 0–255).
        [[nodiscard]]
        int GetBackgroundImageOpacity()
            {
            TransferDataFromWindow();
            return m_backgroundImageOpacity;
            }

        /// @returns @c true if page numbering should reset to 1 on this page.
        [[nodiscard]]
        bool GetResetPageNumbering()
            {
            TransferDataFromWindow();
            return m_resetPageNumbering;
            }

        /** @brief Applies the edits made to the preview grid (e.g., items removed) to @c canvas.
            @param canvas The canvas to sync the edited grid to. This should be called
                after resizing @c canvas's grid (via SetFixedObjectsGridSize()) to the
                dialog's reported row/column counts.*/
        void ApplyGridEdits(Canvas* canvas) const;

      private:
        void CreateControls();
        void SelectCell(size_t row, size_t column);
        void ResizeFixedObjectsGrid();
        void BindPreviewPanelMouseEvents();
        void PaintPreview(wxPaintEvent& event);
        void DrawPreview(wxGCDC& dc);
        [[nodiscard]]
        std::pair<size_t, size_t> CellFromPoint(const wxPoint& pt) const;

        Canvas* m_canvas{ nullptr };
        wxPanel* m_previewPanel{ nullptr };
        wxColourPickerCtrl* m_watermarkColorPicker{ nullptr };
        wxColourPickerCtrl* m_bgColorPicker{ nullptr };
        wxStaticText* m_bgOpacityLabel{ nullptr };
        Thumbnail* m_bgThumbnail{ nullptr };

        EditMode m_editMode{ EditMode::Insert };

        int m_rowCount{ 1 };
        int m_columnCount{ 1 };
        size_t m_selectedRow{ 0 };
        size_t m_selectedColumn{ 0 };
        // Local, editable copy of the canvas's fixed object grid. Edits made in the
        // preview panel (e.g., deleting an item) are applied here first and only
        // committed back to the canvas via ApplyGridEdits().
        std::vector<std::vector<std::shared_ptr<Wisteria::GraphItems::GraphItemBase>>>
            m_fixedObjectsGrid;
        // drag-and-drop state for moving an item from one cell to another in the preview panel
        bool m_isDraggingItem{ false };
        size_t m_dragSourceRow{ 0 };
        size_t m_dragSourceColumn{ 0 };
        size_t m_dragTargetRow{ 0 };
        size_t m_dragTargetColumn{ 0 };
        wxPoint m_dragPosition;
        bool m_insertAfter{ true };
        int m_relativePageIndex{ wxNOT_FOUND };
        wxArrayString m_pageNames;
        wxString m_pageName;

        wxString m_watermarkLabel;
        wxColour m_watermarkColor{ wxColour(255, 0, 0) };
        wxColour m_backgroundColor{ *wxWHITE };
        int m_backgroundImageOpacity{ wxALPHA_OPAQUE };
        bool m_resetPageNumbering{ false };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PAGE_DIALOG_H
