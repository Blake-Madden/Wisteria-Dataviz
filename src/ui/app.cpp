///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "app.h"
#include "../base/label.h"
#ifdef __WXMSW__
    #include <psapi.h>
    #include <debugapi.h>
#elif defined(__UNIX__)
    #include <sys/resource.h>
    #include <sys/sysinfo.h>
#endif
#include <wx/stc/stc.h>
#include <wx/xml/xml.h>

using namespace Wisteria;

/// @brief Temporarily turn off AppName being appended to @c wxStandardPaths calls.
/// @private
class NoAppInfoAppend
    {
public:
    NoAppInfoAppend()
        { wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_None); }
    ~NoAppInfoAppend()
        { wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_AppName); }
    };

//----------------------------------------------------------
Wisteria::UI::BaseApp::BaseApp()
    {
#if defined (__VISUALC__) && defined (wxUSE_ON_FATAL_EXCEPTION)
    // call this to tell the library to call our OnFatalException()
    wxHandleFatalExceptions();
#endif
    }

//----------------------------------------------------------
void Wisteria::UI::BaseApp::OnFatalException()
    {
    GenerateReport(wxDebugReport::Context_Exception);
    }

//----------------------------------------------------------
bool Wisteria::UI::BaseApp::OnInit()
    {
    // prepare profile report (only if compiled with profiling)
    m_profileReportPath = wxString(wxStandardPaths::Get().GetTempDir() +
                                   wxFileName::GetPathSeparator() + GetAppName() +
                                   L" Profile.dat");
    SET_PROFILER_REPORT_PATH(m_profileReportPath.mb_str());
    DUMP_PROFILER_REPORT(); // flush out data in temp file from previous run

    // Logs will be written to file now, delete the old logging system.
    m_logFile = new LogFile{ !IsAppendingDailyLog() };
    delete wxLog::SetActiveTarget(m_logFile);

    // log some system information
    wxDateTime buildDate;
    buildDate.ParseDate(__DATE__);
    wxLogMessage(L"Log File Location: %s", m_logFile->GetLogFilePath());
    wxLogMessage(L"%s %s (build %s)", GetAppName(), GetAppSubName(), buildDate.Format(L"%G.%m.%d"));
    wxLogMessage(L"App Location: %s", wxStandardPaths::Get().GetExecutablePath());
    wxLogMessage(wxVERSION_STRING);
    wxLogMessage(L"OS: %s", wxGetOsDescription());
#ifdef __WXGTK__
    wxLogMessage(L"Linux Info: %s", wxPlatformInfo::Get().GetLinuxDistributionInfo().Description);
    wxLogMessage(L"Desktop Environment: %s", wxPlatformInfo::Get().GetDesktopEnvironment());
#endif
    wxLogMessage(L"CPU Architecture: %s", wxGetCpuArchitectureName());
    wxLogMessage(L"CPU Count: %d", wxThread::GetCPUCount());
#ifdef _OPENMP 
    wxLogMessage(L"OpenMP Version: %s", std::to_wstring(_OPENMP));
#endif
#ifdef __WXMSW__
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (::GlobalMemoryStatusEx(&status))
        {
        wxLogMessage(L"Physical Memory (Total): %.02f Gbs.",
                     safe_divide<double>(status.ullTotalPhys, 1024 * 1024 * 1024));
        wxLogMessage(L"Physical Memory (Available): %.02f Gbs.",
                     safe_divide<double>(status.ullAvailPhys, 1024 * 1024 * 1024));
        }
#elif defined(__UNIX__)
    struct sysinfo status{};
    if (sysinfo(&status) == 0)
        {
        wxLogMessage(L"Physical Memory: %.02f Gbs.",
                     safe_divide<double>(status.totalram, 1024 * 1024 * 1024));
        }
#endif
    if (wxGraphicsRenderer::GetDefaultRenderer())
        {
        wxLogMessage(L"Graphics Renderer: %s",
            wxGraphicsRenderer::GetDefaultRenderer()->GetName());
        }
#ifdef __WXMSW__
    if (wxGraphicsRenderer::GetDirect2DRenderer())
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
    wxLogMessage(L"XML Parser: %s",
                 wxXmlDocument::GetLibraryVersionInfo().GetVersionString());
    wxLogMessage(L"JPEG Library: %s",
                 wxJPEGHandler::GetLibraryVersionInfo().GetVersionString());
    wxLogMessage(L"PNG Library: %s",
                 wxPNGHandler::GetLibraryVersionInfo().GetVersionString());
    wxLogMessage(L"TIFF Library: %s",
                 wxTIFFHandler::GetLibraryVersionInfo().GetVersionString());
    wxLogMessage(L"RegEx Library: %s",
                 wxRegEx::GetLibraryVersionInfo().GetVersionString());

    wxLogMessage(L"Default System Font: %s, %d pt.",
        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName(),
        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
    wxLogMessage(L"Screen Size: %d wide, %d tall",
        wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
        wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));
    if (wxSystemSettings::GetAppearance().GetName().length())
        {
        wxLogMessage(L"System Theme: %s",
                     wxSystemSettings::GetAppearance().GetName());
        }

    // subroutine to log the system colors
    const auto logSystemColor = [](const wxSystemColour color, const wxString& description)
        {
        if (const auto sysColor = wxSystemSettings::GetColour(color); sysColor.IsOk())
            {
            wxLogVerbose(L"%s: %s %s", description, sysColor.GetAsString(wxC2S_HTML_SYNTAX),
                wxTheColourDatabase->FindName(sysColor.GetRGB()).MakeCapitalized());
            }
        };

    logSystemColor(wxSYS_COLOUR_WINDOW, DONTTRANSLATE(L"Window Color"));
    logSystemColor(wxSYS_COLOUR_MENU, DONTTRANSLATE(L"Menu Color"));
    logSystemColor(wxSYS_COLOUR_WINDOWFRAME, DONTTRANSLATE(L"Window Frame Color"));
    logSystemColor(wxSYS_COLOUR_BTNFACE, DONTTRANSLATE(L"Dialog/Controls Color"));
    logSystemColor(wxSYS_COLOUR_HIGHLIGHT, DONTTRANSLATE(L"Highlighted Item Color"));
    logSystemColor(wxSYS_COLOUR_WINDOWTEXT, DONTTRANSLATE(L"Window Text Color"));
    logSystemColor(wxSYS_COLOUR_MENUTEXT, DONTTRANSLATE(L"Menu Text Color"));
    logSystemColor(wxSYS_COLOUR_HIGHLIGHTTEXT, DONTTRANSLATE(L"Highlighted Text Color"));
    logSystemColor(wxSYS_COLOUR_GRAYTEXT, DONTTRANSLATE(L"Grayed Text Color"));
    logSystemColor(wxSYS_COLOUR_HOTLIGHT, DONTTRANSLATE(L"Hyperlink Color"));

    // fix color mapping on Windows
    wxSystemOptions::SetOption(DONTTRANSLATE(L"msw.remap"), 0);

    // set the locale (for number formatting, etc.) and load any translations
    wxUILocale::UseDefault();

    wxLogMessage(L"System Language: %s", wxUILocale::GetCurrent().GetName());
    wxLogMessage(L"Translation Catalogs Location: %s",
        wxStandardPaths::Get().GetLocalizedResourcesDir(
            wxUILocale::GetCurrent().GetName(),
            wxStandardPaths::ResourceCat_Messages));

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
int Wisteria::UI::BaseApp::OnExit()
    {
    wxLogDebug(__func__);
    SaveFileHistoryMenu();
    wxDELETE(m_docManager);

// dump max memory usage
#ifndef NDEBUG
    #ifdef __WXMSW__
        // https://docs.microsoft.com/en-us/windows/win32/psapi/collecting-memory-usage-information-for-a-process?redirectedfrom=MSDN
        PROCESS_MEMORY_COUNTERS memCounter;
        ::ZeroMemory(&memCounter, sizeof(PROCESS_MEMORY_COUNTERS));
        if (::GetProcessMemoryInfo(::GetCurrentProcess(), &memCounter, sizeof(memCounter)))
            {
            const wxString memMsg = wxString::Format(L"Peak Memory Usage: %.02f Gbs.",
                // PeakWorkingSetSize is in bytes
                safe_divide<double>(memCounter.PeakWorkingSetSize, 1024*1024*1024));
            wxLogDebug(memMsg);
            wxPrintf(memMsg + L"\n");
            OutputDebugString(wxString{ memMsg + L"\n" }.wc_str());
            }
    #elif defined(__UNIX__)
        rusage usage;
        memset(&usage, 0, sizeof(rusage));
        if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
            const wxString memMsg = wxString::Format(L"Peak Memory Usage: %.02f Gbs.",
                // ru_maxrss is in kilobytes
                safe_divide<double>(usage.ru_maxrss, 1024*1024));
            wxLogDebug(memMsg);
            wxPrintf(memMsg + L"\n");
            }
    #endif
#endif
    return wxApp::OnExit();
    }

//----------------------------------------------------------
wxBitmap Wisteria::UI::BaseApp::CreateSplashscreen(const wxBitmap& bitmap, const wxString& appName,
                                     const wxString& appSubName, const wxString& vendorName,
                                     const bool includeCopyright)
    {
    const int ftSize = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
    const auto backscreenHeight = bitmap.GetHeight() * math_constants::fifth;

    wxBitmap canvasBmp(bitmap);
    wxMemoryDC memDC(canvasBmp);
    wxGCDC gcdc(memDC);

    // prepare font for drawing the app name
    Wisteria::GraphItems::Label appLabel(
            Wisteria::GraphItems::GraphItemInfo(appName).
            Pen(wxNullPen).
            Font(wxFont(ftSize, wxFONTFAMILY_DECORATIVE, wxFONTSTYLE_NORMAL,
                        wxFONTWEIGHT_BOLD, false,
                        DONTTRANSLATE(L"Georgia"))).
            FontColor(wxColour(L"#315184")).DPIScaling(1.0).
            Anchoring(Anchoring::TopLeftCorner).AnchorPoint({ 0, 0 }).
            Padding(4, 0, 4, 4));
    auto boundingBox = appLabel.GetBoundingBox(gcdc);
    const auto fontUpscaling = safe_divide<double>(backscreenHeight, boundingBox.GetHeight()) *
        math_constants::half;
    appLabel.SetScaling(fontUpscaling);

    // draw translucent backscreens on image so that text written on it can be read
        {
        wxDCPenChanger pc(gcdc, *wxBLACK_PEN);
        wxDCBrushChanger bc(gcdc, wxBrush(wxColour(255, 255, 255, 174)));
        gcdc.DrawRectangle(wxRect(0,0,canvasBmp.GetWidth(), backscreenHeight));
        gcdc.DrawLine(0, backscreenHeight, canvasBmp.GetWidth(), backscreenHeight);
        if (includeCopyright)
            {
            gcdc.DrawRectangle(wxRect(0,canvasBmp.GetHeight() -
                               backscreenHeight,canvasBmp.GetWidth(), backscreenHeight));
            gcdc.DrawLine(0, canvasBmp.GetHeight()-backscreenHeight,
                          canvasBmp.GetWidth(), canvasBmp.GetHeight()-backscreenHeight);
            }
        }

    const auto spacePos = appName.find(L' ');

    if (spacePos == std::wstring::npos)
        { appLabel.Draw(gcdc); }
    else
        {
        // write the app name with alternating font colors
        appLabel.SetText(appName.substr(0, spacePos));
        boundingBox = appLabel.GetBoundingBox(gcdc);
        appLabel.Draw(gcdc);

        appLabel.GetGraphItemInfo().FontColor(wxColour(2, 186, 2)).
            Padding(4, 0, 4, 0);
        appLabel.Offset(boundingBox.GetWidth(), 0);
        appLabel.SetText(appName.substr(spacePos+1));
        boundingBox = appLabel.GetBoundingBox(gcdc);
        appLabel.Draw(gcdc);

        appLabel.GetGraphItemInfo().FontColor(*wxBLACK).
            Padding(4, 4, 4, 2);
        appLabel.Offset(boundingBox.GetWidth(), 0);
        appLabel.SetText(appSubName);
        appLabel.Draw(gcdc);
        }

    if (includeCopyright)
        {
        // draw the copyright at the bottom
        wxDateTime buildDate;
        buildDate.ParseDate(__DATE__);

        Wisteria::GraphItems::Label copyrightInfo(
            Wisteria::GraphItems::GraphItemInfo(
            wxString::Format(L"\x00A9%d %s. %s",
                buildDate.GetYear(), vendorName,
                _(L"All rights reserved."))).
            Pen(wxNullPen).
            Font(wxFont(ftSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                        wxFONTWEIGHT_NORMAL, false,
                GraphItems::Label::GetFirstAvailableMonospaceFont())).
            FontColor(*wxBLACK).Padding(4, 4, 4, 4).DPIScaling(1.0).
            Anchoring(Anchoring::BottomRightCorner).
            AnchorPoint(wxRect{ gcdc.GetSize() }.GetBottomRight()));
        copyrightInfo.SetScaling(fontUpscaling * math_constants::third);

        copyrightInfo.Draw(gcdc);
        }

    // draw a border around the image
    gcdc.SetPen(*wxLIGHT_GREY_PEN);
    const auto penWidth = gcdc.GetPen().GetWidth();
    gcdc.DrawLine(0, 0, 0, canvasBmp.GetHeight()-penWidth);
    gcdc.DrawLine(0, canvasBmp.GetHeight()-penWidth,
                  canvasBmp.GetWidth()-penWidth, canvasBmp.GetHeight()-penWidth);
    gcdc.DrawLine(canvasBmp.GetWidth()-penWidth, canvasBmp.GetHeight()-penWidth,
                  canvasBmp.GetWidth()-penWidth, 0);
    gcdc.DrawLine(canvasBmp.GetWidth()-penWidth, 0, 0, 0);

    memDC.SelectObject(wxNullBitmap);

    return canvasBmp;
    }

//----------------------------------------------------------
void Wisteria::UI::BaseApp::GenerateReport(wxDebugReport::Context ctx)
    {
    wxDebugReportCompress* report = new wxDebugReportCompress;

    // add all standard files: currently this means just a minidump and an
    // XML file with system info and stack trace
    report->AddAll(ctx);

    const wxDateTime dt = wxDateTime::Now();
    report->AddText(L"Timestamp.log", dt.FormatISODate() + L' ' +
                    dt.FormatISOTime(), _(L"Timestamp of this report"));

    report->AddFile(m_logFile->GetLogFilePath(), _(L"Log Report"));
    wxString settingsPath = wxStandardPaths::Get().GetUserDataDir() +
                            wxFileName::GetPathSeparator() + L"Settings.xml";
    if (!wxFile::Exists(settingsPath))
        {
        settingsPath = wxStandardPaths::Get().GetUserDataDir() +
                       wxFileName::GetPathSeparator() + GetAppName() +
                       wxFileName::GetPathSeparator() + L"Settings.xml"; }
    report->AddFile(settingsPath, _(L"Settings File"));

    if (wxDebugReportPreviewStd().Show(*report) )
        {
        report->Process();
        const wxString newReportPath = wxStandardPaths::Get().GetDocumentsDir() +
                                 wxFileName::GetPathSeparator() + GetAppName() +
                                 L"CrashReport.zip";
        if (wxCopyFile(report->GetCompressedFileName(), newReportPath, true))
            {
            wxMessageBox(wxString::Format(_(L"An error report has been saved to:\n\"%s\".\n\n"
                "Please email this file to %s to have this issue reviewed. "
                "Thank you for your patience."), newReportPath, m_supportEmail),
                _(L"Error Report"), wxOK | wxICON_INFORMATION);
        #ifdef __WXMSW__
            ShellExecute(NULL, DONTTRANSLATE(L"open"),
                wxStandardPaths::Get().GetDocumentsDir().wc_str(),
                NULL, NULL, SW_SHOWNORMAL);
        #endif
            }
        }

    delete report;
    }

//----------------------------------------------------------
void Wisteria::UI::BaseApp::SaveFileHistoryMenu()
    {
    wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
    config.SetPath(DONTTRANSLATE(L"Recent File List",
                                 DTExplanation::SystemEntry, L"This goes into the registry"));
    GetDocManager()->FileHistorySave(config);
    }

//----------------------------------------------------------
void Wisteria::UI::BaseApp::LoadFileHistoryMenu()
    {
    if (GetMainFrame()->GetMenuBar() && GetMainFrame()->GetMenuBar()->GetMenuCount() )
        {
        GetDocManager()->FileHistoryUseMenu(GetMainFrame()->GetMenuBar()->GetMenu(0));
        }
    // load the file history
    wxConfig config(GetAppName() + DONTTRANSLATE(L"MRU"), GetVendorName());
    config.SetPath(DONTTRANSLATE(L"Recent File List", DTExplanation::SystemEntry));
    GetDocManager()->FileHistoryLoad(config);
    }

//----------------------------------------------------------
void Wisteria::UI::BaseApp::ClearFileHistoryMenu()
    {
    while (GetDocManager()->GetHistoryFilesCount())
        { GetDocManager()->GetFileHistory()->RemoveFileFromHistory(0); }
    }

//----------------------------------------------------------
wxString Wisteria::UI::BaseApp::FindResourceFile(const wxString& subFile) const
    {
    // Resources folder + file (OSX uses this)
    wxString foundFile = wxStandardPaths::Get().GetResourcesDir() +
        wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }

    NoAppInfoAppend noAppInfo;

    // all users' data dir + file    
    foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetConfigDir(), subFile);
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // user data dir + file
    foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetUserConfigDir(), subFile);
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // data dir + file
    foundFile = FindResourceFileWithAppInfo(wxStandardPaths::Get().GetDataDir(), subFile);
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
#ifdef __WXOSX__
    // centralized location for all users on OSX
    foundFile = _DT(L"/Library/Application Support/") + wxTheApp->GetAppName() + L"/" + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
#endif
    // Some special logic for Linux, where prefix logic is all over the map.
    // Sometimes the program might be installed to a different prefix than what
    // wxWidgets is detecting.
#ifdef __UNIX__
    // this is usually the default
    foundFile = FindResourceFileWithAppInfo(_DT(L"/usr/local/share/"), subFile);
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // older systems might do this
    foundFile = FindResourceFileWithAppInfo(_DT(L"/usr/share/"), subFile);
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
#endif
    // ...or, program dir + file
    foundFile = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPathWithSep() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // ...or, cwd + file
    foundFile = wxFileName::GetCwd() + wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // give up, can't find it anywhere
    wxLogWarning(L"'%s': unable to find resource file.", subFile);
    return wxString{};
    }

//----------------------------------------------------------
wxString Wisteria::UI::BaseApp::FindResourceDirectory(const wxString& subDir) const
    {
    // Resources folder + file (OSX uses this)
    wxString foundFolder = wxStandardPaths::Get().GetResourcesDir() +
        wxFileName::GetPathSeparator() + subDir;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }

    NoAppInfoAppend noAppInfo;

    // all users' data dir + subfolder
    foundFolder = FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetConfigDir(), subDir);
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // user data dir + subfolder
    foundFolder = FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetUserConfigDir(), subDir);
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // data dir + subfolder
    foundFolder = FindResourceDirectoryWithAppInfo(wxStandardPaths::Get().GetDataDir(), subDir);
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // Some special logic for Linux, where prefix logic is all over the map.
    // Sometimes the program might be installed to a different prefix than what
    // wxWidgets is detecting.
#if defined (__UNIX__) || defined (__APPLE__)
    // this is usually the default
    foundFolder = FindResourceDirectoryWithAppInfo(_DT(L"/usr/local/share/"), subDir);
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // older systems might do this
    foundFolder = FindResourceDirectoryWithAppInfo(_DT(L"/usr/share/"), subDir);
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
#endif
    // ...or, program dir + subfolder
    foundFolder = wxStandardPaths::Get().GetExecutablePath() +
                    wxFileName::GetPathSeparator() + subDir;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // ...or, cwd + subfolder
    foundFolder = wxFileName::GetCwd() + wxFileName::GetPathSeparator() + subDir;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // give up, can't find it anywhere
    return wxString{};
    }

//----------------------------------------------------------
wxString Wisteria::UI::BaseApp::FindResourceFileWithAppInfo(
    const wxString& folder, const wxString& subFile) const
    {
    wxString appFolderNameNoSpaces = GetAppName();
    appFolderNameNoSpaces.Replace(L" ", wxString{}, true);

    // try the folder + file
    wxString foundFile = folder + wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // try the folder + program name + file
    foundFile = folder + wxFileName::GetPathSeparator() + GetAppName() +
        wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    foundFile = folder + wxFileName::GetPathSeparator() + appFolderNameNoSpaces +
        wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    // try the folder + vendor name + program name + file
    foundFile = folder + wxFileName::GetPathSeparator() + GetVendorName() +
        wxFileName::GetPathSeparator() + GetAppName() +
        wxFileName::GetPathSeparator() + subFile;
    if (wxFileName::FileExists(foundFile))
        { return foundFile; }
    return wxString{};
    }

//----------------------------------------------------------
wxString Wisteria::UI::BaseApp::FindResourceDirectoryWithAppInfo(
    const wxString& folder, const wxString& subFolder) const
    {
    wxString appFolderNameNoSpaces = GetAppName();
    appFolderNameNoSpaces.Replace(L" ", wxString{}, true);

    // try the folder + file
    wxString foundFolder = folder + wxFileName::GetPathSeparator() + subFolder;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // try the folder + program name + file
    foundFolder = folder + wxFileName::GetPathSeparator() + GetAppName() +
        wxFileName::GetPathSeparator() + subFolder;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    foundFolder = folder + wxFileName::GetPathSeparator() + appFolderNameNoSpaces +
        wxFileName::GetPathSeparator() + subFolder;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    // try the folder + vendor name + program name + file
    foundFolder = folder + wxFileName::GetPathSeparator() + GetVendorName() +
        wxFileName::GetPathSeparator() + GetAppName() +
        wxFileName::GetPathSeparator() + subFolder;
    if (wxFileName::DirExists(foundFolder))
        { return foundFolder; }
    return wxString{};
    }
