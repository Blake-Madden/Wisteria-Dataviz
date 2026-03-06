///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaview.h"
#include "../base/reportprintout.h"
#include "../ui/controls/datasetgridtable.h"
#include "../ui/dialogs/datasetimportdlg.h"
#include "wisteriaapp.h"

wxIMPLEMENT_DYNAMIC_CLASS(WisteriaView, wxView);

//-------------------------------------------
bool WisteriaView::OnCreate(wxDocument* doc, long flags)
    {
    if (!wxView::OnCreate(doc, flags))
        {
        return false;
        }

    // hide the main frame when a document window is opened
    wxGetApp().GetMainFrame()->Hide();

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

    // bind insert dataset button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertDataset, this,
                  ID_INSERT_DATASET);

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

    // show the main frame when the last document is being closed
    if (wxGetApp().GetDocumentCount() == 1)
        {
        wxGetApp().GetMainFrame()->CenterOnScreen();
        wxGetApp().GetMainFrame()->Show();
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

    // set up sidebar image list from the app's persistent list
    m_sideBar->SetImageList(wxGetApp().GetProjectSideBarImageList());

    // helper uses member function ApplyColumnHeaderIcons()

    // IDs for sidebar items
    const wxWindowID dataFolderId = wxNewId();
    wxWindowID nextId = wxNewId();

    if (!filename.empty())
        {
        // load the JSON configuration file
        m_pages = m_reportBuilder.LoadConfigurationFile(filename, m_workArea);
        }

    // add the "Data" folder
    m_sideBar->InsertItem(0, _(L"Data"), dataFolderId, DATA_ICON_INDEX);

    if (!filename.empty())
        {
        // add datasets as subitems under "Data"
        const auto& datasets = m_reportBuilder.GetDatasets();
        for (const auto& [dsName, dataset] : datasets)
            {
            const wxWindowID dsId = nextId;
            nextId = wxNewId();

            auto* table = new Wisteria::UI::DatasetGridTable(dataset);
            auto* grid = new wxGrid(m_workArea, dsId);
            grid->SetDoubleBuffered(true);
            grid->GetGridWindow()->SetDoubleBuffered(true);
            grid->SetTable(table, true);
            grid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
            grid->EnableEditing(false);
            ApplyColumnHeaderIcons(grid, table);
            grid->AutoSizeColumns(false);
            AdjustGridColumnsForIcons(grid);
            grid->Hide();
            m_workArea->GetSizer()->Add(grid, wxSizerFlags{ 1 }.Expand());
            m_workWindows.AddWindow(grid);

            m_sideBar->InsertSubItemById(dataFolderId, dsName, dsId, std::nullopt);
            }

        // add pages as top-level folders
        size_t pageNum{ 1 };
        for (auto* canvas : m_pages)
            {
            const wxWindowID pageId = nextId;
            nextId = wxNewId();

            canvas->SetId(pageId);
            canvas->Hide();
            m_workArea->GetSizer()->Add(canvas, wxSizerFlags{ 1 }.Expand());
            m_workWindows.AddWindow(canvas);

            m_sideBar->InsertItem(m_sideBar->GetFolderCount(),
                                  wxString::Format(_(L"Page %zu"), pageNum), pageId,
                                  PAGE_ICON_INDEX);
            ++pageNum;
            }
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
        // select the first page folder
        m_sideBar->SelectFolder(1, true);
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

    // show the window matching the selected sidebar item
    const wxWindowID selectedId = event.GetInt();
    if (selectedId != wxID_ANY)
        {
        if (auto* window = m_workWindows.FindWindowById(selectedId); window != nullptr)
            {
            if (window->IsKindOf(wxCLASSINFO(Wisteria::Canvas)))
                {
                dynamic_cast<Wisteria::Canvas*>(window)->ResetResizeDelay();
                }
            window->Show();
            }
        }

    m_workArea->Layout();
    m_sideBar->Refresh();
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

//-------------------------------------------
void WisteriaView::ApplyColumnHeaderIcons(wxGrid* grid, Wisteria::UI::DatasetGridTable* table)
    {
    const auto iconSize = wxSize{ grid->FromDIP(16), grid->FromDIP(16) };

    auto* attrProvider = new Wisteria::UI::DatasetGridAttrProvider();
    for (int col = 0; col < table->GetNumberCols(); ++col)
        {
        wxString svgName;
        switch (table->GetColumnType(col))
            {
        case Wisteria::UI::DatasetGridColumnType::Id:
            svgName = L"categorical.svg";
            break;
        case Wisteria::UI::DatasetGridColumnType::Categorical:
            svgName = L"categorical.svg";
            break;
        case Wisteria::UI::DatasetGridColumnType::Date:
            svgName = L"date.svg";
            break;
        case Wisteria::UI::DatasetGridColumnType::Continuous:
            svgName = L"scale.svg";
            break;
            }
        const auto bmpBundle = wxGetApp().GetResourceManager().GetSVG(svgName);
        if (bmpBundle.IsOk())
            {
            attrProvider->SetColumnHeaderRenderer(
                col, Wisteria::UI::DatasetColumnHeaderRenderer(bmpBundle.GetBitmap(iconSize)));
            }
        }
    table->SetAttrProvider(attrProvider);
    }

//-------------------------------------------
void WisteriaView::AdjustGridColumnsForIcons(wxGrid* grid)
    {
    const int iconOffset = grid->FromDIP(16) + 8;
    int maxColWidth = grid->GetClientSize().GetWidth() / 4;
    if (maxColWidth <= 0)
        {
        maxColWidth = grid->GetParent()->GetClientSize().GetWidth() / 4;
        }
    for (int col = 0; col < grid->GetNumberCols(); ++col)
        {
        int newWidth = grid->GetColSize(col) + iconOffset;
        if (maxColWidth > 0)
            {
            newWidth = std::min(newWidth, maxColWidth);
            }
        grid->SetColSize(col, newWidth);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertDataset([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog fileDlg(m_frame, _(L"Select Dataset"), wxString{}, wxString{},
                         Wisteria::Data::Dataset::GetDataFileFilter(),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const wxString filePath = fileDlg.GetPath();

    Wisteria::UI::DatasetImportDlg importDlg(m_frame, filePath);
    if (importDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        const auto columnPreview = importDlg.GetColumnPreviewInfo();
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->Import(filePath, importDlg.GetImportInfo(), importDlg.GetWorksheet());

        AddDatasetToProject(dataset, wxFileName{ filePath }.GetName(), columnPreview);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Import Error"), wxOK | wxICON_ERROR,
                     m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::AddDatasetToProject(
    const std::shared_ptr<Wisteria::Data::Dataset>& dataset, const wxString& name,
    const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo /*= {}*/)
    {
    const wxWindowID dsId = wxNewId();

    auto* table = new Wisteria::UI::DatasetGridTable(dataset);

    // apply currency symbols from the column preview info
    size_t contIdx{ 0 };
    for (const auto& col : columnInfo)
        {
        if (col.m_type == Wisteria::Data::Dataset::ColumnImportType::Numeric)
            {
            if (!col.m_currencySymbol.empty())
                {
                table->SetCurrencySymbol(contIdx, col.m_currencySymbol);
                }
            ++contIdx;
            }
        }

    auto* grid = new wxGrid(m_workArea, dsId);
    grid->SetDoubleBuffered(true);
    grid->GetGridWindow()->SetDoubleBuffered(true);
    grid->SetTable(table, true);
    grid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
    grid->EnableEditing(false);
    ApplyColumnHeaderIcons(grid, table);
    m_workArea->GetSizer()->Add(grid, wxSizerFlags{ 1 }.Expand());
    m_workArea->Layout();
    grid->AutoSizeColumns(false);
    AdjustGridColumnsForIcons(grid);
    grid->Hide();
    m_workWindows.AddWindow(grid);

    // add as subitem under the "Data" folder
    if (m_sideBar->GetFolderCount() > 0)
        {
        m_sideBar->InsertSubItemById(m_sideBar->GetFolder(0).GetId(), name, dsId, DATA_ICON_INDEX);
        m_sideBar->SelectSubItemById(m_sideBar->GetFolder(0).GetId(), dsId);
        }

    m_workArea->Layout();
    m_sideBar->ResetState();
    m_sideBar->Refresh();
    }
