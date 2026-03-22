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
#include "../ui/dialogs/inserthistogramdlg.h"
#include "../ui/dialogs/insertimgdlg.h"
#include "../ui/dialogs/insertlabeldlg.h"
#include "../ui/dialogs/insertlineplotdlg.h"
#include "../ui/dialogs/insertlrroadmapdlg.h"
#include "../ui/dialogs/insertpagedlg.h"
#include "../ui/dialogs/insertpiechartdlg.h"
#include "../ui/dialogs/insertproconroadmapdlg.h"
#include "../ui/dialogs/insertscatterplotdlg.h"
#include "../ui/dialogs/insertshapedlg.h"
#include "../ui/dialogs/insertstemandleafdlg.h"
#include "../ui/dialogs/insertwcurvedlg.h"
#include "../ui/dialogs/insertwlsparklinedlg.h"
#include "../ui/dialogs/insertwordclouddlg.h"
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
    doc->SetTitle(title);

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

    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED,
        [this](wxRibbonButtonBarEvent& event) { event.PopupMenu(&m_saveMenu); }, ID_SAVE_PROJECT);
    m_frame->Bind(
        wxEVT_MENU,
        [this](wxCommandEvent& event)
        {
            m_projectFilePath.clear();
            OnSaveProject(event);
        },
        ID_SAVE_PROJECT_AS);

    // bind individual graph menu items
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertChernoffPlot, this, ID_NEW_CHERNOFFPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertScatterPlot, this, ID_NEW_SCATTERPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertLinePlot, this, ID_NEW_LINEPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWCurvePlot, this, ID_NEW_WCURVE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertLRRoadmap, this, ID_NEW_LR_ROADMAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertProConRoadmap, this, ID_NEW_PROCON_ROADMAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertHistogram, this, ID_NEW_HISTOGRAM);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWordCloud, this, ID_NEW_WORD_CLOUD);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWLSparkline, this, ID_NEW_WIN_LOSS_SPARKLINE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertStemAndLeaf, this, ID_NEW_STEMANDLEAF);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertPieChart, this, ID_NEW_PIECHART);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertLabel, this, ID_NEW_LABEL);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertImage, this, ID_NEW_IMAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertShape, this, ID_NEW_SHAPE);

    // bind edit/delete item buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditItem, this, ID_EDIT_ITEM);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnDeleteItem, this, ID_DELETE_ITEM);

    // bind DELETE key to delete selected item
    m_frame->Bind(wxEVT_CHAR_HOOK,
                  [this](wxKeyEvent& event)
                  {
                      if (event.GetKeyCode() == WXK_DELETE)
                          {
                          wxCommandEvent cmd;
                          OnDeleteItem(cmd);
                          }
                      else
                          {
                          event.Skip();
                          }
                  });

    // bind canvas double-click to edit the selected item
    m_frame->Bind(wxEVT_WISTERIA_CANVAS_DCLICK, &WisteriaView::OnCanvasDClick, this);

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

    m_projectFilePath = doc->GetFilename();
    LoadProject();

    if (initialDataset != nullptr)
        {
        AddDatasetToProject(initialDataset, initialDatasetName, initialColumnInfo,
                            { initialFilePath, wxString{}, initialWorksheet, initialFullColumnInfo,
                              initialImportInfo });
        AddPageToProject(1, 1, wxString{});
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

    wxGetApp().DestroyLogWindow();

    Activate(false);

    if (deleteWindow)
        {
        m_frame->Destroy();
        m_frame = nullptr;
        }

    // show the main frame when the last document is being closed
    if (wxGetApp().GetDocumentCount() == 1)
        {
        // show the empty mainframe when the last document is being closed
        wxArrayString mruFiles;
        for (size_t i = 0; i < wxGetApp().GetDocManager()->GetFileHistory()->GetCount(); ++i)
            {
            mruFiles.Add(wxGetApp().GetDocManager()->GetFileHistory()->GetHistoryFile(i));
            }
        wxGetApp().GetStartPage()->SetMRUList(mruFiles);
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
            if (dsName.empty())
                {
                continue;
                }
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
    Wisteria::UI::InsertPageDlg dlg(nullptr, m_frame);
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

    Wisteria::UI::InsertPageDlg dlg(canvas, m_frame, wxID_ANY, _(L"Edit Page"));
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
        m_pagesButtonBar->EnableButton(ID_EDIT_ITEM, enabled);
        m_pagesButtonBar->EnableButton(ID_DELETE_ITEM, enabled);
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

    appendItem(m_saveMenu, ID_SAVE_PROJECT, _(L"Save"), L"file-save.svg");
    appendItem(m_saveMenu, ID_SAVE_PROJECT_AS, _(L"Save As..."), L"file-save.svg");

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
    appendItem(m_surveyGraphMenu, ID_NEW_WORD_CLOUD, _(L"Word Cloud..."), L"wordcloud.svg");
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
    auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();

    // The legend occupies its own cell in the canvas grid, adjacent to the graph.
    // If the grid doesn't have room for the legend in the requested direction,
    // we expand it by adding a column (for left/right) or row (for top/bottom).
    //
    // When the legend goes to the left and the graph is in column 0, there is
    // no column to the left, so we add one and shift the graph to column 1.
    // Similarly, when the legend goes on top and the graph is in row 0,
    // we add a row and shift the graph down to row 1.
    if (legendPlacement == Wisteria::UI::LegendPlacement::Right)
        {
        if (graphCol + 1 >= gridCols)
            {
            canvas->SetFixedObjectsGridSize(gridRows, gridCols + 1);
            }
        }
    else if (legendPlacement == Wisteria::UI::LegendPlacement::Left)
        {
        if (graphCol == 0)
            {
            canvas->SetFixedObjectsGridSize(gridRows, gridCols + 1);
            }
        }
    else if (legendPlacement == Wisteria::UI::LegendPlacement::Bottom)
        {
        if (graphRow + 1 >= gridRows)
            {
            canvas->SetFixedObjectsGridSize(gridRows + 1, gridCols);
            }
        }
    else if (legendPlacement == Wisteria::UI::LegendPlacement::Top)
        {
        if (graphRow == 0)
            {
            canvas->SetFixedObjectsGridSize(gridRows + 1, gridCols);
            }
        }

    const auto plotRow = (legendPlacement == Wisteria::UI::LegendPlacement::Top && graphRow == 0) ?
                             graphRow + 1 :
                             graphRow;
    const auto plotCol = (legendPlacement == Wisteria::UI::LegendPlacement::Left && graphCol == 0) ?
                             graphCol + 1 :
                             graphCol;
    canvas->SetFixedObject(plotRow, plotCol, plot);

    if (legend != nullptr)
        {
        auto* legendLabel = dynamic_cast<Wisteria::GraphItems::Label*>(legend.get());
        if (legendLabel != nullptr)
            {
            legendLabel->SetIsLegend(true);
            }

        if (legendPlacement == Wisteria::UI::LegendPlacement::Right)
            {
            canvas->SetFixedObject(plotRow, plotCol + 1, std::move(legend));
            }
        else if (legendPlacement == Wisteria::UI::LegendPlacement::Left)
            {
            canvas->SetFixedObject(plotRow, plotCol - 1, std::move(legend));
            }
        else if (legendPlacement == Wisteria::UI::LegendPlacement::Bottom)
            {
            legend->SetPageHorizontalAlignment(Wisteria::PageHorizontalAlignment::LeftAligned);
            canvas->SetFixedObject(plotRow + 1, plotCol, std::move(legend));
            }
        else if (legendPlacement == Wisteria::UI::LegendPlacement::Top)
            {
            legend->SetPageHorizontalAlignment(Wisteria::PageHorizontalAlignment::LeftAligned);
            canvas->SetFixedObject(plotRow - 1, plotCol, std::move(legend));
            }
        }

    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
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
    const auto chernoffSvg = wxGetApp().GetResourceManager().GetSVG(L"chernoffplot.svg");
    if (chernoffSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(chernoffSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateEnhancedLegend(Wisteria::Graphs::LegendOptions{}
                                                                    .IncludeHeader(true)
                                                                    .Placement(side)
                                                                    .PlacementHint(hint))) :
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
    const auto scatterSvg = wxGetApp().GetResourceManager().GetSVG(L"scatterplot.svg");
    if (scatterSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(scatterSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnCanvasDClick(wxCommandEvent& event) { OnEditItem(event); }

//-------------------------------------------
void WisteriaView::OnEditItem([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    // find the selected item in the canvas grid
    const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
    std::shared_ptr<Wisteria::GraphItems::GraphItemBase> selectedItem;
    size_t itemRow{ 0 };
    size_t itemCol{ 0 };
    size_t selectedCount{ 0 };
    for (size_t row = 0; row < gridRows; ++row)
        {
        for (size_t col = 0; col < gridCols; ++col)
            {
            auto item = canvas->GetFixedObject(row, col);
            if (item != nullptr && (item->IsSelected() || !item->GetSelectedIds().empty()))
                {
                ++selectedCount;
                selectedItem = item;
                itemRow = row;
                itemCol = col;
                }
            }
        }

    if (selectedCount > 1)
        {
        wxMessageBox(_(L"Please select only one item to edit."), _(L"Edit"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    if (selectedItem == nullptr)
        {
        return;
        }

    // labels are not Graph2D, handle them first
    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(selectedItem.get());
    if (label != nullptr && !label->IsLegend())
        {
        EditLabel(*label, canvas, itemRow, itemCol);
        return;
        }

    // images are not Graph2D either
    auto* image = dynamic_cast<Wisteria::GraphItems::Image*>(selectedItem.get());
    if (image != nullptr)
        {
        EditImage(*image, canvas, itemRow, itemCol);
        return;
        }

    // shapes are not Graph2D (check FillableShape before Shape since it derives from Shape)
    if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::FillableShape)))
        {
        auto* fillableShape =
            dynamic_cast<Wisteria::GraphItems::FillableShape*>(selectedItem.get());
        if (fillableShape != nullptr)
            {
            EditFillableShape(*fillableShape, canvas, itemRow, itemCol);
            }
        return;
        }
    if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Shape)))
        {
        auto* shape = dynamic_cast<Wisteria::GraphItems::Shape*>(selectedItem.get());
        if (shape != nullptr)
            {
            EditShape(*shape, canvas, itemRow, itemCol);
            }
        return;
        }

    if (!selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Graph2D)))
        {
        return;
        }
    auto* graph = dynamic_cast<Wisteria::Graphs::Graph2D*>(selectedItem.get());
    if (graph == nullptr)
        {
        return;
        }

    // dispatch to the appropriate edit function based on graph type
    // (check derived classes before their base classes)
    if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        EditScatterPlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        EditChernoffPlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
        {
        EditWCurvePlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        EditLinePlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        EditLRRoadmap(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        EditProConRoadmap(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
        {
        EditHistogram(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WordCloud)))
        {
        EditWordCloud(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        EditWLSparkline(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::StemAndLeafPlot)))
        {
        EditStemAndLeaf(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        EditPieChart(*graph, canvas, itemRow, itemCol);
        }
    }

//-------------------------------------------
void WisteriaView::OnDeleteItem([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    const auto isLegend = [](const auto item)
    {
        if (item == nullptr)
            {
            return false;
            }
        if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
            {
            if (auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(item.get());
                label != nullptr && label->IsLegend())
                {
                return true;
                }
            }
        if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend)))
            {
            return true;
            }
        return false;
    };

    // find the selected item in the canvas grid
    const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
    std::shared_ptr<Wisteria::GraphItems::GraphItemBase> selectedItem;
    size_t itemRow{ 0 };
    size_t itemCol{ 0 };
    size_t selectedCount{ 0 };
    for (size_t row = 0; row < gridRows; ++row)
        {
        for (size_t col = 0; col < gridCols; ++col)
            {
            auto item = canvas->GetFixedObject(row, col);
            if (item != nullptr && (item->IsSelected() || !item->GetSelectedIds().empty()))
                {
                // skip legend labels
                if (isLegend(item))
                    {
                    continue;
                    }
                ++selectedCount;
                selectedItem = item;
                itemRow = row;
                itemCol = col;
                }
            }
        }

    if (selectedCount > 1)
        {
        wxMessageBox(_(L"Please select only one item to delete."), _(L"Delete"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    if (selectedItem == nullptr)
        {
        return;
        }

    if (wxMessageBox(_(L"Are you sure you want to delete the selected item?"), _(L"Delete Item"),
                     wxYES_NO | wxICON_QUESTION, m_frame) != wxYES)
        {
        return;
        }

    // if the item is a graph with a legend, clear the legend cell too
    auto* graph = dynamic_cast<Wisteria::Graphs::Graph2D*>(selectedItem.get());
    if (graph != nullptr)
        {
        const auto& legendInfo = graph->GetLegendInfo();
        if (legendInfo.has_value())
            {
            const auto oldSide = legendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && itemRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && itemRow + 1 < gridRows) ||
                (oldSide == Wisteria::Side::Left && itemCol > 0) ||
                (oldSide == Wisteria::Side::Right && itemCol + 1 < gridCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? itemRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? itemRow + 1 :
                                                                               itemRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? itemCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? itemCol + 1 :
                                                                              itemCol;

                if (isLegend(canvas->GetFixedObject(legendRow, legendCol)))
                    {
                    canvas->SetFixedObject(legendRow, legendCol, nullptr);
                    }
                }
            }
        }

    canvas->SetFixedObject(itemRow, itemCol, nullptr);
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();
    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditScatterPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertScatterPlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Scatter Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto scatterSvg = wxGetApp().GetResourceManager().GetSVG(L"scatterplot.svg");
    if (scatterSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(scatterSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

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

        // carry forward property templates, preserving {{placeholders}}
        // unless the user changed the value
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        const auto* oldScatter = dynamic_cast<const Wisteria::Graphs::ScatterPlot*>(&graph);
        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.x", dlg.GetXVariable(),
                     oldScatter != nullptr ? oldScatter->GetXColumnName() : wxString{});
        carryForward(L"variables.y", dlg.GetYVariable(),
                     oldScatter != nullptr ? oldScatter->GetYColumnName() : wxString{});
        const auto oldGroupName =
            (oldScatter != nullptr && !oldScatter->GetSeriesList().empty()) ?
                oldScatter->GetSeriesList().front().GetGroupColumnName().value_or(wxString{}) :
                wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        // clear the old graph and its legend (if any)
        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            // ensure the legend cell is within the grid
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditChernoffPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                    const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertChernoffDlg dlg(canvas, &m_reportBuilder, m_frame,
                                        _(L"Edit Chernoff Faces Plot"), wxID_ANY, wxDefaultPosition,
                                        wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto chernoffSvg = wxGetApp().GetResourceManager().GetSVG(L"chernoffplot.svg");
    if (chernoffSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(chernoffSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

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

        // carry forward property templates, preserving {{placeholders}}
        // unless the user changed the value
        const auto* oldChernoff = dynamic_cast<const Wisteria::Graphs::ChernoffFacesPlot*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));

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
            const auto oldExpanded =
                (oldChernoff != nullptr) ? oldChernoff->GetFeatureColumnName(fid) : wxString{};
            carryForward(L"variables." + propName, var, oldExpanded);
            }

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        // clear the old graph and its legend (if any)
        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            // ensure the legend cell is within the grid
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    if (legendItem->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
                        {
                        if (auto* label =
                                dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                            label != nullptr && label->IsLegend())
                            {
                            canvas->SetFixedObject(legendRow, legendCol, nullptr);
                            }
                        }
                    if (legendItem->IsKindOf(
                            wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend)))
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 dlg.GetUseEnhancedLegend() ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateEnhancedLegend(Wisteria::Graphs::LegendOptions{}
                                                                    .IncludeHeader(true)
                                                                    .Placement(side)
                                                                    .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertLinePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto lineSvg = wxGetApp().GetResourceManager().GetSVG(L"lineplot.svg");
    if (lineSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(lineSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::LinePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->AutoSpline(dlg.GetAutoSpline());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);

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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditLinePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Line Plot"),
                                        wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto lineSvg = wxGetApp().GetResourceManager().GetSVG(L"lineplot.svg");
    if (lineSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(lineSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::LinePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->AutoSpline(dlg.GetAutoSpline());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);

        const auto* oldLine = dynamic_cast<const Wisteria::Graphs::LinePlot*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.x", dlg.GetXVariable(),
                     oldLine != nullptr ? oldLine->GetXColumnName() : wxString{});
        carryForward(L"variables.y", dlg.GetYVariable(),
                     oldLine != nullptr ? oldLine->GetYColumnName() : wxString{});
        const auto oldGroupName =
            (oldLine != nullptr) ? oldLine->GetGroupColumnName().value_or(wxString{}) : wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWCurvePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWCurveDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto wcurveSvg = wxGetApp().GetResourceManager().GetSVG(L"wcurve.svg");
    if (wcurveSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wcurveSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WCurvePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);

        plot->SetTimeIntervalLabel(dlg.GetTimeIntervalLabel());

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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWCurvePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertWCurveDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit W-Curve Plot"),
                                      wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                      Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto wcurveSvg = wxGetApp().GetResourceManager().GetSVG(L"wcurve.svg");
    if (wcurveSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wcurveSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WCurvePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);

        plot->SetTimeIntervalLabel(dlg.GetTimeIntervalLabel());

        const auto* oldWCurve = dynamic_cast<const Wisteria::Graphs::WCurvePlot*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.x", dlg.GetXVariable(),
                     oldWCurve != nullptr ? oldWCurve->GetXColumnName() : wxString{});
        carryForward(L"variables.y", dlg.GetYVariable(),
                     oldWCurve != nullptr ? oldWCurve->GetYColumnName() : wxString{});
        const auto oldGroupName = (oldWCurve != nullptr) ?
                                      oldWCurve->GetGroupColumnName().value_or(wxString{}) :
                                      wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertLRRoadmap([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLRRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto roadmapSvg = wxGetApp().GetResourceManager().GetSVG(L"roadmap.svg");
    if (roadmapSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(roadmapSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::LRRoadmap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> pValueCol =
            dlg.GetPValueVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetPValueVariable());
        const std::optional<wxString> dvName =
            dlg.GetDVName().empty() ? std::nullopt : std::optional<wxString>(dlg.GetDVName());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetPredictorVariable(),
                      dlg.GetCoefficientVariable(), pValueCol, dlg.GetPLevel(),
                      dlg.GetPredictorsToInclude(), dvName);

        if (dlg.GetAddDefaultCaption())
            {
            plot->AddDefaultCaption();
            }

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.predictor", dlg.GetPredictorVariable());
        plot->SetPropertyTemplate(L"variables.coefficient", dlg.GetCoefficientVariable());
        if (!dlg.GetPValueVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.pvalue", dlg.GetPValueVariable());
            }

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditLRRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertLRRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame,
                                         _(L"Edit Linear Regression Roadmap"), wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto roadmapSvg = wxGetApp().GetResourceManager().GetSVG(L"roadmap.svg");
    if (roadmapSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(roadmapSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::LRRoadmap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> pValueCol =
            dlg.GetPValueVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetPValueVariable());
        const std::optional<wxString> dvName =
            dlg.GetDVName().empty() ? std::nullopt : std::optional<wxString>(dlg.GetDVName());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetPredictorVariable(),
                      dlg.GetCoefficientVariable(), pValueCol, dlg.GetPLevel(),
                      dlg.GetPredictorsToInclude(), dvName);

        if (dlg.GetAddDefaultCaption())
            {
            plot->AddDefaultCaption();
            }

        const auto* oldRoadmap = dynamic_cast<const Wisteria::Graphs::LRRoadmap*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.predictor", dlg.GetPredictorVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetPredictorColumnName() : wxString{});
        carryForward(L"variables.coefficient", dlg.GetCoefficientVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetCoefficientColumnName() : wxString{});
        carryForward(L"variables.pvalue", dlg.GetPValueVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetPValueColumnName() : wxString{});

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertProConRoadmap([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertProConRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto roadmapSvg = wxGetApp().GetResourceManager().GetSVG(L"roadmap.svg");
    if (roadmapSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(roadmapSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::ProConRoadmap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> posValueCol =
            dlg.GetPositiveValueVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetPositiveValueVariable());
        const std::optional<wxString> negValueCol =
            dlg.GetNegativeValueVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetNegativeValueVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetPositiveVariable(), posValueCol,
                      dlg.GetNegativeVariable(), negValueCol, dlg.GetMinimumCount());

        plot->SetPositiveLegendLabel(dlg.GetPositiveLabel());
        plot->SetNegativeLegendLabel(dlg.GetNegativeLabel());

        if (dlg.GetAddDefaultCaption())
            {
            plot->AddDefaultCaption();
            }

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.positive", dlg.GetPositiveVariable());
        if (!dlg.GetPositiveValueVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.positive-value", dlg.GetPositiveValueVariable());
            }
        plot->SetPropertyTemplate(L"variables.negative", dlg.GetNegativeVariable());
        if (!dlg.GetNegativeValueVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.negative-value", dlg.GetNegativeValueVariable());
            }

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditProConRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                     const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertProConRoadmapDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Pro && Con Roadmap"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto roadmapSvg = wxGetApp().GetResourceManager().GetSVG(L"roadmap.svg");
    if (roadmapSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(roadmapSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::ProConRoadmap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> posValueCol =
            dlg.GetPositiveValueVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetPositiveValueVariable());
        const std::optional<wxString> negValueCol =
            dlg.GetNegativeValueVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetNegativeValueVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetPositiveVariable(), posValueCol,
                      dlg.GetNegativeVariable(), negValueCol, dlg.GetMinimumCount());

        plot->SetPositiveLegendLabel(dlg.GetPositiveLabel());
        plot->SetNegativeLegendLabel(dlg.GetNegativeLabel());

        if (dlg.GetAddDefaultCaption())
            {
            plot->AddDefaultCaption();
            }

        const auto* oldRoadmap = dynamic_cast<const Wisteria::Graphs::ProConRoadmap*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.positive", dlg.GetPositiveVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetPositiveColumnName() : wxString{});
        carryForward(L"variables.positive-value", dlg.GetPositiveValueVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetPositiveValueColumnName() : wxString{});
        carryForward(L"variables.negative", dlg.GetNegativeVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetNegativeColumnName() : wxString{});
        carryForward(L"variables.negative-value", dlg.GetNegativeValueVariable(),
                     oldRoadmap != nullptr ? oldRoadmap->GetNegativeValueColumnName() : wxString{});

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertHistogram([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertHistogramDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto histSvg = wxGetApp().GetResourceManager().GetSVG(L"histogram.svg");
    if (histSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(histSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::Histogram>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(
            dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol,
            static_cast<Wisteria::Graphs::Histogram::BinningMethod>(dlg.GetBinningMethod()),
            static_cast<Wisteria::RoundingMethod>(dlg.GetRoundingMethod()),
            static_cast<Wisteria::Graphs::Histogram::IntervalDisplay>(dlg.GetIntervalDisplay()),
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetBinLabelDisplay()),
            dlg.GetShowFullRange(), std::nullopt, std::make_pair(std::nullopt, std::nullopt),
            dlg.GetNeatIntervals());

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.continuous", dlg.GetContinuousVariable());
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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditHistogram(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertHistogramDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Histogram"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto histSvg = wxGetApp().GetResourceManager().GetSVG(L"histogram.svg");
    if (histSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(histSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::Histogram>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(
            dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol,
            static_cast<Wisteria::Graphs::Histogram::BinningMethod>(dlg.GetBinningMethod()),
            static_cast<Wisteria::RoundingMethod>(dlg.GetRoundingMethod()),
            static_cast<Wisteria::Graphs::Histogram::IntervalDisplay>(dlg.GetIntervalDisplay()),
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetBinLabelDisplay()),
            dlg.GetShowFullRange(), std::nullopt, std::make_pair(std::nullopt, std::nullopt),
            dlg.GetNeatIntervals());

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldHistogram = dynamic_cast<const Wisteria::Graphs::Histogram*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.continuous", dlg.GetContinuousVariable(),
                     oldHistogram != nullptr ? oldHistogram->GetContinuousColumnName() :
                                               wxString{});
        const auto oldGroupName = (oldHistogram != nullptr) ?
                                      oldHistogram->GetGroupColumnName().value_or(wxString{}) :
                                      wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWordCloud([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWordCloudDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto wcSvg = wxGetApp().GetResourceManager().GetSVG(L"wordcloud.svg");
    if (wcSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wcSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WordCloud>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetWordVariable(), weightCol,
                      dlg.GetMinFrequency(), dlg.GetMaxFrequency(), dlg.GetMaxWords());

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.word", dlg.GetWordVariable());
        if (!dlg.GetWeightVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.weight", dlg.GetWeightVariable());
            }

        // word clouds do not support legends
        PlaceGraphWithLegend(canvas, plot, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                             Wisteria::UI::LegendPlacement::None);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWordCloud(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertWordCloudDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Word Cloud"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto wcSvg = wxGetApp().GetResourceManager().GetSVG(L"wordcloud.svg");
    if (wcSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wcSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WordCloud>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetWordVariable(), weightCol,
                      dlg.GetMinFrequency(), dlg.GetMaxFrequency(), dlg.GetMaxWords());

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldWC = dynamic_cast<const Wisteria::Graphs::WordCloud*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.word", dlg.GetWordVariable(),
                     oldWC != nullptr ? oldWC->GetWordColumnName() : wxString{});
        carryForward(L"variables.weight", dlg.GetWeightVariable(),
                     oldWC != nullptr ? oldWC->GetWeightColumnName() : wxString{});

        // word clouds do not support legends; clear old graph directly
        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        PlaceGraphWithLegend(canvas, plot, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                             Wisteria::UI::LegendPlacement::None);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWLSparkline([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWLSparklineDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto wlSvg = wxGetApp().GetResourceManager().GetSVG(L"sparkline.svg");
    if (wlSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wlSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WinLossSparkline>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->HighlightBestRecords(dlg.GetHighlightBestRecords());

        const std::optional<wxString> postseasonCol =
            dlg.GetPostseasonVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetPostseasonVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetSeasonVariable(), dlg.GetWonVariable(),
                      dlg.GetShutoutVariable(), dlg.GetHomeGameVariable(), postseasonCol);

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.season", dlg.GetSeasonVariable());
        plot->SetPropertyTemplate(L"variables.won", dlg.GetWonVariable());
        plot->SetPropertyTemplate(L"variables.shutout", dlg.GetShutoutVariable());
        plot->SetPropertyTemplate(L"variables.home-game", dlg.GetHomeGameVariable());
        if (!dlg.GetPostseasonVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.postseason", dlg.GetPostseasonVariable());
            }

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWLSparkline(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertWLSparklineDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Win/Loss Sparkline"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto wlSvg = wxGetApp().GetResourceManager().GetSVG(L"winlosssparkline.svg");
    if (wlSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wlSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WinLossSparkline>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->HighlightBestRecords(dlg.GetHighlightBestRecords());

        const std::optional<wxString> postseasonCol =
            dlg.GetPostseasonVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetPostseasonVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetSeasonVariable(), dlg.GetWonVariable(),
                      dlg.GetShutoutVariable(), dlg.GetHomeGameVariable(), postseasonCol);

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldWL = dynamic_cast<const Wisteria::Graphs::WinLossSparkline*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.season", dlg.GetSeasonVariable(),
                     oldWL != nullptr ? oldWL->GetSeasonColumnName() : wxString{});
        carryForward(L"variables.won", dlg.GetWonVariable(),
                     oldWL != nullptr ? oldWL->GetWonColumnName() : wxString{});
        carryForward(L"variables.shutout", dlg.GetShutoutVariable(),
                     oldWL != nullptr ? oldWL->GetShutoutColumnName() : wxString{});
        carryForward(L"variables.home-game", dlg.GetHomeGameVariable(),
                     oldWL != nullptr ? oldWL->GetHomeGameColumnName() : wxString{});
        carryForward(L"variables.postseason", dlg.GetPostseasonVariable(),
                     oldWL != nullptr ? oldWL->GetPostseasonColumnName() : wxString{});

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertStemAndLeaf([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertStemAndLeafDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto slSvg = wxGetApp().GetResourceManager().GetSVG(L"stem-leaf.svg");
    if (slSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(slSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::StemAndLeafPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol);

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.continuous", dlg.GetContinuousVariable());
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
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditStemAndLeaf(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertStemAndLeafDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Stem-and-Leaf Plot"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto slSvg = wxGetApp().GetResourceManager().GetSVG(L"stem-leaf.svg");
    if (slSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(slSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::StemAndLeafPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol);

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldSL = dynamic_cast<const Wisteria::Graphs::StemAndLeafPlot*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.continuous", dlg.GetContinuousVariable(),
                     oldSL != nullptr ? oldSL->GetContinuousColumnName() : wxString{});
        const auto oldGroupName =
            (oldSL != nullptr) ? oldSL->GetGroupColumnName().value_or(wxString{}) : wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertPieChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertPieChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto pieSvg = wxGetApp().GetResourceManager().GetSVG(L"piechart.svg");
    if (pieSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(pieSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::PieChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        const std::optional<wxString> group2Col =
            dlg.GetGroup2Variable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetGroup2Variable());
        plot->SetData(dlg.GetSelectedDataset(), weightCol, dlg.GetGroupVariable(), group2Col);

        // apply styling options
        plot->IncludeDonutHole(dlg.GetIncludeDonutHole());
        if (dlg.GetIncludeDonutHole())
            {
            plot->GetDonutHoleLabel() = dlg.GetDonutHoleLabel();
            }
        plot->UseColorLabels(dlg.GetUseColorLabels());
        plot->SetOuterPieMidPointLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetOuterMidPointLabelDisplay()));
        plot->SetOuterLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetOuterLabelDisplay()));
        plot->SetInnerPieMidPointLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetInnerMidPointLabelDisplay()));
        plot->SetLabelPlacement(static_cast<Wisteria::LabelPlacement>(dlg.GetLabelPlacement()));
        plot->SetPieStyle(static_cast<Wisteria::PieStyle>(dlg.GetPieStyle()));
        plot->ShowOuterPieLabels(dlg.GetShowOuterPieLabels());
        plot->ShowInnerPieLabels(dlg.GetShowInnerPieLabels());

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.group-1", dlg.GetGroupVariable());
        if (!dlg.GetWeightVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.aggregate", dlg.GetWeightVariable());
            }
        if (!dlg.GetGroup2Variable().empty())
            {
            plot->SetPropertyTemplate(L"variables.group-2", dlg.GetGroup2Variable());
            }

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditPieChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertPieChartDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Pie Chart"),
                                        wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto pieSvg = wxGetApp().GetResourceManager().GetSVG(L"piechart.svg");
    if (pieSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(pieSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::PieChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        const std::optional<wxString> group2Col =
            dlg.GetGroup2Variable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetGroup2Variable());
        plot->SetData(dlg.GetSelectedDataset(), weightCol, dlg.GetGroupVariable(), group2Col);

        // apply styling options
        plot->IncludeDonutHole(dlg.GetIncludeDonutHole());
        if (dlg.GetIncludeDonutHole())
            {
            plot->GetDonutHoleLabel() = dlg.GetDonutHoleLabel();
            }
        plot->UseColorLabels(dlg.GetUseColorLabels());
        plot->SetOuterPieMidPointLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetOuterMidPointLabelDisplay()));
        plot->SetOuterLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetOuterLabelDisplay()));
        plot->SetInnerPieMidPointLabelDisplay(
            static_cast<Wisteria::BinLabelDisplay>(dlg.GetInnerMidPointLabelDisplay()));
        plot->SetLabelPlacement(static_cast<Wisteria::LabelPlacement>(dlg.GetLabelPlacement()));
        plot->SetPieStyle(static_cast<Wisteria::PieStyle>(dlg.GetPieStyle()));
        plot->ShowOuterPieLabels(dlg.GetShowOuterPieLabels());
        plot->ShowInnerPieLabels(dlg.GetShowInnerPieLabels());

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldPie = dynamic_cast<const Wisteria::Graphs::PieChart*>(&graph);
        const auto carryForward = [&graph, &plot](const wxString& prop, const wxString& newVal,
                                                  const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                plot->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                plot->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));
        carryForward(L"variables.group-1", dlg.GetGroupVariable(),
                     oldPie != nullptr ? oldPie->GetGroupColumn1Name() : wxString{});
        carryForward(L"variables.aggregate", dlg.GetWeightVariable(),
                     oldPie != nullptr ? oldPie->GetWeightColumnName() : wxString{});
        carryForward(L"variables.group-2", dlg.GetGroup2Variable(),
                     oldPie != nullptr ? oldPie->GetGroupColumn2Name() : wxString{});

        const auto legendPlacement = dlg.GetLegendPlacement();
        const auto hint = (legendPlacement == Wisteria::UI::LegendPlacement::Right) ?
                              Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                          (legendPlacement == Wisteria::UI::LegendPlacement::Left) ?
                              Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                              Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
        const auto side =
            (legendPlacement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
            (legendPlacement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
            (legendPlacement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                        Wisteria::Side::Bottom;

        canvas->SetFixedObject(graphRow, graphCol, nullptr);
        const auto& oldLegendInfo = graph.GetLegendInfo();
        if (oldLegendInfo.has_value())
            {
            const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
            const auto oldSide = oldLegendInfo->GetPlacement();
            const bool hasLegendCell =
                (oldSide == Wisteria::Side::Top && graphRow > 0) ||
                (oldSide == Wisteria::Side::Bottom && graphRow + 1 < gRows) ||
                (oldSide == Wisteria::Side::Left && graphCol > 0) ||
                (oldSide == Wisteria::Side::Right && graphCol + 1 < gCols);
            if (hasLegendCell)
                {
                const size_t legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                         (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                               graphRow;
                const size_t legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                         (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                              graphCol;
                auto legendItem = canvas->GetFixedObject(legendRow, legendCol);
                if (legendItem != nullptr)
                    {
                    auto* label = dynamic_cast<Wisteria::GraphItems::Label*>(legendItem.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        canvas->SetFixedObject(legendRow, legendCol, nullptr);
                        }
                    }
                }
            }

        PlaceGraphWithLegend(canvas, plot,
                             (legendPlacement != Wisteria::UI::LegendPlacement::None) ?
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                     plot->CreateLegend(Wisteria::Graphs::LegendOptions{}
                                                            .IncludeHeader(true)
                                                            .Placement(side)
                                                            .PlacementHint(hint))) :
                                 std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertLabel([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLabelDlg dlg(canvas, nullptr, m_frame);
    const auto labelBmp = wxGetApp().ReadSvgIcon(L"label.svg");
    if (labelBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(labelBmp);
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    auto label = std::make_shared<Wisteria::GraphItems::Label>(
        Wisteria::GraphItems::GraphItemInfo(dlg.GetLabelText()));
    dlg.ApplyPageOptions(*label);
    dlg.ApplyToLabel(*label);

    const auto rawText = dlg.GetLabelText();
    const auto expanded = m_reportBuilder.ExpandConstants(rawText);
    if (expanded != rawText)
        {
        label->SetPropertyTemplate(L"text", rawText);
        }
    label->SetText(expanded);
    label->SetDPIScaleFactor(canvas->FromDIP(1));

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), label);
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditLabel(Wisteria::GraphItems::Label& label, Wisteria::Canvas* canvas,
                             const size_t labelRow, const size_t labelCol)
    {
    Wisteria::UI::InsertLabelDlg dlg(canvas, nullptr, m_frame, _(L"Edit Label"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto labelBmp = wxGetApp().ReadSvgIcon(L"label.svg");
    if (labelBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(labelBmp);
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(labelRow, labelCol);
    dlg.LoadFromLabel(label, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    auto newLabel = std::make_shared<Wisteria::GraphItems::Label>(
        Wisteria::GraphItems::GraphItemInfo(dlg.GetLabelText()));
    dlg.ApplyPageOptions(*newLabel);
    dlg.ApplyToLabel(*newLabel);

    const auto rawText = dlg.GetLabelText();
    const auto expanded = m_reportBuilder.ExpandConstants(rawText);
    if (expanded != rawText)
        {
        newLabel->SetPropertyTemplate(L"text", rawText);
        }
    newLabel->SetText(expanded);
    newLabel->SetDPIScaleFactor(canvas->FromDIP(1));

    canvas->SetFixedObject(labelRow, labelCol, newLabel);
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertImage([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertImageDlg dlg(canvas, nullptr, m_frame);
    const auto imgBmp = wxGetApp().ReadSvgIcon(L"image.svg");
    if (imgBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(imgBmp);
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    const auto paths = dlg.GetImagePaths();
    if (paths.empty())
        {
        return;
        }

    // resolve relative paths against the project directory
    const wxString projectDir =
        m_projectFilePath.empty() ? wxString{} : wxFileName{ m_projectFilePath }.GetPathWithSep();

    // load and optionally stitch multiple images
    std::vector<wxBitmap> bmps;
    for (const auto& path : paths)
        {
        wxString resolvedPath = path;
        if (!wxFileName(path).IsAbsolute() && !projectDir.empty())
            {
            resolvedPath = projectDir + path;
            }
        auto loadedBmp = Wisteria::GraphItems::Image::LoadFile(resolvedPath);
        if (loadedBmp.IsOk())
            {
            bmps.push_back(loadedBmp);
            }
        }
    if (bmps.empty())
        {
        return;
        }

    wxImage resultImg;
    if (bmps.size() == 1)
        {
        resultImg = bmps[0].ConvertToImage();
        }
    else if (dlg.GetStitchDirection() == L"vertical")
        {
        resultImg = Wisteria::GraphItems::Image::StitchVertically(bmps);
        }
    else
        {
        resultImg = Wisteria::GraphItems::Image::StitchHorizontally(bmps);
        }

    const auto effect = dlg.GetImageEffect();
    if (effect != Wisteria::ImageEffect::NoEffect)
        {
        resultImg = Wisteria::GraphItems::Image::ApplyEffect(effect, resultImg);
        }

    auto image = std::make_shared<Wisteria::GraphItems::Image>(resultImg);
    dlg.ApplyPageOptions(*image);
    dlg.ApplyToImage(*image);

    // helper to make absolute paths relative to the project file
    const auto makeRelative = [this](const wxString& filePath) -> wxString
    {
        if (!m_projectFilePath.empty())
            {
            wxFileName fn(filePath);
            if (fn.IsAbsolute())
                {
                fn.MakeRelativeTo(wxFileName(m_projectFilePath).GetPath());
                return fn.GetFullPath(wxPATH_UNIX);
                }
            }
        return filePath;
    };

    // cache import paths for round-tripping
    if (paths.GetCount() == 1)
        {
        image->SetPropertyTemplate(L"image-import.path", makeRelative(paths[0]));
        }
    else
        {
        wxString joined;
        for (size_t idx = 0; idx < paths.GetCount(); ++idx)
            {
            if (idx > 0)
                {
                joined += L"\t";
                }
            joined += makeRelative(paths[idx]);
            }
        image->SetPropertyTemplate(L"image-import.paths", joined);
        image->SetPropertyTemplate(L"image-import.stitch", dlg.GetStitchDirection());
        }

    // cache effect for round-tripping
    if (effect != Wisteria::ImageEffect::NoEffect)
        {
        const auto effectStr = Wisteria::ReportEnumConvert::ConvertImageEffectToString(effect);
        if (effectStr.has_value())
            {
            image->SetPropertyTemplate(L"image-import.effect", effectStr.value());
            }
        }

    // apply custom size
    if (dlg.IsCustomSizeEnabled())
        {
        const auto reqWidth = dlg.GetImageWidth();
        const auto reqHeight = dlg.GetImageHeight();
        const auto bestSz = Wisteria::GraphItems::Image::ToBestSize(resultImg.GetSize(),
                                                                    wxSize{ reqWidth, reqHeight });
        image->SetSize(bestSz);
        image->SetPropertyTemplate(L"size.width", std::to_wstring(reqWidth));
        image->SetPropertyTemplate(L"size.height", std::to_wstring(reqHeight));
        }

    image->SetDPIScaleFactor(canvas->FromDIP(1));

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), image);
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditImage(Wisteria::GraphItems::Image& image, Wisteria::Canvas* canvas,
                             const size_t imageRow, const size_t imageCol)
    {
    Wisteria::UI::InsertImageDlg dlg(canvas, nullptr, m_frame, _(L"Edit Image"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto imgBmp = wxGetApp().ReadSvgIcon(L"image.svg");
    if (imgBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(imgBmp);
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(imageRow, imageCol);
    dlg.LoadFromImage(image, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto paths = dlg.GetImagePaths();
    if (paths.empty())
        {
        return;
        }

    // resolve relative paths against the project directory
    const wxString projectDir2 =
        m_projectFilePath.empty() ? wxString{} : wxFileName(m_projectFilePath).GetPathWithSep();

    // load and optionally stitch multiple images
    std::vector<wxBitmap> bmps;
    for (const auto& path : paths)
        {
        wxString resolvedPath = path;
        if (!wxFileName(path).IsAbsolute() && !projectDir2.empty())
            {
            resolvedPath = projectDir2 + path;
            }
        auto loadedBmp = Wisteria::GraphItems::Image::LoadFile(resolvedPath);
        if (loadedBmp.IsOk())
            {
            bmps.push_back(loadedBmp);
            }
        }
    if (bmps.empty())
        {
        return;
        }

    wxImage resultImg;
    if (bmps.size() == 1)
        {
        resultImg = bmps[0].ConvertToImage();
        }
    else if (dlg.GetStitchDirection() == L"vertical")
        {
        resultImg = Wisteria::GraphItems::Image::StitchVertically(bmps);
        }
    else
        {
        resultImg = Wisteria::GraphItems::Image::StitchHorizontally(bmps);
        }

    const auto effect = dlg.GetImageEffect();
    if (effect != Wisteria::ImageEffect::NoEffect)
        {
        resultImg = Wisteria::GraphItems::Image::ApplyEffect(effect, resultImg);
        }

    auto newImage = std::make_shared<Wisteria::GraphItems::Image>(resultImg);
    dlg.ApplyPageOptions(*newImage);
    dlg.ApplyToImage(*newImage);

    // helper to make absolute paths relative to the project file
    const auto makeRelative2 = [this](const wxString& filePath) -> wxString
    {
        if (!m_projectFilePath.empty())
            {
            wxFileName fn(filePath);
            if (fn.IsAbsolute())
                {
                fn.MakeRelativeTo(wxFileName(m_projectFilePath).GetPath());
                return fn.GetFullPath(wxPATH_UNIX);
                }
            }
        return filePath;
    };

    // cache import paths for round-tripping
    if (paths.GetCount() == 1)
        {
        newImage->SetPropertyTemplate(L"image-import.path", makeRelative2(paths[0]));
        }
    else
        {
        wxString joined;
        for (size_t idx = 0; idx < paths.GetCount(); ++idx)
            {
            if (idx > 0)
                {
                joined += L"\t";
                }
            joined += makeRelative2(paths[idx]);
            }
        newImage->SetPropertyTemplate(L"image-import.paths", joined);
        newImage->SetPropertyTemplate(L"image-import.stitch", dlg.GetStitchDirection());
        }

    // cache effect for round-tripping
    if (effect != Wisteria::ImageEffect::NoEffect)
        {
        const auto effectStr = Wisteria::ReportEnumConvert::ConvertImageEffectToString(effect);
        if (effectStr.has_value())
            {
            newImage->SetPropertyTemplate(L"image-import.effect", effectStr.value());
            }
        }

    // apply custom size
    if (dlg.IsCustomSizeEnabled())
        {
        const auto reqWidth = dlg.GetImageWidth();
        const auto reqHeight = dlg.GetImageHeight();
        const auto bestSz = Wisteria::GraphItems::Image::ToBestSize(resultImg.GetSize(),
                                                                    wxSize{ reqWidth, reqHeight });
        newImage->SetSize(bestSz);
        newImage->SetPropertyTemplate(L"size.width", std::to_wstring(reqWidth));
        newImage->SetPropertyTemplate(L"size.height", std::to_wstring(reqHeight));
        }

    newImage->SetDPIScaleFactor(canvas->FromDIP(1));

    canvas->SetFixedObject(imageRow, imageCol, newImage);
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertShape([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertShapeDlg dlg(canvas, nullptr, m_frame);
    const auto shapeBmp = wxGetApp().ReadSvgIcon(L"shape.svg");
    if (shapeBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(shapeBmp);
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    const auto labelText = dlg.GetLabelText();
    const auto expanded = m_reportBuilder.ExpandConstants(labelText);

    const wxPen shapePen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() };

    // cache the user-specified size for round-tripping
    const auto shapeWidth = std::to_wstring(dlg.GetShapeWidth());
    const auto shapeHeight = std::to_wstring(dlg.GetShapeHeight());

    if (dlg.IsFillable())
        {
        auto shape = std::make_shared<Wisteria::GraphItems::FillableShape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() },
            dlg.GetFillPercent());
        shape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        shape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        shape->SetFixedWidthOnCanvas(true);
        shape->SetPropertyTemplate(L"size.width", shapeWidth);
        shape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            shape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), shape);
        }
    else
        {
        auto shape = std::make_shared<Wisteria::GraphItems::Shape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() });
        shape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        shape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        shape->SetFixedWidthOnCanvas(true);
        shape->SetPropertyTemplate(L"size.width", shapeWidth);
        shape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            shape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), shape);
        }

    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditShape(Wisteria::GraphItems::Shape& shape, Wisteria::Canvas* canvas,
                             const size_t shapeRow, const size_t shapeCol)
    {
    Wisteria::UI::InsertShapeDlg dlg(canvas, nullptr, m_frame, _(L"Edit Shape"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto shapeBmp = wxGetApp().ReadSvgIcon(L"shape.svg");
    if (shapeBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(shapeBmp);
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(shapeRow, shapeCol);
    dlg.LoadFromShape(shape, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto labelText = dlg.GetLabelText();
    const auto expanded = m_reportBuilder.ExpandConstants(labelText);
    const wxPen shapePen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() };
    const auto shapeWidth = std::to_wstring(dlg.GetShapeWidth());
    const auto shapeHeight = std::to_wstring(dlg.GetShapeHeight());

    if (dlg.IsFillable())
        {
        auto newShape = std::make_shared<Wisteria::GraphItems::FillableShape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() },
            dlg.GetFillPercent());
        newShape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        newShape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        newShape->SetFixedWidthOnCanvas(true);
        newShape->SetPropertyTemplate(L"size.width", shapeWidth);
        newShape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            newShape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(shapeRow, shapeCol, newShape);
        }
    else
        {
        auto newShape = std::make_shared<Wisteria::GraphItems::Shape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() });
        newShape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        newShape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        newShape->SetFixedWidthOnCanvas(true);
        newShape->SetPropertyTemplate(L"size.width", shapeWidth);
        newShape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            newShape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(shapeRow, shapeCol, newShape);
        }

    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditFillableShape(Wisteria::GraphItems::FillableShape& shape,
                                     Wisteria::Canvas* canvas, const size_t shapeRow,
                                     const size_t shapeCol)
    {
    Wisteria::UI::InsertShapeDlg dlg(canvas, nullptr, m_frame, _(L"Edit Fillable Shape"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto shapeBmp = wxGetApp().ReadSvgIcon(L"shape.svg");
    if (shapeBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(shapeBmp);
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(shapeRow, shapeCol);
    dlg.LoadFromFillableShape(shape, canvas);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto labelText = dlg.GetLabelText();
    const auto expanded = m_reportBuilder.ExpandConstants(labelText);
    const wxPen shapePen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() };
    const auto shapeWidth = std::to_wstring(dlg.GetShapeWidth());
    const auto shapeHeight = std::to_wstring(dlg.GetShapeHeight());

    if (dlg.IsFillable())
        {
        auto newShape = std::make_shared<Wisteria::GraphItems::FillableShape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() },
            dlg.GetFillPercent());
        newShape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        newShape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        newShape->SetFixedWidthOnCanvas(true);
        newShape->SetPropertyTemplate(L"size.width", shapeWidth);
        newShape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            newShape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(shapeRow, shapeCol, newShape);
        }
    else
        {
        auto newShape = std::make_shared<Wisteria::GraphItems::Shape>(
            Wisteria::GraphItems::GraphItemInfo{ expanded }
                .Anchoring(Wisteria::Anchoring::TopLeftCorner)
                .Pen(shapePen)
                .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
                .FontColor(dlg.GetLabelFontColor())
                .DPIScaling(canvas->FromDIP(1)),
            dlg.GetIconShape(), wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() });
        newShape->SetPageHorizontalAlignment(dlg.GetHorizontalAlignment());
        newShape->SetPageVerticalAlignment(dlg.GetVerticalAlignment());
        newShape->SetFixedWidthOnCanvas(true);
        newShape->SetPropertyTemplate(L"size.width", shapeWidth);
        newShape->SetPropertyTemplate(L"size.height", shapeHeight);
        if (expanded != labelText)
            {
            newShape->SetPropertyTemplate(L"label.text", labelText);
            }
        canvas->SetFixedObject(shapeRow, shapeCol, newShape);
        }

    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnSaveProject([[maybe_unused]] wxCommandEvent& event)
    {
    wxString filePath = m_projectFilePath;

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
        m_projectFilePath = filePath;
        const wxString savedTitle = wxFileName{ filePath }.GetName();
        GetDocument()->SetFilename(filePath, true);
        GetDocument()->SetTitle(savedTitle);
        GetDocument()->Modify(false);
        m_frame->SetTitle(savedTitle);
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

    // columns-order (preserves original spreadsheet column ordering)
    wxArrayString colOrder;
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded)
            {
            colOrder.Add(col.m_name);
            }
        }
    if (!colOrder.empty())
        {
        dsNode->Add(L"columns-order", colOrder);
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
        if (sOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            subName.CmpNoCase(sourceName) != 0)
            {
            auto subObj = wxSimpleJSON::Create(wxString::Format(
                L"{\"name\": \"%s\", "
                L"\"section\": {}, \"filter\": {}, \"filter-and\": [], \"filter-or\": [], "
                L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                L"\"recode-re\": [], \"columns-rename\": [], "
                L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                L"\"collapse-except\": [], \"formulas\": []}",
                EscapeJsonStr(subName)));
            SaveSubsetFilters(subObj, sOpts);

            SaveSubsets(subObj, subName);
            SavePivots(subObj, subName);
            SaveMerges(subObj, subName);

            const auto txIt = transformOpts.find(subName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(subObj, txIt->second);
                SaveFormulas(subObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"pivots", L"merges", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
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
        if (pOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            pivName.CmpNoCase(sourceName) != 0)
            {
            auto pivObj = wxSimpleJSON::Create(
                pivName.empty() ?
                    wxString(L"{\"subsets\": [], \"pivots\": [], \"merges\": [], "
                             L"\"recode-re\": [], \"columns-rename\": [], "
                             L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                             L"\"collapse-except\": [], \"formulas\": []}") :
                    wxString::Format(L"{\"name\": \"%s\", "
                                     L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                                     L"\"recode-re\": [], \"columns-rename\": [], "
                                     L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                                     L"\"collapse-except\": [], \"formulas\": []}",
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
            SavePivots(pivObj, pivName);
            SaveMerges(pivObj, pivName);

            const auto txIt = transformOpts.find(pivName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(pivObj, txIt->second);
                SaveFormulas(pivObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"merges", L"pivots", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
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
    const auto& transformOpts = m_reportBuilder.GetDatasetTransformOptions();

    auto mrgArray = parentNode->GetProperty(L"merges");
    for (const auto& [mrgName, mOpts] : mergeOpts)
        {
        if (mOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            mrgName.CmpNoCase(sourceName) != 0)
            {
            auto mrgObj = wxSimpleJSON::Create(
                wxString::Format(L"{\"name\": \"%s\", \"by\": [], "
                                 L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                                 L"\"recode-re\": [], \"columns-rename\": [], "
                                 L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                                 L"\"collapse-except\": [], \"formulas\": []}",
                                 EscapeJsonStr(mrgName)));
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

            SaveSubsets(mrgObj, mrgName);
            SavePivots(mrgObj, mrgName);
            SaveMerges(mrgObj, mrgName);

            const auto txIt = transformOpts.find(mrgName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(mrgObj, txIt->second);
                SaveFormulas(mrgObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"pivots", L"merges", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
                {
                if (mrgObj->GetProperty(key)->ArraySize() == 0)
                    {
                    mrgObj->DeleteProperty(key);
                    }
                }

            mrgArray->ArrayAdd(mrgObj);
            }
        }
    }

//-------------------------------------------
wxString WisteriaView::ColorToStr(const wxColour& color) const
    {
    const auto hexStr = color.GetAsString(wxC2S_HTML_SYNTAX);
    // check constants table first (preserves {{ConstantName}} syntax)
    for (const auto& constant : m_reportBuilder.GetConstants())
        {
        if (constant.m_value.CmpNoCase(hexStr) == 0)
            {
            return L"{{" + constant.m_name + L"}}";
            }
        }
    // check named Wisteria colors
    for (const auto& [name, colorEnum] : Wisteria::ReportBuilder::GetColorMap())
        {
        if (Wisteria::Colors::ColorBrewer::GetColor(colorEnum) == color)
            {
            return wxString(name);
            }
        }
    return hexStr;
    }

//-------------------------------------------
wxString WisteriaView::SavePenToStr(const wxPen& pen) const
    {
    if (!pen.IsOk() || pen == wxNullPen)
        {
        return L"null";
        }

    const auto colorStr = ColorToStr(pen.GetColour());
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
wxString WisteriaView::SaveBrushToStr(const wxBrush& brush) const
    {
    if (!brush.IsOk() || brush == wxNullBrush)
        {
        return L"null";
        }

    const auto colorStr = ColorToStr(brush.GetColour());
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

    // scaling — use the original cached value, not the canvas-overwritten one
    const double itemScaling = item->GetGraphItemInfo().GetOriginalCanvasScaling();
    if (!compare_doubles(itemScaling, 1.0))
        {
        itemNode->Add(L"scaling", itemScaling);
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

    // vertical-page-alignment (default is TopAligned)
    const auto vpaStr = Wisteria::ReportEnumConvert::ConvertPageVerticalAlignmentToString(
        item->GetPageVerticalAlignment());
    if (vpaStr.has_value() &&
        item->GetPageVerticalAlignment() != Wisteria::PageVerticalAlignment::TopAligned)
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
wxString WisteriaView::SaveLabelPropertiesToStr(const Wisteria::GraphItems::Label& label) const
    {
    wxString json = L"{";

    // text (prefer template if it has {{constants}})
    const auto textTmpl = label.GetPropertyTemplate(L"text");
    const auto& text = textTmpl.empty() ? label.GetText() : textTmpl;
    if (!text.empty())
        {
        json += L"\"text\": \"" + EscapeJsonStr(text) + L"\"";
        }

    // bold
    if (label.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"bold\": true";
        }

    // color (font color, if not default black)
    const auto& fontColor = label.GetFontColor();
    if (fontColor.IsOk() && fontColor != *wxBLACK)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        const auto colorTmpl = label.GetPropertyTemplate(L"color");
        json += L"\"color\": \"" +
                EscapeJsonStr(colorTmpl.empty() ? ColorToStr(fontColor) : colorTmpl) + L"\"";
        }

    // background color
    const auto& bgColor = label.GetFontBackgroundColor();
    if (bgColor.IsOk() && bgColor != wxTransparentColour)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"background\": \"" + ColorToStr(bgColor) + L"\"";
        }

    // orientation (default is horizontal)
    if (label.GetTextOrientation() == Wisteria::Orientation::Vertical)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"orientation\": \"vertical\"";
        }

    // line-spacing (default is 1)
    if (!compare_doubles(label.GetLineSpacing(), 1.0))
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += wxString::Format(L"\"line-spacing\": %g", label.GetLineSpacing());
        }

    // text-alignment (default is flush-left)
    if (label.GetTextAlignment() != Wisteria::TextAlignment::FlushLeft)
        {
        const auto taStr =
            Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(label.GetTextAlignment());
        if (taStr.has_value())
            {
            if (json.Last() != L'{')
                {
                json += L", ";
                }
            json += L"\"text-alignment\": \"" + taStr.value() + L"\"";
            }
        }

    // padding
    if (label.GetTopPadding() != 0 || label.GetRightPadding() != 0 ||
        label.GetBottomPadding() != 0 || label.GetLeftPadding() != 0)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += wxString::Format(L"\"padding\": [%d, %d, %d, %d]", label.GetTopPadding(),
                                 label.GetRightPadding(), label.GetBottomPadding(),
                                 label.GetLeftPadding());
        }

    // pen
    const auto& pen = label.GetPen();
    if (pen.IsOk() && pen != wxNullPen)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"pen\": " + SavePenToStr(pen);
        }

    // header
    const auto& header = label.GetHeaderInfo();
    if (header.IsEnabled())
        {
        wxString hdrStr = L"{";
        if (header.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
            {
            hdrStr += L"\"bold\": true";
            }
        if (header.GetFontColor().IsOk() && header.GetFontColor() != *wxBLACK)
            {
            if (hdrStr.Last() != L'{')
                {
                hdrStr += L", ";
                }
            hdrStr += L"\"color\": \"" + ColorToStr(header.GetFontColor()) + L"\"";
            }
        if (!compare_doubles(header.GetRelativeScaling(), 1.0))
            {
            if (hdrStr.Last() != L'{')
                {
                hdrStr += L", ";
                }
            hdrStr += wxString::Format(L"\"relative-scaling\": %g", header.GetRelativeScaling());
            }
        if (header.GetLabelAlignment() != Wisteria::TextAlignment::FlushLeft)
            {
            const auto htaStr = Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(
                header.GetLabelAlignment());
            if (htaStr.has_value())
                {
                if (hdrStr.Last() != L'{')
                    {
                    hdrStr += L", ";
                    }
                hdrStr += L"\"text-alignment\": \"" + htaStr.value() + L"\"";
                }
            }
        hdrStr += L"}";
        if (hdrStr != L"{}")
            {
            if (json.Last() != L'{')
                {
                json += L", ";
                }
            json += L"\"header\": " + hdrStr;
            }
        }

    json += L"}";
    return json;
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
        wxString hdrObj = L"{";
        if (header.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
            {
            hdrObj += L"\"bold\": true";
            }
        if (header.GetFontColor().IsOk() && header.GetFontColor() != *wxBLACK)
            {
            if (hdrObj.Last() != L'{')
                {
                hdrObj += L", ";
                }
            hdrObj += L"\"color\": \"" + EscapeJsonStr(ColorToStr(header.GetFontColor())) + L"\"";
            }
        if (!compare_doubles(header.GetRelativeScaling(), 1.0))
            {
            if (hdrObj.Last() != L'{')
                {
                hdrObj += L", ";
                }
            hdrObj += wxString::Format(L"\"relative-scaling\": %g", header.GetRelativeScaling());
            }
        if (header.GetLabelAlignment() != Wisteria::TextAlignment::FlushLeft)
            {
            const auto htaStr = Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(
                header.GetLabelAlignment());
            if (htaStr.has_value())
                {
                if (hdrObj.Last() != L'{')
                    {
                    hdrObj += L", ";
                    }
                hdrObj += L"\"text-alignment\": \"" + htaStr.value() + L"\"";
                }
            }
        hdrObj += L"}";
        if (hdrObj != L"{}")
            {
            headerStr = hdrObj;
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
        node->Add(L"color", colorTemplate.empty() ? ColorToStr(fontColor) : colorTemplate);
        }

    // background color
    const auto& bgColor = label->GetFontBackgroundColor();
    if (bgColor.IsOk() && bgColor != wxTransparentColour)
        {
        node->Add(L"background", ColorToStr(bgColor));
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

    // helper to make absolute paths relative to the project file
    const auto makeRelative = [this](wxString filePath) -> wxString
    {
        if (!m_projectFilePath.empty())
            {
            wxFileName fn(filePath);
            if (fn.IsAbsolute())
                {
                const wxFileName projectDir(m_projectFilePath);
                fn.MakeRelativeTo(projectDir.GetPath());
                filePath = fn.GetFullPath(wxPATH_UNIX);
                }
            }
        return filePath;
    };

    // image-import from property templates
    const auto pathsTemplate = image->GetPropertyTemplate(L"image-import.paths");
    const auto pathTemplate = image->GetPropertyTemplate(L"image-import.path");
    const auto effectTemplate = image->GetPropertyTemplate(L"image-import.effect");
    const auto stitchTemplate = image->GetPropertyTemplate(L"image-import.stitch");

    if (!pathsTemplate.empty())
        {
        // multiple paths (tab-separated)
        tmpl += L", \"image-import\": {\"paths\": [";
        wxStringTokenizer tokenizer(pathsTemplate, L"\t");
        bool first = true;
        while (tokenizer.HasMoreTokens())
            {
            if (!first)
                {
                tmpl += L", ";
                }
            tmpl += L"\"" + EscapeJsonStr(makeRelative(tokenizer.GetNextToken())) + L"\"";
            first = false;
            }
        tmpl += L"]";
        if (!stitchTemplate.empty())
            {
            tmpl += L", \"stitch\": \"" + EscapeJsonStr(stitchTemplate) + L"\"";
            }
        if (!effectTemplate.empty())
            {
            tmpl += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
            }
        tmpl += L"}";
        }
    else if (!pathTemplate.empty())
        {
        // single path
        tmpl += L", \"image-import\": {\"path\": \"" + EscapeJsonStr(makeRelative(pathTemplate)) +
                L"\"";
        if (!effectTemplate.empty())
            {
            tmpl += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
            }
        tmpl += L"}";
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
            const wxString colorVal =
                labelColorTmpl.empty() ? ColorToStr(shape->GetFontColor()) : labelColorTmpl;
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
            const wxString colorVal =
                labelColorTmpl.empty() ? ColorToStr(shape->GetFontColor()) : labelColorTmpl;
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
    const auto numDisplayStr =
        Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(axis->GetNumberDisplay());
    if (numDisplayStr.has_value() && axis->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
        {
        tmpl += L", \"number-display\": \"" + numDisplayStr.value() + L"\"";
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
            if (axis->AreBracketsSimplified())
                {
                tmpl += L", \"simplify\": true";
                }
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
    // indexed variables like y[0], y[1] are collapsed into "y": [...]
    const auto& templates = graph->GetPropertyTemplates();
    wxString varsStr;
    std::map<wxString, std::vector<std::pair<size_t, wxString>>> indexedVars;
    for (const auto& [prop, val] : templates)
        {
        if (prop.StartsWith(L"variables."))
            {
            const auto varName = prop.Mid(10); // skip "variables."
            // check for indexed pattern like "y[0]"
            const auto bracketPos = varName.find(L'[');
            if (bracketPos != wxString::npos && varName.EndsWith(L"]"))
                {
                const auto baseName = varName.Left(bracketPos);
                const auto idxStr = varName.Mid(bracketPos + 1, varName.length() - bracketPos - 2);
                unsigned long idx = 0;
                if (idxStr.ToULong(&idx))
                    {
                    indexedVars[baseName].emplace_back(static_cast<size_t>(idx), val);
                    }
                }
            else
                {
                if (!varsStr.empty())
                    {
                    varsStr += L", ";
                    }
                varsStr += L"\"" + varName + L"\": \"" + EscapeJsonStr(val) + L"\"";
                }
            }
        }
    // write indexed variables as arrays
    for (auto& [baseName, entries] : indexedVars)
        {
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        if (!varsStr.empty())
            {
            varsStr += L", ";
            }
        varsStr += L"\"" + baseName + L"\": [";
        for (size_t i = 0; i < entries.size(); ++i)
            {
            if (i > 0)
                {
                varsStr += L", ";
                }
            varsStr += L"\"" + EscapeJsonStr(entries[i].second) + L"\"";
            }
        varsStr += L"]";
        }

    // title
    wxString titleStr;
    if (!graph->GetTitle().GetText().empty())
        {
        titleStr = SaveLabelPropertiesToStr(graph->GetTitle());
        }

    // subtitle
    wxString subtitleStr;
    if (!graph->GetSubtitle().GetText().empty())
        {
        subtitleStr = SaveLabelPropertiesToStr(graph->GetSubtitle());
        }

    // caption
    wxString captionStr;
    if (!graph->GetCaption().GetText().empty())
        {
        captionStr = SaveLabelPropertiesToStr(graph->GetCaption());
        }

    // background-color
    const auto& bgColor = graph->GetPlotBackgroundColor();
    if (bgColor.IsOk() && !bgColor.IsTransparent())
        {
        graphNode->Add(L"background-color", ColorToStr(bgColor));
        }

    // stipple-shape (only meaningful when box-effect is stipple-shape)
    if (graph->GetStippleShape() != Wisteria::Icons::IconShape::Square)
        {
        const auto iconStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(graph->GetStippleShape());
        if (iconStr.has_value())
            {
            wxString ssObj = L"{\"icon\": \"" + iconStr.value() + L"\"";
            const auto& ssColor = graph->GetStippleShapeColor();
            if (ssColor.IsOk() &&
                ssColor != Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::White))
                {
                ssObj += L", \"color\": \"" + ColorToStr(ssColor) + L"\"";
                }
            ssObj += L"}";
            graphNode->Add(L"stipple-shape", wxSimpleJSON::Create(ssObj));
            }
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

        // number-display
        if (axis->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
            {
            const auto numDisplayStr =
                Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(axis->GetNumberDisplay());
            if (numDisplayStr.has_value())
                {
                axisObj += L", \"number-display\": \"" + numDisplayStr.value() + L"\"";
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

    // add sub-objects directly to the node
    if (!datasetName.empty())
        {
        graphNode->Add(L"dataset", datasetName);
        }
    if (!varsStr.empty())
        {
        graphNode->Add(L"variables", wxSimpleJSON::Create(L"{" + varsStr + L"}"));
        }
    if (!titleStr.empty())
        {
        graphNode->Add(L"title", wxSimpleJSON::Create(titleStr));
        }
    if (!subtitleStr.empty())
        {
        graphNode->Add(L"sub-title", wxSimpleJSON::Create(subtitleStr));
        }
    if (!captionStr.empty())
        {
        graphNode->Add(L"caption", wxSimpleJSON::Create(captionStr));
        }
    if (!axesStr.empty())
        {
        graphNode->Add(L"axes", wxSimpleJSON::Create(L"[" + axesStr + L"]"));
        }
    if (!refLinesStr.empty())
        {
        graphNode->Add(L"reference-lines", wxSimpleJSON::Create(L"[" + refLinesStr + L"]"));
        }
    if (!refAreasStr.empty())
        {
        graphNode->Add(L"reference-areas", wxSimpleJSON::Create(L"[" + refAreasStr + L"]"));
        }

    // annotations
    const auto& annotations = graph->GetAnnotations();
    if (!annotations.empty())
        {
        wxString annotationsStr;
        for (const auto& ann : annotations)
            {
            const auto* label =
                dynamic_cast<const Wisteria::GraphItems::Label*>(ann.GetObject().get());
            if (label == nullptr)
                {
                continue;
                }
            wxString annObj = L"{\"label\": " + SaveLabelPropertiesToStr(*label);
            // anchor point
            const auto anchor = ann.GetAnchorPoint();
            annObj +=
                wxString::Format(L", \"anchor\": {\"x\": %g, \"y\": %g}", anchor.m_x, anchor.m_y);
            // interest points
            const auto& interestPts = ann.GetInterestPoints();
            if (!interestPts.empty())
                {
                wxString ptsStr;
                for (const auto& pt : interestPts)
                    {
                    if (!ptsStr.empty())
                        {
                        ptsStr += L", ";
                        }
                    ptsStr += wxString::Format(L"{\"x\": %g, \"y\": %g}", pt.m_x, pt.m_y);
                    }
                annObj += L", \"interest-points\": [" + ptsStr + L"]";
                }
            annObj += L"}";
            if (!annotationsStr.empty())
                {
                annotationsStr += L", ";
                }
            annotationsStr += annObj;
            }
        if (!annotationsStr.empty())
            {
            graphNode->Add(L"annotations", wxSimpleJSON::Create(L"[" + annotationsStr + L"]"));
            }
        }

    // legend
    const auto& legendInfo = graph->GetLegendInfo();
    if (legendInfo.has_value())
        {
        wxString legendObj = L"{";
        const auto placement = legendInfo->GetPlacement();
        if (placement == Wisteria::Side::Left)
            {
            legendObj += L"\"placement\": \"left\"";
            }
        else if (placement == Wisteria::Side::Top)
            {
            legendObj += L"\"placement\": \"top\"";
            }
        else if (placement == Wisteria::Side::Bottom)
            {
            legendObj += L"\"placement\": \"bottom\"";
            }
        else
            {
            legendObj += L"\"placement\": \"right\"";
            }
        if (!legendInfo->IsIncludingHeader())
            {
            legendObj += L", \"include-header\": false";
            }
        if (legendInfo->GetRingPerimeter() == Wisteria::Perimeter::Inner)
            {
            legendObj += L", \"ring\": \"inner\"";
            }
        if (!legendInfo->GetTitle().empty())
            {
            legendObj += L", \"title\": \"" + EscapeJsonStr(legendInfo->GetTitle()) + L"\"";
            }
        legendObj += L"}";
        graphNode->Add(L"legend", wxSimpleJSON::Create(legendObj));
        }

    // pen (item-level pen for the graph)
    const auto& graphPen = graph->GetPen();
    if (!graphPen.IsOk() || graphPen == wxNullPen)
        {
        graphNode->AddNull(L"pen");
        }
    else if (!(graphPen.GetColour() == *wxBLACK && graphPen.GetWidth() <= 1 &&
               graphPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        graphNode->Add(L"pen", wxSimpleJSON::Create(SavePenToStr(graphPen)));
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
            colorsArr += L"\"" + ColorToStr(brushes[i].GetColour()) + L"\"";
            }
        colorsArr += L"]";

        if (allSolid)
            {
            graphNode->Add(L"brush-scheme",
                           wxSimpleJSON::Create(L"{\"color-scheme\": " + colorsArr + L"}"));
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
            graphNode->Add(L"brush-scheme",
                           wxSimpleJSON::Create(L"{\"brush-styles\": " + stylesArr +
                                                L", \"color-scheme\": " + colorsArr + L"}"));
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
            colorsArr += L"\"" + ColorToStr(colors[i]) + L"\"";
            }
        colorsArr += L"]";
        graphNode->Add(L"color-scheme", wxSimpleJSON::Create(colorsArr));
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
        graphNode->Add(L"icon-scheme", wxSimpleJSON::Create(iconsArr));
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
            node->Add(L"showcase-lines", wxSimpleJSON::Create(L"[" + showcaseArr + L"]"));
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
                node->Add(L"line-scheme", wxSimpleJSON::Create(lsArr));
                }
            }
        // w-curve-plot specific
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
            {
            const auto* wcPlot = dynamic_cast<const Wisteria::Graphs::WCurvePlot*>(graph);
            node->Add(L"time-interval-label", wcPlot->GetTimeIntervalLabel());
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
        // regression-line-scheme
        if (scatterPlot->GetRegressionLineStyleScheme() != nullptr &&
            !scatterPlot->GetRegressionLineStyleScheme()->GetLineStyles().empty())
            {
            const auto& lineStyles = scatterPlot->GetRegressionLineStyleScheme()->GetLineStyles();
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
                node->Add(L"regression-line-scheme", wxSimpleJSON::Create(lsArr));
                }
            }
        // bubble-plot specific
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
            {
            const auto* bubblePlot = dynamic_cast<const Wisteria::Graphs::BubblePlot*>(graph);
            if (bubblePlot->GetMinBubbleRadius() != 4)
                {
                node->Add(L"min-bubble-radius",
                          static_cast<double>(bubblePlot->GetMinBubbleRadius()));
                }
            if (bubblePlot->GetMaxBubbleRadius() != 30)
                {
                node->Add(L"max-bubble-radius",
                          static_cast<double>(bubblePlot->GetMaxBubbleRadius()));
                }
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
        if (barChart->GetBarOrientation() == Wisteria::Orientation::Vertical)
            {
            node->Add(L"bar-orientation", wxString{ _DT(L"vertical") });
            }
        if (barChart->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
            {
            const auto numDisplayStr = Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(
                barChart->GetNumberDisplay());
            if (numDisplayStr.has_value())
                {
                node->Add(L"number-display", numDisplayStr.value());
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
            node->Add(L"showcase-bars", wxSimpleJSON::Create(L"[" + showcaseBarArr + L"]"));
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
                    decalsArr += L", \"color\": \"" + ColorToStr(decalColor) + L"\"";
                    }
                const auto& decalBgColor = decal.GetFontBackgroundColor();
                if (decalBgColor.IsOk())
                    {
                    decalsArr += L", \"background\": \"" + ColorToStr(decalBgColor) + L"\"";
                    }
                decalsArr += L"}}";
                }
            }
        if (!decalsArr.empty())
            {
            node->Add(L"decals", wxSimpleJSON::Create(L"[" + decalsArr + L"]"));
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
            node->Add(L"bar-sort", wxSimpleJSON::Create(sortObj));
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
                    bgArr += L"\"color\": \"" + ColorToStr(bg.m_barColor) + L"\"";
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
            node->Add(L"bar-groups", wxSimpleJSON::Create(bgArr));
            }

            // first-bar-brackets (from cached property templates)
            {
            wxString fbbArr;
            for (size_t i = 0;; ++i)
                {
                const auto idx = std::to_wstring(i);
                const auto startBlock =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].start-block");
                const auto startBlockRe =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].start-block-re");
                const auto endBlock =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block");
                const auto endBlockRe =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block-re");
                const auto label =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label");
                if (startBlock.empty() && startBlockRe.empty())
                    {
                    break;
                    }
                if (!fbbArr.empty())
                    {
                    fbbArr += L", ";
                    }
                fbbArr += L"{";
                if (!startBlockRe.empty())
                    {
                    fbbArr += L"\"start-block-re\": \"" + EscapeJsonStr(startBlockRe) + L"\", ";
                    fbbArr += L"\"end-block-re\": \"" + EscapeJsonStr(endBlockRe) + L"\"";
                    }
                else
                    {
                    fbbArr += L"\"start-block\": \"" + EscapeJsonStr(startBlock) + L"\", ";
                    fbbArr += L"\"end-block\": \"" + EscapeJsonStr(endBlock) + L"\"";
                    }
                if (!label.empty())
                    {
                    fbbArr += L", \"label\": \"" + EscapeJsonStr(label) + L"\"";
                    }
                fbbArr += L"}";
                }
            if (!fbbArr.empty())
                {
                node->Add(L"first-bar-brackets", wxSimpleJSON::Create(L"[" + fbbArr + L"]"));
                }
            }
            // last-bar-brackets (from cached property templates)
            {
            wxString lbbArr;
            for (size_t i = 0;; ++i)
                {
                const auto idx = std::to_wstring(i);
                const auto startBlock =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].start-block");
                const auto startBlockRe =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].start-block-re");
                const auto endBlock =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block");
                const auto endBlockRe =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block-re");
                const auto label =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label");
                if (startBlock.empty() && startBlockRe.empty())
                    {
                    break;
                    }
                if (!lbbArr.empty())
                    {
                    lbbArr += L", ";
                    }
                lbbArr += L"{";
                if (!startBlockRe.empty())
                    {
                    lbbArr += L"\"start-block-re\": \"" + EscapeJsonStr(startBlockRe) + L"\", ";
                    lbbArr += L"\"end-block-re\": \"" + EscapeJsonStr(endBlockRe) + L"\"";
                    }
                else
                    {
                    lbbArr += L"\"start-block\": \"" + EscapeJsonStr(startBlock) + L"\", ";
                    lbbArr += L"\"end-block\": \"" + EscapeJsonStr(endBlock) + L"\"";
                    }
                if (!label.empty())
                    {
                    lbbArr += L", \"label\": \"" + EscapeJsonStr(label) + L"\"";
                    }
                lbbArr += L"}";
                }
            if (!lbbArr.empty())
                {
                node->Add(L"last-bar-brackets", wxSimpleJSON::Create(L"[" + lbbArr + L"]"));
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
                node->Add(L"showcase-slices", wxSimpleJSON::Create(L"[" + showcaseSliceArr + L"]"));
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
            node->Add(L"showcase-slices", wxSimpleJSON::Create(scObj));
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
            node->Add(L"inner-pie-line-pen", wxSimpleJSON::Create(SavePenToStr(ipPen)));
            }
        // margin notes
        if (!pieChart->GetLeftMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetLeftMarginNote(), canvas);
            if (marginNode)
                {
                node->Add(L"left-margin-note", marginNode);
                }
            }
        if (!pieChart->GetRightMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetRightMarginNote(), canvas);
            if (marginNode)
                {
                node->Add(L"right-margin-note", marginNode);
                }
            }
        // donut hole
        if (pieChart->IsIncludingDonutHole())
            {
            wxString donutStr = L"{";
            if (!compare_doubles(pieChart->GetDonutHoleProportion(), math_constants::half))
                {
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
                donutStr += L"\"color\": \"" + ColorToStr(pieChart->GetDonutHoleColor()) + L"\"";
                }
            donutStr += L"}";
            auto donutNode = wxSimpleJSON::Create(donutStr);
            if (!pieChart->GetDonutHoleLabel().GetText().empty())
                {
                const auto labelNode = SaveLabel(&pieChart->GetDonutHoleLabel(), canvas);
                if (labelNode)
                    {
                    donutNode->Add(L"label", labelNode);
                    }
                }
            node->Add(L"donut-hole", donutNode);
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
        node->Add(L"face-color", ColorToStr(chernoff->GetFaceColor()));
        node->Add(L"show-labels", chernoff->IsShowingLabels());
        node->Add(L"outline-color", ColorToStr(chernoff->GetOutlineColor()));
        const auto gStr = Wisteria::ReportEnumConvert::ConvertGenderToString(chernoff->GetGender());
        if (gStr.has_value())
            {
            node->Add(L"gender", gStr.value());
            }
        node->Add(L"eye-color", ColorToStr(chernoff->GetEyeColor()));
        node->Add(L"hair-color", ColorToStr(chernoff->GetHairColor()));
        node->Add(L"lipstick-color", ColorToStr(chernoff->GetLipstickColor()));
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
            node->Add(L"shapes", wxSimpleJSON::Create(shapesArr));
            }
        // grid-round
        if (waffle->GetGridRounding().has_value())
            {
            const auto& gr = waffle->GetGridRounding().value();
            node->Add(L"grid-round", wxSimpleJSON::Create(wxString::Format(
                                         L"{\"cell-count\": %zu, \"shape-index\": %zu}",
                                         gr.m_numberOfCells, gr.m_shapesIndex)));
            }
        // row-count
        if (waffle->GetRowCount().has_value())
            {
            node->Add(L"row-count", static_cast<double>(waffle->GetRowCount().value()));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        const auto* candlePlot = dynamic_cast<const Wisteria::Graphs::CandlestickPlot*>(graph);
        if (candlePlot->GetPlotType() != Wisteria::Graphs::CandlestickPlot::PlotType::Candlestick)
            {
            const auto ptStr = Wisteria::ReportEnumConvert::ConvertCandlestickPlotTypeToString(
                candlePlot->GetPlotType());
            if (ptStr.has_value())
                {
                node->Add(L"plot-type", ptStr.value());
                }
            }
        const auto& gainBrush = candlePlot->GetGainBrush();
        if (gainBrush.IsOk() && gainBrush != wxNullBrush &&
            gainBrush.GetColour() !=
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Green))
            {
            node->Add(L"gain-brush", wxSimpleJSON::Create(SaveBrushToStr(gainBrush)));
            }
        const auto& lossBrush = candlePlot->GetLossBrush();
        if (lossBrush.IsOk() && lossBrush != wxNullBrush &&
            lossBrush.GetColour() !=
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Red))
            {
            node->Add(L"loss-brush", wxSimpleJSON::Create(SaveBrushToStr(lossBrush)));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        const auto* gantt = dynamic_cast<const Wisteria::Graphs::GanttChart*>(graph);
        if (gantt->GetDateDisplayInterval() != Wisteria::DateInterval::FiscalQuarterly)
            {
            const auto diStr = Wisteria::ReportEnumConvert::ConvertDateIntervalToString(
                gantt->GetDateDisplayInterval());
            if (diStr.has_value())
                {
                node->Add(L"date-interval", diStr.value());
                }
            }
        if (gantt->GetFiscalYearType() != Wisteria::FiscalYear::USBusiness)
            {
            const auto fyStr =
                Wisteria::ReportEnumConvert::ConvertFiscalYearToString(gantt->GetFiscalYearType());
            if (fyStr.has_value())
                {
                node->Add(L"fy-type", fyStr.value());
                }
            }
        if (gantt->GetLabelDisplay() != Wisteria::Graphs::GanttChart::TaskLabelDisplay::Days)
            {
            const auto tlStr = Wisteria::ReportEnumConvert::ConvertTaskLabelDisplayToString(
                gantt->GetLabelDisplay());
            if (tlStr.has_value())
                {
                node->Add(L"task-label-display", tlStr.value());
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        const auto* sankey = dynamic_cast<const Wisteria::Graphs::SankeyDiagram*>(graph);
        if (sankey->GetGroupLabelDisplay() != Wisteria::BinLabelDisplay::BinName)
            {
            const auto glStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                sankey->GetGroupLabelDisplay());
            if (glStr.has_value())
                {
                node->Add(L"group-label-display", glStr.value());
                }
            }
        if (sankey->GetColumnHeaderDisplay() != Wisteria::GraphColumnHeader::NoDisplay)
            {
            const auto ghStr = Wisteria::ReportEnumConvert::ConvertGraphColumnHeaderToString(
                sankey->GetColumnHeaderDisplay());
            if (ghStr.has_value())
                {
                node->Add(L"group-header-display", ghStr.value());
                }
            }
        // column-headers (cached as indexed templates)
        wxString colArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                sankey->GetPropertyTemplate(L"column-headers[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!colArr.empty())
                {
                colArr += L", ";
                }
            colArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!colArr.empty())
            {
            node->Add(L"column-headers", wxSimpleJSON::Create(L"[" + colArr + L"]"));
            }
        if (sankey->GetFlowShape() != Wisteria::FlowShape::Curvy)
            {
            const auto fsStr =
                Wisteria::ReportEnumConvert::ConvertFlowShapeToString(sankey->GetFlowShape());
            if (fsStr.has_value())
                {
                node->Add(L"flow-shape", fsStr.value());
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        const auto* heatmap = dynamic_cast<const Wisteria::Graphs::HeatMap*>(graph);
        if (heatmap->GetGroupColumnCount().has_value())
            {
            node->Add(L"group-column-count",
                      static_cast<double>(heatmap->GetGroupColumnCount().value()));
            }
        if (!heatmap->IsShowingGroupHeaders())
            {
            node->Add(L"show-group-header", false);
            }
        if (!heatmap->GetGroupHeaderPrefix().empty())
            {
            node->Add(L"group-header-prefix", heatmap->GetGroupHeaderPrefix());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        const auto* sparkline = dynamic_cast<const Wisteria::Graphs::WinLossSparkline*>(graph);
        if (!sparkline->IsHighlightingBestRecords())
            {
            node->Add(L"highlight-best-records", false);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        const auto* likert = dynamic_cast<const Wisteria::Graphs::LikertChart*>(graph);
        // survey-format
        const auto sfStr = Wisteria::ReportEnumConvert::ConvertLikertSurveyQuestionFormatToString(
            likert->GetSurveyType());
        if (sfStr.has_value())
            {
            node->Add(L"survey-format", sfStr.value());
            }
        // colors (only if non-default)
        if (likert->GetNegativeColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Orange))
            {
            node->Add(L"negative-color", ColorToStr(likert->GetNegativeColor()));
            }
        if (likert->GetPositiveColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Cerulean))
            {
            node->Add(L"positive-color", ColorToStr(likert->GetPositiveColor()));
            }
        if (likert->GetNeutralColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::AshGrey))
            {
            node->Add(L"neutral-color", ColorToStr(likert->GetNeutralColor()));
            }
        if (likert->GetNoResponseColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::White))
            {
            node->Add(L"no-response-color", ColorToStr(likert->GetNoResponseColor()));
            }
        // simplify / apply-default-labels (tracked as property templates)
        if (!likert->GetPropertyTemplate(L"simplify").empty())
            {
            node->Add(L"simplify", true);
            }
        if (!likert->GetPropertyTemplate(L"apply-default-labels").empty())
            {
            node->Add(L"apply-default-labels", true);
            }
        // boolean options
        if (likert->IsShowingResponseCounts())
            {
            node->Add(L"show-response-counts", true);
            }
        if (!likert->IsShowingPercentages())
            {
            node->Add(L"show-percentages", false);
            }
        if (!likert->IsShowingSectionHeaders())
            {
            node->Add(L"show-section-headers", false);
            }
        if (likert->IsSettingBarSizesToRespondentSize())
            {
            node->Add(L"adjust-bar-widths-to-respondent-size", true);
            }
        // header labels
        node->Add(L"positive-label", likert->GetPositiveHeader());
        node->Add(L"negative-label", likert->GetNegativeHeader());
        node->Add(L"no-response-label", likert->GetNoResponseHeader());
        // question-brackets
        const auto& brackets = likert->GetQuestionsBrackets();
        if (!brackets.empty())
            {
            wxString bArr = L"[";
            for (size_t i = 0; i < brackets.size(); ++i)
                {
                if (i > 0)
                    {
                    bArr += L", ";
                    }
                bArr += L"{\"start\": \"" + EscapeJsonStr(brackets[i].m_question1) +
                        L"\", \"end\": \"" + EscapeJsonStr(brackets[i].m_question2) +
                        L"\", \"title\": \"" + EscapeJsonStr(brackets[i].m_title) + L"\"}";
                }
            bArr += L"]";
            node->Add(L"question-brackets", wxSimpleJSON::Create(bArr));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        const auto* lrRoadmap = dynamic_cast<const Wisteria::Graphs::LRRoadmap*>(graph);
        // p-value-threshold
        if (lrRoadmap->GetPValueThreshold().has_value())
            {
            node->Add(L"p-value-threshold", lrRoadmap->GetPValueThreshold().value());
            }
        // predictors-to-include
        if (lrRoadmap->GetPredictorsToInclude().has_value())
            {
            const auto preds = static_cast<int>(lrRoadmap->GetPredictorsToInclude().value());
            wxString predsArr = L"[";
            bool hasEntry{ false };
            if (preds ==
                (Wisteria::Influence::InfluencePositive | Wisteria::Influence::InfluenceNegative |
                 Wisteria::Influence::InfluenceNeutral))
                {
                predsArr += L"\"all\"";
                hasEntry = true;
                }
            else
                {
                if (preds & Wisteria::Influence::InfluencePositive)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"positive\"";
                    hasEntry = true;
                    }
                if (preds & Wisteria::Influence::InfluenceNegative)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"negative\"";
                    hasEntry = true;
                    }
                if (preds & Wisteria::Influence::InfluenceNeutral)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"neutral\"";
                    hasEntry = true;
                    }
                }
            predsArr += L"]";
            if (hasEntry)
                {
                node->Add(L"predictors-to-include", wxSimpleJSON::Create(predsArr));
                }
            }
        // shared roadmap properties
        const auto* roadmap = dynamic_cast<const Wisteria::Graphs::Roadmap*>(graph);
        const auto& roadPen = roadmap->GetRoadPen();
        if (roadPen.IsOk() && roadPen != wxNullPen &&
            !(roadPen.GetColour() == *wxBLACK && roadPen.GetColour().IsOpaque() &&
              roadPen.GetWidth() == 10 && roadPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"road-pen", wxSimpleJSON::Create(SavePenToStr(roadPen)));
            }
        const auto& lanePen = roadmap->GetLaneSeparatorPen();
        if (lanePen.IsOk() && lanePen != wxNullPen)
            {
            node->Add(L"lane-separator-pen", wxSimpleJSON::Create(SavePenToStr(lanePen)));
            }
        if (roadmap->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                roadmap->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (roadmap->GetLaneSeparatorStyle() !=
            Wisteria::Graphs::Roadmap::LaneSeparatorStyle::SingleLine)
            {
            const auto lsStr = Wisteria::ReportEnumConvert::ConvertLaneSeparatorStyleToString(
                roadmap->GetLaneSeparatorStyle());
            if (lsStr.has_value())
                {
                node->Add(L"lane-separator-style", lsStr.value());
                }
            }
        if (roadmap->GetRoadStopTheme() !=
            Wisteria::Graphs::Roadmap::RoadStopTheme::LocationMarkers)
            {
            const auto rsStr = Wisteria::ReportEnumConvert::ConvertRoadStopThemeToString(
                roadmap->GetRoadStopTheme());
            if (rsStr.has_value())
                {
                node->Add(L"road-stop-theme", rsStr.value());
                }
            }
        if (roadmap->GetMarkerLabelDisplay() !=
            Wisteria::Graphs::Roadmap::MarkerLabelDisplay::NameAndValue)
            {
            const auto mlStr = Wisteria::ReportEnumConvert::ConvertMarkerLabelDisplayToString(
                roadmap->GetMarkerLabelDisplay());
            if (mlStr.has_value())
                {
                node->Add(L"marker-label-display", mlStr.value());
                }
            }
        if (lrRoadmap->HasDefaultCaption())
            {
            node->Add(L"default-caption", true);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        const auto* pcRoadmap = dynamic_cast<const Wisteria::Graphs::ProConRoadmap*>(graph);
        // minimum-count
        if (pcRoadmap->GetMinimumCount().has_value())
            {
            node->Add(L"minimum-count", static_cast<double>(pcRoadmap->GetMinimumCount().value()));
            }
        // positive/negative legend labels
        node->Add(L"positive-legend-label", pcRoadmap->GetPositiveLabel());
        node->Add(L"negative-legend-label", pcRoadmap->GetNegativeLabel());
        // shared roadmap properties
        const auto* roadmap = dynamic_cast<const Wisteria::Graphs::Roadmap*>(graph);
        const auto& roadPen = roadmap->GetRoadPen();
        if (roadPen.IsOk() && roadPen != wxNullPen &&
            !(roadPen.GetColour() == *wxBLACK && roadPen.GetColour().IsOpaque() &&
              roadPen.GetWidth() == 10 && roadPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"road-pen", wxSimpleJSON::Create(SavePenToStr(roadPen)));
            }
        const auto& lanePen = roadmap->GetLaneSeparatorPen();
        if (lanePen.IsOk() && lanePen != wxNullPen)
            {
            node->Add(L"lane-separator-pen", wxSimpleJSON::Create(SavePenToStr(lanePen)));
            }
        if (roadmap->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                roadmap->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (roadmap->GetLaneSeparatorStyle() !=
            Wisteria::Graphs::Roadmap::LaneSeparatorStyle::SingleLine)
            {
            const auto lsStr = Wisteria::ReportEnumConvert::ConvertLaneSeparatorStyleToString(
                roadmap->GetLaneSeparatorStyle());
            if (lsStr.has_value())
                {
                node->Add(L"lane-separator-style", lsStr.value());
                }
            }
        if (roadmap->GetRoadStopTheme() !=
            Wisteria::Graphs::Roadmap::RoadStopTheme::LocationMarkers)
            {
            const auto rsStr = Wisteria::ReportEnumConvert::ConvertRoadStopThemeToString(
                roadmap->GetRoadStopTheme());
            if (rsStr.has_value())
                {
                node->Add(L"road-stop-theme", rsStr.value());
                }
            }
        if (roadmap->GetMarkerLabelDisplay() !=
            Wisteria::Graphs::Roadmap::MarkerLabelDisplay::NameAndAbsoluteValue)
            {
            const auto mlStr = Wisteria::ReportEnumConvert::ConvertMarkerLabelDisplayToString(
                roadmap->GetMarkerLabelDisplay());
            if (mlStr.has_value())
                {
                node->Add(L"marker-label-display", mlStr.value());
                }
            }
        if (pcRoadmap->HasDefaultCaption())
            {
            node->Add(L"default-caption", true);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        const auto* table = dynamic_cast<const Wisteria::Graphs::Table*>(graph);
        // default-borders (only if any are false)
        if (!table->IsShowingTopBorder() || !table->IsShowingRightBorder() ||
            !table->IsShowingBottomBorder() || !table->IsShowingLeftBorder())
            {
            node->Add(L"default-borders",
                      wxSimpleJSON::Create(wxString::Format(
                          L"[%s, %s, %s, %s]", table->IsShowingTopBorder() ? L"true" : L"false",
                          table->IsShowingRightBorder() ? L"true" : L"false",
                          table->IsShowingBottomBorder() ? L"true" : L"false",
                          table->IsShowingLeftBorder() ? L"true" : L"false")));
            }
        // min-width-proportion
        if (table->GetMinWidthProportion().has_value())
            {
            node->Add(L"min-width-proportion", table->GetMinWidthProportion().value());
            }
        // min-height-proportion
        if (table->GetMinHeightProportion().has_value())
            {
            node->Add(L"min-height-proportion", table->GetMinHeightProportion().value());
            }
        // clear-trailing-row-formatting
        if (table->IsClearingTrailingRowFormatting())
            {
            node->Add(L"clear-trailing-row-formatting", true);
            }
        // highlight-pen (default is solid red, 1px)
        const auto& hlPen = table->GetHighlightPen();
        if (hlPen.IsOk() && hlPen != wxNullPen &&
            !(hlPen.GetColour() ==
                  Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Red) &&
              hlPen.GetWidth() == 1 && hlPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"highlight-pen", wxSimpleJSON::Create(SavePenToStr(hlPen)));
            }
        // cached procedural properties
        for (const auto& prop : { L"variables",
                                  L"row-sort",
                                  L"insert-group-header",
                                  L"row-group",
                                  L"column-group",
                                  L"alternate-row-color",
                                  L"row-add",
                                  L"row-suppression",
                                  L"column-suppression",
                                  L"row-formatting",
                                  L"row-color",
                                  L"row-bold",
                                  L"row-borders",
                                  L"row-content-align",
                                  L"column-formatting",
                                  L"column-color",
                                  L"column-bold",
                                  L"column-borders",
                                  L"column-content-align",
                                  L"column-highlight",
                                  L"aggregates",
                                  L"row-totals",
                                  L"cell-update",
                                  L"cell-annotations",
                                  L"footnotes" })
            {
            const auto cachedJson = graph->GetPropertyTemplate(prop);
            if (!cachedJson.empty())
                {
                node->Add(wxString(prop), wxSimpleJSON::Create(cachedJson));
                }
            }
        // transpose (cached as "true" string)
        if (graph->GetPropertyTemplate(L"transpose") == L"true")
            {
            node->Add(L"transpose", true);
            }
        // link-id (cached as numeric string)
        const auto linkIdStr = graph->GetPropertyTemplate(L"link-id");
        if (!linkIdStr.empty())
            {
            long linkIdVal{ 0 };
            if (wxString(linkIdStr).ToLong(&linkIdVal))
                {
                node->Add(L"link-id", static_cast<double>(linkIdVal));
                }
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
            wmObj->Add(L"color", ColorToStr(wmColor));
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
    const auto& transformOpts = m_reportBuilder.GetDatasetTransformOptions();

    auto datasetsArray = root->GetProperty(L"datasets");
    const auto& insertionOrder = m_reportBuilder.GetDatasetInsertionOrder();
    for (const auto& dsName : insertionOrder)
        {
        // only write top-level datasets (those with a file path);
        // derived datasets (subsets, pivots, merges) are nested under their parent
        const auto optIt = importOpts.find(dsName);
        if (optIt == importOpts.cend() || optIt->second.m_filePath.empty() ||
            !datasets.contains(dsName))
            {
            continue;
            }

        // build dataset template with name and path up front
        wxString dsTmpl = L"{";
            {
            wxFileName dataPath(optIt->second.m_filePath);
            // only write "name" if it differs from the file stem
            const auto fileStem = dataPath.GetName();
            if (dsName.CmpNoCase(fileStem) != 0)
                {
                dsTmpl += wxString::Format(L"\"name\": \"%s\", ", EscapeJsonStr(dsName));
                }
            dataPath.MakeRelativeTo(projectDir.GetPath());
            dsTmpl += wxString::Format(L"\"path\": \"%s\", ",
                                       EscapeJsonStr(dataPath.GetFullPath(wxPATH_UNIX)));
            }
        dsTmpl += L"\"categorical-columns\": [], \"date-columns\": [], "
                  L"\"recode-re\": [], \"columns-rename\": [], "
                  L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                  L"\"collapse-except\": [], \"formulas\": [], "
                  L"\"subsets\": [], \"pivots\": [], \"merges\": []}";
        auto dsNode = wxSimpleJSON::Create(dsTmpl);

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

        const auto& nameTmpl = canvas->GetNameTemplate();
        const auto& pageName = nameTmpl.empty() ? canvas->GetLabel() : nameTmpl;
        auto pageObj = pageName.empty() ?
                           wxSimpleJSON::Create(L"{\"rows\": []}") :
                           wxSimpleJSON::Create(L"{\"name\": \"" + EscapeJsonStr(pageName) +
                                                L"\", \"rows\": []}");

        auto rowsArray = pageObj->GetProperty(L"rows");
        const auto [rowCount, colCount] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < rowCount; ++row)
            {
            auto rowObj = wxSimpleJSON::Create(L"{\"items\": []}");
            auto itemsArray = rowObj->GetProperty(L"items");
            for (size_t col = 0; col < colCount; ++col)
                {
                const auto item = canvas->GetFixedObject(row, col);
                // skip legend labels (serialized as part of the graph)
                if (item != nullptr && item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
                    {
                    const auto* label =
                        dynamic_cast<const Wisteria::GraphItems::Label*>(item.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        continue;
                        }
                    }
                auto itemNode = SavePageItem(item.get(), canvas);
                if (itemNode != nullptr)
                    {
                    itemsArray->ArrayAdd(itemNode);
                    }
                }
            // skip rows where all items were filtered out (e.g., legend-only rows)
            if (itemsArray->ArraySize() > 0)
                {
                rowsArray->ArrayAdd(rowObj);
                }
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
