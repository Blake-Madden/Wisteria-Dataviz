/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __RADIOCHOICE_DIALOG_H__
#define __RADIOCHOICE_DIALOG_H__

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/valgen.h>
#include <wx/html/htmlwin.h>
#include <wx/htmllbox.h>
#include <wx/filename.h>
#include <wx/bannerwindow.h>
#include "dialogwithhelp.h"
#include "../../math/safe_math.h"

namespace Wisteria::UI
    {
    /// @brief A dialog with a banner and series of radio buttons to choose from.
    /// @details (Descriptions can also be included for each option.)
    ///     This dialog is similar to @c wxSingleChoiceDialog, but uses radio buttons
    ///     instead of a list.
    class RadioBoxDlg final : public DialogWithHelp
        {
    public:
        /** @brief Constructor.
            @param parent The parent window.
            @param bannerLabel A label to show at the top of the dialog.
            @param bannerDescription A label to show under the banner's title.
            @param optionsLabel Descriptive label shown above the radio buttons.
            @param caption The title of the export dialog.
            @param choices The choices for the radio buttons.
            @param descriptions Descriptions for the choices.
            @param showHelpButton @c true to show a help button.
            @param id The window ID.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        RadioBoxDlg(wxWindow* parent,
                 const wxString& bannerLabel,
                 const wxString& bannerDescription,
                 const wxString& optionsLabel,
                 const wxString& caption,
                 const wxArrayString& choices, const wxArrayString& descriptions,
                 const bool showHelpButton = false,
                 wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE) :
                 m_choices(choices),
                 m_descriptions(descriptions), m_bannerLabel(bannerLabel),
                 m_bannerDescription(bannerDescription), m_optionsLabel(optionsLabel)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
            DialogWithHelp::Create(parent, id, caption, pos, size, style);

            CreateControls(showHelpButton);
            GetSizer()->SetSizeHints(this);
            Centre();

            Bind(wxEVT_RADIOBOX, &RadioBoxDlg::OnRadioBoxChange, this);
            }
        /// @private
        RadioBoxDlg() = delete;
        /// @private
        RadioBoxDlg(const RadioBoxDlg&) = delete;
        /// @private
        RadioBoxDlg(RadioBoxDlg&&) = delete;
        /// @private
        RadioBoxDlg& operator=(const RadioBoxDlg&) = delete;
        /// @private
        RadioBoxDlg& operator=(RadioBoxDlg&&) = delete;
        /// @returns The selected item.
        [[nodiscard]]
        int GetSelection() const noexcept
            { return m_selected; }
        /** @brief Sets the selected item (i.e., radio button index).
            @param selected The radio button to select.*/
        void SetSelection(const int selected)
            {
            wxASSERT(m_choices.GetCount());
            wxASSERT(selected >= 0);
            if (selected < 0)
                { m_selected = 0; }
            else if (selected >= static_cast<int>(m_choices.GetCount()))
                { m_selected = static_cast<int>(m_choices.GetCount())-1; }
            else
                { m_selected = selected; }
            TransferDataToWindow();
            wxCommandEvent cmd;
            OnRadioBoxChange(cmd);
            }
    private:
        /// Creates the controls and sizers.
        void CreateControls(const bool showHelpButton);
        void OnRadioBoxChange([[maybe_unused]] wxCommandEvent& event);

        wxArrayString m_choices;
        wxArrayString m_descriptions;
        wxString m_bannerLabel;
        wxString m_bannerDescription;
        wxString m_optionsLabel;
        int m_selected{ 0 };

        wxHtmlWindow* m_descriptionLabel{ nullptr };

        wxString m_helpProjectFolder;
        wxString m_helpTopic;
        };
    }

/** @}*/

#endif //__RADIOCHOICE_DIALOG_H__
