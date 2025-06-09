/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef IMAGEMERGE_DLG_H
#define IMAGEMERGE_DLG_H

#include "../controls/listctrlex.h"
#include "../controls/thumbnail.h"
#include "dialogwithhelp.h"
#include <vector>
#include <wx/filename.h>
#include <wx/infobar.h>
#include <wx/radiobox.h>
#include <wx/richmsgdlg.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog which displays images that can be merged into another one
        (vertically or horizontally).*/
    class ImageMergeDlg final : public Wisteria::UI::DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param imgPaths The images to merge.
            @param orientation Whether to stitch the images horizontally or vertically.
            @param id The window ID.
            @param caption The title of the dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit ImageMergeDlg(wxWindow* parent, const wxArrayString& imgPaths,
                               wxOrientation orientation, wxWindowID id = wxID_ANY,
                               const wxString& caption = _(L"Merge Images"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER)
            : m_orientRadio((orientation == wxOrientation::wxHORIZONTAL) ? 0 : 1)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
            Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

            CreateControls(imgPaths);
            Centre();
            }

        /// @private
        ImageMergeDlg(const ImageMergeDlg& that) = delete;
        /// @private
        ImageMergeDlg& operator=(const ImageMergeDlg& that) = delete;

        /// @returns The output path that the client provided.
        [[nodiscard]]
        const wxString& GetMergedFilePath() const noexcept
            {
            return m_mergedFilePath;
            }

      private:
        void CreateControls(const wxArrayString& imgPaths);

        void AdjustThumbnailsHorizontally();
        void AdjustThumbnailsVertically();

        wxFileName m_baseImagePath;
        int m_orientRadio{ 0 };

        wxStaticBoxSizer* m_horizontalThumbsSizer{ nullptr };
        wxStaticBoxSizer* m_verticalThumbsSizer{ nullptr };

        wxString m_mergedFilePath;
        };
    } // namespace Wisteria::UI

    /** @}*/

#endif // IMAGEMERGE_DLG_H
