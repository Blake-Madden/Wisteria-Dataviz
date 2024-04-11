/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __LISTCTRL_SORT_DLG_H__
#define __LISTCTRL_SORT_DLG_H__

#include "../../base/graphitems.h"
#include "../controls/listctrlex.h"
#include "dialogwithhelp.h"
#include <wx/string.h>
#include <wx/wx.h>

/// @brief A dialog to specify how to sort a list control.
class ListCtrlSortDlg final : public Wisteria::UI::DialogWithHelp
    {
  public:
    /** @brief Constructor.
        @param parent The parent window.
        @param columnChoices The column names from the buddy list control.
        @param id The dialog's ID.
        @param caption The dialog's caption.
        @param pos The dialog's position.
        @param size The dialog's size.
        @param style The dialog's style.*/
    ListCtrlSortDlg(wxWindow* parent, const wxArrayString& columnChoices, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"Sort Columns"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
        : m_columnChoices(columnChoices)
        {
        Create(parent, id, caption, pos, size, style);
        }

    /// @private
    ListCtrlSortDlg(const ListCtrlSortDlg& that) = delete;
    /// @private
    ListCtrlSortDlg& operator=(const ListCtrlSortDlg& that) = delete;

    /// @brief Sets the column sorting information.
    /// @param sortColumns The columns to sort by.
    void
    FillSortCriteria(const std::vector<std::pair<size_t, Wisteria::SortDirection>>& sortColumns);

    /// @returns The columns' names and sorting information.
    [[nodiscard]]
    std::vector<std::pair<wxString, Wisteria::SortDirection>> GetColumnsInfo() const;

  private:
    /// Creation
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxString& caption = _(L"Sort Columns"),
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
        {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

        CreateControls();
        Centre();
        return true;
        }

    void CreateControls();

    [[nodiscard]]
    const static wxString GetAscendingLabel()
        {
        return _(L"Smallest to Largest");
        }

    [[nodiscard]]
    const static wxString GetDescendingLabel()
        {
        return _(L"Largest to Smallest");
        }

    wxArrayString m_columnChoices;
    ListCtrlEx* m_columnList{ nullptr };
    std::shared_ptr<ListCtrlExNumericDataProvider> m_data{
        std::make_shared<ListCtrlExNumericDataProvider>()
    };
    };

    /** @}*/

#endif //__LISTCTRL_SORT_DLG_H__
