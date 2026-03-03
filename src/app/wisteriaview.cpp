///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaview.h"
#include "../base/reportprintout.h"
#include "wisteriaapp.h"

wxIMPLEMENT_DYNAMIC_CLASS(WisteriaView, wxView);

//-------------------------------------------
bool WisteriaView::OnCreate(wxDocument* doc, long flags)
    {
    if (!wxView::OnCreate(doc, flags))
        {
        return false;
        }

    const wxSize windowSize(std::max(wxGetApp().GetMainFrame()->GetClientSize().GetWidth(),
                                     wxGetApp().GetMainFrame()->FromDIP(800)),
                            std::max(wxGetApp().GetMainFrame()->GetClientSize().GetHeight(),
                                     wxGetApp().GetMainFrame()->FromDIP(600)));

    const wxFileName fn(doc->GetFilename());
    const wxString title =
        !fn.GetName().empty() ? fn.GetName() : wxFileName::StripExtension(doc->GetTitle());

    m_frame = new wxDocChildFrame(doc, this, wxGetApp().GetMainFrame(), wxID_ANY, title,
                                  wxDefaultPosition, windowSize, wxDEFAULT_FRAME_STYLE);

    // set the icon
    wxIcon appIcon;
    const auto appSvg = wxGetApp().GetResourceManager().GetSVG(L"wisteria.svg");
    if (appSvg.IsOk())
        {
        appIcon.CopyFromBitmap(appSvg.GetBitmap(m_frame->FromDIP(wxSize{ 32, 32 })));
        m_frame->SetIcon(appIcon);
        }

    // create the ribbon
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* ribbon = wxGetApp().CreateRibbon(m_frame, doc);
    sizer->Add(ribbon, wxSizerFlags{}.Expand());

    // create the splitter with sidebar and work area
    m_splitter = new wxSplitterWindow(m_frame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize(m_frame->FromDIP(150));

    m_sideBar = new Wisteria::UI::SideBar(m_splitter);
    m_workArea = new wxPanel(m_splitter, wxID_ANY);
    m_workArea->SetSizer(new wxBoxSizer(wxVERTICAL));

    m_splitter->SplitVertically(m_sideBar, m_workArea, m_frame->FromDIP(200));

    sizer->Add(m_splitter, wxSizerFlags{ 1 }.Expand());
    m_frame->SetSizer(sizer);

    // bind sidebar click event
    m_sideBar->Bind(Wisteria::UI::wxEVT_SIDEBAR_CLICK, &WisteriaView::OnSidebarClick, this);

    // bind print button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPrintAll, this, wxID_PRINT);

    m_frame->CenterOnScreen();
    if (wxGetApp().GetMainFrame()->IsMaximized())
        {
        m_frame->Maximize();
        m_frame->SetSize(m_frame->GetSize());
        }
    m_frame->Show(true);
    Activate(true);

    LoadProject();

    return true;
    }

//-------------------------------------------
bool WisteriaView::OnClose(bool deleteWindow)
    {
    if (!wxView::OnClose(deleteWindow))
        {
        return false;
        }

    Activate(false);

    if (deleteWindow)
        {
        m_frame->Destroy();
        m_frame = nullptr;
        }

    return true;
    }

//-------------------------------------------
void WisteriaView::ShowSideBar(const bool show)
    {
    m_sidebarShown = show;
    if (m_splitter != nullptr && m_sideBar != nullptr && m_workArea != nullptr)
        {
        if (show)
            {
            if (!m_splitter->IsSplit())
                {
                m_splitter->SplitVertically(m_sideBar, m_workArea, m_frame->FromDIP(200));
                }
            }
        else
            {
            m_splitter->Unsplit(m_sideBar);
            }
        }
    }

//-------------------------------------------
void WisteriaView::LoadProject()
    {
    const wxString filename = GetDocument()->GetFilename();
    if (filename.empty())
        {
        return;
        }

    // load the JSON configuration file
    m_pages = m_reportBuilder.LoadConfigurationFile(filename, m_workArea);

    // set up sidebar image list from the app's persistent list
    m_sideBar->SetImageList(wxGetApp().GetProjectSideBarImageList());

    constexpr size_t DATA_ICON_INDEX = 0;
    constexpr size_t PAGE_ICON_INDEX = 1;

    // IDs for sidebar items
    const wxWindowID dataFolderId = wxNewId();
    wxWindowID nextId = wxNewId();

    // add the "Data" folder
    m_sideBar->InsertItem(0, _(L"Data"), dataFolderId, DATA_ICON_INDEX);

    // add datasets as subitems under "Data"
    const auto& datasets = m_reportBuilder.GetDatasets();
    for (const auto& [dsName, dataset] : datasets)
        {
        const wxWindowID dsId = nextId;
        nextId = wxNewId();

        auto* grid = new wxGrid(m_workArea, dsId);
        grid->CreateGrid(0, 0);
        grid->EnableEditing(false);
        grid->Hide();
        m_workArea->GetSizer()->Add(grid, wxSizerFlags{ 1 }.Expand());
        m_workWindows.AddWindow(grid);

        m_sideBar->InsertSubItemById(dataFolderId, dsName, dsId, std::nullopt);
        }

    // add pages as top-level folders
    size_t pageNum = 1;
    for (auto* canvas : m_pages)
        {
        const wxWindowID pageId = nextId;
        nextId = wxNewId();

        canvas->SetId(pageId);
        canvas->Hide();
        m_workArea->GetSizer()->Add(canvas, wxSizerFlags{ 1 }.Expand());
        m_workWindows.AddWindow(canvas);

        m_sideBar->InsertItem(m_sideBar->GetFolderCount(),
                              wxString::Format(_(L"Page %zu"), pageNum), pageId, PAGE_ICON_INDEX);
        ++pageNum;
        }

    // expand the Data folder
    if (m_sideBar->GetFolderCount() > 0)
        {
        m_sideBar->GetFolder(0).Expand();
        }

    // select and show the first page if available
    if (!m_pages.empty())
        {
        const auto firstPageId = m_pages[0]->GetId();
        auto* firstPage = m_workWindows.FindWindowById(firstPageId);
        if (firstPage != nullptr)
            {
            firstPage->Show();
            }
        // select the first page folder (folder index 1, after "Data")
        m_sideBar->SelectFolder(1, true, false);
        }

    m_workArea->Layout();
    m_sideBar->Refresh();
    }

//-------------------------------------------
void WisteriaView::OnSidebarClick(wxCommandEvent& event)
    {
    // hide all work windows
    for (auto* window : m_workWindows.GetWindows())
        {
        if (window != nullptr)
            {
            window->Hide();
            }
        }

    // determine which ID was selected
    const auto [folderId, subItemId] = m_sideBar->GetSelectedSubItemId();

    wxWindowID selectedId = wxID_ANY;
    if (subItemId.has_value())
        {
        selectedId = subItemId.value();
        }
    else if (folderId.has_value())
        {
        selectedId = folderId.value();
        }

    if (selectedId != wxID_ANY)
        {
        auto* window = m_workWindows.FindWindowById(selectedId);
        if (window != nullptr)
            {
            window->Show();
            }
        }

    m_workArea->Layout();
    }

//-------------------------------------------
void WisteriaView::OnPrintAll([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    auto printOut = std::make_unique<Wisteria::ReportPrintout>(m_pages, m_pages[0]->GetLabel());
#if defined(__WXMSW__) || defined(__WXOSX__)
    wxPrinterDC dc = wxPrinterDC(m_pages[0]->GetPrinterSettings());
#else
    wxPostScriptDC dc = wxPostScriptDC(m_pages[0]->GetPrinterSettings());
#endif
    printOut->SetUp(dc);

    wxPrinter printer;
    printer.GetPrintDialogData().SetPrintData(m_pages[0]->GetPrinterSettings());
    printer.GetPrintDialogData().SetAllPages(true);
    printer.GetPrintDialogData().SetFromPage(1);
    printer.GetPrintDialogData().SetToPage(static_cast<int>(m_pages.size()));
    if (!printer.Print(m_frame, printOut.get(), true))
        {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK | wxICON_WARNING);
            }
        }
    }
