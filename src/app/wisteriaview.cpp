///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriaview.h"
#include "../reporting/pdfreportprintout.h"
#include "../reporting/reportprintout.h"
#include "../reporting/svgreportprintout.h"
#include "../ui/controls/datasetgridtable.h"
#include "../ui/dialogs/datasetimportdlg.h"
#include "../ui/dialogs/editors/insert_bullet_chart_dlg.h"
#include "../ui/dialogs/editors/insert_nightingale_rose_chart_dlg.h"
#include "../ui/dialogs/editors/insert_waterfallchart_dlg.h"
#include "../ui/dialogs/editors/insertboxplotdlg.h"
#include "../ui/dialogs/editors/insertbubbleplotdlg.h"
#include "../ui/dialogs/editors/insertcandlestickplotdlg.h"
#include "../ui/dialogs/editors/insertcatbarchartdlg.h"
#include "../ui/dialogs/editors/insertchernoffdlg.h"
#include "../ui/dialogs/editors/insertchoroplethmapdlg.h"
#include "../ui/dialogs/editors/insertcommonaxisdlg.h"
#include "../ui/dialogs/editors/insertduboisspiralchartdlg.h"
#include "../ui/dialogs/editors/insertfunnelchartdlg.h"
#include "../ui/dialogs/editors/insertganttchartdlg.h"
#include "../ui/dialogs/editors/insertheatmapdlg.h"
#include "../ui/dialogs/editors/inserthistogramdlg.h"
#include "../ui/dialogs/editors/insertimgdlg.h"
#include "../ui/dialogs/editors/insertitemdlg.h"
#include "../ui/dialogs/editors/insertkpicarddlg.h"
#include "../ui/dialogs/editors/insertlabeldlg.h"
#include "../ui/dialogs/editors/insertlikertdlg.h"
#include "../ui/dialogs/editors/insertlineplotdlg.h"
#include "../ui/dialogs/editors/insertlrroadmapdlg.h"
#include "../ui/dialogs/editors/insertmultiserieslineplotdlg.h"
#include "../ui/dialogs/editors/insertpagedlg.h"
#include "../ui/dialogs/editors/insertpictographdlg.h"
#include "../ui/dialogs/editors/insertpiechartdlg.h"
#include "../ui/dialogs/editors/insertproconroadmapdlg.h"
#include "../ui/dialogs/editors/insertracetrackchartdlg.h"
#include "../ui/dialogs/editors/insertsankeydiagramdlg.h"
#include "../ui/dialogs/editors/insertscalechartdlg.h"
#include "../ui/dialogs/editors/insertscatterplotdlg.h"
#include "../ui/dialogs/editors/insertshapedlg.h"
#include "../ui/dialogs/editors/insertstemandleafdlg.h"
#include "../ui/dialogs/editors/inserttabledlg.h"
#include "../ui/dialogs/editors/insertwafflechartdlg.h"
#include "../ui/dialogs/editors/insertwcurvedlg.h"
#include "../ui/dialogs/editors/insertwilmarthbridgeplotdlg.h"
#include "../ui/dialogs/editors/insertwlsparklinedlg.h"
#include "../ui/dialogs/editors/insertwordclouddlg.h"
#include "../ui/dialogs/editors/joindlg.h"
#include "../ui/dialogs/editors/pivotlongerdlg.h"
#include "../ui/dialogs/editors/pivotwiderrdlg.h"
#include "../ui/dialogs/editors/subsetdlg.h"
#include "../ui/dialogs/pdfexportdlg.h"
#include "../ui/dialogs/pptxexportdlg.h"
#include "../ui/dialogs/projectsettingsdlg.h"
#include "../ui/dialogs/svgexportdlg.h"
#include "wisteriaapp.h"
#include "wisteriadoc.h"
#include <array>
#include <wx/filename.h>
#include <wx/rearrangectrl.h>
#include <wx/wupdlock.h>

wxIMPLEMENT_DYNAMIC_CLASS(WisteriaView, wxView);

class PastePlacementDlg final : public Wisteria::UI::InsertItemDlg
    {
  public:
    PastePlacementDlg(Wisteria::Canvas* canvas, wxWindow* parent)
        : Wisteria::UI::InsertItemDlg(canvas, nullptr, parent, _(L"Paste Item"), wxID_ANY,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                      Wisteria::UI::InsertItemDlg::EditMode::Insert,
                                      Wisteria::UI::ItemDlgIncludeCanvasPlacement)
        {
        CreateControls();
        CreatePageOptionsPage();
        FinalizeControls();
        TransferDataToWindow();
        SetMinSize(GetSize());
        Centre();
        }
    };

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

    const std::array<wxAcceleratorEntry, 6> entries = {
        wxAcceleratorEntry(wxACCEL_CTRL, L'O', wxID_OPEN),
        wxAcceleratorEntry(wxACCEL_CTRL, L'S', ID_SAVE_PROJECT),
        wxAcceleratorEntry(wxACCEL_CTRL, L'P', wxID_PRINT),
        wxAcceleratorEntry(wxACCEL_CTRL, L'C', wxID_COPY),
        wxAcceleratorEntry(wxACCEL_CTRL, L'V', wxID_PASTE),
        wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F5, ID_REFRESH_ALL)
    };
    m_frame->SetAcceleratorTable(wxAcceleratorTable(entries.size(), entries.data()));

    // set the icon
    const auto appSvg = wxGetApp().GetResourceManager().GetSVG(L"wisteria.svg");
    if (appSvg.IsOk())
        {
        wxIcon appIcon;
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
    m_datasetButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(wxDocChildFrame::FindWindowById(ID_DATASET_BUTTONBAR));
    m_graphButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(wxDocChildFrame::FindWindowById(ID_GRAPH_BUTTONBAR));
    m_pagesButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(wxDocChildFrame::FindWindowById(ID_PAGES_BUTTONBAR));
    m_objectsButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(wxDocChildFrame::FindWindowById(ID_OBJECTS_BUTTONBAR));
    m_sourcesButtonBar =
        dynamic_cast<wxRibbonButtonBar*>(wxDocChildFrame::FindWindowById(ID_SOURCES_BUTTONBAR));

    // build the graph dropdown menus
    BuildGraphMenus();

    // bind sidebar click event
    m_sideBar->Bind(Wisteria::UI::wxEVT_SIDEBAR_CLICK, &WisteriaView::OnSidebarClick, this);

    // log button: show main frame and activate its embedded log tab
    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED, []([[maybe_unused]] wxRibbonButtonBarEvent&)
        { wxGetApp().GetMainFrameEx()->ActivateLogTab(); }, ID_VIEW_LOG_REPORT);

    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaApp::OnOpenDropdown, &wxGetApp(),
                  wxID_OPEN);

    // bind copy/paste (route accelerator events to the active canvas)
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnCopyItem, this, wxID_COPY);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnPasteItem, this, wxID_PASTE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnCopyItem, this, wxID_COPY);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPasteItem, this, wxID_PASTE);

    // bind print button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPrintAll, this, wxID_PRINT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnPrintAll, this, wxID_PRINT);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPrintSetup, this, ID_PRINT_SETUP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnPrintSetup, this, ID_PRINT_SETUP);

    // bind SVG export button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnSvgExport, this, ID_SVG_EXPORT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnSvgExport, this, ID_SVG_EXPORT);

    // bind PDF export button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPdfExport, this, ID_PDF_EXPORT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnPdfExport, this, ID_PDF_EXPORT);

    // bind PowerPoint export button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPptxExport, this, ID_PPTX_EXPORT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnPptxExport, this, ID_PPTX_EXPORT);

    // bind project settings button
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnProjectSettings, this,
                  ID_PROJECT_SETTINGS);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnProjectSettings, this, ID_PROJECT_SETTINGS);

    // bind save button
    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event) { GetDocument()->Save(); },
        ID_SAVE_PROJECT);
    m_frame->Bind(
        wxEVT_MENU,
        [this]([[maybe_unused]]
               wxCommandEvent& event) { GetDocument()->Save(); },
        ID_SAVE_PROJECT);

    // bind refresh-all button
    m_frame->Bind(
        wxEVT_RIBBONBUTTONBAR_CLICKED,
        [this]([[maybe_unused]]
               wxCommandEvent& event) { ReloadProject(); },
        ID_REFRESH_ALL);
    m_frame->Bind(
        wxEVT_MENU,
        [this]([[maybe_unused]]
               wxCommandEvent& event) { ReloadProject(); },
        ID_REFRESH_ALL);

    // bind insert and edit dataset buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertDataset, this,
                  ID_INSERT_DATASET);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditDataset, this,
                  ID_EDIT_DATASET);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnDeleteDataset, this,
                  ID_DELETE_DATASET);

    // bind pivot and subset buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotWider, this, ID_PIVOT_WIDER);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnPivotLonger, this,
                  ID_PIVOT_LONGER);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnSubsetDataset, this,
                  ID_SUBSET_DATASET);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnJoinDataset, this,
                  ID_JOIN_DATASET);

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
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnRearrangePages, this,
                  ID_REARRANGE_PAGES);

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
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertChoroplethMap, this, ID_NEW_CHOROPLETH_MAP);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWLSparkline, this, ID_NEW_WIN_LOSS_SPARKLINE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertStemAndLeaf, this, ID_NEW_STEMANDLEAF);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertPieChart, this, ID_NEW_PIECHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWaffleChart, this, ID_NEW_WAFFLE_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertRaceTrackChart, this, ID_NEW_RACETRACK_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertNightingaleRoseChart, this,
                  ID_NEW_NIGHTINGALE_ROSE_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertDuBoisSpiralChart, this,
                  ID_NEW_DUBOIS_SPIRAL_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertPictograph, this, ID_NEW_PICTOGRAPH);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertBulletChart, this, ID_NEW_BULLET_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWaterfallChart, this, ID_NEW_WATERFALL_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertFunnelChart, this, ID_NEW_FUNNEL_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertWilmarthBridgePlot, this,
                  ID_NEW_WILMARTH_BRIDGE_PLOT);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertCatBarChart, this, ID_NEW_BARCHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertScaleChart, this, ID_NEW_SCALE_CHART);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertTable, this, ID_NEW_TABLE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertLabel, this, ID_NEW_LABEL);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertKpiCard, this,
                  ID_NEW_KPI_CARD);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertImage, this, ID_NEW_IMAGE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertShape, this, ID_NEW_SHAPE);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertCommonAxis, this,
                  ID_NEW_COMMON_AXIS);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnInsertSpacer, this,
                  ID_NEW_SPACER);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_DROPDOWN_CLICKED, &WisteriaView::OnDividerDropdown, this,
                  ID_NEW_DIVIDER);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertDivider, this,
                  ID_NEW_DIVIDER_HORIZONTAL_SINGLE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertDivider, this,
                  ID_NEW_DIVIDER_HORIZONTAL_DOUBLE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertDivider, this, ID_NEW_DIVIDER_VERTICAL_SINGLE);
    m_frame->Bind(wxEVT_MENU, &WisteriaView::OnInsertDivider, this, ID_NEW_DIVIDER_VERTICAL_DOUBLE);

    // bind edit/delete item buttons
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnEditItem, this, ID_EDIT_ITEM);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnDeleteItem, this, ID_DELETE_ITEM);
    m_frame->Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &WisteriaView::OnGoToDatasource, this,
                  ID_GOTO_DATASOURCE);

    // bind DELETE key to delete selected item
    m_frame->Bind(wxEVT_CHAR_HOOK,
                  [this](wxKeyEvent& evt)
                  {
                      if (evt.GetKeyCode() == WXK_DELETE || evt.GetKeyCode() == WXK_NUMPAD_DELETE ||
                          evt.GetKeyCode() == WXK_BACK)
                          {
                          if (GetActiveCanvas() != nullptr)
                              {
                              wxCommandEvent cmd;
                              OnDeleteItem(cmd);
                              return;
                              }
                          }
                      evt.Skip();
                  });

    // bind canvas double-click to edit the selected item
    m_frame->Bind(wxEVT_WISTERIA_CANVAS_DCLICK, &WisteriaView::OnCanvasDClick, this);

    // refresh ribbon button states whenever the canvas selection changes
    m_frame->Bind(wxEVT_WISTERIA_CANVAS_SELECTION_CHANGED,
                  [this]([[maybe_unused]]
                         wxCommandEvent& evt) { UpdateGraphButtonStates(); });

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
        // when Open was used to pick a data file, that path seeds the import;
        // otherwise prompt for one here
        wxString filePath = wxGetApp().GetPendingDatasetImportPath();
        if (filePath.empty())
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

            filePath = fileDlg.GetPath();
            }

        Wisteria::UI::DatasetImportDlg importDlg(wxGetApp().GetMainFrame(), filePath);
        if (importDlg.ShowModal() != wxID_OK)
            {
            m_frame->Destroy();
            m_frame = nullptr;
            return false;
            }

        try
            {
            // the user may have browsed to a different file from within the import dialog
            initialFilePath = importDlg.GetFilePath();
            initialColumnInfo = importDlg.GetColumnPreviewInfo();
            initialImportInfo = importDlg.GetImportInfo();
            initialWorksheet = importDlg.GetWorksheet();
            initialDataset = std::make_shared<Wisteria::Data::Dataset>();
            initialDataset->Import(initialFilePath, initialImportInfo, initialWorksheet);
            initialDatasetName = wxFileName{ initialFilePath }.GetName();
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

    LoadProject(GetDocument()->GetFilename());

    // seed the per-project export options from the global app settings when the
    // project file did not carry its own (new projects, or ones saved before these
    // options existed)
    auto& appSettings = wxGetApp().GetAppSettings();
    if (!GetReportBuilder().HasLoadedSvgExportOptions())
        {
        auto& svgOpts = GetReportBuilder().GetSvgExportOptions();
        svgOpts = appSettings->GetSvgExportOptions();
        svgOpts.m_paperId = appSettings->GetPaperId();
        svgOpts.m_paperOrientation =
            static_cast<wxPrintOrientation>(appSettings->GetPrintOrientation());
        }
    if (!GetReportBuilder().HasLoadedPdfExportOptions())
        {
        auto& pdfOpts = GetReportBuilder().GetPdfExportOptions();
        pdfOpts.m_paperSize = appSettings->GetPaperId();
        pdfOpts.m_paperOrientation =
            static_cast<wxPrintOrientation>(appSettings->GetPrintOrientation());
        }
    if (!GetReportBuilder().HasLoadedPowerPointExportOptions())
        {
        GetReportBuilder().GetPowerPointExportOptions() = appSettings->GetPowerPointExportOptions();
        }

    if (initialDataset != nullptr)
        {
        AddDatasetToProject(initialDataset, initialDatasetName, initialColumnInfo,
                            { initialFilePath, wxString{}, initialWorksheet, initialFullColumnInfo,
                              initialImportInfo });
        m_reportBuilder.GetDatasetTransformOptions()[initialDatasetName].m_columnNamesSort =
            initialImportInfo.GetColumnNamesSort();
        AddPageToProject(1, 1, wxString{});
        // adding the page selects it in the sidebar; re-select the dataset instead
        // so the user lands on their imported data rather than the empty page
        m_sideBar->SelectSubItem(0, 0);
        }

    UpdateDatasetButtonStates();
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
bool WisteriaView::LoadProject(const wxString& filename)
    {
    // set up sidebar image list from the app's persistent list
    m_sideBar->SetImageList(wxGetApp().GetProjectSideBarImageList());

    // helper uses member function ApplyColumnHeaderIcons()

    // IDs for sidebar items
    const wxWindowID dataFolderId = wxNewId();
    wxWindowID nextId = wxNewId();

    bool loadedClean = true;

    if (!filename.empty())
        {
        // offer to locate any datasets that are no longer where the project expects
        m_reportBuilder.SetMissingDatasetResolver(
            [this](const wxString& missingPath) -> std::optional<wxString>
            {
                if (wxMessageBox(wxString::Format(_(L"Dataset not found.\n\n"
                                                    "'%s' could not be found.\n\n"
                                                    "Do you wish to look for it?"),
                                                  missingPath),
                                 _(L"Dataset Not Found"), wxYES_NO | wxICON_QUESTION | wxCENTRE,
                                 m_workArea) != wxYES)
                    {
                    return std::nullopt;
                    }
                wxFileDialog fileDlg(m_workArea, _(L"Locate Dataset"),
                                     wxFileName{ GetDocument()->GetFilename() }.GetPath(),
                                     wxFileName{ missingPath }.GetFullName(),
                                     Wisteria::Data::Dataset::GetDataFileFilter(),
                                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
                if (fileDlg.ShowModal() != wxID_OK)
                    {
                    return std::nullopt;
                    }
                return fileDlg.GetPath();
            });

            {
            const wxBusyCursor busyCursor;
            // load the JSON configuration file
            m_pages = m_reportBuilder.LoadConfigurationFile(filename, m_workArea);
            m_reportBuilder.SetMissingDatasetResolver({});
            for (auto* page : m_pages)
                {
                ApplyGlobalPrintSettings(page);
                page->FitToPageWhenPrinting(true);
                page->SetSizeFromPaperSize();
                page->MaintainAspectRatio(true);
                }
            }

        const auto pendingErrors = m_reportBuilder.UnloadPendingErrorMessages();
        loadedClean = pendingErrors.empty();
        for (const auto& errMsg : pendingErrors)
            {
            wxMessageBox(errMsg.m_message, errMsg.m_title, wxOK | wxICON_WARNING | wxCENTRE,
                         m_workArea);
            }
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
        // add datasets as subitems under "Data" in the order they were loaded
        const auto& importOpts = m_reportBuilder.GetDatasetImportOptions();
        const auto& allDatasets = m_reportBuilder.GetDatasets();
        for (const auto& dsName : m_reportBuilder.GetDatasetInsertionOrder())
            {
            if (dsName.empty())
                {
                continue;
                }
            const auto dsIt = allDatasets.find(dsName);
            if (dsIt == allDatasets.cend())
                {
                continue;
                }
            const auto& dataset = dsIt->second;
            const wxWindowID dsId = nextId;
            nextId = wxNewId();

            const auto optIt = importOpts.find(dsName);
            const auto& colInfo = (optIt != importOpts.cend()) ?
                                      optIt->second.m_columnPreviewInfo :
                                      Wisteria::Data::Dataset::ColumnPreviewInfo{};
            auto* table = colInfo.empty() ? new Wisteria::UI::DatasetGridTable(dataset) :
                                            new Wisteria::UI::DatasetGridTable(dataset, colInfo);

            // apply currency symbols to the continuous columns
            size_t contIdx{ 0 };
            for (const auto& col : colInfo)
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

            const wxString displayName = !canvas->GetNameTemplate().empty() ?
                                             canvas->GetNameTemplate() :
                                             wxString::Format(_(L"Page %zu"), pageNum);
            canvas->SetLabel(displayName);
            m_sideBar->InsertItem(m_sideBar->GetFolderCount(), displayName, pageId,
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

    return loadedClean;
    }

//-------------------------------------------
void WisteriaView::TearDownProject()
    {
    if (m_constantsGrid != nullptr)
        {
        m_constantsGrid->Unbind(wxEVT_GRID_CELL_CHANGED, &WisteriaView::OnConstantEdited, this);
        }

    // detach the work-area children from the sizer, then destroy them
    if (auto* sizer = m_workArea->GetSizer(); sizer != nullptr)
        {
        sizer->Clear(false);
        }
    for (auto* window : m_workWindows.GetWindows())
        {
        if (window != nullptr)
            {
            window->Destroy();
            }
        }
    m_workWindows.Clear();
    m_constantsGrid = nullptr;
    m_pages.clear();

    m_sideBar->DeleteAllFolders();

    // replace the builder with a fresh one
    m_reportBuilder = Wisteria::ReportBuilder{};
    }

//-------------------------------------------
void WisteriaView::ReloadProject()
    {
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        return;
        }

    // Serialize next to the real project file so dataset paths (written relative to
    // the output file's directory) resolve the same way on reload. Never-saved
    // projects fall back to the system temp directory.
    const wxString projectPath = doc->GetFilename();
    const wxString tempPrefix = projectPath.empty() ?
                                    wxString{ L"wisteria_reload" } :
                                    wxFileName{ projectPath }.GetPathWithSep() + L"wisteria_reload";
    const wxString tempPath = wxFileName::CreateTempFileName(tempPrefix);
    if (tempPath.empty() || !doc->SaveProject(tempPath))
        {
        wxMessageBox(_(L"Unable to reload the project after the change."), _(L"Reload Error"),
                     wxOK | wxICON_ERROR, m_frame);
        return;
        }

    // remember what the user was looking at
    const wxString selectedLabel = m_sideBar->GetSelectedLabel();
    const auto selectedFolder = m_sideBar->GetSelectedFolder();
    const int sashPos = (m_splitter != nullptr) ? m_splitter->GetSashPosition() : 0;

    bool reloadedClean{ false };
        {
        wxWindowUpdateLocker noUpdate{ m_frame };

        TearDownProject();
        reloadedClean = LoadProject(tempPath);

        wxRemoveFile(tempPath);

        // restore the previous selection
        if (const auto [folderIdx, subIdx] = m_sideBar->FindSubItem(selectedLabel);
            folderIdx.has_value() && subIdx.has_value())
            {
            m_sideBar->SelectSubItem(folderIdx.value(), subIdx.value());
            }
        else if (selectedFolder.has_value() && selectedFolder.value() < m_sideBar->GetFolderCount())
            {
            m_sideBar->SelectFolder(selectedFolder.value());
            }

        if (m_splitter != nullptr && sashPos > 0)
            {
            m_splitter->SetSashPosition(sashPos);
            }
        }

    // Only flag the document dirty when the rebuild came back intact. A degraded
    // reload leaves an in-memory project that is worse than what is on disk, so
    // marking it modified would invite a reflexive save over the good file.
    if (reloadedClean)
        {
        doc->Modify(true);
        }
    else
        {
        wxMessageBox(_(L"The project could not be fully reloaded after the change.\n\n"
                       "It has not been marked as modified, so the file on disk is unchanged. "
                       "Close the project without saving to keep that version."),
                     _(L"Reload Incomplete"), wxOK | wxICON_WARNING, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnSidebarClick(const wxCommandEvent& event)
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
    UpdateDatasetButtonStates();
    UpdateGraphButtonStates();
    }

//-------------------------------------------
void WisteriaView::OnPrintSetup([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    wxPageSetupDialogData pageSetupData;
    wxPrintData printData = m_pages.front()->GetPrinterSettings();

    // apply global settings
    auto& settings = wxGetApp().GetAppSettings();
    printData.SetOrientation(static_cast<wxPrintOrientation>(settings->GetPrintOrientation()));
    printData.SetPaperId(settings->GetPaperId());

    pageSetupData.SetPrintData(printData);

    wxPageSetupDialog dialog(m_frame, &pageSetupData);
    if (dialog.ShowModal() == wxID_OK)
        {
        wxPrintData updatedData = dialog.GetPageSetupData().GetPrintData();
        settings->SetPrintOrientation(updatedData.GetOrientation());
        settings->SetPaperId(updatedData.GetPaperId());
        settings->SaveSettingsFile();
        for (auto* docBase : wxGetApp().GetDocManager()->GetDocuments())
            {
            if (auto* view =
                    dynamic_cast<WisteriaView*>(dynamic_cast<wxDocument*>(docBase)->GetFirstView());
                view != nullptr)
                {
                view->RefreshPagePrintSettings();
                }
            }
        }
    }

//-------------------------------------------
void WisteriaView::OnCopyItem([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }
    wxCommandEvent copyEvent(wxEVT_MENU, wxID_COPY);
    canvas->GetEventHandler()->ProcessEvent(copyEvent);
    }

//-------------------------------------------
void WisteriaView::OnPasteItem([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }
    std::shared_ptr<Wisteria::GraphItems::GraphItemBase> canvasItem{
        Wisteria::Canvas::GetLabelClipboard()
    };
    if (canvasItem == nullptr)
        {
        canvasItem = Wisteria::Canvas::GetImageClipboard();
        }
    if (canvasItem == nullptr)
        {
        wxMessageBox(_(L"No item on the clipboard."), _(L"Paste"), wxOK | wxICON_INFORMATION);
        return;
        }

    PastePlacementDlg dlg(canvas, canvas);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvasItem->SetScaling(1.0);
    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), canvasItem);

    // re-compute row proportions now that the destination row has content
    // (an empty row would otherwise be collapsed to zero height by CalcRowDimensions)
    canvas->ZoomReset();
    canvas->CalcRowDimensions();
    wxGCDC gdc(canvas);
    canvas->CalcAllSizes(gdc);
    canvas->ResetResizeDelay();
    canvas->SendSizeEvent();
    canvas->Refresh();
    canvas->Update();
    }

//-------------------------------------------
void WisteriaView::OnPrintAll([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    auto printOut =
        std::make_unique<Wisteria::ReportPrintout>(m_pages, m_pages.front()->GetLabel());

    auto& settings = wxGetApp().GetAppSettings();
    wxPrintData printData = m_pages.front()->GetPrinterSettings();
    printData.SetOrientation(static_cast<wxPrintOrientation>(settings->GetPrintOrientation()));
    printData.SetPaperId(settings->GetPaperId());

#if defined(__WXMSW__) || defined(__WXOSX__)
    wxPrinterDC dc(printData);
#else
    wxPostScriptDC dc(printData);
#endif
    printOut->SetUp(dc);

    wxPrinter printer;
    wxPrintDialogData dialogData;
    dialogData.SetPrintData(printData);
    dialogData.SetAllPages(true);
    dialogData.SetFromPage(1);
    dialogData.SetToPage(static_cast<int>(m_pages.size()));

    // Explicitly set print dialog data
    printer.GetPrintDialogData() = dialogData;

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
void WisteriaView::OnSvgExport([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    Wisteria::SVGReportOptions& savedOptions = GetReportBuilder().GetSvgExportOptions();

    wxPrintData printData;
    printData.SetOrientation(savedOptions.m_paperOrientation);
    printData.SetPaperId(savedOptions.m_paperId);

    // use the saved page size if valid, fall back to export's paper size
    const wxSize defaultPageSize = (savedOptions.m_pageSize != wxDefaultSize) ?
                                       savedOptions.m_pageSize :
                                       Wisteria::SVGReportPrintout::GetPaperSizeDIPs(
                                           savedOptions.m_paperId, savedOptions.m_paperOrientation);

    Wisteria::UI::SvgExportDlg sizeDlg(m_frame, defaultPageSize, printData, &savedOptions);
    if (sizeDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    // persist all choices back to project.
    // page size only applies in manual mode, so ignore it under global print settings
    const wxSize newPageSize = sizeDlg.GetPageSize();
    const bool newUseGlobal = sizeDlg.UseGlobalPrintSettings();
    const auto& dlgPrintData = sizeDlg.GetPrintData();
    const bool pageSizeChanged = !newUseGlobal && (savedOptions.m_pageSize != newPageSize);
    const bool changed =
        pageSizeChanged || (savedOptions.m_useGlobalPrintSettings != newUseGlobal) ||
        (savedOptions.m_paperId != dlgPrintData.GetPaperId()) ||
        (savedOptions.m_paperOrientation != dlgPrintData.GetOrientation()) ||
        (savedOptions.m_includeTransitions != sizeDlg.IncludeTransitions()) ||
        (savedOptions.m_includeHighlighting != sizeDlg.IncludeHighlighting()) ||
        (savedOptions.m_includeLayoutOptions != sizeDlg.IncludeLayoutOptions()) ||
        (savedOptions.m_includeDarkModeToggle != sizeDlg.IncludeDarkModeToggle()) ||
        (savedOptions.m_includeSlideshow != sizeDlg.IncludeSlideshow()) ||
        (savedOptions.m_includePageShadow != sizeDlg.IncludePageShadow()) ||
        (savedOptions.m_includeLayerControls != sizeDlg.IncludeLayerControls()) ||
        (savedOptions.m_themeColor != sizeDlg.GetThemeColor()) ||
        (savedOptions.m_layout != sizeDlg.GetLayout());
    if (!newUseGlobal)
        {
        savedOptions.m_pageSize = newPageSize;
        }
    savedOptions.m_useGlobalPrintSettings = newUseGlobal;
    savedOptions.m_paperId = dlgPrintData.GetPaperId();
    savedOptions.m_paperOrientation = dlgPrintData.GetOrientation();
    savedOptions.m_includeTransitions = sizeDlg.IncludeTransitions();
    savedOptions.m_includeHighlighting = sizeDlg.IncludeHighlighting();
    savedOptions.m_includeLayoutOptions = sizeDlg.IncludeLayoutOptions();
    savedOptions.m_includeDarkModeToggle = sizeDlg.IncludeDarkModeToggle();
    savedOptions.m_includeSlideshow = sizeDlg.IncludeSlideshow();
    savedOptions.m_includePageShadow = sizeDlg.IncludePageShadow();
    savedOptions.m_includeLayerControls = sizeDlg.IncludeLayerControls();
    savedOptions.m_themeColor = sizeDlg.GetThemeColor();
    savedOptions.m_layout = sizeDlg.GetLayout();
    if (changed)
        {
        GetDocument()->Modify(true);
        }

    wxFileDialog fileDlg(m_frame, _(L"Export to SVG"), wxString{},
                         GetDocument()->GetUserReadableName(), _(L"SVG files (*.svg)|*.svg"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    // RAII creates the report, maybe_unused is to silence clang-tidy false positive
    [[maybe_unused]]
    Wisteria::SVGReportPrintout svgReport(
        m_pages, Wisteria::SVGReportOptions(fileDlg.GetPath())
                     .PageSize(savedOptions.m_pageSize)
                     .UseGlobalPrintSettings(savedOptions.m_useGlobalPrintSettings)
                     .PaperId(savedOptions.m_paperId)
                     .PaperOrientation(savedOptions.m_paperOrientation)
                     .Transitions(savedOptions.m_includeTransitions)
                     .PageShadow(savedOptions.m_includePageShadow)
                     .Highlighting(savedOptions.m_includeHighlighting)
                     .LayoutOptions(savedOptions.m_includeLayoutOptions)
                     .DarkModeToggle(savedOptions.m_includeDarkModeToggle)
                     .Slideshow(savedOptions.m_includeSlideshow)
                     .LayerControls(savedOptions.m_includeLayerControls)
                     .Layout(savedOptions.m_layout)
                     .ThemeColor(savedOptions.m_themeColor));
    }

//-------------------------------------------
void WisteriaView::OnPdfExport([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    Wisteria::PdfExportOptions& savedPdfOptions = GetReportBuilder().GetPdfExportOptions();
    Wisteria::PdfExportOptions options = savedPdfOptions;
    options.m_title = GetReportBuilder().GetName().empty() ? GetDocument()->GetUserReadableName() :
                                                             GetReportBuilder().GetName();
    options.m_subject = GetReportBuilder().GetSubject();
    options.m_keywords = GetReportBuilder().GetKeywords();

    wxPrintData printData;
    printData.SetPaperId(options.m_paperSize);
    printData.SetOrientation(options.m_paperOrientation);

    Wisteria::UI::PdfExportDlg pdfOptionsDlg(m_frame, printData, options);
    if (pdfOptionsDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    options = pdfOptionsDlg.GetOptions();

    wxFileDialog fileDlg(m_frame, _(L"Export to PDF"), wxString{},
                         GetDocument()->GetUserReadableName(), _(L"PDF files (*.pdf)|*.pdf"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const bool docInfoChanged = (GetReportBuilder().GetName() != options.m_title ||
                                 GetReportBuilder().GetSubject() != options.m_subject ||
                                 GetReportBuilder().GetKeywords() != options.m_keywords);
    if (docInfoChanged)
        {
        GetReportBuilder().SetName(options.m_title);
        GetReportBuilder().SetSubject(options.m_subject);
        GetReportBuilder().SetKeywords(options.m_keywords);
        }

    const bool pdfPaperChanged =
        (savedPdfOptions.m_author != options.m_author) ||
        (savedPdfOptions.m_paperSize != options.m_paperSize) ||
        (savedPdfOptions.m_paperOrientation != options.m_paperOrientation) ||
        (savedPdfOptions.m_compress != options.m_compress);
    if (pdfPaperChanged)
        {
        savedPdfOptions.m_author = options.m_author;
        savedPdfOptions.m_paperSize = options.m_paperSize;
        savedPdfOptions.m_paperOrientation = options.m_paperOrientation;
        savedPdfOptions.m_compress = options.m_compress;
        }

    Wisteria::ReportPDFExport pdfReport(m_pages, fileDlg.GetPath(), options);

    if (docInfoChanged || pdfPaperChanged)
        {
        GetDocument()->Modify(true);
        }
    }

//-------------------------------------------
void WisteriaView::OnPptxExport([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_pages.empty())
        {
        return;
        }

    Wisteria::PowerPointExportOptions& savedOptions =
        GetReportBuilder().GetPowerPointExportOptions();
    Wisteria::PowerPointExportOptions options = savedOptions;
    options.m_title = GetReportBuilder().GetName().empty() ? GetDocument()->GetUserReadableName() :
                                                             GetReportBuilder().GetName();
    options.m_subject = GetReportBuilder().GetSubject();
    options.m_keywords = GetReportBuilder().GetKeywords();

    Wisteria::UI::PptxExportDlg pptxOptionsDlg(m_frame, options);
    if (pptxOptionsDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    options = pptxOptionsDlg.GetOptions();

    wxFileDialog fileDlg(m_frame, _(L"Export to PowerPoint"), wxString{},
                         GetDocument()->GetUserReadableName(),
                         _(L"PowerPoint files (*.pptx)|*.pptx"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const bool docInfoChanged = (GetReportBuilder().GetName() != options.m_title ||
                                 GetReportBuilder().GetSubject() != options.m_subject ||
                                 GetReportBuilder().GetKeywords() != options.m_keywords);
    if (docInfoChanged)
        {
        GetReportBuilder().SetName(options.m_title);
        GetReportBuilder().SetSubject(options.m_subject);
        GetReportBuilder().SetKeywords(options.m_keywords);
        }

    const bool pptxOptionsChanged =
        (savedOptions.m_author != options.m_author) ||
        (savedOptions.m_publisher != options.m_publisher) ||
        (savedOptions.m_slideSize != options.m_slideSize) ||
        (savedOptions.m_customWidthInches != options.m_customWidthInches) ||
        (savedOptions.m_customHeightInches != options.m_customHeightInches) ||
        (savedOptions.m_transition != options.m_transition) ||
        (savedOptions.m_transitionSpeed != options.m_transitionSpeed) ||
        (savedOptions.m_advanceOnClick != options.m_advanceOnClick) ||
        (savedOptions.m_advanceAutomatically != options.m_advanceAutomatically) ||
        (savedOptions.m_advanceSeconds != options.m_advanceSeconds) ||
        (savedOptions.m_loopContinuously != options.m_loopContinuously) ||
        (savedOptions.m_includeAccessibilityNotes != options.m_includeAccessibilityNotes) ||
        (savedOptions.m_includeTitleSlide != options.m_includeTitleSlide) ||
        (savedOptions.m_titleSlideTheme != options.m_titleSlideTheme);
    if (pptxOptionsChanged)
        {
        savedOptions = options;
        }

    Wisteria::ReportPowerPointExport pptxReport(m_pages, fileDlg.GetPath(), options);

    if (docInfoChanged || pptxOptionsChanged)
        {
        GetDocument()->Modify(true);
        }
    }

//-------------------------------------------
void WisteriaView::OnProjectSettings([[maybe_unused]] wxCommandEvent& event)
    {
    Wisteria::UI::ProjectSettingsDlg dlg(m_frame);
    dlg.LoadFromProject(GetReportBuilder());
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    GetReportBuilder().SetName(dlg.GetProjectName());
    GetReportBuilder().SetSubject(dlg.GetSubject());
    GetReportBuilder().SetKeywords(dlg.GetKeywords());
    GetReportBuilder().SetWatermarkLabel(dlg.GetWatermarkLabel());
    GetReportBuilder().SetWatermarkColor(dlg.GetWatermarkColor());

    // apply the new watermark to all pages
    const Wisteria::Canvas::Watermark newWatermark{ dlg.GetWatermarkLabel(),
                                                    dlg.GetWatermarkColor() };
    for (auto* page : m_pages)
        {
        if (page != nullptr)
            {
            page->SetWatermark(newWatermark);
            page->Refresh();
            }
        }

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::ApplyColumnHeaderIcons(const wxGrid* grid, Wisteria::UI::DatasetGridTable* table)
    {
    const auto iconSize = wxSize{ grid->FromDIP(16), grid->FromDIP(16) };

    auto* attrProvider = new Wisteria::UI::DatasetGridAttrProvider();
    for (int col = 0; col < table->GetNumberCols(); ++col)
        {
        wxString svgName;
        switch (table->GetColumnType(col))
            {
        case Wisteria::UI::DatasetGridColumnType::Id:
            [[fallthrough]];
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
void WisteriaView::OnInsertPage([[maybe_unused]] wxCommandEvent& event)
    {
    wxArrayString pageNames;
    for (size_t i = 2; i < m_sideBar->GetFolderCount(); ++i)
        {
        pageNames.Add(m_sideBar->GetFolderText(i));
        }

    Wisteria::UI::InsertPageDlg dlg(nullptr, pageNames, &m_reportBuilder,
                                    dynamic_cast<WisteriaDoc*>(GetDocument()), m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    std::optional<size_t> insertIndex;
    if (dlg.GetRelativePageIndex() != wxNOT_FOUND)
        {
        insertIndex = (dlg.GetInsertPosition() == 0) ?
                          static_cast<size_t>(dlg.GetRelativePageIndex()) :
                          static_cast<size_t>(dlg.GetRelativePageIndex() + 1);
        }

    auto* newCanvas =
        AddPageToProject(dlg.GetRows(), dlg.GetColumns(), dlg.GetPageName(), insertIndex);
    if (newCanvas != nullptr)
        {
        dlg.ApplyGridEdits(newCanvas);
        newCanvas->SetLayer(dlg.GetLayer());
        newCanvas->SetWatermark(
            Wisteria::Canvas::Watermark{ dlg.GetWatermarkLabel(), dlg.GetWatermarkColor() });
        newCanvas->SetBackgroundColor(dlg.GetPageBackgroundColor());
        newCanvas->ResetsPageNumbering(dlg.GetResetPageNumbering());
        const auto bgImgPath = dlg.GetBackgroundImagePath();
        if (!bgImgPath.empty())
            {
            const auto bmp = Wisteria::GraphItems::Image::LoadFile(bgImgPath);
            if (bmp.IsOk())
                {
                newCanvas->SetBackgroundImagePath(bgImgPath);
                newCanvas->SetBackgroundImage(
                    wxBitmapBundle{ wxBitmap{ bmp } },
                    static_cast<uint8_t>(dlg.GetBackgroundImageOpacity()));
                }
            }
        }
    }

//-------------------------------------------
void WisteriaView::OnEditPage([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertPageDlg dlg(canvas, wxArrayString{}, &m_reportBuilder,
                                    dynamic_cast<WisteriaDoc*>(GetDocument()), m_frame, wxID_ANY,
                                    _(L"Edit Page"), wxDefaultPosition, wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                    Wisteria::UI::InsertPageDlg::EditMode::Edit);
    if (m_sideBar->GetSelectedFolder())
        {
        dlg.SetPageName(m_sideBar->GetFolderText(m_sideBar->GetSelectedFolder().value()));
        }
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvas->SetLayer(dlg.GetLayer());
    canvas->SetFixedObjectsGridSize(dlg.GetRows(), dlg.GetColumns());
    dlg.ApplyGridEdits(canvas);
    canvas->FitToPageWhenPrinting(true);
    canvas->SetSizeFromPaperSize();
    canvas->MaintainAspectRatio(true);
    canvas->SetWatermark(
        Wisteria::Canvas::Watermark{ dlg.GetWatermarkLabel(), dlg.GetWatermarkColor() });
    canvas->SetBackgroundColor(dlg.GetPageBackgroundColor());
    canvas->ResetsPageNumbering(dlg.GetResetPageNumbering());
    const auto bgImgPath = dlg.GetBackgroundImagePath();
    if (!bgImgPath.empty())
        {
        const auto bmp = Wisteria::GraphItems::Image::LoadFile(bgImgPath);
        if (bmp.IsOk())
            {
            canvas->SetBackgroundImagePath(bgImgPath);
            canvas->SetBackgroundImage(wxBitmapBundle{ wxBitmap{ bmp } },
                                       static_cast<uint8_t>(dlg.GetBackgroundImageOpacity()));
            }
        }
    else
        {
        canvas->SetBackgroundImagePath(wxString{});
        canvas->SetBackgroundImage(wxBitmapBundle{});
        }
    UpdateCanvas(canvas);

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

    if (wxMessageBox(_(L"Are you sure you want to delete the selected page?"), _(L"Delete Page"),
                     wxYES_NO | wxICON_QUESTION, m_frame) != wxYES)
        {
        return;
        }

    wxWindowUpdateLocker wl{ m_frame };

    m_sideBar->SelectFolder(0, false, false, false);
    m_sideBar->DeleteFolder(selectedFolder.value());
    m_sideBar->SelectFolder(selectedFolder.value() < m_sideBar->GetFolderCount() ?
                                selectedFolder.value() :
                                m_sideBar->GetFolderCount() - 1);

    RemovePageFromProject(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::RemovePageFromProject(Wisteria::Canvas* canvas)
    {
    auto foundPage = std::ranges::find(m_pages, canvas);
    if (foundPage == m_pages.end())
        {
        wxFAIL_MSG(L"Canvas not found when removing page?!");
        return;
        }
    m_pages.erase(foundPage);
    m_workWindows.RemoveWindowById(canvas->GetId());
    }

//-------------------------------------------
std::optional<size_t> WisteriaView::ParseDefaultPageNumber(const wxString& name)
    {
    // "Page %zu" is localizable, so don't hardcode "Page "; translators reposition
    // placeholders but leave the conversion specifier itself intact, so split the
    // (looked-up, but not yet formatted) template on the literal "%zu" to get the
    // prefix/suffix for whatever language is active.
    const wxString templateStr{ _(L"Page %zu") };
    // quneiform-suppress-begin
    const auto placeholderPos = templateStr.Find(L"%zu");
    // quneiform-suppress-end
    if (placeholderPos == wxNOT_FOUND)
        {
        return std::nullopt;
        }

    const wxString prefix = templateStr.Left(static_cast<size_t>(placeholderPos));
    const wxString suffix = templateStr.Mid(static_cast<size_t>(placeholderPos) + 3);

    if (!name.StartsWith(prefix) || !name.EndsWith(suffix) ||
        name.length() < prefix.length() + suffix.length())
        {
        return std::nullopt;
        }

    const wxString middle =
        name.Mid(prefix.length(), name.length() - prefix.length() - suffix.length());
    if (middle.empty() ||
        !std::ranges::all_of(middle, [](const auto chr)
                             { return std::iswdigit(static_cast<wchar_t>(chr)) != 0; }))
        {
        return std::nullopt;
        }

    unsigned long value{ 0 };
    if (!middle.ToULong(&value))
        {
        return std::nullopt;
        }
    return static_cast<size_t>(value);
    }

//-------------------------------------------
void WisteriaView::OnRearrangePages([[maybe_unused]] wxCommandEvent& event)
    {
    struct PageFolderInfo
        {
        wxString m_label;
        wxWindowID m_id{ wxID_ANY };
        Wisteria::Canvas* m_canvas{ nullptr };
        };

    std::vector<PageFolderInfo> currentPages;
    for (size_t i = 2; i < m_sideBar->GetFolderCount(); ++i)
        {
        currentPages.push_back(
            { m_sideBar->GetFolderText(i), m_sideBar->GetFolder(i).GetId(), m_pages[i - 2] });
        }

    wxArrayString pageNames;
    wxArrayInt order;
    bool hasAutoNamedPages{ false };
    for (size_t i = 0; i < currentPages.size(); ++i)
        {
        pageNames.Add(currentPages[i].m_label);
        order.Add(static_cast<int>(i));
        if (ParseDefaultPageNumber(currentPages[i].m_label).has_value())
            {
            hasAutoNamedPages = true;
            }
        }

    wxRearrangeDialog dlg(m_frame, wxString{}, _(L"Reorder Pages"), order, pageNames);

    wxCheckBox* resyncCheck{ nullptr };
    if (hasAutoNamedPages)
        {
        auto* extra = new wxPanel(&dlg);
        auto* extraSizer = new wxBoxSizer(wxVERTICAL);
        resyncCheck = new wxCheckBox(extra, wxID_ANY, _(L"Resequence page numbering"));
        resyncCheck->SetValue(true);
        extraSizer->Add(resyncCheck, wxSizerFlags{}.Border());
        extra->SetSizerAndFit(extraSizer);
        dlg.AddExtraControls(extra);
        }

    dlg.Center();
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const wxArrayInt newOrder = dlg.GetOrder();
    std::vector<size_t> keptIndices;
    std::vector<size_t> removedIndices;
    for (const auto entry : newOrder)
        {
        if (entry >= 0)
            {
            keptIndices.push_back(static_cast<size_t>(entry));
            }
        else
            {
            removedIndices.push_back(static_cast<size_t>(~entry));
            }
        }

    if (!removedIndices.empty())
        {
        const wxString msg = wxString::Format(
            wxPLURAL(L"Are you sure you want to remove %zu unchecked page from the project?",
                     L"Are you sure you want to remove %zu unchecked pages from the project?",
                     removedIndices.size()),
            removedIndices.size());
        if (wxMessageBox(msg, _(L"Remove Pages"), wxYES_NO | wxICON_QUESTION, m_frame) != wxYES)
            {
            return;
            }
        }

    wxWindowUpdateLocker winLock{ m_frame };

    auto* activeCanvas = GetActiveCanvas();

    // reset to a stable selection before folders start shifting around
    m_sideBar->SelectFolder(0, false, false, false);

    // keep removed canvases until the select below hides them
    std::vector<Wisteria::Canvas*> newPages;
    std::vector<std::pair<wxString, wxWindowID>> newFolders;
    for (size_t newPos = 0; newPos < keptIndices.size(); ++newPos)
        {
        const auto& info = currentPages[keptIndices[newPos]];
        wxString label = info.m_label;
        if (resyncCheck != nullptr && resyncCheck->GetValue() &&
            ParseDefaultPageNumber(label).has_value())
            {
            label = wxString::Format(_(L"Page %zu"), newPos + 1);
            }
        newPages.push_back(info.m_canvas);
        newFolders.emplace_back(label, info.m_id);

        if (label != info.m_label)
            {
            info.m_canvas->SetLabel(label);
            info.m_canvas->SetNameTemplate(label);
            }
        }

    m_pages = newPages;

    while (m_sideBar->GetFolderCount() > 2)
        {
        m_sideBar->DeleteFolder(m_sideBar->GetFolderCount() - 1);
        }
    for (const auto& [label, folderId] : newFolders)
        {
        m_sideBar->InsertItem(m_sideBar->GetFolderCount(), label, folderId, PAGE_ICON_INDEX);
        }

    // select (with event, so the matching canvas/grid is actually shown) either the page
    // that was active before the rearrange (if it's still around) or fall back to the Data section
    const auto activePos =
        (activeCanvas != nullptr) ? std::ranges::find(m_pages, activeCanvas) : m_pages.end();
    if (activePos != m_pages.end())
        {
        m_sideBar->SelectFolder(static_cast<size_t>(std::distance(m_pages.begin(), activePos)) + 2);
        }
    else if (m_sideBar->GetFolderCount() > 0)
        {
        m_sideBar->SelectFolder(0);
        }

    // now safe to drop removed canvases from tracking (the select above already hid them)
    for (const auto removedIdx : removedIndices)
        {
        m_workWindows.RemoveWindowById(currentPages[removedIdx].m_canvas->GetId());
        }

    m_sideBar->SaveState();

    m_workArea->Layout();
    m_sideBar->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
Wisteria::Canvas* WisteriaView::AddPageToProject(const size_t rows, const size_t columns,
                                                 const wxString& name,
                                                 const std::optional<size_t> position)
    {
    const wxWindowID pageId = wxNewId();

    auto* canvas = new Wisteria::Canvas(m_workArea, pageId);
    ApplyGlobalPrintSettings(canvas);
    canvas->SetFixedObjectsGridSize(rows, columns);
    canvas->FitToPageWhenPrinting(true);
    canvas->SetSizeFromPaperSize();
    canvas->MaintainAspectRatio(true);

    canvas->Bind(wxEVT_LEFT_UP,
                 [this](wxMouseEvent& event)
                 {
                     event.Skip();
                     CallAfter([this]() { UpdateGraphButtonStates(); });
                 });
    canvas->Bind(wxEVT_KEY_UP,
                 [this](wxKeyEvent& event)
                 {
                     event.Skip();
                     CallAfter([this]() { UpdateGraphButtonStates(); });
                 });

    canvas->Hide();
    m_workArea->GetSizer()->Add(canvas, wxSizerFlags{ 1 }.Expand());
    m_workWindows.AddWindow(canvas);

    const size_t insertIndex = position.value_or(m_pages.size());
    if (insertIndex >= m_pages.size())
        {
        m_pages.push_back(canvas);
        }
    else
        {
        m_pages.insert(m_pages.begin() + insertIndex, canvas);
        }

    const wxString displayName =
        !name.empty() ? name : wxString::Format(_(L"Page %zu"), m_pages.size());
    canvas->SetLabel(displayName);

    const size_t sidebarInsertIndex =
        position.has_value() ?
            position.value() + 2 : // offset by Data (0) and Constants (1) folders
            m_sideBar->GetFolderCount();

    m_sideBar->InsertItem(sidebarInsertIndex, displayName, pageId, PAGE_ICON_INDEX);
    m_sideBar->SelectFolder(sidebarInsertIndex, true);
    m_sideBar->SaveState();

    m_workArea->Layout();
    m_sideBar->Refresh();
    UpdateDatasetButtonStates();
    UpdateGraphButtonStates();

    GetDocument()->Modify(true);
    return canvas;
    }

//-------------------------------------------
bool WisteriaView::IsPageSelected() const noexcept
    {
    return std::ranges::any_of(m_pages, [](const auto* canvas)
                               { return canvas != nullptr && canvas->IsShown(); });
    }

//-------------------------------------------
bool WisteriaView::IsDatasetSelected() const noexcept
    {
    return (m_sideBar->GetSelectedFolder() && m_sideBar->GetSelectedFolder().value() == 0 &&
            !m_reportBuilder.GetDatasets().empty());
    }

//-------------------------------------------
bool WisteriaView::IsGraphSelected() const noexcept
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return false;
        }

    const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
    std::shared_ptr<Wisteria::GraphItems::GraphItemBase> selectedItem;
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
                }
            }
        }

    return (selectedCount == 1 && selectedItem != nullptr &&
            selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Graph2D)));
    }

//-------------------------------------------
bool WisteriaView::IsCanvasItemSelected() const noexcept
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return false;
        }

    const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
    size_t selectedCount{ 0 };
    for (size_t row = 0; row < gridRows; ++row)
        {
        for (size_t col = 0; col < gridCols; ++col)
            {
            auto item = canvas->GetFixedObject(row, col);
            if (item != nullptr && (item->IsSelected() || !item->GetSelectedIds().empty()))
                {
                ++selectedCount;
                }
            }
        }

    return selectedCount == 1;
    }

//-------------------------------------------
void WisteriaView::UpdateDatasetButtonStates() const
    {
    const bool enabled = IsDatasetSelected();

    if (m_datasetButtonBar != nullptr)
        {
        m_datasetButtonBar->EnableButton(ID_EDIT_DATASET, enabled);
        m_datasetButtonBar->EnableButton(ID_DELETE_DATASET, enabled);
        }
    }

//-------------------------------------------
void WisteriaView::UpdateGraphButtonStates() const
    {
    const bool enabled = IsPageSelected();

    if (m_pagesButtonBar != nullptr)
        {
        m_pagesButtonBar->EnableButton(ID_EDIT_PAGE, enabled);
        m_pagesButtonBar->EnableButton(ID_DELETE_PAGE, enabled);
        m_pagesButtonBar->EnableButton(ID_EDIT_ITEM, enabled);
        m_pagesButtonBar->EnableButton(ID_DELETE_ITEM, enabled);
        m_pagesButtonBar->EnableButton(ID_REARRANGE_PAGES, m_pages.size() >= 2);
        }

    // graphs - always enabled; page selection is handled at insertion time
    if (m_graphButtonBar != nullptr)
        {
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BASIC, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_BUSINESS, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_STATISTICAL, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SURVEY, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_EDUCATION, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SOCIAL, true);
        m_graphButtonBar->EnableButton(ID_INSERT_GRAPH_SPORTS, true);
        }

    // objects - insert always enabled
    if (m_objectsButtonBar != nullptr)
        {
        m_objectsButtonBar->EnableButton(ID_NEW_LABEL, true);
        m_objectsButtonBar->EnableButton(ID_NEW_KPI_CARD, true);
        m_objectsButtonBar->EnableButton(ID_NEW_IMAGE, true);
        m_objectsButtonBar->EnableButton(ID_NEW_SHAPE, true);
        m_objectsButtonBar->EnableButton(ID_NEW_COMMON_AXIS, true);
        m_objectsButtonBar->EnableButton(ID_NEW_SPACER, true);
        m_objectsButtonBar->EnableButton(ID_NEW_DIVIDER, true);
        m_objectsButtonBar->EnableButton(wxID_COPY, IsCanvasItemSelected());
        m_objectsButtonBar->EnableButton(wxID_PASTE, true);
        m_objectsButtonBar->EnableButton(ID_EDIT_ITEM, IsCanvasItemSelected());
        m_objectsButtonBar->EnableButton(ID_DELETE_ITEM, IsCanvasItemSelected());
        m_objectsButtonBar->EnableButton(ID_GOTO_DATASOURCE, IsGraphSelected());
        }

    if (m_sourcesButtonBar != nullptr)
        {
        m_sourcesButtonBar->EnableButton(ID_GOTO_DATASOURCE, IsGraphSelected());
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
void WisteriaView::OnDividerDropdown(wxCommandEvent& event)
    {
    dynamic_cast<wxRibbonButtonBarEvent&>(event).PopupMenu(&m_dividerMenu);
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

    appendItem(m_dividerMenu, ID_NEW_DIVIDER_HORIZONTAL_SINGLE, _(L"Horizontal (Single Line)"),
               L"divider-horizontal-single.svg");
    appendItem(m_dividerMenu, ID_NEW_DIVIDER_HORIZONTAL_DOUBLE, _(L"Horizontal (Double Line)"),
               L"divider-horizontal-double.svg");
    m_dividerMenu.AppendSeparator();
    appendItem(m_dividerMenu, ID_NEW_DIVIDER_VERTICAL_SINGLE, _(L"Vertical (Single Line)"),
               L"divider-vertical-single.svg");
    appendItem(m_dividerMenu, ID_NEW_DIVIDER_VERTICAL_DOUBLE, _(L"Vertical (Double Line)"),
               L"divider-vertical-double.svg");

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
    appendItem(m_basicGraphMenu, ID_NEW_RACETRACK_CHART, _(L"Race Track Chart..."),
               L"racetrack.svg");
    appendItem(m_basicGraphMenu, ID_NEW_NIGHTINGALE_ROSE_CHART, _(L"Nightingale Rose Chart..."),
               L"rose.svg");
    appendItem(m_basicGraphMenu, ID_NEW_DUBOIS_SPIRAL_CHART, _(L"Du Bois Spiral Chart..."),
               L"dubois-spiral.svg");
    appendItem(m_basicGraphMenu, ID_NEW_PICTOGRAPH, _(L"Pictograph..."), L"pictograph.svg");
    m_basicGraphMenu.AppendSeparator();
    appendItem(m_basicGraphMenu, ID_NEW_CHOROPLETH_MAP, _(L"Choropleth Map..."), L"choropleth.svg");

    // Business graphs
    appendItem(m_businessGraphMenu, ID_NEW_GANTT, _(L"Gantt Chart..."), L"gantt.svg");
    appendItem(m_businessGraphMenu, ID_NEW_CANDLESTICK, _(L"Candlestick Plot..."),
               L"candlestick.svg");
    appendItem(m_businessGraphMenu, ID_NEW_BULLET_CHART, _(L"Bullet Chart..."), L"bulletchart.svg");
    appendItem(m_businessGraphMenu, ID_NEW_WATERFALL_CHART, _(L"Waterfall Chart..."),
               L"waterfallchart.svg");
    appendItem(m_businessGraphMenu, ID_NEW_FUNNEL_CHART, _(L"Funnel Chart..."), L"funnel.svg");

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
    m_statisticalGraphMenu.AppendSeparator();
    appendItem(m_statisticalGraphMenu, ID_NEW_WILMARTH_BRIDGE_PLOT, _(L"Wilmarth Bridge Plot..."),
               L"wilmarth-bridge.svg");

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
void WisteriaView::SetDialogIcon(wxDialog& dlg, const wxString& svgName)
    {
    const auto svg = wxGetApp().GetResourceManager().GetSVG(svgName);
    if (svg.IsOk())
        {
        wxIcon icon;
        icon.CopyFromBitmap(svg.GetBitmap(dlg.FromDIP(wxSize{ 32, 32 })));
        dlg.SetIcon(icon);
        }
    }

//-------------------------------------------
std::pair<Wisteria::Side, Wisteria::LegendCanvasPlacementHint>
WisteriaView::GetLegendSideAndHint(Wisteria::UI::LegendPlacement placement)
    {
    const auto hint = (placement == Wisteria::UI::LegendPlacement::Right) ?
                          Wisteria::LegendCanvasPlacementHint::RightOfGraph :
                      (placement == Wisteria::UI::LegendPlacement::Left) ?
                          Wisteria::LegendCanvasPlacementHint::LeftOfGraph :
                          Wisteria::LegendCanvasPlacementHint::AboveOrBeneathGraph;
    const auto side = (placement == Wisteria::UI::LegendPlacement::Right) ? Wisteria::Side::Right :
                      (placement == Wisteria::UI::LegendPlacement::Left)  ? Wisteria::Side::Left :
                      (placement == Wisteria::UI::LegendPlacement::Top)   ? Wisteria::Side::Top :
                                                                            Wisteria::Side::Bottom;
    return { side, hint };
    }

//-------------------------------------------
std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
WisteriaView::BuildLegend(const Wisteria::UI::InsertGraphDlg& dlg,
                          const Wisteria::UI::LegendPlacement legendPlacement,
                          const std::function<std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                              const Wisteria::Graphs::LegendOptions&)>& createLegend)
    {
    if (legendPlacement == Wisteria::UI::LegendPlacement::None)
        {
        return nullptr;
        }
    const auto [side, hint] = GetLegendSideAndHint(legendPlacement);
    return createLegend(Wisteria::Graphs::LegendOptions{}
                            .IncludeHeader(dlg.IsLegendIncludingHeader())
                            .Title(dlg.GetLegendTitle())
                            .RingPerimeter(dlg.GetLegendRingPerimeter())
                            .Placement(side)
                            .PlacementHint(hint));
    }

//-------------------------------------------
std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
WisteriaView::BuildLegend(const Wisteria::UI::InsertGraphDlg& dlg, Wisteria::Graphs::Graph2D& plot,
                          const Wisteria::UI::LegendPlacement legendPlacement)
    {
    return BuildLegend(dlg, legendPlacement, [&plot](const Wisteria::Graphs::LegendOptions& options)
                       { return plot.CreateLegend(options); });
    }

//-------------------------------------------
void WisteriaView::ClearGraphAndLegend(Wisteria::Canvas* canvas,
                                       const Wisteria::Graphs::Graph2D& graph, size_t graphRow,
                                       size_t graphCol)
    {
    canvas->SetFixedObject(graphRow, graphCol, nullptr);
    const auto& oldLegendInfo = graph.GetLegendInfo();
    if (oldLegendInfo.has_value())
        {
        const auto [gRows, gCols] = canvas->GetFixedObjectsGridSize();
        const auto oldSide = oldLegendInfo->GetPlacement();
        const bool hasLegendCell = (oldSide == Wisteria::Side::Top && graphRow > 0) ||
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
                const bool isChoroplethLegend =
                    legendItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChoroplethLegend));
                const bool isChernoffLegend = legendItem->IsKindOf(
                    wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend));
                if (isChoroplethLegend || isChernoffLegend ||
                    (label != nullptr && label->IsLegend()))
                    {
                    canvas->SetFixedObject(legendRow, legendCol, nullptr);
                    }
                }
            }
        }
    }

//-------------------------------------------
void WisteriaView::CarryForwardProperty(const Wisteria::Graphs::Graph2D& oldGraph,
                                        Wisteria::Graphs::Graph2D& newGraph, const wxString& prop,
                                        const wxString& newVal, const wxString& oldExpanded)
    {
    if (newVal != oldExpanded || newVal.empty())
        {
        newGraph.SetPropertyTemplate(prop, newVal);
        }
    else
        {
        const auto oldTemplate = oldGraph.GetPropertyTemplate(prop);
        newGraph.SetPropertyTemplate(prop, oldTemplate.empty() ? newVal : oldTemplate);
        }
    }

//-------------------------------------------
Wisteria::Canvas* WisteriaView::GetActiveCanvas() const noexcept
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
Wisteria::Canvas* WisteriaView::EnsureActivePage()
    {
    if (auto* canvas = GetActiveCanvas(); canvas != nullptr)
        {
        return canvas;
        }

    if (m_pages.empty())
        {
        return nullptr;
        }

    wxArrayString pageNames;
    for (const auto* pg : m_pages)
        {
        pageNames.Add(pg->GetLabel());
        }

    const int sel = wxGetSingleChoiceIndex(_(L"Select the page to insert into:"), _(L"Select Page"),
                                           pageNames, m_frame);
    if (sel == wxNOT_FOUND)
        {
        return nullptr;
        }

    m_sideBar->SelectFolder(static_cast<size_t>(sel) + 2, true, true);

    return m_pages[static_cast<size_t>(sel)];
    }

//-------------------------------------------
void WisteriaView::PlaceGraphWithLegend(
    Wisteria::Canvas* canvas, const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
    std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend, const size_t graphRow,
    const size_t graphCol, const Wisteria::UI::LegendPlacement legendPlacement) const
    {
    PlaceGraphAndLegendInGrid(canvas, plot, std::move(legend), graphRow, graphCol, legendPlacement);

    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::PlaceGraphAndLegendInGrid(
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
        // If parent graph has dynamically built screen reader description, then it will
        // be explaining anything that the legend would, hence making this legend redundant;
        // hide it.
        // Extended Chernoff faces legend is the exception, it provided additional information
        // that the plot can't describe.
        if (plot->IsUsingAutoAccessibility() &&
            !legend->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend)))
            {
            legend->GetAutoAccessibilityAttributes() = wxSVGAttributes{}.AriaHidden();
            }

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

    // a legend that was dropped or moved to another side leaves its row or column
    // empty, so close the gap around the outside of the grid
    canvas->RemoveEmptyOuterCells();
    }

//-------------------------------------------
void WisteriaView::OnInsertChernoffPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertChernoffDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"chernoffplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildChernoffFacesPlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        auto legend =
            BuildLegend(dlg, legendPlacement,
                        [&plot, &dlg](const Wisteria::Graphs::LegendOptions& options)
                            -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                        {
                            return dlg.GetUseEnhancedLegend() ?
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateEnhancedLegend(options)) :
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateLegend(options));
                        });
        PlaceGraphWithLegend(canvas, plot, std::move(legend), dlg.GetSelectedRow(),
                             dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertScatterPlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertScatterPlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"scatterplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildScatterPlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
wxString WisteriaView::BuildAggPosJson(const wxString& pos, const wxString& dimension, int offset)
    {
    if (pos.empty())
        {
        return wxString(L"null");
        }
    // numeric positions are row/column indices
    long index{ 0 };
    if (pos.ToLong(&index) && index >= 0)
        {
        if (offset != 0)
            {
            return wxString::Format(L"{\"origin\":%ld, \"offset\":%d}", index, offset);
            }
        return std::to_wstring(index);
        }
    if (pos.CmpNoCase(L"last-column") == 0 || pos.CmpNoCase(L"last-row") == 0)
        {
        if (offset != 0)
            {
            return wxString::Format(L"{\"origin\":\"%s\", \"offset\":%d}", pos, offset);
            }
        return wxString::Format(L"\"%s\"", pos);
        }
    if (offset != 0)
        {
        return wxString::Format(L"{\"origin\":\"%s:%s\", \"offset\":%d}", dimension, pos, offset);
        }
    return wxString::Format(L"\"%s:%s\"", dimension, pos);
    }

//-------------------------------------------
void WisteriaView::OnInsertTable([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertTableDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"table.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto table = dlg.BuildTable();

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
    SetDialogIcon(dlg, L"table.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto table = dlg.BuildTable(&graph);

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

    if (selectedCount == 0 || selectedItem == nullptr)
        {
        wxMessageBox(_(L"Please select an item to edit."), _(L"Edit"), wxOK | wxICON_INFORMATION,
                     m_frame);
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

    // standalone axes (common axes) are not Graph2D
    auto* axis = dynamic_cast<Wisteria::GraphItems::Axis*>(selectedItem.get());
    if (axis != nullptr)
        {
        EditCommonAxis(*axis, canvas, itemRow, itemCol);
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScaleChart)))
        {
        EditScaleChart(*graph, canvas, itemRow, itemCol);
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChoroplethMap)))
        {
        EditChoroplethMap(*graph, canvas, itemRow, itemCol);
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
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::RaceTrackChart)))
        {
        EditRaceTrackChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::NightingaleRoseChart)))
        {
        EditNightingaleRoseChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::DuBoisSpiralChart)))
        {
        EditDuBoisSpiralChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Pictograph)))
        {
        EditPictograph(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BulletChart)))
        {
        EditBulletChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaterfallChart)))
        {
        EditWaterfallChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::FunnelChart)))
        {
        EditFunnelChart(*graph, canvas, itemRow, itemCol);
        }
    else if (selectedItem->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WilmarthBridgePlot)))
        {
        EditWilmarthBridgePlot(*graph, canvas, itemRow, itemCol);
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

    const auto isLegend = [](const auto& item)
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

    if (selectedCount == 0 || selectedItem == nullptr)
        {
        wxMessageBox(_(L"Please select an item to delete."), _(L"Delete"),
                     wxOK | wxICON_INFORMATION, m_frame);
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
void WisteriaView::OnGoToDatasource([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = GetActiveCanvas();
    if (canvas == nullptr)
        {
        return;
        }

    // find the selected item in the canvas grid
    const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
    std::shared_ptr<Wisteria::GraphItems::GraphItemBase> selectedItem;
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
                }
            }
        }

    if (selectedCount != 1 || selectedItem == nullptr)
        {
        return;
        }

    auto* graph = dynamic_cast<Wisteria::Graphs::Graph2D*>(selectedItem.get());
    if (graph == nullptr)
        {
        return;
        }

    wxString dsName;
    // choropleths' dataset is optional, can be used to simply show
    // a map and its features
    if (auto* choropleth = dynamic_cast<Wisteria::Graphs::ChoroplethMap*>(graph);
        choropleth != nullptr)
        {
        dsName = choropleth->GetDataSourceName();
        }
    else if (const auto& dataset = graph->GetDataset(); dataset != nullptr)
        {
        for (const auto& [name, ds] : m_reportBuilder.GetDatasets())
            {
            if (ds.get() == dataset.get())
                {
                dsName = name;
                break;
                }
            }
        }

    if (dsName.empty())
        {
        wxMessageBox(_(L"This graph isn't connected to a dataset."), _(L"Datasource"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    if (const auto found = m_sideBar->FindSubItem(dsName);
        found.first.has_value() && found.second.has_value())
        {
        m_sideBar->SelectSubItem(found);
        }
    }

//-------------------------------------------
void WisteriaView::EditScatterPlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertScatterPlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Scatter Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"scatterplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildScatterPlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertBubblePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"bubbleplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBubblePlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditBubblePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertBubblePlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Bubble Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"bubbleplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBubblePlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditChernoffPlot(const Wisteria::Graphs::Graph2D& graph,
                                    Wisteria::Canvas* canvas, const size_t graphRow,
                                    const size_t graphCol) const
    {
    Wisteria::UI::InsertChernoffDlg dlg(canvas, &m_reportBuilder, m_frame,
                                        _(L"Edit Chernoff Faces Plot"), wxID_ANY, wxDefaultPosition,
                                        wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"chernoffplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildChernoffFacesPlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        auto legend =
            BuildLegend(dlg, legendPlacement,
                        [&plot, &dlg](const Wisteria::Graphs::LegendOptions& options)
                            -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                        {
                            return dlg.GetUseEnhancedLegend() ?
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateEnhancedLegend(options)) :
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateLegend(options));
                        });
        PlaceGraphWithLegend(canvas, plot, std::move(legend), dlg.GetSelectedRow(),
                             dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertLinePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"lineplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLinePlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditLinePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Line Plot"),
                                        wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"lineplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLinePlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertMultiSeriesLinePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"lineplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildMultiSeriesLinePlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditMultiSeriesLinePlot(const Wisteria::Graphs::Graph2D& graph,
                                           Wisteria::Canvas* canvas, const size_t graphRow,
                                           const size_t graphCol) const
    {
    Wisteria::UI::InsertMultiSeriesLinePlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Multi-Series Line Plot"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"lineplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildMultiSeriesLinePlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWCurveDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"wcurve.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWCurvePlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWCurvePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertWCurveDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit W-Curve Plot"),
                                      wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                      Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"wcurve.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWCurvePlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLRRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"roadmap.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLRRoadmap();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditLRRoadmap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertLRRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame,
                                         _(L"Edit Linear Regression Roadmap"), wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"roadmap.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLRRoadmap(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertProConRoadmapDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"roadmap.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildProConRoadmap();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditProConRoadmap(const Wisteria::Graphs::Graph2D& graph,
                                     Wisteria::Canvas* canvas, const size_t graphRow,
                                     const size_t graphCol) const
    {
    Wisteria::UI::InsertProConRoadmapDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Pro && Con Roadmap"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"roadmap.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildProConRoadmap(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertGanttChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"gantt.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildGanttChart();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditGanttChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertGanttChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Gantt Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"gantt.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildGanttChart(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertCandlestickPlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"candlestick.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildCandlestickPlot();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditCandlestickPlot(const Wisteria::Graphs::Graph2D& graph,
                                       Wisteria::Canvas* canvas, const size_t graphRow,
                                       const size_t graphCol) const
    {
    Wisteria::UI::InsertCandlestickPlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Candlestick Plot"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"candlestick.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildCandlestickPlot(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertSankeyDiagramDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"sankey.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildSankeyDiagram();

        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditSankeyDiagram(const Wisteria::Graphs::Graph2D& graph,
                                     Wisteria::Canvas* canvas, const size_t graphRow,
                                     const size_t graphCol) const
    {
    Wisteria::UI::InsertSankeyDiagramDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Sankey Diagram"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"sankey.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildSankeyDiagram(&graph);

        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

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
    auto* canvas = EnsureActivePage();
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
    SetDialogIcon(dlg, L"boxplot.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBoxPlot(doc);
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditBoxPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                               const size_t graphRow, const size_t graphCol) const
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
    SetDialogIcon(dlg, L"boxplot.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBoxPlot(doc, &graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
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
    SetDialogIcon(dlg, L"barchart.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildCatBarChart(doc);
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditCatBarChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
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
    SetDialogIcon(dlg, L"barchart.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildCatBarChart(doc, &graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLikertDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"likert7.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLikertChart();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditLikertChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertLikertDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Likert Chart"),
                                      wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                      Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"likert7.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildLikertChart(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement), graphRow,
                             graphCol, legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertHeatMap([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertHeatMapDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"heatmap.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildHeatMap();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditHeatMap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                               const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertHeatMapDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Heat Map"),
                                       wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                       Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"heatmap.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildHeatMap(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertHistogramDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"histogram.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildHistogram();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditHistogram(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertHistogramDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Histogram"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"histogram.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildHistogram(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertScaleChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertScaleChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"scale.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildScaleChart();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditScaleChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertScaleChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Scale Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"scale.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildScaleChart(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWordCloudDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"wordcloud.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWordCloud();

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
void WisteriaView::OnInsertChoroplethMap([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertChoroplethMapDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"choropleth.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildChoroplethMap();

        // uses a specialized legend
        const bool wantsLegend = dlg.IsMappingData() || dlg.IsUsingProportionalSymbols();
        const auto legendPlacement =
            wantsLegend ? dlg.GetLegendPlacement() : Wisteria::UI::LegendPlacement::None;

        auto legendObject =
            BuildLegend(dlg, legendPlacement,
                        [&plot, &dlg](const Wisteria::Graphs::LegendOptions& options)
                            -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                        {
                            return dlg.GetSymbolColumn().empty() ?
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateLegend(options)) :
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateChoroplethLegend(options));
                        });

        PlaceGraphWithLegend(canvas, plot, std::move(legendObject), dlg.GetSelectedRow(),
                             dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditChoroplethMap(const Wisteria::Graphs::Graph2D& graph,
                                     Wisteria::Canvas* canvas, const size_t graphRow,
                                     const size_t graphCol) const
    {
    Wisteria::UI::InsertChoroplethMapDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Choropleth Map"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"choropleth.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildChoroplethMap(&graph);

        const bool wantsLegend = dlg.IsMappingData() || dlg.IsUsingProportionalSymbols();
        const auto legendPlacement =
            wantsLegend ? dlg.GetLegendPlacement() : Wisteria::UI::LegendPlacement::None;
        const wxString newSymbolColumn = dlg.GetSymbolColumn();

        auto legendObject =
            BuildLegend(dlg, legendPlacement,
                        [&plot, &newSymbolColumn](const Wisteria::Graphs::LegendOptions& options)
                            -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                        {
                            return newSymbolColumn.empty() ?
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateLegend(options)) :
                                       std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                           plot->CreateChoroplethLegend(options));
                        });

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);
        PlaceGraphWithLegend(canvas, plot, std::move(legendObject), dlg.GetSelectedRow(),
                             dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWordCloud(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertWordCloudDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Word Cloud"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"wordcloud.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWordCloud(&graph);

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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWLSparklineDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"sparkline.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWinLossSparkline();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWLSparkline(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertWLSparklineDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Win/Loss Sparkline"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"sparkline.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWinLossSparkline(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertStemAndLeafDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"stem-leaf.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildStemAndLeafPlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditStemAndLeaf(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertStemAndLeafDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Stem-and-Leaf Plot"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"stem-leaf.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildStemAndLeafPlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
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

    Wisteria::UI::InsertPieChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"piechart.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildPieChart(doc);
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditPieChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                const size_t graphRow, const size_t graphCol) const
    {
    auto* doc = dynamic_cast<WisteriaDoc*>(GetDocument());
    if (doc == nullptr)
        {
        wxASSERT_MSG(doc, L"Invalid document connected to view?!");
        return;
        }

    Wisteria::UI::InsertPieChartDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Pie Chart"),
                                        wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"piechart.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildPieChart(doc, &graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWaffleChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"waffle.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWaffleChart();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWaffleChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertWaffleChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Waffle Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"waffle.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWaffleChart(&graph);

        // clear old legend if present
        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertRaceTrackChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertRaceTrackChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"racetrack.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildRaceTrackChart();

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
void WisteriaView::EditRaceTrackChart(const Wisteria::Graphs::Graph2D& graph,
                                      Wisteria::Canvas* canvas, const size_t graphRow,
                                      const size_t graphCol) const
    {
    Wisteria::UI::InsertRaceTrackChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Race Track Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"racetrack.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildRaceTrackChart(&graph);

        // clear old legend if present
        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

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
void WisteriaView::OnInsertNightingaleRoseChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertNightingaleRoseChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"rose.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildNightingaleRoseChart();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditNightingaleRoseChart(const Wisteria::Graphs::Graph2D& graph,
                                            Wisteria::Canvas* canvas, const size_t graphRow,
                                            const size_t graphCol) const
    {
    Wisteria::UI::InsertNightingaleRoseChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Nightingale Rose Chart"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"rose.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildNightingaleRoseChart(&graph);

        // clear old legend if present
        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertBulletChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertBulletChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"bulletchart.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBulletChart();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditBulletChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertBulletChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Bullet Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"bulletchart.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildBulletChart(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWaterfallChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWaterfallChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"waterfallchart.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWaterfallChart();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWaterfallChart(const Wisteria::Graphs::Graph2D& graph,
                                      Wisteria::Canvas* canvas, const size_t graphRow,
                                      const size_t graphCol) const
    {
    Wisteria::UI::InsertWaterfallChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Waterfall Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"waterfallchart.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWaterfallChart(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertFunnelChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertFunnelChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"funnel.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildFunnelChart();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditFunnelChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                   const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertFunnelChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Funnel Chart"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"funnel.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildFunnelChart(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertDuBoisSpiralChart([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertDuBoisSpiralChartDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"dubois-spiral.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildDuBoisSpiralChart();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditDuBoisSpiralChart(const Wisteria::Graphs::Graph2D& graph,
                                         Wisteria::Canvas* canvas, const size_t graphRow,
                                         const size_t graphCol) const
    {
    Wisteria::UI::InsertDuBoisSpiralChartDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Du Bois Spiral Chart"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"dubois-spiral.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildDuBoisSpiralChart(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertPictograph([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertPictographDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"pictograph.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildPictograph();
        canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditPictograph(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  const size_t graphRow, const size_t graphCol) const
    {
    Wisteria::UI::InsertPictographDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Pictograph"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"pictograph.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildPictograph(&graph);
        canvas->SetFixedObject(graphRow, graphCol, plot);

        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnInsertWilmarthBridgePlot([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertWilmarthBridgePlotDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"wilmarth-bridge.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWilmarthBridgePlot();
        const auto legendPlacement = dlg.GetLegendPlacement();

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
                             dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Error"), wxOK | wxICON_ERROR, m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::EditWilmarthBridgePlot(const Wisteria::Graphs::Graph2D& graph,
                                          Wisteria::Canvas* canvas, const size_t graphRow,
                                          const size_t graphCol) const
    {
    Wisteria::UI::InsertWilmarthBridgePlotDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Wilmarth Bridge Plot"), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"wilmarth-bridge.svg");
    dlg.SetSelectedCell(graphRow, graphCol);
    dlg.LoadFromGraph(graph);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        auto plot = dlg.BuildWilmarthBridgePlot(&graph);
        const auto legendPlacement = dlg.GetLegendPlacement();

        // clear old legend if present
        ClearGraphAndLegend(canvas, graph, graphRow, graphCol);

        PlaceGraphWithLegend(canvas, plot, BuildLegend(dlg, *plot, legendPlacement),
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
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLabelDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"label.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), dlg.BuildLabel());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertKpiCard([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertKpiCardDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"kpi-card.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), dlg.BuildKpiCard());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditLabel(const Wisteria::GraphItems::Label& label, Wisteria::Canvas* canvas,
                             const size_t labelRow, const size_t labelCol) const
    {
    const auto spacerType = WisteriaApp::GetSpacerType(label);
    if (spacerType != Wisteria::SpacerType::NotSpacer)
        {
        const bool isEmptySpacer{ spacerType == Wisteria::SpacerType::EmptySpacer };

        // reuse the label dialog purely for its "Placement" (cell-picker) page;
        // a spacer takes no other properties, so the Label/Shapes pages are hidden
        Wisteria::UI::InsertLabelDlg dlg(
            canvas, &m_reportBuilder, m_frame,
            isEmptySpacer ? _(L"Edit Empty Spacer") : _(L"Edit Spacer"), wxID_ANY,
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            Wisteria::UI::InsertItemDlg::EditMode::Edit, Wisteria::UI::LabelDlgIncludePageOptions);
        SetDialogIcon(dlg, L"spacer.svg");
        dlg.SetSelectedCell(labelRow, labelCol);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        canvas->SetFixedObject(labelRow, labelCol,
                               Wisteria::UI::InsertLabelDlg::BuildSpacerLabel(canvas, spacerType));
        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        return;
        }

    const auto dividerType = WisteriaApp::GetDividerType(label);
    if (dividerType != Wisteria::DividerType::NotDivider)
        {
        auto newLabel = Wisteria::UI::InsertLabelDlg::BuildDividerLabel(canvas, dividerType);

        // reuse the label dialog purely for its "Placement" page; a divider's text/font/
        // shapes aren't applicable, so the Label/Shapes pages are hidden
        Wisteria::UI::InsertLabelDlg dlg(
            canvas, &m_reportBuilder, m_frame, _(L"Edit Divider"), wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            Wisteria::UI::InsertItemDlg::EditMode::Edit, Wisteria::UI::LabelDlgIncludePageOptions);
        SetDialogIcon(dlg, WisteriaApp::GetItemIconName(&label));
        dlg.SetSelectedCell(labelRow, labelCol);
        dlg.LoadFromLabel(label);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        dlg.ApplyPageOptions(*newLabel);

        canvas->SetFixedObject(labelRow, labelCol, newLabel);
        UpdateCanvas(canvas);

        GetDocument()->Modify(true);
        return;
        }

    Wisteria::UI::InsertLabelDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Label"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"label.svg");
    dlg.SetSelectedCell(labelRow, labelCol);
    dlg.LoadFromLabel(label);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvas->SetFixedObject(labelRow, labelCol, dlg.BuildLabel());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertSpacer([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertLabelDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Insert Spacer"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Insert, Wisteria::UI::LabelDlgIncludePageOptions);
    SetDialogIcon(dlg, L"spacer.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    canvas->SetFixedObject(
        dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
        Wisteria::UI::InsertLabelDlg::BuildSpacerLabel(canvas, Wisteria::SpacerType::Spacer));
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertDivider(wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    const auto type = (event.GetId() == ID_NEW_DIVIDER_HORIZONTAL_SINGLE) ?
                          Wisteria::DividerType::HorizontalSingleLine :
                      (event.GetId() == ID_NEW_DIVIDER_HORIZONTAL_DOUBLE) ?
                          Wisteria::DividerType::HorizontalDoubleLine :
                      (event.GetId() == ID_NEW_DIVIDER_VERTICAL_SINGLE) ?
                          Wisteria::DividerType::VerticalSingleLine :
                          Wisteria::DividerType::VerticalDoubleLine;
    const wxString iconName =
        (type == Wisteria::DividerType::HorizontalSingleLine) ? L"divider-horizontal-single.svg" :
        (type == Wisteria::DividerType::HorizontalDoubleLine) ? L"divider-horizontal-double.svg" :
        (type == Wisteria::DividerType::VerticalSingleLine)   ? L"divider-vertical-single.svg" :
                                                                L"divider-vertical-double.svg";

    auto label = Wisteria::UI::InsertLabelDlg::BuildDividerLabel(canvas, type);

    Wisteria::UI::InsertLabelDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Insert Divider"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Insert, Wisteria::UI::LabelDlgIncludePageOptions);
    SetDialogIcon(dlg, iconName);
    dlg.LoadFromLabel(*label);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();
    dlg.ApplyPageOptions(*label);

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), label);
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertImage([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
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
    SetDialogIcon(dlg, L"image.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    auto image = dlg.BuildImage(doc);
    if (image == nullptr)
        {
        return;
        }

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), image);
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditImage(Wisteria::GraphItems::Image& image, Wisteria::Canvas* canvas,
                             const size_t imageRow, const size_t imageCol) const
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
    SetDialogIcon(dlg, L"image.svg");
    dlg.SetSelectedCell(imageRow, imageCol);
    dlg.LoadFromImage(image);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    auto newImage = dlg.BuildImage(doc);
    if (newImage == nullptr)
        {
        return;
        }

    canvas->SetFixedObject(imageRow, imageCol, newImage);
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertShape([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertShapeDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"shape.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), dlg.BuildShape());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditShape(const Wisteria::GraphItems::Shape& shape, Wisteria::Canvas* canvas,
                             const size_t shapeRow, const size_t shapeCol) const
    {
    Wisteria::UI::InsertShapeDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Shape"), wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"shape.svg");
    dlg.SetSelectedCell(shapeRow, shapeCol);
    dlg.LoadFromShape(shape);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvas->SetFixedObject(shapeRow, shapeCol, dlg.BuildShape());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditFillableShape(const Wisteria::GraphItems::FillableShape& shape,
                                     Wisteria::Canvas* canvas, const size_t shapeRow,
                                     const size_t shapeCol) const
    {
    Wisteria::UI::InsertShapeDlg dlg(canvas, &m_reportBuilder, m_frame, _(L"Edit Fillable Shape"),
                                     wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                     Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"shape.svg");
    dlg.SetSelectedCell(shapeRow, shapeCol);
    dlg.LoadFromFillableShape(shape);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    canvas->SetFixedObject(shapeRow, shapeCol, dlg.BuildShape());
    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertCommonAxis([[maybe_unused]] wxCommandEvent& event)
    {
    auto* canvas = EnsureActivePage();
    if (canvas == nullptr)
        {
        return;
        }

    Wisteria::UI::InsertCommonAxisDlg dlg(canvas, &m_reportBuilder, m_frame);
    SetDialogIcon(dlg, L"axis.svg");
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    dlg.ApplyGridSize();

    auto commonAxis = dlg.BuildCommonAxis();
    if (commonAxis == nullptr)
        {
        return;
        }

    canvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), std::move(commonAxis));

    UpdateCanvas(canvas);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::EditCommonAxis(Wisteria::GraphItems::Axis& axis, Wisteria::Canvas* canvas,
                                  const size_t axisRow, const size_t axisCol)
    {
    Wisteria::UI::InsertCommonAxisDlg dlg(
        canvas, &m_reportBuilder, m_frame, _(L"Edit Common Axis"), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
        Wisteria::UI::InsertItemDlg::EditMode::Edit);
    SetDialogIcon(dlg, L"axis.svg");
    dlg.LoadFromAxis(axis);
    dlg.LoadPageOptions(axis);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    auto commonAxis = dlg.BuildCommonAxis();
    if (commonAxis == nullptr)
        {
        return;
        }

    canvas->SetFixedObject(axisRow, axisCol, std::move(commonAxis));

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
void WisteriaView::RefreshPagePrintSettings()
    {
    for (auto* page : m_pages)
        {
        ApplyGlobalPrintSettings(page);
        page->FitToPageWhenPrinting(true);
        page->SetSizeFromPaperSize();
        page->MaintainAspectRatio(true);
        UpdateCanvas(page);
        }
    }

//-------------------------------------------
void WisteriaView::ApplyGlobalPrintSettings(Wisteria::Canvas* canvas)
    {
    if (canvas != nullptr)
        {
        auto& appSettings = wxGetApp().GetAppSettings();
        auto& printData = canvas->GetPrinterSettings();
        printData.SetOrientation(
            static_cast<wxPrintOrientation>(appSettings->GetPrintOrientation()));
        printData.SetPaperId(appSettings->GetPaperId());
        }
    }
