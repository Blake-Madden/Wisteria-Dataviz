///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaapp.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaapp.h"
#include "../import/text_matrix.h"
#include "wisteriadoc.h"
#include "wisteriaview.h"
#include <array>
#include <wx/aboutdlg.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast,cppcoreguidelines-avoid-non-const-global-variables)
wxIMPLEMENT_APP(WisteriaApp);
wxIMPLEMENT_CLASS(MainFrame, Wisteria::UI::BaseMainFrame);

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
    SetAppName(WISTERIA_APP_NAME);
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

    Wisteria::Settings::SetReportEditingEnabled(true);
    GetResourceManager().LoadArchive(FindResourceFile(L"res.wad"));
    wxArtProvider::Push(new WisteriaArtProvider{});

    // create the document template
    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    [[maybe_unused]]
    auto* docTemplate = new wxDocTemplate(GetDocManager(), _(L"Wisteria project"), L"*.wdv",
                                          wxString{}, L"wdv", _DT(L"Wisteria Doc"), L"WisteriaView",
                                          wxCLASSINFO(WisteriaDoc), wxCLASSINFO(WisteriaView));
    SetAppFileExtension(L"wdv");

        // load MRU file history before building the start page
        {
        wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
        config.SetPath(DONTTRANSLATE(L"Recent File List", DTExplanation::SystemEntry));
        GetDocManager()->FileHistoryLoad(config);
        }

    LoadInterface();
    InitProjectSidebar();

    BaseApp::LogSystemInfo();

    return true;
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    }

//-------------------------------------------
void WisteriaApp::LoadInterface()
    {
    const wxArrayString extensions{ GetAppFileExtension() };

    SetMainFrame(new MainFrame(GetDocManager(), nullptr, extensions, WISTERIA_APP_NAME,
                               wxPoint{ 0, 0 }, GetAppSettings()->GetAppWindowSize(),
                               wxDEFAULT_FRAME_STYLE));

    GetMainFrame()->InitControls(CreateRibbon(GetMainFrame()));

    // create the embedded log panel (hidden until Log tab is activated)
    GetMainFrameEx()->m_logDataProvider = std::make_shared<Wisteria::UI::ListCtrlExDataProvider>();
    GetMainFrameEx()->m_logPanel = new wxPanel(GetMainFrameEx());
    GetMainFrameEx()->m_logPanel->Hide();
    GetMainFrameEx()->m_logListCtrl =
        new Wisteria::UI::ListCtrlEx(GetMainFrameEx()->m_logPanel, wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxBORDER_NONE);
    GetMainFrameEx()->m_logListCtrl->SetVirtualDataProvider(GetMainFrameEx()->m_logDataProvider);
    auto* logPanelSizer = new wxBoxSizer(wxVERTICAL);
    logPanelSizer->Add(GetMainFrameEx()->m_logListCtrl, wxSizerFlags{ 1 }.Expand());
    GetMainFrameEx()->m_logPanel->SetSizer(logPanelSizer);
    GetMainFrameEx()->GetSizer()->Add(GetMainFrameEx()->m_logPanel, wxSizerFlags{ 1 }.Expand());

    GetMainFrameEx()->SetLogAutoRefresh(GetAppSettings()->IsLogAutoRefresh());
    wxLog::SetVerbose(GetAppSettings()->IsLogVerbose());

    const std::array<wxAcceleratorEntry, 1> entries = { wxAcceleratorEntry(wxACCEL_CTRL, L'O',
                                                                           wxID_OPEN) };
    GetMainFrame()->SetAcceleratorTable(wxAcceleratorTable(entries.size(), entries.data()));

    // add start page
    wxArrayString mruFiles;
    for (size_t i = 0; i < GetDocManager()->GetFileHistory()->GetCount(); ++i)
        {
        mruFiles.Add(GetDocManager()->GetFileHistory()->GetHistoryFile(i));
        }
    m_startPage = new wxStartPage(GetMainFrame(), wxID_ANY, mruFiles,
                                  GetResourceManager().GetSVG(L"wisteria.svg"));
    m_startPage->AddButton(GetResourceManager().GetSVG(L"wisteria.svg"),
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
                             else if (wxStartPage::IsBrowseId(event.GetId()))
                                 {
                                     {
                                     wxCommandEvent openEvent(wxEVT_MENU, wxID_OPEN);
                                     GetMainFrame()->ProcessWindowEvent(openEvent);
                                     }
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
               wxCommandEvent& event)
        {
            wxAboutDialogInfo aboutInfo;
            aboutInfo.SetCopyright(_(L"Copyright (c) 2005-2026 Blake Madden"));
            wxArrayString devs;
            devs.Add(_DT(L"Blake Madden"));
            aboutInfo.SetDevelopers(devs);
            aboutInfo.SetName(WISTERIA_APP_NAME);
            aboutInfo.SetDescription(_(L"Data visualization application."));
            wxAboutBox(aboutInfo, GetMainFrame());
        },
        wxID_ABOUT);

    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event)
        {
            wxPageSetupDialogData pageSetupData;
            wxPrintData printData;

            // apply global settings
            auto& settings = wxGetApp().GetAppSettings();
            printData.SetOrientation(
                static_cast<wxPrintOrientation>(settings->GetPrintOrientation()));
            printData.SetPaperId(settings->GetPaperId());

            pageSetupData.SetPrintData(printData);

            wxPageSetupDialog dialog(GetMainFrame(), &pageSetupData);
            if (dialog.ShowModal() == wxID_OK)
                {
                wxPrintData updatedData = dialog.GetPageSetupData().GetPrintData();
                settings->SetPrintOrientation(updatedData.GetOrientation());
                settings->SetPaperId(updatedData.GetPaperId());
                settings->SaveSettingsFile();
                }
        },
        ID_PRINT_SETUP);

    // capture window state before the frame is destroyed
    GetMainFrame()->Bind(
        wxEVT_CLOSE_WINDOW,
        [this](wxCloseEvent& event)
        {
            GetAppSettings()->SetAppWindowMaximized(GetMainFrame()->IsMaximized());
            GetAppSettings()->SetAppWindowWidth(GetMainFrame()->GetSize().GetWidth());
            GetAppSettings()->SetAppWindowHeight(GetMainFrame()->GetSize().GetHeight());
            event.Skip();
        });

    // ribbon page-changed: show/hide log panel and manage auto-refresh timer
    GetMainFrame()->Bind(wxEVT_RIBBONBAR_PAGE_CHANGED,
                         [this](wxRibbonBarEvent& evt)
                         {
                             const bool showLog = GetMainFrameEx()->IsLogTabActive();
                             if (m_startPage != nullptr)
                                 {
                                 m_startPage->Show(!showLog);
                                 }
                             if (GetMainFrameEx()->m_logPanel != nullptr)
                                 {
                                 GetMainFrameEx()->m_logPanel->Show(showLog);
                                 }
                             if (showLog)
                                 {
                                 if (GetMainFrameEx()->m_logEditButtonBar != nullptr)
                                     {
                                     GetMainFrameEx()->m_logEditButtonBar->ToggleButton(
                                         ID_LOG_TAB_REALTIME_UPDATE,
                                         GetMainFrameEx()->m_logAutoRefresh);
                                     GetMainFrameEx()->m_logEditButtonBar->ToggleButton(
                                         ID_LOG_TAB_VERBOSE, wxLog::GetVerbose());
                                     }
                                 if (GetMainFrameEx()->m_logAutoRefresh)
                                     {
                                     GetMainFrameEx()->m_logAutoRefreshTimer.Start(3000);
                                     }
                                 ReadLogIntoListCtrl(GetMainFrameEx()->m_logListCtrl);
                                 GetMainFrameEx()->m_logListCtrl->SetFocus();
                                 }
                             else
                                 {
                                 GetMainFrameEx()->m_logAutoRefreshTimer.Stop();
                                 }
                             GetMainFrame()->Layout();
                             evt.Skip();
                         });

    // log tab ribbon button handlers
    const auto withLogList = [this](auto fn)
    {
        return [this, fn](wxRibbonButtonBarEvent& event)
        {
            if (GetMainFrameEx()->m_logListCtrl != nullptr && GetMainFrameEx()->IsLogTabActive())
                {
                fn(GetMainFrameEx()->m_logListCtrl, event);
                }
        };
    };

    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED,
                         withLogList([](Wisteria::UI::ListCtrlEx* list, wxRibbonButtonBarEvent& evt)
                                     { list->OnSave(evt); }),
                         ID_LOG_TAB_SAVE);
    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED,
                         withLogList([](Wisteria::UI::ListCtrlEx* list, wxRibbonButtonBarEvent& evt)
                                     { list->OnPrint(evt); }),
                         ID_LOG_TAB_PRINT);
    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED,
                         withLogList([](Wisteria::UI::ListCtrlEx* list, wxRibbonButtonBarEvent& evt)
                                     { list->OnCopy(evt); }),
                         ID_LOG_TAB_COPY);
    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED,
                         withLogList([](Wisteria::UI::ListCtrlEx* list, wxRibbonButtonBarEvent& evt)
                                     { list->OnSelectAll(evt); }),
                         ID_LOG_TAB_SELECT_ALL);
    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED,
                         withLogList([](Wisteria::UI::ListCtrlEx* list, wxRibbonButtonBarEvent& evt)
                                     { list->OnMultiColumnSort(evt); }),
                         ID_LOG_TAB_SORT);
    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]] wxRibbonButtonBarEvent&)
        {
            if (GetMainFrameEx()->m_logListCtrl != nullptr && GetMainFrameEx()->IsLogTabActive())
                {
                if (GetLogFile() != nullptr)
                    {
                    GetLogFile()->Clear();
                    }
                GetMainFrameEx()->m_logListCtrl->DeleteAllItems();
                }
        },
        ID_LOG_TAB_CLEAR);
    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]] wxRibbonButtonBarEvent&)
        {
            if (GetMainFrameEx()->m_logListCtrl != nullptr && GetMainFrameEx()->IsLogTabActive())
                {
                ReadLogIntoListCtrl(GetMainFrameEx()->m_logListCtrl);
                }
        },
        ID_LOG_TAB_REFRESH);
    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]] wxRibbonButtonBarEvent&)
        {
            GetMainFrameEx()->SetLogAutoRefresh(!GetMainFrameEx()->m_logAutoRefresh);
            GetAppSettings()->SetLogAutoRefresh(GetMainFrameEx()->m_logAutoRefresh);
            GetAppSettings()->SaveSettingsFile();
        },
        ID_LOG_TAB_REALTIME_UPDATE);
    GetMainFrame()->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]] wxRibbonButtonBarEvent&)
        {
            wxLog::SetVerbose(!wxLog::GetVerbose());
            GetAppSettings()->SetLogVerbose(wxLog::GetVerbose());
            GetAppSettings()->SaveSettingsFile();
        },
        ID_LOG_TAB_VERBOSE);
    GetMainFrame()->Bind(
        wxEVT_TIMER,
        [this]([[maybe_unused]] wxTimerEvent&)
        {
            if (GetMainFrameEx()->m_logListCtrl != nullptr && GetMainFrameEx()->IsLogTabActive())
                {
                ReadLogIntoListCtrl(GetMainFrameEx()->m_logListCtrl);
                }
        },
        GetMainFrameEx()->m_logAutoRefreshTimer.GetId());

    GetMainFrame()->CenterOnScreen();
    if (GetAppSettings()->IsAppWindowMaximized())
        {
        GetMainFrame()->Maximize();
        GetMainFrame()->SetSize(GetMainFrame()->GetSize());
        }
    m_startPage->SetFocus();
    GetMainFrame()->Show(true);
    }

//-------------------------------------------
void WisteriaApp::LoadRibbonLogPage(wxRibbonBar* ribbon)
    {
    GetMainFrameEx()->m_logRibbonPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Log"));

    auto* exportBar = new wxRibbonButtonBar(new wxRibbonPanel(
        GetMainFrameEx()->GetLogRibbonPage(), wxID_ANY, _(L"Export"), wxNullBitmap,
        wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE));
    exportBar->AddButton(ID_LOG_TAB_SAVE, _(L"Save"), ReadSvgIcon(L"file-save.svg"),
                         _(L"Save the log report."));
    exportBar->AddButton(ID_LOG_TAB_PRINT, _(L"Print"), ReadSvgIcon(L"print.svg"),
                         _(L"Print the log report."));

    GetMainFrameEx()->m_logEditButtonBar = new wxRibbonButtonBar(
        new wxRibbonPanel(GetMainFrameEx()->GetLogRibbonPage(), wxID_ANY, _(L"Edit"), wxNullBitmap,
                          wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_COPY, _(L"Copy Selection"),
                                                    ReadSvgIcon(L"copy.svg"),
                                                    _(L"Copy the selected items."));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_SELECT_ALL, _(L"Select All"),
                                                    ReadSvgIcon(L"select-all.svg"),
                                                    _(L"Select the entire list."));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_SORT, _(L"Sort"),
                                                    ReadSvgIcon(L"sort.svg"), _(L"Sort the list."));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(
        ID_LOG_TAB_CLEAR, _(L"Clear"), ReadSvgIcon(L"clear.svg"), _(L"Clear the log report."));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_REFRESH, _(L"Refresh"),
                                                    ReadSvgIcon(L"reload.svg"),
                                                    _(L"Refresh the log report."));
    GetMainFrameEx()->m_logEditButtonBar->AddToggleButton(
        ID_LOG_TAB_REALTIME_UPDATE, _(L"Auto Refresh"), ReadSvgIcon(L"realtime.svg"),
        _(L"Refresh the log report automatically."));
    GetMainFrameEx()->m_logEditButtonBar->AddToggleButton(
        ID_LOG_TAB_VERBOSE, _(L"Verbose"), ReadSvgIcon(L"edit.svg"),
        _(L"Toggles whether the logging system includes more detailed information."));
    }

//-------------------------------------------
void WisteriaApp::ReadLogIntoListCtrl(Wisteria::UI::ListCtrlEx* listCtrl)
    {
    if (listCtrl == nullptr || GetLogFile() == nullptr)
        {
        return;
        }
    listCtrl->SetLabel(wxString::Format(
        // TRANSLATORS: %1$s is the application name;
        // %2$s is today's date in ISO format (YYYY-MM-DD)
        _(L"%1$s Log %2$s"), GetAppDisplayName(), wxDateTime::Now().FormatISODate()));
    const long style = listCtrl->GetExtraStyle();
    listCtrl->SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
    const wxWindowUpdateLocker wl{ listCtrl };

    if (listCtrl->GetColumnCount() < 4)
        {
        listCtrl->DeleteAllColumns();
        listCtrl->InsertColumn(0, _(L"Message"));
        listCtrl->InsertColumn(1, _(L"Timestamp"));
        listCtrl->InsertColumn(2, _(L"Function"));
        listCtrl->InsertColumn(3, _(L"Source"));
        }
    listCtrl->EnableAlternateRowColours(false);
    listCtrl->DeleteAllItems();

    const lily_of_the_valley::text_column_delimited_character_parser parser(L'\t');
    lily_of_the_valley::text_column<lily_of_the_valley::text_column_delimited_character_parser>
        myColumn(parser, std::nullopt);
    lily_of_the_valley::text_row<Wisteria::UI::ListCtrlExDataProvider::ListCellString> myRow(
        std::nullopt);
    myRow.treat_consecutive_delimiters_as_one(false);
    myRow.add_column(myColumn);

    auto* dataProvider = dynamic_cast<Wisteria::UI::ListCtrlExDataProvider*>(
        listCtrl->GetVirtualDataProvider().get());
    if (dataProvider == nullptr)
        {
        return;
        }
    lily_of_the_valley::text_matrix<Wisteria::UI::ListCtrlExDataProvider::ListCellString> importer(
        &dataProvider->GetMatrix());
    importer.add_row_definition(myRow);

    const wxString logBuffer{ GetLogFile()->Read() };
    lily_of_the_valley::text_preview preview;
    size_t rowCount = preview(logBuffer, L'\t', true, false);
    rowCount = importer.read(logBuffer, rowCount, 4, true);

    listCtrl->SetVirtualDataSize(rowCount, 4);
    listCtrl->SetItemCount(static_cast<long>(rowCount));

    for (long i = 0; i < listCtrl->GetItemCount(); ++i)
        {
        const auto currentRow = listCtrl->GetItemText(i, 0);
        const wxColour rowColor =
            (currentRow.find(L"Error: ") != wxString::npos) ?
                wxColour(242, 94, 101) :
            (currentRow.find(L"Warning: ") != wxString::npos) ?
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Yellow) :
            (currentRow.find(L"Debug: ") != wxString::npos) ? wxColour(143, 214, 159) :
                                                              wxNullColour;
        if (rowColor.IsOk())
            {
            listCtrl->SetRowAttributes(
                i, wxListItemAttr(wxColour{ 0, 0, 0 }, rowColor, listCtrl->GetFont()));
            }
        }

    if (listCtrl->GetItemCount() > 0)
        {
        listCtrl->EnsureVisible(listCtrl->GetItemCount() - 1);
        }
    listCtrl->SetSortedColumn(0, Wisteria::SortDirection::SortAscending);
    listCtrl->SetExtraStyle(style);
    listCtrl->DistributeColumns(-1);
    }

//-------------------------------------------
void MainFrame::ActivateLogTab()
    {
    if (m_logRibbonPage == nullptr)
        {
        return;
        }
    if (!IsShown())
        {
        Show();
        }
    Raise();
    GetRibbon()->SetActivePage(m_logRibbonPage);
    if (auto* app = dynamic_cast<WisteriaApp*>(wxTheApp))
        {
        if (app->GetStartPage() != nullptr)
            {
            app->GetStartPage()->Hide();
            }
        }
    m_logPanel->Show();
    if (m_logEditButtonBar != nullptr)
        {
        m_logEditButtonBar->ToggleButton(ID_LOG_TAB_REALTIME_UPDATE, m_logAutoRefresh);
        m_logEditButtonBar->ToggleButton(ID_LOG_TAB_VERBOSE, wxLog::GetVerbose());
        }
    Layout();
    wxGetApp().ReadLogIntoListCtrl(m_logListCtrl);
    m_logListCtrl->SetFocus();
    }

//-------------------------------------------
void MainFrame::SetLogAutoRefresh(const bool enable)
    {
    m_logAutoRefresh = enable;
    if (m_logEditButtonBar != nullptr)
        {
        m_logEditButtonBar->ToggleButton(ID_LOG_TAB_REALTIME_UPDATE, enable);
        }
    if (enable && IsLogTabActive())
        {
        m_logAutoRefreshTimer.Start(3000);
        }
    else
        {
        m_logAutoRefreshTimer.Stop();
        }
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
        projectButtonBar->AddButton(ID_PDF_EXPORT, _(L"PDF Export"), ReadSvgIcon(L"pdf.svg"),
                                    _(L"Export all pages to PDF"));
        projectButtonBar->AddButton(ID_PROJECT_SETTINGS, _(L"Project Settings"),
                                    ReadSvgIcon(L"project-settings.svg"),
                                    _(L"Edit the project settings"));

        // Print panel
        auto* printPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Print"));
        auto* printButtonBar = new wxRibbonButtonBar(printPanel, wxID_ANY);
        printButtonBar->AddButton(wxID_PRINT, _(L"Print"), ReadSvgIcon(L"print.svg"),
                                  _(L"Print all pages"));
        printButtonBar->AddButton(ID_PRINT_SETUP, _(L"Page Setup"), ReadSvgIcon(L"print-setup.svg"),
                                  _(L"Configure print settings"));

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
        objectsButtonBar->AddButton(ID_NEW_COMMON_AXIS, _(L"Axis"), ReadSvgIcon(L"axis.svg"),
                                    _(L"Insert an axis"));
        objectsButtonBar->AddButton(wxID_COPY, _(L"Copy"), ReadSvgIcon(L"copy.svg"),
                                    _(L"Copy the selected item"));
        objectsButtonBar->AddButton(wxID_PASTE, _(L"Paste"), ReadSvgIcon(L"paste.svg"),
                                    _(L"Paste the copied item"));
        objectsButtonBar->AddButton(ID_EDIT_ITEM, _(L"Edit"), ReadSvgIcon(L"edit.svg"),
                                    _(L"Edit the selected item"));
        objectsButtonBar->AddButton(ID_DELETE_ITEM, _(L"Delete"), ReadSvgIcon(L"delete.svg"),
                                    _(L"Delete the selected item"));

        // Data tab
        auto* dataPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Data"));

        // Datasets panel
        auto* dataPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Datasets"));
        auto* dataButtonBar = new wxRibbonButtonBar(dataPanel, ID_DATASET_BUTTONBAR);

        dataButtonBar->AddButton(ID_INSERT_DATASET, _(L"Add"), ReadSvgIcon(L"data-add.svg"),
                                 _(L"Import a dataset into the project"));
        dataButtonBar->AddButton(ID_EDIT_DATASET, _(L"Edit"), ReadSvgIcon(L"data-edit.svg"),
                                 _(L"Edit the selected dataset's import options"));
        dataButtonBar->AddButton(ID_DELETE_DATASET, _(L"Delete"), ReadSvgIcon(L"data-delete.svg"),
                                 _(L"Delete the selected dataset from the project"));

        // Transformations panel
        auto* transformPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Transformations"));
        auto* transformButtonBar = new wxRibbonButtonBar(transformPanel, wxID_ANY);

        transformButtonBar->AddButton(ID_SUBSET_DATASET, _(L"Subset"), ReadSvgIcon(L"subset.svg"),
                                      _(L"Create a subset of a dataset"));
        transformButtonBar->AddButton(ID_JOIN_DATASET, _(L"Join"), ReadSvgIcon(L"join.svg"),
                                      _(L"Join two datasets"));
        transformButtonBar->AddButton(ID_PIVOT_WIDER, _(L"Pivot Wider"),
                                      ReadSvgIcon(L"pivot-wider.svg"),
                                      _(L"Pivot a dataset wider (unstack)"));
        transformButtonBar->AddButton(ID_PIVOT_LONGER, _(L"Pivot Longer"),
                                      ReadSvgIcon(L"pivot-longer.svg"),
                                      _(L"Pivot a dataset longer (stack)"));

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

        // Tools panel (project frames only — navigates to main frame log tab)
        auto* toolsPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Tools"));
        auto* toolsButtonBar = new wxRibbonButtonBar(toolsPanel, wxID_ANY);
        toolsButtonBar->AddButton(ID_VIEW_LOG_REPORT, _(L"Log"), ReadSvgIcon(L"log-book.svg"),
                                  _(L"View the log report"));
        }
    else
        {
        // Print panel
        auto* printPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Print"));
        auto* printButtonBar = new wxRibbonButtonBar(printPanel, wxID_ANY);
        printButtonBar->AddButton(ID_PRINT_SETUP, _(L"Page Setup"), ReadSvgIcon(L"print-setup.svg"),
                                  _(L"Configure print settings"));

        // Log tab (main frame only)
        LoadRibbonLogPage(ribbon);
        }

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
wxString WisteriaApp::GetGraphTypeString(const Wisteria::Graphs::Graph2D* graph)
    {
    if (graph == nullptr)
        {
        return {};
        }

    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::MultiSeriesLinePlot)))
        {
        return _DT(L"multi-series-line-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
        {
        return _DT(L"w-curve-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        return _DT(L"line-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
        {
        return _DT(L"bubble-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        return _DT(L"scatter-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        return _DT(L"likert-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CategoricalBarChart)))
        {
        return _DT(L"categorical-bar-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
        {
        return _DT(L"histogram");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScaleChart)))
        {
        return _DT(L"scale-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BarChart)))
        {
        return _DT(L"bar-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        return _DT(L"box-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        return _DT(L"pie-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        return _DT(L"heatmap");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        return _DT(L"table");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        return _DT(L"gantt-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        return _DT(L"candlestick-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        return _DT(L"linear-regression-roadmap");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        return _DT(L"pro-con-roadmap");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        return _DT(L"waffle-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WordCloud)))
        {
        return _DT(L"word-cloud");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        return _DT(L"sankey-diagram");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        return _DT(L"win-loss-sparkline");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        return _DT(L"chernoff-faces");
        }
    return {};
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
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Axis)))
        {
        return L"axis.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
        {
        return L"label.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend)))
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
