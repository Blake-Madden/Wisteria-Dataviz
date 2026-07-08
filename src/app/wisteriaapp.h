///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaapp.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_APP_H
#define WISTERIA_APP_H

#include "../base/enums.h"
#include "../base/version.h"
#include "../ui/app.h"
#include "../ui/controls/listctrlex.h"
#include "../ui/controls/sidebar.h"
#include "../ui/mainframe.h"
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

namespace Wisteria::GraphItems
    {
    class GraphItemBase;
    class Label;
    } // namespace Wisteria::GraphItems

class WisteriaApp;

/// @brief Main frame with an embedded log tab.
class MainFrame final : public Wisteria::UI::BaseMainFrame
    {
  public:
    using Wisteria::UI::BaseMainFrame::BaseMainFrame;

    /// @returns The Log ribbon tab page.
    [[nodiscard]]
    wxRibbonPage* GetLogRibbonPage() const noexcept
        {
        return m_logRibbonPage;
        }

    /// @returns @c true if the Log ribbon tab is currently the active page.
    [[nodiscard]]
    bool IsLogTabActive() const noexcept
        {
        return (GetRibbon() != nullptr && m_logRibbonPage != nullptr &&
                GetRibbon()->GetActivePage() == GetRibbon()->GetPageNumber(m_logRibbonPage));
        }

    /// @brief Show main frame, switch to the Log tab, and refresh the log list.
    void ActivateLogTab();

    /// @brief Enables or disables log auto-refresh, syncing the ribbon button and timer.
    void SetLogAutoRefresh(bool enable);

    friend class WisteriaApp;

  private:
    wxPanel* m_logPanel{ nullptr };
    Wisteria::UI::ListCtrlEx* m_logListCtrl{ nullptr };
    std::shared_ptr<Wisteria::UI::ListCtrlExDataProvider> m_logDataProvider;
    wxTimer m_logAutoRefreshTimer;
    bool m_logAutoRefresh{ false };
    wxRibbonButtonBar* m_logEditButtonBar{ nullptr };
    wxRibbonPage* m_logRibbonPage{ nullptr };

    wxDECLARE_CLASS(MainFrame);
    };

/// @brief The Wisteria Dataviz application.
class WisteriaApp final : public Wisteria::UI::BaseApp
    {
  public:
    WisteriaApp() = default;

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

    /// @returns The main frame cast to the custom MainFrame type.
    [[nodiscard]]
    MainFrame* GetMainFrameEx() noexcept
        {
        return dynamic_cast<MainFrame*>(GetMainFrame());
        }

    /// @brief Reads the log file into the given list control.
    void ReadLogIntoListCtrl(Wisteria::UI::ListCtrlEx* listCtrl);

    /// @brief Returns the SVG icon filename for a canvas item,
    ///     based on its RTTI type.
    /// @param item The canvas item to look up.
    /// @returns The SVG filename (e.g., @c "barchart.svg"), or an empty
    ///     string if the type is not recognized.
    [[nodiscard]]
    static wxString GetItemIconName(const Wisteria::GraphItems::GraphItemBase* item);

    /// @returns Which kind of spacer @p label matches, or Wisteria::SpacerType::NotSpacer
    ///     if it's a regular label (e.g., has text, or is a visible empty-text
    ///     label used for a divider; see ReportBuilder::LoadSpacer()/
    ///     LoadEmptySpacer()).
    /// @param label The label to inspect.
    [[nodiscard]]
    static Wisteria::SpacerType GetSpacerType(const Wisteria::GraphItems::Label& label);

    /// @returns Which kind of divider line @p label matches, or
    ///     Wisteria::DividerType::NotDivider if it's a regular label.
    /// @param label The label to inspect.
    [[nodiscard]]
    static Wisteria::DividerType GetDividerType(const Wisteria::GraphItems::Label& label);

    /// @brief Returns the serializable type name for a graph,
    ///     based on its RTTI type.
    /// @param graph The graph type to look up.
    /// @returns The type string for the graph, or an empty
    ///     string if the type is not recognized.
    [[nodiscard]]
    static wxString GetGraphTypeString(const Wisteria::Graphs::Graph2D* graph);

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
    bool OnInit() override;
    int OnExit() override;
    void LoadInterface();
    void InitProjectSidebar();
    void LoadRibbonLogPage(wxRibbonBar* ribbon);

    std::unique_ptr<AppSettings> m_appSettings{ nullptr };
    wxStartPage* m_startPage{ nullptr };
    std::vector<wxBitmapBundle> m_projectSideBarImageList;
    };

/// @brief Extended icon provider, which is connected to the
///     application's custom icons.
class WisteriaArtProvider final : public wxArtProvider
    {
  public:
    /// @brief Constructs the custom art provider and registers it with wxWidgets.
    WisteriaArtProvider();

  protected:
    /// @brief Creates a bitmap bundle for the given art ID.
    /// @param id The art ID to look up.
    /// @param client The art client requesting the bundle.
    /// @param size The preferred size of the bitmap.
    /// @returns The bitmap bundle for the given art ID.
    [[nodiscard]]
    wxBitmapBundle CreateBitmapBundle(const wxArtID& id, const wxArtClient& client,
                                      const wxSize& size) final;

  private:
    std::map<wxArtID, wxString> m_idFileMap;
    };

/// @brief Custom command IDs for the application.
constexpr wxWindowID ID_INSERT_DATASET{ wxID_HIGHEST + 1 };
constexpr wxWindowID ID_INSERT_PAGE{ wxID_HIGHEST + 2 };

constexpr wxWindowID ID_DATASET_BUTTONBAR{ wxID_HIGHEST + 56 };

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
constexpr wxWindowID ID_DELETE_PAGE{ wxID_HIGHEST + 49 };
constexpr wxWindowID ID_PAGES_BUTTONBAR{ wxID_HIGHEST + 35 };

// Data transformations
constexpr wxWindowID ID_EDIT_DATASET{ wxID_HIGHEST + 50 };
constexpr wxWindowID ID_PIVOT_WIDER{ wxID_HIGHEST + 36 };
constexpr wxWindowID ID_PIVOT_LONGER{ wxID_HIGHEST + 37 };
constexpr wxWindowID ID_SUBSET_DATASET{ wxID_HIGHEST + 51 };
constexpr wxWindowID ID_JOIN_DATASET{ wxID_HIGHEST + 54 };

// Project
constexpr wxWindowID ID_SAVE_PROJECT{ wxID_HIGHEST + 38 };
constexpr wxWindowID ID_SAVE_PROJECT_AS{ wxID_HIGHEST + 41 };

// Item editing
constexpr wxWindowID ID_EDIT_ITEM{ wxID_HIGHEST + 39 };
constexpr wxWindowID ID_DELETE_ITEM{ wxID_HIGHEST + 40 };

// Objects (labels, images, shapes, common axes)
constexpr wxWindowID ID_NEW_LABEL{ wxID_HIGHEST + 42 };
constexpr wxWindowID ID_OBJECTS_BUTTONBAR{ wxID_HIGHEST + 43 };
constexpr wxWindowID ID_NEW_IMAGE{ wxID_HIGHEST + 44 };
constexpr wxWindowID ID_NEW_SHAPE{ wxID_HIGHEST + 45 };
constexpr wxWindowID ID_NEW_COMMON_AXIS{ wxID_HIGHEST + 55 };
constexpr wxWindowID ID_NEW_SPACER{ wxID_HIGHEST + 69 };
constexpr wxWindowID ID_NEW_EMPTY_SPACER{ wxID_HIGHEST + 70 };
constexpr wxWindowID ID_NEW_DIVIDER{ wxID_HIGHEST + 71 };
constexpr wxWindowID ID_NEW_DIVIDER_HORIZONTAL_SINGLE{ wxID_HIGHEST + 72 };
constexpr wxWindowID ID_NEW_DIVIDER_HORIZONTAL_DOUBLE{ wxID_HIGHEST + 73 };
constexpr wxWindowID ID_NEW_DIVIDER_VERTICAL_SINGLE{ wxID_HIGHEST + 74 };
constexpr wxWindowID ID_NEW_DIVIDER_VERTICAL_DOUBLE{ wxID_HIGHEST + 75 };

// Multi-series graphs
constexpr wxWindowID ID_NEW_MULTI_SERIES_LINEPLOT{ wxID_HIGHEST + 46 };

// Constants
constexpr wxWindowID ID_ADD_CONSTANT{ wxID_HIGHEST + 47 };
constexpr wxWindowID ID_DELETE_CONSTANT{ wxID_HIGHEST + 48 };

// Dataset deletion
constexpr wxWindowID ID_DELETE_DATASET{ wxID_HIGHEST + 53 };

// Export
constexpr wxWindowID ID_SVG_EXPORT{ wxID_HIGHEST + 52 };
constexpr wxWindowID ID_PDF_EXPORT{ wxID_HIGHEST + 57 };
constexpr wxWindowID ID_PRINT_SETUP{ wxID_HIGHEST + 58 };
constexpr wxWindowID ID_PROJECT_SETTINGS{ wxID_HIGHEST + 59 };

// Tools
constexpr wxWindowID ID_VIEW_LOG_REPORT{ wxID_HIGHEST + 33 };

// Log tab ribbon buttons
constexpr wxWindowID ID_LOG_TAB_SAVE{ wxID_HIGHEST + 60 };
constexpr wxWindowID ID_LOG_TAB_PRINT{ wxID_HIGHEST + 61 };
constexpr wxWindowID ID_LOG_TAB_COPY{ wxID_HIGHEST + 62 };
constexpr wxWindowID ID_LOG_TAB_SELECT_ALL{ wxID_HIGHEST + 63 };
constexpr wxWindowID ID_LOG_TAB_SORT{ wxID_HIGHEST + 64 };
constexpr wxWindowID ID_LOG_TAB_CLEAR{ wxID_HIGHEST + 65 };
constexpr wxWindowID ID_LOG_TAB_REFRESH{ wxID_HIGHEST + 66 };
constexpr wxWindowID ID_LOG_TAB_REALTIME_UPDATE{ wxID_HIGHEST + 67 };
constexpr wxWindowID ID_LOG_TAB_VERBOSE{ wxID_HIGHEST + 68 };

wxDECLARE_APP(WisteriaApp);

#endif // WISTERIA_APP_H
