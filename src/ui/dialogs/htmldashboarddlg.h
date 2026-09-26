/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef HTML_DASHBOARD_DLG_H
#define HTML_DASHBOARD_DLG_H

#include "../../reporting/htmldashboardprintout.h"
#include "dialogwithhelp.h"
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Options dialog for exporting an HTML dashboard.
    class HtmlDashboardDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param themes The names of the available themes.
            @param options The options to pre-populate the dialog with.
                The @c m_filePath and @c m_css fields are ignored.
            @param theme The name of the theme to select initially.
            @param id The window ID.
            @param caption The title of the dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        HtmlDashboardDlg(wxWindow* parent, const wxArrayString& themes,
                         const Wisteria::HtmlDashboardOptions& options, const wxString& theme,
                         wxWindowID id = wxID_ANY,
                         const wxString& caption = _(L"HTML Export Options"),
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);

        /// @private
        HtmlDashboardDlg(const HtmlDashboardDlg&) = delete;
        /// @private
        HtmlDashboardDlg& operator=(const HtmlDashboardDlg&) = delete;

        /// @returns The dashboard title.
        [[nodiscard]]
        const wxString& GetDashboardTitle() const noexcept
            {
            return m_dashboardTitle;
            }

        /// @returns The name of the selected theme.
        [[nodiscard]]
        const wxString& GetTheme() const noexcept
            {
            return m_theme;
            }

        /// @returns The initial view.
        [[nodiscard]]
        Wisteria::HtmlDashboardOptions::View GetInitialView() const noexcept;

        /// @returns The initial light/dark mode.
        [[nodiscard]]
        Wisteria::HtmlDashboardOptions::ColorMode GetInitialColorMode() const noexcept;

        /// @returns Whether to include the Auto/Light/Dark toggle.
        [[nodiscard]]
        bool IncludeColorModeToggle() const noexcept
            {
            return m_includeColorModeToggle;
            }

        /// @returns Whether large numbers count up when a page is revealed.
        [[nodiscard]]
        bool CountUpNumbers() const noexcept
            {
            return m_countUpNumbers;
            }

        /// @returns The page size (in DIPs/pixels).
        [[nodiscard]]
        wxSize GetPageSize() const noexcept
            {
            return { m_pageWidth, m_pageHeight };
            }

      private:
        void CreateControls(const wxArrayString& themes);

        void OnOK([[maybe_unused]] wxCommandEvent& event)
            {
            TransferDataFromWindow();
            if (IsModal())
                {
                EndModal(wxID_OK);
                }
            else
                {
                Show(false);
                }
            }

        wxString m_dashboardTitle;
        wxString m_theme;
        // indices into the radio boxes (Atlas, Story and Auto, Light, Dark)
        int m_view{ 1 };
        int m_colorMode{ 0 };
        bool m_includeColorModeToggle{ true };
        bool m_countUpNumbers{ true };
        int m_pageWidth{ 1280 };
        int m_pageHeight{ 720 };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // HTML_DASHBOARD_DLG_H
