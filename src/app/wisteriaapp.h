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
#include "../ui/dialogs/listdlg.h"
#include "../wxStartPage/startpage.h"
#include "appsettings.h"
#include <map>
#include <vector>
#include <wx/artprov.h>
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
    int OnExit() override;

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

    /// @returns The log window.
    [[nodiscard]]
    Wisteria::UI::ListDlg* GetLogWindow() noexcept
        {
        return m_logWindow;
        }

    /// @brief Shows or hides the log report window.
    void OnViewLogReport();

    /// @returns The start page.
    [[nodiscard]]
    wxStartPage* GetStartPage() const noexcept
        {
        return m_startPage;
        }

    /// @brief Constructs the settings handler.
    /// @warning This must be called after `BaseApp::OnInit()` has been called.
    void CreateAppSettings() { m_appSettings = std::make_unique<AppSettings>(); }

    /// @returns The application settings.
    [[nodiscard]]
    std::unique_ptr<AppSettings>& GetAppSettings() noexcept
        {
        wxASSERT_MSG(m_appSettings, L"Call CreateAppSettings() to construct this first!");
        return m_appSettings;
        }

  private:
    void LoadInterface();
    void InitProjectSidebar();

    std::unique_ptr<AppSettings> m_appSettings{ nullptr };
    Wisteria::UI::ListDlg* m_logWindow{ nullptr };
    wxStartPage* m_startPage{ nullptr };
    std::vector<wxBitmapBundle> m_projectSideBarImageList;
    };

/// @brief Extended icon provider, which is connected to the
///     application's custom icons.
class WisteriaArtProvider final : public wxArtProvider
    {
  public:
    WisteriaArtProvider();

  protected:
    [[nodiscard]]
    wxBitmapBundle CreateBitmapBundle(const wxArtID& id, const wxArtClient& client,
                                      const wxSize& size) final;

  private:
    std::map<wxArtID, wxString> m_idFileMap;
    };

/// @brief Custom command IDs for the application.
constexpr wxWindowID ID_INSERT_DATASET{ wxID_HIGHEST + 1 };
constexpr wxWindowID ID_INSERT_PAGE{ wxID_HIGHEST + 2 };

// graph category dropdown buttons
constexpr wxWindowID ID_INSERT_GRAPH_BASIC{ wxID_HIGHEST + 3 };
constexpr wxWindowID ID_INSERT_GRAPH_BUSINESS{ wxID_HIGHEST + 4 };
constexpr wxWindowID ID_INSERT_GRAPH_STATISTICAL{ wxID_HIGHEST + 5 };
constexpr wxWindowID ID_INSERT_GRAPH_SURVEY{ wxID_HIGHEST + 6 };
constexpr wxWindowID ID_INSERT_GRAPH_EDUCATION{ wxID_HIGHEST + 7 };
constexpr wxWindowID ID_INSERT_GRAPH_SOCIAL{ wxID_HIGHEST + 8 };
constexpr wxWindowID ID_INSERT_GRAPH_SPORTS{ wxID_HIGHEST + 9 };

// graph button bar (for enabling/disabling)
constexpr wxWindowID ID_GRAPH_BUTTONBAR{ wxID_HIGHEST + 10 };

// Basic graphs
constexpr wxWindowID ID_NEW_BARCHART{ wxID_HIGHEST + 11 };
constexpr wxWindowID ID_NEW_PIECHART{ wxID_HIGHEST + 12 };
constexpr wxWindowID ID_NEW_LINEPLOT{ wxID_HIGHEST + 13 };
constexpr wxWindowID ID_NEW_TABLE{ wxID_HIGHEST + 14 };
constexpr wxWindowID ID_NEW_SANKEY_DIAGRAM{ wxID_HIGHEST + 15 };
constexpr wxWindowID ID_NEW_WAFFLE_CHART{ wxID_HIGHEST + 16 };
constexpr wxWindowID ID_NEW_WORD_CLOUD{ wxID_HIGHEST + 17 };

// Business graphs
constexpr wxWindowID ID_NEW_GANTT{ wxID_HIGHEST + 18 };
constexpr wxWindowID ID_NEW_CANDLESTICK{ wxID_HIGHEST + 19 };

// Statistical graphs
constexpr wxWindowID ID_NEW_HISTOGRAM{ wxID_HIGHEST + 20 };
constexpr wxWindowID ID_NEW_BOXPLOT{ wxID_HIGHEST + 21 };
constexpr wxWindowID ID_NEW_HEATMAP{ wxID_HIGHEST + 22 };
constexpr wxWindowID ID_NEW_SCATTERPLOT{ wxID_HIGHEST + 23 };
constexpr wxWindowID ID_NEW_BUBBLEPLOT{ wxID_HIGHEST + 24 };
constexpr wxWindowID ID_NEW_CHERNOFFPLOT{ wxID_HIGHEST + 25 };
constexpr wxWindowID ID_NEW_STEMANDLEAF{ wxID_HIGHEST + 26 };

// Survey graphs
constexpr wxWindowID ID_NEW_LIKERT{ wxID_HIGHEST + 27 };
constexpr wxWindowID ID_NEW_PROCON_ROADMAP{ wxID_HIGHEST + 28 };

// Education graphs
constexpr wxWindowID ID_NEW_SCALE_CHART{ wxID_HIGHEST + 29 };

// Social Sciences graphs
constexpr wxWindowID ID_NEW_WCURVE{ wxID_HIGHEST + 30 };
constexpr wxWindowID ID_NEW_LR_ROADMAP{ wxID_HIGHEST + 31 };

// Sports graphs
constexpr wxWindowID ID_NEW_WIN_LOSS_SPARKLINE{ wxID_HIGHEST + 32 };

// Pages
constexpr wxWindowID ID_EDIT_PAGE{ wxID_HIGHEST + 34 };
constexpr wxWindowID ID_PAGES_BUTTONBAR{ wxID_HIGHEST + 35 };

// Tools
constexpr wxWindowID ID_VIEW_LOG_REPORT{ wxID_HIGHEST + 33 };

wxDECLARE_APP(WisteriaApp);

#endif // WISTERIA_APP_H
