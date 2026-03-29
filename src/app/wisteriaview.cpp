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
#include "../ui/dialogs/insertboxplotdlg.h"
#include "../ui/dialogs/insertbubbleplotdlg.h"
#include "../ui/dialogs/insertcandlestickplotdlg.h"
#include "../ui/dialogs/insertcatbarchartdlg.h"
#include "../ui/dialogs/insertchernoffdlg.h"
#include "../ui/dialogs/insertganttchartdlg.h"
#include "../ui/dialogs/insertheatmapdlg.h"
#include "../ui/dialogs/inserthistogramdlg.h"
#include "../ui/dialogs/insertimgdlg.h"
#include "../ui/dialogs/insertlabeldlg.h"
#include "../ui/dialogs/insertlikertdlg.h"
#include "../ui/dialogs/insertlineplotdlg.h"
#include "../ui/dialogs/insertlrroadmapdlg.h"
#include "../ui/dialogs/insertmultiserieslineplotdlg.h"
#include "../ui/dialogs/insertpagedlg.h"
#include "../ui/dialogs/insertpiechartdlg.h"
#include "../ui/dialogs/insertproconroadmapdlg.h"
#include "../ui/dialogs/insertsankeydiagramdlg.h"
#include "../ui/dialogs/insertscatterplotdlg.h"
#include "../ui/dialogs/insertshapedlg.h"
#include "../ui/dialogs/insertstemandleafdlg.h"
#include "../ui/dialogs/inserttabledlg.h"
#include "../ui/dialogs/insertwafflechartdlg.h"
#include "../ui/dialogs/insertwcurvedlg.h"
#include "../ui/dialogs/insertwlsparklinedlg.h"
#include "../ui/dialogs/insertwordclouddlg.h"
#include "../ui/dialogs/pivotlongerdlg.h"
#include "../ui/dialogs/pivotwiderrdlg.h"
#include "wisteriaapp.h"
#include "wisteriadoc.h"

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
    m_objectsButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(m_frame->FindWindowById(ID_OBJECTS_BUTTONBAR));

    // build the graph dropdown menus
    BuildGraphMenus();

    // bind sidebar click event
    m_sideBar->Bind(Wisteria::UI::wxEVT_SIDEBAR_CLICK, &WisteriaView::OnSidebarClick, this);

    // bind print button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPrintAll, this, wxID_PRINT);

    // bind save button
    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event) { GetDocument()->Save(); },
        ID_SAVE_PROJECT);

    // bind insert dataset button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertDataset, this,
                  ID_INSERT_DATASET);

    // bind pivot buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotWider, this, ID_PIVOT_WIDER);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotLonger, this,
                  ID_PIVOT_LONGER);

    // bind add constant button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnRibbonAddConstant, this,
                  ID_ADD_CONSTANT);
    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event)
        {
            // select the Constants folder (index 1) in the sidebar
            m_sideBar->SelectFolder(1, true);
            // trigger the sidebar click so the constants grid is shown
            wxCommandEvent sidebarEvt(Wisteria::UI::wxEVT_SIDEBAR_CLICK);
            sidebarEvt.SetInt(m_constantsGrid->GetId());
            OnSidebarClick(sidebarEvt);
            // delete selected constant
            wxCommandEvent delEvt(wxEVT_MENU, wxID_DELETE);
            OnDeleteConstant(delEvt);
        },
        ID_DELETE_CONSTANT);

    // bind page buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertPage, this, ID_INSERT_PAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditPage, this, ID_EDIT_PAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnDeletePage, this, ID_DELETE_PAGE);

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
        [this]([[maybe_unused]]
               wxCommandEvent& event) { GetDocument()->SaveAs(); },
        ID_SAVE_PROJECT_AS);

    // bind individual graph menu items
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertChernoffPlot, this, ID_NEW_CHERNOFFPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertScatterPlot, this, ID_NEW_SCATTERPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertBubblePlot, this, ID_NEW_BUBBLEPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertLinePlot, this, ID_NEW_LINEPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertMultiSeriesLinePlot, this,
                  ID_NEW_MULTI_SERIES_LINEPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWCurvePlot, this, ID_NEW_WCURVE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertLRRoadmap, this, ID_NEW_LR_ROADMAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertProConRoadmap, this, ID_NEW_PROCON_ROADMAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertGanttChart, this, ID_NEW_GANTT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertCandlestickPlot, this, ID_NEW_CANDLESTICK);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertSankeyDiagram, this, ID_NEW_SANKEY_DIAGRAM);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertBoxPlot, this, ID_NEW_BOXPLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertLikertChart, this, ID_NEW_LIKERT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertHeatMap, this, ID_NEW_HEATMAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertHistogram, this, ID_NEW_HISTOGRAM);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWordCloud, this, ID_NEW_WORD_CLOUD);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWLSparkline, this, ID_NEW_WIN_LOSS_SPARKLINE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertStemAndLeaf, this, ID_NEW_STEMANDLEAF);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertPieChart, this, ID_NEW_PIECHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWaffleChart, this, ID_NEW_WAFFLE_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertCatBarChart, this, ID_NEW_BARCHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertTable, this, ID_NEW_TABLE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertLabel, this, ID_NEW_LABEL);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertImage, this, ID_NEW_IMAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertShape, this, ID_NEW_SHAPE);

    // bind edit/delete item buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditItem, this, ID_EDIT_ITEM);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnDeleteItem, this, ID_DELETE_ITEM);

    // bind DELETE key to delete selected item
    m_frame->Bind(wxEVT_CHAR_HOOK,
                  [this](wxKeyEvent& evt)
                  {
                      if (evt.GetKeyCode() == WXK_DELETE || evt.GetKeyCode() == WXK_NUMPAD_DELETE ||
                          evt.GetKeyCode() == WXK_BACK)
                          {
                          wxCommandEvent cmd;
                          OnDeleteItem(cmd);
                          }
                      else
                          {
                          evt.Skip();
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

    // add the "Constants" item (grid is shown when this sidebar item is clicked)
    const wxWindowID constantsId = nextId;
    nextId = wxNewId();
    m_sideBar->InsertItem(1, _(L"Constants"), constantsId, CONSTANTS_ICON_INDEX);

    m_constantsGrid = new wxGrid(m_workArea, constantsId);
    m_constantsGrid->SetDoubleBuffered(true);
    m_constantsGrid->GetGridWindow()->SetDoubleBuffered(true);
    m_constantsGrid->CreateGrid(0, 4);
    m_constantsGrid->SetColLabelValue(0, _(L"Dataset"));
    m_constantsGrid->SetColLabelValue(1, _(L"Name"));
    m_constantsGrid->SetColLabelValue(2, _(L"Value"));
    m_constantsGrid->SetColLabelValue(3, _(L"Calculated"));
    m_constantsGrid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
    m_constantsGrid->EnableEditing(true);
    // Calculated column is read-only (computed)
    auto* readOnlyAttr = new wxGridCellAttr();
    readOnlyAttr->SetReadOnly();
    m_constantsGrid->SetColAttr(3, readOnlyAttr);
        // add icons to column headers
        {
        const wxSize iconSize{ 16, 16 };
        auto* constAttrProvider = new Wisteria::UI::DatasetGridAttrProvider();
        const std::array<wxString, 4> iconNames = { L"data.svg", L"label.svg", L"constants.svg",
                                                    L"equals.svg" };
        for (size_t col = 0; col < iconNames.size(); ++col)
            {
            const auto bmp = wxGetApp().ReadSvgIcon(iconNames[col], iconSize);
            if (bmp.IsOk())
                {
                constAttrProvider->SetColumnHeaderRenderer(
                    static_cast<int>(col), Wisteria::UI::DatasetColumnHeaderRenderer(bmp));
                }
            }
        m_constantsGrid->GetTable()->SetAttrProvider(constAttrProvider);
        }

    m_constantsGrid->Hide();
    m_workArea->GetSizer()->Add(m_constantsGrid, wxSizerFlags{ 1 }.Expand());
    m_workWindows.AddWindow(m_constantsGrid);

    m_constantsGrid->Bind(wxEVT_GRID_CELL_CHANGED, &WisteriaView::OnConstantEdited, this);
    m_constantsGrid->GetGridWindow()->Bind(
        wxEVT_KEY_DOWN,
        [this](wxKeyEvent& evt)
        {
            if (evt.GetKeyCode() == WXK_DELETE || evt.GetKeyCode() == WXK_NUMPAD_DELETE ||
                evt.GetKeyCode() == WXK_BACK)
                {
                const int row = m_constantsGrid->GetGridCursorRow();
                const int col = m_constantsGrid->GetGridCursorCol();
                if (row >= 0 && col >= 0 && !m_constantsGrid->IsReadOnly(row, col))
                    {
                    const wxString oldValue = m_constantsGrid->GetCellValue(row, col);
                    m_constantsGrid->SetCellValue(row, col, wxString{});
                    wxGridEvent gridEvt(m_constantsGrid->GetId(), wxEVT_GRID_CELL_CHANGED,
                                        m_constantsGrid, row, col);
                    gridEvt.SetString(oldValue);
                    m_constantsGrid->ProcessWindowEvent(gridEvt);
                    }
                }
            else if (evt.GetKeyCode() == WXK_INSERT && evt.ControlDown())
                {
                wxCommandEvent addEvt(wxEVT_MENU, wxID_ADD);
                OnAddConstant(addEvt);
                }
            else
                {
                evt.Skip();
                }
        });

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

            m_sideBar->InsertSubItemById(dataFolderId, dsName, dsId,
                                         GetDatasetIconFromName(dsName));
            }

        // populate constants grid with data from the loaded project
        PopulateConstantsGrid();

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
                                  !canvas->GetNameTemplate().empty() ?
                                      canvas->GetNameTemplate() :
                                      wxString::Format(_(L"Page %zu"), pageNum),
                                  pageId, PAGE_ICON_INDEX);
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
    const int iconOffset = grid->FromDIP(24);
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
void WisteriaView::PopulateConstantsGrid()
    {
    if (m_constantsGrid == nullptr)
        {
        return;
        }

    // clear existing rows
    if (m_constantsGrid->GetNumberRows() > 0)
        {
        m_constantsGrid->DeleteRows(0, m_constantsGrid->GetNumberRows());
        }

    // build the dataset name choices for the dropdown
    // (empty string = top-level constant, otherwise a dataset name)
    wxArrayString dsChoices = { wxString{} };
    for (const auto& [dsName, dataset] : m_reportBuilder.GetDatasets())
        {
        if (!dsName.empty())
            {
            dsChoices.Add(dsName);
            }
        }

    int row{ 0 };

    // helper to set the Dataset cell's choice editor for a row
    const auto setDatasetChoiceEditor = [this, &dsChoices](int gridRow)
    { m_constantsGrid->SetCellEditor(gridRow, 0, new wxGridCellChoiceEditor(dsChoices)); };

    // add top-level constants (no dataset)
    for (const auto& c : m_reportBuilder.GetConstants())
        {
        m_constantsGrid->AppendRows(1);
        m_constantsGrid->SetCellValue(row, 0, wxString{});
        m_constantsGrid->SetCellValue(row, 1, c.m_name);
        m_constantsGrid->SetCellValue(row, 2, c.m_value);
        m_constantsGrid->SetCellValue(row, 3, m_reportBuilder.GetExpandedValue(c.m_name));
        setDatasetChoiceEditor(row);
        ++row;
        }

    // add dataset formula constants
    for (const auto& [dsName, txOpts] : m_reportBuilder.GetDatasetTransformOptions())
        {
        for (const auto& f : txOpts.m_formulas)
            {
            m_constantsGrid->AppendRows(1);
            m_constantsGrid->SetCellValue(row, 0, dsName);
            m_constantsGrid->SetCellValue(row, 1, f.m_name);
            m_constantsGrid->SetCellValue(row, 2, f.m_value);
            m_constantsGrid->SetCellValue(row, 3, m_reportBuilder.GetExpandedValue(f.m_name));
            setDatasetChoiceEditor(row);
            ++row;
            }
        }

    m_constantsGrid->AutoSizeColumns(false);
    // ensure columns are at least wide enough for their header labels plus icon
    const int minColWidth = m_constantsGrid->FromDIP(120);
    for (int col = 0; col < m_constantsGrid->GetNumberCols(); ++col)
        {
        if (m_constantsGrid->GetColSize(col) < minColWidth)
            {
            m_constantsGrid->SetColSize(col, minColWidth);
            }
        }
    }

//-------------------------------------------
void WisteriaView::OnConstantEdited(wxGridEvent& event)
    {
    const int row = event.GetRow();
    const int col = event.GetCol();

    // the new dataset value (after editing)
    const wxString newDsName = m_constantsGrid->GetCellValue(row, 0);
    const wxString name = m_constantsGrid->GetCellValue(row, 1);
    const wxString value = m_constantsGrid->GetCellValue(row, 2);

    // if dataset column was edited...
    if (col == 0)
        {
        // dataset column changed: move the constant between
        // top-level and a dataset's formulas (or between datasets)
        const wxString oldDsName = event.GetString();

        // remove from old location
        if (oldDsName.empty())
            {
            // was a top-level constant
            auto& constants = m_reportBuilder.GetConstants();
            constants.erase({ name, value });
            // force a reset of the calculated values mapped to the constants
            m_reportBuilder.SetConstants(constants);
            }
        else
            {
            // was a dataset formula
            auto txOpts = m_reportBuilder.GetDatasetTransformOptions();
            auto txIt = txOpts.find(oldDsName);
            if (txIt != txOpts.end())
                {
                auto& formulas = txIt->second.m_formulas;
                formulas.erase(
                    std::remove_if(formulas.begin(), formulas.end(),
                                   [&name](const Wisteria::ReportBuilder::DatasetFormulaInfo& f)
                                   { return f.m_name == name; }),
                    formulas.end());
                m_reportBuilder.SetDatasetTransformOptions(oldDsName, txIt->second);
                }
            }

        // add to new location
        if (newDsName.empty())
            {
            // moving to top-level constants
            auto& constants = m_reportBuilder.GetConstants();
            constants.emplace(name, value);
            // force a reset of the calculated values mapped to the constants
            m_reportBuilder.SetConstants(constants);
            }
        else
            {
            // moving to a dataset's formulas
            auto allTxOpts = m_reportBuilder.GetDatasetTransformOptions();
            allTxOpts[newDsName].m_formulas.push_back({ name, value });
            m_reportBuilder.SetDatasetTransformOptions(newDsName, allTxOpts[newDsName]);
            m_reportBuilder.RecalcFormula(name, value, newDsName);
            }
        }
    // ...or other columns of a "regular" constant
    else if (newDsName.empty())
        {
        auto& constants = m_reportBuilder.GetConstants();
        // if name changed, delete the old constant and insert new one
        if (col == 1)
            {
            const wxString oldName = event.GetString();
            constants.erase({ oldName, value });
            constants.insert({ name, value });
            }
        // ...or value changed
        else
            {
            auto pos = constants.find({ name, value });
            auto nh = constants.extract(pos);
            nh.value().m_value = value;
            constants.insert(std::move(nh));
            }

        m_reportBuilder.SetConstants(constants);
        }
    // ...or other column of a dataset formula
    else
        {
        // editing Name or Value of a dataset formula
        auto txOpts = m_reportBuilder.GetDatasetTransformOptions();
        auto txIt = txOpts.find(newDsName);
        if (txIt == txOpts.end())
            {
            return;
            }

        int formulaIdx = 0;
        for (int r = 0; r < row; ++r)
            {
            if (m_constantsGrid->GetCellValue(r, 0) == newDsName)
                {
                ++formulaIdx;
                }
            }

        if (static_cast<size_t>(formulaIdx) >= txIt->second.m_formulas.size())
            {
            return;
            }

        if (col == 1)
            {
            txIt->second.m_formulas[formulaIdx].m_name = name;
            }
        else if (col == 2)
            {
            txIt->second.m_formulas[formulaIdx].m_value = value;
            }

        m_reportBuilder.SetDatasetTransformOptions(newDsName, txIt->second);
        m_reportBuilder.RecalcFormula(name, value, newDsName);
        }

    PopulateConstantsGrid();
    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnAddConstant([[maybe_unused]] wxCommandEvent& event)
    {
    m_constantsGrid->AppendRows();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnDeleteConstant([[maybe_unused]] wxCommandEvent& event)
    {
    std::set<int> selectedRows;

    // rows where the entire row was selected (via labels)
    wxArrayInt fullRows = m_constantsGrid->GetSelectedRows();
    for (int row : fullRows)
        {
        selectedRows.insert(row);
        }

    // rows from rectangular block selections
    wxGridBlocks blocks = m_constantsGrid->GetSelectedBlocks();
    for (const auto& block : blocks)
        {
        for (int r = block.GetTopRow(); r <= block.GetBottomRow(); ++r)
            {
            selectedRows.insert(r);
            }
        }

    // rows from individually selected cells
    wxGridCellCoordsArray cells = m_constantsGrid->GetSelectedCells();
    for (const auto& cell : cells)
        {
        selectedRows.insert(cell.GetRow());
        }

    if (selectedRows.empty())
        {
        wxMessageBox(_(L"Please select a constant to delete."), _(L"Delete Constants"), wxOK,
                     m_frame);
        return;
        }

    if (wxMessageBox(wxPLURAL(L"Delete selected constant?", L"Delete selected constants?",
                              selectedRows.size()),
                     _(L"Delete Constants"), wxYES_NO | wxICON_QUESTION, m_frame) != wxYES)
        {
        return;
        }

    for (auto row : selectedRows)
        {
        const auto constantName = m_constantsGrid->GetCellValue(row, 1);
        if (m_constantsGrid->GetCellValue(row, 0).empty())
            {
            m_reportBuilder.GetConstants().erase({ constantName, wxString{} });
            }
        else
            {
            const wxString dsName{ m_constantsGrid->GetCellValue(row, 0) };
            auto& txOpts = m_reportBuilder.GetDatasetTransformOptions();
            auto txIt = txOpts.find(dsName);
            if (txIt == txOpts.end())
                {
                return;
                }
            auto nh = txOpts.extract(txIt);
            std::erase(nh.mapped().m_formulas,
                       Wisteria::ReportBuilder::DatasetFormulaInfo{ constantName, wxString{} });
            txOpts.insert(std::move(nh));
            }
        }
    PopulateConstantsGrid();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnRibbonAddConstant([[maybe_unused]] wxCommandEvent& event)
    {
    // select the Constants folder (index 1) in the sidebar
    m_sideBar->SelectFolder(1, true);
    // trigger the sidebar click so the constants grid is shown
    wxCommandEvent sidebarEvt(Wisteria::UI::wxEVT_SIDEBAR_CLICK);
    sidebarEvt.SetInt(m_constantsGrid->GetId());
    OnSidebarClick(sidebarEvt);
    // add a new constant
    wxCommandEvent addEvt(wxEVT_MENU, wxID_ADD);
    OnAddConstant(addEvt);
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
        m_sideBar->InsertSubItemById(m_sideBar->GetFolder(0).GetId(), name, dsId,
                                     GetDatasetIconFromName(name));
        m_sideBar->SelectSubItemById(m_sideBar->GetFolder(0).GetId(), dsId);
        }

    m_workArea->Layout();
    m_sideBar->SaveState();
    m_sideBar->Refresh();

    GetDocument()->Modify(true);
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
        canvas->SetNameTemplate(displayName);

        // adjust the splitter sash to match the sidebar's new min width
        const auto minWidth = m_sideBar->GetMinSize().GetWidth();
        m_splitter->SetSashPosition(minWidth);
        }

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnDeletePage([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        wxFAIL_MSG(L"No active canvas when deleting page?!");
        return;
        }

    const auto selectedFolder = m_sideBar->GetSelectedFolder();
    if (!selectedFolder)
        {
        wxFAIL_MSG(L"Sidebar folder not found when deleting page?!");
        return;
        }

    wxWindowUpdateLocker wl{ m_frame };

    m_sideBar->SelectFolder(0, false, false, false);
    m_sideBar->DeleteFolder(selectedFolder.value());
    m_sideBar->SelectFolder(selectedFolder.value() < m_sideBar->GetFolderCount() ?
                                selectedFolder.value() :
                                m_sideBar->GetFolderCount() - 1);

    auto foundPage = std::ranges::find(m_pages, canvas);
    if (foundPage == m_pages.end())
        {
        wxFAIL_MSG(L"Canvas not found when deleting page?!");
        return;
        }
    m_pages.erase(foundPage);
    m_workWindows.RemoveWindowById(canvas->GetId());

    GetDocument()->Modify(true);
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

    GetDocument()->Modify(true);
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

    // graphs
    if (m_graphButtonBar != nullptr)
        {
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BASIC, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BUSINESS, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_STATISTICAL, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SURVEY, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_EDUCATION, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SOCIAL, enabled);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SPORTS, enabled);
        }

    // objects
    if (m_objectsButtonBar != nullptr)
        {
        m_objectsButtonBar->EnableButton(ID_NEW_LABEL, enabled);
        m_objectsButtonBar->EnableButton(ID_NEW_IMAGE, enabled);
        m_objectsButtonBar->EnableButton(ID_NEW_SHAPE, enabled);
        m_objectsButtonBar->EnableButton(ID_EDIT_ITEM, enabled);
        m_objectsButtonBar->EnableButton(ID_DELETE_ITEM, enabled);
        }
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
    appendItem(m_basicGraphMenu, ID_NEW_MULTI_SERIES_LINEPLOT, _(L"Multi-Series Line Plot..."),
               L"lineplot.svg");
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

    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
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

        plot->SetPropertyTemplate(L"enhanced-legend",
                                  dlg.GetUseEnhancedLegend() ? L"true" : L"false");

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
        plot->SetShapeScheme(dlg.GetShapeScheme());

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
void WisteriaView::OnInsertTable([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertTableDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto tableSvg = wxGetApp().GetResourceManager().GetSVG(L"table.svg");
    if (tableSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(tableSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto table = std::make_shared<Wisteria::Graphs::Table>(canvas);
        dlg.ApplyGraphOptions(*table);
        dlg.ApplyPageOptions(*table);

        // resolve variables
        std::vector<wxString> columns;
        const auto varFormula = dlg.GetVariableFormula();
        if (!varFormula.empty())
            {
            auto expanded =
                m_reportBuilder.ExpandColumnSelections(varFormula, dlg.GetSelectedDataset());
            if (expanded.has_value())
                {
                columns = std::move(expanded.value());
                }
            }
        else
            {
            columns = dlg.GetSelectedVariables();
            }

        table->SetData(dlg.GetSelectedDataset(), columns, dlg.GetTranspose());
        dlg.ApplyAxisOverrides(*table);
        table->SetDefaultBorders(true, true, true, true);
        table->SetMinWidthProportion(dlg.GetMinWidthProportion());
        table->SetMinHeightProportion(dlg.GetMinHeightProportion());
        table->ClearTrailingRowFormatting(dlg.GetClearTrailingRowFormatting());

        if (dlg.GetBoldHeaderRow())
            {
            table->BoldRow(0);
            }
        if (dlg.GetCenterHeaderRow())
            {
            table->SetRowHorizontalPageAlignment(0, Wisteria::PageHorizontalAlignment::Centered);
            }
        if (dlg.GetBoldFirstColumn())
            {
            table->BoldColumn(0);
            }
        if (dlg.GetAlternateRowColors())
            {
            table->ApplyAlternateRowColors(dlg.GetAlternateRowColor(), 1);
            }

        // cache property templates for round-tripping
        table->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        if (!varFormula.empty())
            {
            table->SetPropertyTemplate(L"variables", wxString::Format(L"\"%s\"", varFormula));
            }
        else
            {
            // store as JSON array string
            wxString varsJson = L"[";
            for (size_t i = 0; i < columns.size(); ++i)
                {
                if (i > 0)
                    {
                    varsJson += L", ";
                    }
                varsJson += wxString::Format(L"\"%s\"", columns[i]);
                }
            varsJson += L"]";
            table->SetPropertyTemplate(L"variables", varsJson);
            }
        if (dlg.GetTranspose())
            {
            table->SetPropertyTemplate(L"transpose", L"true");
            }
        if (dlg.GetAlternateRowColors())
            {
            const auto color = dlg.GetAlternateRowColor();
            table->SetPropertyTemplate(
                L"alternate-row-color",
                wxString::Format(L"{\"color\":\"%s\"}", color.GetAsString(wxC2S_HTML_SYNTAX)));
            }
        table->SetPropertyTemplate(L"ui.bold-header-row",
                                   dlg.GetBoldHeaderRow() ? L"true" : L"false");
        table->SetPropertyTemplate(L"ui.center-header-row",
                                   dlg.GetCenterHeaderRow() ? L"true" : L"false");
        table->SetPropertyTemplate(L"ui.bold-first-column",
                                   dlg.GetBoldFirstColumn() ? L"true" : L"false");

        PlaceGraphWithLegend(canvas, table, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                             Wisteria::UI::LegendPlacement::None);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditTable(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                             const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertTableDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Table"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto tableSvg = wxGetApp().GetResourceManager().GetSVG(L"table.svg");
    if (tableSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(tableSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto table = std::make_shared<Wisteria::Graphs::Table>(canvas);
        dlg.ApplyGraphOptions(*table);
        dlg.ApplyPageOptions(*table);

        // resolve variables
        std::vector<wxString> columns;
        const auto varFormula = dlg.GetVariableFormula();
        if (!varFormula.empty())
            {
            auto expanded =
                m_reportBuilder.ExpandColumnSelections(varFormula, dlg.GetSelectedDataset());
            if (expanded.has_value())
                {
                columns = std::move(expanded.value());
                }
            }
        else
            {
            columns = dlg.GetSelectedVariables();
            }

        table->SetData(dlg.GetSelectedDataset(), columns, dlg.GetTranspose());
        dlg.ApplyAxisOverrides(*table);
        table->SetDefaultBorders(true, true, true, true);
        table->SetMinWidthProportion(dlg.GetMinWidthProportion());
        table->SetMinHeightProportion(dlg.GetMinHeightProportion());
        table->ClearTrailingRowFormatting(dlg.GetClearTrailingRowFormatting());

        if (dlg.GetBoldHeaderRow())
            {
            table->BoldRow(0);
            }
        if (dlg.GetCenterHeaderRow())
            {
            table->SetRowHorizontalPageAlignment(0, Wisteria::PageHorizontalAlignment::Centered);
            }
        if (dlg.GetBoldFirstColumn())
            {
            table->BoldColumn(0);
            }
        if (dlg.GetAlternateRowColors())
            {
            table->ApplyAlternateRowColors(dlg.GetAlternateRowColor(), 1);
            }

        // cache property templates for round-tripping
        const auto carryForward = [&graph, &table](const wxString& prop, const wxString& newVal,
                                                   const wxString& oldExpanded)
        {
            if (newVal != oldExpanded || newVal.empty())
                {
                table->SetPropertyTemplate(prop, newVal);
                }
            else
                {
                const auto oldTemplate = graph.GetPropertyTemplate(prop);
                table->SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
                }
        };

        carryForward(L"dataset", dlg.GetSelectedDatasetName(),
                     graph.GetPropertyTemplate(L"dataset"));

        if (!varFormula.empty())
            {
            table->SetPropertyTemplate(L"variables", wxString::Format(L"\"%s\"", varFormula));
            }
        else
            {
            wxString varsJson = L"[";
            for (size_t i = 0; i < columns.size(); ++i)
                {
                if (i > 0)
                    {
                    varsJson += L", ";
                    }
                varsJson += wxString::Format(L"\"%s\"", columns[i]);
                }
            varsJson += L"]";
            table->SetPropertyTemplate(L"variables", varsJson);
            }

        if (dlg.GetTranspose())
            {
            table->SetPropertyTemplate(L"transpose", L"true");
            }
        if (dlg.GetAlternateRowColors())
            {
            const auto color = dlg.GetAlternateRowColor();
            table->SetPropertyTemplate(
                L"alternate-row-color",
                wxString::Format(L"{\"color\":\"%s\"}", color.GetAsString(wxC2S_HTML_SYNTAX)));
            }

        table->SetPropertyTemplate(L"ui.bold-header-row",
                                   dlg.GetBoldHeaderRow() ? L"true" : L"false");
        table->SetPropertyTemplate(L"ui.center-header-row",
                                   dlg.GetCenterHeaderRow() ? L"true" : L"false");
        table->SetPropertyTemplate(L"ui.bold-first-column",
                                   dlg.GetBoldFirstColumn() ? L"true" : L"false");

        // carry forward any advanced property templates from the original table
        for (const auto& prop : { L"row-sort",
                                  L"insert-group-header",
                                  L"row-group",
                                  L"column-group",
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
            const auto cached = graph.GetPropertyTemplate(prop);
            if (!cached.empty())
                {
                table->SetPropertyTemplate(wxString(prop), cached);
                }
            }

        PlaceGraphWithLegend(canvas, table, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                             Wisteria::UI::LegendPlacement::None);
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
    if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
        {
        EditBubblePlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::MultiSeriesLinePlot)))
        {
        EditMultiSeriesLinePlot(*graph, canvas, itemRow, itemCol);
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        EditBoxPlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CategoricalBarChart)))
        {
        EditCatBarChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        EditLikertChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        EditHeatMap(*graph, canvas, itemRow, itemCol);
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        EditCandlestickPlot(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        EditSankeyDiagram(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        EditGanttChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        EditWaffleChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        EditTable(*graph, canvas, itemRow, itemCol);
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
    UpdateCanvas(canvas);
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
    dlg.LoadFromGraph(graph);

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
        plot->SetShapeScheme(dlg.GetShapeScheme());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertBubblePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertBubblePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto bubbleSvg = wxGetApp().GetResourceManager().GetSVG(L"bubbleplot.svg");
    if (bubbleSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bubbleSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::BubblePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->ShowRegressionLines(dlg.GetShowRegressionLines());
        plot->ShowConfidenceBands(dlg.GetShowConfidenceBands());
        plot->SetShapeScheme(dlg.GetShapeScheme());
        plot->SetMinBubbleRadius(dlg.GetMinBubbleRadius());
        plot->SetMaxBubbleRadius(dlg.GetMaxBubbleRadius());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(),
                      dlg.GetSizeVariable(), groupCol);

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.y", dlg.GetYVariable());
        plot->SetPropertyTemplate(L"variables.x", dlg.GetXVariable());
        plot->SetPropertyTemplate(L"variables.size", dlg.GetSizeVariable());
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
void WisteriaView::EditBubblePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertBubblePlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Bubble Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto bubbleSvg = wxGetApp().GetResourceManager().GetSVG(L"bubbleplot.svg");
    if (bubbleSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bubbleSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::BubblePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->ShowRegressionLines(dlg.GetShowRegressionLines());
        plot->ShowConfidenceBands(dlg.GetShowConfidenceBands());
        plot->SetShapeScheme(dlg.GetShapeScheme());
        plot->SetMinBubbleRadius(dlg.GetMinBubbleRadius());
        plot->SetMaxBubbleRadius(dlg.GetMaxBubbleRadius());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(),
                      dlg.GetSizeVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

        const auto* oldBubble = dynamic_cast<const Wisteria::Graphs::BubblePlot*>(&graph);
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
                     oldBubble != nullptr ? oldBubble->GetXColumnName() : wxString{});
        carryForward(L"variables.y", dlg.GetYVariable(),
                     oldBubble != nullptr ? oldBubble->GetYColumnName() : wxString{});
        carryForward(L"variables.size", dlg.GetSizeVariable(),
                     oldBubble != nullptr ? oldBubble->GetSizeColumnName() : wxString{});
        const auto oldGroupName =
            (oldBubble != nullptr && !oldBubble->GetSeriesList().empty()) ?
                oldBubble->GetSeriesList().front().GetGroupColumnName().value_or(wxString{}) :
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
    dlg.LoadFromGraph(graph);

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

        plot->SetPropertyTemplate(L"enhanced-legend",
                                  dlg.GetUseEnhancedLegend() ? L"true" : L"false");

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
        dlg.ApplyAxisOverrides(*plot);

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
        plot->SetShapeScheme(dlg.GetShapeScheme());

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
    dlg.LoadFromGraph(graph);

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
        plot->SetShapeScheme(dlg.GetShapeScheme());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertMultiSeriesLinePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertMultiSeriesLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
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
        auto plot = std::make_shared<Wisteria::Graphs::MultiSeriesLinePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->AutoSpline(dlg.GetAutoSpline());
        plot->SetShapeScheme(dlg.GetShapeScheme());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariables(), dlg.GetXVariable());

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        const auto& yVars = dlg.GetYVariables();
        for (size_t i = 0; i < yVars.size(); ++i)
            {
            plot->SetPropertyTemplate(wxString::Format(L"variables.y[%zu]", i), yVars[i]);
            }
        plot->SetPropertyTemplate(L"variables.x", dlg.GetXVariable());

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
void WisteriaView::EditMultiSeriesLinePlot(Wisteria::Graphs::Graph2D& graph,
                                           Wisteria::Canvas* canvas, const size_t graphRow,
                                           const size_t graphCol)
    {
    Wisteria::UI::InsertMultiSeriesLinePlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Multi-Series Line Plot"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
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
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::MultiSeriesLinePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->AutoSpline(dlg.GetAutoSpline());
        plot->SetShapeScheme(dlg.GetShapeScheme());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariables(), dlg.GetXVariable());
        dlg.ApplyAxisOverrides(*plot);

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

        // cache indexed Y variable templates
        const auto& yVars = dlg.GetYVariables();
        for (size_t i = 0; i < yVars.size(); ++i)
            {
            const auto key = wxString::Format(L"variables.y[%zu]", i);
            carryForward(key, yVars[i], graph.GetPropertyTemplate(key));
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
        plot->SetShapeScheme(dlg.GetShapeScheme());

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
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WCurvePlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetShapeScheme(dlg.GetShapeScheme());

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetYVariable(), dlg.GetXVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

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
        dlg.ApplyAxisOverrides(*plot);

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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
        dlg.ApplyAxisOverrides(*plot);

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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertGanttChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertGanttChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto ganttSvg = wxGetApp().GetResourceManager().GetSVG(L"gantt.svg");
    if (ganttSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(ganttSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::GanttChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetLabelDisplay(dlg.GetTaskLabelDisplay());
        const std::optional<wxString> resourceCol =
            dlg.GetResourceVariable().empty() ? std::nullopt :
                                                std::optional<wxString>(dlg.GetResourceVariable());
        const std::optional<wxString> descCol =
            dlg.GetDescriptionVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetDescriptionVariable());
        const std::optional<wxString> compCol =
            dlg.GetCompletionVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetCompletionVariable());
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetDateInterval(), dlg.GetFiscalYearType(),
                      dlg.GetTaskVariable(), dlg.GetStartDateVariable(), dlg.GetEndDateVariable(),
                      resourceCol, descCol, compCol, groupCol);

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.task", dlg.GetTaskVariable());
        plot->SetPropertyTemplate(L"variables.start-date", dlg.GetStartDateVariable());
        plot->SetPropertyTemplate(L"variables.end-date", dlg.GetEndDateVariable());
        if (!dlg.GetResourceVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.resource", dlg.GetResourceVariable());
            }
        if (!dlg.GetDescriptionVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.description", dlg.GetDescriptionVariable());
            }
        if (!dlg.GetCompletionVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.completion", dlg.GetCompletionVariable());
            }
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
void WisteriaView::EditGanttChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertGanttChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Gantt Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto ganttSvg = wxGetApp().GetResourceManager().GetSVG(L"gantt.svg");
    if (ganttSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(ganttSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::GanttChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetLabelDisplay(dlg.GetTaskLabelDisplay());
        const std::optional<wxString> resourceCol =
            dlg.GetResourceVariable().empty() ? std::nullopt :
                                                std::optional<wxString>(dlg.GetResourceVariable());
        const std::optional<wxString> descCol =
            dlg.GetDescriptionVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetDescriptionVariable());
        const std::optional<wxString> compCol =
            dlg.GetCompletionVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetCompletionVariable());
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetDateInterval(), dlg.GetFiscalYearType(),
                      dlg.GetTaskVariable(), dlg.GetStartDateVariable(), dlg.GetEndDateVariable(),
                      resourceCol, descCol, compCol, groupCol);
        dlg.ApplyAxisOverrides(*plot);

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
        carryForward(L"variables.task", dlg.GetTaskVariable(),
                     graph.GetPropertyTemplate(L"variables.task"));
        carryForward(L"variables.start-date", dlg.GetStartDateVariable(),
                     graph.GetPropertyTemplate(L"variables.start-date"));
        carryForward(L"variables.end-date", dlg.GetEndDateVariable(),
                     graph.GetPropertyTemplate(L"variables.end-date"));
        carryForward(L"variables.resource", dlg.GetResourceVariable(),
                     graph.GetPropertyTemplate(L"variables.resource"));
        carryForward(L"variables.description", dlg.GetDescriptionVariable(),
                     graph.GetPropertyTemplate(L"variables.description"));
        carryForward(L"variables.completion", dlg.GetCompletionVariable(),
                     graph.GetPropertyTemplate(L"variables.completion"));
        carryForward(L"variables.group", dlg.GetGroupVariable(),
                     graph.GetPropertyTemplate(L"variables.group"));

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertCandlestickPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertCandlestickPlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto candleSvg = wxGetApp().GetResourceManager().GetSVG(L"candlestick.svg");
    if (candleSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(candleSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::CandlestickPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetPlotType(dlg.GetPlotType());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetDateVariable(), dlg.GetOpenVariable(),
                      dlg.GetHighVariable(), dlg.GetLowVariable(), dlg.GetCloseVariable());

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.date", dlg.GetDateVariable());
        plot->SetPropertyTemplate(L"variables.open", dlg.GetOpenVariable());
        plot->SetPropertyTemplate(L"variables.high", dlg.GetHighVariable());
        plot->SetPropertyTemplate(L"variables.low", dlg.GetLowVariable());
        plot->SetPropertyTemplate(L"variables.close", dlg.GetCloseVariable());

        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditCandlestickPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                       const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertCandlestickPlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Candlestick Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto candleSvg = wxGetApp().GetResourceManager().GetSVG(L"candlestick.svg");
    if (candleSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(candleSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::CandlestickPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetPlotType(dlg.GetPlotType());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetDateVariable(), dlg.GetOpenVariable(),
                      dlg.GetHighVariable(), dlg.GetLowVariable(), dlg.GetCloseVariable());
        dlg.ApplyAxisOverrides(*plot);

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
        carryForward(L"variables.date", dlg.GetDateVariable(),
                     graph.GetPropertyTemplate(L"variables.date"));
        carryForward(L"variables.open", dlg.GetOpenVariable(),
                     graph.GetPropertyTemplate(L"variables.open"));
        carryForward(L"variables.high", dlg.GetHighVariable(),
                     graph.GetPropertyTemplate(L"variables.high"));
        carryForward(L"variables.low", dlg.GetLowVariable(),
                     graph.GetPropertyTemplate(L"variables.low"));
        carryForward(L"variables.close", dlg.GetCloseVariable(),
                     graph.GetPropertyTemplate(L"variables.close"));

        canvas->SetFixedObject(graphRow, graphCol, plot);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertSankeyDiagram([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertSankeyDiagramDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto sankeySvg = wxGetApp().GetResourceManager().GetSVG(L"sankey.svg");
    if (sankeySvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(sankeySvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::SankeyDiagram>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetFlowShape(dlg.GetFlowShape());
        plot->SetGroupLabelDisplay(dlg.GetGroupLabelDisplay());
        plot->SetColumnHeaderDisplay(dlg.GetColumnHeaderDisplay());

        const std::optional<wxString> fromWeightCol =
            dlg.GetFromWeightVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetFromWeightVariable());
        const std::optional<wxString> toWeightCol =
            dlg.GetToWeightVariable().empty() ? std::nullopt :
                                                std::optional<wxString>(dlg.GetToWeightVariable());
        const std::optional<wxString> fromGroupCol =
            dlg.GetFromGroupVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetFromGroupVariable());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetFromVariable(), dlg.GetToVariable(),
                      fromWeightCol, toWeightCol, fromGroupCol);

        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.from", dlg.GetFromVariable());
        plot->SetPropertyTemplate(L"variables.to", dlg.GetToVariable());
        if (!dlg.GetFromWeightVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.from-weight", dlg.GetFromWeightVariable());
            }
        if (!dlg.GetToWeightVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.to-weight", dlg.GetToWeightVariable());
            }
        if (!dlg.GetFromGroupVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.from-group", dlg.GetFromGroupVariable());
            }

        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditSankeyDiagram(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                     const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertSankeyDiagramDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Sankey Diagram"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto sankeySvg = wxGetApp().GetResourceManager().GetSVG(L"sankey.svg");
    if (sankeySvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(sankeySvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::SankeyDiagram>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        plot->SetFlowShape(dlg.GetFlowShape());
        plot->SetGroupLabelDisplay(dlg.GetGroupLabelDisplay());
        plot->SetColumnHeaderDisplay(dlg.GetColumnHeaderDisplay());

        const std::optional<wxString> fromWeightCol =
            dlg.GetFromWeightVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetFromWeightVariable());
        const std::optional<wxString> toWeightCol =
            dlg.GetToWeightVariable().empty() ? std::nullopt :
                                                std::optional<wxString>(dlg.GetToWeightVariable());
        const std::optional<wxString> fromGroupCol =
            dlg.GetFromGroupVariable().empty() ?
                std::nullopt :
                std::optional<wxString>(dlg.GetFromGroupVariable());

        plot->SetData(dlg.GetSelectedDataset(), dlg.GetFromVariable(), dlg.GetToVariable(),
                      fromWeightCol, toWeightCol, fromGroupCol);
        dlg.ApplyAxisOverrides(*plot);

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
        carryForward(L"variables.from", dlg.GetFromVariable(),
                     graph.GetPropertyTemplate(L"variables.from"));
        carryForward(L"variables.to", dlg.GetToVariable(),
                     graph.GetPropertyTemplate(L"variables.to"));
        carryForward(L"variables.from-weight", dlg.GetFromWeightVariable(),
                     graph.GetPropertyTemplate(L"variables.from-weight"));
        carryForward(L"variables.to-weight", dlg.GetToWeightVariable(),
                     graph.GetPropertyTemplate(L"variables.to-weight"));
        carryForward(L"variables.from-group", dlg.GetFromGroupVariable(),
                     graph.GetPropertyTemplate(L"variables.from-group"));

        canvas->SetFixedObject(graphRow, graphCol, plot);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertBoxPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

    Wisteria::UI::InsertBoxPlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto bpSvg = wxGetApp().GetResourceManager().GetSVG(L"boxplot.svg");
    if (bpSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bpSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::BoxPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

        plot->SetBoxEffect(dlg.GetBoxEffect());
        plot->ShowAllPoints(dlg.GetShowAllPoints());
        plot->ShowLabels(dlg.GetShowLabels());
        plot->ShowMidpointConnection(dlg.GetShowMidpointConnection());

        // apply stipple shape or image settings based on the selected effect
        const auto boxEffect = dlg.GetBoxEffect();
        if (boxEffect == Wisteria::BoxEffect::StippleShape)
            {
            plot->SetStippleShape(dlg.GetStippleShape());
            plot->SetStippleShapeColor(dlg.GetStippleShapeColor());
            }
        else if (boxEffect == Wisteria::BoxEffect::StippleImage && !dlg.GetImagePaths().empty())
            {
            wxImage img(doc->ResolveFilePath(dlg.GetImagePaths()[0]), wxBITMAP_TYPE_ANY);
            if (img.IsOk() && dlg.GetImageEffect() != Wisteria::ImageEffect::NoEffect)
                {
                img = Wisteria::GraphItems::Image::ApplyEffect(dlg.GetImageEffect(), img);
                }
            if (img.IsOk())
                {
                plot->SetStippleBrush(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                }
            }
        else if ((boxEffect == Wisteria::BoxEffect::CommonImage ||
                  boxEffect == Wisteria::BoxEffect::Image) &&
                 !dlg.GetImagePaths().empty())
            {
            const auto imgEffect = dlg.GetImageEffect();
            std::vector<wxBitmapBundle> images;
            images.reserve(dlg.GetImagePaths().GetCount());
            for (const auto& path : dlg.GetImagePaths())
                {
                wxImage img(doc->ResolveFilePath(path), wxBITMAP_TYPE_ANY);
                if (img.IsOk() && imgEffect != Wisteria::ImageEffect::NoEffect)
                    {
                    img = Wisteria::GraphItems::Image::ApplyEffect(imgEffect, img);
                    }
                if (img.IsOk())
                    {
                    images.emplace_back(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                    }
                }
            plot->SetImageScheme(
                std::make_shared<Wisteria::Images::Schemes::ImageScheme>(std::move(images)));
            }

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.aggregate", dlg.GetContinuousVariable());
        if (!dlg.GetGroupVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.group-1", dlg.GetGroupVariable());
            }

        // cache stipple shape and image settings for round-tripping
        const auto shapeStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(dlg.GetStippleShape());
        if (shapeStr)
            {
            plot->SetPropertyTemplate(L"stipple-shape", shapeStr.value());
            }
        plot->SetPropertyTemplate(L"stipple-shape-color",
                                  dlg.GetStippleShapeColor().GetAsString(wxC2S_HTML_SYNTAX));
        if (!dlg.GetImagePaths().empty())
            {
            wxString paths;
            for (size_t idx = 0; idx < dlg.GetImagePaths().GetCount(); ++idx)
                {
                if (!paths.empty())
                    {
                    paths += L"\t";
                    }
                paths += dlg.GetImagePaths()[idx];
                }
            plot->SetPropertyTemplate(L"image-paths", paths);
            }
        if (dlg.IsImageCustomSizeEnabled())
            {
            plot->SetPropertyTemplate(L"image-width", std::to_wstring(dlg.GetImageWidth()));
            plot->SetPropertyTemplate(L"image-height", std::to_wstring(dlg.GetImageHeight()));
            }
            {
            const auto resizeStr = Wisteria::ReportEnumConvert::ConvertResizeMethodToString(
                dlg.GetImageResizeMethod());
            if (resizeStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-resize-method", resizeStr.value());
                }
            }
            {
            const auto effectStr =
                Wisteria::ReportEnumConvert::ConvertImageEffectToString(dlg.GetImageEffect());
            if (effectStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-effect", effectStr.value());
                }
            }
        plot->SetPropertyTemplate(
            L"image-stitch", (dlg.GetImageStitchDirection() == Wisteria::Orientation::Vertical) ?
                                 L"vertical" :
                                 L"horizontal");

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
void WisteriaView::EditBoxPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                               const size_t graphRow, const size_t graphCol)
    {
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

    Wisteria::UI::InsertBoxPlotDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Box Plot"),
                                       wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                       Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto bpSvg = wxGetApp().GetResourceManager().GetSVG(L"boxplot.svg");
    if (bpSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bpSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::BoxPlot>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol);
        dlg.ApplyAxisOverrides(*plot);

        plot->SetBoxEffect(dlg.GetBoxEffect());
        plot->ShowAllPoints(dlg.GetShowAllPoints());
        plot->ShowLabels(dlg.GetShowLabels());
        plot->ShowMidpointConnection(dlg.GetShowMidpointConnection());

        // apply stipple shape or image settings based on the selected effect
        const auto boxEffect = dlg.GetBoxEffect();
        if (boxEffect == Wisteria::BoxEffect::StippleShape)
            {
            plot->SetStippleShape(dlg.GetStippleShape());
            plot->SetStippleShapeColor(dlg.GetStippleShapeColor());
            }
        else if (boxEffect == Wisteria::BoxEffect::StippleImage && !dlg.GetImagePaths().empty())
            {
            wxImage img(doc->ResolveFilePath(dlg.GetImagePaths()[0]), wxBITMAP_TYPE_ANY);
            if (img.IsOk() && dlg.GetImageEffect() != Wisteria::ImageEffect::NoEffect)
                {
                img = Wisteria::GraphItems::Image::ApplyEffect(dlg.GetImageEffect(), img);
                }
            if (img.IsOk())
                {
                plot->SetStippleBrush(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                }
            }
        else if ((boxEffect == Wisteria::BoxEffect::CommonImage ||
                  boxEffect == Wisteria::BoxEffect::Image) &&
                 !dlg.GetImagePaths().empty())
            {
            const auto imgEffect = dlg.GetImageEffect();
            std::vector<wxBitmapBundle> images;
            images.reserve(dlg.GetImagePaths().GetCount());
            for (const auto& path : dlg.GetImagePaths())
                {
                wxImage img(doc->ResolveFilePath(path), wxBITMAP_TYPE_ANY);
                if (img.IsOk() && imgEffect != Wisteria::ImageEffect::NoEffect)
                    {
                    img = Wisteria::GraphItems::Image::ApplyEffect(imgEffect, img);
                    }
                if (img.IsOk())
                    {
                    images.emplace_back(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                    }
                }
            plot->SetImageScheme(
                std::make_shared<Wisteria::Images::Schemes::ImageScheme>(std::move(images)));
            }

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldBoxPlot = dynamic_cast<const Wisteria::Graphs::BoxPlot*>(&graph);
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
        carryForward(L"variables.aggregate", dlg.GetContinuousVariable(),
                     oldBoxPlot != nullptr ? oldBoxPlot->GetContinuousColumnName() : wxString{});
        const auto oldGroupName = (oldBoxPlot != nullptr) ?
                                      oldBoxPlot->GetGroupColumnName().value_or(wxString{}) :
                                      wxString{};
        carryForward(L"variables.group-1", dlg.GetGroupVariable(), oldGroupName);

        // cache stipple shape and image settings for round-tripping
        const auto shapeStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(dlg.GetStippleShape());
        if (shapeStr)
            {
            plot->SetPropertyTemplate(L"stipple-shape", shapeStr.value());
            }
        plot->SetPropertyTemplate(L"stipple-shape-color",
                                  dlg.GetStippleShapeColor().GetAsString(wxC2S_HTML_SYNTAX));
        if (!dlg.GetImagePaths().empty())
            {
            wxString paths;
            for (size_t idx = 0; idx < dlg.GetImagePaths().GetCount(); ++idx)
                {
                if (!paths.empty())
                    {
                    paths += L"\t";
                    }
                paths += dlg.GetImagePaths()[idx];
                }
            plot->SetPropertyTemplate(L"image-paths", paths);
            }
        if (dlg.IsImageCustomSizeEnabled())
            {
            plot->SetPropertyTemplate(L"image-width", std::to_wstring(dlg.GetImageWidth()));
            plot->SetPropertyTemplate(L"image-height", std::to_wstring(dlg.GetImageHeight()));
            }
            {
            const auto resizeStr = Wisteria::ReportEnumConvert::ConvertResizeMethodToString(
                dlg.GetImageResizeMethod());
            if (resizeStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-resize-method", resizeStr.value());
                }
            }
            {
            const auto effectStr =
                Wisteria::ReportEnumConvert::ConvertImageEffectToString(dlg.GetImageEffect());
            if (effectStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-effect", effectStr.value());
                }
            }
        plot->SetPropertyTemplate(
            L"image-stitch", (dlg.GetImageStitchDirection() == Wisteria::Orientation::Vertical) ?
                                 L"vertical" :
                                 L"horizontal");

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertCatBarChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

    Wisteria::UI::InsertCatBarChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto bcSvg = wxGetApp().GetResourceManager().GetSVG(L"barchart.svg");
    if (bcSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bcSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::CategoricalBarChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        plot->SetBarOrientation(dlg.GetBarOrientation());

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetCategoricalVariable(), weightCol, groupCol,
                      dlg.GetBarLabelDisplay());

        // apply custom bar sort
        if (dlg.HasCustomBarSort())
            {
            plot->SetPropertyTemplate(L"bar-sort", L"true");
            if (dlg.GetBarSortComparison().has_value())
                {
                plot->SortBars(dlg.GetBarSortComparison().value(), dlg.GetBarSortDirection());
                }
            else if (!dlg.GetBarSortLabels().empty())
                {
                plot->SortBars(dlg.GetBarSortLabels(), dlg.GetBarSortDirection());
                }
            }

        // apply bar groups
        for (const auto& group : dlg.GetBarGroups())
            {
            plot->AddBarGroup(
                group.m_startLabel, group.m_endLabel,
                group.m_decal.empty() ? std::nullopt : std::optional<wxString>(group.m_decal),
                group.m_color.IsOk() ? std::optional<wxColour>(group.m_color) : std::nullopt,
                group.m_color.IsOk() ? std::optional<wxBrush>(wxBrush(group.m_color)) :
                                       std::nullopt);
            }

        plot->SetBarEffect(dlg.GetBoxEffect());

        // apply stipple shape or image settings based on the selected effect
        const auto boxEffect = dlg.GetBoxEffect();
        if (boxEffect == Wisteria::BoxEffect::StippleShape)
            {
            plot->SetStippleShape(dlg.GetStippleShape());
            plot->SetStippleShapeColor(dlg.GetStippleShapeColor());
            }
        else if (boxEffect == Wisteria::BoxEffect::StippleImage && !dlg.GetImagePaths().empty())
            {
            wxImage img(doc->ResolveFilePath(dlg.GetImagePaths()[0]), wxBITMAP_TYPE_ANY);
            if (img.IsOk() && dlg.GetImageEffect() != Wisteria::ImageEffect::NoEffect)
                {
                img = Wisteria::GraphItems::Image::ApplyEffect(dlg.GetImageEffect(), img);
                }
            if (img.IsOk())
                {
                plot->SetStippleBrush(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                }
            }
        else if ((boxEffect == Wisteria::BoxEffect::CommonImage ||
                  boxEffect == Wisteria::BoxEffect::Image) &&
                 !dlg.GetImagePaths().empty())
            {
            const auto imgEffect = dlg.GetImageEffect();
            std::vector<wxBitmapBundle> images;
            images.reserve(dlg.GetImagePaths().GetCount());
            for (const auto& path : dlg.GetImagePaths())
                {
                wxImage img(doc->ResolveFilePath(path), wxBITMAP_TYPE_ANY);
                if (img.IsOk() && imgEffect != Wisteria::ImageEffect::NoEffect)
                    {
                    img = Wisteria::GraphItems::Image::ApplyEffect(imgEffect, img);
                    }
                if (img.IsOk())
                    {
                    images.emplace_back(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                    }
                }
            plot->SetImageScheme(
                std::make_shared<Wisteria::Images::Schemes::ImageScheme>(std::move(images)));
            }

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        plot->SetPropertyTemplate(L"variables.category", dlg.GetCategoricalVariable());
        if (!dlg.GetWeightVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.aggregate", dlg.GetWeightVariable());
            }
        if (!dlg.GetGroupVariable().empty())
            {
            plot->SetPropertyTemplate(L"variables.group", dlg.GetGroupVariable());
            }

        // cache stipple shape and image settings for round-tripping
        const auto shapeStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(dlg.GetStippleShape());
        if (shapeStr)
            {
            plot->SetPropertyTemplate(L"stipple-shape", shapeStr.value());
            }
        plot->SetPropertyTemplate(L"stipple-shape-color",
                                  dlg.GetStippleShapeColor().GetAsString(wxC2S_HTML_SYNTAX));
        if (!dlg.GetImagePaths().empty())
            {
            wxString paths;
            for (size_t idx = 0; idx < dlg.GetImagePaths().GetCount(); ++idx)
                {
                if (!paths.empty())
                    {
                    paths += L"\t";
                    }
                paths += dlg.GetImagePaths()[idx];
                }
            plot->SetPropertyTemplate(L"image-paths", paths);
            }
        if (dlg.IsImageCustomSizeEnabled())
            {
            plot->SetPropertyTemplate(L"image-width", std::to_wstring(dlg.GetImageWidth()));
            plot->SetPropertyTemplate(L"image-height", std::to_wstring(dlg.GetImageHeight()));
            }
            {
            const auto resizeStr = Wisteria::ReportEnumConvert::ConvertResizeMethodToString(
                dlg.GetImageResizeMethod());
            if (resizeStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-resize-method", resizeStr.value());
                }
            }
            {
            const auto effectStr =
                Wisteria::ReportEnumConvert::ConvertImageEffectToString(dlg.GetImageEffect());
            if (effectStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-effect", effectStr.value());
                }
            }
        plot->SetPropertyTemplate(
            L"image-stitch", (dlg.GetImageStitchDirection() == Wisteria::Orientation::Vertical) ?
                                 L"vertical" :
                                 L"horizontal");

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
void WisteriaView::EditCatBarChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

    Wisteria::UI::InsertCatBarChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Bar Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto bcSvg = wxGetApp().GetResourceManager().GetSVG(L"barchart.svg");
    if (bcSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(bcSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::CategoricalBarChart>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        plot->SetBarOrientation(dlg.GetBarOrientation());

        const std::optional<wxString> weightCol =
            dlg.GetWeightVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(dlg.GetWeightVariable());
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetCategoricalVariable(), weightCol, groupCol,
                      dlg.GetBarLabelDisplay());

        // restore custom bar sort from the previous chart
        if (dlg.HasCustomBarSort())
            {
            plot->SetPropertyTemplate(L"bar-sort", L"true");
            if (dlg.GetBarSortComparison().has_value())
                {
                plot->SortBars(dlg.GetBarSortComparison().value(), dlg.GetBarSortDirection());
                }
            else if (!dlg.GetBarSortLabels().empty())
                {
                plot->SortBars(dlg.GetBarSortLabels(), dlg.GetBarSortDirection());
                }
            }

        // restore bar groups and placement
        plot->SetBarGroupPlacement(dlg.GetBarGroupPlacement());
        for (const auto& group : dlg.GetBarGroups())
            {
            plot->AddBarGroup(
                group.m_startLabel, group.m_endLabel,
                group.m_decal.empty() ? std::nullopt : std::optional<wxString>(group.m_decal),
                group.m_color.IsOk() ? std::optional<wxColour>(group.m_color) : std::nullopt,
                group.m_color.IsOk() ? std::optional<wxBrush>(wxBrush(group.m_color)) :
                                       std::nullopt);
            }

        plot->SetBarEffect(dlg.GetBoxEffect());

        // apply stipple shape or image settings based on the selected effect
        const auto boxEffect = dlg.GetBoxEffect();
        if (boxEffect == Wisteria::BoxEffect::StippleShape)
            {
            plot->SetStippleShape(dlg.GetStippleShape());
            plot->SetStippleShapeColor(dlg.GetStippleShapeColor());
            }
        else if (boxEffect == Wisteria::BoxEffect::StippleImage && !dlg.GetImagePaths().empty())
            {
            wxImage img(doc->ResolveFilePath(dlg.GetImagePaths()[0]), wxBITMAP_TYPE_ANY);
            if (img.IsOk() && dlg.GetImageEffect() != Wisteria::ImageEffect::NoEffect)
                {
                img = Wisteria::GraphItems::Image::ApplyEffect(dlg.GetImageEffect(), img);
                }
            if (img.IsOk())
                {
                plot->SetStippleBrush(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                }
            }
        else if ((boxEffect == Wisteria::BoxEffect::CommonImage ||
                  boxEffect == Wisteria::BoxEffect::Image) &&
                 !dlg.GetImagePaths().empty())
            {
            const auto imgEffect = dlg.GetImageEffect();
            std::vector<wxBitmapBundle> images;
            images.reserve(dlg.GetImagePaths().GetCount());
            for (const auto& path : dlg.GetImagePaths())
                {
                wxImage img(doc->ResolveFilePath(path), wxBITMAP_TYPE_ANY);
                if (img.IsOk() && imgEffect != Wisteria::ImageEffect::NoEffect)
                    {
                    img = Wisteria::GraphItems::Image::ApplyEffect(imgEffect, img);
                    }
                if (img.IsOk())
                    {
                    images.emplace_back(wxBitmapBundle::FromBitmap(wxBitmap(img)));
                    }
                }
            plot->SetImageScheme(
                std::make_shared<Wisteria::Images::Schemes::ImageScheme>(std::move(images)));
            }

        dlg.ApplyAxisOverrides(*plot);

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldBarChart =
            dynamic_cast<const Wisteria::Graphs::CategoricalBarChart*>(&graph);
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
        carryForward(L"variables.category", dlg.GetCategoricalVariable(),
                     oldBarChart != nullptr ? oldBarChart->GetCategoricalColumnName() : wxString{});
        const auto oldWeightName = (oldBarChart != nullptr) ?
                                       oldBarChart->GetWeightColumnName().value_or(wxString{}) :
                                       wxString{};
        carryForward(L"variables.aggregate", dlg.GetWeightVariable(), oldWeightName);
        const auto oldGroupName = (oldBarChart != nullptr) ?
                                      oldBarChart->GetGroupColumnName().value_or(wxString{}) :
                                      wxString{};
        carryForward(L"variables.group", dlg.GetGroupVariable(), oldGroupName);

        // cache stipple shape and image settings for round-tripping
        const auto shapeStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(dlg.GetStippleShape());
        if (shapeStr)
            {
            plot->SetPropertyTemplate(L"stipple-shape", shapeStr.value());
            }
        plot->SetPropertyTemplate(L"stipple-shape-color",
                                  dlg.GetStippleShapeColor().GetAsString(wxC2S_HTML_SYNTAX));
        if (!dlg.GetImagePaths().empty())
            {
            wxString paths;
            for (size_t idx = 0; idx < dlg.GetImagePaths().GetCount(); ++idx)
                {
                if (!paths.empty())
                    {
                    paths += L"\t";
                    }
                paths += dlg.GetImagePaths()[idx];
                }
            plot->SetPropertyTemplate(L"image-paths", paths);
            }
        if (dlg.IsImageCustomSizeEnabled())
            {
            plot->SetPropertyTemplate(L"image-width", std::to_wstring(dlg.GetImageWidth()));
            plot->SetPropertyTemplate(L"image-height", std::to_wstring(dlg.GetImageHeight()));
            }
            {
            const auto resizeStr = Wisteria::ReportEnumConvert::ConvertResizeMethodToString(
                dlg.GetImageResizeMethod());
            if (resizeStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-resize-method", resizeStr.value());
                }
            }
            {
            const auto effectStr =
                Wisteria::ReportEnumConvert::ConvertImageEffectToString(dlg.GetImageEffect());
            if (effectStr.has_value())
                {
                plot->SetPropertyTemplate(L"image-effect", effectStr.value());
                }
            }
        plot->SetPropertyTemplate(
            L"image-stitch", (dlg.GetImageStitchDirection() == Wisteria::Orientation::Vertical) ?
                                 L"vertical" :
                                 L"horizontal");

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertLikertChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLikertDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto lkSvg = wxGetApp().GetResourceManager().GetSVG(L"likert7.svg");
    if (lkSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(lkSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        const auto& questions = dlg.GetQuestionVariables();
        const auto dataset = dlg.GetSelectedDataset();
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());

        auto surveyFormat =
            Wisteria::Graphs::LikertChart::DeduceScale(dataset, questions, groupCol);

        // if a group variable is selected, upgrade to categorized variant
        if (groupCol.has_value())
            {
            using LF = Wisteria::Graphs::LikertChart::LikertSurveyQuestionFormat;
            switch (surveyFormat)
                {
            case LF::TwoPoint:
                surveyFormat = LF::TwoPointCategorized;
                break;
            case LF::ThreePoint:
                surveyFormat = LF::ThreePointCategorized;
                break;
            case LF::FourPoint:
                surveyFormat = LF::FourPointCategorized;
                break;
            case LF::FivePoint:
                surveyFormat = LF::FivePointCategorized;
                break;
            case LF::SixPoint:
                surveyFormat = LF::SixPointCategorized;
                break;
            case LF::SevenPoint:
                surveyFormat = LF::SevenPointCategorized;
                break;
            default:
                break;
                }
            }

        auto plot = std::make_shared<Wisteria::Graphs::LikertChart>(
            canvas, surveyFormat, dlg.GetNegativeColor(), dlg.GetPositiveColor(),
            dlg.GetNeutralColor(), dlg.GetNoResponseColor());
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        plot->SetData(dataset, questions, groupCol);
        dlg.ApplyAxisOverrides(*plot);

        plot->ShowResponseCounts(dlg.GetShowResponseCounts());
        plot->ShowPercentages(dlg.GetShowPercentages());
        plot->ShowSectionHeaders(dlg.GetShowSectionHeaders());
        plot->SetBarSizesToRespondentSize(dlg.GetAdjustBarWidths());
        plot->SetPositiveHeader(dlg.GetPositiveLabel());
        plot->SetNegativeHeader(dlg.GetNegativeLabel());
        plot->SetNoResponseHeader(dlg.GetNoResponseLabel());
        for (const auto& bracket : dlg.GetQuestionsBrackets())
            {
            plot->AddQuestionsBracket(bracket);
            }

        // cache dataset and variable names for round-tripping
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        for (size_t i = 0; i < questions.size(); ++i)
            {
            plot->SetPropertyTemplate(L"variables.questions[" + std::to_wstring(i) + L"]",
                                      questions[i]);
            }
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
void WisteriaView::EditLikertChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertLikertDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Likert Chart"),
                                      wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                      Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto lkSvg = wxGetApp().GetResourceManager().GetSVG(L"likert7.svg");
    if (lkSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(lkSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        const auto& questions = dlg.GetQuestionVariables();
        const auto dataset = dlg.GetSelectedDataset();
        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());

        auto surveyFormat =
            Wisteria::Graphs::LikertChart::DeduceScale(dataset, questions, groupCol);

        // if a group variable is selected, upgrade to categorized variant
        if (groupCol.has_value())
            {
            using LF = Wisteria::Graphs::LikertChart::LikertSurveyQuestionFormat;
            switch (surveyFormat)
                {
            case LF::TwoPoint:
                surveyFormat = LF::TwoPointCategorized;
                break;
            case LF::ThreePoint:
                surveyFormat = LF::ThreePointCategorized;
                break;
            case LF::FourPoint:
                surveyFormat = LF::FourPointCategorized;
                break;
            case LF::FivePoint:
                surveyFormat = LF::FivePointCategorized;
                break;
            case LF::SixPoint:
                surveyFormat = LF::SixPointCategorized;
                break;
            case LF::SevenPoint:
                surveyFormat = LF::SevenPointCategorized;
                break;
            default:
                break;
                }
            }

        auto plot = std::make_shared<Wisteria::Graphs::LikertChart>(
            canvas, surveyFormat, dlg.GetNegativeColor(), dlg.GetPositiveColor(),
            dlg.GetNeutralColor(), dlg.GetNoResponseColor());
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        plot->SetData(dataset, questions, groupCol);
        dlg.ApplyAxisOverrides(*plot);

        plot->ShowResponseCounts(dlg.GetShowResponseCounts());
        plot->ShowPercentages(dlg.GetShowPercentages());
        plot->ShowSectionHeaders(dlg.GetShowSectionHeaders());
        plot->SetBarSizesToRespondentSize(dlg.GetAdjustBarWidths());
        plot->SetPositiveHeader(dlg.GetPositiveLabel());
        plot->SetNegativeHeader(dlg.GetNegativeLabel());
        plot->SetNoResponseHeader(dlg.GetNoResponseLabel());
        for (const auto& bracket : dlg.GetQuestionsBrackets())
            {
            plot->AddQuestionsBracket(bracket);
            }

        // carry forward property templates
        plot->SetPropertyTemplate(L"dataset", dlg.GetSelectedDatasetName());
        for (size_t i = 0; i < questions.size(); ++i)
            {
            plot->SetPropertyTemplate(L"variables.questions[" + std::to_wstring(i) + L"]",
                                      questions[i]);
            }
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
                const auto legendRow = (oldSide == Wisteria::Side::Top)    ? graphRow - 1 :
                                       (oldSide == Wisteria::Side::Bottom) ? graphRow + 1 :
                                                                             graphRow;
                const auto legendCol = (oldSide == Wisteria::Side::Left)  ? graphCol - 1 :
                                       (oldSide == Wisteria::Side::Right) ? graphCol + 1 :
                                                                            graphCol;
                canvas->SetFixedObject(legendRow, legendCol, nullptr);
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
                             graphRow, graphCol, legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertHeatMap([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertHeatMapDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto hmSvg = wxGetApp().GetResourceManager().GetSVG(L"heatmap.svg");
    if (hmSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(hmSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::HeatMap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol,
                      static_cast<size_t>(dlg.GetGroupColumnCount()));
        dlg.ApplyAxisOverrides(*plot);

        plot->ShowGroupHeaders(dlg.GetShowGroupHeaders());
        plot->SetGroupHeaderPrefix(dlg.GetGroupHeaderPrefix());

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
void WisteriaView::EditHeatMap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                               const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertHeatMapDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Heat Map"),
                                       wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                       Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto hmSvg = wxGetApp().GetResourceManager().GetSVG(L"heatmap.svg");
    if (hmSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(hmSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::HeatMap>(canvas);
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            dlg.GetGroupVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(dlg.GetGroupVariable());
        plot->SetData(dlg.GetSelectedDataset(), dlg.GetContinuousVariable(), groupCol,
                      static_cast<size_t>(dlg.GetGroupColumnCount()));
        dlg.ApplyAxisOverrides(*plot);

        plot->ShowGroupHeaders(dlg.GetShowGroupHeaders());
        plot->SetGroupHeaderPrefix(dlg.GetGroupHeaderPrefix());

        // carry forward property templates, preserving {{placeholders}}
        const auto* oldHeatMap = dynamic_cast<const Wisteria::Graphs::HeatMap*>(&graph);
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
                     oldHeatMap != nullptr ? oldHeatMap->GetContinuousColumnName() : wxString{});
        const auto oldGroupName = (oldHeatMap != nullptr) ?
                                      oldHeatMap->GetGroupColumnName().value_or(wxString{}) :
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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
    const auto wlSvg = wxGetApp().GetResourceManager().GetSVG(L"sparkline.svg");
    if (wlSvg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(wlSvg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
        dlg.ApplyAxisOverrides(*plot);

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
    dlg.LoadFromGraph(graph);

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
        dlg.ApplyAxisOverrides(*plot);

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
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWaffleChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWaffleChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    const auto waffleBmp = wxGetApp().ReadSvgIcon(L"waffle.svg");
    if (waffleBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(waffleBmp);
        dlg.SetIcon(icon);
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WaffleChart>(
            canvas, dlg.GetShapes(), dlg.GetGridRounding(), dlg.GetRowCount());
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);

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
void WisteriaView::EditWaffleChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol)
    {
    Wisteria::UI::InsertWaffleChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Waffle Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    const auto waffleBmp = wxGetApp().ReadSvgIcon(L"waffle.svg");
    if (waffleBmp.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(waffleBmp);
        dlg.SetIcon(icon);
        }
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = std::make_shared<Wisteria::Graphs::WaffleChart>(
            canvas, dlg.GetShapes(), dlg.GetGridRounding(), dlg.GetRowCount());
        dlg.ApplyGraphOptions(*plot);
        dlg.ApplyPageOptions(*plot);
        dlg.ApplyAxisOverrides(*plot);

        // clear old legend if present
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
        Wisteria::GraphItems::GraphItemInfo{ dlg.GetLabelText() });
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
    UpdateCanvas(canvas);

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
    dlg.LoadFromLabel(label);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    auto newLabel = std::make_shared<Wisteria::GraphItems::Label>(
        Wisteria::GraphItems::GraphItemInfo{ dlg.GetLabelText() });
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
    UpdateCanvas(canvas);

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
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
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
        doc->GetFilename().empty() ? wxString{} : wxFileName{ doc->GetFilename() }.GetPathWithSep();

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
    else if (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical)
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

    // cache import paths for round-tripping
    if (paths.GetCount() == 1)
        {
        image->SetPropertyTemplate(L"image-import.path", doc->MakeRelativePath(paths[0]));
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
            joined += doc->MakeRelativePath(paths[idx]);
            }
        image->SetPropertyTemplate(L"image-import.paths", joined);
        image->SetPropertyTemplate(L"image-import.stitch",
                                   (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical) ?
                                       L"vertical" :
                                       L"horizontal");
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
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditImage(Wisteria::GraphItems::Image& image, Wisteria::Canvas* canvas,
                             const size_t imageRow, const size_t imageCol)
    {
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

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
    dlg.LoadFromImage(image);

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
        doc->GetFilename().empty() ? wxString{} : wxFileName{ doc->GetFilename() }.GetPathWithSep();

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
    else if (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical)
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

    // cache import paths for round-tripping
    if (paths.GetCount() == 1)
        {
        newImage->SetPropertyTemplate(L"image-import.path", doc->MakeRelativePath(paths[0]));
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
            joined += doc->MakeRelativePath(paths[idx]);
            }
        newImage->SetPropertyTemplate(L"image-import.paths", joined);
        newImage->SetPropertyTemplate(
            L"image-import.stitch", (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical) ?
                                        L"vertical" :
                                        L"horizontal");
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
    UpdateCanvas(canvas);

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

    UpdateCanvas(canvas);

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
    dlg.LoadFromShape(shape);

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

    UpdateCanvas(canvas);

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
    dlg.LoadFromFillableShape(shape);

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

    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::UpdateCanvas(Wisteria::Canvas* canvas)
    {
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();
    }

//-------------------------------------------
size_t WisteriaView::GetDatasetIconFromName(const wxString& name)
    {
    if (const auto foundPos = GetReportBuilder().GetDatasetPivotOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetPivotOptions().cend())
        {
        if (foundPos->second.m_type == Wisteria::ReportBuilder::PivotType::Wider)
            {
            return DATA_PIVOT_WIDER_ICON_INDEX;
            }
        else
            {
            return DATA_PIVOT_LONGER_ICON_INDEX;
            }
        }
    if (const auto foundPos = GetReportBuilder().GetDatasetSubsetOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetSubsetOptions().cend())
        {
        return DATA_SUBSET_ICON_INDEX;
        }
    if (const auto foundPos = GetReportBuilder().GetDatasetMergeOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetMergeOptions().cend())
        {
        return DATA_JOIN_ICON_INDEX;
        }
    return DATA_ICON_INDEX;
    }
