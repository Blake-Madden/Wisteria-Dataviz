/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include "../../app/appsettings.h"
#include "../controls/sidebarbook.h"
#include "dialogwithhelp.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Dialog for editing the application's global settings.
    class OptionsDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style.*/
        explicit OptionsDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                            const wxString& caption = _(L"Options"),
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                         wxRESIZE_BORDER);

        /// @private
        OptionsDlg() = delete;
        /// @private
        OptionsDlg(const OptionsDlg&) = delete;
        /// @private
        OptionsDlg& operator=(const OptionsDlg&) = delete;

        /// @brief Pre-fills the dialog from the application settings.
        /// @param settings The settings to read values from.
        void LoadFromSettings(const AppSettings& settings);

        /// @brief Writes the dialog's values to the application settings.
        /// @param settings The settings to update.
        void ApplyToSettings(AppSettings& settings);

      private:
        constexpr static wxWindowID ID_PAGE_SETUP_SECTION{ wxID_HIGHEST + 1 };
        constexpr static wxWindowID ID_GENERAL_SECTION{ wxID_HIGHEST + 2 };

        void CreateControls();
        [[nodiscard]]
        wxPanel* CreatePageSetupPage(wxWindow* parent);
        [[nodiscard]]
        wxPanel* CreateGeneralPage(wxWindow* parent);

        SideBarBook* m_sideBarBook{ nullptr };
        wxChoice* m_paperChoice{ nullptr };
        wxChoice* m_duplexChoice{ nullptr };
        std::vector<wxPaperSize> m_paperIds;

        bool m_portrait{ true };
        bool m_landscape{ false };
        bool m_collate{ true };
        bool m_color{ true };
        bool m_logAutoRefresh{ false };
        bool m_logVerbose{ false };
        };
    } // namespace Wisteria::UI

/// @}

#endif // OPTIONS_DIALOG_H
