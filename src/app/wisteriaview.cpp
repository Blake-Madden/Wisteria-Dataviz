///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaview.h"
#include "../base/reportprintout.h"
#include "../graphs/scatterplot.h"
#include "../ui/controls/datasetgridtable.h"
#include "../ui/dialogs/datasetimportdlg.h"
#include "../ui/dialogs/insertchernoffdlg.h"
#include "../ui/dialogs/insertpagedlg.h"
#include "../ui/dialogs/insertscatterplotdlg.h"
#include "../ui/dialogs/pivotlongerdlg.h"
#include "../ui/dialogs/pivotwiderrdlg.h"
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

    // find button bars for enabling/disabling
    m_graphButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(m_frame->FindWindowById(ID_GRAPH_BUTTONBAR));
    m_pagesButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(m_frame->FindWindowById(ID_PAGES_BUTTONBAR));

    // build the graph dropdown menus
    BuildGraphMenus();

    // bind sidebar click event
    m_sideBar->Bind(Wisteria::UI::wxEVT_SIDEBAR_CLICK, &WisteriaView::OnSidebarClick, this);

    // bind print button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPrintAll, this, wxID_PRINT);

    // bind insert dataset button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertDataset, this,
                  ID_INSERT_DATASET);

    // bind pivot buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotWider, this, ID_PIVOT_WIDER);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotLonger, this,
                  ID_PIVOT_LONGER);

    // bind page buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertPage, this, ID_INSERT_PAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditPage, this, ID_EDIT_PAGE);

    // bind graph category dropdown buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_BASIC);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_BUSINESS);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_STATISTICAL);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_SURVEY);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_EDUCATION);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_SOCIAL);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnGraphDropdown, this,
                  ID_INSERT_GRAPH_SPORTS);

    // bind individual graph menu items
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertChernoffPlot, this, ID_NEW_CHERNOFFPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertScatterPlot, this, ID_NEW_SCATTERPLOT);

    m_frame->CenterOnScreen();
    if (wxGetApp().GetMainFrame()->IsMaximized())
        {
        m_frame->Maximize();
        m_frame->SetSize(m_frame->GetSize());
        }

    // for new projects, prompt for an initial dataset before continuing
    std::shared_ptr<Wisteria::Data::Dataset> initialDataset;
    wxString initialDatasetName;
    wxString initialFilePath;
    Wisteria::Data::Dataset::ColumnPreviewInfo initialColumnInfo;
    Wisteria::Data::Dataset::ColumnPreviewInfo initialFullColumnInfo;
    Wisteria::Data::ImportInfo initialImportInfo;
    std::variant<wxString, size_t> initialWorksheet{ static_cast<size_t>(1) };
    if (doc->GetFilename().empty())
        {
        wxFileDialog fileDlg(wxGetApp().GetMainFrame(), _(L"Select Dataset"), wxString{},
                             wxString{}, Wisteria::Data::Dataset::GetDataFileFilter(),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

        if (fileDlg.ShowModal() != wxID_OK)
            {
            m_frame->Destroy();
            m_frame = nullptr;
            return false;
            }

        const wxString filePath = fileDlg.GetPath();

        Wisteria::UI::DatasetImportDlg importDlg(wxGetApp().GetMainFrame(), filePath);
        if (importDlg.ShowModal() != wxID_OK)
            {
            m_frame->Destroy();
            m_frame = nullptr;
            return false;
            }

        try
            {
            initialColumnInfo = importDlg.GetColumnPreviewInfo();
            initialImportInfo = importDlg.GetImportInfo();
            initialWorksheet = importDlg.GetWorksheet();
            initialDataset = std::make_shared<Wisteria::Data::Dataset>();
            initialDataset->Import(filePath, initialImportInfo, initialWorksheet);
            initialDatasetName = wxFileName{ filePath }.GetName();
            initialFilePath = filePath;
            initialFullColumnInfo = importDlg.GetFullColumnPreviewInfo();
            }
        catch (const std::exception& exc)
            {
            wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Import Error"), wxOK | wxICON_ERROR,
                         wxGetApp().GetMainFrame());
            m_frame->Destroy();
            m_frame = nullptr;
            return false;
            }
        }

    LoadProject();

    if (initialDataset != nullptr)
        {
        AddDatasetToProject(
            initialDataset, initialDatasetName, initialColumnInfo,
            { initialFilePath, wxString{}, initialWorksheet,
              initialFullColumnInfo, initialImportInfo });
        }

    UpdateGraphButtonStates();

    // hide the main frame when a document window is opened
    wxGetApp().GetMainFrame()->Hide();

    m_frame->Show(true);
    Activate(true);

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
    wxBusyInfo busy{ wxBusyInfoFlags{}.Text(_(L"Loading project...")) };
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
        const auto& importOpts = m_reportBuilder.GetDatasetImportOptions();
        for (const auto& [dsName, dataset] : m_reportBuilder.GetDatasets())
            {
            const wxWindowID dsId = nextId;
            nextId = wxNewId();

            const auto optIt = importOpts.find(dsName);
            const auto& colInfo = (optIt != importOpts.cend()) ?
                                      optIt->second.m_columnPreviewInfo :
                                      Wisteria::Data::Dataset::ColumnPreviewInfo{};
            auto* table = colInfo.empty() ? new Wisteria::UI::DatasetGridTable(dataset) :
                                            new Wisteria::UI::DatasetGridTable(dataset, colInfo);
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

            m_sideBar->InsertSubItemById(dataFolderId, dsName, dsId, DATA_ICON_INDEX);
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
    UpdateGraphButtonStates();
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
        const auto fullColumnPreview = importDlg.GetFullColumnPreviewInfo();
        const auto importInfo = importDlg.GetImportInfo();
        const auto worksheet = importDlg.GetWorksheet();
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->Import(filePath, importInfo, worksheet);

        wxString dsName =
            m_reportBuilder.GenerateUniqueDatasetName(wxFileName{ filePath }.GetName());
        if (m_reportBuilder.GetDatasets().contains(wxFileName{ filePath }.GetName()))
            {
            wxTextEntryDialog nameDlg(m_frame,
                                      _(L"A dataset with this name already exists.\n"
                                        "Please enter a different name:"),
                                      _(L"Dataset Name"), dsName);
            while (nameDlg.ShowModal() == wxID_OK)
                {
                dsName = nameDlg.GetValue().Strip(wxString::both);
                if (dsName.empty())
                    {
                    wxMessageBox(_(L"The name cannot be empty."), _(L"Name Required"),
                                 wxOK | wxICON_WARNING, m_frame);
                    continue;
                    }
                if (m_reportBuilder.GetDatasets().contains(dsName))
                    {
                    wxMessageBox(_(L"A dataset with this name already exists."),
                                 _(L"Duplicate Name"), wxOK | wxICON_WARNING, m_frame);
                    continue;
                    }
                break;
                }
            if (dsName.empty() || m_reportBuilder.GetDatasets().contains(dsName))
                {
                return;
                }
            }

        AddDatasetToProject(dataset, dsName, columnPreview,
                            { filePath, wxString{}, worksheet,
                              fullColumnPreview, importInfo });
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Import Error"), wxOK | wxICON_ERROR,
                     m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnPivotWider([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().empty())
        {
        wxMessageBox(_(L"Please import a dataset first."), _(L"No Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::PivotWiderDlg dlg(&m_reportBuilder, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto pivotedDataset = dlg.GetPivotedDataset();
    if (pivotedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    AddDatasetToProject(pivotedDataset, outputName);

    const auto dlgOpts = dlg.GetPivotOptions();
    Wisteria::ReportBuilder::DatasetPivotOptions pivotOpts;
    pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Wider;
    pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    pivotOpts.m_idColumns = dlgOpts.m_idColumns;
    pivotOpts.m_namesFromColumn = dlgOpts.m_namesFromColumn;
    pivotOpts.m_valuesFromColumns = dlgOpts.m_valuesFromColumns;
    pivotOpts.m_namesSep = dlgOpts.m_namesSep;
    pivotOpts.m_namesPrefix = dlgOpts.m_namesPrefix;
    pivotOpts.m_fillValue = dlgOpts.m_fillValue;
    m_reportBuilder.SetDatasetPivotOptions(outputName, pivotOpts);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnPivotLonger([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().empty())
        {
        wxMessageBox(_(L"Please import a dataset first."), _(L"No Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::PivotLongerDlg dlg(&m_reportBuilder, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto pivotedDataset = dlg.GetPivotedDataset();
    if (pivotedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    AddDatasetToProject(pivotedDataset, outputName);

    const auto dlgOpts = dlg.GetPivotOptions();
    Wisteria::ReportBuilder::DatasetPivotOptions pivotOpts;
    pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Longer;
    pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    pivotOpts.m_columnsToKeep = dlgOpts.m_columnsToKeep;
    pivotOpts.m_fromColumns = dlgOpts.m_fromColumns;
    pivotOpts.m_namesTo = dlgOpts.m_namesTo;
    pivotOpts.m_valuesTo = dlgOpts.m_valuesTo;
    pivotOpts.m_namesPattern = dlgOpts.m_namesPattern;
    m_reportBuilder.SetDatasetPivotOptions(outputName, pivotOpts);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::AddDatasetToProject(
    const std::shared_ptr<Wisteria::Data::Dataset>& dataset, const wxString& name,
    const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo /*= {}*/,
    const Wisteria::ReportBuilder::DatasetImportOptions& importOptions /*= {}*/)
    {
    m_reportBuilder.AddDataset(name, dataset, importOptions);

    const wxWindowID dsId = wxNewId();

    auto* table = columnInfo.empty() ? new Wisteria::UI::DatasetGridTable(dataset) :
                                       new Wisteria::UI::DatasetGridTable(dataset, columnInfo);

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
    m_sideBar->SaveState();
    m_sideBar->Refresh();
    }

//-------------------------------------------
void WisteriaView::OnInsertPage([[maybe_unused]] wxCommandEvent& event)
    {
    Wisteria::UI::InsertPageDlg dlg(m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    AddPageToProject(dlg.GetRows(), dlg.GetColumns(), dlg.GetPageName());
    }

//-------------------------------------------
void WisteriaView::OnEditPage([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    const auto [currentRows, currentCols] = canvas->GetFixedObjectsGridSize();

    Wisteria::UI::InsertPageDlg dlg(m_frame, wxID_ANY, _(L"Edit Page"));
    dlg.SetRows(currentRows);
    dlg.SetColumns(currentCols);
    if (m_sideBar->GetSelectedFolder())
        {
        dlg.SetPageName(m_sideBar->GetFolderText(m_sideBar->GetSelectedFolder().value()));
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvas->SetFixedObjectsGridSize(dlg.GetRows(), dlg.GetColumns());

    // update the sidebar label for this page
    const auto selectedFolder = m_sideBar->GetSelectedFolder();
    if (selectedFolder.has_value())
        {
        const wxString displayName = !dlg.GetPageName().empty() ?
                                         dlg.GetPageName() :
                                         m_sideBar->GetFolderText(selectedFolder.value());
        m_sideBar->SetFolderText(selectedFolder.value(), displayName);
        canvas->SetLabel(displayName);

        // adjust the splitter sash to match the sidebar's new min width
        const auto minWidth = m_sideBar->GetMinSize().GetWidth();
        m_splitter->SetSashPosition(minWidth);
        }
    }

//-------------------------------------------
void WisteriaView::AddPageToProject(const size_t rows, const size_t columns, const wxString& name)
    {
    const wxWindowID pageId = wxNewId();

    auto* canvas = new Wisteria::Canvas(m_workArea, pageId);
    canvas->SetFixedObjectsGridSize(rows, columns);

    canvas->Hide();
    m_workArea->GetSizer()->Add(canvas, wxSizerFlags{ 1 }.Expand());
    m_workWindows.AddWindow(canvas);
    m_pages.push_back(canvas);

    const wxString displayName =
        !name.empty() ? name : wxString::Format(_(L"Page %zu"), m_pages.size());
    canvas->SetLabel(displayName);
    m_sideBar->InsertItem(m_sideBar->GetFolderCount(), displayName, pageId, PAGE_ICON_INDEX);
    m_sideBar->SelectFolder(m_sideBar->GetFolderCount() - 1, true);
    m_sideBar->SaveState();

    m_workArea->Layout();
    m_sideBar->Refresh();
    UpdateGraphButtonStates();
    }

//-------------------------------------------
bool WisteriaView::IsPageSelected() const noexcept
    {
    return std::any_of(m_pages.cbegin(), m_pages.cend(),
                       [](const auto* canvas) { return canvas != nullptr && canvas->IsShown(); });
    }

//-------------------------------------------
void WisteriaView::UpdateGraphButtonStates()
    {
    const bool enabled = IsPageSelected();

    if (m_pagesButtonBar != nullptr)
        {
        m_pagesButtonBar->EnableButton(ID_EDIT_PAGE, enabled);
        }

    if (m_graphButtonBar == nullptr)
        {
        return;
        }

    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BASIC, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BUSINESS, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_STATISTICAL, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SURVEY, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_EDUCATION, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SOCIAL, enabled);
    m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SPORTS, enabled);
    }

//-------------------------------------------
void WisteriaView::OnGraphDropdown(wxCommandEvent& event)
    {
    const auto id = event.GetId();
    if (id == ID_INSERT_GRAPH_BASIC)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_basicGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_BUSINESS)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_businessGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_STATISTICAL)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_statisticalGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_SURVEY)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_surveyGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_EDUCATION)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_educationGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_SOCIAL)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_socialGraphMenu);
        }
    else if (id == ID_INSERT_GRAPH_SPORTS)
        {
        dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_sportsGraphMenu);
        }
    }

//-------------------------------------------
void WisteriaView::BuildGraphMenus()
    {
    const auto iconSize = wxSize{ m_frame->FromDIP(16), m_frame->FromDIP(16) };

    // helper to append a menu item with an icon
    const auto appendItem =
        [&](wxMenu& menu, wxWindowID id, const wxString& label, const wxString& svgName)
    {
        auto* item = new wxMenuItem(&menu, id, label);
        const auto bmp = wxGetApp().GetResourceManager().GetSVG(svgName);
        if (bmp.IsOk())
            {
            item->SetBitmap(bmp.GetBitmap(iconSize));
            }
        menu.Append(item);
    };

    // Basic graphs
    appendItem(m_basicGraphMenu, ID_NEW_BARCHART, _(L"Bar Chart..."), L"barchart.svg");
    m_basicGraphMenu.AppendSeparator();
    appendItem(m_basicGraphMenu, ID_NEW_PIECHART, _(L"Pie Chart..."), L"piechart.svg");
    m_basicGraphMenu.AppendSeparator();
    appendItem(m_basicGraphMenu, ID_NEW_LINEPLOT, _(L"Line Plot..."), L"lineplot.svg");
    m_basicGraphMenu.AppendSeparator();
    appendItem(m_basicGraphMenu, ID_NEW_TABLE, _(L"Table..."), L"table.svg");
    appendItem(m_basicGraphMenu, ID_NEW_SANKEY_DIAGRAM, _(L"Sankey Diagram..."), L"sankey.svg");
    appendItem(m_basicGraphMenu, ID_NEW_WAFFLE_CHART, _(L"Waffle Chart..."), L"waffle.svg");
    appendItem(m_basicGraphMenu, ID_NEW_WORD_CLOUD, _(L"Word Cloud..."), L"wordcloud.svg");

    // Business graphs
    appendItem(m_businessGraphMenu, ID_NEW_GANTT, _(L"Gantt Chart..."), L"gantt.svg");
    appendItem(m_businessGraphMenu, ID_NEW_CANDLESTICK, _(L"Candlestick Plot..."),
               L"candlestick.svg");

    // Statistical graphs
    appendItem(m_statisticalGraphMenu, ID_NEW_HISTOGRAM, _(L"Histogram..."), L"histogram.svg");
    m_statisticalGraphMenu.AppendSeparator();
    appendItem(m_statisticalGraphMenu, ID_NEW_BOXPLOT, _(L"Box Plot..."), L"boxplot.svg");
    appendItem(m_statisticalGraphMenu, ID_NEW_STEMANDLEAF, _(L"Stem-and-Leaf Plot..."),
               L"stem-leaf.svg");
    m_statisticalGraphMenu.AppendSeparator();
    appendItem(m_statisticalGraphMenu, ID_NEW_HEATMAP, _(L"Heat Map..."), L"heatmap.svg");
    m_statisticalGraphMenu.AppendSeparator();
    appendItem(m_statisticalGraphMenu, ID_NEW_SCATTERPLOT, _(L"Scatter Plot..."),
               L"scatterplot.svg");
    appendItem(m_statisticalGraphMenu, ID_NEW_BUBBLEPLOT, _(L"Bubble Plot..."), L"bubbleplot.svg");
    appendItem(m_statisticalGraphMenu, ID_NEW_CHERNOFFPLOT, _(L"Chernoff Faces Plot..."),
               L"chernoffplot.svg");

    // Survey graphs
    appendItem(m_surveyGraphMenu, ID_NEW_LIKERT, _(L"Likert Chart..."), L"likert7.svg");
    m_surveyGraphMenu.AppendSeparator();
    appendItem(m_surveyGraphMenu, ID_NEW_PROCON_ROADMAP, _(L"Pro && Con Roadmap..."),
               L"roadmap.svg");

    // Education graphs
    appendItem(m_educationGraphMenu, ID_NEW_SCALE_CHART, _(L"Scale Chart..."), L"scale.svg");

    // Social Sciences graphs
    appendItem(m_socialGraphMenu, ID_NEW_WCURVE, _(L"W-Curve Plot..."), L"wcurve.svg");
    appendItem(m_socialGraphMenu, ID_NEW_LR_ROADMAP, _(L"Linear Regression Roadmap..."),
               L"roadmap.svg");

    // Sports graphs
    appendItem(m_sportsGraphMenu, ID_NEW_WIN_LOSS_SPARKLINE, _(L"Win/Loss Sparkline..."),
               L"sparkline.svg");
    }

//-------------------------------------------
Wisteria::Canvas* WisteriaView::GetActiveCanvas() noexcept
    {
    for (auto* canvas : m_pages)
        {
        if (canvas != nullptr && canvas->IsShown())
            {
            return canvas;
            }
        }
    return nullptr;
    }

//-------------------------------------------
void WisteriaView::PlaceGraphWithLegend(
    Wisteria::Canvas* canvas, const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
    std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend, const size_t graphRow,
    const size_t graphCol, const Wisteria::UI::LegendPlacement legendPlacement)
    {
    using LP = Wisteria::UI::LegendPlacement;
    auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();

    // The legend occupies its own cell in the canvas grid, adjacent to the graph.
    // If the grid doesn't have room for the legend in the requested direction,
    // we expand it by adding a column (for left/right) or row (for top/bottom).
    //
    // When the legend goes to the left and the graph is in column 0, there is
    // no column to the left, so we add one and shift the graph to column 1.
    // Similarly, when the legend goes on top and the graph is in row 0,
    // we add a row and shift the graph down to row 1.
    if (legendPlacement == LP::Right)
        {
        if (graphCol + 1 >= gridCols)
            {
            canvas->SetFixedObjectsGridSize(gridRows, gridCols + 1);
            }
        }
    else if (legendPlacement == LP::Left)
        {
        if (graphCol == 0)
            {
            canvas->SetFixedObjectsGridSize(gridRows, gridCols + 1);
            }
        }
    else if (legendPlacement == LP::Bottom)
        {
        if (graphRow + 1 >= gridRows)
            {
            canvas->SetFixedObjectsGridSize(gridRows + 1, gridCols);
            }
        }
    else if (legendPlacement == LP::Top)
        {
        if (graphRow == 0)
            {
            canvas->SetFixedObjectsGridSize(gridRows + 1, gridCols);
            }
        }

    const auto plotRow = (legendPlacement == LP::Top && graphRow == 0) ? graphRow + 1 : graphRow;
    const auto plotCol = (legendPlacement == LP::Left && graphCol == 0) ? graphCol + 1 : graphCol;
    canvas->SetFixedObject(plotRow, plotCol, plot);

    if (legend != nullptr)
        {
        if (legendPlacement == LP::Right)
            {
            canvas->SetFixedObject(plotRow, plotCol + 1, std::move(legend));
            }
        else if (legendPlacement == LP::Left)
            {
            canvas->SetFixedObject(plotRow, plotCol - 1, std::move(legend));
            }
        else if (legendPlacement == LP::Bottom)
            {
            legend->SetPageHorizontalAlignment(Wisteria::PageHorizontalAlignment::LeftAligned);
            canvas->SetFixedObject(plotRow + 1, plotCol, std::move(legend));
            }
        else if (legendPlacement == LP::Top)
            {
            legend->SetPageHorizontalAlignment(Wisteria::PageHorizontalAlignment::LeftAligned);
            canvas->SetFixedObject(plotRow - 1, plotCol, std::move(legend));
            }
        }

    wxClientDC dc(canvas);
    canvas->CalcAllSizes(dc);
    canvas->Refresh();
    }

//-------------------------------------------
void WisteriaView::OnInsertChernoffPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertChernoffDlg dlg(canvas, &m_reportBuilder, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot =
            std::make_shared<Wisteria::Graphs::ChernoffFacesPlot>(canvas, dlg.GetSkinColorDarker());
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetSkinColorRange(dlg.GetSkinColorLighter(), dlg.GetSkinColorDarker());
        plot->SetGender(dlg.GetGender());
        plot->SetHairStyle(dlg.GetHairStyle());
        plot->SetFacialHair(dlg.GetFacialHair());
        plot->SetEyeColor(dlg.GetEyeColor());
        plot->SetHairColor(dlg.GetHairColor());
        plot->ShowLabels(dlg.GetShowLabels());

        using FID = Wisteria::Graphs::ChernoffFacesPlot::FeatureId;
        const auto optVar = [&dlg](FID id) -> std::optional<wxString>
        {
            const auto var = dlg.GetFeatureVariable(id);
            return var.empty() ? std::nullopt : std::optional<wxString>(var);
        };

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetFeatureVariable(FID::FaceWidth),
                      optVar(FID::FaceHeight), optVar(FID::EyeSize), optVar(FID::EyePosition),
                      optVar(FID::EyebrowSlant), optVar(FID::PupilDirection), optVar(FID::NoseSize),
                      optVar(FID::MouthWidth), optVar(FID::SmileFrown), optVar(FID::FaceColor),
                      optVar(FID::EarSize));

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;

        PlaceGraphWithLegend(
            canvas, plot,
            (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(plot->CreateExtendedLegend(
                    Wisteria::Graphs::LegendOptions{}.IncludeHeader(true).PlacementHint(hint))) :
                std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
            dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertScatterPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertScatterPlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::ScatterPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->ShowRegressionLines(dlg.GetShowRegressionLines());
        plot->ShowConfidenceBands(dlg.GetShowConfidenceBands());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;

        PlaceGraphWithLegend(
            canvas, plot,
            (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(plot->CreateLegend(
                    Wisteria::Graphs::LegendOptions{}.IncludeHeader(true).PlacementHint(hint))) :
                std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
            dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }
