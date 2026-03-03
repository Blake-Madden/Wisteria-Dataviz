///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaapp.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_APP_H
#define WISTERIA_APP_H

#include "../base/version.h"
#include "../ui/app.h"
#include "../ui/controls/sidebar.h"
#include <vector>
#include <wx/ribbon/art.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/splitter.h>
#include <wx/wx.h>

/// @brief The Wisteria Dataviz application.
class WisteriaApp final : public Wisteria::UI::BaseApp
    {
  public:
    WisteriaApp() = default;

    bool OnInit() override;

    /// @brief Creates the ribbon bar for a given parent window.
    /// @param parent The parent window.
    /// @param doc The document associated with the ribbon, or @c nullptr
    ///     for the main frame ribbon.
    /// @returns The ribbon bar.
    wxRibbonBar* CreateRibbon(wxWindow* parent, const wxDocument* doc = nullptr);

    /// @returns The image list for project sidebars.
    [[nodiscard]]
    std::vector<wxBitmapBundle>& GetProjectSideBarImageList() noexcept
        {
        return m_projectSideBarImageList;
        }

  private:
    void LoadInterface();
    void InitProjectSidebar();

    std::vector<wxBitmapBundle> m_projectSideBarImageList;
    };

wxDECLARE_APP(WisteriaApp);

#endif // WISTERIA_APP_H
