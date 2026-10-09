///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaapp.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaapp.h"
#include "../data/dataset.h"
#include "../import/text_matrix.h"
#include "wisteriadoc.h"
#include "wisteriaview.h"
#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <utility>
#include <wx/aboutdlg.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/datetime.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/numformatter.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/valgen.h>

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast,cppcoreguidelines-avoid-non-const-global-variables)
wxIMPLEMENT_APP(WisteriaApp);
wxIMPLEMENT_CLASS(MainFrame, Wisteria::UI::BaseMainFrame);

//-------------------------------------------
WisteriaArtProvider::WisteriaArtProvider()
    {
    // cppcheck-suppress useInitializationList
    m_idFileMap = { { L"ID_CONTINUOUS", L"images/scale.svg" },
                    { L"ID_CATEGORICAL", L"images/categorical.svg" },
                    { L"ID_DISCRETE", L"images/discrete.svg" },
                    { L"ID_DATE", L"images/date.svg" },
                    { L"ID_DICHOTOMOUS_CATEGORICAL", L"images/dichotomous-categorical.svg" },
                    { L"ID_DICHOTOMOUS_DISCRETE", L"images/dichotomous-discrete.svg" },
                    { wxART_FILE_OPEN, L"images/file-open.svg" },
                    { wxART_FILE_SAVE, L"images/file-save.svg" },
                    { wxART_PRINT, L"images/print.svg" },
                    { wxART_COPY, L"images/copy.svg" },
                    { wxART_FIND, L"images/find.svg" },
                    { wxART_DELETE, L"images/delete.svg" },
                    { L"ID_SELECT_ALL", L"images/select-all.svg" },
                    { L"ID_LIST_SORT", L"images/sort.svg" },
                    { L"ID_CLEAR", L"images/clear.svg" },
                    { L"ID_REFRESH", L"images/reload.svg" },
                    { L"ID_REALTIME_UPDATE", L"images/realtime.svg" },
                    { wxART_EDIT, L"images/edit.svg" } };
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

    GetMainFrame()->InitControls(CreateRibbon(GetMainFrame()->GetPanel()));

    // create the embedded log panel (hidden until Log tab is activated)
    GetMainFrameEx()->m_logDataProvider = std::make_shared<Wisteria::UI::ListCtrlExDataProvider>();
    GetMainFrameEx()->m_logPanel = new wxPanel(GetMainFrame()->GetPanel());
    GetMainFrameEx()->m_logPanel->Hide();
    GetMainFrameEx()->m_logListCtrl =
        new Wisteria::UI::ListCtrlEx(GetMainFrameEx()->m_logPanel, wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxBORDER_NONE);
    GetMainFrameEx()->m_logListCtrl->SetVirtualDataProvider(GetMainFrameEx()->m_logDataProvider);
    auto* logPanelSizer = new wxBoxSizer(wxVERTICAL);
    logPanelSizer->Add(GetMainFrameEx()->m_logListCtrl, wxSizerFlags{ 1 }.Expand());
    GetMainFrameEx()->m_logPanel->SetSizer(logPanelSizer);
    GetMainFrame()->GetPanel()->GetSizer()->Add(GetMainFrameEx()->m_logPanel,
                                                wxSizerFlags{ 1 }.Expand());

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
    m_startPage = new wxStartPage(GetMainFrame()->GetPanel(), wxID_ANY, mruFiles,
                                  GetResourceManager().GetSVG(L"images/wisteria.svg"));
    m_startPage->AddButton(GetResourceManager().GetSVG(L"images/wisteria.svg"),
                           _(L"Create a New Project"));
    m_startPage->AddButton(wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON),
                           _(L"Open a Project"));
    GetMainFrame()->GetPanel()->GetSizer()->Add(m_startPage, wxSizerFlags{ 1 }.Expand());

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

    // let Open accept a dataset as well as a project file (bound on the
    // document manager, which owns the default wxID_OPEN handler)
    GetDocManager()->Bind(wxEVT_MENU, &WisteriaApp::OnOpenProjectOrDataset, this, wxID_OPEN);
    GetMainFrame()->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaApp::OnOpenDropdown, this,
                         wxID_OPEN);

    wxIcon appIcon;
    const auto appSvg = GetResourceManager().GetSVG(L"images/wisteria.svg");
    if (appSvg.IsOk())
        {
        appIcon.CopyFromBitmap(appSvg.GetBitmap(GetMainFrame()->FromDIP(wxSize{ 32, 32 })));
        GetMainFrame()->SetIcon(appIcon);
        GetMainFrame()->SetLogo(appSvg);
        }

    GetMainFrame()->Bind(wxEVT_CLOSE_WINDOW,
                         [this](wxCloseEvent& event)
                         {
                             // If project windows are still open, then the user is just dismissing
                             // this frame (e.g., the log or script workbench embedded here),
                             // not the whole app.
                             if (event.CanVeto() && wxGetApp().GetDocumentCount() > 0)
                                 {
                                 GetMainFrame()->Hide();
                                 event.Veto();
                                 return;
                                 }

                             event.Skip();
                         });

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
                             // this event bubbles up from child document frames (whose parent
                             // window is the main frame), so ignore anything not coming from the
                             // main frame's own ribbon; otherwise, switching tabs on a project
                             // window's ribbon would show/focus the main frame's log tab controls
                             // and steal focus to the main frame
                             if (evt.GetEventObject() != GetMainFrameEx()->GetRibbon())
                                 {
                                 evt.Skip();
                                 return;
                                 }
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
                             GetMainFrame()->GetPanel()->Layout();
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
wxString WisteriaApp::GetProjectOrDataFileFilter() const
    {
    const wxString projectWildcard = L"*." + GetAppFileExtension();
    const wxString dataWildcards = Wisteria::Data::Dataset::GetDataFileWildcards();
    return wxString::Format( // TRANSLATORS: %s are file wildcards. Do not reorder them,
                             // and keep the '|' and ';' separators.
               _(L"All Supported Files (%s;%s)|%s;%s|"
                 "Wisteria Project (%s)|%s|"),
               projectWildcard, dataWildcards, projectWildcard, dataWildcards, projectWildcard,
               projectWildcard) +
           Wisteria::Data::Dataset::GetDataFileFilter();
    }

//-------------------------------------------
void WisteriaApp::OnOpenProjectOrDataset([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog fileDlg(GetMainFrame(), _(L"Open"), wxString{}, wxString{},
                         GetProjectOrDataFileFilter(),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const wxString filePath = fileDlg.GetPath();
    if (wxFileName{ filePath }.GetExt().CmpNoCase(GetAppFileExtension()) == 0)
        {
        if (GetDocManager()->CreateDocument(filePath, wxDOC_SILENT) == nullptr)
            {
            GetDocManager()->OnOpenFileFailure();
            }
        }
    else
        {
        StartProjectFromDataset(filePath);
        }
    }

//-------------------------------------------
void WisteriaApp::OnOpenDropdown(wxCommandEvent& event)
    {
    auto& dropdownEvent = dynamic_cast<wxRibbonButtonBarEvent&>(event);

    wxMenu mruMenu;
    auto* fileHistory = GetDocManager()->GetFileHistory();
    const size_t fileCount = fileHistory->GetCount();
    if (fileCount == 0)
        {
        mruMenu.Append(wxID_ANY, _(L"No Recently Opened Files"))->Enable(false);
        }
    else
        {
        for (size_t i = 0; i < fileCount; ++i)
            {
            const wxString filePath{ fileHistory->GetHistoryFile(i) };
            auto* item = mruMenu.Append(wxID_ANY, wxFileName{ filePath }.GetFullName());
            mruMenu.Bind(
                wxEVT_MENU,
                [this, filePath]([[maybe_unused]] wxCommandEvent&)
                {
                    if (GetDocManager()->CreateDocument(filePath, wxDOC_SILENT) == nullptr)
                        {
                        GetDocManager()->OnOpenFileFailure();
                        }
                },
                item->GetId());
            }
        }

    dropdownEvent.PopupMenu(&mruMenu);
    }

//-------------------------------------------
void WisteriaApp::StartProjectFromDataset(const wxString& datasetPath)
    {
    // seed the chosen dataset so that WisteriaView::OnCreate() skips its own
    // dataset prompt and imports this file
    m_pendingDatasetImportPath = datasetPath;
    GetDocManager()->CreateNewDocument();
    m_pendingDatasetImportPath.clear();
    }

//-------------------------------------------
void MainFrame::OpenFileNew(const wxString& path)
    {
    if (Wisteria::Data::Dataset::IsSupportedFileExtension(wxFileName{ path }.GetExt()))
        {
        wxGetApp().StartProjectFromDataset(path);
        }
    else
        {
        BaseMainFrame::OpenFileNew(path);
        }
    }

//-------------------------------------------
void WisteriaApp::LoadRibbonLogPage(wxRibbonBar* ribbon)
    {
    GetMainFrameEx()->m_logRibbonPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Log"));

    auto* exportBar = new wxRibbonButtonBar(new wxRibbonPanel(
        GetMainFrameEx()->GetLogRibbonPage(), wxID_ANY, _(L"Export"), wxNullBitmap,
        wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE));
    exportBar->AddButton(ID_LOG_TAB_SAVE, _(L"Save"), ReadSvgIcon(L"images/file-save.svg"),
                         _(L"Save the log report."));
    exportBar->SetKeyTip(ID_LOG_TAB_SAVE, _DT(L"S"));
    exportBar->AddButton(ID_LOG_TAB_PRINT, _(L"Print"), ReadSvgIcon(L"images/print.svg"),
                         _(L"Print the log report."));
    exportBar->SetKeyTip(ID_LOG_TAB_PRINT, _DT(L"R"));

    GetMainFrameEx()->m_logEditButtonBar = new wxRibbonButtonBar(
        new wxRibbonPanel(GetMainFrameEx()->GetLogRibbonPage(), wxID_ANY, _(L"Edit"), wxNullBitmap,
                          wxDefaultPosition, wxDefaultSize, wxRIBBON_PANEL_NO_AUTO_MINIMISE));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_COPY, _(L"Copy Selection"),
                                                    ReadSvgIcon(L"images/copy.svg"),
                                                    _(L"Copy the selected items."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_COPY, _DT(L"C"));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_SELECT_ALL, _(L"Select All"),
                                                    ReadSvgIcon(L"images/select-all.svg"),
                                                    _(L"Select the entire list."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_SELECT_ALL, _DT(L"T"));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(
        ID_LOG_TAB_SORT, _(L"Sort"), ReadSvgIcon(L"images/sort.svg"), _(L"Sort the list."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_SORT, _DT(L"O"));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_CLEAR, _(L"Clear"),
                                                    ReadSvgIcon(L"images/clear.svg"),
                                                    _(L"Clear the log report."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_CLEAR, _DT(L"B"));
    GetMainFrameEx()->m_logEditButtonBar->AddButton(ID_LOG_TAB_REFRESH, _(L"Refresh"),
                                                    ReadSvgIcon(L"images/reload.svg"),
                                                    _(L"Refresh the log report."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_REFRESH, _DT(L"F"));
    GetMainFrameEx()->m_logEditButtonBar->AddToggleButton(
        ID_LOG_TAB_REALTIME_UPDATE, _(L"Auto Refresh"), ReadSvgIcon(L"images/realtime.svg"),
        _(L"Refresh the log report automatically."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_REALTIME_UPDATE, _DT(L"U"));
    GetMainFrameEx()->m_logEditButtonBar->AddToggleButton(
        ID_LOG_TAB_VERBOSE, _(L"Verbose"), ReadSvgIcon(L"images/edit.svg"),
        _(L"Toggles whether the logging system includes more detailed information."));
    GetMainFrameEx()->m_logEditButtonBar->SetKeyTip(ID_LOG_TAB_VERBOSE, _DT(L"V"));
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

    // File tab (shows the backstage, project windows only)
    if (isProjectRibbon)
        {
        auto* filePage = new wxRibbonPage(ribbon, wxID_ANY, _(L"File"));
        ribbon->SetBackstagePage(filePage);
        ribbon->SetPageKeyTip(filePage, _DT(L"F"));
        }

    // Home tab (the main frame's, or the Pages tab for project windows)
    const auto homeIcon =
        ReadSvgIcon(wxSystemSettings::GetAppearance().IsDark() ? L"images/home-dark-mode.svg" :
                                                                 L"images/home.svg",
                    wxSize{ 16, 16 });
    wxRibbonPage* homePage{ nullptr };
    if (!isProjectRibbon)
        {
        homePage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Home"), homeIcon);
        ribbon->SetPageKeyTip(homePage, _DT(L"H"));

        // Project panel
        auto* projectPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Project"));
        projectPanel->SetKeyTip(_DT(L"Q"));
        auto* projectButtonBar = new wxRibbonButtonBar(projectPanel, wxID_ANY);

        projectButtonBar->AddButton(wxID_NEW, _(L"New"), ReadSvgIcon(L"images/wisteria.svg"),
                                    _(L"Create a new project"));
        projectButtonBar->SetKeyTip(wxID_NEW, _DT(L"N"));

        projectButtonBar->AddHybridButton(
            wxID_OPEN, _(L"Open"), ReadSvgIcon(L"images/file-open.svg"), _(L"Open a data file"));
        projectButtonBar->SetKeyTip(wxID_OPEN, _DT(L"O"));
        projectButtonBar->SetDropdownKeyTip(wxID_OPEN, _DT(L"U"));
        }

    if (isProjectRibbon)
        {
        // Home tab
        homePage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Home"), homeIcon);
        ribbon->SetPageKeyTip(homePage, _DT(L"H"));

        // Pages panel
        auto* pagesPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Pages"));
        pagesPanel->SetKeyTip(_DT(L"S"));
        auto* pagesButtonBar = new wxRibbonButtonBar(pagesPanel, ID_PAGES_BUTTONBAR);

        pagesButtonBar->AddButton(ID_INSERT_PAGE, _(L"Add"), ReadSvgIcon(L"images/page-add.svg"),
                                  _(L"Add a new page to the project"));
        pagesButtonBar->SetKeyTip(ID_INSERT_PAGE, _DT(L"N"));
        pagesButtonBar->AddButton(ID_EDIT_PAGE, _(L"Edit"), ReadSvgIcon(L"images/page-edit.svg"),
                                  _(L"Edit the current page"));
        pagesButtonBar->SetKeyTip(ID_EDIT_PAGE, _DT(L"I"));
        pagesButtonBar->AddButton(ID_DELETE_PAGE, _(L"Delete"),
                                  ReadSvgIcon(L"images/page-delete.svg"),
                                  _(L"Delete the current page"));
        pagesButtonBar->SetKeyTip(ID_DELETE_PAGE, _DT(L"T"));
        pagesButtonBar->AddButton(ID_REARRANGE_PAGES, _(L"Reorder"),
                                  ReadSvgIcon(L"images/sort.svg"),
                                  _(L"Reorder or remove the project's pages"));
        pagesButtonBar->SetKeyTip(ID_REARRANGE_PAGES, _DT(L"R"));
        pagesButtonBar->AddButton(ID_PRINT_SETUP, _(L"Page Layout"),
                                  ReadSvgIcon(L"images/print-setup.svg"),
                                  _(L"Configure print settings"));
        pagesButtonBar->SetKeyTip(ID_PRINT_SETUP, _DT(L"G"));
        pagesButtonBar->AddButton(ID_REFRESH_ALL, _(L"Refresh All"),
                                  ReadSvgIcon(L"images/reload.svg"), _(L"Reload the project"));
        pagesButtonBar->SetKeyTip(ID_REFRESH_ALL, _DT(L"L"));

        // Objects panel (labels, images, shapes)
        auto* objectsPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Objects"));
        objectsPanel->SetKeyTip(_DT(L"OB"));
        auto* objectsButtonBar = new wxRibbonButtonBar(objectsPanel, ID_OBJECTS_BUTTONBAR);

        objectsButtonBar->AddButton(ID_NEW_LABEL, _(L"Label"), ReadSvgIcon(L"images/label.svg"),
                                    _(L"Insert a text label"));
        objectsButtonBar->SetKeyTip(ID_NEW_LABEL, _DT(L"B"));
        objectsButtonBar->AddButton(ID_NEW_KPI_CARD, _(L"KPI Card"),
                                    ReadSvgIcon(L"images/kpi-card.svg"),
                                    _(L"Insert a KPI card (a big number with a caption)"));
        objectsButtonBar->SetKeyTip(ID_NEW_KPI_CARD, _DT(L"C"));
        objectsButtonBar->AddButton(ID_NEW_IMAGE, _(L"Image"), ReadSvgIcon(L"images/image.svg"),
                                    _(L"Insert an image"));
        objectsButtonBar->SetKeyTip(ID_NEW_IMAGE, _DT(L"F"));
        objectsButtonBar->AddButton(ID_NEW_SHAPE, _(L"Shape"), ReadSvgIcon(L"images/shape.svg"),
                                    _(L"Insert a shape"));
        objectsButtonBar->SetKeyTip(ID_NEW_SHAPE, _DT(L"J"));
        objectsButtonBar->AddButton(ID_NEW_COMMON_AXIS, _(L"Axis"), ReadSvgIcon(L"images/axis.svg"),
                                    _(L"Insert an axis"));
        objectsButtonBar->SetKeyTip(ID_NEW_COMMON_AXIS, _DT(L"X"));
        objectsButtonBar->AddButton(ID_NEW_SPACER, _(L"Spacer"), ReadSvgIcon(L"images/spacer.svg"),
                                    _(L"Insert a spacer"));
        objectsButtonBar->SetKeyTip(ID_NEW_SPACER, _DT(L"V"));
        objectsButtonBar->AddDropdownButton(ID_NEW_DIVIDER, _(L"Divider"),
                                            ReadSvgIcon(L"images/divider-horizontal-double.svg"),
                                            _(L"Insert a divider line"));
        objectsButtonBar->SetKeyTip(ID_NEW_DIVIDER, _DT(L"Q"));
        objectsButtonBar->AddButton(wxID_COPY, _(L"Copy"), ReadSvgIcon(L"images/copy.svg"),
                                    _(L"Copy the selected item"));
        objectsButtonBar->SetKeyTip(wxID_COPY, _DT(L"Y"));
        objectsButtonBar->AddButton(wxID_PASTE, _(L"Paste"), ReadSvgIcon(L"images/paste.svg"),
                                    _(L"Paste the copied item"));
        objectsButtonBar->SetKeyTip(wxID_PASTE, _DT(L"U"));
        objectsButtonBar->AddButton(ID_EDIT_ITEM, _(L"Edit"), ReadSvgIcon(L"images/edit.svg"),
                                    _(L"Edit the selected item"));
        objectsButtonBar->SetKeyTip(ID_EDIT_ITEM, _DT(L"W"));
        objectsButtonBar->AddButton(ID_GOTO_DATASOURCE, _(L"Datasource"),
                                    ReadSvgIcon(L"images/data.svg"),
                                    _(L"Jump to this graph's dataset"));
        objectsButtonBar->SetKeyTip(ID_GOTO_DATASOURCE, _DT(L"Z"));
        objectsButtonBar->AddButton(ID_DELETE_ITEM, _(L"Delete"), ReadSvgIcon(L"images/delete.svg"),
                                    _(L"Delete the selected item"));
        objectsButtonBar->SetKeyTip(ID_DELETE_ITEM, _DT(L"OD"));

        // Data tab
        auto* dataPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Data"));
        ribbon->SetPageKeyTip(dataPage, _DT(L"D"));

        // Datasets panel
        auto* dataPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Datasets"));
        dataPanel->SetKeyTip(_DT(L"S"));
        auto* dataButtonBar = new wxRibbonButtonBar(dataPanel, ID_DATASET_BUTTONBAR);

        dataButtonBar->AddButton(ID_INSERT_DATASET, _(L"Add"), ReadSvgIcon(L"images/data-add.svg"),
                                 _(L"Import a dataset into the project"));
        dataButtonBar->SetKeyTip(ID_INSERT_DATASET, _DT(L"N"));
        dataButtonBar->AddButton(ID_EDIT_DATASET, _(L"Edit"), ReadSvgIcon(L"images/data-edit.svg"),
                                 _(L"Edit the selected dataset's import options"));
        dataButtonBar->SetKeyTip(ID_EDIT_DATASET, _DT(L"I"));
        dataButtonBar->AddButton(ID_DELETE_DATASET, _(L"Delete"),
                                 ReadSvgIcon(L"images/data-delete.svg"),
                                 _(L"Delete the selected dataset from the project"));
        dataButtonBar->SetKeyTip(ID_DELETE_DATASET, _DT(L"T"));

        // Transformations panel
        auto* transformPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Transformations"));
        transformPanel->SetKeyTip(_DT(L"R"));
        auto* transformButtonBar = new wxRibbonButtonBar(transformPanel, wxID_ANY);

        transformButtonBar->AddButton(ID_SUBSET_DATASET, _(L"Subset"),
                                      ReadSvgIcon(L"images/subset.svg"),
                                      _(L"Create a subset of a dataset"));
        transformButtonBar->SetKeyTip(ID_SUBSET_DATASET, _DT(L"U"));
        transformButtonBar->AddButton(ID_JOIN_DATASET, _(L"Join"), ReadSvgIcon(L"images/join.svg"),
                                      _(L"Join two datasets"));
        transformButtonBar->SetKeyTip(ID_JOIN_DATASET, _DT(L"J"));
        transformButtonBar->AddButton(ID_PIVOT_WIDER, _(L"Pivot Wider"),
                                      ReadSvgIcon(L"images/pivot-wider.svg"),
                                      _(L"Pivot a dataset wider (unstack)"));
        transformButtonBar->SetKeyTip(ID_PIVOT_WIDER, _DT(L"V"));
        transformButtonBar->AddButton(ID_PIVOT_LONGER, _(L"Pivot Longer"),
                                      ReadSvgIcon(L"images/pivot-longer.svg"),
                                      _(L"Pivot a dataset longer (stack)"));
        transformButtonBar->SetKeyTip(ID_PIVOT_LONGER, _DT(L"O"));

        // Constants panel
        auto* constantsPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Constants"));
        constantsPanel->SetKeyTip(_DT(L"F"));
        auto* constantsButtonBar = new wxRibbonButtonBar(constantsPanel, wxID_ANY);

        constantsButtonBar->AddButton(ID_ADD_CONSTANT, _(L"Add"),
                                      ReadSvgIcon(L"images/constants-add.svg"),
                                      _(L"Add a constant to the project"));
        constantsButtonBar->SetKeyTip(ID_ADD_CONSTANT, _DT(L"B"));
        constantsButtonBar->AddButton(ID_DELETE_CONSTANT, _(L"Delete"),
                                      ReadSvgIcon(L"images/constants-delete.svg"),
                                      _(L"Delete the selected constant"));
        constantsButtonBar->SetKeyTip(ID_DELETE_CONSTANT, _DT(L"C"));

        // Sources panel
        auto* sourcesPanel = new wxRibbonPanel(dataPage, wxID_ANY, _(L"Sources"));
        sourcesPanel->SetKeyTip(_DT(L"Q"));
        auto* sourcesButtonBar = new wxRibbonButtonBar(sourcesPanel, ID_SOURCES_BUTTONBAR);

        sourcesButtonBar->AddButton(ID_GOTO_DATASOURCE, _(L"Datasource"),
                                    ReadSvgIcon(L"images/data.svg"),
                                    _(L"Jump to this graph's dataset"));
        sourcesButtonBar->SetKeyTip(ID_GOTO_DATASOURCE, _DT(L"G"));

        // Analyses tab
        auto* analysesPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Analyses"));
        ribbon->SetPageKeyTip(analysesPage, _DT(L"A"));

        // Graph category panel
        auto* graphPanel = new wxRibbonPanel(analysesPage, wxID_ANY, _(L"Graphs"));
        graphPanel->SetKeyTip(_DT(L"G"));
        auto* graphButtonBar = new wxRibbonButtonBar(graphPanel, ID_GRAPH_BUTTONBAR);

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_BASIC, _(L"Basic"),
                                          ReadSvgIcon(L"images/chart-basic.svg"),
                                          _(L"Basic graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_BASIC, _DT(L"B"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_BUSINESS, _(L"Business"),
                                          ReadSvgIcon(L"images/chart-business.svg"),
                                          _(L"Business graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_BUSINESS, _DT(L"U"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_STATISTICAL, _(L"Statistical"),
                                          ReadSvgIcon(L"images/chart-statistical.svg"),
                                          _(L"Statistical graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_STATISTICAL, _DT(L"S"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SURVEY, _(L"Survey"),
                                          ReadSvgIcon(L"images/chart-survey.svg"),
                                          _(L"Survey data graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_SURVEY, _DT(L"R"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_EDUCATION, _(L"Education"),
                                          ReadSvgIcon(L"images/chart-education.svg"),
                                          _(L"Education graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_EDUCATION, _DT(L"C"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SOCIAL, _(L"Social Sciences"),
                                          ReadSvgIcon(L"images/chart-social.svg"),
                                          _(L"Social sciences graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_SOCIAL, _DT(L"O"));

        graphButtonBar->AddDropdownButton(ID_INSERT_GRAPH_SPORTS, _(L"Sports"),
                                          ReadSvgIcon(L"images/chart-sports.svg"),
                                          _(L"Sports graphs"));
        graphButtonBar->SetKeyTip(ID_INSERT_GRAPH_SPORTS, _DT(L"T"));
        }
    else
        {
        // Print panel
        auto* printPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Page"));
        printPanel->SetKeyTip(_DT(L"R"));
        auto* printButtonBar = new wxRibbonButtonBar(printPanel, wxID_ANY);
        printButtonBar->AddButton(ID_PRINT_SETUP, _(L"Page Layout"),
                                  ReadSvgIcon(L"images/print-setup.svg"),
                                  _(L"Configure page view settings"));
        printButtonBar->SetKeyTip(ID_PRINT_SETUP, _DT(L"G"));

        // Log tab
        LoadRibbonLogPage(ribbon);
        ribbon->SetPageKeyTip(GetMainFrameEx()->GetLogRibbonPage(), _DT(L"L"));
        }

    // Help tab
    auto* helpPage = new wxRibbonPage(ribbon, wxID_ANY, _(L"Help"));
    ribbon->SetPageKeyTip(helpPage, _DT(L"E"));

    ribbon->SetToggleButtonKeyTip(_DT(L"M"));
    ribbon->SetHelpButtonKeyTip(_DT(L"K"));

    if (isProjectRibbon)
        {
        // Tools panel (project frames only, navigates to main frame log tab)
        auto* toolsPanel = new wxRibbonPanel(helpPage, wxID_ANY, _(L"Tools"));
        toolsPanel->SetKeyTip(_DT(L"C"));
        auto* toolsButtonBar = new wxRibbonButtonBar(toolsPanel, wxID_ANY);
        toolsButtonBar->AddButton(ID_VIEW_LOG_REPORT, _(L"Log"),
                                  ReadSvgIcon(L"images/log-book.svg"), _(L"View the log report"));
        toolsButtonBar->SetKeyTip(ID_VIEW_LOG_REPORT, _DT(L"B"));
        }

    auto* aboutPanel = new wxRibbonPanel(helpPage, wxID_ANY, _(L"About"));
    aboutPanel->SetKeyTip(_DT(L"O"));
    auto* aboutButtonBar = new wxRibbonButtonBar(aboutPanel, wxID_ANY);

    aboutButtonBar->AddButton(wxID_ABOUT, _(L"About"), ReadSvgIcon(L"images/wisteria.svg"),
                              _(L"About Wisteria Dataviz"));
    aboutButtonBar->SetKeyTip(wxID_ABOUT, _DT(L"B"));

    ribbon->SetArtProvider(new wxRibbonMSWFlatArtProvider);
    ribbon->Realize();
    ribbon->SetActivePage(homePage);

    return ribbon;
    }

//-------------------------------------------
wxBackstage* WisteriaApp::CreateBackstage(wxWindow* parent, wxRibbonBar* ribbon, wxWindow* content,
                                          wxDocument* doc)
    {
    auto* backstage = new wxBackstage(parent);
    backstage->AddButton(ID_BACKSTAGE_NEW, _(L"New"));

    const int margin = parent->FromDIP(30);

    auto* newPage = backstage->AddPage(ID_BACKSTAGE_NEW);
    auto* newSizer = new wxBoxSizer(wxVERTICAL);
    newSizer->Add(new wxBackstageHeading(newPage, wxID_ANY, _(L"New")),
                  wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    auto* newProjectButton = new wxBackstageButton(
        newPage, ID_BACKSTAGE_NEW_PROJECT, _(L"New Project"),
        GetResourceManager().GetSVG(L"images/wisteria.svg"), wxBackstageButtonStyle::Card);
    newSizer->Add(newProjectButton, wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));
    newPage->SetSizer(newSizer);

    backstage->Bind(
        wxEVT_BUTTON, [this](wxCommandEvent&) { GetDocManager()->CreateNewDocument(); },
        ID_BACKSTAGE_NEW_PROJECT);

    // Open page
    backstage->AddButton(ID_BACKSTAGE_OPEN, _(L"Open"));
    backstage->AddSeparator();
    backstage->AddButton(ID_BACKSTAGE_INFO, _(L"Info"));

    // Save and Save As have no pages, so they are just actions
    backstage->AddButton(ID_BACKSTAGE_SAVE, _(L"Save"));
    backstage->AddButton(ID_BACKSTAGE_SAVE_AS, _(L"Save As"));
    backstage->AddSeparator();
    backstage->AddButton(ID_BACKSTAGE_PRINT, _(L"Print"));
    backstage->AddButton(ID_BACKSTAGE_EXPORT, _(L"Export"));
    backstage->AddFlexibleSpace();
    backstage->AddButton(ID_BACKSTAGE_SETTINGS, _(L"Settings"));
    backstage->AddButton(ID_BACKSTAGE_CLOSE, _(L"Close"));

    auto* openPage = backstage->AddPage(ID_BACKSTAGE_OPEN);
    auto* openSizer = new wxBoxSizer(wxVERTICAL);
    openSizer->Add(new wxBackstageHeading(openPage, wxID_ANY, _(L"Open")),
                   wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    auto* browseButton =
        new wxBackstageButton(openPage, ID_BACKSTAGE_OPEN_BROWSE, _(L"Browse"),
                              wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON),
                              wxBackstageButtonStyle::Wide, _(L"Open a project or data file"));

    auto* openColumns = new wxBoxSizer(wxHORIZONTAL);

    // left column: Browse
    openColumns->Add(browseButton, wxSizerFlags{}.Top().Border(wxLEFT | wxTOP, margin));

    // right column: recent files
    auto* recentColumn = new wxBoxSizer(wxVERTICAL);
    recentColumn->Add(
        new wxBackstageHeading(openPage, wxID_ANY, _(L"Recent"), wxBackstageHeadingStyle::Section),
        wxSizerFlags{}.Border(wxBOTTOM, margin / 2));
    auto* recentList = new wxBackstageMRUList(openPage, ID_BACKSTAGE_RECENT_LIST);
    recentList->SetEmptyText(_(L"You haven't opened any projects recently."));
    recentList->SetMinSize(openPage->FromDIP(wxSize{ 560, 340 }));
    recentColumn->Add(recentList, wxSizerFlags{ 1 }.Expand());
    openColumns->Add(recentColumn, wxSizerFlags{ 1 }.Expand().Border(wxALL, margin));

    openSizer->Add(openColumns, wxSizerFlags{ 1 }.Expand());
    openPage->SetSizer(openSizer);

    // the file history changes while the app runs, so refresh before showing the page
    recentList->SetFiles(*GetDocManager()->GetFileHistory());
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [this, recentList](wxNotifyEvent& event)
        {
            recentList->SetFiles(*GetDocManager()->GetFileHistory());
            event.Skip();
        },
        ID_BACKSTAGE_OPEN);

    backstage->Bind(
        wxEVT_BUTTON,
        [this](wxCommandEvent&)
        {
            wxCommandEvent openEvent(wxEVT_MENU, wxID_OPEN);
            GetDocManager()->ProcessEvent(openEvent);
        },
        ID_BACKSTAGE_OPEN_BROWSE);
    backstage->Bind(
        wxEVT_BACKSTAGE_ITEM_CLICKED,
        [this](wxCommandEvent& event)
        {
            if (GetDocManager()->CreateDocument(event.GetString(), wxDOC_SILENT) == nullptr)
                {
                GetDocManager()->OnOpenFileFailure();
                }
        },
        ID_BACKSTAGE_RECENT_LIST);

    // Print page
    auto* printPage = backstage->AddPage(ID_BACKSTAGE_PRINT);
    auto* printSizer = new wxBoxSizer(wxVERTICAL);
    printSizer->Add(new wxBackstageHeading(printPage, wxID_ANY, _(L"Print")),
                    wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    // Print button with the number of copies
    auto* printTopSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* printButton = new wxBackstageButton(printPage, ID_BACKSTAGE_PRINT_NOW, _(L"Print"),
                                              GetResourceManager().GetSVG(L"images/print.svg"));
    printButton->SetIconSize(wxSize{ 48, 48 });
    printTopSizer->Add(printButton, wxSizerFlags{}.Top());
    auto* copiesSizer = new wxBoxSizer(wxHORIZONTAL);
    copiesSizer->Add(new wxStaticText(printPage, wxID_ANY, _(L"Copies:")),
                     wxSizerFlags{}.CenterVertical().Border(wxRIGHT, margin / 3));
    auto* copiesCtrl = new wxSpinCtrl(printPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                      wxDefaultSize, wxSP_ARROW_KEYS, 1, 999, 1);
    copiesCtrl->SetValidator(wxGenericValidator{ &m_printCopies });
    copiesSizer->Add(copiesCtrl, wxSizerFlags{}.CenterVertical());
    printTopSizer->Add(copiesSizer, wxSizerFlags{}.Top().Border(wxLEFT, margin));
    printSizer->Add(printTopSizer, wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    // settings, which are drop-down buttons showing their current value
    printSizer->Add(new wxBackstageHeading(printPage, wxID_ANY, _(L"Settings"),
                                           wxBackstageHeadingStyle::Section),
                    wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    const auto createSettingButton = [printPage, printSizer, margin]()
    {
        auto* button = new wxBackstageButton(printPage, wxID_ANY, wxString{}, wxBitmapBundle{},
                                             wxBackstageButtonStyle::Wide);
        button->ShowDropDownArrow();
        button->SetMinSize(printPage->FromDIP(wxSize{ 340, -1 }));
        printSizer->Add(button, wxSizerFlags{}.Border(wxLEFT | wxTOP, margin / 2));
        return button;
    };
    auto* orientationButton = createSettingButton();
    auto* paperButton = createSettingButton();
    auto* sidesButton = createSettingButton();
    auto* collateButton = createSettingButton();
    auto* colorButton = createSettingButton();
    printPage->SetSizer(printSizer);

    const std::vector<wxPaperSize> paperSizes{ wxPAPER_LETTER,    wxPAPER_LEGAL, wxPAPER_TABLOID,
                                               wxPAPER_EXECUTIVE, wxPAPER_A3,    wxPAPER_A4,
                                               wxPAPER_A5,        wxPAPER_B4,    wxPAPER_B5 };

    // name of a paper size without its dimensions (e.g., "Letter")
    const auto paperName = [](const wxPaperSize paperId)
    {
        const auto* paperType = wxThePrintPaperDatabase->FindPaperType(paperId);
        return (paperType != nullptr) ? paperType->GetName().BeforeFirst(L',') : wxString{};
    };
    // dimensions of a paper size (e.g., 8.5" x 11" (216 x 279 mm))
    const auto paperDimensions = [](const wxPaperSize paperId)
    {
        const auto* paperType = wxThePrintPaperDatabase->FindPaperType(paperId);
        if (paperType == nullptr)
            {
            return wxString{};
            }
        // paper sizes are in tenths of a millimeter
        const wxSize sizeTenthsMM = paperType->GetSize();
        constexpr double TENTHS_MM_PER_INCH{ 254.0 };
        const auto inches = [](const int tenthsMM)
        {
            return wxNumberFormatter::ToString(tenthsMM / TENTHS_MM_PER_INCH, 2,
                                               wxNumberFormatter::Style_NoTrailingZeroes);
        };
        return wxString::Format(_(L"%s\" x %s\" (%d x %d mm)"), inches(sizeTenthsMM.GetWidth()),
                                inches(sizeTenthsMM.GetHeight()),
                                wxRound(sizeTenthsMM.GetWidth() / 10.0),
                                wxRound(sizeTenthsMM.GetHeight() / 10.0));
    };

    // shows the current print settings on the buttons
    const auto refreshSettingButtons = [this, orientationButton, paperButton, sidesButton,
                                        collateButton, colorButton, paperName, paperDimensions]()
    {
        const auto& settings = GetAppSettings();

        const bool landscape = (GetPrintJobOrientation() == wxLANDSCAPE);
        orientationButton->SetLabel(landscape ? _(L"Landscape Orientation") :
                                                _(L"Portrait Orientation"));
        orientationButton->SetIcon(GetResourceManager().GetSVG(
            landscape ? L"images/page-landscape.svg" : L"images/page-portrait.svg"));

        paperButton->SetLabel(paperName(GetPrintJobPaperId()));
        paperButton->SetDescription(paperDimensions(GetPrintJobPaperId()));
        paperButton->SetIcon(GetResourceManager().GetSVG(L"images/paper-size.svg"));

        const auto duplex = settings->GetPrintDuplex();
        sidesButton->SetLabel((duplex == wxDUPLEX_SIMPLEX) ? _(L"Print on One Side") :
                                                             _(L"Print on Both Sides"));
        sidesButton->SetDescription((duplex == wxDUPLEX_SIMPLEX) ? _(L"Only print on one side") :
                                    (duplex == wxDUPLEX_HORIZONTAL) ?
                                                                   _(L"Flip pages on short edge") :
                                                                   _(L"Flip pages on long edge"));
        sidesButton->SetIcon(GetResourceManager().GetSVG((duplex == wxDUPLEX_SIMPLEX) ?
                                                             L"images/print-one-sided.svg" :
                                                             L"images/print-two-sided.svg"));

        const bool collated = settings->IsPrintCollated();
        collateButton->SetLabel(collated ? _(L"Collated") : _(L"Uncollated"));
        collateButton->SetDescription(collated ? _(L"1,2,3   1,2,3   1,2,3") :
                                                 _(L"1,1,1   2,2,2   3,3,3"));
        collateButton->SetIcon(GetResourceManager().GetSVG(
            collated ? L"images/print-collated.svg" : L"images/print-uncollated.svg"));

        const bool color = settings->IsPrintColor();
        colorButton->SetLabel(color ? _(L"Color") : _(L"Grayscale"));
        colorButton->SetDescription(
            color ? _(L"Print in color") : _(L"Print in shades of gray (if supported by printer)"));
        colorButton->SetIcon(GetResourceManager().GetSVG(color ? L"images/print-color.svg" :
                                                                 L"images/print-grayscale.svg"));

        for (auto* button :
             { orientationButton, paperButton, sidesButton, collateButton, colorButton })
            {
            button->Refresh();
            }
        orientationButton->GetParent()->Layout();
    };
    refreshSettingButtons();

    // shows a menu below a button, calling onChosen with the chosen item's index
    const auto popupChoices = [](wxWindow* anchor, const std::vector<wxString>& labels,
                                 const size_t current, const std::function<void(size_t)>& onChosen)
    {
        wxMenu menu;
        std::vector<int> itemIds;
        for (size_t i = 0; i < labels.size(); ++i)
            {
            auto* item = menu.AppendRadioItem(wxID_ANY, labels[i]);
            item->Check(i == current);
            itemIds.push_back(item->GetId());
            }
        menu.Bind(wxEVT_MENU,
                  [&itemIds, &onChosen](wxCommandEvent& event)
                  {
                      const auto itemPos =
                          std::find(itemIds.cbegin(), itemIds.cend(), event.GetId());
                      if (itemPos != itemIds.cend())
                          {
                          onChosen(static_cast<size_t>(std::distance(itemIds.cbegin(), itemPos)));
                          }
                  });
        anchor->PopupMenu(&menu, wxPoint{ 0, anchor->GetSize().GetHeight() });
    };

    // any change to the settings is saved back to the app's print settings
    orientationButton->Bind(
        wxEVT_BUTTON,
        [this, orientationButton, popupChoices, refreshSettingButtons](wxCommandEvent&)
        {
            popupChoices(orientationButton,
                         { _(L"Portrait Orientation"), _(L"Landscape Orientation") },
                         (GetPrintJobOrientation() == wxLANDSCAPE) ? 1 : 0,
                         [this, refreshSettingButtons](const size_t index)
                         {
                             m_printJobOrientation = (index == 1) ? wxLANDSCAPE : wxPORTRAIT;
                             refreshSettingButtons();
                         });
        });

    paperButton->Bind(wxEVT_BUTTON,
                      [this, paperButton, popupChoices, refreshSettingButtons, paperSizes,
                       paperName](wxCommandEvent&)
                      {
                          // the current paper size is always offered, even if it isn't a common one
                          std::vector<wxPaperSize> choices{ paperSizes };
                          if (std::find(choices.cbegin(), choices.cend(), GetPrintJobPaperId()) ==
                              choices.cend())
                              {
                              choices.push_back(GetPrintJobPaperId());
                              }
                          std::vector<wxString> labels;
                          size_t current{ 0 };
                          for (size_t i = 0; i < choices.size(); ++i)
                              {
                              labels.push_back(paperName(choices[i]));
                              if (choices[i] == GetPrintJobPaperId())
                                  {
                                  current = i;
                                  }
                              }
                          popupChoices(paperButton, labels, current,
                                       [this, choices, refreshSettingButtons](const size_t index)
                                       {
                                           m_printJobPaperId = choices[index];
                                           refreshSettingButtons();
                                       });
                      });

    sidesButton->Bind(
        wxEVT_BUTTON,
        [this, sidesButton, popupChoices, refreshSettingButtons](wxCommandEvent&)
        {
            // the order matches wxDuplexMode
            popupChoices(sidesButton,
                         { _(L"Print on One Side"), _(L"Print on Both Sides (Flip on Short Edge)"),
                           _(L"Print on Both Sides (Flip on Long Edge)") },
                         static_cast<size_t>(GetAppSettings()->GetPrintDuplex()),
                         [this, refreshSettingButtons](const size_t index)
                         {
                             GetAppSettings()->SetPrintDuplex(static_cast<wxDuplexMode>(index));
                             GetAppSettings()->SaveSettingsFile();
                             refreshSettingButtons();
                         });
        });

    collateButton->Bind(wxEVT_BUTTON,
                        [this, collateButton, popupChoices, refreshSettingButtons](wxCommandEvent&)
                        {
                            popupChoices(collateButton,
                                         { _(L"Collated (1,2,3   1,2,3   1,2,3)"),
                                           _(L"Uncollated (1,1,1   2,2,2   3,3,3)") },
                                         GetAppSettings()->IsPrintCollated() ? 0 : 1,
                                         [this, refreshSettingButtons](const size_t index)
                                         {
                                             GetAppSettings()->SetPrintCollated(index == 0);
                                             GetAppSettings()->SaveSettingsFile();
                                             refreshSettingButtons();
                                         });
                        });

    colorButton->Bind(wxEVT_BUTTON,
                      [this, colorButton, popupChoices, refreshSettingButtons](wxCommandEvent&)
                      {
                          popupChoices(colorButton, { _(L"Color"), _(L"Grayscale") },
                                       GetAppSettings()->IsPrintColor() ? 0 : 1,
                                       [this, refreshSettingButtons](const size_t index)
                                       {
                                           GetAppSettings()->SetPrintColor(index == 0);
                                           GetAppSettings()->SaveSettingsFile();
                                           refreshSettingButtons();
                                       });
                      });

    printPage->TransferDataToWindow();

    // other project windows may have changed the settings, so refresh before showing the page
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [refreshSettingButtons](wxNotifyEvent& event)
        {
            refreshSettingButtons();
            event.Skip();
        },
        ID_BACKSTAGE_PRINT);

    // the project window does the printing
    backstage->Bind(
        wxEVT_BUTTON,
        [this, backstage, printPage](wxCommandEvent&)
        {
            printPage->TransferDataFromWindow();
            wxCommandEvent printEvent(wxEVT_MENU, ID_BACKSTAGE_PRINT_NOW);
            printEvent.SetInt(m_printCopies);
            backstage->ProcessWindowEvent(printEvent);
        },
        ID_BACKSTAGE_PRINT_NOW);

    // Export page, with its own side panel of export types
    auto* exportPage = backstage->AddPage(ID_BACKSTAGE_EXPORT);
    auto* exportSizer = new wxBoxSizer(wxVERTICAL);
    exportSizer->Add(new wxBackstageHeading(exportPage, wxID_ANY, _(L"Export")),
                     wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    struct ExportType
        {
        wxWindowID m_commandId;
        wxString m_name;
        wxString m_buttonLabel;
        wxString m_iconPath;
        std::vector<wxString> m_bullets;
        };

    const std::vector<ExportType> exportTypes{
        { ID_HTML_EXPORT,
          _(L"HTML Dashboard"),
          _(L"Export Dashboard"),
          L"images/dashboard.svg",
          { _(L"Exports all of the project's pages to an interactive HTML dashboard."),
            _(L"Opens in any web browser, with no extra software needed."),
            _(L"Choose the theme and color mode in the export options.") } },
        { ID_SVG_EXPORT,
          _(L"SVG"),
          _(L"Export SVG"),
          L"images/report.svg",
          { _(L"Exports all of the project's pages to a single SVG file."),
            _(L"Vector graphics stay sharp at any size."),
            _(L"Transitions, highlighting, and a slideshow can be included.") } },
        { ID_PDF_EXPORT,
          _(L"PDF"),
          _(L"Export PDF"),
          L"images/pdf.svg",
          { _(L"Exports all of the project's pages to a PDF document."),
            _(L"Easy to share and print, and looks the same on any device.") } },
        { ID_PPTX_EXPORT,
          _(L"PowerPoint"),
          _(L"Export PowerPoint"),
          L"images/powerpoint.svg",
          { _(L"Creates a PowerPoint presentation from the project's pages."),
            _(L"Opens in PowerPoint and other presentation software.") } },
        { ID_ODP_EXPORT,
          _(L"OpenDocument Presentation"),
          _(L"Export ODP"),
          L"images/odp.svg",
          { _(L"Creates an OpenDocument presentation from the project's pages."),
            _(L"Opens in LibreOffice Impress and other presentation software.") } }
    };

    auto* exportColumns = new wxBoxSizer(wxHORIZONTAL);
    auto* exportTypesSizer = new wxBoxSizer(wxVERTICAL);
    auto* exportBook = new wxSimplebook(exportPage);
    std::vector<wxBackstageButton*> exportNavButtons;

    for (const auto& exportType : exportTypes)
        {
        auto* navButton = new wxBackstageButton(exportPage, wxID_ANY, exportType.m_name,
                                                GetResourceManager().GetSVG(exportType.m_iconPath),
                                                wxBackstageButtonStyle::Wide);
        navButton->SetMinSize(exportPage->FromDIP(wxSize{ 240, -1 }));
        exportTypesSizer->Add(navButton, wxSizerFlags{}.Border(wxBOTTOM, margin / 4));
        exportNavButtons.push_back(navButton);

        // the export button and a bulleted list describing it
        auto* detailPanel = new wxPanel(exportBook);
        auto* detailSizer = new wxBoxSizer(wxVERTICAL);
        auto* exportButton =
            new wxBackstageButton(detailPanel, exportType.m_commandId, exportType.m_buttonLabel,
                                  GetResourceManager().GetSVG(exportType.m_iconPath));
        exportButton->SetIconSize(wxSize{ 64, 64 });
        detailSizer->Add(exportButton, wxSizerFlags{}.Border(wxBOTTOM, margin / 2));
        for (const auto& bullet : exportType.m_bullets)
            {
            auto* bulletSizer = new wxBoxSizer(wxHORIZONTAL);
            bulletSizer->Add(new wxStaticText(detailPanel, wxID_ANY, wxString{ L"•" }),
                             wxSizerFlags{}.Top().Border(wxRIGHT, margin / 3));
            auto* bulletText = new wxStaticText(detailPanel, wxID_ANY, bullet);
            bulletText->Wrap(detailPanel->FromDIP(420));
            bulletSizer->Add(bulletText, wxSizerFlags{ 1 }.Top());
            detailSizer->Add(bulletSizer, wxSizerFlags{}.Border(wxBOTTOM, margin / 4));
            }
        detailPanel->SetSizer(detailSizer);
        exportBook->AddPage(detailPanel, exportType.m_name);
        }

    // shows an export type's details and highlights its side panel button
    const auto selectExportType = [backstage, exportBook, exportNavButtons](const size_t index)
    {
        const wxColour pageColor = backstage->GetPageBackgroundColour();
        const bool darkPage = (backstage->GetPageForegroundColour().GetLuminance() > 0.5);
        const wxColour selectedColor = pageColor.ChangeLightness(darkPage ? 130 : 92);
        exportBook->SetSelection(index);
        for (size_t i = 0; i < exportNavButtons.size(); ++i)
            {
            exportNavButtons[i]->SetCalloutColour((i == index) ? selectedColor : wxColour{});
            }
        exportBook->GetParent()->Layout();
    };
    for (size_t i = 0; i < exportNavButtons.size(); ++i)
        {
        exportNavButtons[i]->Bind(wxEVT_BUTTON,
                                  [selectExportType, i](wxCommandEvent&) { selectExportType(i); });
        }

    // the project window does the exporting (through its regular export handlers)
    for (const auto& exportType : exportTypes)
        {
        backstage->Bind(
            wxEVT_BUTTON,
            [backstage, commandId = exportType.m_commandId](wxCommandEvent&)
            {
                wxCommandEvent exportEvent(wxEVT_MENU, commandId);
                backstage->ProcessWindowEvent(exportEvent);
            },
            exportType.m_commandId);
        }

    exportColumns->Add(exportTypesSizer, wxSizerFlags{}.Top().Border(wxLEFT | wxTOP, margin));
    exportColumns->Add(exportBook, wxSizerFlags{ 1 }.Expand().Border(wxALL, margin));
    exportSizer->Add(exportColumns, wxSizerFlags{ 1 }.Expand());
    exportPage->SetSizer(exportSizer);
    selectExportType(0);

    // Info page
    auto* infoPage = backstage->AddPage(ID_BACKSTAGE_INFO);
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);
    infoSizer->Add(new wxBackstageHeading(infoPage, wxID_ANY, _(L"Info")),
                   wxSizerFlags{}.Border(wxLEFT | wxTOP, margin));

    auto* infoColumns = new wxBoxSizer(wxHORIZONTAL);

    // project name, folder, and file actions
    auto* infoMainSizer = new wxBoxSizer(wxVERTICAL);
    auto* projectName =
        new wxBackstageHeading(infoPage, wxID_ANY, wxString{}, wxBackstageHeadingStyle::Section);
    infoMainSizer->Add(projectName, wxSizerFlags{}.Border(wxBOTTOM, margin / 6));
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* projectFolder = new wxStaticText(infoPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                           wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
    projectFolder->SetMinSize(infoPage->FromDIP(wxSize{ 420, -1 }));
    pathSizer->Add(projectFolder, wxSizerFlags{ 1 }.CenterVertical());
    auto* copyPathButton = new wxBitmapButton(
        infoPage, ID_BACKSTAGE_COPY_PATH, GetResourceManager().GetSVG(L"images/copy.svg"),
        wxDefaultPosition, infoPage->FromDIP(wxSize{ 16, 16 }), wxBORDER_SIMPLE);
    pathSizer->Add(copyPathButton, wxSizerFlags{}.CenterVertical().Border(wxLEFT, margin / 4));
    infoMainSizer->Add(pathSizer, wxSizerFlags{}.Border(wxBOTTOM, margin / 2));
    infoColumns->Add(infoMainSizer, wxSizerFlags{ 1 }.Border(wxLEFT | wxTOP, margin));

    // properties, dates, and people
    auto* infoPropertiesSizer = new wxBoxSizer(wxVERTICAL);
    auto* infoGrid = new wxFlexGridSizer(2, wxSize{ margin, margin / 6 });
    const auto addInfoSection = [infoPage, infoGrid, margin](const wxString& title)
    {
        infoGrid->Add(
            new wxBackstageHeading(infoPage, wxID_ANY, title, wxBackstageHeadingStyle::Section),
            wxSizerFlags{}.Border(wxTOP, margin / 2));
        infoGrid->AddSpacer(0);
    };
    const auto addInfoRow = [infoPage, infoGrid](const wxString& label)
    {
        infoGrid->Add(new wxStaticText(infoPage, wxID_ANY, label));
        auto* value = new wxStaticText(infoPage, wxID_ANY, wxString{});
        infoGrid->Add(value);
        return value;
    };
    addInfoSection(_(L"Properties"));
    auto* sizeValue = addInfoRow(_(L"Size"));
    auto* pagesValue = addInfoRow(_(L"Pages"));
    auto* datasourcesValue = addInfoRow(_(L"Datasources"));
    addInfoSection(_(L"Related Dates"));
    auto* modifiedValue = addInfoRow(_(L"Last Modified"));
    auto* createdValue = addInfoRow(_(L"Created"));
    infoPropertiesSizer->Add(infoGrid);
    infoColumns->Add(infoPropertiesSizer, wxSizerFlags{}.Top().Border(wxALL, margin));

    infoSizer->Add(infoColumns, wxSizerFlags{ 1 }.Expand());
    infoPage->SetSizer(infoSizer);

    // everything that can be known about the project's file
    const auto refreshInfo = [doc, projectName, projectFolder, copyPathButton, sizeValue,
                              pagesValue, datasourcesValue, modifiedValue, createdValue, infoPage]()
    {
        const wxString notSaved{ _(L"Not saved yet") };
        const wxFileName projectFile{ doc->GetFilename() };
        const bool fileExists = projectFile.IsOk() && projectFile.FileExists();

        projectName->SetLabel(doc->GetUserReadableName());
        projectFolder->SetLabel(fileExists ? projectFile.GetPath() : notSaved);
        copyPathButton->Enable(fileExists);

        const auto* view = dynamic_cast<WisteriaView*>(doc->GetFirstView());
        pagesValue->SetLabel((view != nullptr) ? wxNumberFormatter::ToString(
                                                     static_cast<long>(view->GetPages().size())) :
                                                 wxString{});
        // only the imported datasets, not the pivots, subsets, or merges derived from them
        datasourcesValue->SetLabel(
            (view != nullptr) ? wxNumberFormatter::ToString(static_cast<long>(
                                    view->GetReportBuilder().GetDatasetImportOptions().size())) :
                                wxString{});

        wxDateTime modified;
        wxDateTime created;
        if (fileExists)
            {
            projectFile.GetTimes(nullptr, &modified, &created);
            }
        const auto formatDate = [&notSaved](const wxDateTime& date)
        { return date.IsValid() ? date.FormatDate() + L" " + date.FormatTime() : notSaved; };
        sizeValue->SetLabel(fileExists ? projectFile.GetHumanReadableSize() : notSaved);
        modifiedValue->SetLabel(formatDate(modified));
        createdValue->SetLabel(formatDate(created));

        infoPage->Layout();
    };
    refreshInfo();

    // the project may have been saved since the page was last shown
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [refreshInfo](wxNotifyEvent& event)
        {
            refreshInfo();
            event.Skip();
        },
        ID_BACKSTAGE_INFO);

    copyPathButton->Bind(wxEVT_BUTTON,
                         [doc](wxCommandEvent&)
                         {
                             if (wxTheClipboard->Open())
                                 {
                                 wxTheClipboard->SetData(new wxTextDataObject(doc->GetFilename()));
                                 wxTheClipboard->Close();
                                 }
                         });

    // Settings has no page, so it is just an action that opens the project's settings
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [backstage](wxNotifyEvent&)
        {
            wxCommandEvent settingsEvent(wxEVT_MENU, ID_PROJECT_SETTINGS);
            backstage->ProcessWindowEvent(settingsEvent);
        },
        ID_BACKSTAGE_SETTINGS);

    // Close has no page, so it is just an action
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [this, doc](wxNotifyEvent&)
        {
            // closing destroys the project window that this backstage lives in
            CallAfter(
                [this, doc]()
                {
                    if (GetDocManager()->GetDocuments().Find(doc) != nullptr)
                        {
                        GetDocManager()->CloseDocument(doc);
                        }
                });
        },
        ID_BACKSTAGE_CLOSE);

    // route the actions to the project window's regular Save and Save As handlers
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [backstage](wxNotifyEvent&)
        {
            wxCommandEvent saveEvent(wxEVT_MENU, ID_SAVE_PROJECT);
            backstage->ProcessWindowEvent(saveEvent);
        },
        ID_BACKSTAGE_SAVE);
    backstage->Bind(
        wxEVT_BACKSTAGE_CLICKED,
        [backstage](wxNotifyEvent&)
        {
            wxCommandEvent saveAsEvent(wxEVT_MENU, ID_SAVE_PROJECT_AS);
            backstage->ProcessWindowEvent(saveAsEvent);
        },
        ID_BACKSTAGE_SAVE_AS);

    backstage->Hide();
    ribbon->SetBackstage(backstage, content);
    return backstage;
    }

//-------------------------------------------
void WisteriaApp::InitProjectSidebar()
    {
    // fill in the icons for the projects' sidebars
    // Do NOT change the ordering of these (indices are used by LoadProject());
    // new ones always get added at the bottom.
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/data.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/page.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/constants.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/pivot-wider.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/pivot-longer.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/subset.svg"));
    m_projectSideBarImageList.emplace_back(ReadSvgIcon(L"images/join.svg"));
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
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BulletChart)))
        {
        return _DT(L"bullet-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaterfallChart)))
        {
        return _DT(L"waterfall-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::FunnelChart)))
        {
        return _DT(L"funnel-chart");
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
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChoroplethMap)))
        {
        return _DT(L"choropleth-map");
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
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::RaceTrackChart)))
        {
        return _DT(L"race-track-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WilmarthBridgePlot)))
        {
        return _DT(L"wilmarth-bridge-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::NightingaleRoseChart)))
        {
        return _DT(L"nightingale-rose-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::DuBoisSpiralChart)))
        {
        return _DT(L"dubois-spiral-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::DuelingPieChart)))
        {
        return _DT(L"dueling-pie-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Pictograph)))
        {
        return _DT(L"pictograph");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::StemAndLeafPlot)))
        {
        return _DT(L"stem-and-leaf-plot");
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
        return L"images/bubbleplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        return L"images/scatterplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::MultiSeriesLinePlot)))
        {
        return L"images/lineplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
        {
        return L"images/wcurve.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        return L"images/lineplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        return L"images/candlestick.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        return L"images/chernoffplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        return L"images/gantt.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
        {
        return L"images/histogram.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        return L"images/likert7.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CategoricalBarChart)))
        {
        return L"images/barchart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScaleChart)))
        {
        return L"images/scale.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BulletChart)))
        {
        return L"images/bulletchart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BarChart)))
        {
        return L"images/barchart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        return L"images/boxplot.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        return L"images/heatmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChoroplethMap)))
        {
        return L"images/choropleth.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        return L"images/piechart.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        return L"images/table.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        return L"images/sankey.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        return L"images/waffle.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::RaceTrackChart)))
        {
        return L"images/racetrack.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WilmarthBridgePlot)))
        {
        return L"images/wilmarth-bridge.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::NightingaleRoseChart)))
        {
        return L"images/rose.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::DuBoisSpiralChart)))
        {
        return L"images/dubois-spiral.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::DuelingPieChart)))
        {
        return L"images/dueling-pie.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Pictograph)))
        {
        return L"images/pictograph.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::StemAndLeafPlot)))
        {
        return L"images/stem-leaf.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WordCloud)))
        {
        return L"images/wordcloud.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        return L"images/roadmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        return L"images/roadmap.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        return L"images/sparkline.svg";
        }
    // non-graph items
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Axis)))
        {
        return L"images/axis.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
        {
        const auto* label = dynamic_cast<const Wisteria::GraphItems::Label*>(item);
        if (label == nullptr)
            {
            return L"images/label.svg";
            }

        const auto spacerType = GetSpacerType(*label);
        if (spacerType == Wisteria::SpacerType::EmptySpacer ||
            spacerType == Wisteria::SpacerType::Spacer)
            {
            return L"images/spacer.svg";
            }

        switch (GetDividerType(*label))
            {
        case Wisteria::DividerType::HorizontalSingleLine:
            return L"images/divider-horizontal-single.svg";
        case Wisteria::DividerType::HorizontalDoubleLine:
            return L"images/divider-horizontal-double.svg";
        case Wisteria::DividerType::VerticalSingleLine:
            return L"images/divider-vertical-single.svg";
        case Wisteria::DividerType::VerticalDoubleLine:
            return L"images/divider-vertical-double.svg";
        case Wisteria::DividerType::NotDivider:
        default:
            return L"images/label.svg";
            }
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend)))
        {
        return L"images/label.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Image)))
        {
        return L"images/image.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::FillableShape)))
        {
        return L"images/shape.svg";
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Shape)))
        {
        return L"images/shape.svg";
        }

    return {};
    }

//-------------------------------------------
const std::vector<Wisteria::GalleryItemInfo>& WisteriaApp::GetGalleryItemCatalog()
    {
    using Wisteria::GalleryGroup;
    using Wisteria::GalleryItemBehavior;
    using Wisteria::GalleryItemType;

    static const std::vector<Wisteria::GalleryItemInfo> catalog = {
        // Objects
        { GalleryItemType::Label, _(L"Label"), L"images/label.svg", GalleryGroup::Objects },
        { GalleryItemType::KpiCard, _(L"KPI Card"), L"images/kpi-card.svg", GalleryGroup::Objects },
        { GalleryItemType::Image, _(L"Image"), L"images/image.svg", GalleryGroup::Objects },
        { GalleryItemType::Shape, _(L"Shape"), L"images/shape.svg", GalleryGroup::Objects },
        { GalleryItemType::Axis, _(L"Axis"), L"images/axis.svg", GalleryGroup::Objects,
          GalleryItemBehavior::RequiresExistingGraphs },
        { GalleryItemType::Spacer, _(L"Spacer"), L"images/spacer.svg", GalleryGroup::Objects },
        { GalleryItemType::DividerHorizontalSingle, _(L"Divider: Horizontal (Single)"),
          L"images/divider-horizontal-single.svg", GalleryGroup::Objects },
        { GalleryItemType::DividerHorizontalDouble, _(L"Divider: Horizontal (Double)"),
          L"images/divider-horizontal-double.svg", GalleryGroup::Objects },
        { GalleryItemType::DividerVerticalSingle, _(L"Divider: Vertical (Single)"),
          L"images/divider-vertical-single.svg", GalleryGroup::Objects },
        { GalleryItemType::DividerVerticalDouble, _(L"Divider: Vertical (Double)"),
          L"images/divider-vertical-double.svg", GalleryGroup::Objects },
        // Basic
        { GalleryItemType::BarChart, _(L"Bar Chart"), L"images/barchart.svg", GalleryGroup::Basic },
        { GalleryItemType::PieChart, _(L"Pie Chart"), L"images/piechart.svg", GalleryGroup::Basic },
        { GalleryItemType::LinePlot, _(L"Line Plot"), L"images/lineplot.svg", GalleryGroup::Basic },
        { GalleryItemType::MultiSeriesLinePlot, _(L"Multi-Series Line Plot"),
          L"images/lineplot.svg", GalleryGroup::Basic },
        { GalleryItemType::Table, _(L"Table"), L"images/table.svg", GalleryGroup::Basic },
        { GalleryItemType::SankeyDiagram, _(L"Sankey Diagram"), L"images/sankey.svg",
          GalleryGroup::Basic },
        { GalleryItemType::WaffleChart, _(L"Waffle Chart"), L"images/waffle.svg",
          GalleryGroup::Basic },
        { GalleryItemType::RaceTrackChart, _(L"Race Track Chart"), L"images/racetrack.svg",
          GalleryGroup::Basic },
        { GalleryItemType::NightingaleRoseChart, _(L"Nightingale Rose Chart"), L"images/rose.svg",
          GalleryGroup::Basic },
        { GalleryItemType::DuBoisSpiralChart, _(L"Du Bois Spiral Chart"),
          L"images/dubois-spiral.svg", GalleryGroup::Basic },
        { GalleryItemType::DuelingPieChart, _(L"Dueling Pie Chart"), L"images/dueling-pie.svg",
          GalleryGroup::Basic },
        { GalleryItemType::Pictograph, _(L"Pictograph"), L"images/pictograph.svg",
          GalleryGroup::Basic },
        { GalleryItemType::ChoroplethMap, _(L"Choropleth Map"), L"images/choropleth.svg",
          GalleryGroup::Basic },
        // Business
        { GalleryItemType::GanttChart, _(L"Gantt Chart"), L"images/gantt.svg",
          GalleryGroup::Business },
        { GalleryItemType::CandlestickPlot, _(L"Candlestick Plot"), L"images/candlestick.svg",
          GalleryGroup::Business },
        { GalleryItemType::BulletChart, _(L"Bullet Chart"), L"images/bulletchart.svg",
          GalleryGroup::Business },
        { GalleryItemType::WaterfallChart, _(L"Waterfall Chart"), L"images/waterfallchart.svg",
          GalleryGroup::Business },
        { GalleryItemType::FunnelChart, _(L"Funnel Chart"), L"images/funnel.svg",
          GalleryGroup::Business },
        // Statistical
        { GalleryItemType::Histogram, _(L"Histogram"), L"images/histogram.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::BoxPlot, _(L"Box Plot"), L"images/boxplot.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::StemAndLeafPlot, _(L"Stem-and-Leaf Plot"), L"images/stem-leaf.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::HeatMap, _(L"Heat Map"), L"images/heatmap.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::ScatterPlot, _(L"Scatter Plot"), L"images/scatterplot.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::BubblePlot, _(L"Bubble Plot"), L"images/bubbleplot.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::ChernoffFacesPlot, _(L"Chernoff Faces Plot"), L"images/chernoffplot.svg",
          GalleryGroup::Statistical },
        { GalleryItemType::WilmarthBridgePlot, _(L"Wilmarth Bridge Plot"),
          L"images/wilmarth-bridge.svg", GalleryGroup::Statistical },
        // Survey
        { GalleryItemType::LikertChart, _(L"Likert Chart"), L"images/likert7.svg",
          GalleryGroup::Survey },
        { GalleryItemType::WordCloud, _(L"Word Cloud"), L"images/wordcloud.svg",
          GalleryGroup::Survey },
        { GalleryItemType::ProConRoadmap, _(L"Pro && Con Roadmap"), L"images/roadmap.svg",
          GalleryGroup::Survey },
        // Education
        { GalleryItemType::ScaleChart, _(L"Scale Chart"), L"images/scale.svg",
          GalleryGroup::Education },
        // Social Sciences
        { GalleryItemType::WCurvePlot, _(L"W-Curve Plot"), L"images/wcurve.svg",
          GalleryGroup::Social },
        { GalleryItemType::LRRoadmap, _(L"Linear Regression Roadmap"), L"images/roadmap.svg",
          GalleryGroup::Social },
        // Sports
        { GalleryItemType::WinLossSparkline, _(L"Win/Loss Sparkline"), L"images/sparkline.svg",
          GalleryGroup::Sports },
    };

    return catalog;
    }

//-------------------------------------------
Wisteria::SpacerType WisteriaApp::GetSpacerType(const Wisteria::GraphItems::Label& label)
    {
    // a divider label (visible, possibly with empty text but an outline pen) is not a spacer
    if (!label.GetText().empty() || label.IsShown())
        {
        return Wisteria::SpacerType::NotSpacer;
        }

    return (label.GetCanvasHeightProportion().has_value() &&
            compare_doubles(label.GetCanvasHeightProportion().value(), 0.0)) ?
               Wisteria::SpacerType::EmptySpacer :
               Wisteria::SpacerType::Spacer;
    }

//-------------------------------------------
Wisteria::DividerType WisteriaApp::GetDividerType(const Wisteria::GraphItems::Label& label)
    {
    // a divider is a shown, textless label with an outline pen configured
    if (!label.GetText().empty() || !label.IsShown() || !label.GetPen().IsOk())
        {
        return Wisteria::DividerType::NotDivider;
        }

    const auto& info = label.GetGraphItemInfo();
    if (info.IsShowingTopOutline() || info.IsShowingBottomOutline())
        {
        return (info.IsShowingTopOutline() && info.IsShowingBottomOutline()) ?
                   Wisteria::DividerType::HorizontalDoubleLine :
                   Wisteria::DividerType::HorizontalSingleLine;
        }
    if (info.IsShowingLeftOutline() || info.IsShowingRightOutline())
        {
        return (info.IsShowingLeftOutline() && info.IsShowingRightOutline()) ?
                   Wisteria::DividerType::VerticalDoubleLine :
                   Wisteria::DividerType::VerticalSingleLine;
        }

    return Wisteria::DividerType::NotDivider;
    }
