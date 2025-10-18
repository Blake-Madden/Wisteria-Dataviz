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

        /** @brief Fixes a wxBitmap from a wxBitmapBundle for use in wxStaticBitmap.

            @details On non-Windows platforms, wxStaticBitmap interprets power-of-2 bitmap sizes
            (e.g., 16x16, 32x32, 128x128) as stock icon sizes, causing them to be drawn at 16x16
            regardless of the actual bitmap size.

            This function applies a 1-pixel downscale to power-of-2 sizes to bypass the
            stock icon shortcut. Non-power-of-2 sizes, and all sizes on Windows,
            are returned unmodified.

            @param bundle The @c  wxBitmapBundle containing the bitmap(s) to display.
            @param size The intended display size of the bitmap. Must be fully specified
                        (width and height > 0). This should be in DIPs (logical units).
            @return A wxBitmap suitable for use in @c wxStaticBitmap.
                    Returns @c wxNullBitmap if size is invalid.
        */
        wxBitmap FixStaticBitmapImage(const wxBitmapBundle& bundle, wxSize size) const
            {
            wxASSERT_MSG(size.GetWidth() > 0 && size.GetHeight() > 0,
                         "FixStaticBitmapImage requires positive, fully-specified size!");

            if (size.GetWidth() <= 0 || size.GetHeight() <= 0)
                {
                wxLogError("FixStaticBitmapImage called with invalid size: %dx%d", size.GetWidth(),
                           size.GetHeight());
                return wxNullBitmap;
                }

#ifndef __WXMSW__
            // e.g. 2.0 on Retina, 1.25 on 125% MSW
            const double scaling = GetContentScaleFactor();

            size = wxSize{ static_cast<int>(std::lround(size.GetWidth() * scaling)),
                           static_cast<int>(std::lround(size.GetHeight() * scaling)) };

            if (!is_power_of_two(static_cast<uint32_t>(size.GetWidth())) ||
                !is_power_of_two(static_cast<uint32_t>(size.GetHeight())))
                {
                // non-power-of-2 sizes are safe
                return bundle.GetBitmap(size);
                }

            // power-of-2 size: downscale by 1 pixel to avoid GTK/Cocoa 16x16 quirk
            wxBitmap bmp = bundle.GetBitmap(size);
            const int scaledPixel{ static_cast<int>(1 * scaling) };
            wxBitmap::Rescale(
                bmp, wxSize{ size.GetWidth() - scaledPixel, size.GetHeight() - scaledPixel });
            bmp.SetScaleFactor(scaling);

            return bmp;
#else
            return bundle.GetBitmap(FromDIP(size));
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
