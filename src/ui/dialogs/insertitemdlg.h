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
#include <utility>
#include <wx/dcbuffer.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Base dialog for inserting an item into a canvas cell.
        @details Provides a sidebar with a "Page" section showing the canvas grid.
            Users click (or Tab/Shift+Tab through) cells to select where
            the item will be placed. The selected cell is drawn with a dotted outline.

            Derived dialogs (e.g., "Insert Pie Chart") can override
            CreateControls() to add additional sidebar pages for item-specific options.*/
    class InsertItemDlg : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        InsertItemDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                      const wxString& caption, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        InsertItemDlg(const InsertItemDlg&) = delete;
        /// @private
        InsertItemDlg& operator=(const InsertItemDlg&) = delete;

        /// @returns The selected row (zero-based).
        [[nodiscard]]
        size_t GetSelectedRow() const noexcept
            {
            return m_selectedRow;
            }

        /// @returns The selected column (zero-based).
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

        /// @returns Whether the item should use a fixed width on the canvas.
        [[nodiscard]]
        bool GetFixedWidth() const;

      protected:
        /// @brief Creates the dialog controls.
        /// @details Override in derived classes to add additional sidebar pages.
        ///     Call the base-class version first to create the "Page" page.
        ///     After all controls are added, call FinalizeControls() to add
        ///     the OK/Cancel buttons and finalize the layout.
        virtual void CreateControls();

        /// @brief Adds OK/Cancel buttons and finalizes layout.
        /// @details Must be called after all sidebar pages have been added.
        void FinalizeControls();

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

        /// @returns The canvas.
        [[nodiscard]]
        Canvas* GetCanvas() noexcept
            {
            return m_canvas;
            }

        /// @brief ID for the Page sidebar section.
        constexpr static wxWindowID ID_PAGE_SECTION{ wxID_HIGHEST + 1 };

      private:
        void SelectCell(size_t row, size_t column);
        [[nodiscard]]
        static wxString GetItemTypeName(const std::shared_ptr<GraphItems::GraphItemBase>& item);

        Canvas* m_canvas{ nullptr };
        const ReportBuilder* m_reportBuilder{ nullptr };
        SideBarBook* m_sideBarBook{ nullptr };
        wxPanel* m_gridPanel{ nullptr };

        size_t m_rows{ 1 };
        size_t m_columns{ 1 };
        size_t m_selectedRow{ 0 };
        size_t m_selectedColumn{ 0 };

        wxChoice* m_horizontalAlignChoice{ nullptr };
        wxChoice* m_verticalAlignChoice{ nullptr };
        wxSpinCtrlDouble* m_scalingSpin{ nullptr };
        wxSpinCtrl* m_marginTopSpin{ nullptr };
        wxSpinCtrl* m_marginRightSpin{ nullptr };
        wxSpinCtrl* m_marginBottomSpin{ nullptr };
        wxSpinCtrl* m_marginLeftSpin{ nullptr };
        wxSpinCtrl* m_paddingTopSpin{ nullptr };
        wxSpinCtrl* m_paddingRightSpin{ nullptr };
        wxSpinCtrl* m_paddingBottomSpin{ nullptr };
        wxSpinCtrl* m_paddingLeftSpin{ nullptr };
        wxCheckBox* m_fitRowToContentCheck{ nullptr };
        wxCheckBox* m_fixedWidthCheck{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_ITEM_DIALOG_H
