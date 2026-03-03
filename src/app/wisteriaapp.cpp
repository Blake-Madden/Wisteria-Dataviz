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

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast,cppcoreguidelines-avoid-non-const-global-variables)
wxIMPLEMENT_APP(WisteriaApp);

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

    GetResourceManager().LoadArchive(FindResourceFile(L"res.wad"));

    InitProjectSidebar();

    // create the document template
    [[maybe_unused]]
    auto* docTemplate = new wxDocTemplate(GetDocManager(), _(L"Wisteria project"), L"*.json",
                                          wxString{}, L"json", L"Wisteria Doc", L"WisteriaView",
                                          wxCLASSINFO(WisteriaDoc), wxCLASSINFO(WisteriaView));

    SetAppFileExtension(L"json");

    LoadInterface();

    return true;
    }

//-------------------------------------------
void WisteriaApp::LoadInterface()
    {
    const wxArrayString extensions{ GetAppFileExtension() };

    SetMainFrame(new Wisteria::UI::BaseMainFrame(GetDocManager(), nullptr, extensions,
                                                 _WISTERIA_APP_NAME, wxPoint{ 0, 0 },
                                                 wxSize{ 800, 600 }, wxDEFAULT_FRAME_STYLE));

    GetMainFrame()->InitControls(CreateRibbon(GetMainFrame()));

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
        [this](wxCommandEvent& event)
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

    GetMainFrame()->CenterOnScreen();
    GetMainFrame()->Maximize();
    GetMainFrame()->Show(true);
    }

//-------------------------------------------
wxRibbonBar* WisteriaApp::CreateRibbon(wxWindow* parent, const wxDocument* doc)
    {
    const bool isProjectRibbon = (doc != nullptr);
    const auto iconSize = parent->FromDIP(wxSize{ 32, 32 });

    auto* ribbon = new wxRibbonBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxRIBBON_BAR_SHOW_PAGE_ICONS | wxRIBBON_BAR_DEFAULT_STYLE);

    // Home tab
    const auto homeIcon = GetResourceManager().GetSVG(L"home.svg");
    auto* homePage = new wxRibbonPage(
        ribbon, wxID_ANY, _(L"Home"),
        homeIcon.IsOk() ? homeIcon.GetBitmap(parent->FromDIP(wxSize{ 16, 16 })) : wxBitmap{});

    // Project panel with Open button
    auto* projectPanel = new wxRibbonPanel(homePage, wxID_ANY, _(L"Project"));
    auto* projectButtonBar = new wxRibbonButtonBar(projectPanel, wxID_ANY);

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
        }
    else
        {
        // TODO: main frame ribbon content
        }

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
