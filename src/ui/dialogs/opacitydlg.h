/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef OPACITY_DLG_H
#define OPACITY_DLG_H

#include "../controls/thumbnail.h"
#include <wx/dialog.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    class OpacityDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param parent The dialog's parent.
            @param opacity The starting opacity.
            @param image The image whose opacity you are changing.
            @param id The dialog's ID.
            @param caption The dialog's title.
            @param pos The dialog's screen position.
            @param size The dialog's initial size.
            @param style The dialog's flags.*/
        OpacityDlg(wxWindow* parent, uint8_t opacity, wxBitmap image, wxWindowID id = wxID_ANY,
                   const wxString& caption = _(L"Set Opacity"),
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                   long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);
        /// @private
        OpacityDlg() = delete;
        /// @private
        OpacityDlg(const OpacityDlg&) = delete;
        /// @private
        OpacityDlg& operator=(const OpacityDlg&) = delete;

        /// @returns The selected opacity.
        [[nodiscard]]
        int GetOpacity() const noexcept
            {
            return m_opacity;
            }

      private:
        void CreateControls();
        void OnChangeOpacity(const wxScrollEvent& event);

        Thumbnail* m_thumb{ nullptr };
        int m_opacity{ wxALPHA_OPAQUE }; // validator needs an int, not a uint8_t
        wxBitmap m_image;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // OPACITY_DLG_H
