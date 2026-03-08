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
                    { L"ID_SELECT_ALL", L"select-all.svg" },
                    { L"ID_LIST_SORT", L"sort.svg" },
                    { L"ID_CLEAR", L"clear.svg" },
                    { L"ID_REFRESH", L"reload.svg" },
                    { L"ID_REALTIME_UPDATE", L"realtime.svg" } };
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

    InitProjectSidebar();

    // create the document template
    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    [[maybe_unused]]
    auto* docTemplate =
        new wxDocTemplate(GetDocManager(), _(L"Wisteria project"), L"*.json", wxString{}, L"json",
                          _DT(L"Wisteria Doc"), L"WisteriaView", wxCLASSINFO(WisteriaDoc),
                          wxCLASSINFO(WisteriaView));
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    SetAppFileExtension(L"json");

    LoadInterface();

    return true;
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
                         [this](wxCommandEvent& event)
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
    m_logWindow->GetListCtrl()->DistributeColumns(-1);

    m_logWindow->Show();
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
    const auto iconSize = parent->FromDIP(wxSize{ 32, 32 });

    auto* ribbon = new wxRibbonBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxRIBBON_BAR_SHOW_PAGE_ICONS | wxRIBBON_BAR_DEFAULT_STYLE);

    // Home tab
    const auto homeIcon = GetResourceManager().GetSVG(
        wxSystemSettings::GetAppearance().IsDark() ? L"home-dark-mode.svg" : L"home.svg");
    auto* homePage = new wxRibbonPage(
        ribbon, wxID_ANY, _(L"Home"),
        homeIcon.IsOk() ? homeIcon.GetBitmap(parent->FromDIP(wxSize{ 16, 16 })) : wxBitmap{});

    // Project panel with New and Open buttons
    auto* projectPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Project"));
    auto* projectButtonBar = new wxRibbonButtonBar(projectPanel, wxID_ANY);

    const auto newIcon = GetResourceManager().GetSVG(L"wisteria.svg");
    projectButtonBar->AddButton(wxID_NEW, _(L"New"),
                                newIcon.IsOk() ? newIcon.GetBitmap(iconSize) : wxBitmap{},
                                _(L"Create a new project"));

    const auto openIcon = GetResourceManager().GetSVG(L"file-open.svg");
    projectButtonBar->AddButton(wxID_OPEN, _(L"Open"),
                                openIcon.IsOk() ? openIcon.GetBitmap(iconSize) : wxBitmap{},
                                _(L"Open a data file"));

    if (isProjectRibbon)
        {
        // Print panel
        auto* printPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Print"));
        auto* printButtonBar = new wxRibbonButtonBar(printPanel, wxID_ANY);

        const auto printIcon = GetResourceManager().GetSVG(L"print.svg");
        printButtonBar->AddButton(wxID_PRINT, _(L"Print"),
                                  printIcon.IsOk() ? printIcon.GetBitmap(iconSize) : wxBitmap{},
                                  _(L"Print all pages"));

        // Insert panel
        auto* insertPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Insert"));
        auto* insertButtonBar = new wxRibbonButtonBar(insertPanel, wxID_ANY);

        const auto dataIcon = GetResourceManager().GetSVG(L"data.svg");
        insertButtonBar->AddButton(ID_INSERT_DATASET, _(L"Dataset"),
                                   dataIcon.IsOk() ? dataIcon.GetBitmap(iconSize) : wxBitmap{},
                                   _(L"Import a dataset into the project"));

        const auto pageIcon = GetResourceManager().GetSVG(L"page.svg");
        insertButtonBar->AddButton(ID_INSERT_PAGE, _(L"Page"),
                                   pageIcon.IsOk() ? pageIcon.GetBitmap(iconSize) : wxBitmap{},
                                   _(L"Add a new page to the project"));

        // Graph category panel
        auto* graphPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Graphs"));
        auto* graphButtonBar = new wxRibbonButtonBar(graphPanel, ID_GRAPH_BUTTONBAR);

        const auto basicIcon = GetResourceManager().GetSVG(L"chart-basic.svg");
        graphButtonBar->AddDropdownButton(
            ID_INSERT_GRAPH_BASIC, _(L"Basic"),
            basicIcon.IsOk() ? basicIcon.GetBitmap(iconSize) : wxBitmap{}, _(L"Basic graphs"));

        const auto businessIcon = GetResourceManager().GetSVG(L"chart-business.svg");
        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_BUSINESS, _(L"Business"),
                                          businessIcon.IsOk() ? businessIcon.GetBitmap(iconSize) :
                                                                wxBitmap{},
                                          _(L"Business graphs"));

        const auto statisticalIcon = GetResourceManager().GetSVG(L"chart-statistical.svg");
        graphButtonBar->AddDropdownButton(
            ID_INSERT_GRAPH_STATISTICAL, _(L"Statistical"),
            statisticalIcon.IsOk() ? statisticalIcon.GetBitmap(iconSize) : wxBitmap{},
            _(L"Statistical graphs"));

        const auto surveyIcon = GetResourceManager().GetSVG(L"chart-survey.svg");
        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SURVEY, _(L"Survey"),
                                          surveyIcon.IsOk() ? surveyIcon.GetBitmap(iconSize) :
                                                              wxBitmap{},
                                          _(L"Survey data graphs"));

        const auto educationIcon = GetResourceManager().GetSVG(L"chart-education.svg");
        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_EDUCATION, _(L"Education"),
                                          educationIcon.IsOk() ? educationIcon.GetBitmap(iconSize) :
                                                                 wxBitmap{},
                                          _(L"Education graphs"));

        const auto socialIcon = GetResourceManager().GetSVG(L"chart-social.svg");
        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SOCIAL, _(L"Social Sciences"),
                                          socialIcon.IsOk() ? socialIcon.GetBitmap(iconSize) :
                                                              wxBitmap{},
                                          _(L"Social sciences graphs"));

        const auto sportsIcon = GetResourceManager().GetSVG(L"chart-sports.svg");
        graphButtonBar->AddDropdownButton(
            ID_INSERT_GRAPH_SPORTS, _(L"Sports"),
            sportsIcon.IsOk() ? sportsIcon.GetBitmap(iconSize) : wxBitmap{}, _(L"Sports graphs"));
        }
    else
        {
        // TODO: main frame ribbon content
        }

    // Tools panel
    auto* toolsPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Tools"));
    auto* toolsButtonBar = new wxRibbonButtonBar(toolsPanel, wxID_ANY);

    const auto logIcon = GetResourceManager().GetSVG(L"log-book.svg");
    toolsButtonBar->AddButton(ID_VIEW_LOG_REPORT, _(L"Log"),
                              logIcon.IsOk() ? logIcon.GetBitmap(iconSize) : wxBitmap{},
                              _(L"View the log report"));

    // Help tab
    auto* helpPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Help"));

    auto* aboutPanel = new wxRibbonPanel(helpPage, wxID_ANY, _(L"About"));
    auto* aboutButtonBar = new wxRibbonButtonBar(aboutPanel, wxID_ANY);

    const auto appIcon = GetResourceManager().GetSVG(L"wisteria.svg");
    aboutButtonBar->AddButton(wxID_ABOUT, _(L"About"),
                              appIcon.IsOk() ? appIcon.GetBitmap(iconSize) : wxBitmap{},
                              _(L"About Wisteria Dataviz"));

    ribbon->SetArtProvider(new wxRibbonMSWArtProvider);
    ribbon->Realize();

    return ribbon;
    }

//-------------------------------------------
void WisteriaApp::InitProjectSidebar()
    {
    // fill in the icons for the projects' sidebars
    // Do NOT change the ordering of these (indices are used by LoadProject());
    // new ones always get added at the bottom.
    m_projectSideBarImageList.push_back(GetResourceManager().GetSVG(L"data.svg"));
    m_projectSideBarImageList.push_back(GetResourceManager().GetSVG(L"page.svg"));
    }
