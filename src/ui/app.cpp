///////////////////////////////////////////////////////////////////////////////
// Name:        app.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "app.h"
#include "../base/label.h"
#include "../util/hardwareinfo.h"
#include "wx/xrc/xh_bmp.h"
#include "wx/xrc/xh_menu.h"
#include "wx/xrc/xmlres.h"
#include <wx/regex.h>
#include <wx/stc/stc.h>
#include <wx/xml/xml.h>

/// @brief Temporarily turn off AppName being appended to @c wxStandardPaths calls.
/// @private
class NoAppInfoAppend
    {
  public:
    NoAppInfoAppend() { wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_None); }

    ~NoAppInfoAppend() { wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_AppName); }
    };

namespace Wisteria::UI
    {
    //----------------------------------------------------------
    BaseApp::BaseApp()
        {
#if defined(__VISUALC__) && defined(wxUSE_ON_FATAL_EXCEPTION)
        // call this to tell the library to call our OnFatalException()
        wxHandleFatalExceptions();
#endif
        }

    //----------------------------------------------------------
    void BaseApp::OnFatalException() { GenerateReport(wxDebugReport::Context_Exception); }

    //----------------------------------------------------------
    bool BaseApp::OnInit()
        {
        if (!wxApp::OnInit())
            {
            return false;
            }

        [[maybe_unused]]
        const auto wxStringToFsPath = [](const wxString& str)
        {
#ifdef _WIN32
            return std::filesystem::path(str.wc_str()); // UTF-16 on Windows
#else
            return std::filesystem::path(str.utf8_str().data()); // UTF-8 on POSIX
#endif
        };

        // prepare profile report (only if compiled with profiling)
        m_profileReportPath =
            wxString(wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() +
                     GetAppName() + L" Profile.dat");
        SET_PROFILER_REPORT_PATH(wxStringToFsPath(m_profileReportPath));
        DUMP_PROFILER_REPORT(); // flush out data in temp file from previous run

        // Logs will be written to file now, delete the old logging system.
        m_logFile = new LogFile{ !IsAppendingDailyLog() };
        delete wxLog::SetActiveTarget(m_logFile);

        // fix color mapping on Windows
        wxSystemOptions::SetOption(DONTTRANSLATE(L"msw.remap"), 0);

        // Set the locale (for number formatting, etc.) and load translation catalog locations.
        // (Note that constructing the wxLocale object is needed for
        //  localizing the C runtime functions.)
        m_locale = new wxLocale(wxLANGUAGE_DEFAULT, wxLOCALE_LOAD_DEFAULT);
        wxUILocale::UseDefault();

        wxInitAllImageHandlers();
        wxPropertyGrid::RegisterAdditionalEditors();
        wxFileSystem::AddHandler(new wxZipFSHandler);
        wxFileSystem::AddHandler(new wxMemoryFSHandler);

        // load the XRC handlers
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        wxXmlResource::Get()->AddHandler(new wxIconXmlHandler);
        wxXmlResource::Get()->AddHandler(new wxMenuXmlHandler);
        wxXmlResource::Get()->AddHandler(new wxMenuBarXmlHandler);

        // create the document manager
        SetDocManager(new Wisteria::UI::DocManager);

        wxDialog::EnableLayoutAdaptation(true);

        return true;
        }

    //----------------------------------------------------------
    int BaseApp::OnExit()
        {
        wxLogDebug(__func__);
        SaveFileHistoryMenu();
        wxDELETE(m_docManager);
        wxDELETE(m_locale);

        return wxApp::OnExit();
        }

    //----------------------------------------------------------
    void BaseApp::LogSystemInfo() const
        {
        // log some system information
        wxDateTime buildDate;
        buildDate.ParseDate(__DATE__);
        wxLogMessage(L"Log File Location: %s", m_logFile->GetLogFilePath());
        wxLogMessage(L"%s %s (build %s)", GetAppName(), GetAppSubName(),
                     buildDate.Format(L"%G.%m.%d"));
        wxLogMessage(L"App Location: %s", wxStandardPaths::Get().GetExecutablePath());
        wxLogMessage(wxVERSION_STRING);
        wxLogMessage(L"OS: %s", wxGetOsDescription());
#ifdef __WXGTK__
        wxLogMessage(L"Linux Info: %s",
                     wxPlatformInfo::Get().GetLinuxDistributionInfo().Description);
        wxLogMessage(L"Desktop Environment: %s", wxPlatformInfo::Get().GetDesktopEnvironment());
#endif
        wxLogMessage(L"CPU Architecture: %s", wxGetCpuArchitectureName());
        wxLogMessage(L"CPU Count: %d", wxThread::GetCPUCount());
#ifdef _OPENMP
        wxLogMessage(L"OpenMP Version: %s", std::to_wstring(_OPENMP));
#endif
        if (const auto physicalMemory = wxSystemHardwareInfo::GetMemory(); physicalMemory != -1)
            {
            wxLogMessage(L"Physical Memory: %s",
                         wxFileName::GetHumanReadableSize(
                             static_cast<wxULongLong>(physicalMemory.GetValue())));
            }
        if (wxGraphicsRenderer::GetDefaultRenderer() != nullptr)
            {
            wxLogMessage(L"Graphics Renderer: %s",
                         wxGraphicsRenderer::GetDefaultRenderer()->GetName());
            }
#ifdef __WXMSW__
        if (wxGraphicsRenderer::GetDirect2DRenderer() != nullptr)
            {
            wxLogMessage(L"Direct2D Rendering: available; will attempt to use Direct2D");
            }
        else
            {
            wxLogMessage(L"Direct2D Rendering: unavailable");
            }
#endif
        wxLogMessage(L"Web Engine: %s",
                     wxWebSession::GetDefault().GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"Code Editor: %s",
                     wxStyledTextCtrl::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"XML Parser: %s", wxXmlDocument::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"JPEG Library: %s",
                     wxJPEGHandler::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"PNG Library: %s", wxPNGHandler::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"TIFF Library: %s",
                     wxTIFFHandler::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"RegEx Library: %s", wxRegEx::GetLibraryVersionInfo().GetVersionString());
        wxLogMessage(L"Wisteria-Dataviz: %s", Wisteria::GetLibraryVersionInfo().GetVersionString());

        wxLogMessage(L"Default System Font: %s, %d pt.",
                     wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName(),
                     wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
        wxLogMessage(L"Screen Size: %d wide, %d tall", wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
                     wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));
        if (!wxSystemSettings::GetAppearance().GetName().empty())
            {
            wxLogMessage(L"System Theme: %s", wxSystemSettings::GetAppearance().GetName());
            }

        wxLogMessage(L"System Language: %s", wxUILocale::GetCurrent().GetName());
        wxLogMessage(L"System Encoding: %s", wxLocale::GetSystemEncodingName());
        wxLogMessage(L"Resources Location: %s", wxStandardPaths::Get().GetResourcesDir());
        wxLogMessage(
            L"Translation Catalogs Location: %s",
            wxStandardPaths::Get().GetLocalizedResourcesDir(wxUILocale::GetCurrent().GetName(),
                                                            wxStandardPaths::ResourceCat_Messages));
        // log any command lines
        if (argc > 1)
            {
            wxString cmdline;
            for (int i = 1; i < argc; ++i)
                {
                if (i > 1)
                    {
                    cmdline += L" ";
                    }
                cmdline += argv[i];
                }
            if (!cmdline.empty())
                {
                wxLogMessage(L"Command Line: %s", cmdline);
                }
            }
        }

    //----------------------------------------------------------
    wxBitmap BaseApp::ReadSvgIcon(const wxString& path,
                                  const wxSize baseSize /*= wxSize{ 32, 32 }*/)
        {
        const auto contentScalingFactor{ GetMainFrame()->GetContentScaleFactor() };
        const wxSize buttonSize =
            GetMainFrame()->FromDIP(wxSize{ wxRound(baseSize.GetWidth() * contentScalingFactor),
                                            wxRound(baseSize.GetHeight() * contentScalingFactor) });
        wxBitmap loadedImage{ GetResourceManager().GetSVG(path).GetBitmap(buttonSize) };
        wxASSERT_MSG(loadedImage.IsOk(), L"Failed to load SVG image.");
        loadedImage.SetScaleFactor(contentScalingFactor);
        return loadedImage;
        }

    //----------------------------------------------------------
    wxBitmap BaseApp::CreateSplashscreen(const wxBitmap& bitmap, const wxString& appName,
                                         const wxString& appSubName, const wxString& vendorName,
                                         const bool includeCopyright,
                                         const wxString& copyrightPrefix /*= wxString{}*/)
        {
        wxASSERT_MSG(bitmap.IsOk(), L"Invalid base image for splashscreen");
        const int ftSize = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
        const auto backscreenHeight = bitmap.GetLogicalHeight() * math_constants::fifth;

        const wxString theFontName{ Wisteria::GraphItems::Label::GetFirstAvailableFont(
            { DONTTRANSLATE(L"Roboto"), DONTTRANSLATE(L"Orbitron"), DONTTRANSLATE(L"Georgia") }) };

        wxBitmap canvasBmp(bitmap);
        wxMemoryDC memDC(canvasBmp);
        wxGCDC gcdc(memDC);

        // prepare font for drawing the app name
        GraphItems::Label appLabel(
            GraphItems::GraphItemInfo(appName)
                .Pen(wxNullPen)
                .Font(wxFont(ftSize, wxFONTFAMILY_DECORATIVE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, theFontName))
                .FontColor(wxColour{ 255, 255, 255 })
                .DPIScaling(1.0)
                .Anchoring(Anchoring::TopLeftCorner)
                .AnchorPoint({ 0, 0 })
                .Padding(4, 0, 4, 4));
        auto boundingBox = appLabel.GetBoundingBox(gcdc);
        const auto fontUpscaling =
            safe_divide<double>(backscreenHeight, boundingBox.GetHeight()) * math_constants::half;
        appLabel.SetScaling(std::max(fontUpscaling, 1.0));

        wxRect bottomBackScreen(0, canvasBmp.GetLogicalHeight() - backscreenHeight,
                                canvasBmp.GetLogicalWidth(), backscreenHeight);

            // draw translucent backscreens on image so that text written on it can be read
            {
            const wxDCPenChanger pc{ gcdc, wxColour{ 0, 0, 0 } };
            const wxDCBrushChanger bc{ gcdc, wxBrush{ wxColour{ 61, 60, 59, 175 } } };
            gcdc.DrawRectangle(wxRect(0, 0, canvasBmp.GetLogicalWidth(), backscreenHeight));
            gcdc.DrawLine(0, backscreenHeight, canvasBmp.GetLogicalWidth(), backscreenHeight);
            if (includeCopyright)
                {
                gcdc.DrawRectangle(bottomBackScreen);
                gcdc.DrawLine(0, canvasBmp.GetLogicalHeight() - backscreenHeight,
                              canvasBmp.GetLogicalWidth(),
                              canvasBmp.GetLogicalHeight() - backscreenHeight);
                }
            }

        const auto spacePos = appName.find(L' ');

        if (spacePos == wxString::npos)
            {
            appLabel.Draw(gcdc);
            }
        else
            {
            // write the app name with alternating font colors
            appLabel.SetText(appName.substr(0, spacePos));
            boundingBox = appLabel.GetBoundingBox(gcdc);
            appLabel.Draw(gcdc);

            appLabel.GetGraphItemInfo().Padding(4, 0, 4, 2);
            appLabel.Offset(boundingBox.GetWidth(), 0);
            appLabel.SetText(appName.substr(spacePos + 1));
            boundingBox = appLabel.GetBoundingBox(gcdc);
            appLabel.Draw(gcdc);

            appLabel.GetGraphItemInfo().FontColor(wxColour{ L"#F89522" }).Padding(4, 4, 4, 2);
            appLabel.Offset(boundingBox.GetWidth(), 0);
            appLabel.SetText(appSubName);
            appLabel.Draw(gcdc);
            }

        if (includeCopyright)
            {
            // draw the copyright at the bottom
            wxDateTime buildDate;
            buildDate.ParseDate(__DATE__);

            GraphItems::Label copyrightInfo(
                GraphItems::GraphItemInfo(wxString::Format(L"%s\u00A9%d %s. %s", copyrightPrefix,
                                                           buildDate.GetYear(), vendorName,
                                                           _(L"All rights reserved.")))
                    .Pen(wxNullPen)
                    .Font(wxFont(ftSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_NORMAL, false, theFontName))
                    .FontColor(wxColour{ 255, 255, 255 })
                    .Padding(4, 4, 4, 4)
                    .DPIScaling(1.0)
                    .Anchoring(Anchoring::BottomRightCorner));

            const auto adjustedLeft{ bottomBackScreen.GetWidth() * math_constants::quarter };
            bottomBackScreen.SetWidth(bottomBackScreen.GetWidth() * math_constants::three_quarters);
            bottomBackScreen.SetLeft(adjustedLeft);
            copyrightInfo.SetBoundingBox(bottomBackScreen, gcdc, 1.0);
            copyrightInfo.SetAnchorPoint(bottomBackScreen.GetBottomRight());
            copyrightInfo.SetPageVerticalAlignment(PageVerticalAlignment::BottomAligned);

            copyrightInfo.Draw(gcdc);
            }

        // draw a border around the image
        gcdc.SetPen(wxColour{ 241, 241, 241 });
        const auto penWidth = gcdc.GetPen().GetWidth();
        gcdc.DrawLine(0, 0, 0, canvasBmp.GetLogicalHeight() - penWidth);
        gcdc.DrawLine(0, canvasBmp.GetLogicalHeight() - penWidth,
                      canvasBmp.GetLogicalWidth() - penWidth,
                      canvasBmp.GetLogicalHeight() - penWidth);
        gcdc.DrawLine(canvasBmp.GetLogicalWidth() - penWidth,
                      canvasBmp.GetLogicalHeight() - penWidth,
                      canvasBmp.GetLogicalWidth() - penWidth, 0);
        gcdc.DrawLine(canvasBmp.GetLogicalWidth() - penWidth, 0, 0, 0);

        memDC.SelectObject(wxNullBitmap);

        return canvasBmp;
        }

    //----------------------------------------------------------
    void BaseApp::GenerateReport(const wxDebugReport::Context ctx) const
        {
        auto* report = new wxDebugReportCompress;

        // add all standard files: currently this means just a minidump and an
        // XML file with system info and stack trace
        report->AddAll(ctx);

        const wxDateTime dt = wxDateTime::Now();
        report->AddText(L"Timestamp.log", dt.FormatISODate() + L' ' + dt.FormatISOTime(),
                        _(L"Timestamp of this report"));

        report->AddFile(m_logFile->GetLogFilePath(), _(L"Log Report"));
        wxString settingsPath = wxStandardPaths::Get().GetUserDataDir() +
                                wxFileName::GetPathSeparator() + L"Settings.xml";
        if (!wxFile::Exists(settingsPath))
            {
            settingsPath = wxStandardPaths::Get().GetUserDataDir() +
                           wxFileName::GetPathSeparator() + GetAppName() +
                           wxFileName::GetPathSeparator() + L"Settings.xml";
            }
        report->AddFile(settingsPath, _(L"Settings File"));

        if (wxDebugReportPreviewStd().Show(*report))
            {
            report->Process();
            const wxString newReportPath = wxFileName(wxStandardPaths::Get().GetDocumentsDir(),
                                                      GetAppName() + " CrashReport.zip")
                                               .GetFullPath();
            if (wxCopyFile(report->GetCompressedFileName(), newReportPath, true))
                {
                wxMessageBox(
                    wxString::Format(_(L"An error report has been saved to:\n\"%s\".\n\n"
                                       "Please email this file to %s to have this issue reviewed. "
                                       "Thank you for your patience."),
                                     newReportPath, m_supportEmail),
                    _(L"Error Report"), wxOK | wxICON_INFORMATION);
#ifdef __WXMSW__
                ShellExecute(NULL, DONTTRANSLATE(L"open"),
                             wxStandardPaths::Get().GetDocumentsDir().wc_str(), NULL, NULL,
                             SW_SHOWNORMAL);
#endif
                }
            }

        delete report;
        }

    //----------------------------------------------------------
    void BaseApp::SaveFileHistoryMenu()
        {
        wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
        config.SetPath(DONTTRANSLATE(L"Recent File List", DTExplanation::SystemEntry,
                                     L"This goes into the registry"));
        GetDocManager()->FileHistorySave(config);
        }

    //----------------------------------------------------------
    void BaseApp::LoadFileHistoryMenu()
        {
        if ((GetMainFrame()->GetMenuBar() != nullptr) &&
            (GetMainFrame()->GetMenuBar()->GetMenuCount() != 0U))
            {
            GetDocManager()->FileHistoryUseMenu(GetMainFrame()->GetMenuBar()->GetMenu(0));
            }
        // load the file history
        wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
        config.SetPath(DONTTRANSLATE(L"Recent File List", DTExplanation::SystemEntry));
        GetDocManager()->FileHistoryLoad(config);
        }

    //----------------------------------------------------------
    void BaseApp::ClearFileHistoryMenu()
        {
        while (GetDocManager()->GetHistoryFilesCount() != 0U)
            {
            GetDocManager()->GetFileHistory()->RemoveFileFromHistory(0);
            }
        }

    //----------------------------------------------------------
    wxString BaseApp::FindResourceFile(const wxString& subFile) const
        {
        // Resources folder + file (OSX uses this)
        wxString foundFile =
            wxStandardPaths::Get().GetResourcesDir() + wxFileName::GetPathSeparator() + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }

        const NoAppInfoAppend noAppInfo;

        // all users' data dir + file
        foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetConfigDir(), subFile);
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // user data dir + file
        foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetUserConfigDir(), subFile);
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // data dir + file
        foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetDataDir(), subFile);
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
#ifdef __WXOSX__
        // centralized location for all users on macOS
        foundFile = _DT(L"/Library/Application Support/") + wxTheApp->GetAppName() + L"/" + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
#endif
        // Some special logic for UNIX-like systems, where prefix logic is all over the map.
        // Sometimes the program might be installed to a different prefix than what
        // wxWidgets is detecting.
#ifdef __UNIX__
        // this is usually the default
        foundFile = FindResourceFileWithAppInfo(_DT(L"/usr/local/share/"), subFile);
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // older systems might do this
        foundFile = FindResourceFileWithAppInfo(_DT(L"/usr/share/"), subFile);
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
#endif
        // ...or, program dir + file
        foundFile =
            wxFileName{ wxStandardPaths::Get().GetExecutablePath() }.GetPathWithSep() + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // ...or, cwd + file
        foundFile = wxFileName::GetCwd() + wxFileName::GetPathSeparator() + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // give up, can't find it anywhere
        wxLogWarning(L"'%s': unable to find resource file.", subFile);
        return {};
        }

    //----------------------------------------------------------
    wxString BaseApp::FindResourceDirectory(const wxString& subDir) const
        {
        // Resources folder + file (OSX uses this)
        wxString foundFolder =
            wxStandardPaths::Get().GetResourcesDir() + wxFileName::GetPathSeparator() + subDir;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }

        const NoAppInfoAppend noAppInfo;

        // all users' data dir + subfolder
        foundFolder =
            FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetConfigDir(), subDir);
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // user data dir + subfolder
        foundFolder =
            FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetUserConfigDir(), subDir);
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // data dir + subfolder
        foundFolder = FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetDataDir(), subDir);
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
#ifdef __WXOSX__
        // centralized location for all users on macOS
        foundFolder =
            _DT(L"/Library/Application Support/") + wxTheApp->GetAppName() + L"/" + subDir;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
#endif
        // Some special logic for UNIX-like systems, where prefix logic is all over the map.
        // Sometimes the program might be installed to a different prefix than what
        // wxWidgets is detecting.
#if defined(__UNIX__)
        // this is usually the default
        foundFolder = FindResourceDirectoryWithAppInfo(_DT(L"/usr/local/share/"), subDir);
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // older systems might do this
        foundFolder = FindResourceDirectoryWithAppInfo(_DT(L"/usr/share/"), subDir);
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
#endif
        // ...or, program dir + subfolder
        foundFolder =
            wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPathWithSep() + subDir;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // ...or, cwd + subfolder
        foundFolder = wxFileName::GetCwd() + wxFileName::GetPathSeparator() + subDir;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // give up, can't find it anywhere
        return {};
        }

    //----------------------------------------------------------
    wxString BaseApp::FindResourceFileWithAppInfo(const wxString& folder,
                                                  const wxString& subFile) const
        {
        wxString appFolderNameNoSpaces = GetAppName();
        appFolderNameNoSpaces.Replace(L" ", wxString{}, true);

        if (appFolderNameNoSpaces.empty())
            {
            return {};
            }

        // try the folder + program name + file
        wxString foundFile = folder + wxFileName::GetPathSeparator() + GetAppName() +
                             wxFileName::GetPathSeparator() + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        foundFile = folder + wxFileName::GetPathSeparator() + appFolderNameNoSpaces +
                    wxFileName::GetPathSeparator() + subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        // try the folder + vendor name + program name + file
        foundFile = folder + wxFileName::GetPathSeparator() + GetVendorName() +
                    wxFileName::GetPathSeparator() + GetAppName() + wxFileName::GetPathSeparator() +
                    subFile;
        if (wxFileName::FileExists(foundFile))
            {
            return foundFile;
            }
        return {};
        }

    //----------------------------------------------------------
    wxString BaseApp::FindResourceDirectoryWithAppInfo(const wxString& folder,
                                                       const wxString& subFolder) const
        {
        wxString appFolderNameNoSpaces = GetAppName();
        appFolderNameNoSpaces.Replace(L" ", wxString{}, true);

        if (appFolderNameNoSpaces.empty())
            {
            return {};
            }

        // try the folder + program name + file
        wxString foundFolder = folder + wxFileName::GetPathSeparator() + GetAppName() +
                               wxFileName::GetPathSeparator() + subFolder;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        foundFolder = folder + wxFileName::GetPathSeparator() + appFolderNameNoSpaces +
                      wxFileName::GetPathSeparator() + subFolder;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        // try the folder + vendor name + program name + file
        foundFolder = folder + wxFileName::GetPathSeparator() + GetVendorName() +
                      wxFileName::GetPathSeparator() + GetAppName() +
                      wxFileName::GetPathSeparator() + subFolder;
        if (wxFileName::DirExists(foundFolder))
            {
            return foundFolder;
            }
        return {};
        }

    //----------------------------------------------------------
    wxWindow* BaseApp::GetParentingWindow()
        {
        if (GetMainFrame() != nullptr && GetMainFrame()->IsShown())
            {
            return GetMainFrame();
            }
        if (GetDocManager() != nullptr)
            {
            // active document window
            if (GetDocManager()->GetCurrentDocument() != nullptr &&
                GetDocManager()->GetCurrentDocument()->GetDocumentWindow() != nullptr &&
                GetDocManager()->GetCurrentDocument()->GetDocumentWindow()->IsShown())
                {
                return GetDocManager()->GetCurrentDocument()->GetDocumentWindow();
                }
            // first document window that is visible
            for (const auto& doc : GetDocManager()->GetDocumentsVector())
                {
                if (doc != nullptr && doc->GetDocumentWindow() != nullptr &&
                    doc->GetDocumentWindow()->IsShown())
                    {
                    return doc->GetDocumentWindow();
                    }
                }
            }
        return GetTopWindow();
        }
    } // namespace Wisteria::UI
