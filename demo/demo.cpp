/////////////////////////////////////////////////////////////////////////////
// Name:        demo.cpp
// Purpose:     Wisteria Library Demo
// Author:      Blake Madden
// Created:     01/23/2022
// Copyright:   (c) Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
/////////////////////////////////////////////////////////////////////////////

#include "demo.h"

using namespace Wisteria;
using namespace Wisteria::Colors;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Data;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;
using namespace Wisteria::UI;

wxIMPLEMENT_APP(MyApp);

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// MyApp
// ---------------------------------------------------------------------------

// Initialise this in OnInit, not statically
bool MyApp::OnInit()
    {
    if (!wxApp::OnInit())
        {
        return false;
        }

    wxUILocale::UseDefault();

    wxInitAllImageHandlers();

    // enable this to route wxLog messages to a file:
    // auto logFile = new LogFile{ true };
    // delete wxLog::SetActiveTarget(logFile);

    MyFrame* frame = new MyFrame;
    frame->Show(true);

    return true;
    }

// ---------------------------------------------------------------------------
// MyFrame
// ---------------------------------------------------------------------------

// Define my frame constructor
MyFrame::MyFrame()
    : wxMDIParentFrame(nullptr, wxID_ANY, _(L"Wisteria Demo"), wxDefaultPosition, wxSize(750, 500))
    {
    SetSize(FromDIP(wxSize(750, 500)));

    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxSize iconSize = Image::GetSVGSize(appDir + L"/res/wisteria.svg");

    SetIcon(wxBitmapBundle::FromSVGFile(appDir + L"/res/wisteria.svg", iconSize).GetIcon(iconSize));

    // Associate the menu bar with the frame
    SetMenuBar(CreateMainMenubar());

    // This shows that the standard window menu may be customized:
    wxMenu* const windowMenu = GetWindowMenu();
    if (windowMenu != nullptr)
        {
        // we can change the labels of standard items (which also means we can
        // set up accelerators for them as they're part of the label)
        windowMenu->SetLabel(wxID_MDI_WINDOW_TILE_HORZ, _(L"Tile Horizontally\tCtrl-Shift-H"));
        windowMenu->SetLabel(wxID_MDI_WINDOW_TILE_VERT, _(L"Tile Vertically\tCtrl-Shift-V"));

        // we can also change the help string
        windowMenu->SetHelpString(wxID_MDI_WINDOW_CASCADE, _(L"Arrange windows in cascade"));

        // we can remove some items
        windowMenu->Delete(wxID_MDI_WINDOW_ARRANGE_ICONS);

        // and we can add completely custom commands -- but then we must handle
        // them ourselves, see OnCloseAll()
        windowMenu->AppendSeparator();
        windowMenu->Append(wxID_CLOSE_ALL, _(L"&Close All Windows\tCtrl-Shift-C"),
                           _(L"Close all open windows"));

        SetWindowMenu(windowMenu);
        }

    CreateStatusBar();
    CreateToolBar(wxNO_BORDER | wxTB_FLAT | wxTB_HORIZONTAL);
    InitToolBar(GetToolBar());

    // Accelerators
    wxAcceleratorEntry entries[7]{ 0 };
    entries[0].Set(wxACCEL_CTRL, L'N', wxID_NEW);
    entries[1].Set(wxACCEL_CTRL, L'X', wxID_EXIT);
    entries[2].Set(wxACCEL_CTRL, L'A', wxID_ABOUT);
    entries[3].Set(wxACCEL_CTRL, L'S', wxID_SAVE);
    entries[4].Set(wxACCEL_CTRL, L'P', wxID_PRINT);
    entries[5].Set(wxACCEL_CTRL, L'C', wxID_COPY);
    entries[6].Set(wxACCEL_CTRL, L'O', wxID_OPEN);
    wxAcceleratorTable accel(std::size(entries), entries);
    SetAcceleratorTable(accel);

    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_BOXPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_HISTOGRAM);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this,
         MyApp::ControlIDs::ID_NEW_HISTOGRAM_UNIQUE_VALUES);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_GANTT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_CANDLESTICK_AXIS);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_LINEPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_LINEPLOT_CUSTOMIZED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_BARCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_BARCHART_STYLIZED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_BARCHART_IMAGE);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_CATEGORICAL_BARCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this,
         MyApp::ControlIDs::ID_NEW_CATEGORICAL_BARCHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this,
         MyApp::ControlIDs::ID_NEW_CATEGORICAL_BARCHART_STIPPLED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_PIECHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_PIECHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_DONUTCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_DONUTCHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_WCURVE);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_LR_ROADMAP_GRAPH);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_PROCON_ROADMAP_GRAPH);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_SANKEY_DIAGRAM);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_GROUPED_SANKEY_DIAGRAM);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_WORD_CLOUD);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_LIKERT_3POINT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_LIKERT_7POINT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_HEATMAP);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_HEATMAP_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_SCALE_CHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_MULTIPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_MULTIPLOT_COMMON_AXIS);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ControlIDs::ID_NEW_TABLE);

    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, wxID_NEW);
    Bind(wxEVT_MENU, &MyFrame::OnOpenProject, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MyFrame::OnSaveWindow, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MyFrame::OnPrintWindow, this, wxID_PRINT);
    Bind(wxEVT_MENU, &MyFrame::OnPrintAll, this, MyApp::ID_PRINT_ALL);
    Bind(wxEVT_MENU, &MyFrame::OnCopyWindow, this, wxID_COPY);
    Bind(wxEVT_MENU, &MyFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnCloseAll, this, wxID_CLOSE_ALL);
    Bind(wxEVT_MENU, &MyFrame::OnCloseAll, this, wxID_CLOSE_ALL);
    Bind(wxEVT_MENU, &MyFrame::OnClose, this, wxID_CLOSE);

    Bind(wxEVT_MENU, &MyFrame::OnTextClassifier, this, MyApp::ID_TEXT_CLASSIFIER);
    }

wxMenuBar* MyFrame::CreateMainMenubar()
    {
    wxMenu* fileMenu = new wxMenu;

    fileMenu->Append(MyApp::ID_NEW_BARCHART, _(L"Bar Chart"));
    fileMenu->Append(MyApp::ID_NEW_BARCHART_STYLIZED, _(L"Bar Chart (Stylized)"));
    fileMenu->Append(MyApp::ID_NEW_BARCHART_IMAGE, _(L"Bar Chart (Common Image)"));
    fileMenu->Append(MyApp::ID_NEW_CATEGORICAL_BARCHART, _(L"Bar Chart (Categorical Data)"));
    fileMenu->Append(MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED,
                     _(L"Bar Chart (Categorical Data, Grouped)"));
    fileMenu->Append(MyApp::ID_NEW_CATEGORICAL_BARCHART_STIPPLED, _(L"Bar Chart (Stipple Icon)"));
    fileMenu->Append(MyApp::ID_NEW_PIECHART, _(L"Pie Chart"));
    fileMenu->Append(MyApp::ID_NEW_PIECHART_GROUPED, _(L"Pie Chart (with Subgroup)"));
    fileMenu->Append(MyApp::ID_NEW_DONUTCHART, _(L"Donut Chart"));
    fileMenu->Append(MyApp::ID_NEW_DONUTCHART_GROUPED, _(L"Donut Chart (with Subgroup)"));
    fileMenu->Append(MyApp::ID_NEW_HISTOGRAM, _(L"Histogram"));
    fileMenu->Append(MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES,
                     _(L"Histogram (Discrete Category Counts)"));
    fileMenu->Append(MyApp::ID_NEW_LINEPLOT, _(L"Line Plot"));
    fileMenu->Append(MyApp::ID_NEW_LINEPLOT_CUSTOMIZED, _(L"Line Plot (Customized)"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_BOXPLOT, _(L"Box Plot"));
    fileMenu->Append(MyApp::ID_NEW_HEATMAP, _(L"Heat Map"));
    fileMenu->Append(MyApp::ID_NEW_HEATMAP_GROUPED, _(L"Heat Map (Grouped)"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_SCALE_CHART, _(L"Scale Chart"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_GANTT, _(L"Gantt Chart"));
    fileMenu->Append(MyApp::ID_NEW_CANDLESTICK_AXIS, _(L"Candlestick Plot"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_LIKERT_3POINT, _(L"Likert Chart (3-Point Scale)"));
    fileMenu->Append(MyApp::ID_NEW_LIKERT_7POINT, _(L"Likert Chart (7-Point Scale)"));
    fileMenu->Append(MyApp::ID_NEW_WCURVE, _(L"W-Curve Plot"));
    fileMenu->Append(MyApp::ID_NEW_LR_ROADMAP_GRAPH, _(L"Linear Regression Roadmap"));
    fileMenu->Append(MyApp::ID_NEW_PROCON_ROADMAP_GRAPH, _(L"Pros & Cons Roadmap"));
    fileMenu->Append(MyApp::ID_NEW_SANKEY_DIAGRAM, _(L"Sankey Diagram"));
    fileMenu->Append(MyApp::ID_NEW_GROUPED_SANKEY_DIAGRAM, _(L"Grouped Sankey Diagram"));
    fileMenu->Append(MyApp::ID_NEW_WORD_CLOUD, _(L"Word Cloud"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_MULTIPLOT, _(L"Multiple Plots"));
    fileMenu->Append(MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS, _(L"Multiple Plots (Common Axis)"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_TABLE, _(L"Table"));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_SAVE, _(L"&Save\tCtrl+S"), _(L"Save as Image"));
    fileMenu->Append(wxID_PRINT, _(L"&Print...\tCtrl+P"), _(L"Print"));
    fileMenu->Append(MyApp::ID_PRINT_ALL, _(L"&Print All..."), _(L"Print All"));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_TEXT_CLASSIFIER, _(L"&Text Classifier..."),
                     _(L"Demonstrates the Text Classifier feature"));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_CLOSE, _(L"&Close child\tCtrl+F4"));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_EXIT, _(L"&Exit\tAlt-X"), _(L"Quit the program"));

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT, _(L"&About...\tF1"));

    wxMenuBar* mbar = new wxMenuBar;
    mbar->Append(fileMenu, _(L"&File"));
    mbar->Append(menuHelp, _(L"&Help"));

    return mbar;
    }

void MyFrame::OnTextClassifier([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog recodingFileDlg(this, _(L"Select Recoding Data"), wxString{}, wxString{},
                                 Dataset::GetDataFileFilter(),
                                 wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

    if (recodingFileDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    wxString recodingWorksheetName;
    if (wxFileName{ recodingFileDlg.GetPath() }.GetExt().CmpNoCase(L"xlsx") == 0)
        {
        ExcelReader xlReader{ recodingFileDlg.GetPath() };
        if (xlReader.GetWorksheetNames().size() == 1)
            {
            recodingWorksheetName = xlReader.GetWorksheetNames()[0];
            }
        else
            {
            wxArrayString choices;
            for (const auto& worksheet : xlReader.GetWorksheetNames())
                {
                choices.push_back(worksheet);
                }
            wxSingleChoiceDialog selDlg(this, _(L"Select Worksheet"),
                                        _(L"Select the worksheet to use:"), choices);
            if (selDlg.ShowModal() != wxID_OK)
                {
                return;
                }
            recodingWorksheetName = selDlg.GetStringSelection();
            }
        }

    VariableSelectDlg recodingVarDlg(
        this,
        Dataset::ReadColumnInfo(recodingFileDlg.GetPath(), Data::ImportInfo{}, std::nullopt,
                                recodingWorksheetName),
        { VariableSelectDlg::VariableListInfo()
              .Label(_(L"Matching Regular Expressions"))
              .SingleSelection(true),
          VariableSelectDlg::VariableListInfo().Label(_(L"Replacements")).SingleSelection(true) });
    if (recodingVarDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    wxFileDialog classiferFileDlg(this, _(L"Select Classifier Data"), wxString{}, wxString{},
                                  Dataset::GetDataFileFilter(),
                                  wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

    if (classiferFileDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    wxString classifierWorksheetName;
    if (wxFileName{ classiferFileDlg.GetPath() }.GetExt().CmpNoCase(L"xlsx") == 0)
        {
        ExcelReader xlReader{ classiferFileDlg.GetPath() };
        if (xlReader.GetWorksheetNames().size() == 1)
            {
            classifierWorksheetName = xlReader.GetWorksheetNames()[0];
            }
        else
            {
            wxArrayString choices;
            for (const auto& worksheet : xlReader.GetWorksheetNames())
                {
                choices.push_back(worksheet);
                }
            wxSingleChoiceDialog selDlg(this, _(L"Select Worksheet"),
                                        _(L"Select the worksheet to use:"), choices);
            if (selDlg.ShowModal() != wxID_OK)
                {
                return;
                }
            classifierWorksheetName = selDlg.GetStringSelection();
            }
        }

    VariableSelectDlg classiferVarDlg(
        this,
        Dataset::ReadColumnInfo(classiferFileDlg.GetPath(), Data::ImportInfo{}, std::nullopt,
                                classifierWorksheetName),
        { VariableSelectDlg::VariableListInfo().Label(_(L"Categories")).SingleSelection(true),
          VariableSelectDlg::VariableListInfo()
              .Label(_(L"Subcategories"))
              .SingleSelection(true)
              .Required(false),
          VariableSelectDlg::VariableListInfo().Label(_(L"Patterns")).SingleSelection(true),
          VariableSelectDlg::VariableListInfo()
              .Label(_(L"Negation Patterns"))
              .SingleSelection(true)
              .Required(false) });
    if (classiferVarDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    wxFileDialog surveyFileDlg(this, _(L"Select Survey Data"), wxString{}, wxString{},
                               Dataset::GetDataFileFilter(),
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
    if (surveyFileDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    wxString surveyWorksheetName;
    if (wxFileName{ surveyFileDlg.GetPath() }.GetExt().CmpNoCase(L"xlsx") == 0)
        {
        ExcelReader xlReader{ surveyFileDlg.GetPath() };
        if (xlReader.GetWorksheetNames().size() == 1)
            {
            surveyWorksheetName = xlReader.GetWorksheetNames()[0];
            }
        else
            {
            wxArrayString choices;
            for (const auto& worksheet : xlReader.GetWorksheetNames())
                {
                choices.push_back(worksheet);
                }
            wxSingleChoiceDialog selDlg(this, _(L"Select Worksheet"),
                                        _(L"Select the worksheet to use:"), choices);
            if (selDlg.ShowModal() != wxID_OK)
                {
                return;
                }
            surveyWorksheetName = selDlg.GetStringSelection();
            }
        }

    VariableSelectDlg surveyVarDlg(
        this,
        Dataset::ReadColumnInfo(surveyFileDlg.GetPath(), Data::ImportInfo{}, std::nullopt,
                                surveyWorksheetName),
        { VariableSelectDlg::VariableListInfo().Label(_(L"Comments")).SingleSelection(true) });
    if (surveyVarDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    auto recodingData = std::make_shared<Data::Dataset>();
    auto classifierData = std::make_shared<Data::Dataset>();
    auto surveyData = std::make_shared<Data::Dataset>();
    try
        {
        recodingData->Import(recodingFileDlg.GetPath(),
                             Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(
                                 recodingFileDlg.GetPath(), Data::ImportInfo{}, std::nullopt,
                                 recodingWorksheetName)),
                             recodingWorksheetName);

        classifierData->Import(classiferFileDlg.GetPath(),
                               Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(
                                   classiferFileDlg.GetPath(), Data::ImportInfo{}, std::nullopt,
                                   classifierWorksheetName)),
                               classifierWorksheetName);

        surveyData->Import(surveyFileDlg.GetPath(),
                           Dataset::ImportInfoFromPreview(
                               Dataset::ReadColumnInfo(surveyFileDlg.GetPath(), Data::ImportInfo{},
                                                       std::nullopt, surveyWorksheetName))
                               .MDCodes(ImportInfo::GetCommonMDCodes())
                               .ReplacementStrings(Data::ImportInfo::DatasetToRegExMap(
                                   recodingData, recodingVarDlg.GetSelectedVariables(0)[0],
                                   recodingVarDlg.GetSelectedVariables(1)[0])),
                           surveyWorksheetName);

        Wisteria::Data::TextClassifier textClassifier;
        textClassifier.SetClassifierData(
            classifierData, classiferVarDlg.GetSelectedVariables(0)[0],
            (classiferVarDlg.GetSelectedVariables(1).size() > 0 ?
                 std::optional<wxString>{ classiferVarDlg.GetSelectedVariables(1)[0] } :
                 std::nullopt),
            classiferVarDlg.GetSelectedVariables(2)[0],
            (classiferVarDlg.GetSelectedVariables(3).size() > 0 ?
                 std::optional<wxString>{ classiferVarDlg.GetSelectedVariables(3)[0] } :
                 std::nullopt));
        const auto [matchedData, unclassifiedData] =
            textClassifier.ClassifyData(surveyData, surveyVarDlg.GetSelectedVariables(0)[0]);
        matchedData->ExportCSV(wxFileName{ surveyFileDlg.GetPath() }.GetPathWithSep() +
                               L"Matched.csv");
        unclassifiedData->ExportCSV(wxFileName{ surveyFileDlg.GetPath() }.GetPathWithSep() +
                                    L"Unclassified.csv");

        wxMessageBox(_(L"Matched and Unclassified output files successfully created."),
                     _(L"Text Classification Complete"), wxOK | wxCENTRE);
        }
    catch (const std::exception& err)
        {
        wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                     wxOK | wxICON_ERROR | wxCENTRE);
        return;
        }
    }

void MyFrame::OnQuit([[maybe_unused]] wxCommandEvent& event) { Close(); }

void MyFrame::OnAbout([[maybe_unused]] wxCommandEvent& event)
    {
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetCopyright(_(L"Copyright (c) 2025"));
    wxArrayString devs;
    devs.Add(_DT(L"Blake Madden"));
    aboutInfo.SetDevelopers(devs);
    aboutInfo.SetName(Wisteria::GetLibraryVersionInfo().ToString());
    aboutInfo.SetDescription(_(L"Demonstration of Wisteria Dataviz, "
                               "a wxWidgets-based data visualization library."));

    wxAboutBox(aboutInfo, this);
    }

void MyFrame::OnOpenProject([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog fileDlg(this, _(L"Open Project"), wxEmptyString, GetLabel(),
                         _(L"Project File (*.json)|*.json"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    ReportBuilder rb;
    auto report = rb.LoadConfigurationFile(fileDlg.GetPath(), this);

    for (auto& page : report)
        {
        // create and show a child frame for each page
        MyChild* subframe = new MyChild(this);
        page->Reparent(subframe);
        subframe->m_canvas = page;
        subframe->Show(true);
        }
    }

void MyFrame::OnNewWindow(wxCommandEvent& event)
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    // create and show another child frame
    MyChild* subframe = new MyChild(this);

    subframe->m_canvas = new Wisteria::Canvas(subframe);

    // Box Plot
    if (event.GetId() == MyApp::ID_NEW_BOXPLOT)
        {
        subframe->SetTitle(_(L"Box Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(
                appDir + L"/datasets/mpg.csv",
                ImportInfo()
                    .ContinuousColumns({ L"hwy" })
                    .CategoricalColumns(
                        { { L"class", CategoricalImportMethod::ReadAsStrings },
                          { L"manufacturer", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto plot = std::make_shared<BoxPlot>(subframe->m_canvas);

        plot->SetData(mpgData, L"hwy",
                      // leave this as std::nullopt to not create grouped boxes
                      L"class");

        // Show all points (not just outliers).
        // The points within the boxes and whiskers will be
        // bee-swarm jittered to visualize the distribution.
        plot->ShowAllPoints(true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // scale chart
    else if (event.GetId() == MyApp::ID_NEW_SCALE_CHART)
        {
        subframe->SetTitle(_(L"scale chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto testScoresData = std::make_shared<Data::Dataset>();
        try
            {
            testScoresData->ImportCSV(
                appDir + L"/datasets/Student Scores.csv",
                ImportInfo()
                    .ContinuousColumns({ L"test_score" })
                    .IdColumn(L"Week")
                    .CategoricalColumns({ { L"NAME", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<ScaleChart>(subframe->m_canvas);
        plot->AddScale(
            std::vector<BarChart::BarBlock>{
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(59)
                        .Brush(ColorBrewer::GetColor(Colors::Color::PastelRed, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _(L"F (fail)") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A" }.LabelFitting(
                            LabelFit::DisplayAsIs))) } },
            std::nullopt, L"Grades");
        plot->AddScale(
            std::vector<BarChart::BarBlock>{
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(59)
                        .Brush(ColorBrewer::GetColor(Colors::Color::PastelRed, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _(L"F (fail)") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D-" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D+" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C-" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C+" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B-" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B+" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A-" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A+" }.LabelFitting(
                            LabelFit::DisplayAsIs))) },
            },
            std::nullopt, L"Grades");
        plot->SetMainScaleValues({ 10, 20, 30, 40, 50, 60, 70, 80, 90 }, 0);
        plot->SetData(testScoresData, L"TEST_SCORE", L"NAME");
        plot->SetDataColumnHeader(_(L"Test Scores"));

        subframe->m_canvas->SetFixedObject(0, 0, plot);

        auto legend{ plot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
            LegendCanvasPlacementHint::RightOfGraph)) };
        subframe->m_canvas->SetFixedObject(0, 1, std::move(legend));

        // after changing legend's text, recalculate how much of the
        // canvas it should consume
        subframe->m_canvas->CalcRowDimensions();
        }
    // Heatmap
    else if (event.GetId() == MyApp::ID_NEW_HEATMAP)
        {
        subframe->SetTitle(_(L"Heatmap"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto testScoresData = std::make_shared<Data::Dataset>();
        try
            {
            testScoresData->ImportCSV(
                appDir + L"/datasets/Student Scores.csv",
                ImportInfo().ContinuousColumns({ L"test_score" }).IdColumn(L"Week"));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<HeatMap>(subframe->m_canvas);

        // add a title to the plot
        plot->GetTitle()
            .GetGraphItemInfo()
            .Text(_(L"Test Scores"))
            .ChildAlignment(RelativeAlignment::FlushLeft)
            .Pen(wxNullPen)
            .Padding(4, 0, 0, 4)
            .Font(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)).MakeLarger());

        plot->SetData(testScoresData, L"TEST_SCORE");

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // customize the header of the legend and add it to the canvas
        auto legend{ plot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
            LegendCanvasPlacementHint::RightOfGraph)) };
        legend->SetLine(0, _(L"Range of Scores"));
        subframe->m_canvas->SetFixedObject(0, 1, std::move(legend));

        // after changing legend's text, recalculate how much of the
        // canvas it should consume
        subframe->m_canvas->CalcRowDimensions();
        }
    // Heatmap (grouped)
    else if (event.GetId() == MyApp::ID_NEW_HEATMAP_GROUPED)
        {
        subframe->SetTitle(_(L"Heatmap (Grouped)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto testScoresData = std::make_shared<Data::Dataset>();
        try
            {
            testScoresData->ImportCSV(
                appDir + L"/datasets/Student Scores.csv",
                ImportInfo()
                    .ContinuousColumns({ L"test_score" })
                    .IdColumn(L"Week")
                    .CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<HeatMap>(subframe->m_canvas);
        // add a title to the plot
        plot->GetTitle()
            .GetGraphItemInfo()
            .Text(_(L"Test Scores"))
            .ChildAlignment(RelativeAlignment::FlushLeft)
            .Pen(wxNullPen)
            .Padding(4, 0, 0, 4)
            .Font(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)).MakeLarger());

        // use group and put all of the students' heatmaps into one column
        plot->SetData(testScoresData, L"TEST_SCORE", L"Name", 1);
        // say "Students" at the top instead of "Groups"
        plot->SetGroupHeaderPrefix(_(L"Students"));

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // customize the header of the legend and add it to the canvas
        auto legend{ plot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
            LegendCanvasPlacementHint::RightOfGraph)) };
        subframe->m_canvas->SetFixedObject(0, 1, std::move(legend));
        }
    // Histogram
    else if (event.GetId() == MyApp::ID_NEW_HISTOGRAM)
        {
        subframe->SetTitle(_(L"Histogram"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto mtcarsData = std::make_shared<Data::Dataset>();
        try
            {
            mtcarsData->ImportCSV(
                appDir + L"/datasets/mtcars.csv",
                ImportInfo()
                    .ContinuousColumns({ L"mpg" })
                    .CategoricalColumns({ { L"Gear", CategoricalImportMethod::ReadAsIntegers } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<Histogram>(
            subframe->m_canvas,
            std::make_shared<Brushes::Schemes::BrushScheme>(Colors::Schemes::Decade1980s()));

        plot->SetData(mtcarsData, L"mpg",
                      // grouping variable, we won't use one here
                      std::nullopt,
                      // make the ranges neat integers
                      Histogram::BinningMethod::BinByIntegerRange,
                      // don't round the data
                      RoundingMethod::NoRounding,
                      // show labels at the edges of the bars, showing the ranges
                      Histogram::IntervalDisplay::Cutpoints,
                      // show the counts and percentages above the bars
                      BinLabelDisplay::BinValueAndPercentage,
                      // not used with range binning
                      true,
                      // don't request a specify bin start
                      std::nullopt,
                      // explicitly request 5 bins
                      std::make_pair(5, std::nullopt));

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // add a legend if grouping (in this case, we aren't)
        if (plot->GetGroupCount() > 0)
            {
            subframe->m_canvas->SetFixedObject(
                0, 1,
                plot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
                    LegendCanvasPlacementHint::RightOfGraph)));
            }
        }
    // Histogram (discrete categories from a grouping variable get their own bars)
    else if (event.GetId() == MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES)
        {
        subframe->SetTitle(_(L"Histogram (Discrete Category Counts)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(appDir + L"/datasets/mpg.csv",
                               ImportInfo().ContinuousColumns({ L"cyl" }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<Histogram>(
            subframe->m_canvas,
            std::make_shared<Brushes::Schemes::BrushScheme>(Colors::Schemes::Decade1980s()));

        plot->SetData(mpgData, L"cyl", std::nullopt,
                      // don't create range-based bins;
                      // instead, create one for each unique value.
                      Histogram::BinningMethod::BinUniqueValues,
                      // If the data is floating point, you can tell it to
                      // to be rounded here when categorizing it into discrete bins.
                      // In this case, the data is already discrete, so no rounding needed.
                      RoundingMethod::NoRounding,
                      // since we aren't using ranges, show labels under the middle of the bins.
                      Histogram::IntervalDisplay::Midpoints, BinLabelDisplay::BinValue,
                      // pass in false to remove the empty '7' bin
                      true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Line Plot
    else if (event.GetId() == MyApp::ID_NEW_LINEPLOT)
        {
        subframe->SetTitle(_(L"Line Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto linePlotData = std::make_shared<Data::Dataset>();
        try
            {
            linePlotData->ImportCSV(
                appDir + L"/datasets/Spelling Grades.csv",
                ImportInfo()
                    .
                // first the Y column
                ContinuousColumns({ L"AVG_GRADE" })
                    .
                // group and X
                CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings },
                                     { L"WEEK_NAME", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto linePlot = std::make_shared<LinePlot>(
            subframe->m_canvas,
            // use a different color scheme
            std::make_shared<Colors::Schemes::Decade1960s>(),
            // or create your own scheme
            // std::make_shared<Colors::Schemes::ColorScheme>
            //     (Colors::Schemes::ColorScheme{
            //         ColorBrewer::GetColor(Colors::Color::Auburn),
            //         ColorBrewer::GetColor(Colors::Color::OctoberMist) }),

            // turn off markers by using a shape scheme filled with blank icons
            // (having just one icon in this scheme will get recycled for each line)
            std::make_shared<IconScheme>(IconScheme{ IconShape::Blank }));
        // add padding around the plot
        linePlot->SetCanvasMargins(5, 5, 5, 5);

        // Set the data and use the grouping column from the dataset to create separate lines.
        // Also, use a categorical column for the X axis.
        linePlot->SetData(linePlotData, L"AVG_GRADE", L"WEEK_NAME", L"Gender");

        // add some titles
        linePlot->GetTitle().SetText(_(L"Average Grades"));
        linePlot->GetSubtitle().SetText(_(L"Average grades taken from\n"
                                          "last 5 weeks' spelling tests."));
        linePlot->GetCaption().SetText(_(L"Note: not all grades have been\n"
                                         "entered yet for last week."));
        // remove default titles
        linePlot->GetBottomXAxis().GetTitle().SetText(L"");
        linePlot->GetLeftYAxis().GetTitle().SetText(L"");

        // add the line plot and its legend to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);
        subframe->m_canvas->SetFixedObject(
            0, 1,
            linePlot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
                LegendCanvasPlacementHint::RightOfGraph)));

        /* A note about dataset design. If you have a dataset built like this:

           X    Y1    Y2
           -------------
           1    7     9
           2    7.5   11

           and you wish to plot Y1 and Y2 as separate lines along the X values, then
           you will need to pivot the dataset longer. To do this, call Pivot::PivotLonger() and
           pass in the dataset to get back a "long" dataset that you can then use with
           the line plot.

           For example:

           Pivot pv;
           auto newDataset = pv.PivotLonger(myDataset,
               { L"x" }, { L"y1", L"y2" }, { L"GROUP" },
               L"YValues");

           At this point, you can pass newDataset to the line plot and specify "X" and "YValues" as
           the X and Y and "GROUP" as the grouping column. From there, this will create a line for
           the Y1 values and another line for the Y2 values.*/
        }
    // Line Plot (customized)
    else if (event.GetId() == MyApp::ID_NEW_LINEPLOT_CUSTOMIZED)
        {
        subframe->SetTitle(_(L"Line Plot (Customized)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto linePlotData = std::make_shared<Data::Dataset>();
        try
            {
            linePlotData->ImportCSV(
                appDir + L"/datasets/Spelling Grades.csv",
                ImportInfo()
                    .
                // first the Y column, then the X
                ContinuousColumns({ L"AVG_GRADE", L"WeeK" })
                    .CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto linePlot = std::make_shared<LinePlot>(
            subframe->m_canvas,
            // create your own color scheme
            std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::ColorScheme{
                ColorBrewer::GetColor(Colors::Color::Auburn),
                ColorBrewer::GetColor(Colors::Color::GrannySmithApple) }),
            // use custom markers
            std::make_shared<IconScheme>(IconScheme{ IconShape::Diamond, IconShape::Hexagon }));
        // add padding around the plot
        linePlot->SetCanvasMargins(5, 5, 5, 5);

        // Set the data and use the grouping column from the dataset to create separate lines.
        // Also, use a continuous column for the X axis, where we will set the labels
        // ourselves later.
        linePlot->SetData(linePlotData, L"AVG_GRADE", L"WeeK", L"Gender");
        // after setting the data, customize the appearance of one of the lines by index
        linePlot->GetLine(1).GetPen().SetStyle(wxPenStyle::wxPENSTYLE_DOT_DASH);
        // iterate through the lines and change their color based on their names
        // (which will override the color scheme)
        for (auto& line : linePlot->GetLines())
            {
            if (line.GetText().CmpNoCase(L"Male") == 0)
                {
                line.GetPen().SetColour(ColorBrewer::GetColor(Colors::Color::CelestialBlue));
                }
            else
                {
                line.GetPen().SetColour(ColorBrewer::GetColor(Colors::Color::PinkSherbet));
                }
            }

        // change the color for any point less than 60 to red to show if failing
        linePlot->SetPointColorCriteria([]([[maybe_unused]]
                                           const double x,
                                           const double y)
                                        { return (y < 60.0) ? *wxRED : wxColour(); });

        // add a note
        auto note = std::make_shared<Label>(
            GraphItemInfo(_(L"What happened this week?\nAre we sure this is correct???"))
                .Pen(*wxLIGHT_GREY)
                .FontBackgroundColor(ColorBrewer::GetColor(Color::AntiqueWhite))
                .Anchoring(Anchoring::TopRightCorner)
                .Padding(4, 4, 4, 4));
        linePlot->AddAnnotation(note,
                                // top corner of note
                                wxPoint(3, 38),
                                // the suspect data point to make the note point to
                                { wxPoint(4, 59) });

        // add some titles
        linePlot->GetTitle().SetText(_(L"Average Grades"));
        linePlot->GetSubtitle().SetText(_(L"Average grades taken from\n"
                                          "last 5 weeks' spelling tests."));
        linePlot->GetCaption().SetText(_(L"Note: not all grades have been\n"
                                         "entered yet for last week."));
        // remove default titles
        linePlot->GetBottomXAxis().GetTitle().SetText(L"");
        linePlot->GetLeftYAxis().GetTitle().SetText(L"");

        // customize the X axis labels
        for (int i = 1; i < 6; ++i)
            {
            linePlot->GetBottomXAxis().SetCustomLabel(
                i, Label(wxString::Format(
                       /* TRANSLATORS: Week # of the school year */
                       _(L"Week %i"), i)));
            }

        // add a red background for failing grades
        // (note that this will appear on the legend and the plot)
        linePlot->AddReferenceArea(
            ReferenceArea(AxisType::LeftYAxis, 0, 59, _(L"Failing"), *wxRED));

        // add the line plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);

        // add a legend to the side and center it vertically
        auto legend = linePlot->CreateLegend(LegendOptions().IncludeHeader(false).PlacementHint(
            LegendCanvasPlacementHint::RightOfGraph));
        legend->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        subframe->m_canvas->SetFixedObject(0, 1, std::move(legend));

        // to add another right-aligned legend under the graph, uncomment the following:

        // subframe->m_canvas->SetFixedObjectsGridSize(2, 2);
        // legend = linePlot->CreateLegend(LegendCanvasPlacementHint::AboveOrBeneathGraph);
        // legend->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
        // subframe->m_canvas->SetFixedObject(1, 0, legend);
        // subframe->m_canvas->CalcRowDimensions();

        // add a watermark to the bottom right corner
        subframe->m_canvas->SetWatermarkLogo(
            wxBitmapBundle::FromSVGFile(appDir + L"/res/wisteria.svg",
                                        Image::GetSVGSize(appDir + L"/res/wisteria.svg")),
            wxSize(32, 32));
        }
    // Gantt Chart
    else if (event.GetId() == MyApp::ID_NEW_GANTT)
        {
        subframe->SetTitle(_(L"Gantt Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);

        auto companyAcquisitionData = std::make_shared<Data::Dataset>();
        try
            {
            const auto datasetPath{ appDir + L"/datasets/economics/company_acquisition.csv" };
            companyAcquisitionData->ImportCSV(
                datasetPath,
                // preview the data and deduce how to import it
                Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(datasetPath)));
            // we could also import the dataset by explicitly defining the columns, as such:
            /*companyAcquisitionData->ImportCSV(datasetPath,
                ImportInfo().
                ContinuousColumns({ L"Completion" }).
                DateColumns({
                    { L"Start", DateImportMethod::Automatic, wxEmptyString },
                    { L"End", DateImportMethod::Automatic, wxEmptyString }
                    }).
                CategoricalColumns({
                    { L"Task" },
                    { L"Description" },
                    { L"Resource" }
                    }));*/
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto ganttChart =
            std::make_shared<GanttChart>(subframe->m_canvas,
                                         // use a different color scheme where the colors
                                         // stand out more from each other
                                         std::make_shared<Colors::Schemes::Decade1920s>());
        ganttChart->SetData(companyAcquisitionData, DateInterval::FiscalQuarterly,
                            FiscalYear::USBusiness, L"Task", L"Start", "End",
                            // these columns are optional
                            L"Resource", L"Description", L"Completion", L"Resource");

        // add deadlines
        auto releaseDate =
            ganttChart->GetScalingAxis().FindDatePosition(wxDateTime(25, wxDateTime::Dec, 2022));
        if (releaseDate)
            {
            ganttChart->AddReferenceLine(
                ReferenceLine(AxisType::BottomXAxis, releaseDate.value(), _(L"Release"),
                              ColorBrewer::GetColor(Colors::Color::TractorRed)));
            }

        auto updateReleaseDate =
            ganttChart->GetScalingAxis().FindDatePosition(wxDateTime(15, wxDateTime::Mar, 2023));
        if (updateReleaseDate)
            {
            ganttChart->AddReferenceLine(ReferenceLine(
                AxisType::BottomXAxis, updateReleaseDate.value(), _(L"Hotfix Release"),
                ColorBrewer::GetColor(Colors::Color::TractorRed,
                                      Wisteria::Settings::GetTranslucencyValue())));
            }

        ganttChart->SetCanvasMargins(5, 5, 5, 5);
        subframe->m_canvas->SetFixedObject(0, 0, ganttChart);
        // add a legend, showing whom is assigned to which tasks
        subframe->m_canvas->SetFixedObject(
            0, 1,
            ganttChart->CreateLegend(LegendOptions().IncludeHeader(false).PlacementHint(
                LegendCanvasPlacementHint::RightOfGraph)));
        }
    else if (event.GetId() == MyApp::ID_NEW_CANDLESTICK_AXIS)
        {
        subframe->SetTitle(_(L"Candlestick Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto silverFuturesData = std::make_shared<Data::Dataset>();
        try
            {
            const auto datasetPath{ appDir + L"/datasets/economics/silver_futures.csv" };
            silverFuturesData->ImportCSV(
                datasetPath,
                // preview the data and deduce how to import it
                Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(datasetPath)));
            // we could also import the dataset by explicitly defining the columns, as such:
            /* silverFuturesData->ImportCSV(datasetPath,
                ImportInfo().
                ContinuousColumns({ L"Open", L"High", L"Low", L"Close/Last" }).
                DateColumns({ { L"Date", DateImportMethod::Automatic, wxEmptyString } }));*/
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto candlestickPlot = std::make_shared<CandlestickPlot>(subframe->m_canvas);
        // Plot's left axis will start at zero by default so that the scale
        // isn't misleading; you can, however, turn that off like this
        // to better see the daily activity.
        // This should be done before calling SetData() so that it bases
        // axis range on the data.
        candlestickPlot->GetLeftYAxis().StartAtZero(false);

        // Uncomment this to fit the entire year onto the canvas
        // so that there isn't a scrollbar.
        // candlestickPlot->SetPointsPerDefaultCanvasSize(365);

        candlestickPlot->SetData(
            silverFuturesData, L"Date", L"Open", L"High", L"Low",
            _DT(L"Close/Last", DTExplanation::Syntax, L"Name of variable from dataset"));

        candlestickPlot->GetTitle().SetText(_(L"Silver COMEX 2021 Trend"));

        candlestickPlot->SetCanvasMargins(5, 5, 5, 5);
        subframe->m_canvas->SetFixedObject(0, 0, candlestickPlot);
        }
    // Bar Chart
    else if (event.GetId() == MyApp::ID_NEW_BARCHART)
        {
        subframe->SetTitle(_(L"Bar Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto plot = std::make_shared<BarChart>(subframe->m_canvas);

        // make it a horizontal barchart
        plot->SetBarOrientation(Orientation::Horizontal);

        auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

        plot->AddBar(
            BarChart::Bar(1, { BarChart::BarBlock(BarChart::BarBlockInfo(92).Brush(barColor)) },
                          L"", Label(_(L"Bugs")), BoxEffect::Solid));

        plot->AddBar(
            BarChart::Bar(2, { BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor)) },
                          L"", Label(_(L"Pending feature requests")), BoxEffect::Solid));

        plot->AddBar(
            BarChart::Bar(3, { BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor)) },
                          L"", Label(_(L"Unfinished help topics")), BoxEffect::Solid));

        plot->AddBar(
            BarChart::Bar(4, { BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor)) },
                          L"", Label(_(L"Missing unit tests")), BoxEffect::Solid));

        plot->IncludeSpacesBetweenBars();

        // only show the labels on the axis
        plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);

        plot->GetBarAxis().GetTitle().GetGraphItemInfo().Text(L"ISSUES");

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart (Stylized)
    else if (event.GetId() == MyApp::ID_NEW_BARCHART_STYLIZED)
        {
        subframe->SetTitle(_(L"Bar Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto plot = std::make_shared<BarChart>(subframe->m_canvas);

        // make it a horizontal barchart
        plot->SetBarOrientation(Orientation::Horizontal);

        auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

        plot->AddBar(
            BarChart::Bar(1,
                          { // this bar will have two sections to it, where a red section
                            // refers to the more critical bugs
                            BarChart::BarBlock(BarChart::BarBlockInfo(22).Brush(*wxRED)),
                            BarChart::BarBlock(BarChart::BarBlockInfo(72).Brush(barColor)) },
                          L"", Label(_(L"Bugs")), BoxEffect::Glassy,
                          // we will make the width of the bar twice as wide as the others
                          // to show how important it is
                          wxALPHA_OPAQUE, 2));

        // Note that because the first bar has an unusual width, this will offset
        // the positions of the following bars. Therefore, we need to place them
        // at positions like 2.5, 3.5, etc. Normally, they would just go on points like 2 or 3.
        plot->AddBar(
            BarChart::Bar(2.5, { BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor)) },
                          L"", Label(_(L"Pending feature requests")), BoxEffect::Glassy,
                          // this bar will be translucent
                          75, 1));

        plot->AddBar(BarChart::Bar(
            3.5, { BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor)) }, L"",
            Label(_(L"Unfinished help topics")), BoxEffect::Glassy, wxALPHA_OPAQUE, 1));

        plot->AddBar(BarChart::Bar(
            4.5, { BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor)) }, L"",
            Label(_(L"Missing unit tests")), BoxEffect::Glassy, wxALPHA_OPAQUE, 1));

        // only show the labels on the axis
        plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        // force the custom labels set at points like 2.5 to be shown
        const auto [rangeStart, rangeEnd] = plot->GetBarAxis().GetRange();
        plot->GetBarAxis().SetRange(rangeStart, rangeEnd, 1, 0.5, 1);

        plot->GetBarAxis()
            .GetTitle()
            .GetGraphItemInfo()
            .Text(L"ISSUES")
            .Orient(Orientation::Horizontal)
            .Padding(5, 10, 0, 0)
            .LabelAlignment(TextAlignment::Centered);
        plot->GetBarAxis().GetTitle().SplitTextByCharacter();

        // align the axis labels over to the left
        plot->GetBarAxis().SetPerpendicularLabelAxisAlignment(
            AxisLabelAlignment::AlignWithBoundary);

        plot->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart (common image)
    else if (event.GetId() == MyApp::ID_NEW_BARCHART_IMAGE)
        {
        subframe->SetTitle(_(L"Bar Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto plot = std::make_shared<BarChart>(subframe->m_canvas);

        // make it a horizontal barchart
        plot->SetBarOrientation(Orientation::Vertical);

        // Photo by ThisisEngineering RAEng on Unsplash
        auto bgImage = GraphItems::Image::LoadFile(
            appDir + L"/res/thisisengineering-raeng-64YrPKiguAE-unsplash.jpg");
        plot->SetImageScheme(std::make_shared<Wisteria::Images::Schemes::ImageScheme>(
            std::vector<wxBitmapBundle>{ wxBitmapBundle(bgImage) }));
        // To create a selective colorization effect with the bars, uncomment the following:
        //
        // bgImage = GraphItems::Image::ApplyEffect(Wisteria::ImageEffect::Grayscale, bgImage);
        // GraphItems::Image::SetOpacity(bgImage, 75, false);
        // plot->SetPlotBackgroundImage(bgImage);
        // plot->SetPlotBackgroundImageFit(ImageFit::CropAndCenter);

        auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

        plot->AddBar(
            BarChart::Bar(1,
                          { // this bar will have two sections to it, where a red section
                            // refers to the more critical bugs
                            BarChart::BarBlock(BarChart::BarBlockInfo(22).Brush(*wxRED)),
                            BarChart::BarBlock(BarChart::BarBlockInfo(72).Brush(barColor)) },
                          L"", Label(_(L"Bugs")), BoxEffect::CommonImage));

        plot->AddBar(
            BarChart::Bar(2, { BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor)) },
                          L"", Label(_(L"Pending feature requests")), BoxEffect::CommonImage));

        plot->AddBar(
            BarChart::Bar(3, { BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor)) },
                          L"", Label(_(L"Unfinished help topics")), BoxEffect::CommonImage));

        plot->AddBar(
            BarChart::Bar(4, { BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor)) },
                          L"", Label(_(L"Missing unit tests")), BoxEffect::CommonImage));

        // only show the labels on the axis
        plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        // force the custom labels set at points like 2.5 to be shown
        const auto [rangeStart, rangeEnd] = plot->GetBarAxis().GetRange();
        plot->GetBarAxis().SetRange(rangeStart, rangeEnd, 1);

        plot->GetBarAxis().GetTitle().GetGraphItemInfo().Text(L"ISSUES").Padding(5, 10, 0, 0);

        // align the axis labels over to the left
        plot->GetBarAxis().SetPerpendicularLabelAxisAlignment(
            AxisLabelAlignment::AlignWithBoundary);

        plot->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart, using the dataset interface to aggregate labels
    else if (event.GetId() == MyApp::ID_NEW_CATEGORICAL_BARCHART)
        {
        subframe->SetTitle(_(L"Bar Chart (Categorical Data)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(appDir + L"/datasets/mpg.csv",
                               ImportInfo().CategoricalColumns(
                                   { { L"manufacturer", CategoricalImportMethod::ReadAsStrings },
                                     { L"model", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<CategoricalBarChart>(
            subframe->m_canvas,
            std::make_shared<Brushes::Schemes::BrushScheme>(Colors::Schemes::Decade1980s()));

        plot->SetData(mpgData, L"manufacturer");

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart, using the dataset interface to aggregate labels,
    // along with a grouping column
    else if (event.GetId() == MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED)
        {
        subframe->SetTitle(_(L"Bar Chart (Categorical Data, Grouped)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(appDir + L"/datasets/mpg.csv",
                               ImportInfo().CategoricalColumns(
                                   { { L"manufacturer", CategoricalImportMethod::ReadAsStrings },
                                     { L"class", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<CategoricalBarChart>(
            subframe->m_canvas,
            std::make_shared<Brushes::Schemes::BrushScheme>(Colors::Schemes::Decade1980s()));

        plot->SetData(mpgData, L"manufacturer", std::nullopt, L"class");
        plot->SetBarOpacity(220);
        plot->SetBarEffect(BoxEffect::Glassy);

        subframe->m_canvas->SetFixedObject(0, 0, plot);

        subframe->m_canvas->SetFixedObject(
            0, 1,
            plot->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
                LegendCanvasPlacementHint::RightOfGraph)));
        }
    // Bar Chart using a stipple icon
    else if (event.GetId() == MyApp::ID_NEW_CATEGORICAL_BARCHART_STIPPLED)
        {
        subframe->SetTitle(_(L"Bar Chart (Stipple Icon)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(
                appDir + L"/datasets/mpg.csv",
                ImportInfo().CategoricalColumns(
                    { { L"manufacturer", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto plot = std::make_shared<CategoricalBarChart>(subframe->m_canvas);

        plot->SetData(mpgData, L"manufacturer");

        plot->SetBarEffect(BoxEffect::StippleShape);
        plot->SetStippleShape(Icons::IconShape::Car);
        plot->SetStippleShapeColor(wxColour(29, 29, 37));

        // do this to use an image instead of a built-in vector icon:
        /* plot->SetStippleBrush(wxBitmapBundle::FromSVGFile(appDir +
           L"/res/tobias_Blue_Twingo.svg", Image::GetSVGSize(appDir +
           L"/res/tobias_Blue_Twingo.svg")));

           plot->SetBarEffect(BoxEffect::StippleImage);*/

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Pie Chart
    else if (event.GetId() == MyApp::ID_NEW_PIECHART)
        {
        subframe->SetTitle(_(L"Pie Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(appDir + L"/datasets/institutional_research/fall_enrollment.csv",
                               ImportInfo()
                                   .ContinuousColumns({ L"Enrollment" })
                                   .CategoricalColumns(
                                       { { L"Course", CategoricalImportMethod::ReadAsStrings },
                                         { L"COLLEGE", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(),
                                    PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            {
            foundSlice->SetDescription(_(L"Includes both literary and composition courses"));
            }

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Donut Chart
    else if (event.GetId() == MyApp::ID_NEW_DONUTCHART)
        {
        subframe->SetTitle(_(L"Donut Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(appDir + L"/datasets/institutional_research/fall_enrollment.csv",
                               ImportInfo()
                                   .ContinuousColumns({ L"Enrollment" })
                                   .CategoricalColumns(
                                       { { L"Course", CategoricalImportMethod::ReadAsStrings },
                                         { L"COLLEGE", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(),
                                    PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            {
            foundSlice->SetDescription(_(L"Includes both literary and composition courses"));
            }

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);
        // add a donut hole
        plot->IncludeDonutHole(true);
        plot->GetDonutHoleLabel().SetText(_(L"Enrollment\nFall 2023"));
        plot->SetDonutHoleProportion(.5);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Pie Chart (with Subgroup)
    else if (event.GetId() == MyApp::ID_NEW_PIECHART_GROUPED)
        {
        subframe->SetTitle(_(L"Pie Chart (with Subgroup)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(appDir + L"/datasets/institutional_research/fall_enrollment.csv",
                               ImportInfo()
                                   .ContinuousColumns({ L"Enrollment" })
                                   .CategoricalColumns(
                                       { { L"Course", CategoricalImportMethod::ReadAsStrings },
                                         { L"COLLEGE", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(),
                                    PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            {
            foundSlice->SetDescription(_(L"Includes both literary and composition courses"));
            }
        // turn off all but one of the outer labels for the inner ring
        // to draw attention to it
        plot->ShowInnerPieLabels(true, { L"Visual Basic.NET" });

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // add a legend for the inner ring (i.e., the subgroup column,
        // which will also show headers for their parent groups)
        subframe->m_canvas->SetFixedObject(
            0, 1,
            plot->CreateLegend(LegendOptions()
                                   .RingPerimeter(Perimeter::Inner)
                                   .PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        }
    // Donut Chart (with Subgroup)
    else if (event.GetId() == MyApp::ID_NEW_DONUTCHART_GROUPED)
        {
        subframe->SetTitle(_(L"Donut Chart (with Subgroup)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(
                appDir + L"/datasets/institutional_research/fall_enrollment.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Enrollment" })
                    .CategoricalColumns({ { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                                          { L"Course", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        // hide all outer labels for the main (i.e., outer) ring
        plot->ShowOuterPieLabels(false);
        // show one of the outer labels for the inner ring
        // and add a custom description to it
        std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
                      [](auto& slice)
                      {
                          if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") == 0)
                              {
                              slice.ShowGroupLabel(true);
                              slice.SetDescription(_(L"Drop this from the catalog?"));
                              }
                      });
        // place the label around the pie, not off to the side
        plot->SetLabelPlacement(LabelPlacement::NextToParent);

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);
        // add a donut hole
        plot->IncludeDonutHole(true);
        plot->GetDonutHoleLabel().SetText(_(L"Enrollment\nFall 2023"));
        plot->SetDonutHoleProportion(.8);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // add a legend for the inner ring (i.e., the subgroup column,
        // which will also show headers for their parent groups)
        subframe->m_canvas->SetFixedObject(
            0, 1,
            plot->CreateLegend(LegendOptions()
                                   .RingPerimeter(Perimeter::Inner)
                                   .PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        }
    // Sankey Diagram
    else if (event.GetId() == MyApp::ID_NEW_SANKEY_DIAGRAM)
        {
        subframe->SetTitle(_(L"Sankey Diagram"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto sankeyData = std::make_shared<Data::Dataset>();
        try
            {
            sankeyData->ImportCSV(
                appDir + L"/datasets/historical/titanic.csv",
                ImportInfo().CategoricalColumns(
                    { { L"Sex", CategoricalImportMethod::ReadAsStrings },
                      { L"Embarked", CategoricalImportMethod::ReadAsStrings },
                      { L"Pclass", CategoricalImportMethod::ReadAsIntegers },
                      { L"Survived", CategoricalImportMethod::ReadAsIntegers } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto sankey = std::make_shared<SankeyDiagram>(subframe->m_canvas);
        sankey->SetData(sankeyData, L"Sex", L"Survived", std::nullopt, std::nullopt, std::nullopt);
        sankey->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, sankey);
        }
    // Grouped Sankey Diagram
    else if (event.GetId() == MyApp::ID_NEW_GROUPED_SANKEY_DIAGRAM)
        {
        subframe->SetTitle(_(L"Grouped Sankey Diagram"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto sankeyData = std::make_shared<Data::Dataset>();
        try
            {
            sankeyData->ImportCSV(
                appDir + L"/datasets/institutional_research/hs_graduate_matriculation.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Graduated", L"Enrolled" })
                    .CategoricalColumns({ { L"County" },
                                          { _DT(L"High School", DTExplanation::Syntax,
                                                L"Name of variable from dataset") },
                                          { L"University" } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto sankey = std::make_shared<SankeyDiagram>(subframe->m_canvas);
        sankey->SetData(
            sankeyData,
            _DT(L"High School", DTExplanation::Syntax, L"Name of variable from dataset"),
            L"University", L"Graduated", L"Enrolled", L"County");
        sankey->SetGroupLabelDisplay(BinLabelDisplay::BinNameAndValue);
        sankey->SetColumnHeaderDisplay(GraphColumnHeader::AsHeader);
        sankey->SetColumnHeaders({ _(L"Of @COUNT@ High School Graduates"),
                                   _(L"@COUNT@ Enrolled at Miskatonic University") });
        sankey->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, sankey);
        }
    // Word Cloud
    else if (event.GetId() == MyApp::ID_NEW_WORD_CLOUD)
        {
        subframe->SetTitle(_(L"Word Cloud"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto friendsData = std::make_shared<Data::Dataset>();
        try
            {
            friendsData->ImportCSV(
                appDir + L"/datasets/social/friends descriptions.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Frequency" })
                    .CategoricalColumns({ { L"Word", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto wordCloud = std::make_shared<WordCloud>(subframe->m_canvas);
        // remove the low-frequency words, and also the extreme high frequency
        // ones to remove the main characters
        wordCloud->SetData(friendsData, L"Word", L"Frequency", 2, 100, 25);
        wordCloud->GetTitle()
            .GetGraphItemInfo()
            .Padding(5, 5, 25, 5)
            .Text(_(L"Top Words from Episode Descriptions of the Sitcom 'Friends'"));
        wordCloud->GetTitle().GetFont().MakeBold();

        wordCloud->GetCaption()
            .GetGraphItemInfo()
            .Padding(25, 5, 5, 5)
            .Text(_(L"Note: main characters (Rachel, Ross, Monica, Chandler, Joey, & Phoebe) "
                    "and common words have been excluded."));

        wordCloud->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, wordCloud);
        }
    // Linear Regression Roadmap
    else if (event.GetId() == MyApp::ID_NEW_LR_ROADMAP_GRAPH)
        {
        subframe->SetTitle(_(L"Linear Regression Roadmap"));
        subframe->m_canvas->SetFixedObjectsGridSize(2, 1);

        auto roadmapData = std::make_shared<Data::Dataset>();
        try
            {
            roadmapData->ImportCSV(
                appDir + L"/datasets/institutional_research/first-year_osprey.csv",
                ImportInfo()
                    .ContinuousColumns({ L"coefficient" })
                    .CategoricalColumns({ { L"factor", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto roadmap = std::make_shared<LRRoadmap>(subframe->m_canvas);
        roadmap->SetData(roadmapData, L"factor", L"coefficient", std::nullopt, std::nullopt,
                         std::nullopt, // TRANSLATORS: Grade Point Average
                         _(L"GPA"));
        roadmap->SetCanvasMargins(5, 5, 5, 5);
        // add the default caption explaining how to read the graph
        roadmap->AddDefaultCaption();
        roadmap->GetTitle().SetText(_(
            L"First-Year Osprey Roadmap\n"
            "How do background characteristics and decisions affect First - Year Students' GPA?"));
        // add a title with a blue banner background and white font
        roadmap->GetTitle().GetHeaderInfo().Enable(true).FontColor(*wxWHITE).GetFont().MakeBold();
        roadmap->GetTitle().SetPadding(5, 5, 5, 5);
        roadmap->GetTitle().SetFontColor(*wxWHITE);
        roadmap->GetTitle().SetFontBackgroundColor(ColorBrewer::GetColor(Colors::Color::NavyBlue));

        subframe->m_canvas->SetFixedObject(0, 0, roadmap);

        // add the legend at the bottom (beneath the explanatory caption)
        auto legend = roadmap->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
            LegendCanvasPlacementHint::AboveOrBeneathGraph));
        subframe->m_canvas->SetFixedObject(1, 0, std::move(legend));

        subframe->m_canvas->CalcRowDimensions();
        }
    // SWOT Roadmap
    else if (event.GetId() == MyApp::ID_NEW_PROCON_ROADMAP_GRAPH)
        {
        subframe->SetTitle(_(L"Pros & Cons Roadmap"));
        subframe->m_canvas->SetFixedObjectsGridSize(3, 1);

        auto swData = std::make_shared<Data::Dataset>();
        try
            {
            swData->ImportCSV(appDir + L"/datasets/economics/erp_migration_survey.csv",
                              ImportInfo().CategoricalColumns(
                                  { { L"Strength", CategoricalImportMethod::ReadAsStrings },
                                    { L"Weakness", CategoricalImportMethod::ReadAsStrings },
                                    { L"Opportunity", CategoricalImportMethod::ReadAsStrings },
                                    { L"Threat", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        // strengths and weaknesses
        auto SWroadmap = std::make_shared<ProConRoadmap>(subframe->m_canvas);
        SWroadmap->SetData(swData, L"Strength", std::nullopt, L"Weakness", std::nullopt, 2);
        SWroadmap->SetCanvasMargins(5, 5, 0, 5);
        SWroadmap->GetLeftYAxis().GetTitle().SetText(_(L"Strengths & Weaknesses"));
        SWroadmap->GetLeftYAxis().GetTitle().SetMinimumUserSizeDIPs(30, std::nullopt);
        // don't include the counts on the labels
        SWroadmap->SetMarkerLabelDisplay(Roadmap::MarkerLabelDisplay::Name);
        // use road signs and a white road line
        SWroadmap->SetRoadStopTheme(Roadmap::RoadStopTheme::RoadSigns);
        SWroadmap->GetLaneSeparatorPen().SetColour(*wxWHITE);

        // opportunities and threats
        auto OTroadmap = std::make_shared<ProConRoadmap>(subframe->m_canvas);
        OTroadmap->SetData(swData, L"Opportunity", std::nullopt, L"Threat", std::nullopt,
                           // ignore items that are only mentioned once
                           2);
        OTroadmap->SetCanvasMargins(0, 5, 5, 5);
        OTroadmap->GetLeftYAxis().GetTitle().SetText(_(L"Opportunities & Threats"));
        OTroadmap->GetLeftYAxis().GetTitle().SetMinimumUserSizeDIPs(30, std::nullopt);
        // add the default caption explaining how to read the graph
        OTroadmap->AddDefaultCaption();
        // don't include the counts on the labels
        OTroadmap->SetMarkerLabelDisplay(Roadmap::MarkerLabelDisplay::Name);
        // use road signs and a white road line
        OTroadmap->SetRoadStopTheme(Roadmap::RoadStopTheme::RoadSigns);
        OTroadmap->GetLaneSeparatorPen().SetColour(*wxWHITE);

        // add the legend at the bottom (beneath the explanatory caption)
        OTroadmap->SetPositiveLegendLabel(_(L"Strengths & Opportunities"));
        OTroadmap->SetNegativeLegendLabel(_(L"Weaknesses & Threats"));
        auto legend = OTroadmap->CreateLegend(LegendOptions().IncludeHeader(true).PlacementHint(
            LegendCanvasPlacementHint::AboveOrBeneathGraph));

        // add a title with a green banner background and white font
        Label topTitle(
            GraphItemInfo(
                _(L"ERP Migration SWOT Analysis\n"
                  "Employee Survey Results Regarding Proposed Migration to new ERP Software"))
                .Padding(5, 5, 5, 5)
                .ChildAlignment(RelativeAlignment::FlushLeft)
                .FontColor(*wxWHITE)
                .FontBackgroundColor(ColorBrewer::GetColor(Colors::Color::HunterGreen)));
        topTitle.GetHeaderInfo().Enable(true).FontColor(*wxWHITE).GetFont().MakeBold();
        subframe->m_canvas->GetTopTitles().push_back(topTitle);

        // set a common scale for the road stop sizes between the two roadmaps
        SWroadmap->SetMagnitude(std::max(SWroadmap->GetMagnitude(), OTroadmap->GetMagnitude()));
        OTroadmap->SetMagnitude(std::max(SWroadmap->GetMagnitude(), OTroadmap->GetMagnitude()));

        // add everything to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, SWroadmap);
        subframe->m_canvas->SetFixedObject(1, 0, OTroadmap);
        subframe->m_canvas->SetFixedObject(2, 0, std::move(legend));
        subframe->m_canvas->GetRowInfo(2).LockProportion(true);

        subframe->m_canvas->CalcRowDimensions();

        // make the canvas tall since we are stacking two graphs on top of each other
        subframe->m_canvas->SetCanvasMinHeightDIPs(
            subframe->m_canvas->GetDefaultCanvasHeightDIPs() * 2);
        // also, fit it to the entire page when printing (preferably in portrait)
        subframe->m_canvas->FitToPageWhenPrinting(true);
        }
    // W-Curve plot
    else if (event.GetId() == MyApp::ID_NEW_WCURVE)
        {
        subframe->SetTitle(_(L"W-Curve Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto wcurveData = std::make_shared<Data::Dataset>();
        try
            {
            wcurveData->ImportCSV(
                appDir + L"/datasets/institutional_research/sense_of_belonging.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Year", L"Belong" })
                    .CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto WCurve = std::make_shared<WCurvePlot>(subframe->m_canvas,
                                                   std::make_shared<Colors::Schemes::EarthTones>());
        // add padding around the plot
        WCurve->SetCanvasMargins(5, 5, 5, 5);

        // set the data and use the grouping column from the dataset to create separate lines
        WCurve->SetData(wcurveData, L"Belong", L"Year", L"Name");
        WCurve->GetTopXAxis().GetTitle().SetText(
            _(L"THE TRANSITION OF FOUR STUDENTS USING THE W-CURVE"));
        WCurve->GetTopXAxis().GetTitle().SetBottomPadding(5);

        // Uncomment this to add a story-telling note at the bottom corner:

        /* auto storyNote = std::make_unique<Label>(
            GraphItemInfo(_(L"Frank reported that he experienced a"
                " \u201Cdownward spiral\u201D during his first year on campus.")).
            Anchoring(Anchoring::BottomLeftCorner).
            FontBackgroundColor(ColorBrewer::GetColor(Color::Canary)).
            LabelAlignment(TextAlignment::RaggedRight).
            LabelStyling(LabelStyle::DottedLinedPaper).Padding(4, 4, 4, 4));
        storyNote->GetFont().MakeSmaller();
        storyNote->SplitTextToFitLength(25);

        WCurve->AddAnnotation(storyNote,
            wxPoint(1, WCurve->GetLeftYAxis().GetRange().first));*/

        // add the line plot and its legend to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, WCurve);
        subframe->m_canvas->SetFixedObject(
            0, 1,
            WCurve->CreateLegend(LegendOptions().IncludeHeader(false).PlacementHint(
                LegendCanvasPlacementHint::RightOfGraph)));
        }
    // Likert (3-Point)
    else if (event.GetId() == MyApp::ID_NEW_LIKERT_3POINT)
        {
        subframe->SetTitle(_(L"Likert Chart (3-Point Scale, with Grouping)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        // import the dataset (this is available in the "datasets" folder)
        auto surveyData = std::make_shared<Data::Dataset>();
        try
            {
            const auto datasetPath{ appDir + L"/datasets/Graph Library Survey.csv" };
            surveyData->ImportCSV(
                datasetPath,
                // preview the data and deduce how to import it
                Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(datasetPath)));
            // we could also import the dataset by explicitly defining the columns, as such:
            /* surveyData->ImportCSV(datasetPath,
                Data::ImportInfo().
                    CategoricalColumns(
                        {
                        { L"Gender" },
                        { L"I am happy with my current graphics library",
                          CategoricalImportMethod::ReadAsIntegers },
                        { L"Customization is important to me",
                          CategoricalImportMethod::ReadAsIntegers },
                        { L"A simple API is important to me",
                          CategoricalImportMethod::ReadAsIntegers },
                        { L"Support for obscure graphs is important to me",
                          CategoricalImportMethod::ReadAsIntegers },
                        { L"Extensibility is important to me",
                          CategoricalImportMethod::ReadAsIntegers },
                        { LR"(Standard, "out-of-the-box" graph support is important to me)",
                          CategoricalImportMethod::ReadAsIntegers },
                        { L"Data importing features are important to me",
                          CategoricalImportMethod::ReadAsIntegers }
                        }));*/
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        // Original data has a scale going from 1-7, but we want to simplify
        // it to 1-3. To do this, we will collapse all the positive levels
        // into one, and all negative levels into another level.
        auto categoricalNames{ surveyData->GetCategoricalColumnNames() };
        Dataset::RemoveColumnNamesFromList(categoricalNames, { L"Gender" });
        const auto responsesScale = LikertChart::Simplify(
            surveyData, categoricalNames, LikertChart::LikertSurveyQuestionFormat::SevenPoint);

        /* Simplify() will use stock labels for the responses.
           To change these, do the following:
        const ColumnWithStringTable::StringTableType codes =
            {
                { 0, L"No answer" },
                { 1, L"Negative" },
                { 2, L"Neither" },
                { 3, L"Positive" }
            };

        LikertChart::SetLabels(surveyData, codes);*/

        auto likertChart = std::make_shared<LikertChart>(
            subframe->m_canvas,
            // Simplify() will return LikertChart::LikertSurveyQuestionFormat::ThreePoint
            responsesScale);
        likertChart->SetData(
            surveyData, categoricalNames,
            // passing in a grouping column will change it from ThreePoint -> ThreePointCategorized
            L"Gender");

        // groups with lower responses will have narrower bars
        likertChart->SetBarSizesToRespondentSize(true);

        likertChart->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, likertChart);
        }
    // Likert (7-Point)
    else if (event.GetId() == MyApp::ID_NEW_LIKERT_7POINT)
        {
        subframe->SetTitle(_(L"Likert Chart (7-Point Scale)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);

        // import the dataset (this is available in the "datasets" folder)
        auto surveyData = std::make_shared<Data::Dataset>();
        try
            {
            const auto datasetPath = appDir + L"/datasets/Graph Library Survey.csv";
            surveyData->ImportCSV(
                datasetPath, Dataset::ImportInfoFromPreview(Dataset::ReadColumnInfo(datasetPath)));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto categoricalNames{ surveyData->GetCategoricalColumnNames() };
        Dataset::RemoveColumnNamesFromList(categoricalNames, { L"Gender" });

        // Because the responses in the dataset were coded 1-7, we will need to
        // add meaningful labels to the dataset. The following will add stock
        // labels to represent the responses.
        LikertChart::SetLabels(
            surveyData, categoricalNames,
            LikertChart::CreateLabels(LikertChart::LikertSurveyQuestionFormat::SevenPoint));

        auto likertChart = std::make_shared<LikertChart>(
            subframe->m_canvas, LikertChart::LikertSurveyQuestionFormat::SevenPoint);
        likertChart->SetData(surveyData, categoricalNames);

        // add brackets around some of the questions to group them
        likertChart->AddQuestionsBracket(LikertChart::QuestionsBracket{
            _DT(L"Customization is important to me", DTExplanation::Syntax,
                L"Name of variable from dataset"),
            _DT(L"Extensibility is important to me", DTExplanation::Syntax,
                L"Name of variable from dataset"),
            _(L"Advanced Features") });
        likertChart->AddQuestionsBracket(LikertChart::QuestionsBracket{
            _DT(LR"(Standard, "out-of-the-box" graph support is important to me)",
                DTExplanation::Syntax, L"Name of variable from dataset"),
            _DT(L"Data importing features are important to me", DTExplanation::Syntax,
                L"Name of variable from dataset"),
            _(L"Standard Features") });

        likertChart->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, likertChart);
        subframe->m_canvas->SetFixedObject(0, 1,
                                           likertChart->CreateLegend(LegendOptions().PlacementHint(
                                               LegendCanvasPlacementHint::RightOfGraph)));

        // when printing, make it landscape and stretch it to fill the entire page
        subframe->m_canvas->GetPrinterSettings().SetOrientation(wxPrintOrientation::wxLANDSCAPE);
        subframe->m_canvas->FitToPageWhenPrinting(true);
        }
    // Multiple plots
    else if (event.GetId() == MyApp::ID_NEW_MULTIPLOT)
        {
        subframe->SetTitle(_(L"Multiple Plots"));
        subframe->m_canvas->SetFixedObjectsGridSize(2, 2);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(
                appDir + L"/datasets/institutional_research/fall_enrollment.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Enrollment" })
                    .CategoricalColumns({ { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                                          { L"Course", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        auto donutChart = std::make_shared<PieChart>(subframe->m_canvas);
        donutChart->SetData(pieData, L"Enrollment", L"COLLEGE");

        // apply the slice's colors to its respective outside label
        donutChart->UseColorLabels(true);
        // add a donut hole
        donutChart->IncludeDonutHole(true);
        donutChart->GetDonutHoleLabel().SetText(_(L"Enrollment\nFall 2023"));
        donutChart->SetDonutHoleProportion(.5);

        subframe->m_canvas->SetFixedObject(0, 0, donutChart);

        // add a pie chart on the side, that will fill up the whole right side
        auto groupedPieChart = std::make_shared<PieChart>(subframe->m_canvas);
        groupedPieChart->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        groupedPieChart->SetOuterPieMidPointLabelDisplay(BinLabelDisplay::BinName);

        // bring attention to the smallest slices within each group
        groupedPieChart->ShowcaseSmallestInnerPieSlices(true, true);

        groupedPieChart->SetLabelPlacement(LabelPlacement::NextToParent);

        // apply the slice's colors to its respective outside label
        groupedPieChart->UseColorLabels(true);

        groupedPieChart->GetGraphItemInfo().CanvasHeightProportion(1);

        subframe->m_canvas->SetFixedObject(0, 1, groupedPieChart);

        // add a large note to the canvas
        // (into the second row, beneath the donut chart)
        auto note = std::make_shared<Label>(
            GraphItemInfo(_(L"NOTE\n"
                            "Should we consider dropping VB.NET from the catalog?\n"
                            "Enrollment has been really low the last few years."))
                .Padding(4, 4, 4, 4)
                .Scaling(2)
                .DPIScaling(subframe->m_canvas->GetDPIScaleFactor())
                .
            // will set the proportions of the note's row based on how tall the note is
            FitCanvasHeightToContent(true)
                .Pen(wxNullPen));
        // make the font smaller, and customize the header's appearance
        note->GetFont().MakeSmaller().MakeSmaller();
        note->GetHeaderInfo().Enable(true).FontColor(*wxBLUE).GetFont().MakeBold();
        note->GetHeaderInfo().GetFont().MakeSmaller();
        subframe->m_canvas->SetFixedObject(1, 0, note);

        // In the first column (the donut chart and the note beneath it),
        // this sets the proportions of the rows based on how tall the note is.
        // (This will happen because we enabled the FitCanvasHeightToContent() property
        // for the note above.)
        subframe->m_canvas->CalcRowDimensions();

        // set the canvas's print orientation to landscape
        subframe->m_canvas->GetPrinterSettings().SetOrientation(wxPrintOrientation::wxLANDSCAPE);
        }
    // Multiple plots with a common axis
    else if (event.GetId() == MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS)
        {
        subframe->SetTitle(_(L"Multiple Plots (Common Axis)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 3);
        auto spellingData = std::make_shared<Data::Dataset>();
        try
            {
            spellingData->ImportCSV(
                appDir + L"/datasets/Spelling Grades.csv",
                ImportInfo()
                    .ContinuousColumns({ L"Week", L"AVG_GRADE" })
                    .CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
        // create your own color scheme
        const auto colors = std::make_shared<Colors::Schemes::ColorScheme>(
            Colors::Schemes::ColorScheme{ ColorBrewer::GetColor(Colors::Color::GrannySmithApple),
                                          ColorBrewer::GetColor(Colors::Color::Auburn) });

        auto linePlot = std::make_shared<LinePlot>(
            subframe->m_canvas, colors,
            // use custom markers
            std::make_shared<IconScheme>(IconScheme{ IconShape::Diamond, IconShape::Hexagon }));

        // set the data and use the grouping column from the dataset to create separate lines
        linePlot->SetData(spellingData, L"AVG_GRADE", L"WeeK", L"Gender");

        // customize the X axis labels
        for (int i = 1; i < 6; ++i)
            {
            linePlot->GetBottomXAxis().SetCustomLabel(i,
                                                      Label(wxString::Format(
                                                          // TRANSLATORS: Week # of the school year
                                                          _(L"Week %i"), i)));
            }

        // instead of adding the legend to the canvas, overlay it on top of the line plot
        auto lineLegend = linePlot->CreateLegend(LegendOptions().IncludeHeader(false).PlacementHint(
            LegendCanvasPlacementHint::EmbeddedOnGraph));
        lineLegend->SetAnchoring(Anchoring::BottomRightCorner);
        linePlot->AddAnnotation(std::move(lineLegend),
                                wxPoint(linePlot->GetBottomXAxis().GetRange().second,
                                        linePlot->GetLeftYAxis().GetRange().first));

        // add the line plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);

        // create a box plot with the same data
        auto boxPlot = std::make_shared<BoxPlot>(
            subframe->m_canvas, std::make_shared<Brushes::Schemes::BrushScheme>(*colors));

        boxPlot->SetData(spellingData, L"AVG_GRADE", std::nullopt);

        // customize the box appearance
        boxPlot->SetBoxCorners(BoxCorners::Rounded);
        boxPlot->GetLeftYAxis().GetAxisLinePen() = wxNullPen;

        // add the box plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 1, boxPlot);

        subframe->m_canvas->SetFixedObject(0, 2,
                                           // construct a common axis connected to the line and box
                                           // plots, and add it to the right of them on the canvas
                                           CommonAxisBuilder::BuildYAxis(subframe->m_canvas,
                                                                         { linePlot, boxPlot },
                                                                         AxisType::RightYAxis));

        // add a centered title and subtitle on the canvas
        // (above the plots)
        subframe->m_canvas->GetTopTitles().emplace_back(_(L"Average Grades"));
        subframe->m_canvas->GetTopTitles().emplace_back(
            Label(GraphItemInfo(_(L"Average grades taken from last 5 weeks' spelling tests."))
                      .FontColor(ColorBrewer::GetColor(Color::DarkGray))
                      .Pen(wxNullPen)
                      .Font(subframe->m_canvas->GetTopTitles().back().GetFont().MakeSmaller())));
        }
    // Table
    else if (event.GetId() == MyApp::ID_NEW_TABLE)
        {
        subframe->SetTitle(_(L"Table"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto juniorSeniorMajors = std::make_shared<Data::Dataset>();
        try
            {
            juniorSeniorMajors->ImportCSV(
                appDir + L"/datasets/institutional_research/junior_&_senior_majors(pop_20).csv",
                ImportInfo()
                    .ContinuousColumns({ L"Female", L"Male" })
                    .CategoricalColumns({ { L"Division" }, { L"Department" } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        auto tableGraph = std::make_shared<Table>(subframe->m_canvas);
        tableGraph->SetData(juniorSeniorMajors, { L"Division", L"Department", L"Female", L"Male" });
        // group the schools together in the first row
        tableGraph->GroupColumn(0);

        // add ratio aggregate column and group row totals
        tableGraph->InsertAggregateColumn(Table::AggregateInfo(AggregateType::Ratio), _(L"Ratio"),
                                          std::nullopt);
        tableGraph->InsertRowTotals();

        // make the headers and row groups bold (and center the headers)
        tableGraph->BoldRow(0);
        tableGraph->BoldColumn(0);
        tableGraph->SetRowHorizontalPageAlignment(0, PageHorizontalAlignment::Centered);

        const auto& ratioOutliers =
            // Find outlier in the female-to-male ratios for the majors.
            // (Note that we use a more liberal search, considering
            // z-scores > 2 as being outliers
            tableGraph->GetOutliers(tableGraph->GetColumnCount() - 1, 2);
        // if any outliers, make a note of it off to the side
        if (ratioOutliers.size())
            {
            tableGraph->AddCellAnnotation(
                { _(L"Majors with the most lopsided female-to-male ratios"), ratioOutliers,
                  Side::Right, std::nullopt, wxColour() });
            }

        // if you also want to place annotations on the left of the table,
        // then center it within its drawing area like so:
        // tableGraph->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);

        // add a title
        subframe->m_canvas->GetTopTitles().push_back(
            Label(GraphItemInfo(_(L"Top 20 Majors for Juniors & Seniors (AY2021-22)"))
                      .Padding(5, 5, 5, 5)
                      .Pen(wxNullPen)
                      .ChildAlignment(RelativeAlignment::FlushLeft)
                      .FontBackgroundColor(ColorBrewer::GetColor(Color::MossGreen))));

        tableGraph->GetCaption().SetText(_(L"Source: Office of Institutional Research"));
        tableGraph->GetCaption().SetPadding(5, 5, 5, 5);

        // add the table to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, tableGraph);

        // make the canvas tall since it's a long table, but not very wide
        subframe->m_canvas->SetCanvasMinHeightDIPs(subframe->m_canvas->GetDefaultCanvasWidthDIPs());
        subframe->m_canvas->SetCanvasMinWidthDIPs(subframe->m_canvas->GetDefaultCanvasHeightDIPs());
        // also, fit it to the entire page when printing (preferably in portrait)
        subframe->m_canvas->FitToPageWhenPrinting(true);
        }

    subframe->Maximize(true);
    subframe->Show(true);
    }

void MyFrame::OnSaveWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        {
        return;
        }

    pChild->m_canvas->OnSave(event);
    }

void MyFrame::OnPrintWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        {
        return;
        }

    pChild->m_canvas->OnPrint(event);
    }

void MyFrame::OnPrintAll([[maybe_unused]] wxCommandEvent& event)
    {
    // get all the open canvases
    std::vector<Canvas*> canvases;
    const auto& windows = GetChildren();
    for (const auto& window : windows)
        {
        auto pChild = dynamic_cast<MyChild*>(window);
        if (pChild == nullptr)
            {
            continue;
            }
        canvases.push_back(pChild->m_canvas);
        }
    if (canvases.size() == 0)
        {
        return;
        }

    // add them to a report printer (using the first canvas's print settings)
    auto printOut = std::make_unique<ReportPrintout>(canvases, canvases[0]->GetLabel());
#if defined(__WXMSW__) || defined(__WXOSX__)
    wxPrinterDC dc = wxPrinterDC(canvases[0]->GetPrinterSettings());
#else
    wxPostScriptDC dc = wxPostScriptDC(canvases[0]->GetPrinterSettings());
#endif
    printOut->SetUp(dc);

    wxPrinter printer;
    printer.GetPrintDialogData().SetPrintData(canvases[0]->GetPrinterSettings());
    printer.GetPrintDialogData().SetAllPages(true);
    printer.GetPrintDialogData().SetFromPage(1);
    printer.GetPrintDialogData().SetToPage(canvases.size());
    if (!printer.Print(this, printOut.get(), true))
        {
        // just show a message if a real error occurred. They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK | wxICON_WARNING);
            }
        }
    }

void MyFrame::OnCopyWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        {
        return;
        }

    pChild->m_canvas->OnCopy(event);
    }

void MyFrame::OnCloseAll([[maybe_unused]] wxCommandEvent& event)
    {
    for (auto child : GetChildren())
        {
        if (child->IsKindOf(wxCLASSINFO(wxMDIChildFrame)))
            {
            child->Close();
            }
        }
    }

void MyFrame::OnClose([[maybe_unused]] wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        {
        return;
        }

    pChild->Close();
    }

void MyFrame::InitToolBar(wxToolBar* toolBar)
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxSize iconSize{ wxSize(16, 16) };

    toolBar->AddTool(MyApp::ID_NEW_BARCHART, _(L"Bar Chart"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize),
                     _(L"Bar Chart"));
    toolBar->AddTool(MyApp::ID_NEW_BARCHART_STYLIZED, _(L"Bar Chart (Stylized)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart-stylized.svg", iconSize),
                     _(L"Bar Chart (Stylized)"));
    toolBar->AddTool(MyApp::ID_NEW_BARCHART_IMAGE, _(L"Bar Chart (Commom Image)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart-image.svg", iconSize),
                     _(L"Bar Chart (Commom Image)"));
    toolBar->AddTool(MyApp::ID_NEW_CATEGORICAL_BARCHART, _(L"Bar Chart (Categorical Data)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize),
                     _(L"Bar Chart (Categorical Data)"));
    toolBar->AddTool(MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED,
                     _(L"Bar Chart (Categorical Data, Grouped)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize),
                     _(L"Bar Chart (Categorical Data, Grouped)"));
    toolBar->AddTool(MyApp::ID_NEW_CATEGORICAL_BARCHART_STIPPLED, _(L"Bar Chart (Stipple Icon)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize),
                     _(L"Bar Chart (Stipple Icon)"));

    toolBar->AddTool(MyApp::ID_NEW_PIECHART, _(L"Pie Chart"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/piechart.svg", iconSize),
                     _(L"Pie Chart"));
    toolBar->AddTool(
        MyApp::ID_NEW_PIECHART_GROUPED, _(L"Pie Chart (with Subgroup)"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/piechart-subgrouped.svg", iconSize),
        _(L"Pie Chart (with Subgroup)"));

    toolBar->AddTool(MyApp::ID_NEW_DONUTCHART, _(L"Donut Chart"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/donut.svg", iconSize),
                     _(L"Donut Chart"));
    toolBar->AddTool(MyApp::ID_NEW_DONUTCHART_GROUPED, _(L"Donut Chart (with Subgroup)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/donut-subgrouped.svg", iconSize),
                     _(L"Donut Chart (with Subgroup)"));

    toolBar->AddTool(MyApp::ID_NEW_HISTOGRAM, _(L"Histogram"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/histogram.svg", iconSize),
                     _(L"Histogram"));

    toolBar->AddTool(MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES,
                     _(L"Histogram (Discrete Category Counts)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/histogram.svg", iconSize),
                     _(L"Histogram (Discrete Category Counts)"));

    toolBar->AddTool(MyApp::ID_NEW_LINEPLOT, _(L"Line Plot"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/lineplot.svg", iconSize),
                     _(L"Line Plot"));
    toolBar->AddTool(MyApp::ID_NEW_LINEPLOT_CUSTOMIZED, _(L"Line Plot (Customized)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/lineplot-points.svg", iconSize),
                     _(L"Line Plot (Customized)"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_BOXPLOT, _(L"Box Plot"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/boxplot.svg", iconSize),
                     _(L"Box Plot"));
    toolBar->AddTool(MyApp::ID_NEW_HEATMAP, _(L"Heat Map"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/heatmap.svg", iconSize),
                     _(L"Heat Map"));
    toolBar->AddTool(MyApp::ID_NEW_HEATMAP_GROUPED, _(L"Heat Map (Grouped)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/heatmap-grouped.svg", iconSize),
                     _(L"Heat Map (Grouped)"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_SCALE_CHART, _(L"Scale Chart"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/scale.svg", iconSize),
                     _(L"Scale Chart"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_GANTT, _(L"Gantt Chart"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/gantt.svg", iconSize),
                     _(L"Gantt Chart"));

    toolBar->AddTool(MyApp::ID_NEW_CANDLESTICK_AXIS, _(L"Candlestick Plot"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/candlestick.svg", iconSize),
                     _(L"Candlestick Plot"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_LIKERT_3POINT, _(L"Likert Chart (3-Point Scale)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/likert3.svg", iconSize),
                     _(L"Likert Chart (3-Point Scale)"));
    toolBar->AddTool(MyApp::ID_NEW_LIKERT_7POINT, _(L"Likert Chart (7-Point Scale)"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/likert7.svg", iconSize),
                     _(L"Likert Chart (7-Point Scale)"));
    toolBar->AddTool(MyApp::ID_NEW_WCURVE, _(L"W-Curve Plot"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/wcurve.svg", iconSize),
                     _(L"W-Curve Plot"));
    toolBar->AddTool(MyApp::ID_NEW_LR_ROADMAP_GRAPH, _(L"Linear Regression Roadmap"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/roadmap.svg", iconSize),
                     _(L"Linear Regression Roadmap"));
    toolBar->AddTool(MyApp::ID_NEW_PROCON_ROADMAP_GRAPH, _(L"Pros & Cons Roadmap"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/roadmap.svg", iconSize),
                     _(L"Pros & Cons Roadmap"));
    toolBar->AddTool(MyApp::ID_NEW_SANKEY_DIAGRAM, _(L"Sankey Diagram"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/sankey.svg", iconSize),
                     _(L"Sankey Diagram"));
    toolBar->AddTool(MyApp::ID_NEW_GROUPED_SANKEY_DIAGRAM, _(L"Grouped Sankey Diagram"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/sankey.svg", iconSize),
                     _(L"Grouped Sankey Diagram"));
    toolBar->AddTool(MyApp::ID_NEW_WORD_CLOUD, _(L"Word Cloud"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/wordcloud.svg", iconSize),
                     _(L"Word Cloud"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_MULTIPLOT, _(L"Multiple Plots"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot.svg", iconSize),
                     _(L"Multiple Plots"));
    toolBar->AddTool(
        MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS, _(L"Multiple Plots (Common Axis)"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot-common-axis.svg", iconSize),
        _(L"Multiple Plots (Common Axis)"));
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_TABLE, _(L"Table"),
                     wxBitmapBundle::FromSVGFile(appDir + L"/res/spreadsheet.svg", iconSize),
                     _(L"Table"));

    toolBar->Realize();
    }

// ---------------------------------------------------------------------------
// MyChild
// ---------------------------------------------------------------------------

MyChild::MyChild(wxMDIParentFrame* parent) : wxMDIChildFrame(parent, wxID_ANY, L"")
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxSize iconSize = Image::GetSVGSize(appDir + L"/res/wisteria.svg");

    SetIcon(wxBitmapBundle::FromSVGFile(appDir + L"/res/wisteria.svg", iconSize).GetIcon(iconSize));

    // create our menu bar and associate it with the frame
    SetMenuBar(MyFrame::CreateMainMenubar());

    // this should work for MDI frames as well as for normal ones, provided
    // they can be resized at all
    if (!IsAlwaysMaximized())
        {
        SetSizeHints(FromDIP(200), FromDIP(200));
        }
    }
