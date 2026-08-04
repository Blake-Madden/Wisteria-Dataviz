/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef IMAGEEFFECT_DLG_H
#define IMAGEEFFECT_DLG_H

#include "../controls/thumbnail.h"
#include "dialogwithhelp.h"
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/sizer.h>
#include <wx/string.h>

namespace Wisteria::UI
    {
    /** @brief Dialog which previews an effect (e.g., color balance, sepia)
            being applied to an image, then saves the result to a new file.*/
    class ImageEffectDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param imgPath The image to apply an effect to.
            @param id The window ID.
            @param caption The title of the dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit ImageEffectDlg(wxWindow* parent, const wxString& imgPath, wxWindowID id = wxID_ANY,
                                const wxString& caption = _(L"Apply Image Effect"),
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize,
                                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                             wxRESIZE_BORDER)
            {
            wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS |
                                            wxWS_EX_CONTEXTHELP);
            DialogWithHelp::Create(parent, id, caption, pos, size, style);

            CreateControls(imgPath);
            Centre();
            }

        /// @private
        ImageEffectDlg(const ImageEffectDlg& that) = delete;
        /// @private
        ImageEffectDlg& operator=(const ImageEffectDlg& that) = delete;

        /// @returns The output path that the client saved the edited image to
        ///     (will be empty if the dialog was cancelled).
        [[nodiscard]]
        const wxString& GetEffectFilePath() const noexcept
            {
            return m_effectFilePath;
            }

      private:
        void CreateControls(const wxString& imgPath);
        void UpdatePreview();

        wxFileName m_baseImagePath;
        wxImage m_originalImage;
        int m_imageEffect{ 0 };

        Thumbnail* m_thumbnail{ nullptr };

        wxString m_effectFilePath;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // IMAGEEFFECT_DLG_H
