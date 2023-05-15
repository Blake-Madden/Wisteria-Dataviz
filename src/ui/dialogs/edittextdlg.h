/** @addtogroup UI
    @brief Classes for the user interface.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __EDIT_TEXT_DIALOG_H__
#define __EDIT_TEXT_DIALOG_H__

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/valgen.h>

/// @brief Dialog for editing text from a project.
class EditTextDlg final : public wxDialog
    {
public:
    /** @brief Constructor.
        @param parent The parent window.
        @param id The dialog's ID.
        @param caption The dialog's caption.
        @param description A description label to show beneath the text.
        @param pos The dialog's window position.
        @param size The dialog's size.
        @param style The dialog's style.*/
    explicit EditTextDlg(wxWindow* parent,
             wxWindowID id = wxID_ANY,
             const wxString& caption = _(L"Edit Text"),
             const wxString& description = wxEmptyString,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxSize(600, 600),
             long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
        { Create(parent, id, caption, description, pos, size, style); }
    /// @private
    EditTextDlg(const EditTextDlg& that) = delete;
    /// @private
    EditTextDlg& operator=(const EditTextDlg& that) = delete;
    /// @brief Sets the text to display for editing.
    /// @param text The text to display.
    /// @todo Use move semantics.
    void SetValue(const wxString& text)
        {
        m_value = text;
        TransferDataToWindow();
        }
     /// @returns The edited text.
     [[nodiscard]]
     const wxString& GetValue() const noexcept
        { return m_value; }
private:
    /// Creation.
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxString& caption = wxEmptyString,
        const wxString& description = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE)
        {
        SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style );
        SetMinSize(FromDIP(size));

        m_description = description;

        CreateControls();
        Centre();

        return true;
        }

    /// Creates the controls and sizers.
    void CreateControls();

    wxString m_value;
    wxString m_description;
    };

/** @}*/

#endif //__EDIT_TEXT_DIALOG_H__
