///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "app.h"
#include "../base/label.h"
#ifdef __WXMSW__
    #include <psapi.h>
    #include <debugapi.h>
#endif

using namespace Wisteria;

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

    // logs will be written to file now, delete the old logging system
    m_logBuffer = new LogFile;
    delete wxLog::SetActiveTarget(m_logBuffer);

    // log some system information
    wxDateTime buildDate;
    buildDate.ParseDate(__DATE__);
    wxLogMessage(L"Log File Location: %s", m_logBuffer->GetLogFilePath());
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
    if (wxGraphicsRenderer::GetDefaultRenderer())
        {
        wxLogMessage(L"Graphics Renderer: %s",
            wxGraphicsRenderer::GetDefaultRenderer()->GetName());
        }
#ifdef __WXMSW__
    if (wxGraphicsRenderer::GetDirect2DRenderer())
        { wxLogMessage(L"Direct2D Rendering Available: will attempt to use Direct2D"); }
#endif
    wxLogMessage(L"Default System Font: %s, %d pt.",
        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName(),
        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
    wxLogMessage(L"Screen Size: %d wide, %d tall",
        wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
        wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));
    wxLogMessage(L"System Theme: %s", wxSystemSettings::GetAppearance().GetName().empty() ?
        wxString(L"[unnamed]") : wxSystemSettings::GetAppearance().GetName());
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
    wxLogDebug(__WXFUNCTION__);
    SaveFileHistoryMenu();
    wxDELETE(m_docManager);

#ifdef __WXMSW__
    #if wxDEBUG_LEVEL >= 2
        // dump max memory usage
        // https://docs.microsoft.com/en-us/windows/win32/psapi/collecting-memory-usage-information-for-a-process?redirectedfrom=MSDN
        PROCESS_MEMORY_COUNTERS memCounter;
        ::ZeroMemory(&memCounter, sizeof(PROCESS_MEMORY_COUNTERS));
        if (::GetProcessMemoryInfo(::GetCurrentProcess(), &memCounter, sizeof(memCounter)))
            {
            const wxString memMsg = wxString::Format(L"Peak Memory Usage: %.02fGbs.",
                safe_divide<double>(memCounter.PeakWorkingSetSize, 1024*1024*1024));
            wxLogDebug(memMsg);
            OutputDebugString(memMsg.wc_str());
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
            FontColor(wxColour(L"#315184")).
            DPIScaling(GetMainFrame() != nullptr ? GetMainFrame()->GetDPIScaleFactor() : 1.0).
            Scaling(2.0).
            Anchoring(Anchoring::TopLeftCorner).AnchorPoint({ 0, 0 }).
            Padding(4, 0, 4, 4));
    auto boundingBox = appLabel.GetBoundingBox(gcdc);

    const wxCoord BackscreenHeight = std::max(boundingBox.GetHeight(), bitmap.GetHeight()/5);

    // draw translucent backscreens on image so that text written on it can be read
        {
        wxDCPenChanger pc(gcdc, *wxBLACK_PEN);
        wxDCBrushChanger bc(gcdc, wxBrush(wxColour(255, 255, 255, 174)));
        gcdc.DrawRectangle(wxRect(0,0,canvasBmp.GetWidth(), BackscreenHeight));
        gcdc.DrawLine(0, BackscreenHeight, canvasBmp.GetWidth(), BackscreenHeight);
        if (includeCopyright)
            {
            gcdc.DrawRectangle(wxRect(0,canvasBmp.GetHeight() -
                               BackscreenHeight,canvasBmp.GetWidth(), BackscreenHeight));
            gcdc.DrawLine(0, canvasBmp.GetHeight()-BackscreenHeight,
                          canvasBmp.GetWidth(), canvasBmp.GetHeight()-BackscreenHeight);
            }
        }

    const auto spacePos = appName.find(L' ');

    if (spacePos == wxNOT_FOUND)
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
            wxString::Format(L"%c%d %s. %s",
                0xA9, buildDate.GetYear(), vendorName,
                _("All rights reserved."))).
            Pen(wxNullPen).
            DPIScaling(GetMainFrame() != nullptr ? GetMainFrame()->GetDPIScaleFactor() : 1.0).
            Font(wxFont(ftSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                        wxFONTWEIGHT_NORMAL, false,
                GraphItems::Label::GetFirstAvailableFont(
                    { _DT(L"Consolas"), _DT(L"Times New Roman") }))).
            FontColor(*wxBLACK).Padding(4, 4, 4, 4).Anchoring(Anchoring::BottomRightCorner).
            AnchorPoint(wxRect{ gcdc.GetSize() }.GetBottomRight()));

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
    report->AddText(L"Timestamp.log", dt.FormatISODate() + wxT(' ') +
                    dt.FormatISOTime(), _(L"Timestamp of this report"));

    report->AddFile(m_logBuffer->GetLogFilePath(), _(L"Log Report"));
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
        wxString newReportPath = wxStandardPaths::Get().GetDocumentsDir() +
                                 wxFileName::GetPathSeparator() + GetAppName() +
                                 L" Crash Report.zip";
        if (wxCopyFile(report->GetCompressedFileName(), newReportPath, true))
            {
            wxMessageBox(wxString::Format(_(L"An error report has been saved to:\n\"%s\".\n\n"
                "Please email this file to %s to have this issue reviewed. "
                "Thank you for your patience."), newReportPath, m_supportEmail),
                _(L"Error Report"), wxOK | wxICON_INFORMATION);
        #ifdef __WXMSW__
            ShellExecute(NULL, DONTTRANSLATE(L"open"), wxStandardPaths::Get().GetDocumentsDir().wc_str(),
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