/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WXDIALOG_WITH_HELP_H__
#define __WXDIALOG_WITH_HELP_H__

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/filename.h>

/// @brief Namespace for user-interface items.
/// @details This includes dialogs for exporting, importing, printing, etc.
namespace Wisteria::UI
    {
    /** @brief Dialog with built-in support for help events.
        @details A path to the dialog's HTML help topic can be specified.*/
    class DialogWithHelp : public wxDialog
        {
    public:
        /** @brief Constructor.
            @param parent the parent of the dialog.
            @param id the window ID for this dialog.
            @param caption the title of this dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit DialogWithHelp(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& caption = wxEmptyString,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER) :
            wxDialog(parent, id, caption, pos, size, style)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);

            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogWithHelp::OnHelpClicked, this, wxID_HELP);
            Bind(wxEVT_HELP, &DialogWithHelp::OnContextHelp, this);

            Centre();
            }
        /// @brief Two-step Constructor (Create() should be called after construction).
        DialogWithHelp()
            {
            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogWithHelp::OnHelpClicked, this, wxID_HELP);
            Bind(wxEVT_HELP, &DialogWithHelp::OnContextHelp, this);
            }
        /// @private
        DialogWithHelp(const DialogWithHelp&) = delete;
        /// @private
        DialogWithHelp(DialogWithHelp&&) = delete;
        /// @private
        DialogWithHelp& operator=(const DialogWithHelp&) = delete;
        /// @private
        DialogWithHelp& operator=(DialogWithHelp&&) = delete;
        /** @brief Sets the help topic for the dialog.
            @param helpProjectDirectory The folder/base URL where the topics are stored.
            @param topicPath The path (after @c helpProjectDirectory) to the topic.*/
        void SetHelpTopic(const wxString& helpProjectDirectory, const wxString& topicPath)
            {
            m_helpProjectFolder = helpProjectDirectory;
            m_helpTopic = topicPath;
            }
    private:
        void OnHelpClicked([[maybe_unused]] wxCommandEvent& event)
            {
            if (m_helpTopic.length())
                { wxLaunchDefaultBrowser(wxFileName::FileNameToURL(m_helpProjectFolder + wxFileName::GetPathSeparator() + m_helpTopic)); }
            }
        void OnContextHelp([[maybe_unused]] wxHelpEvent& event)
            {
            wxCommandEvent cmd;
            OnHelpClicked(cmd);
            }
        wxString m_helpProjectFolder;
        wxString m_helpTopic;
        };
    }

/** @}*/

#endif //__WXDIALOG_WITH_HELP_H__
