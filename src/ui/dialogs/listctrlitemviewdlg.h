/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __LISTCTRL_ITEM_VIEW_DLG_H__
#define __LISTCTRL_ITEM_VIEW_DLG_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/statline.h>
#include <vector>
#include "../controls/htmltablewin.h"

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
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& caption = _(L"View Item"),
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN)
        {
        SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create( parent, id, caption, pos, size, style );

        CreateControls();
        GetSizer()->Fit(this);
        GetSizer()->SetSizeHints(this);
        Centre();

        Bind(wxEVT_HTML_LINK_CLICKED, &ListCtrlItemViewDlg::OnHyperlinkClicked, this);
        Bind(wxEVT_BUTTON, &ListCtrlItemViewDlg::OnButtonClick, this);
        return true;
        }

    /** Adds a value to the list.
        @param columnName The header of the column.
        @param value The value to display in the grid.
        @param isUrl Whether the value is a hyperlink that user can click on.*/
    void AddValue(const wxString& columnName, const wxString& value, const bool isUrl = false)
        { m_values.emplace_back(DisplayItem(columnName, value, isUrl)); }

    void OnHyperlinkClicked(wxHtmlLinkEvent& event);
    void OnButtonClick(wxCommandEvent& event);
private:
    void CreateControls();
    struct DisplayItem
        {
        DisplayItem(const wxString& column, const wxString& value, const bool isUrl) :
            m_column(column), m_value(value), m_isUrl(isUrl) {}
        wxString m_column;
        wxString m_value;
        bool m_isUrl{ false };
        };
    std::vector<DisplayItem> m_values;

    Wisteria::UI::HtmlTableWindow* m_htmlWindow{ nullptr };
    };

/** @}*/

#endif //__LISTCTRL_ITEM_VIEW_DLG_H__
