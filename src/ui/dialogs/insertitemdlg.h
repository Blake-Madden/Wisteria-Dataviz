/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_ITEM_DIALOG_H
#define INSERT_ITEM_DIALOG_H

#include "../../base/canvas.h"
#include "../../base/reportbuilder.h"
#include "../../math/mathematics.h"
#include "../controls/sidebarbook.h"
#include "dialogwithhelp.h"
#include <array>
#include <bitset>
#include <utility>
#include <wx/clrpicker.h>
#include <wx/dcbuffer.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Flags controlling which parts of the "Page" page are visible.
    enum ItemDlgPageOptions : int
        {
        /// @brief Show the canvas placement grid.
        ItemDlgIncludeCanvasPlacement = 1 << 0,
        /// @brief Show the page-level controls (alignment, scaling, margins,
        ///     padding, outline, etc.).
        ItemDlgIncludePageSettings = 1 << 1,
        /// @brief Show all page options (the default).
        ItemDlgIncludeAllPageOptions = ItemDlgIncludeCanvasPlacement | ItemDlgIncludePageSettings
        };

    /** @brief Base dialog for inserting an item into a canvas cell.
        @details Provides a sidebar with a "Page" section showing the canvas grid.
            Users click (or Tab/Shift+Tab through) cells to select where
            the item will be placed. The selected cell is drawn with a dotted outline.

            Derived dialogs (e.g., "Insert Pie Chart") can override
            CreateControls() to add additional sidebar pages for item-specific options.*/
    class InsertItemDlg : public DialogWithHelp
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
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.
            @param pageOptions Bitmask of ItemDlgPageOptions controlling which
                parts of the "Page" page are shown.*/
        InsertItemDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                      const wxString& caption, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                      EditMode editMode = EditMode::Insert,
                      int pageOptions = ItemDlgIncludeAllPageOptions);

        /// @private
        InsertItemDlg(const InsertItemDlg&) = delete;
        /// @private
        InsertItemDlg& operator=(const InsertItemDlg&) = delete;

        /// @returns The selected row.
        [[nodiscard]]
        size_t GetSelectedRow() const noexcept
            {
            return m_selectedRow;
            }

        /// @returns The selected column.
        [[nodiscard]]
        size_t GetSelectedColumn() const noexcept
            {
            return m_selectedColumn;
            }

        /// @returns The selected horizontal page alignment.
        [[nodiscard]]
        PageHorizontalAlignment GetHorizontalPageAlignment() const;

        /// @returns The selected vertical page alignment.
        [[nodiscard]]
        PageVerticalAlignment GetVerticalPageAlignment() const;

        /// @returns The scaling factor.
        [[nodiscard]]
        double GetItemScaling() const;

        /// @returns The canvas margins (top, right, bottom, left).
        [[nodiscard]]
        std::array<int, 4> GetCanvasMargins() const;

        /// @returns The padding (top, right, bottom, left).
        [[nodiscard]]
        std::array<int, 4> GetPadding() const;

        /// @returns Whether to fit the row height to the item's content.
        [[nodiscard]]
        bool GetFitRowToContent() const;

        /// @brief Sets the default value for "fit row to content."
        /// @param fit @c true to fit the row height to the item's content.
        void SetFitRowToContent(const bool fit) noexcept { m_fitRowToContent = fit; }

        /// @returns Whether the item should use a fixed width on the canvas.
        [[nodiscard]]
        bool GetFixedWidth() const;

        /// @returns The outline pen (color and width).
        [[nodiscard]]
        wxPen GetOutlinePen() const;

        /// @returns Which sides of the outline are shown
        ///     (top, right, bottom, left), matching
        ///     GraphItemInfo::Outline() order.
        [[nodiscard]]
        std::bitset<4> GetOutlineSides() const;

        /// @returns The number of rows in the grid.
        [[nodiscard]]
        size_t GetRowCount() const noexcept
            {
            return m_rows;
            }

        /// @returns The number of columns in the grid.
        [[nodiscard]]
        size_t GetColumnCount() const noexcept
            {
            return m_columns;
            }

        /// @brief Resizes the canvas grid to match the dialog's grid dimensions.
        /// @details Call after the dialog returns @c wxID_OK and before placing
        ///     any items. This is a no-op if the grid is already large enough.
        void ApplyGridSize()
            {
            if (m_canvas != nullptr)
                {
                const auto [currentRows, currentCols] = m_canvas->GetFixedObjectsGridSize();
                if (m_rows > currentRows || m_columns > currentCols)
                    {
                    m_canvas->SetFixedObjectsGridSize(std::max(currentRows, m_rows),
                                                      std::max(currentCols, m_columns));
                    }
                }
            }

        /// @brief Applies the page-level options (alignment, scaling, margins,
        ///     padding, outline pen, and border sides) to a graph item.
        /// @param item The item to configure.
        void ApplyPageOptions(GraphItems::GraphItemBase& item) const;

        /// @brief Populates the page-level controls from an existing graph item.
        /// @param item The item to read the options from.
        void LoadPageOptions(const GraphItems::GraphItemBase& item);

        /// @brief Selects a specific cell in the grid preview.
        /// @param row The row to select.
        /// @param column The column to select.
        void SetSelectedCell(size_t row, size_t column);

      protected:
        /// @brief Creates the dialog controls.
        /// @details Override in derived classes to add additional sidebar pages.
        ///     Call the base-class version first to create the "Page" page.
        ///     After all controls are added, call FinalizeControls() to add
        ///     the OK/Cancel buttons and finalize the layout.
        virtual void CreateControls();

        /// @brief Adds the "Page" sidebar page.
        /// @details Call this explicitly from CreateControls() at the position
        ///     where the "Page" page should appear in the sidebar.
        void CreatePageOptionsPage();

        /// @brief Adds OK/Cancel buttons and finalizes layout.
        /// @details Must be called after all sidebar pages have been added.
        void FinalizeControls();

        /// @brief Checks whether placing the item would overwrite existing
        ///     canvas cells and prompts the user for confirmation.
        /// @details The base implementation checks only the selected cell.
        ///     Derived classes (e.g., InsertGraphDlg) can override this to
        ///     also check cells used by related items such as legends.
        /// @returns @c true if placement should proceed, @c false to cancel.
        virtual bool ConfirmOverwrite();

        /// @returns The sidebar book control.
        [[nodiscard]]
        SideBarBook* GetSideBarBook() noexcept
            {
            return m_sideBarBook;
            }

        /// @returns The report builder.
        [[nodiscard]]
        const ReportBuilder* GetReportBuilder() const noexcept
            {
            return m_reportBuilder;
            }

        /// @brief Expands any double-brace constant placeholders in @p value
        ///     using the attached report builder.
        /// @details Used by graph edit dialogs to translate a cached raw
        ///     property template (e.g., @c "Revenue {{MaxFY}}") into a real
        ///     column name (e.g., @c "Revenue 2025") before passing it
        ///     to a graph's @c SetData() or to a variable selection dialog.
        /// @param value The string (possibly containing placeholders) to expand.
        /// @returns The expanded string, or @p value unchanged if no report
        ///     builder is available.
        [[nodiscard]]
        wxString ExpandVariable(const wxString& value) const
            {
            return (m_reportBuilder != nullptr) ? m_reportBuilder->ExpandConstants(value) : value;
            }

        /// @returns The canvas.
        [[nodiscard]]
        Canvas* GetCanvas() noexcept
            {
            return m_canvas;
            }

        /// @returns The edit mode.
        [[nodiscard]]
        EditMode GetEditMode() const noexcept
            {
            return m_editMode;
            }

        /// @brief ID for the Page sidebar section.
        /// @note Subclass IDs start at wxID_HIGHEST + 2 to avoid collision with this.
        constexpr static wxWindowID ID_PAGE_SECTION{ wxID_HIGHEST + 1 };

      private:
        void SelectCell(size_t row, size_t column);

        Canvas* m_canvas{ nullptr };
        const ReportBuilder* m_reportBuilder{ nullptr };
        SideBarBook* m_sideBarBook{ nullptr };
        wxPanel* m_gridPanel{ nullptr };

        size_t m_rows{ 1 };
        size_t m_columns{ 1 };
        size_t m_selectedRow{ 0 };
        size_t m_selectedColumn{ 0 };

        EditMode m_editMode{ EditMode::Insert };

        // DDX data members (bound to controls via wxGenericValidator)
        int m_horizontalAlign{ 0 };
        int m_verticalAlign{ 0 };
        int m_marginTop{ 0 };
        int m_marginRight{ 0 };
        int m_marginBottom{ 0 };
        int m_marginLeft{ 0 };
        int m_paddingTop{ 0 };
        int m_paddingRight{ 0 };
        int m_paddingBottom{ 0 };
        int m_paddingLeft{ 0 };
        bool m_fitRowToContent{ false };
        bool m_fixedWidth{ false };
        int m_outlineWidth{ 1 };
        int m_outlineStyle{ 0 };
        bool m_outlineTop{ false };
        bool m_outlineRight{ false };
        bool m_outlineBottom{ false };
        bool m_outlineLeft{ false };

        int m_pageOptions{ ItemDlgIncludeAllPageOptions };

        // controls without DDX validator support
        wxSpinCtrlDouble* m_scalingSpin{ nullptr };
        wxColourPickerCtrl* m_outlineColorPicker{ nullptr };

        // controls needed for event handlers
        wxSpinCtrl* m_rowsSpin{ nullptr };
        wxSpinCtrl* m_columnsSpin{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_ITEM_DIALOG_H
