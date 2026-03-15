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

    // bind save button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnSaveProject, this,
                  ID_SAVE_PROJECT);

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
        AddDatasetToProject(initialDataset, initialDatasetName, initialColumnInfo,
                            { initialFilePath, wxString{}, initialWorksheet, initialFullColumnInfo,
                              initialImportInfo });
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

    // select and show the dataset area
    m_sideBar->SelectFolder(0, true);

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
                            { filePath, wxString{}, worksheet, fullColumnPreview, importInfo });
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

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        const std::pair<FID, wxString> featureProps[] = { { FID::FaceWidth, L"face-width" },
                                                          { FID::FaceHeight, L"face-height" },
                                                          { FID::EyeSize, L"eye-size" },
                                                          { FID::EyePosition, L"eye-position" },
                                                          { FID::EyebrowSlant, L"eyebrow-slant" },
                                                          { FID::PupilDirection,
                                                            L"pupil-position" },
                                                          { FID::NoseSize, L"nose-size" },
                                                          { FID::MouthWidth, L"mouth-width" },
                                                          { FID::SmileFrown, L"mouth-curvature" },
                                                          { FID::FaceColor, L"face-saturation" },
                                                          { FID::EarSize, L"ear-size" } };
        for (const auto& [fid, propName] : featureProps)
            {
            const auto var = dlg.GetFeatureVariable(fid);
            if (!var.empty())
                {
                plot->SetPropertyTemplate(L"variables." + propName, var);
                }
            }

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

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.y", dlg.GetYVariable());
        plot->SetPropertyTemplate(L"variables.x", dlg.GetXVariable());
        if (!dlg.GetGroupVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.group", dlg.GetGroupVariable());
            }

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

//-------------------------------------------
void WisteriaView::OnSaveProject([[maybe_unused]] wxCommandEvent& event)
    {
    wxString filePath = GetDocument()->GetFilename();

    if (filePath.empty())
        {
        wxFileDialog saveDlg(m_frame, _(L"Save Project"), wxString{}, wxString{},
                             _(L"Wisteria Project Files (*.json)|*.json"),
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveDlg.ShowModal() != wxID_OK)
            {
            return;
            }
        filePath = saveDlg.GetPath();
        }

    try
        {
        SaveProject(filePath);
        GetDocument()->SetFilename(filePath, true);
        GetDocument()->Modify(false);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Save Error"), wxOK | wxICON_ERROR,
                     m_frame);
        }
    }

//-------------------------------------------
wxString WisteriaView::EscapeJsonStr(const wxString& str)
    {
    wxString escaped = str;
    escaped.Replace(L"\\", L"\\\\");
    escaped.Replace(L"\"", L"\\\"");
    return escaped;
    }

//-------------------------------------------
void WisteriaView::SaveDatasetImportOptions(
    wxSimpleJSON::Ptr_t& dsNode, const Wisteria::Data::Dataset::ColumnPreviewInfo& colInfo,
    const Wisteria::Data::ImportInfo& info)
    {
    using CIT = Wisteria::Data::Dataset::ColumnImportType;

    // import settings
    if (info.GetSkipRows() > 0)
        {
        dsNode->Add(L"skip-rows", static_cast<double>(info.GetSkipRows()));
        }
    if (!std::isnan(info.GetContinuousMDRecodeValue()))
        {
        dsNode->Add(L"continuous-md-recode-value", info.GetContinuousMDRecodeValue());
        }
    if (info.GetMDCodes().has_value())
        {
        wxArrayString mdArr;
        for (const auto& code : info.GetMDCodes().value())
            {
            mdArr.Add(wxString{ code });
            }
        dsNode->Add(L"md-codes", mdArr);
        }
    if (info.GetTreatLeadingZerosAsText())
        {
        dsNode->Add(L"treat-leading-zeros-as-text", true);
        }
    if (info.GetTreatYearsAsText())
        {
        dsNode->Add(L"treat-years-as-text", true);
        }
    if (info.GetMaxDiscreteValue() != 7)
        {
        dsNode->Add(L"max-discrete-value", static_cast<double>(info.GetMaxDiscreteValue()));
        }

    // id column
    if (!info.GetIdColumn().empty())
        {
        dsNode->Add(L"id-column", info.GetIdColumn());
        }

    // continuous columns
    wxArrayString contCols;
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded && col.m_type == CIT::Numeric)
            {
            contCols.Add(col.m_name);
            }
        }
    if (!contCols.empty())
        {
        dsNode->Add(L"continuous-columns", contCols);
        }

    // categorical columns
    auto catArray = dsNode->GetProperty(L"categorical-columns");
    for (const auto& col : colInfo)
        {
        if (col.m_excluded)
            {
            continue;
            }
        if (col.m_type == CIT::String || col.m_type == CIT::Discrete ||
            col.m_type == CIT::DichotomousString || col.m_type == CIT::DichotomousDiscrete)
            {
            auto catObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            catObj->Add(L"name", col.m_name);
            const auto catIt = std::find_if(
                info.GetCategoricalColumns().cbegin(), info.GetCategoricalColumns().cend(),
                [&col](const auto& ci) { return ci.m_columnName == col.m_name; });
            if (catIt != info.GetCategoricalColumns().cend() &&
                catIt->m_importMethod == Wisteria::Data::CategoricalImportMethod::ReadAsIntegers)
                {
                catObj->Add(L"parser", wxString{ L"as-integers" });
                }
            catArray->ArrayAdd(catObj);
            }
        }

    // date columns
    auto dateArray = dsNode->GetProperty(L"date-columns");
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded && col.m_type == CIT::Date)
            {
            auto dateObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            dateObj->Add(L"name", col.m_name);
            for (const auto& di : info.GetDateColumns())
                {
                if (di.m_columnName == col.m_name)
                    {
                    switch (di.m_importMethod)
                        {
                    case Wisteria::Data::DateImportMethod::IsoDate:
                        dateObj->Add(L"parser", wxString{ L"iso-date" });
                        break;
                    case Wisteria::Data::DateImportMethod::IsoCombined:
                        dateObj->Add(L"parser", wxString{ L"iso-combined" });
                        break;
                    case Wisteria::Data::DateImportMethod::Rfc822:
                        dateObj->Add(L"parser", wxString{ L"rfc822" });
                        break;
                    case Wisteria::Data::DateImportMethod::StrptimeFormatString:
                        dateObj->Add(L"parser", wxString{ L"strptime-format" });
                        if (!di.m_strptimeFormatString.empty())
                            {
                            dateObj->Add(L"format", di.m_strptimeFormatString);
                            }
                        break;
                    case Wisteria::Data::DateImportMethod::Time:
                        dateObj->Add(L"parser", wxString{ L"time" });
                        break;
                    case Wisteria::Data::DateImportMethod::Automatic:
                        [[fallthrough]];
                    default:
                        break;
                        }
                    break;
                    }
                }
            dateArray->ArrayAdd(dateObj);
            }
        }
    }

//-------------------------------------------
void WisteriaView::SaveTransformOptions(
    wxSimpleJSON::Ptr_t& dsNode, const Wisteria::ReportBuilder::DatasetTransformOptions& txOpts)
    {
    if (!txOpts.m_recodeREs.empty())
        {
        auto recodeArray = dsNode->GetProperty(L"recode-re");
        for (const auto& rr : txOpts.m_recodeREs)
            {
            auto rrObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            rrObj->Add(L"column", rr.m_column);
            rrObj->Add(L"pattern", rr.m_pattern);
            rrObj->Add(L"replacement", rr.m_replacement);
            recodeArray->ArrayAdd(rrObj);
            }
        }

    if (!txOpts.m_columnRenames.empty())
        {
        auto renameArray = dsNode->GetProperty(L"columns-rename");
        for (const auto& cr : txOpts.m_columnRenames)
            {
            auto crObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            if (!cr.m_nameRe.empty())
                {
                crObj->Add(L"name-re", cr.m_nameRe);
                crObj->Add(L"new-name-re", cr.m_newNameRe);
                }
            else
                {
                crObj->Add(L"name", cr.m_name);
                crObj->Add(L"new-name", cr.m_newName);
                }
            renameArray->ArrayAdd(crObj);
            }
        }

    if (!txOpts.m_mutateCategoricalColumns.empty())
        {
        auto mutArray = dsNode->GetProperty(L"mutate-categorical-columns");
        for (const auto& mc : txOpts.m_mutateCategoricalColumns)
            {
            auto mcObj = wxSimpleJSON::Create(L"{\"replacements\": []}");
            mcObj->Add(L"source-column", mc.m_sourceColumn);
            mcObj->Add(L"target-column", mc.m_targetColumn);
            auto replArray = mcObj->GetProperty(L"replacements");
            for (const auto& [pattern, replacement] : mc.m_replacements)
                {
                auto rObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
                rObj->Add(L"pattern", pattern);
                rObj->Add(L"replacement", replacement);
                replArray->ArrayAdd(rObj);
                }
            mutArray->ArrayAdd(mcObj);
            }
        }

    if (!txOpts.m_columnsSelect.empty())
        {
        dsNode->Add(L"columns-select", txOpts.m_columnsSelect);
        }

    if (!txOpts.m_collapseMins.empty())
        {
        auto cmArray = dsNode->GetProperty(L"collapse-min");
        for (const auto& cm : txOpts.m_collapseMins)
            {
            auto cmObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            cmObj->Add(L"column", cm.m_column);
            cmObj->Add(L"min", cm.m_min);
            if (!cm.m_otherLabel.empty())
                {
                cmObj->Add(L"other-label", cm.m_otherLabel);
                }
            cmArray->ArrayAdd(cmObj);
            }
        }

    if (!txOpts.m_collapseExcepts.empty())
        {
        auto ceArray = dsNode->GetProperty(L"collapse-except");
        for (const auto& ce : txOpts.m_collapseExcepts)
            {
            auto ceObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            ceObj->Add(L"column", ce.m_column);
            wxArrayString labels;
            for (const auto& lbl : ce.m_labelsToKeep)
                {
                labels.Add(lbl);
                }
            ceObj->Add(L"labels-to-keep", labels);
            if (!ce.m_otherLabel.empty())
                {
                ceObj->Add(L"other-label", ce.m_otherLabel);
                }
            ceArray->ArrayAdd(ceObj);
            }
        }

    if (txOpts.m_columnNamesSort)
        {
        dsNode->Add(L"column-names-sort", true);
        }
    }

//-------------------------------------------
void WisteriaView::SaveFormulas(
    wxSimpleJSON::Ptr_t& dsNode,
    const std::vector<Wisteria::ReportBuilder::DatasetFormulaInfo>& formulas)
    {
    if (formulas.empty())
        {
        return;
        }
    auto formulaArray = dsNode->GetProperty(L"formulas");
    for (const auto& f : formulas)
        {
        auto fObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        fObj->Add(L"name", f.m_name);
        fObj->Add(L"value", f.m_value);
        formulaArray->ArrayAdd(fObj);
        }
    }

//-------------------------------------------
void WisteriaView::SaveSubsetFilters(wxSimpleJSON::Ptr_t& subsetNode,
                                     const Wisteria::ReportBuilder::DatasetSubsetOptions& sOpts)
    {
    using FT = Wisteria::ReportBuilder::DatasetSubsetOptions::FilterType;

    const auto saveFilterObj = [](const Wisteria::ReportBuilder::DatasetFilterInfo& fi)
    {
        auto fObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        fObj->Add(L"column", fi.m_column);
        if (fi.m_operator != L"=")
            {
            fObj->Add(L"operator", fi.m_operator);
            }
        wxArrayString vals;
        for (const auto& v : fi.m_values)
            {
            vals.Add(v);
            }
        fObj->Add(L"values", vals);
        return fObj;
    };

    if (sOpts.m_filterType == FT::Section)
        {
        auto secObj = subsetNode->GetProperty(L"section");
        secObj->Add(L"column", sOpts.m_sectionColumn);
        secObj->Add(L"start-label", sOpts.m_sectionStartLabel);
        secObj->Add(L"end-label", sOpts.m_sectionEndLabel);
        if (!sOpts.m_sectionIncludeSentinelLabels)
            {
            secObj->Add(L"include-sentinel-labels", false);
            }
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::Single && sOpts.m_filters.size() == 1)
        {
        const auto& fi = sOpts.m_filters[0];
        auto filterObj = subsetNode->GetProperty(L"filter");
        filterObj->Add(L"column", fi.m_column);
        if (fi.m_operator != L"=")
            {
            filterObj->Add(L"operator", fi.m_operator);
            }
        wxArrayString vals;
        for (const auto& v : fi.m_values)
            {
            vals.Add(v);
            }
        filterObj->Add(L"values", vals);
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::And)
        {
        auto andArray = subsetNode->GetProperty(L"filter-and");
        for (const auto& fi : sOpts.m_filters)
            {
            andArray->ArrayAdd(saveFilterObj(fi));
            }
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::Or)
        {
        auto orArray = subsetNode->GetProperty(L"filter-or");
        for (const auto& fi : sOpts.m_filters)
            {
            orArray->ArrayAdd(saveFilterObj(fi));
            }
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        }
    else
        {
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    }

//-------------------------------------------
void WisteriaView::SaveSubsets(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const
    {
    const auto& subsetOpts = m_reportBuilder.GetDatasetSubsetOptions();
    const auto& transformOpts = m_reportBuilder.GetDatasetTransformOptions();

    auto subArray = parentNode->GetProperty(L"subsets");
    for (const auto& [subName, sOpts] : subsetOpts)
        {
        if (sOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0)
            {
            auto subObj = wxSimpleJSON::Create(wxString::Format(
                L"{\"name\": \"%s\", "
                L"\"section\": {}, \"filter\": {}, \"filter-and\": [], \"filter-or\": [], "
                L"\"recode-re\": [], \"columns-rename\": [], "
                L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                L"\"collapse-except\": [], \"formulas\": []}",
                EscapeJsonStr(subName)));
            SaveSubsetFilters(subObj, sOpts);

            const auto txIt = transformOpts.find(subName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(subObj, txIt->second);
                SaveFormulas(subObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"recode-re", L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
                {
                if (subObj->GetProperty(key)->ArraySize() == 0)
                    {
                    subObj->DeleteProperty(key);
                    }
                }

            subArray->ArrayAdd(subObj);
            }
        }
    }

//-------------------------------------------
void WisteriaView::SavePivots(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const
    {
    const auto& pivotOpts = m_reportBuilder.GetDatasetPivotOptions();
    const auto& transformOpts = m_reportBuilder.GetDatasetTransformOptions();

    auto pivArray = parentNode->GetProperty(L"pivots");
    for (const auto& [pivName, pOpts] : pivotOpts)
        {
        if (pOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0)
            {
            auto pivObj = wxSimpleJSON::Create(
                wxString::Format(L"{\"name\": \"%s\", "
                                 L"\"subsets\": [], \"recode-re\": [], \"columns-rename\": [], "
                                 L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                                 L"\"collapse-except\": []}",
                                 EscapeJsonStr(pivName)));

            if (pOpts.m_type == Wisteria::ReportBuilder::PivotType::Wider)
                {
                pivObj->Add(L"type", wxString{ L"wider" });
                if (!pOpts.m_idColumns.empty())
                    {
                    wxArrayString ids;
                    for (const auto& id : pOpts.m_idColumns)
                        {
                        ids.Add(id);
                        }
                    pivObj->Add(L"id-columns", ids);
                    }
                if (!pOpts.m_namesFromColumn.empty())
                    {
                    pivObj->Add(L"names-from-column", pOpts.m_namesFromColumn);
                    }
                if (!pOpts.m_valuesFromColumns.empty())
                    {
                    wxArrayString vals;
                    for (const auto& v : pOpts.m_valuesFromColumns)
                        {
                        vals.Add(v);
                        }
                    pivObj->Add(L"values-from-columns", vals);
                    }
                if (!pOpts.m_namesSep.empty() && pOpts.m_namesSep != L"_")
                    {
                    pivObj->Add(L"names-separator", pOpts.m_namesSep);
                    }
                if (!pOpts.m_namesPrefix.empty())
                    {
                    pivObj->Add(L"names-prefix", pOpts.m_namesPrefix);
                    }
                if (!std::isnan(pOpts.m_fillValue))
                    {
                    pivObj->Add(L"fill-value", pOpts.m_fillValue);
                    }
                }
            else
                {
                pivObj->Add(L"type", wxString{ L"longer" });
                if (!pOpts.m_columnsToKeep.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_columnsToKeep)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"columns-to-keep", cols);
                    }
                if (!pOpts.m_fromColumns.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_fromColumns)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"from-columns", cols);
                    }
                if (!pOpts.m_namesTo.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_namesTo)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"names-to", cols);
                    }
                if (!pOpts.m_valuesTo.empty())
                    {
                    pivObj->Add(L"values-to", pOpts.m_valuesTo);
                    }
                if (!pOpts.m_namesPattern.empty())
                    {
                    pivObj->Add(L"names-pattern", pOpts.m_namesPattern);
                    }
                }

            SaveSubsets(pivObj, pivName);

            const auto txIt = transformOpts.find(pivName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(pivObj, txIt->second);
                }

            // clean up empty arrays
            for (const auto& key :
                 { L"subsets", L"recode-re", L"columns-rename", L"mutate-categorical-columns",
                   L"collapse-min", L"collapse-except" })
                {
                if (pivObj->GetProperty(key)->ArraySize() == 0)
                    {
                    pivObj->DeleteProperty(key);
                    }
                }

            pivArray->ArrayAdd(pivObj);
            }
        }
    }

//-------------------------------------------
void WisteriaView::SaveMerges(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const
    {
    const auto& mergeOpts = m_reportBuilder.GetDatasetMergeOptions();

    auto mrgArray = parentNode->GetProperty(L"merges");
    for (const auto& [mrgName, mOpts] : mergeOpts)
        {
        if (mOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0)
            {
            auto mrgObj = wxSimpleJSON::Create(
                wxString::Format(L"{\"name\": \"%s\", \"by\": []}", EscapeJsonStr(mrgName)));
            if (!mOpts.m_type.empty() && mOpts.m_type != L"left-join-unique")
                {
                mrgObj->Add(L"type", mOpts.m_type);
                }
            mrgObj->Add(L"other-dataset", mOpts.m_otherDatasetName);
            if (!mOpts.m_byColumns.empty())
                {
                auto byArray = mrgObj->GetProperty(L"by");
                for (const auto& [left, right] : mOpts.m_byColumns)
                    {
                    auto byObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
                    byObj->Add(L"left-column", left);
                    byObj->Add(L"right-column", right);
                    byArray->ArrayAdd(byObj);
                    }
                }
            else
                {
                mrgObj->DeleteProperty(L"by");
                }
            if (!mOpts.m_suffix.empty() && mOpts.m_suffix != L".x")
                {
                mrgObj->Add(L"suffix", mOpts.m_suffix);
                }

            mrgArray->ArrayAdd(mrgObj);
            }
        }
    }

//-------------------------------------------
wxString WisteriaView::SavePenToStr(const wxPen& pen)
    {
    if (!pen.IsOk() || pen == wxNullPen)
        {
        return L"null";
        }

    const auto colorStr = pen.GetColour().GetAsString(wxC2S_HTML_SYNTAX);
    const auto styleStr = Wisteria::ReportEnumConvert::ConvertPenStyleToString(pen.GetStyle());

    const bool isDefaultWidth = (pen.GetWidth() <= 1);
    const bool isDefaultStyle = (!styleStr.has_value() || styleStr.value() == L"solid");

    // if only a color with default width and style, just return the color string
    if (isDefaultWidth && isDefaultStyle)
        {
        return wxString::Format(L"\"%s\"", colorStr);
        }

    wxString result = L"{";
    result += wxString::Format(L"\"color\": \"%s\"", colorStr);
    if (!isDefaultWidth)
        {
        result += wxString::Format(L", \"width\": %d", pen.GetWidth());
        }
    if (!isDefaultStyle)
        {
        result += wxString::Format(L", \"style\": \"%s\"", styleStr.value());
        }
    result += L"}";
    return result;
    }

//-------------------------------------------
wxString WisteriaView::SaveBrushToStr(const wxBrush& brush)
    {
    if (!brush.IsOk() || brush == wxNullBrush)
        {
        return L"null";
        }

    const auto colorStr = brush.GetColour().GetAsString(wxC2S_HTML_SYNTAX);
    const auto styleStr = Wisteria::ReportEnumConvert::ConvertBrushStyleToString(brush.GetStyle());

    const bool isDefaultStyle = (!styleStr.has_value() || styleStr.value() == L"solid");

    // if solid style, just return the color string
    if (isDefaultStyle)
        {
        return wxString::Format(L"\"%s\"", colorStr);
        }

    return wxString::Format(L"{\"color\": \"%s\", \"style\": \"%s\"}", colorStr, styleStr.value());
    }

//-------------------------------------------
void WisteriaView::SaveItem(wxSimpleJSON::Ptr_t& itemNode,
                            const Wisteria::GraphItems::GraphItemBase* item,
                            const Wisteria::Canvas* canvas) const
    {
    if (item == nullptr || canvas == nullptr)
        {
        return;
        }

    // ID
    if (item->GetId() != wxID_ANY)
        {
        itemNode->Add(L"id", static_cast<double>(item->GetId()));
        }

    // scaling (relative to canvas)
    const double relScale = std::round(item->GetScaling() / canvas->GetScaling() * 10.0) / 10.0;
    if (!compare_doubles(relScale, 1.0))
        {
        itemNode->Add(L"scaling", relScale);
        }

    // canvas-margins [top, right, bottom, left]
    const auto cmt = item->GetTopCanvasMargin();
    const auto cmr = item->GetRightCanvasMargin();
    const auto cmb = item->GetBottomCanvasMargin();
    const auto cml = item->GetLeftCanvasMargin();
    if (cmt != 0 || cmr != 0 || cmb != 0 || cml != 0)
        {
        auto cmArray = itemNode->GetProperty(L"canvas-margins");
        cmArray->ArrayAdd(static_cast<double>(cmt));
        cmArray->ArrayAdd(static_cast<double>(cmr));
        cmArray->ArrayAdd(static_cast<double>(cmb));
        cmArray->ArrayAdd(static_cast<double>(cml));
        }
    else
        {
        itemNode->DeleteProperty(L"canvas-margins");
        }

    // padding [top, right, bottom, left]
    const auto pt = item->GetTopPadding();
    const auto pr = item->GetRightPadding();
    const auto pb = item->GetBottomPadding();
    const auto pl = item->GetLeftPadding();
    if (pt != 0 || pr != 0 || pb != 0 || pl != 0)
        {
        auto padArray = itemNode->GetProperty(L"padding");
        padArray->ArrayAdd(static_cast<double>(pt));
        padArray->ArrayAdd(static_cast<double>(pr));
        padArray->ArrayAdd(static_cast<double>(pb));
        padArray->ArrayAdd(static_cast<double>(pl));
        }
    else
        {
        itemNode->DeleteProperty(L"padding");
        }

    // outline [top, right, bottom, left]
    const auto& info = item->GetGraphItemInfo();
    const bool oTop = info.IsShowingTopOutline();
    const bool oRight = info.IsShowingRightOutline();
    const bool oBottom = info.IsShowingBottomOutline();
    const bool oLeft = info.IsShowingLeftOutline();
    if (oTop || oRight || oBottom || oLeft)
        {
        auto olArray = itemNode->GetProperty(L"outline");
        olArray->ArrayAdd(oTop);
        olArray->ArrayAdd(oRight);
        olArray->ArrayAdd(oBottom);
        olArray->ArrayAdd(oLeft);
        }
    else
        {
        itemNode->DeleteProperty(L"outline");
        }

    // show (default is true)
    if (!item->IsShown())
        {
        itemNode->Add(L"show", false);
        }

    // anchoring (default is Center)
    const auto anchoringStr =
        Wisteria::ReportEnumConvert::ConvertAnchoringToString(item->GetAnchoring());
    if (anchoringStr.has_value() && item->GetAnchoring() != Wisteria::Anchoring::Center)
        {
        itemNode->Add(L"anchoring", anchoringStr.value());
        }

    // horizontal-page-alignment (default is LeftAligned)
    const auto hpaStr = Wisteria::ReportEnumConvert::ConvertPageHorizontalAlignmentToString(
        item->GetPageHorizontalAlignment());
    if (hpaStr.has_value() &&
        item->GetPageHorizontalAlignment() != Wisteria::PageHorizontalAlignment::LeftAligned)
        {
        itemNode->Add(L"horizontal-page-alignment", hpaStr.value());
        }

    // vertical-page-alignment (default is Centered)
    const auto vpaStr = Wisteria::ReportEnumConvert::ConvertPageVerticalAlignmentToString(
        item->GetPageVerticalAlignment());
    if (vpaStr.has_value() &&
        item->GetPageVerticalAlignment() != Wisteria::PageVerticalAlignment::Centered)
        {
        itemNode->Add(L"vertical-page-alignment", vpaStr.value());
        }

    // relative-alignment (default is Centered)
    const auto raStr =
        Wisteria::ReportEnumConvert::ConvertRelativeAlignmentToString(item->GetRelativeAlignment());
    if (raStr.has_value() && item->GetRelativeAlignment() != Wisteria::RelativeAlignment::Centered)
        {
        itemNode->Add(L"relative-alignment", raStr.value());
        }

    // fixed-width (default is false)
    if (item->IsFixedWidthOnCanvas())
        {
        itemNode->Add(L"fixed-width", true);
        }

    // fit-row-to-content (default is false)
    if (item->IsFittingCanvasRowHeightToContent())
        {
        itemNode->Add(L"fit-row-to-content", true);
        }
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SaveLabel(const Wisteria::GraphItems::Label* label,
                                            const Wisteria::Canvas* canvas) const
    {
    if (label == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    // empty-spacer: not shown, canvas height proportion is 0
    if (!label->IsShown() && label->GetCanvasHeightProportion().has_value() &&
        compare_doubles(label->GetCanvasHeightProportion().value(), 0.0))
        {
        return wxSimpleJSON::Create(L"{\"type\": \"empty-spacer\"}");
        }

    // spacer: not shown (but not an empty-spacer)
    if (!label->IsShown())
        {
        return wxSimpleJSON::Create(L"{\"type\": \"spacer\"}");
        }

    // build header JSON string first (needed for template)
    wxString headerStr;
    const auto& header = label->GetHeaderInfo();
    if (header.IsEnabled())
        {
        auto headerNode = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        if (header.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
            {
            headerNode->Add(L"bold", true);
            }
        if (header.GetFontColor().IsOk() && header.GetFontColor() != *wxBLACK)
            {
            headerNode->Add(L"color", header.GetFontColor().GetAsString(wxC2S_HTML_SYNTAX));
            }
        if (!compare_doubles(header.GetRelativeScaling(), 1.0))
            {
            headerNode->Add(L"relative-scaling", header.GetRelativeScaling());
            }
        if (header.GetLabelAlignment() != Wisteria::TextAlignment::FlushLeft)
            {
            const auto htaStr = Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(
                header.GetLabelAlignment());
            if (htaStr.has_value())
                {
                headerNode->Add(L"text-alignment", htaStr.value());
                }
            }
        const wxString printed = headerNode->Print(false);
        if (printed != L"{}")
            {
            headerStr = printed;
            }
        }

    // build template with all sub-objects embedded
    wxString tmpl = L"{\"type\": \"label\"";
    if (!headerStr.empty())
        {
        tmpl += L", \"header\": " + headerStr;
        }
    const auto& pen = label->GetPen();
    if (pen.IsOk() && pen != wxNullPen)
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // left-image
    const auto leftImgPath = label->GetPropertyTemplate(L"left-image.path");
    if (!leftImgPath.empty())
        {
        tmpl += L", \"left-image\": {\"image-import\": {\"path\": \"" + EscapeJsonStr(leftImgPath) +
                L"\"}}";
        }

    // top-image
    const auto topImgPath = label->GetPropertyTemplate(L"top-image.path");
    if (!topImgPath.empty())
        {
        tmpl += L", \"top-image\": {\"image-import\": {\"path\": \"" + EscapeJsonStr(topImgPath) +
                L"\"}";
        if (label->GetTopImageOffset() != 0)
            {
            tmpl += wxString::Format(L", \"offset\": %zu", label->GetTopImageOffset());
            }
        tmpl += L"}";
        }

    // top-shape
    const auto& topShape = label->GetTopShape();
    if (topShape.has_value() && !topShape->empty())
        {
        if (topShape->size() == 1)
            {
            tmpl += L", \"top-shape\": ";
            }
        else
            {
            tmpl += L", \"top-shape\": [";
            }
        for (size_t i = 0; i < topShape->size(); ++i)
            {
            if (i > 0)
                {
                tmpl += L", ";
                }
            const auto& si = topShape->at(i);
            const auto siIconStr = Wisteria::ReportEnumConvert::ConvertIconToString(si.GetShape());
            tmpl += L"{";
            if (siIconStr.has_value())
                {
                tmpl += L"\"icon\": \"" + siIconStr.value() + L"\"";
                }
            const auto siSz = si.GetSizeDIPs();
            tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}",
                                     siSz.GetWidth(), siSz.GetHeight());
            const auto& siPen = si.GetPen();
            if (siPen.IsOk() && siPen != wxNullPen)
                {
                tmpl += L", \"pen\": " + SavePenToStr(siPen);
                }
            const auto& siBrush = si.GetBrush();
            if (siBrush.IsOk() && siBrush != wxNullBrush)
                {
                tmpl += L", \"brush\": " + SaveBrushToStr(siBrush);
                }
            if (!compare_doubles(si.GetFillPercent(), math_constants::full))
                {
                tmpl += wxString::Format(L", \"fill-percent\": %g", si.GetFillPercent());
                }
            if (!si.GetText().empty())
                {
                tmpl += L", \"label\": \"" + EscapeJsonStr(si.GetText()) + L"\"";
                }
            tmpl += L"}";
            }
        if (topShape->size() > 1)
            {
            tmpl += L"]";
            }
        if (label->GetTopImageOffset() != 0 && topImgPath.empty())
            {
            tmpl += wxString::Format(L", \"top-shape-offset\": %zu", label->GetTopImageOffset());
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    // text (prefer template if it has {{constants}})
    const auto textTemplate = label->GetPropertyTemplate(L"text");
    const auto& text = textTemplate.empty() ? label->GetText() : textTemplate;
    if (!text.empty())
        {
        node->Add(L"text", text);
        }

    // color (font color, if not default black)
    const auto& fontColor = label->GetFontColor();
    if (fontColor.IsOk() && fontColor != *wxBLACK)
        {
        const auto colorTemplate = label->GetPropertyTemplate(L"color");
        node->Add(L"color",
                  colorTemplate.empty() ? fontColor.GetAsString(wxC2S_HTML_SYNTAX) : colorTemplate);
        }

    // background color
    const auto& bgColor = label->GetFontBackgroundColor();
    if (bgColor.IsOk() && bgColor != wxTransparentColour)
        {
        node->Add(L"background", bgColor.GetAsString(wxC2S_HTML_SYNTAX));
        }

    // bold
    if (label->GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
        {
        node->Add(L"bold", true);
        }

    // orientation (default is horizontal)
    if (label->GetTextOrientation() == Wisteria::Orientation::Vertical)
        {
        node->Add(L"orientation", L"vertical");
        }

    // line-spacing (default is 1)
    if (!compare_doubles(label->GetLineSpacing(), 1.0))
        {
        node->Add(L"line-spacing", label->GetLineSpacing());
        }

    // text-alignment (default is flush-left)
    if (label->GetTextAlignment() != Wisteria::TextAlignment::FlushLeft)
        {
        const auto taStr =
            Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(label->GetTextAlignment());
        if (taStr.has_value())
            {
            node->Add(L"text-alignment", taStr.value());
            }
        }

    SaveItem(node, label, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SaveImage(const Wisteria::GraphItems::Image* image,
                                            const Wisteria::Canvas* canvas) const
    {
    if (image == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    wxString tmpl = L"{\"type\": \"image\"";

    // image-import path from property template
    const auto pathTemplate = image->GetPropertyTemplate(L"image-import.path");
    if (!pathTemplate.empty())
        {
        tmpl += L", \"image-import\": {\"path\": \"" + EscapeJsonStr(pathTemplate) + L"\"}";
        }

    // size (from cached original values)
    const auto widthStr = image->GetPropertyTemplate(L"size.width");
    const auto heightStr = image->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";
    auto node = wxSimpleJSON::Create(tmpl);

    // resize-method (default is DownscaleOrUpscale)
    if (image->GetResizeMethod() != Wisteria::ResizeMethod::DownscaleOrUpscale)
        {
        const auto rmStr =
            Wisteria::ReportEnumConvert::ConvertResizeMethodToString(image->GetResizeMethod());
        if (rmStr.has_value())
            {
            node->Add(L"resize-method", rmStr.value());
            }
        }

    SaveItem(node, image, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SaveShape(const Wisteria::GraphItems::Shape* shape,
                                            const Wisteria::Canvas* canvas) const
    {
    if (shape == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shape->GetShape());

    wxString tmpl = L"{\"type\": \"shape\"";
    if (iconStr.has_value())
        {
        tmpl += L", \"icon\": \"" + iconStr.value() + L"\"";
        }

    // size (prefer cached original values, fall back to current)
    const auto widthStr = shape->GetPropertyTemplate(L"size.width");
    const auto heightStr = shape->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }
    else
        {
        const auto sz = shape->GetSizeDIPS();
        tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}", sz.GetWidth(),
                                 sz.GetHeight());
        }

    // pen (skip if default black solid width-1)
    const auto& pen = shape->GetPen();
    if (pen.IsOk() && pen != wxNullPen &&
        !(pen.GetColour() == *wxBLACK && pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // brush (skip if default white solid for Shape)
    const auto& brush = shape->GetBrush();
    if (brush.IsOk() && brush != wxNullBrush &&
        !(brush.GetColour() == *wxWHITE && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
        {
        tmpl += L", \"brush\": " + SaveBrushToStr(brush);
        }

    // label (as sub-object if it has color/templates, otherwise plain string)
    if (!shape->GetText().empty())
        {
        const auto labelTextTmpl = shape->GetPropertyTemplate(L"label.text");
        const auto labelColorTmpl = shape->GetPropertyTemplate(L"label.color");
        const auto& labelText = labelTextTmpl.empty() ? shape->GetText() : labelTextTmpl;

        if (!labelColorTmpl.empty() ||
            (shape->GetFontColor().IsOk() && shape->GetFontColor() != *wxBLACK))
            {
            const wxString colorVal = labelColorTmpl.empty() ?
                                          shape->GetFontColor().GetAsString(wxC2S_HTML_SYNTAX) :
                                          labelColorTmpl;
            tmpl += L", \"label\": {\"text\": \"" + EscapeJsonStr(labelText) +
                    L"\", \"color\": \"" + EscapeJsonStr(colorVal) + L"\"}";
            }
        else
            {
            tmpl += L", \"label\": \"" + EscapeJsonStr(labelText) + L"\"";
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    SaveItem(node, shape, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t
WisteriaView::SaveFillableShape(const Wisteria::GraphItems::FillableShape* shape,
                                const Wisteria::Canvas* canvas) const
    {
    if (shape == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shape->GetShape());

    wxString tmpl = L"{\"type\": \"fillable-shape\"";
    if (iconStr.has_value())
        {
        tmpl += L", \"icon\": \"" + iconStr.value() + L"\"";
        }

    // size (prefer cached original values, fall back to current)
    const auto widthStr = shape->GetPropertyTemplate(L"size.width");
    const auto heightStr = shape->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }
    else
        {
        const auto sz = shape->GetSizeDIPS();
        tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}", sz.GetWidth(),
                                 sz.GetHeight());
        }

    // pen (skip if default black solid width-1)
    const auto& pen = shape->GetPen();
    if (pen.IsOk() && pen != wxNullPen &&
        !(pen.GetColour() == *wxBLACK && pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // brush (skip if default black solid for FillableShape)
    const auto& brush = shape->GetBrush();
    if (brush.IsOk() && brush != wxNullBrush &&
        !(brush.GetColour() == *wxBLACK && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
        {
        tmpl += L", \"brush\": " + SaveBrushToStr(brush);
        }

    // label (as sub-object if it has color/templates, otherwise plain string)
    if (!shape->GetText().empty())
        {
        const auto labelTextTmpl = shape->GetPropertyTemplate(L"label.text");
        const auto labelColorTmpl = shape->GetPropertyTemplate(L"label.color");
        const auto& labelText = labelTextTmpl.empty() ? shape->GetText() : labelTextTmpl;

        if (!labelColorTmpl.empty() ||
            (shape->GetFontColor().IsOk() && shape->GetFontColor() != *wxBLACK))
            {
            const wxString colorVal = labelColorTmpl.empty() ?
                                          shape->GetFontColor().GetAsString(wxC2S_HTML_SYNTAX) :
                                          labelColorTmpl;
            tmpl += L", \"label\": {\"text\": \"" + EscapeJsonStr(labelText) +
                    L"\", \"color\": \"" + EscapeJsonStr(colorVal) + L"\"}";
            }
        else
            {
            tmpl += L", \"label\": \"" + EscapeJsonStr(labelText) + L"\"";
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    // fill-percent (prefer template for {{constants}})
    const auto fpTemplate = shape->GetPropertyTemplate(L"fill-percent");
    if (!fpTemplate.empty())
        {
        node->Add(L"fill-percent", fpTemplate);
        }
    else if (!compare_doubles(shape->GetFillPercent(), math_constants::empty))
        {
        node->Add(L"fill-percent", shape->GetFillPercent());
        }

    SaveItem(node, shape, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SaveCommonAxis(const Wisteria::GraphItems::Axis* axis,
                                                 const Wisteria::Canvas* canvas) const
    {
    if (axis == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    wxString tmpl = L"{\"type\": \"common-axis\"";

    // axis-type
    const auto axisTypeStr =
        Wisteria::ReportEnumConvert::ConvertAxisTypeToString(axis->GetAxisType());
    if (axisTypeStr.has_value())
        {
        tmpl += L", \"axis-type\": \"" + axisTypeStr.value() + L"\"";
        }

    // child-ids (cached as comma-separated string)
    const auto childIdsStr = axis->GetPropertyTemplate(L"child-ids");
    if (!childIdsStr.empty())
        {
        tmpl += L", \"child-ids\": [" + childIdsStr + L"]";
        }

    // common-perpendicular-axis
    const auto cpaStr = axis->GetPropertyTemplate(L"common-perpendicular-axis");
    if (cpaStr == L"true")
        {
        tmpl += L", \"common-perpendicular-axis\": true";
        }

    // title
    const auto& title = axis->GetTitle();
    if (!title.GetText().empty() || title.IsShown())
        {
        const auto textTemplate = title.GetPropertyTemplate(L"text");
        const auto& titleText = textTemplate.empty() ? title.GetText() : textTemplate;
        tmpl += L", \"title\": {\"text\": \"" + EscapeJsonStr(titleText) + L"\"}";
        }

    // axis-pen
    const auto& axisPen = axis->GetAxisLinePen();
    if (!axisPen.IsOk() || axisPen == wxNullPen)
        {
        tmpl += L", \"axis-pen\": null";
        }
    else if (!(axisPen.GetColour() == *wxBLACK && axisPen.GetWidth() <= 1 &&
               axisPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"axis-pen\": " + SavePenToStr(axisPen);
        }

    // gridline-pen
    const auto& gridPen = axis->GetGridlinePen();
    if (!gridPen.IsOk() || gridPen == wxNullPen)
        {
        tmpl += L", \"gridline-pen\": null";
        }
    else if (!(gridPen.GetColour() == *wxBLACK && gridPen.GetWidth() <= 1 &&
               gridPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"gridline-pen\": " + SavePenToStr(gridPen);
        }

    // label-display
    const auto ldStr =
        Wisteria::ReportEnumConvert::ConvertAxisLabelDisplayToString(axis->GetLabelDisplay());
    if (ldStr.has_value() &&
        axis->GetLabelDisplay() != Wisteria::AxisLabelDisplay::DisplayCustomLabelsOrValues)
        {
        tmpl += L", \"label-display\": \"" + ldStr.value() + L"\"";
        }

    // number-display
    const auto ndStr =
        Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(axis->GetNumberDisplay());
    if (ndStr.has_value() && axis->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
        {
        tmpl += L", \"number-display\": \"" + ndStr.value() + L"\"";
        }

    // tickmarks
    const auto tmStr =
        Wisteria::ReportEnumConvert::ConvertTickMarkDisplayToString(axis->GetTickMarkDisplay());
    if (tmStr.has_value() &&
        axis->GetTickMarkDisplay() != Wisteria::GraphItems::Axis::TickMark::DisplayType::Inner)
        {
        tmpl += L", \"tickmarks\": {\"display\": \"" + tmStr.value() + L"\"}";
        }

    // double-sided-labels (default is false)
    if (axis->HasDoubleSidedAxisLabels())
        {
        tmpl += L", \"double-sided-labels\": true";
        }

    // range
    const auto [rangeStart, rangeEnd] = axis->GetRange();
    if (!compare_doubles(rangeStart, 0.0) || !compare_doubles(rangeEnd, 0.0))
        {
        tmpl += wxString::Format(L", \"range\": {\"start\": %g, \"end\": %g", rangeStart, rangeEnd);
        if (axis->GetPrecision() != 0)
            {
            tmpl +=
                wxString::Format(L", \"precision\": %d", static_cast<int>(axis->GetPrecision()));
            }
        if (!compare_doubles(axis->GetInterval(), 0.0))
            {
            tmpl += wxString::Format(L", \"interval\": %g", axis->GetInterval());
            }
        if (axis->GetDisplayInterval() != 1)
            {
            tmpl += wxString::Format(L", \"display-interval\": %zu", axis->GetDisplayInterval());
            }
        tmpl += L"}";
        }

    // precision (outside of range)
    if (axis->GetPrecision() != 0 &&
        (compare_doubles(rangeStart, 0.0) && compare_doubles(rangeEnd, 0.0)))
        {
        tmpl += wxString::Format(L", \"precision\": %d", static_cast<int>(axis->GetPrecision()));
        }

    // label-length
    if (axis->GetLabelLineLength() != 100)
        {
        tmpl += wxString::Format(L", \"label-length\": %zu", axis->GetLabelLineLength());
        }

    // custom-labels
    const auto& customLabels = axis->GetCustomLabels();
    if (!customLabels.empty())
        {
        tmpl += L", \"custom-labels\": [";
        bool first = true;
        for (const auto& [value, label] : customLabels)
            {
            if (!first)
                {
                tmpl += L", ";
                }
            first = false;
            const auto labelTextTmpl = label.GetPropertyTemplate(L"text");
            const auto& labelText = labelTextTmpl.empty() ? label.GetText() : labelTextTmpl;
            tmpl += wxString::Format(L"{\"value\": %g, \"label\": \"%s\"}", value,
                                     EscapeJsonStr(labelText));
            }
        tmpl += L"]";
        }

    // brackets
    const auto& brackets = axis->GetBrackets();
    if (!brackets.empty())
        {
        const auto bracketDsName = axis->GetPropertyTemplate(L"brackets.dataset");
        if (!bracketDsName.empty())
            {
            // dataset-based brackets
            tmpl += L", \"brackets\": {\"dataset\": \"" + EscapeJsonStr(bracketDsName) + L"\"";
            const auto labelVar = axis->GetPropertyTemplate(L"bracket.label");
            const auto valueVar = axis->GetPropertyTemplate(L"bracket.value");
            if (!labelVar.empty() || !valueVar.empty())
                {
                tmpl += L", \"variables\": {";
                bool needComma = false;
                if (!labelVar.empty())
                    {
                    tmpl += L"\"label\": \"" + EscapeJsonStr(labelVar) + L"\"";
                    needComma = true;
                    }
                if (!valueVar.empty())
                    {
                    if (needComma)
                        {
                        tmpl += L", ";
                        }
                    tmpl += L"\"value\": \"" + EscapeJsonStr(valueVar) + L"\"";
                    }
                tmpl += L"}";
                }
            // pen from first bracket
            if (!brackets.empty())
                {
                const auto& bPen = brackets[0].GetLinePen();
                if (bPen.IsOk() && bPen != wxNullPen &&
                    !(bPen.GetColour() == *wxBLACK && bPen.GetWidth() <= 2 &&
                      bPen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    tmpl += L", \"pen\": " + SavePenToStr(bPen);
                    }
                }
            // style from first bracket
            if (!brackets.empty())
                {
                const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                    brackets[0].GetBracketLineStyle());
                if (bsStr.has_value() &&
                    brackets[0].GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                    {
                    tmpl += L", \"style\": \"" + bsStr.value() + L"\"";
                    }
                }
            tmpl += L"}";
            }
        else
            {
            // individually defined brackets
            tmpl += L", \"brackets\": [";
            for (size_t i = 0; i < brackets.size(); ++i)
                {
                if (i > 0)
                    {
                    tmpl += L", ";
                    }
                const auto& b = brackets[i];
                tmpl += wxString::Format(L"{\"start\": %g, \"end\": %g", b.GetStartPosition(),
                                         b.GetEndPosition());
                if (!b.GetLabel().GetText().empty())
                    {
                    tmpl += L", \"label\": \"" + EscapeJsonStr(b.GetLabel().GetText()) + L"\"";
                    }
                const auto& bPen = b.GetLinePen();
                if (bPen.IsOk() && bPen != wxNullPen &&
                    !(bPen.GetColour() == *wxBLACK && bPen.GetWidth() <= 2 &&
                      bPen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    tmpl += L", \"pen\": " + SavePenToStr(bPen);
                    }
                const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                    b.GetBracketLineStyle());
                if (bsStr.has_value() &&
                    b.GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                    {
                    tmpl += L", \"style\": \"" + bsStr.value() + L"\"";
                    }
                tmpl += L"}";
                }
            tmpl += L"]";
            }
        }

    // show (default is true)
    if (!axis->IsShown())
        {
        tmpl += L", \"show\": false";
        }

    // show-outer-labels (default is true)
    if (!axis->IsShowingOuterLabels())
        {
        tmpl += L", \"show-outer-labels\": false";
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    SaveItem(node, axis, canvas);
    return node;
    }

//-------------------------------------------
wxString WisteriaView::GetGraphTypeString(const Wisteria::Graphs::Graph2D* graph)
    {
    if (graph == nullptr)
        {
        return {};
        }

    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        return _DT(L"line-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::MultiSeriesLinePlot)))
        {
        return _DT(L"multi-series-line-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        return _DT(L"scatter-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
        {
        return _DT(L"bubble-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CategoricalBarChart)))
        {
        return _DT(L"categorical-bar-chart");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
        {
        return _DT(L"histogram");
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
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
        {
        return _DT(L"w-curve-plot");
        }
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        return _DT(L"likert-chart");
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
void WisteriaView::SaveGraph(const Wisteria::Graphs::Graph2D* graph, wxSimpleJSON::Ptr_t& graphNode,
                             const Wisteria::Canvas* canvas) const
    {
    if (graph == nullptr || canvas == nullptr)
        {
        return;
        }

    // dataset name (from cached property template)
    const wxString datasetName = graph->GetPropertyTemplate(L"dataset");

    // variables (from cached property templates)
    const auto& templates = graph->GetPropertyTemplates();
    wxString varsStr;
    for (const auto& [prop, val] : templates)
        {
        if (prop.StartsWith(L"variables."))
            {
            const auto varName = prop.Mid(10); // skip "variables."
            if (!varsStr.empty())
                {
                varsStr += L", ";
                }
            varsStr += L"\"" + varName + L"\": \"" + EscapeJsonStr(val) + L"\"";
            }
        }

    // title
    const auto& title = graph->GetTitle();
    wxString titleStr;
    if (!title.GetText().empty())
        {
        const auto textTmpl = title.GetPropertyTemplate(L"text");
        const auto& titleText = textTmpl.empty() ? title.GetText() : textTmpl;
        titleStr = L"\"title\": {\"text\": \"" + EscapeJsonStr(titleText) + L"\"}";
        }

    // subtitle
    const auto& subtitle = graph->GetSubtitle();
    wxString subtitleStr;
    if (!subtitle.GetText().empty())
        {
        const auto textTmpl = subtitle.GetPropertyTemplate(L"text");
        const auto& stText = textTmpl.empty() ? subtitle.GetText() : textTmpl;
        subtitleStr = L"\"sub-title\": {\"text\": \"" + EscapeJsonStr(stText) + L"\"}";
        }

    // caption
    const auto& caption = graph->GetCaption();
    wxString captionStr;
    if (!caption.GetText().empty())
        {
        const auto textTmpl = caption.GetPropertyTemplate(L"text");
        const auto& capText = textTmpl.empty() ? caption.GetText() : textTmpl;
        captionStr = L"\"caption\": {\"text\": \"" + EscapeJsonStr(capText) + L"\"}";
        }

    // background-color
    const auto& bgColor = graph->GetPlotBackgroundColor();
    if (bgColor.IsOk() && !bgColor.IsTransparent())
        {
        graphNode->Add(L"background-color", bgColor.GetAsString(wxC2S_HTML_SYNTAX));
        }

    // axes
    const Wisteria::GraphItems::Axis* axes[] = { &graph->GetBottomXAxis(), &graph->GetTopXAxis(),
                                                 &graph->GetLeftYAxis(), &graph->GetRightYAxis() };
    wxString axesStr;
    for (const auto* axis : axes)
        {
        // skip axes that are default/empty (not shown and have no custom settings)
        const auto atStr =
            Wisteria::ReportEnumConvert::ConvertAxisTypeToString(axis->GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }

        // check if axis has any non-default properties worth writing
        const bool hasTitle = !axis->GetTitle().GetText().empty();
        const auto [rStart, rEnd] = axis->GetRange();
        const bool hasRange = !compare_doubles(rStart, 0.0) || !compare_doubles(rEnd, 0.0);
        const bool hasCustomLabels = !axis->GetCustomLabels().empty();
        const bool hasBrackets = !axis->GetBrackets().empty();
        const bool hasLabelDisplay =
            axis->GetLabelDisplay() != Wisteria::AxisLabelDisplay::DisplayCustomLabelsOrValues;
        const bool hasPenOverride = !axis->GetAxisLinePen().IsOk() ||
                                    axis->GetAxisLinePen() == wxNullPen ||
                                    !(axis->GetAxisLinePen().GetColour() == *wxBLACK &&
                                      axis->GetAxisLinePen().GetWidth() <= 1 &&
                                      axis->GetAxisLinePen().GetStyle() == wxPENSTYLE_SOLID);
        const bool hasGridPenOverride = !axis->GetGridlinePen().IsOk() ||
                                        axis->GetGridlinePen() == wxNullPen ||
                                        !(axis->GetGridlinePen().GetColour() == *wxBLACK &&
                                          axis->GetGridlinePen().GetWidth() <= 1 &&
                                          axis->GetGridlinePen().GetStyle() == wxPENSTYLE_SOLID);
        const bool hasTickOverride =
            axis->GetTickMarkDisplay() != Wisteria::GraphItems::Axis::TickMark::DisplayType::Inner;
        const bool isHidden = !axis->IsShown();

        // skip axes with no meaningful non-default properties
        // (hidden axes with nothing else set, e.g., pie chart axes, are also skipped)
        if (!hasTitle && !hasRange && !hasCustomLabels && !hasBrackets && !hasLabelDisplay)
            {
            continue;
            }

        wxString axisObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";

            // title (always write if axis is being serialized;
            // an empty title overrides the default variable name)
            {
            const auto ttTmpl = axis->GetTitle().GetPropertyTemplate(L"text");
            const auto& ttText = ttTmpl.empty() ? axis->GetTitle().GetText() : ttTmpl;
            axisObj += L", \"title\": {\"text\": \"" + EscapeJsonStr(ttText) + L"\"}";
            }

        // axis-pen
        if (!axis->GetAxisLinePen().IsOk() || axis->GetAxisLinePen() == wxNullPen)
            {
            axisObj += L", \"axis-pen\": null";
            }
        else if (hasPenOverride)
            {
            axisObj += L", \"axis-pen\": " + SavePenToStr(axis->GetAxisLinePen());
            }

        // gridline-pen
        if (!axis->GetGridlinePen().IsOk() || axis->GetGridlinePen() == wxNullPen)
            {
            axisObj += L", \"gridline-pen\": null";
            }
        else if (hasGridPenOverride)
            {
            axisObj += L", \"gridline-pen\": " + SavePenToStr(axis->GetGridlinePen());
            }

        // label-display
        if (hasLabelDisplay)
            {
            const auto ldStr = Wisteria::ReportEnumConvert::ConvertAxisLabelDisplayToString(
                axis->GetLabelDisplay());
            if (ldStr.has_value())
                {
                axisObj += L", \"label-display\": \"" + ldStr.value() + L"\"";
                }
            }

        // tickmarks
        if (hasTickOverride)
            {
            const auto tdStr = Wisteria::ReportEnumConvert::ConvertTickMarkDisplayToString(
                axis->GetTickMarkDisplay());
            if (tdStr.has_value())
                {
                axisObj += L", \"tickmarks\": {\"display\": \"" + tdStr.value() + L"\"}";
                }
            }

        // range
        if (hasRange)
            {
            axisObj += wxString::Format(L", \"range\": {\"start\": %g, \"end\": %g", rStart, rEnd);
            if (axis->GetPrecision() != 0)
                {
                axisObj += wxString::Format(L", \"precision\": %d",
                                            static_cast<int>(axis->GetPrecision()));
                }
            if (!compare_doubles(axis->GetInterval(), 0.0))
                {
                axisObj += wxString::Format(L", \"interval\": %g", axis->GetInterval());
                }
            if (axis->GetDisplayInterval() != 1)
                {
                axisObj +=
                    wxString::Format(L", \"display-interval\": %zu", axis->GetDisplayInterval());
                }
            axisObj += L"}";
            }

        // custom-labels
        if (hasCustomLabels)
            {
            axisObj += L", \"custom-labels\": [";
            bool first = true;
            for (const auto& [value, label] : axis->GetCustomLabels())
                {
                if (!first)
                    {
                    axisObj += L", ";
                    }
                first = false;
                const auto ltTmpl = label.GetPropertyTemplate(L"text");
                const auto& ltText = ltTmpl.empty() ? label.GetText() : ltTmpl;
                axisObj += wxString::Format(L"{\"value\": %g, \"label\": \"%s\"}", value,
                                            EscapeJsonStr(ltText));
                }
            axisObj += L"]";
            }

        // brackets
        if (hasBrackets)
            {
            const auto& brackets = axis->GetBrackets();
            const auto bracketDsName = axis->GetPropertyTemplate(L"brackets.dataset");
            if (!bracketDsName.empty())
                {
                axisObj +=
                    L", \"brackets\": {\"dataset\": \"" + EscapeJsonStr(bracketDsName) + L"\"";
                const auto labelVar = axis->GetPropertyTemplate(L"bracket.label");
                const auto valueVar = axis->GetPropertyTemplate(L"bracket.value");
                if (!labelVar.empty() || !valueVar.empty())
                    {
                    axisObj += L", \"variables\": {";
                    bool needComma = false;
                    if (!labelVar.empty())
                        {
                        axisObj += L"\"label\": \"" + EscapeJsonStr(labelVar) + L"\"";
                        needComma = true;
                        }
                    if (!valueVar.empty())
                        {
                        if (needComma)
                            {
                            axisObj += L", ";
                            }
                        axisObj += L"\"value\": \"" + EscapeJsonStr(valueVar) + L"\"";
                        }
                    axisObj += L"}";
                    }
                axisObj += L"}";
                }
            else
                {
                axisObj += L", \"brackets\": [";
                for (size_t i = 0; i < brackets.size(); ++i)
                    {
                    if (i > 0)
                        {
                        axisObj += L", ";
                        }
                    const auto& b = brackets[i];
                    axisObj += wxString::Format(L"{\"start\": %g, \"end\": %g",
                                                b.GetStartPosition(), b.GetEndPosition());
                    if (!b.GetLabel().GetText().empty())
                        {
                        axisObj +=
                            L", \"label\": \"" + EscapeJsonStr(b.GetLabel().GetText()) + L"\"";
                        }
                    const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                        b.GetBracketLineStyle());
                    if (bsStr.has_value() &&
                        b.GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                        {
                        axisObj += L", \"style\": \"" + bsStr.value() + L"\"";
                        }
                    axisObj += L"}";
                    }
                axisObj += L"]";
                }
            }

        // show (default is true)
        if (isHidden)
            {
            axisObj += L", \"show\": false";
            }

        axisObj += L"}";

        if (!axesStr.empty())
            {
            axesStr += L", ";
            }
        axesStr += axisObj;
        }

    // reference-lines
    const auto& refLines = graph->GetReferenceLines();
    wxString refLinesStr;
    for (const auto& rl : refLines)
        {
        const auto atStr = Wisteria::ReportEnumConvert::ConvertAxisTypeToString(rl.GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }
        wxString rlObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";
        rlObj += wxString::Format(L", \"position\": %g", rl.GetAxisPosition());
        if (!rl.GetLabel().empty())
            {
            rlObj += L", \"label\": \"" + EscapeJsonStr(rl.GetLabel()) + L"\"";
            }
        const auto& rlPen = rl.GetPen();
        if (rlPen.IsOk() && rlPen != wxNullPen)
            {
            rlObj += L", \"pen\": " + SavePenToStr(rlPen);
            }
        if (rl.GetLabelPlacement() != Wisteria::ReferenceLabelPlacement::Legend)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertReferenceLabelPlacementToString(
                rl.GetLabelPlacement());
            if (lpStr.has_value())
                {
                rlObj += L", \"reference-label-placement\": \"" + lpStr.value() + L"\"";
                }
            }
        rlObj += L"}";
        if (!refLinesStr.empty())
            {
            refLinesStr += L", ";
            }
        refLinesStr += rlObj;
        }

    // reference-areas
    const auto& refAreas = graph->GetReferenceAreas();
    wxString refAreasStr;
    for (const auto& ra : refAreas)
        {
        const auto atStr = Wisteria::ReportEnumConvert::ConvertAxisTypeToString(ra.GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }
        wxString raObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";
        raObj += wxString::Format(L", \"start\": %g, \"end\": %g", ra.GetAxisPosition(),
                                  ra.GetAxisPosition2());
        if (!ra.GetLabel().empty())
            {
            raObj += L", \"label\": \"" + EscapeJsonStr(ra.GetLabel()) + L"\"";
            }
        const auto& raPen = ra.GetPen();
        if (raPen.IsOk() && raPen != wxNullPen)
            {
            raObj += L", \"pen\": " + SavePenToStr(raPen);
            }
        if (ra.GetReferenceAreaStyle() != Wisteria::ReferenceAreaStyle::Solid)
            {
            const auto rasStr = Wisteria::ReportEnumConvert::ConvertReferenceAreaStyleToString(
                ra.GetReferenceAreaStyle());
            if (rasStr.has_value())
                {
                raObj += L", \"style\": \"" + rasStr.value() + L"\"";
                }
            }
        raObj += L"}";
        if (!refAreasStr.empty())
            {
            refAreasStr += L", ";
            }
        refAreasStr += raObj;
        }

    // now build the template with sub-objects embedded
    wxString additions;
    if (!datasetName.empty())
        {
        additions += L", \"dataset\": \"" + EscapeJsonStr(datasetName) + L"\"";
        }
    if (!varsStr.empty())
        {
        additions += L", \"variables\": {" + varsStr + L"}";
        }
    if (!titleStr.empty())
        {
        additions += L", " + titleStr;
        }
    if (!subtitleStr.empty())
        {
        additions += L", " + subtitleStr;
        }
    if (!captionStr.empty())
        {
        additions += L", " + captionStr;
        }
    if (!axesStr.empty())
        {
        additions += L", \"axes\": [" + axesStr + L"]";
        }
    if (!refLinesStr.empty())
        {
        additions += L", \"reference-lines\": [" + refLinesStr + L"]";
        }
    if (!refAreasStr.empty())
        {
        additions += L", \"reference-areas\": [" + refAreasStr + L"]";
        }

    // pen (item-level pen for the graph)
    const auto& graphPen = graph->GetPen();
    if (!graphPen.IsOk() || graphPen == wxNullPen)
        {
        additions += L", \"pen\": null";
        }
    else if (!(graphPen.GetColour() == *wxBLACK && graphPen.GetWidth() <= 1 &&
               graphPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        additions += L", \"pen\": " + SavePenToStr(graphPen);
        }

    // brush-scheme
    if (graph->GetBrushScheme() != nullptr && !graph->GetBrushScheme()->GetBrushes().empty())
        {
        const auto& brushes = graph->GetBrushScheme()->GetBrushes();
        bool allSolid = true;
        for (const auto& br : brushes)
            {
            if (br.GetStyle() != wxBRUSHSTYLE_SOLID)
                {
                allSolid = false;
                break;
                }
            }

        wxString colorsArr = L"[";
        for (size_t i = 0; i < brushes.size(); ++i)
            {
            if (i > 0)
                {
                colorsArr += L", ";
                }
            colorsArr += L"\"" + brushes[i].GetColour().GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
            }
        colorsArr += L"]";

        if (allSolid)
            {
            additions += L", \"brush-scheme\": {\"color-scheme\": " + colorsArr + L"}";
            }
        else
            {
            wxString stylesArr = L"[";
            for (size_t i = 0; i < brushes.size(); ++i)
                {
                if (i > 0)
                    {
                    stylesArr += L", ";
                    }
                const auto bsStr =
                    Wisteria::ReportEnumConvert::ConvertBrushStyleToString(brushes[i].GetStyle());
                stylesArr +=
                    L"\"" + (bsStr.has_value() ? bsStr.value() : wxString(L"solid")) + L"\"";
                }
            stylesArr += L"]";
            additions += L", \"brush-scheme\": {\"brush-styles\": " + stylesArr +
                         L", \"color-scheme\": " + colorsArr + L"}";
            }
        }

    // color-scheme (only if no brush-scheme was written, since brush-scheme embeds its own)
    if ((graph->GetBrushScheme() == nullptr || graph->GetBrushScheme()->GetBrushes().empty()) &&
        graph->GetColorScheme() != nullptr && !graph->GetColorScheme()->GetColors().empty())
        {
        wxString colorsArr = L"[";
        const auto& colors = graph->GetColorScheme()->GetColors();
        for (size_t i = 0; i < colors.size(); ++i)
            {
            if (i > 0)
                {
                colorsArr += L", ";
                }
            colorsArr += L"\"" + colors[i].GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
            }
        colorsArr += L"]";
        additions += L", \"color-scheme\": " + colorsArr;
        }

    // icon-scheme
    if (graph->GetShapeScheme() != nullptr && !graph->GetShapeScheme()->GetShapes().empty())
        {
        wxString iconsArr = L"[";
        const auto& shapes = graph->GetShapeScheme()->GetShapes();
        for (size_t i = 0; i < shapes.size(); ++i)
            {
            if (i > 0)
                {
                iconsArr += L", ";
                }
            const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shapes[i]);
            iconsArr +=
                L"\"" + (iconStr.has_value() ? iconStr.value() : wxString(L"blank-icon")) + L"\"";
            }
        iconsArr += L"]";
        additions += L", \"icon-scheme\": " + iconsArr;
        }

    // rebuild node with sub-objects
    if (!additions.empty())
        {
        // get existing JSON, inject additions before closing brace
        auto printed = graphNode->Print(false);
        if (printed.EndsWith(L"}"))
            {
            printed.RemoveLast();
            printed += additions + L"}";
            graphNode = wxSimpleJSON::Create(printed);
            }
        }

    SaveItem(graphNode, graph, canvas);
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SaveGraphByType(const Wisteria::Graphs::Graph2D* graph,
                                                  const Wisteria::Canvas* canvas) const
    {
    if (graph == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto typeStr = GetGraphTypeString(graph);
    if (typeStr.empty())
        {
        return wxSimpleJSON::Ptr_t{};
        }

    auto node =
        wxSimpleJSON::Create(L"{\"type\": \"" + typeStr +
                             L"\", \"canvas-margins\": [], \"padding\": [], \"outline\": []}");

    SaveGraph(graph, node, canvas);

    // type-specific options
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        const auto* linePlot = dynamic_cast<const Wisteria::Graphs::LinePlot*>(graph);
        // LinePlot default ghost opacity is max(75, GHOST_OPACITY)
        if (linePlot->GetGhostOpacity() != std::max<uint8_t>(75, Wisteria::Settings::GHOST_OPACITY))
            {
            node->Add(L"ghost-opacity", static_cast<double>(linePlot->GetGhostOpacity()));
            }
        // showcase-lines (cached as indexed templates: showcase-lines[0], [1], ...)
        wxString showcaseArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                graph->GetPropertyTemplate(L"showcase-lines[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!showcaseArr.empty())
                {
                showcaseArr += L", ";
                }
            showcaseArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!showcaseArr.empty())
            {
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"showcase-lines\": [" + showcaseArr + L"]}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        // line-scheme
        if (linePlot->GetLineStyleScheme() != nullptr &&
            !linePlot->GetLineStyleScheme()->GetLineStyles().empty())
            {
            const auto& lineStyles = linePlot->GetLineStyleScheme()->GetLineStyles();
            // skip if it's just the default (single solid+lines entry)
            const bool isDefault =
                (lineStyles.size() == 1 && lineStyles.front().first == wxPENSTYLE_SOLID &&
                 lineStyles.front().second == Wisteria::LineStyle::Lines);
            if (!isDefault)
                {
                wxString lsArr = L"[";
                for (size_t i = 0; i < lineStyles.size(); ++i)
                    {
                    if (i > 0)
                        {
                        lsArr += L", ";
                        }
                    lsArr += L"{";
                    const auto psStr =
                        Wisteria::ReportEnumConvert::ConvertPenStyleToString(lineStyles[i].first);
                    if (psStr.has_value() && lineStyles[i].first != wxPENSTYLE_SOLID)
                        {
                        lsArr += L"\"pen-style\": {\"style\": \"" + psStr.value() + L"\"}, ";
                        }
                    const auto lStr =
                        Wisteria::ReportEnumConvert::ConvertLineStyleToString(lineStyles[i].second);
                    lsArr += L"\"line-style\": \"" +
                             (lStr.has_value() ? lStr.value() : wxString(L"lines")) + L"\"}";
                    }
                lsArr += L"]";
                auto printed2 = node->Print(false);
                if (printed2.EndsWith(L"}"))
                    {
                    printed2.RemoveLast();
                    printed2 += L", \"line-scheme\": " + lsArr + L"}";
                    node = wxSimpleJSON::Create(printed2);
                    }
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        const auto* scatterPlot = dynamic_cast<const Wisteria::Graphs::ScatterPlot*>(graph);
        if (scatterPlot->IsShowingRegressionLines())
            {
            node->Add(L"show-regression-lines", true);
            }
        if (scatterPlot->IsShowingConfidenceBands())
            {
            node->Add(L"show-confidence-bands", true);
            }
        if (!compare_doubles(scatterPlot->GetConfidenceLevel(), 0.95))
            {
            node->Add(L"confidence-level", scatterPlot->GetConfidenceLevel());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BarChart)))
        {
        const auto* barChart = dynamic_cast<const Wisteria::Graphs::BarChart*>(graph);
        // shared BarChart options
        const auto beStr =
            Wisteria::ReportEnumConvert::ConvertBoxEffectToString(barChart->GetBarEffect());
        if (beStr.has_value() && barChart->GetBarEffect() != Wisteria::BoxEffect::Solid)
            {
            node->Add(L"box-effect", beStr.value());
            }
        if (barChart->GetBarOrientation() == Wisteria::Orientation::Horizontal)
            {
            node->Add(L"bar-orientation", wxString{ _DT(L"horizontal") });
            }
        if (barChart->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
            {
            const auto ndStr = Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(
                barChart->GetNumberDisplay());
            if (ndStr.has_value())
                {
                node->Add(L"number-display", ndStr.value());
                }
            }
        if (barChart->GetBinLabelDisplay() != Wisteria::BinLabelDisplay::BinValue)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                barChart->GetBinLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"bar-label-display", blStr.value());
                }
            }
        if (barChart->GetBarGroupPlacement() != Wisteria::LabelPlacement::NextToParent)
            {
            const auto bgStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                barChart->GetBarGroupPlacement());
            if (bgStr.has_value())
                {
                node->Add(L"bar-group-placement", bgStr.value());
                }
            }
        // bar-shapes (if all bars have the same non-default shape)
        if (!barChart->GetBars().empty())
            {
            const auto firstShape = barChart->GetBars().front().GetShape();
            if (firstShape != Wisteria::Graphs::BarChart::BarShape::Rectangle)
                {
                bool allSame = true;
                for (const auto& bar : barChart->GetBars())
                    {
                    if (bar.GetShape() != firstShape)
                        {
                        allSame = false;
                        break;
                        }
                    }
                if (allSame)
                    {
                    const auto bsStr =
                        Wisteria::ReportEnumConvert::ConvertBarShapeToString(firstShape);
                    if (bsStr.has_value())
                        {
                        node->Add(L"bar-shapes", bsStr.value());
                        }
                    }
                }
            }
        if (barChart->GetGhostOpacity() != Wisteria::Settings::GHOST_OPACITY)
            {
            node->Add(L"ghost-opacity", static_cast<double>(barChart->GetGhostOpacity()));
            }
        // showcase-bars (indexed templates)
        wxString showcaseBarArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                graph->GetPropertyTemplate(L"showcase-bars[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!showcaseBarArr.empty())
                {
                showcaseBarArr += L", ";
                }
            showcaseBarArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!showcaseBarArr.empty())
            {
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"showcase-bars\": [" + showcaseBarArr + L"]}";
                node = wxSimpleJSON::Create(printed);
                }
            }

        // decals
        wxString decalsArr;
        for (size_t bi = 0; bi < barChart->GetBars().size(); ++bi)
            {
            const auto& bar = barChart->GetBars()[bi];
            for (size_t bk = 0; bk < bar.GetBlocks().size(); ++bk)
                {
                const auto& decal = bar.GetBlocks()[bk].GetDecal();
                if (decal.GetText().empty())
                    {
                    continue;
                    }
                if (!decalsArr.empty())
                    {
                    decalsArr += L", ";
                    }
                const auto textTmpl = decal.GetPropertyTemplate(L"text");
                const auto& decalText = textTmpl.empty() ? decal.GetText() : textTmpl;
                decalsArr += L"{\"bar\": \"" + EscapeJsonStr(bar.GetAxisLabel().GetText()) + L"\"";
                if (bk != 0)
                    {
                    decalsArr += L", \"block\": " + std::to_wstring(bk);
                    }
                decalsArr += L", \"decal\": {\"text\": \"" + EscapeJsonStr(decalText) + L"\"";
                const auto& decalColor = decal.GetFontColor();
                if (decalColor.IsOk() && decalColor != *wxBLACK)
                    {
                    decalsArr +=
                        L", \"color\": \"" + decalColor.GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
                    }
                const auto& decalBgColor = decal.GetFontBackgroundColor();
                if (decalBgColor.IsOk())
                    {
                    decalsArr += L", \"background\": \"" +
                                 decalBgColor.GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
                    }
                decalsArr += L"}}";
                }
            }
        if (!decalsArr.empty())
            {
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"decals\": [" + decalsArr + L"]}";
                node = wxSimpleJSON::Create(printed);
                }
            }

        if (!barChart->GetBinLabelSuffix().empty())
            {
            node->Add(L"bar-label-suffix", barChart->GetBinLabelSuffix());
            }
        if (barChart->IsIncludingSpacesBetweenBars())
            {
            node->Add(L"include-spaces-between-bars", true);
            }
        if (barChart->IsConstrainingScalingAxisToBars())
            {
            node->Add(L"constrain-scaling-axis-to-bars", true);
            }
        if (barChart->IsApplyingBrushesToUngroupedBars())
            {
            node->Add(L"apply-brushes-to-ungrouped-bars", true);
            }
        if (!barChart->IsHidingGhostedLabels())
            {
            node->Add(L"hide-ghosted-labels", false);
            }
        // bar-sort (only if explicitly set via JSON, not from SetData's internal sort)
        const auto barSortTemplate = graph->GetPropertyTemplate(L"bar-sort");
        if (!barSortTemplate.empty())
            {
            wxString sortObj = L"{\"direction\": \"";
            sortObj += (barChart->GetSortDirection() == Wisteria::SortDirection::SortAscending) ?
                           L"ascending" :
                           L"descending";
            sortObj += L"\"";
            if (barChart->GetSortComparison().has_value())
                {
                sortObj += L", \"by\": \"";
                sortObj += (barChart->GetSortComparison().value() ==
                            Wisteria::Graphs::BarChart::BarSortComparison::SortByBarLength) ?
                               L"length" :
                               L"label";
                sortObj += L"\"";
                }
            else if (!barChart->GetSortLabels().empty())
                {
                sortObj += L", \"labels\": [";
                for (size_t i = 0; i < barChart->GetSortLabels().size(); ++i)
                    {
                    if (i > 0)
                        {
                        sortObj += L", ";
                        }
                    sortObj += L"\"" + EscapeJsonStr(barChart->GetSortLabels()[i]) + L"\"";
                    }
                sortObj += L"]";
                }
            sortObj += L"}";
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"bar-sort\": " + sortObj + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        // bar-groups
        if (!barChart->GetBarGroups().empty())
            {
            wxString bgArr = L"[";
            for (size_t i = 0; i < barChart->GetBarGroups().size(); ++i)
                {
                const auto& bg = barChart->GetBarGroups()[i];
                if (i > 0)
                    {
                    bgArr += L", ";
                    }
                bgArr += L"{";
                bool hasField = false;
                if (bg.m_barColor.IsOk())
                    {
                    bgArr +=
                        L"\"color\": \"" + bg.m_barColor.GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
                    hasField = true;
                    }
                if (bg.m_barBrush.IsOk() && bg.m_barBrush != wxNullBrush)
                    {
                    bgArr += wxString(hasField ? L", " : L"") + L"\"brush\": " +
                             SaveBrushToStr(bg.m_barBrush);
                    hasField = true;
                    }
                bgArr += wxString(hasField ? L", " : L"") + L"\"start\": " +
                         std::to_wstring(bg.m_barPositions.first);
                bgArr += L", \"end\": " + std::to_wstring(bg.m_barPositions.second);
                if (!bg.m_barDecal.empty())
                    {
                    bgArr += L", \"decal\": \"" + EscapeJsonStr(bg.m_barDecal) + L"\"";
                    }
                bgArr += L"}";
                }
            bgArr += L"]";
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"bar-groups\": " + bgArr + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }

        // Histogram-specific options
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
            {
            const auto* histo = dynamic_cast<const Wisteria::Graphs::Histogram*>(graph);
            if (histo->GetBinningMethod() !=
                Wisteria::Graphs::Histogram::BinningMethod::BinByIntegerRange)
                {
                const auto bmStr = Wisteria::ReportEnumConvert::ConvertBinningMethodToString(
                    histo->GetBinningMethod());
                if (bmStr.has_value())
                    {
                    node->Add(L"binning-method", bmStr.value());
                    }
                }
            if (histo->GetIntervalDisplay() !=
                Wisteria::Graphs::Histogram::IntervalDisplay::Cutpoints)
                {
                const auto idStr = Wisteria::ReportEnumConvert::ConvertIntervalDisplayToString(
                    histo->GetIntervalDisplay());
                if (idStr.has_value())
                    {
                    node->Add(L"interval-display", idStr.value());
                    }
                }
            if (histo->GetRoundingMethod() != Wisteria::RoundingMethod::NoRounding)
                {
                const auto rmStr = Wisteria::ReportEnumConvert::ConvertRoundingMethodToString(
                    histo->GetRoundingMethod());
                if (rmStr.has_value())
                    {
                    node->Add(L"rounding", rmStr.value());
                    }
                }
            if (!histo->IsShowingFullRangeOfValues())
                {
                node->Add(L"show-full-range", false);
                }
            if (histo->GetBinsStart().has_value())
                {
                node->Add(L"bins-start", histo->GetBinsStart().value());
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        const auto* pieChart = dynamic_cast<const Wisteria::Graphs::PieChart*>(graph);
        if (pieChart->GetPieSliceEffect() != Wisteria::PieSliceEffect::Solid)
            {
            const auto seStr = Wisteria::ReportEnumConvert::ConvertPieSliceEffectToString(
                pieChart->GetPieSliceEffect());
            if (seStr.has_value())
                {
                node->Add(L"pie-slice-effect", seStr.value());
                }
            }
        if (pieChart->GetPieStyle() != Wisteria::PieStyle::None)
            {
            const auto psStr =
                Wisteria::ReportEnumConvert::ConvertPieStyleToString(pieChart->GetPieStyle());
            if (psStr.has_value())
                {
                node->Add(L"pie-style", psStr.value());
                }
            }
        if (pieChart->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                pieChart->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (pieChart->GetOuterPieMidPointLabelDisplay() != Wisteria::BinLabelDisplay::BinPercentage)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetOuterPieMidPointLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"outer-pie-midpoint-label-display", blStr.value());
                }
            }
        if (pieChart->GetInnerPieMidPointLabelDisplay() != Wisteria::BinLabelDisplay::BinPercentage)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetInnerPieMidPointLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"inner-pie-midpoint-label-display", blStr.value());
                }
            }
        if (pieChart->GetOuterLabelDisplay() != Wisteria::BinLabelDisplay::BinName)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetOuterLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"outer-label-display", blStr.value());
                }
            }
        if (!pieChart->IsShowingOuterPieLabels())
            {
            node->Add(L"include-outer-pie-labels", false);
            }
        if (!pieChart->IsShowingInnerPieLabels())
            {
            node->Add(L"include-inner-pie-labels", false);
            }
        if (pieChart->IsUsingColorLabels())
            {
            node->Add(L"color-labels", true);
            }
        if (pieChart->HasDynamicMargins())
            {
            node->Add(L"dynamic-margins", true);
            }
        if (pieChart->GetGhostOpacity() != Wisteria::Settings::GHOST_OPACITY)
            {
            node->Add(L"ghost-opacity", static_cast<double>(pieChart->GetGhostOpacity()));
            }
        // showcase-slices
        const auto showcaseMode = pieChart->GetShowcaseMode();
        if (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::ExplicitList)
            {
            // array form (indexed templates)
            wxString showcaseSliceArr;
            for (size_t i = 0;; ++i)
                {
                const auto val =
                    graph->GetPropertyTemplate(L"showcase-slices[" + std::to_wstring(i) + L"]");
                if (val.empty())
                    {
                    break;
                    }
                if (!showcaseSliceArr.empty())
                    {
                    showcaseSliceArr += L", ";
                    }
                showcaseSliceArr += L"\"" + EscapeJsonStr(val) + L"\"";
                }
            if (!showcaseSliceArr.empty())
                {
                auto printed = node->Print(false);
                if (printed.EndsWith(L"}"))
                    {
                    printed.RemoveLast();
                    printed += L", \"showcase-slices\": [" + showcaseSliceArr + L"]}";
                    node = wxSimpleJSON::Create(printed);
                    }
                }
            }
        else if (showcaseMode != Wisteria::Graphs::PieChart::ShowcaseMode::None)
            {
            // object form
            wxString scObj = L"{";
            const bool isInner =
                (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestInner ||
                 showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::SmallestInner);
            scObj += isInner ? L"\"pie\": \"inner\"" : L"\"pie\": \"outer\"";
            const bool isLargest =
                (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestOuter ||
                 showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestInner);
            scObj += isLargest ? L", \"category\": \"largest\"" : L", \"category\": \"smallest\"";
            if (isInner)
                {
                scObj += pieChart->IsShowcaseByGroup() ? L", \"by-group\": true" :
                                                         L", \"by-group\": false";
                scObj += pieChart->IsShowcaseShowingOuterPieMidPointLabels() ?
                             L", \"show-outer-pie-midpoint-labels\": true" :
                             L", \"show-outer-pie-midpoint-labels\": false";
                }
            scObj += L"}";
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"showcase-slices\": " + scObj + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        // showcased-ring-labels (for any showcase mode)
        if (showcaseMode != Wisteria::Graphs::PieChart::ShowcaseMode::None)
            {
            const auto periStr = Wisteria::ReportEnumConvert::ConvertPerimeterToString(
                pieChart->GetShowcasedRingLabels());
            if (periStr.has_value() &&
                pieChart->GetShowcasedRingLabels() != Wisteria::Perimeter::Outer)
                {
                node->Add(L"showcased-ring-labels", periStr.value());
                }
            }
        // inner-pie-line-pen
        const auto& ipPen = pieChart->GetInnerPieConnectionLinePen();
        if (ipPen.IsOk() && ipPen != wxNullPen)
            {
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"inner-pie-line-pen\": " + SavePenToStr(ipPen) + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        // margin notes
        if (!pieChart->GetLeftMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetLeftMarginNote(), canvas);
            if (marginNode)
                {
                auto printed = node->Print(false);
                if (printed.EndsWith(L"}"))
                    {
                    printed.RemoveLast();
                    printed += L", \"left-margin-note\": " + marginNode->Print(false) + L"}";
                    node = wxSimpleJSON::Create(printed);
                    }
                }
            }
        if (!pieChart->GetRightMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetRightMarginNote(), canvas);
            if (marginNode)
                {
                auto printed = node->Print(false);
                if (printed.EndsWith(L"}"))
                    {
                    printed.RemoveLast();
                    printed += L", \"right-margin-note\": " + marginNode->Print(false) + L"}";
                    node = wxSimpleJSON::Create(printed);
                    }
                }
            }
        // donut hole
        if (pieChart->IsIncludingDonutHole())
            {
            wxString donutStr = L"{";
            if (!pieChart->GetDonutHoleLabel().GetText().empty())
                {
                const auto labelNode = SaveLabel(&pieChart->GetDonutHoleLabel(), canvas);
                if (labelNode)
                    {
                    donutStr += L"\"label\": " + labelNode->Print(false);
                    }
                }
            if (!compare_doubles(pieChart->GetDonutHoleProportion(), math_constants::half))
                {
                if (donutStr.length() > 1)
                    {
                    donutStr += L", ";
                    }
                donutStr +=
                    wxString::Format(L"\"proportion\": %g", pieChart->GetDonutHoleProportion());
                }
            const wxColour defaultDonutColor{ Wisteria::Colors::ColorBrewer::GetColor(
                Wisteria::Colors::Color::White) };
            if (pieChart->GetDonutHoleColor() != defaultDonutColor)
                {
                if (donutStr.length() > 1)
                    {
                    donutStr += L", ";
                    }
                donutStr += L"\"color\": \"" +
                            pieChart->GetDonutHoleColor().GetAsString(wxC2S_HTML_SYNTAX) + L"\"";
                }
            donutStr += L"}";
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"donut-hole\": " + donutStr + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        const auto* boxPlot = dynamic_cast<const Wisteria::Graphs::BoxPlot*>(graph);
        const auto beStr =
            Wisteria::ReportEnumConvert::ConvertBoxEffectToString(boxPlot->GetBoxEffect());
        if (beStr.has_value() && boxPlot->GetBoxEffect() != Wisteria::BoxEffect::Solid)
            {
            node->Add(L"box-effect", beStr.value());
            }
        if (boxPlot->IsShowingAllPoints())
            {
            node->Add(L"show-all-points", true);
            }
        if (boxPlot->IsShowingLabels())
            {
            node->Add(L"show-labels", true);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        const auto* chernoff = dynamic_cast<const Wisteria::Graphs::ChernoffFacesPlot*>(graph);
        node->Add(L"face-color", chernoff->GetFaceColor().GetAsString(wxC2S_HTML_SYNTAX));
        node->Add(L"show-labels", chernoff->IsShowingLabels());
        node->Add(L"outline-color", chernoff->GetOutlineColor().GetAsString(wxC2S_HTML_SYNTAX));
        const auto gStr = Wisteria::ReportEnumConvert::ConvertGenderToString(chernoff->GetGender());
        if (gStr.has_value())
            {
            node->Add(L"gender", gStr.value());
            }
        node->Add(L"eye-color", chernoff->GetEyeColor().GetAsString(wxC2S_HTML_SYNTAX));
        node->Add(L"hair-color", chernoff->GetHairColor().GetAsString(wxC2S_HTML_SYNTAX));
        node->Add(L"lipstick-color", chernoff->GetLipstickColor().GetAsString(wxC2S_HTML_SYNTAX));
        const auto hsStr =
            Wisteria::ReportEnumConvert::ConvertHairStyleToString(chernoff->GetHairStyle());
        if (hsStr.has_value())
            {
            node->Add(L"hair-style", hsStr.value());
            }
        const auto fhStr =
            Wisteria::ReportEnumConvert::ConvertFacialHairToString(chernoff->GetFacialHair());
        if (fhStr.has_value())
            {
            node->Add(L"facial-hair", fhStr.value());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        const auto* waffle = dynamic_cast<const Wisteria::Graphs::WaffleChart*>(graph);
        // shapes
        const auto& shapes = waffle->GetShapes();
        if (!shapes.empty())
            {
            wxString shapesArr = L"[";
            for (size_t i = 0; i < shapes.size(); ++i)
                {
                if (i > 0)
                    {
                    shapesArr += L", ";
                    }
                const auto& shp = shapes[i];
                shapesArr += L"{";
                const auto iconStr =
                    Wisteria::ReportEnumConvert::ConvertIconToString(shp.GetShape());
                shapesArr += L"\"icon\": \"" +
                             (iconStr.has_value() ? iconStr.value() : wxString(L"square")) + L"\"";
                if (shp.GetRepeatCount() != 1)
                    {
                    shapesArr += L", \"repeat\": " + std::to_wstring(shp.GetRepeatCount());
                    }
                const auto& pen = shp.GetPen();
                if (pen.IsOk() && pen != wxNullPen &&
                    !(pen.GetColour() == *wxBLACK && pen.GetColour().IsOpaque() &&
                      pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    shapesArr += L", \"pen\": " + SavePenToStr(pen);
                    }
                const auto& brush = shp.GetBrush();
                if (brush.IsOk() && brush != wxNullBrush &&
                    !(brush.GetColour() == *wxWHITE && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
                    {
                    shapesArr += L", \"brush\": " + SaveBrushToStr(brush);
                    }
                if (!shp.GetText().empty())
                    {
                    shapesArr +=
                        L", \"label\": {\"text\": \"" + EscapeJsonStr(shp.GetText()) + L"\"}";
                    }
                if (shp.GetFillPercent() < math_constants::full)
                    {
                    shapesArr += wxString::Format(L", \"fill-percent\": %g", shp.GetFillPercent());
                    }
                shapesArr += L"}";
                }
            shapesArr += L"]";
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += L", \"shapes\": " + shapesArr + L"}";
                node = wxSimpleJSON::Create(printed);
                }
            }
        // grid-round
        if (waffle->GetGridRounding().has_value())
            {
            const auto& gr = waffle->GetGridRounding().value();
            auto printed = node->Print(false);
            if (printed.EndsWith(L"}"))
                {
                printed.RemoveLast();
                printed += wxString::Format(
                    L", \"grid-round\": {\"cell-count\": %zu, \"shape-index\": %zu}}",
                    gr.m_numberOfCells, gr.m_shapesIndex);
                node = wxSimpleJSON::Create(printed);
                }
            }
        // row-count
        if (waffle->GetRowCount().has_value())
            {
            node->Add(L"row-count", static_cast<double>(waffle->GetRowCount().value()));
            }
        }

    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaView::SavePageItem(const Wisteria::GraphItems::GraphItemBase* item,
                                               const Wisteria::Canvas* canvas) const
    {
    if (item == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    // Graph2D before other types (it inherits from GraphItemBase directly)
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Graph2D)))
        {
        return SaveGraphByType(dynamic_cast<const Wisteria::Graphs::Graph2D*>(item), canvas);
        }

    // (FillableShape before Shape since it derives from Shape)
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::FillableShape)))
        {
        return SaveFillableShape(dynamic_cast<const Wisteria::GraphItems::FillableShape*>(item),
                                 canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Shape)))
        {
        return SaveShape(dynamic_cast<const Wisteria::GraphItems::Shape*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Image)))
        {
        return SaveImage(dynamic_cast<const Wisteria::GraphItems::Image*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Axis)))
        {
        return SaveCommonAxis(dynamic_cast<const Wisteria::GraphItems::Axis*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
        {
        return SaveLabel(dynamic_cast<const Wisteria::GraphItems::Label*>(item), canvas);
        }

    return wxSimpleJSON::Ptr_t{};
    }

//-------------------------------------------
void WisteriaView::SaveProject(const wxString& filePath)
    {
    const wxFileName projectDir(filePath);

    // project name
    const wxString projectName = m_reportBuilder.GetName().empty() ?
                                     wxFileName(filePath).GetName() :
                                     m_reportBuilder.GetName();

    auto root = wxSimpleJSON::Create(
        wxString::Format(L"{\"name\": \"%s\", \"watermark\": {}, \"print\": {}, "
                         L"\"datasets\": [], \"constants\": [], \"pages\": []}",
                         EscapeJsonStr(projectName)),
        true);

    // watermark
    //----------
    const auto& wmLabel = m_reportBuilder.GetWatermarkLabel();
    const auto& wmColor = m_reportBuilder.GetWatermarkColor();
    if (!wmLabel.empty())
        {
        auto wmObj = root->GetProperty(L"watermark");
        wmObj->Add(L"label", wmLabel);
        if (wmColor.IsOk())
            {
            wmObj->Add(L"color", wmColor.GetAsString(wxC2S_HTML_SYNTAX));
            }
        }
    else
        {
        root->DeleteProperty(L"watermark");
        }

    // print settings
    //---------------
    const auto printOrientation = m_reportBuilder.GetPrintOrientation();
    const auto paperSize = m_reportBuilder.GetPaperSize();
    if (printOrientation != wxPrintOrientation::wxPORTRAIT ||
        paperSize != wxPaperSize::wxPAPER_NONE)
        {
        auto printObj = root->GetProperty(L"print");
        if (printOrientation == wxPrintOrientation::wxLANDSCAPE)
            {
            printObj->Add(L"orientation", wxString{ L"landscape" });
            }
        else
            {
            printObj->Add(L"orientation", wxString{ L"portrait" });
            }
        const auto paperStr = Wisteria::ReportEnumConvert::ConvertPaperSizeToString(paperSize);
        if (paperStr.has_value())
            {
            printObj->Add(L"paper-size", paperStr.value());
            }
        }
    else
        {
        root->DeleteProperty(L"print");
        }

    // datasets
    //---------
    const auto& datasets = m_reportBuilder.GetDatasets();
    const auto& importOpts = m_reportBuilder.GetDatasetImportOptions();
    const auto& pivotOpts = m_reportBuilder.GetDatasetPivotOptions();
    const auto& transformOpts = m_reportBuilder.GetDatasetTransformOptions();
    const auto& subsetOpts = m_reportBuilder.GetDatasetSubsetOptions();
    const auto& mergeOpts = m_reportBuilder.GetDatasetMergeOptions();

    // build a lookup of derived dataset names
    // (pivot, subset, and merge datasets reference a parent)
    std::set<wxString, Wisteria::Data::wxStringLessNoCase> derivedDatasets;
    for (const auto& [name, opts] : pivotOpts)
        {
        derivedDatasets.insert(name);
        }
    for (const auto& [name, opts] : subsetOpts)
        {
        derivedDatasets.insert(name);
        }
    for (const auto& [name, opts] : mergeOpts)
        {
        derivedDatasets.insert(name);
        }

    auto datasetsArray = root->GetProperty(L"datasets");
    for (const auto& [dsName, dataset] : datasets)
        {
        // skip derived datasets; they are nested under their parent
        if (derivedDatasets.contains(dsName))
            {
            continue;
            }

        const auto optIt = importOpts.find(dsName);

        // build dataset template with name and path up front
        wxString dsTmpl = L"{";
        dsTmpl += wxString::Format(L"\"name\": \"%s\", ", EscapeJsonStr(dsName));
        if (optIt != importOpts.cend())
            {
            const auto& optsTmpl = optIt->second;
            if (!optsTmpl.m_filePath.empty())
                {
                wxFileName dataPath(optsTmpl.m_filePath);
                dataPath.MakeRelativeTo(projectDir.GetPath());
                dsTmpl += wxString::Format(L"\"path\": \"%s\", ",
                                           EscapeJsonStr(dataPath.GetFullPath(wxPATH_UNIX)));
                }
            }
        dsTmpl += L"\"categorical-columns\": [], \"date-columns\": [], "
                  L"\"recode-re\": [], \"columns-rename\": [], "
                  L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                  L"\"collapse-except\": [], \"formulas\": [], "
                  L"\"subsets\": [], \"pivots\": [], \"merges\": []}";
        auto dsNode = wxSimpleJSON::Create(dsTmpl);

        if (optIt != importOpts.cend())
            {
            const auto& opts = optIt->second;

            // importer override
            if (!opts.m_importer.empty())
                {
                dsNode->Add(L"importer", opts.m_importer);
                }

            // worksheet
            if (std::holds_alternative<wxString>(opts.m_worksheet))
                {
                const auto& ws = std::get<wxString>(opts.m_worksheet);
                if (!ws.empty())
                    {
                    dsNode->Add(L"worksheet", ws);
                    }
                }
            else
                {
                const auto wsIdx = std::get<size_t>(opts.m_worksheet);
                if (wsIdx != 1)
                    {
                    dsNode->Add(L"worksheet", static_cast<double>(wsIdx));
                    }
                }

            SaveDatasetImportOptions(dsNode, opts.m_columnPreviewInfo, opts.m_importInfo);
            }

        // transform options and formulas
        const auto txIt = transformOpts.find(dsName);
        if (txIt != transformOpts.cend())
            {
            SaveTransformOptions(dsNode, txIt->second);
            SaveFormulas(dsNode, txIt->second.m_formulas);
            }

        // nested pivots, subsets, and merges
        SavePivots(dsNode, dsName);
        SaveSubsets(dsNode, dsName);
        SaveMerges(dsNode, dsName);

        // clean up empty arrays
        for (const auto& key :
             { L"categorical-columns", L"date-columns", L"recode-re", L"columns-rename",
               L"mutate-categorical-columns", L"collapse-min", L"collapse-except", L"formulas",
               L"subsets", L"pivots", L"merges" })
            {
            if (dsNode->GetProperty(key)->ArraySize() == 0)
                {
                dsNode->DeleteProperty(key);
                }
            }

        datasetsArray->ArrayAdd(dsNode);
        }

    // constants
    //----------
    const auto& constants = m_reportBuilder.GetConstants();
    if (!constants.empty())
        {
        auto constArray = root->GetProperty(L"constants");
        for (const auto& c : constants)
            {
            auto cObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            cObj->Add(L"name", c.m_name);
            cObj->Add(L"value", c.m_value);
            constArray->ArrayAdd(cObj);
            }
        }
    else
        {
        root->DeleteProperty(L"constants");
        }

    // pages
    //------
    auto pagesArray = root->GetProperty(L"pages");
    for (const auto* canvas : m_pages)
        {
        if (canvas == nullptr)
            {
            continue;
            }

        auto pageObj = wxSimpleJSON::Create(L"{\"rows\": []}");
        if (!canvas->GetLabel().empty())
            {
            pageObj->Add(L"name", canvas->GetLabel());
            }

        auto rowsArray = pageObj->GetProperty(L"rows");
        const auto [rowCount, colCount] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < rowCount; ++row)
            {
            auto rowObj = wxSimpleJSON::Create(L"{\"items\": []}");
            auto itemsArray = rowObj->GetProperty(L"items");
            for (size_t col = 0; col < colCount; ++col)
                {
                const auto item = canvas->GetFixedObject(row, col);
                auto itemNode = SavePageItem(item.get(), canvas);
                if (itemNode != nullptr)
                    {
                    itemsArray->ArrayAdd(itemNode);
                    }
                }
            rowsArray->ArrayAdd(rowObj);
            }

        pagesArray->ArrayAdd(pageObj);
        }

    wxString output = root->Print();
    output.Replace(L"\t", L" ");
    wxFile outFile(filePath, wxFile::write);
    if (outFile.IsOpened())
        {
        const auto utf8 = output.utf8_string();
        outFile.Write(utf8.c_str(), utf8.length());
        }
    }
