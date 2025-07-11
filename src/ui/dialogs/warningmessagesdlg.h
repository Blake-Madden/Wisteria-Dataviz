/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WARNING_MESSAGES_DLG_H
#define WARNING_MESSAGES_DLG_H

#include "dialogwithhelp.h"
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Dialog to show all queued messages currently in the global WarningManager.
    class WarningMessagesDlg final : public DialogWithHelp
        {
      public:
        /// @brief Default constructor (should be used in conjunction with Create()).
        WarningMessagesDlg() = default;

        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The window position.
            @param size The window size.
            @param style The window style.*/
        explicit WarningMessagesDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                                    const wxString& caption = _(L"Warnings & Prompts Display"),
                                    const wxPoint& pos = wxDefaultPosition,
                                    const wxSize& size = wxDefaultSize,
                                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN)
            {
            Create(parent, id, caption, pos, size, style);
            }

        /// @private
        WarningMessagesDlg(const WarningMessagesDlg&) = delete;
        /// @private
        WarningMessagesDlg& operator=(const WarningMessagesDlg&) = delete;

        /** @brief Creation, used in conjunction with empty constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The window position.
            @param size The window size.
            @param style The window style.
            @returns @c true if successfully created.*/
        bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"Warnings & Prompts Display"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
            DialogWithHelp::Create(parent, id, caption, pos, size, style);

            CreateControls();
            GetSizer()->Fit(this);
            GetSizer()->SetSizeHints(this);
            Centre();
            return true;
            }

      private:
        void CreateControls();
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WARNING_MESSAGES_DLG_H
