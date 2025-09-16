/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_DIALOG_WITH_HELP_H
#define WISTERIA_DIALOG_WITH_HELP_H

#include "../../math/mathematics.h"
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/wx.h>

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
        explicit DialogWithHelp(wxWindow* parent, wxWindowID id = wxID_ANY,
                                const wxString& caption = wxEmptyString,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize,
                                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                             wxRESIZE_BORDER)
            : wxDialog(parent, id, caption, pos, size, style)
            {
            wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);

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
        DialogWithHelp& operator=(const DialogWithHelp&) = delete;

        /** @brief Sets the help topic for the dialog.
            @param helpProjectDirectory The folder/base URL where the topics are stored.
            @param topicPath The path (after @c helpProjectDirectory) to the topic.*/
        void SetHelpTopic(const wxString& helpProjectDirectory, const wxString& topicPath)
            {
            m_helpProjectFolder = helpProjectDirectory;
            m_helpTopic = topicPath;
            }

        /** @brief Fixes a wxBitmap from a wxBitmapBundle for use in wxGenericStaticBitmap on GTK.

            @details On wxGTK, wxGenericStaticBitmap has a quirk where power-of-2 bitmap sizes
            (e.g., 16x16, 32x32, 128x128) can be interpreted as stock icon sizes, causing
            them to be drawn at 16x16 regardless of the actual bitmap size.

            This function applies a 1-pixel downscale to power-of-2 sizes on GTK to bypass the
            stock icon shortcut. Non-power-of-2 sizes, and all sizes on non-GTK platforms,
            are returned unmodified.

            @param bundle The @c  wxBitmapBundle containing the bitmap(s) to display.
            @param size The intended display size of the bitmap. Must be fully specified
                        (width and height > 0).
            @return A wxBitmap suitable for use in @c wxGenericStaticBitmap.
                    Returns @c wxNullBitmap if size is invalid.
        */
        static wxBitmap FixStaticBitmapImage(const wxBitmapBundle& bundle, wxSize size)
            {
            wxASSERT_MSG(size.GetWidth() > 0 && size.GetHeight() > 0,
                         "FixStaticBitmapImage requires positive, fully-specified size!");

            if (size.GetWidth() <= 0 || size.GetHeight() <= 0)
                {
                wxLogError("FixStaticBitmapImage called with invalid size: %dx%d", size.GetWidth(),
                           size.GetHeight());
                return wxNullBitmap;
                }

#ifdef __WXGTK__
            if (!is_power_of_two(size.GetWidth()) || !is_power_of_two(size.GetHeight()))
                {
                // non-power-of-2 sizes are safe
                return bundle.GetBitmap(size);
                }

            // power-of-2 size: downscale by 1 pixel to avoid GTK 16x16 quirk
            wxBitmap bmp = bundle.GetBitmap(size);
            wxBitmap::Rescale(bmp, wxSize{ size.GetWidth() - 1, size.GetHeight() - 1 });

            return bmp;
#else
            // non-GTK: just return the requested size
            return bundle.GetBitmap(size);
#endif
            }

      private:
        void OnHelpClicked([[maybe_unused]] wxCommandEvent& event)
            {
            if (!m_helpTopic.empty())
                {
                wxLaunchDefaultBrowser(wxFileName::FileNameToURL(
                    m_helpProjectFolder + wxFileName::GetPathSeparator() + m_helpTopic));
                }
            }

        void OnContextHelp([[maybe_unused]] wxHelpEvent& event)
            {
            wxCommandEvent cmd;
            OnHelpClicked(cmd);
            }

        wxString m_helpProjectFolder;
        wxString m_helpTopic;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_DIALOG_WITH_HELP_H
