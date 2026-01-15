/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef LISTCTRL_ITEM_VIEW_DLG_H
#define LISTCTRL_ITEM_VIEW_DLG_H

#include <vector>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    struct RowTableItem
        {
        RowTableItem(wxString column, wxString value)
            : m_column(std::move(column)), m_value(std::move(value))
            {
            }

        wxString m_column;
        wxString m_value;
        };

    /// @brief Data provider for a row of items from a grid.
    class ListRowTable final : public wxGridStringTable
        {
      public:
        /// @private
        ListRowTable() = default;
        ListRowTable(ListRowTable&) = delete;
        void operator=(ListRowTable&) = delete;

        /// @brief Constructor.
        /// @param values The values to show.
        explicit ListRowTable(std::vector<RowTableItem> values) : m_values(std::move(values)) {}

        /// @private
        [[nodiscard]]
        int GetNumberRows() final
            {
            return m_values.size();
            }

        /// @private
        [[nodiscard]]
        int GetNumberCols() final
            {
            return 2;
            }

        /// @private
        [[nodiscard]]
        wxString GetValue(int row, int col) final
            {
            wxCHECK_MSG((row >= 0 && row < GetNumberRows()) && (col >= 0 && col < GetNumberCols()),
                        wxString{}, L"invalid row or column index in ListRowTable");
            return (static_cast<size_t>(row) < m_values.size() && (col == 0 || col == 1)) ?
                       (col == 0 ? m_values[row].m_column : m_values[row].m_value) :
                       wxString{};
            }

        /// @private
        /// @brief Will just reset the cell to its original value.
        ///     The intention is that this dialog is read-only (it's just viewing the contents of
        ///     a list's row), but the user may want to go into pseudo edit mode to select
        ///     portions of the text.
        void SetValue([[maybe_unused]] int row, [[maybe_unused]] int col,
                      [[maybe_unused]] const wxString& str) final
            {
            // no-op
            }

      private:
        std::vector<RowTableItem> m_values;
        };

    class ListCtrlItemViewDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @note This dialog needs a 2-step construction. Call this constructor, fill its list
                  via AddValue(), then call Create().*/
        ListCtrlItemViewDlg() = default;
        /// @private
        ListCtrlItemViewDlg(const ListCtrlItemViewDlg&) = delete;
        /// @private
        ListCtrlItemViewDlg& operator=(const ListCtrlItemViewDlg&) = delete;

        /** @brief Creation.
            @param parent The parent window.
            @param id The ID for this dialog.
            @param caption The title of this dialog.
            @param pos The position of this dialog.
            @param size The size of this dialog.
            @param style The style of this dialog.*/
        bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"View Item"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
            wxDialog::Create(parent, id, caption, pos, size, style);

            CreateControls();
            GetSizer()->Fit(this);
            GetSizer()->SetSizeHints(this);
            Centre();

            Bind(wxEVT_BUTTON, &ListCtrlItemViewDlg::OnButtonClick, this);
            return true;
            }

        /** @brief Adds a value to the list.
            @param columnName The header of the column.
            @param value The value to display in the grid.*/
        void AddValue(const wxString& columnName, const wxString& value)
            {
            m_values.emplace_back(columnName, value);
            }

        void OnButtonClick(wxCommandEvent& event);

      private:
        void CreateControls();

        std::vector<RowTableItem> m_values;

        wxGrid* m_grid{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // LISTCTRL_ITEM_VIEW_DLG_H
