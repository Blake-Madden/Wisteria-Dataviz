///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaapp.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaapp.h"
#include "wisteriadoc.h"
#include "wisteriaview.h"
#include <wx/aboutdlg.h>
#include <wx/stdpaths.h>

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast,cppcoreguidelines-avoid-non-const-global-variables)
wxIMPLEMENT_APP(WisteriaApp);

//-------------------------------------------
WisteriaArtProvider::WisteriaArtProvider()
    {
    // cppcheck-suppress useInitializationList
    m_idFileMap = { { L"ID_CONTINUOUS", L"scale.svg" },
                    { L"ID_CATEGORICAL", L"categorical.svg" },
                    { L"ID_DISCRETE", L"discrete.svg" },
                    { L"ID_DATE", L"date.svg" },
                    { L"ID_DICHOTOMOUS_CATEGORICAL", L"dichotomous-categorical.svg" },
                    { L"ID_DICHOTOMOUS_DISCRETE", L"dichotomous-discrete.svg" },
                    { wxART_NEW, L"wisteria.svg" },
                    { wxART_FILE_OPEN, L"file-open.svg" },
                    { wxART_FILE_SAVE, L"file-save.svg" },
                    { wxART_PRINT, L"print.svg" },
                    { wxART_COPY, L"copy.svg" },
                    { wxART_FIND, L"find.svg" },
                    { wxART_DELETE, L"delete.svg" },
                    { L"ID_SELECT_ALL", L"select-all.svg" },
                    { L"ID_LIST_SORT", L"sort.svg" },
                    { L"ID_CLEAR", L"clear.svg" },
                    { L"ID_REFRESH", L"reload.svg" },
                    { L"ID_REALTIME_UPDATE", L"realtime.svg" },
                    { wxART_EDIT, L"edit.svg" } };
    }

//-------------------------------------------
wxBitmapBundle WisteriaArtProvider::CreateBitmapBundle(const wxArtID& id, const wxArtClient& client,
                                                       const wxSize& size)
    {
    const auto filePath = m_idFileMap.find(id);
    return (filePath != m_idFileMap.cend()) ?
               wxGetApp().GetResourceManager().GetSVG(filePath->second) :
               wxArtProvider::CreateBitmapBundle(id, client, size);
    }

//-------------------------------------------
bool WisteriaApp::OnInit()
    {
    SetAppName(_WISTERIA_APP_NAME);
    SetVendorName(L"Blake Madden");

#ifdef __WXMSW__
    MSWEnableDarkMode();
#endif

    if (!BaseApp::OnInit())
        {
        return false;
        }

    CreateAppSettings();

    // load settings from the user's app data folder
    wxString appSettingFolderPath =
        wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator();
    if (!wxFileName::DirExists(appSettingFolderPath))
        {
        wxFileName::Mkdir(appSettingFolderPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        }
    GetAppSettings()->LoadSettingsFile(appSettingFolderPath + L"Settings.xml");

    GetResourceManager().LoadArchive(FindResourceFile(L"res.wad"));
    wxArtProvider::Push(new WisteriaArtProvider{});

    // create the document template
    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    [[maybe_unused]]
    auto* docTemplate =
        new wxDocTemplate(GetDocManager(), _(L"Wisteria project"), L"*.json", wxString{}, L"json",
                          _DT(L"Wisteria Doc"), L"WisteriaView", wxCLASSINFO(WisteriaDoc),
                          wxCLASSINFO(WisteriaView));
    SetAppFileExtension(L"json");

        // load MRU file history before building the start page
        {
        wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
        config.SetPath(DONTTRANSLATE(L"Recent File List", DTExplanation::SystemEntry));
        GetDocManager()->FileHistoryLoad(config);
        }

    LoadInterface();
    InitProjectSidebar();

    return true;
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    }

//-------------------------------------------
void WisteriaApp::LoadInterface()
    {
    const wxArrayString extensions{ GetAppFileExtension() };

    SetMainFrame(new Wisteria::UI::BaseMainFrame(
        GetDocManager(), nullptr, extensions, _WISTERIA_APP_NAME, wxPoint{ 0, 0 },
        GetAppSettings()->GetAppWindowSize(), wxDEFAULT_FRAME_STYLE));

    GetMainFrame()->InitControls(CreateRibbon(GetMainFrame()));

    // add start page
    wxArrayString mruFiles;
    for (size_t i = 0; i < GetDocManager()->GetFileHistory()->GetCount(); ++i)
        {
        mruFiles.Add(GetDocManager()->GetFileHistory()->GetHistoryFile(i));
        }
    m_startPage = new wxStartPage(GetMainFrame(), wxID_ANY, mruFiles,
                                  GetResourceManager().GetSVG(L"wisteria.svg"));
    m_startPage->AddButton(wxArtProvider::GetBitmapBundle(wxART_NEW, wxART_BUTTON),
                           _(L"Create a New Project"));
    m_startPage->AddButton(wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON),
                           _(L"Open a Project"));
    GetMainFrame()->GetSizer()->Add(m_startPage, wxSizerFlags{ 1 }.Expand());

    GetMainFrame()->Bind(wxEVT_STARTPAGE_CLICKED,
                         [this](const wxCommandEvent& event)
                         {
                             if (m_startPage->IsCustomButtonId(event.GetId()))
                                 {
                                 if (event.GetId() == m_startPage->GetButtonID(0))
                                     {
                                     GetDocManager()->CreateNewDocument();
                                     }
                                 else if (event.GetId() == m_startPage->GetButtonID(1))
                                     {
                                     wxCommandEvent openEvent(wxEVT_MENU, wxID_OPEN);
                                     GetMainFrame()->ProcessWindowEvent(openEvent);
                                     }
                                 }
                             else if (wxStartPage::IsFileId(event.GetId()))
                                 {
                                 GetDocManager()->CreateDocument(event.GetString(), wxDOC_SILENT);
                                 }
                             else if (wxStartPage::IsFileListClearId(event.GetId()))
                                 {
                                 ClearFileHistoryMenu();
                                 }
                         });

    wxIcon appIcon;
    const auto appSvg = GetResourceManager().GetSVG(L"wisteria.svg");
    if (appSvg.IsOk())
        {
        appIcon.CopyFromBitmap(appSvg.GetBitmap(GetMainFrame()->FromDIP(wxSize{ 32, 32 })));
        GetMainFrame()->SetIcon(appIcon);
        GetMainFrame()->SetLogo(appSvg);
        }

    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxRibbonButtonBarEvent& event) { OnViewLogReport(); },
        ID_VIEW_LOG_REPORT);

    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event)
        {
            wxAboutDialogInfo aboutInfo;
            aboutInfo.SetCopyright(_(L"Copyright (c) 2005-2026 Blake Madden"));
            wxArrayString devs;
            devs.Add(_DT(L"Blake Madden"));
            aboutInfo.SetDevelopers(devs);
            aboutInfo.SetName(_WISTERIA_APP_NAME);
            aboutInfo.SetDescription(_(L"Data visualization application."));
            wxAboutBox(aboutInfo, GetMainFrame());
        },
        wxID_ABOUT);

    // capture window state before the frame is destroyed
    GetMainFrame()->Bind(
        wxEVT_CLOSE_WINDOW,
        [this](wxCloseEvent& event)
        {
            if (m_logWindow != nullptr)
                {
                m_logWindow->Destroy();
                m_logWindow = nullptr;
                }
            GetAppSettings()->SetAppWindowMaximized(GetMainFrame()->IsMaximized());
            GetAppSettings()->SetAppWindowWidth(GetMainFrame()->GetSize().GetWidth());
            GetAppSettings()->SetAppWindowHeight(GetMainFrame()->GetSize().GetHeight());
            event.Skip();
        });

    GetMainFrame()->CenterOnScreen();
    if (GetAppSettings()->IsAppWindowMaximized())
        {
        GetMainFrame()->Maximize();
        GetMainFrame()->SetSize(GetMainFrame()->GetSize());
        }
    GetMainFrame()->Show(true);
    }

//-------------------------------------------
void WisteriaApp::OnViewLogReport()
    {
    if (m_logWindow != nullptr && m_logWindow->IsShown())
        {
        m_logWindow->Hide();
        return;
        }

    if (m_logWindow == nullptr)
        {
        const wxSize screenSize{ wxSystemSettings::GetMetric(wxSystemMetric::wxSYS_SCREEN_X),
                                 wxSystemSettings::GetMetric(wxSystemMetric::wxSYS_SCREEN_Y) };
        m_logWindow = new Wisteria::UI::ListDlg(
            nullptr, wxNullColour, wxNullColour, wxNullColour,
            Wisteria::UI::LD_SAVE_BUTTON | Wisteria::UI::LD_COPY_BUTTON |
                Wisteria::UI::LD_PRINT_BUTTON | Wisteria::UI::LD_SELECT_ALL_BUTTON |
                Wisteria::UI::LD_FIND_BUTTON | Wisteria::UI::LD_COLUMN_HEADERS |
                Wisteria::UI::LD_SORT_BUTTON | Wisteria::UI::LD_CLEAR_BUTTON |
                Wisteria::UI::LD_REFRESH_BUTTON | Wisteria::UI::LD_LOG_VERBOSE_BUTTON,
            wxID_ANY, _(L"Log Report"), wxString{}, wxDefaultPosition,
            wxSize{ screenSize.GetWidth() / 2, screenSize.GetHeight() / 2 });

        // move over to the right side of the screen
        const int screenWidth{ wxSystemSettings::GetMetric(wxSystemMetric::wxSYS_SCREEN_X) };
        int xPos{ 0 }, yPos{ 0 };
        m_logWindow->GetScreenPosition(&xPos, &yPos);
        m_logWindow->Move(
            wxPoint{ xPos + (screenWidth - (xPos + m_logWindow->GetSize().GetWidth())), yPos });
        }
    m_logWindow->SetActiveLog(GetLogFile());
    m_logWindow->ReadLog();

    m_logWindow->Show();
    m_logWindow->GetListCtrl()->DistributeColumns(-1);
    m_logWindow->SetFocus();
    }

//-------------------------------------------
int WisteriaApp::OnExit()
    {
    if (m_appSettings != nullptr)
        {
        GetAppSettings()->SaveSettingsFile();
        }

    return BaseApp::OnExit();
    }

//-------------------------------------------
wxRibbonBar* WisteriaApp::CreateRibbon(wxWindow* parent, const wxDocument* doc)
    {
    const bool isProjectRibbon = (doc != nullptr);

    auto* ribbon = new wxRibbonBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxRIBBON_BAR_SHOW_PAGE_ICONS | wxRIBBON_BAR_DEFAULT_STYLE);

    // Home tab
    const auto homeIcon = ReadSvgIcon(
        wxSystemSettings::GetAppearance().IsDark() ? L"home-dark-mode.svg" : L"home.svg",
        wxSize{ 16, 16 });
    auto* homePage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Home"), homeIcon);

    // Project panel with New and Open buttons
    auto* projectPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Project"));
    auto* projectButtonBar = new wxRibbonButtonBar(projectPanel, wxID_ANY);

    projectButtonBar->AddButton(wxID_NEW, _(L"New"), ReadSvgIcon(L"wisteria.svg"),
                                _(L"Create a new project"));

    projectButtonBar->AddButton(wxID_OPEN, _(L"Open"), ReadSvgIcon(L"file-open.svg"),
                                _(L"Open a data file"));

    if (isProjectRibbon)
        {
        projectButtonBar->AddHybridButton(ID_SAVE_PROJECT, _(L"Save"),
                                          ReadSvgIcon(L"file-save.svg"), _(L"Save the project"));
        projectButtonBar->AddButton(ID_SVG_EXPORT, _(L"SVG Export"), ReadSvgIcon(L"report.svg"),
                                    _(L"Export all pages to SVG"));

        // Print panel
        auto* printPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Print"));
        auto* printButtonBar = new wxRibbonButtonBar(printPanel, wxID_ANY);
        printButtonBar->AddButton(wxID_PRINT, _(L"Print"), ReadSvgIcon(L"print.svg"),
                                  _(L"Print all pages"));

        // Pages panel
        auto* pagesPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Pages"));
        auto* pagesButtonBar = new wxRibbonButtonBar(pagesPanel, ID_PAGES_BUTTONBAR);

        pagesButtonBar->AddButton(ID_INSERT_PAGE, _(L"Add"), ReadSvgIcon(L"page-add.svg"),
                                  _(L"Add a new page to the project"));
        pagesButtonBar->AddButton(ID_EDIT_PAGE, _(L"Edit"), ReadSvgIcon(L"page-edit.svg"),
                                  _(L"Edit the current page"));
        pagesButtonBar->AddButton(ID_DELETE_PAGE, _(L"Delete"), ReadSvgIcon(L"page-delete.svg"),
                                  _(L"Delete the current page"));

        // Objects panel (labels, images, shapes)
        auto* objectsPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Objects"));
        auto* objectsButtonBar = new wxRibbonButtonBar(objectsPanel, ID_OBJECTS_BUTTONBAR);

        objectsButtonBar->AddButton(ID_NEW_LABEL, _(L"Label"), ReadSvgIcon(L"label.svg"),
                                    _(L"Insert a text label"));
        objectsButtonBar->AddButton(ID_NEW_IMAGE, _(L"Image"), ReadSvgIcon(L"image.svg"),
                                    _(L"Insert an image"));
        objectsButtonBar->AddButton(ID_NEW_SHAPE, _(L"Shape"), ReadSvgIcon(L"shape.svg"),
                                    _(L"Insert a shape"));
        objectsButtonBar->AddButton(ID_EDIT_ITEM, _(L"Edit"), ReadSvgIcon(L"edit.svg"),
                                    _(L"Edit the selected item"));
        objectsButtonBar->AddButton(ID_DELETE_ITEM, _(L"Delete"), ReadSvgIcon(L"delete.svg"),
                                    _(L"Delete the selected item"));

        // Data tab
        auto* dataPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Data"));

        // Datasets panel
        auto* dataPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Datasets"));
        auto* dataButtonBar = new wxRibbonButtonBar(dataPanel, wxID_ANY);

        dataButtonBar->AddButton(ID_INSERT_DATASET, _(L"Add"), ReadSvgIcon(L"data-add.svg"),
                                 _(L"Import a dataset into the project"));
        dataButtonBar->AddButton(ID_EDIT_DATASET, _(L"Edit"), ReadSvgIcon(L"data-edit.svg"),
                                 _(L"Edit the selected dataset's import options"));
        dataButtonBar->AddButton(ID_DELETE_DATASET, _(L"Delete"), ReadSvgIcon(L"data-delete.svg"),
                                 _(L"Delete the selected dataset from the project"));

        // Transformations panel
        auto* transformPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Transformations"));
        auto* transformButtonBar = new wxRibbonButtonBar(transformPanel, wxID_ANY);

        transformButtonBar->AddButton(ID_PIVOT_WIDER, _(L"Pivot Wider"),
                                      ReadSvgIcon(L"pivot-wider.svg"),
                                      _(L"Pivot a dataset wider (unstack)"));
        transformButtonBar->AddButton(ID_PIVOT_LONGER, _(L"Pivot Longer"),
                                      ReadSvgIcon(L"pivot-longer.svg"),
                                      _(L"Pivot a dataset longer (stack)"));
        transformButtonBar->AddButton(ID_SUBSET_DATASET, _(L"Subset"), ReadSvgIcon(L"subset.svg"),
                                      _(L"Create a subset of a dataset"));
        transformButtonBar->AddButton(ID_JOIN_DATASET, _(L"Join"), ReadSvgIcon(L"join.svg"),
                                      _(L"Join two datasets"));

        // Constants panel
        auto* constantsPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Constants"));
        auto* constantsButtonBar = new wxRibbonButtonBar(constantsPanel, wxID_ANY);

        constantsButtonBar->AddButton(ID_ADD_CONSTANT, _(L"Add"), ReadSvgIcon(L"constants-add.svg"),
                                      _(L"Add a constant to the project"));
        constantsButtonBar->AddButton(ID_DELETE_CONSTANT, _(L"Delete"),
                                      ReadSvgIcon(L"constants-delete.svg"),
                                      _(L"Delete the selected constant"));

        // Analyses tab
        auto* analysesPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Analyses"));

        // Graph category panel
        auto* graphPanel = new wxRibbonPanel(analysesPage, wxID_ANY, _(L"Graphs"));
        auto* graphButtonBar = new wxRibbonButtonBar(graphPanel, ID_GRAPH_BUTTONBAR);

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_BASIC, _(L"Basic"),
                                          ReadSvgIcon(L"chart-basic.svg"), _(L"Basic graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_BUSINESS, _(L"Business"),
                                          ReadSvgIcon(L"chart-business.svg"),
                                          _(L"Business graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_STATISTICAL, _(L"Statistical"),
                                          ReadSvgIcon(L"chart-statistical.svg"),
                                          _(L"Statistical graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SURVEY, _(L"Survey"),
                                          ReadSvgIcon(L"chart-survey.svg"),
                                          _(L"Survey data graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_EDUCATION, _(L"Education"),
                                          ReadSvgIcon(L"chart-education.svg"),
                                          _(L"Education graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SOCIAL, _(L"Social Sciences"),
                                          ReadSvgIcon(L"chart-social.svg"),
                                          _(L"Social sciences graphs"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SPORTS, _(L"Sports"),
                                          ReadSvgIcon(L"chart-sports.svg"), _(L"Sports graphs"));
        }
    else
        {
        // TODO: main frame ribbon content
        }

    // Tools panel
    auto* toolsPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Tools"));
    auto* toolsButtonBar = new wxRibbonButtonBar(toolsPanel, wxID_ANY);

    toolsButtonBar->AddButton(ID_VIEW_LOG_REPORT, _(L"Log"), ReadSvgIcon(L"log-book.svg"),
                              _(L"View the log report"));

    // Help tab
    auto* helpPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Help"));

    auto* aboutPanel = new wxRibbonPanel(helpPage, wxID_ANY, _(L"About"));
    auto* aboutButtonBar = new wxRibbonButtonBar(aboutPanel, wxID_ANY);

    aboutButtonBar->AddButton(wxID_ABOUT, _(L"About"), ReadSvgIcon(L"wisteria.svg"),
                              _(L"About Wisteria Dataviz"));

    ribbon->SetArtProvider(new wxRibbonMSWFlatArtProvider);
    ribbon->Realize();

    return ribbon;
    }

//-------------------------------------------
void WisteriaApp::InitProjectSidebar()
    {
    // fill in the icons for the projects' sidebars
    // Do NOT change the ordering of these (indices are used by LoadProject());
    // new ones always get added at the bottom.
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"data.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"page.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"constants.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"pivot-wider.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"pivot-longer.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"subset.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"join.svg"));
    }

//-------------------------------------------
wxString WisteriaApp::GetItemIconName(const Wisteria::GraphItems::GraphItemBase* item)
    {
    if (item == nullptr)
        {
        return {};
        }
    // check most-derived types first
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
        {
        return L"bubbleplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        return L"scatterplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::MultiSeriesLinePlot)))
        {
        return L"lineplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
        {
        return L"wcurve.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        return L"lineplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        return L"candlestick.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        return L"chernoffplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        return L"gantt.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
        {
        return L"histogram.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        return L"likert7.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CategoricalBarChart)))
        {
        return L"barchart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScaleChart)))
        {
        return L"scale.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BarChart)))
        {
        return L"barchart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        return L"boxplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        return L"heatmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        return L"piechart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        return L"table.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        return L"sankey.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        return L"waffle.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::StemAndLeafPlot)))
        {
        return L"stem-leaf.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WordCloud)))
        {
        return L"wordcloud.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        return L"roadmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        return L"roadmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        return L"sparkline.svg";
        }
    // non-graph items
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
        {
        return L"label.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Image)))
        {
        return L"image.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::FillableShape)))
        {
        return L"shape.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Shape)))
        {
        return L"shape.svg";
        }

    return {};
    }
