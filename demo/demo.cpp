/////////////////////////////////////////////////////////////////////////////
// Name:        demo.cpp
// Purpose:     Wisteria Library Demo
// Author:      Blake Madden
// Created:     01/23/2022
// Copyright:   (c) Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
/////////////////////////////////////////////////////////////////////////////

#include "demo.h"

using namespace Wisteria;
using namespace Wisteria::Colors;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Data;

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
    if (!wxApp::OnInit() )
        return false;

    wxUILocale::UseDefault();

    wxInitAllImageHandlers();

    // enable this to route wxLog messages to a file
    // auto logFile = new LogFile;
    // delete wxLog::SetActiveTarget(logFile);

    // enable this to draw additional debug information:
    // Wisteria::Settings::EnableAllDebugFlags();

    MyFrame* frame = new MyFrame;
    frame->Show(true);

    return true;
    }

// ---------------------------------------------------------------------------
// MyFrame
// ---------------------------------------------------------------------------

// Define my frame constructor
MyFrame::MyFrame()
       : wxMDIParentFrame(nullptr, wxID_ANY, _(L"Wisteria Demo"),
                          wxDefaultPosition, wxSize(750, 500))
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
        windowMenu->SetLabel(wxID_MDI_WINDOW_TILE_HORZ,
                             _(L"Tile Horizontally\tCtrl-Shift-H"));
        windowMenu->SetLabel(wxID_MDI_WINDOW_TILE_VERT,
                             _(L"Tile Vertically\tCtrl-Shift-V"));

        // we can also change the help string
        windowMenu->SetHelpString(wxID_MDI_WINDOW_CASCADE,
                                  _(L"Arrange windows in cascade"));

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
    wxAcceleratorEntry entries[6];
    entries[0].Set(wxACCEL_CTRL, L'N', wxID_NEW);
    entries[1].Set(wxACCEL_CTRL, L'X', wxID_EXIT);
    entries[2].Set(wxACCEL_CTRL, L'A', wxID_ABOUT);
    entries[3].Set(wxACCEL_CTRL, L'S', wxID_SAVE);
    entries[4].Set(wxACCEL_CTRL, L'P', wxID_PRINT);
    entries[5].Set(wxACCEL_CTRL, L'C', wxID_COPY);
    wxAcceleratorTable accel(std::size(entries), entries);
    SetAcceleratorTable(accel);

    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_BOXPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_HISTOGRAM);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_GANTT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_CANDLESTICK_AXIS);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_LINEPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_LINEPLOT_CUSTOMIZED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_BARCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_BARCHART_STYLIZED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_BARCHART_IMAGE);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_CATEGORICAL_BARCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_PIECHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_PIECHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_DONUTCHART);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_DONUTCHART_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_WCURVE);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_LIKERT_3POINT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_LIKERT_7POINT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_HEATMAP);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_HEATMAP_GROUPED);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_MULTIPLOT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS);

    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnNewWindow, this, wxID_NEW);
    Bind(wxEVT_MENU, &MyFrame::OnSaveWindow, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MyFrame::OnPrintWindow, this, wxID_PRINT);
    Bind(wxEVT_MENU, &MyFrame::OnCopyWindow, this, wxID_COPY);
    Bind(wxEVT_MENU, &MyFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnCloseAll, this, wxID_CLOSE_ALL);
    Bind(wxEVT_MENU, &MyFrame::OnCloseAll, this, wxID_CLOSE_ALL);
    Bind(wxEVT_MENU, &MyFrame::OnClose, this, wxID_CLOSE);
    }

wxMenuBar* MyFrame::CreateMainMenubar()
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxSize iconSize{ wxSize(16, 16) };

    wxMenu* fileMenu = new wxMenu;

    fileMenu->Append(MyApp::ID_NEW_BARCHART, _(L"Bar Chart"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_BARCHART_STYLIZED, _(L"Bar Chart (Stylized)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart-stylized.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_BARCHART_IMAGE, _(L"Bar Chart (Commom Image)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart-image.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_CATEGORICAL_BARCHART, _(L"Bar Chart (Categorical Data)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED, _(L"Bar Chart (Categorical Data, Grouped)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_PIECHART, _(L"Pie Chart"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/piechart.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_PIECHART_GROUPED, _(L"Pie Chart (with Subgroup)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/piechart-subgrouped.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_DONUTCHART, _(L"Donut Chart"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/donut.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_DONUTCHART_GROUPED, _(L"Donut Chart (with Subgroup)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/donut-subgrouped.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_HISTOGRAM, _(L"Histogram"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/histogram.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES, _(L"Histogram (Discrete Category Counts)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/histogram.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_LINEPLOT, _(L"Line Plot"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/lineplot.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_LINEPLOT_CUSTOMIZED, _(L"Line Plot (Customized)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/lineplot-points.svg", iconSize));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_BOXPLOT, _(L"Box Plot"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/boxplot.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_HEATMAP, _(L"Heat Map"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/heatmap.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_HEATMAP_GROUPED, _(L"Heat Map (Grouped)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/heatmap-grouped.svg", iconSize));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_GANTT, _(L"Gantt Chart"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/gantt.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_CANDLESTICK_AXIS, _(L"Candlestick Plot"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/candlestick.svg", iconSize));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_LIKERT_3POINT, _(L"Likert Chart (3-Point Scale)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/likert3.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_LIKERT_7POINT, _(L"Likert Chart (7-Point Scale)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/likert7.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_WCURVE, _(L"W-Curve Plot"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/wcurve.svg", iconSize));
    fileMenu->AppendSeparator();

    fileMenu->Append(MyApp::ID_NEW_MULTIPLOT, _(L"Multiple Plots"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot.svg", iconSize));
    fileMenu->Append(MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS, _(L"Multiple Plots (Common Axis)"))->
        SetBitmap(wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot-common-axis.svg", iconSize));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_SAVE, _(L"&Save\tCtrl+S"), _(L"Save as Image"));
    fileMenu->Append(wxID_PRINT, _(L"&Print\tCtrl+P"), _(L"Print"));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_CLOSE, _(L"&Close child\tCtrl+F4"));
    fileMenu->AppendSeparator();

    fileMenu->Append(wxID_EXIT, _(L"&Exit\tAlt-X"), _(L"Quit the program"));

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT, _(L"&About\tF1"));

    wxMenuBar* mbar = new wxMenuBar;
    mbar->Append(fileMenu, _(L"&File"));
    mbar->Append(menuHelp, _(L"&Help"));

    return mbar;
    }

void MyFrame::OnQuit([[maybe_unused]] wxCommandEvent& event)
    {
    Close();
    }

void MyFrame::OnAbout([[maybe_unused]] wxCommandEvent& event)
    {
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetCopyright(L"Copyright (c) 2022");
    wxArrayString devs;
    devs.Add(L"Blake Madden");
    aboutInfo.SetDevelopers(devs);
    aboutInfo.SetName(_(L"Wistia Dataviz Library Demo"));
    aboutInfo.SetDescription(_(L"Demostration of Wisteria Dataviz, "
                                "a wxWidgets-based data visualization library."));

    wxAboutBox(aboutInfo, this);
    }

void MyFrame::OnNewWindow(wxCommandEvent& event)
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };

    // create and show another child frame
    MyChild* subframe = new MyChild(this);

    // Box Plot
    if (event.GetId() == MyApp::ID_NEW_BOXPLOT)
        {
        subframe->SetTitle(_(L"Box Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(L"datasets/mpg.csv",
                ImportInfo().ContinuousColumns({ L"hwy" }).
                CategoricalColumns({
                    { L"class", CategoricalImportMethod::ReadAsStrings },
                    { L"manufacturer", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto plot = std::make_shared<BoxPlot>(subframe->m_canvas);

        plot->SetData(mpgData,
                      L"hwy",
                      // leave this as std::nullopt to not create grouped boxes
                      L"class");

        // Show all points (not just outliers).
        // The points within the boxes and whiskers will be
        // bee-swarm jittering to visualize the distribution.
        plot->ShowAllPoints(true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Heatmap
    else if (event.GetId() == MyApp::ID_NEW_HEATMAP)
        {
        subframe->SetTitle(_(L"Heatmap"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto testScoresData = std::make_shared<Data::Dataset>();
        try
            {
            testScoresData->ImportCSV(L"datasets/Student Scores.csv",
                ImportInfo().
                ContinuousColumns({ L"test_score" }).
                CategoricalColumns({ { L"Week", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<HeatMap>(subframe->m_canvas);

        // add a title to the plot
        plot->GetTitle().GetGraphItemInfo().Text(_(L"Test Scores")).
                ChildAlignment(RelativeAlignment::FlushLeft).
                Pen(wxNullPen).Padding(4, 0, 0, 4).
                Font(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)).
                     MakeLarger());

        // use group and put all of the students' heatmaps into one column
        plot->SetData(testScoresData, L"TEST_SCORE", std::nullopt, L"Week");

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // customize the header of the legend and add it to the canvas
        auto legend{ plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true) };
        legend->SetLine(0, _(L"Range of Scores"));
        // after changing legend's text, recalculate how much of the
        // canvas it should consume
        legend->SetCanvasWidthProportion(subframe->m_canvas->CalcMinWidthProportion(legend));
        subframe->m_canvas->SetFixedObject(0, 1, legend);
        }
    // Heatmap (grouped)
    else if (event.GetId() == MyApp::ID_NEW_HEATMAP_GROUPED)
        {
        subframe->SetTitle(_(L"Heatmap (Grouped)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto testScoresData = std::make_shared<Data::Dataset>();
        try
            {
            testScoresData->ImportCSV(L"datasets/Student Scores.csv",
                ImportInfo().
                ContinuousColumns({ L"test_score" }).
                CategoricalColumns({
                    { L"Name", CategoricalImportMethod::ReadAsStrings },
                    { L"Week", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<HeatMap>(subframe->m_canvas);
        // add a title to the plot
        plot->GetTitle().GetGraphItemInfo().Text(_(L"Test Scores")).
            ChildAlignment(RelativeAlignment::FlushLeft).
            Pen(wxNullPen).Padding(4, 0, 0, 4).
            Font(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)).
                 MakeLarger());

        // use group and put all of the students' heatmaps into one column
        plot->SetData(testScoresData, L"TEST_SCORE", L"Name", L"Week", 1);
        // say "Students" at the top instead of "Groups"
        plot->SetGroupHeaderPrefix(_(L"Students"));

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // customize the header of the legend and add it to the canvas
        auto legend{ plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true) };
        subframe->m_canvas->SetFixedObject(0, 1, legend);
        }
    // Histogram
    else if (event.GetId() == MyApp::ID_NEW_HISTOGRAM)
        {
        subframe->SetTitle(_(L"Histogram"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto mtcarsData = std::make_shared<Data::Dataset>();
        try
            {
            mtcarsData->ImportCSV(L"datasets/mtcars.csv",
                ImportInfo().
                ContinuousColumns({ L"mpg" }).
                CategoricalColumns({ { L"Gear", CategoricalImportMethod::ReadAsIntegers } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<Histogram>(subframe->m_canvas,
            std::make_shared<Colors::Schemes::Decade1980s>());

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
            subframe->m_canvas->SetFixedObject(0, 1,
                plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true));
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
            mpgData->ImportCSV(L"datasets/mpg.csv",
                ImportInfo().
                ContinuousColumns({ L"cyl" }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<Histogram>(subframe->m_canvas,
            std::make_shared<Colors::Schemes::Decade1980s>());

        plot->SetData(mpgData, L"cyl",
                      std::nullopt,
                      // don't create range-based bins;
                      // instead, create one for each unique value.
                      Histogram::BinningMethod::BinUniqueValues,
                      // If the data is floating point, you can tell it to
                      // to be rounded here when categorizing it into discrete bins.
                      // In this case, the data is already discrete, so no rounding needed.
                      RoundingMethod::NoRounding,
                      // since we aren't using ranges, show labels under the middle of the bins.
                      Histogram::IntervalDisplay::Midpoints,
                      BinLabelDisplay::BinValue,
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
            linePlotData->ImportCSV(L"datasets/Spelling Grades.csv",
                ImportInfo().
                // first the Y column, then the X
                ContinuousColumns({ L"AVG_GRADE", L"WeeK"}).
                CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto linePlot = std::make_shared<LinePlot>(subframe->m_canvas,
            // use a different color scheme
            std::make_shared<Colors::Schemes::Decade1960s>(),
            // or create your own scheme
            // std::make_shared<Colors::Schemes::ColorScheme>
            //     (Colors::Schemes::ColorScheme{
            //         ColorBrewer::GetColor(Colors::Color::CadmiumRed),
            //         ColorBrewer::GetColor(Colors::Color::OctoberMist) }),

            // turn off markers by using a shape scheme filled with blank icons
            // (having just one icon in this scheme will get recycled for each line)
            std::make_shared<IconShapeScheme>(IconShapeScheme{IconShape::BlankIcon}));
        // add padding around the plot
        linePlot->SetCanvasMargins(5, 5, 5, 5);

        // set the data and use the grouping column from the dataset to create separate lines
        linePlot->SetData(linePlotData, L"AVG_GRADE", L"WeeK", L"Gender");

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
            linePlot->GetBottomXAxis().SetCustomLabel(i,
                Label(wxString::Format(_(L"Week %i"), i)));
            }

        // add the line plot and its legend to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);
        subframe->m_canvas->SetFixedObject(0, 1,
            linePlot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true));
        }
    // Line Plot (customized)
    else if (event.GetId() == MyApp::ID_NEW_LINEPLOT_CUSTOMIZED)
        {
        subframe->SetTitle(_(L"Line Plot (Customized)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto linePlotData = std::make_shared<Data::Dataset>();
        try
            {
            linePlotData->ImportCSV(L"datasets/Spelling Grades.csv",
                ImportInfo().
                // first the Y column, then the X
                ContinuousColumns({ L"AVG_GRADE", L"WeeK"}).
                CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto linePlot = std::make_shared<LinePlot>(subframe->m_canvas,
            // create your own color scheme
            std::make_shared<Colors::Schemes::ColorScheme>
                 (Colors::Schemes::ColorScheme{
                     ColorBrewer::GetColor(Colors::Color::CadmiumRed),
                     ColorBrewer::GetColor(Colors::Color::GrannySmithApple) }),
            // use custom markers
            std::make_shared<IconShapeScheme>(IconShapeScheme
                {IconShape::DiamondIcon, IconShape::HexagonIcon }));
        // add padding around the plot
        linePlot->SetCanvasMargins(5, 5, 5, 5);

        // set the data and use the grouping column from the dataset to create separate lines
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
        linePlot->SetPointColorCriteria(
                                        [](const double x, const double y)
                                          {
                                          return (y < 60.0) ?
                                              wxColour(255, 0, 0) :
                                              wxColour();
                                          });

        // add a note
        auto note = std::make_shared<Label>(
            GraphItemInfo(_(L"What happened this week?\nAre we sure this is correct???")).
            Pen(*wxLIGHT_GREY).
            FontBackgroundColor(ColorBrewer::GetColor(Color::AntiqueWhite)).
            Anchoring(Anchoring::TopRightCorner).Padding(4, 4, 4, 4));
        linePlot->AddEmbeddedObject(note,
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
            linePlot->GetBottomXAxis().SetCustomLabel(i,
                Label(wxString::Format(_(L"Week %i"), i)));
            }

        // add a red background for failing grades
        // (note that this will appear on the legend and the plot)
        linePlot->AddReferenceArea(ReferenceArea(AxisType::LeftYAxis, 0, 59,
                                                 _(L"Failing"), *wxRED));

        // add the line plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);

        // add a legend to the side and center it vertically
        auto legend = linePlot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, false);
        legend->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        subframe->m_canvas->SetFixedObject(0, 1, legend);

        // to add another right-aligned legend under the graph, uncomment the following:

        // subframe->m_canvas->SetFixedObjectsGridSize(2, 2);
        // legend = linePlot->CreateLegend(LegendCanvasPlacementHint::AboveOrBeneathGraph);
        // legend->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
        // subframe->m_canvas->SetRowProportion(0, 1-subframe->m_canvas->CalcMinHeightProportion(legend));
        // subframe->m_canvas->SetRowProportion(1, subframe->m_canvas->CalcMinHeightProportion(legend));
        // subframe->m_canvas->SetFixedObject(1, 0, legend);

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
            companyAcquisitionData->ImportCSV(L"datasets/Company Acquisition.csv",
                ImportInfo().
                ContinuousColumns({ L"Completion" }).
                DateColumns({ { L"Start" }, { L"End" } }).
                CategoricalColumns({
                    { L"Task" },
                    { L"Description" },
                    { L"Resource" }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto ganttChart = std::make_shared<GanttChart>(subframe->m_canvas,
            // use a different color scheme where the colors
            // stand out more from each other
            std::make_shared<Colors::Schemes::Decade1920s>());
        ganttChart->SetData(companyAcquisitionData,
            DateInterval::FiscalQuarterly, FiscalYear::USBusiness,
            L"Task", L"Start", "End",
            // these columns are optional
            L"Resource", L"Description", L"Completion", L"Resource");

        // add deadlines
        auto releaseDate = ganttChart->GetScalingAxis().GetPointFromDate(
            wxDateTime(25, wxDateTime::Dec, 2022));
        if (releaseDate)
            {
            ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
                releaseDate.value(), _(L"Release"),
                ColorBrewer::GetColor(Colors::Color::TractorRed)) );
            }

        auto updateReleaseDate = ganttChart->GetScalingAxis().GetPointFromDate(
            wxDateTime(15, wxDateTime::Mar, 2023));
        if (updateReleaseDate)
            {
            ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
                updateReleaseDate.value(),
                _(L"Hotfix Release"),
                ColorBrewer::GetColor(Colors::Color::TractorRed,
                                      Wisteria::Settings::GetTranslucencyValue())));
            }

        ganttChart->SetCanvasMargins(5, 5, 5, 5);
        subframe->m_canvas->SetFixedObject(0, 0, ganttChart);
        // add a legend, showing whom is assigned to which tasks
        subframe->m_canvas->SetFixedObject(0, 1,
            ganttChart->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, false));
        }
    else if (event.GetId() == MyApp::ID_NEW_CANDLESTICK_AXIS)
        {
        subframe->SetTitle(_(L"Candlestick Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);

        auto silverFuturesData = std::make_shared<Data::Dataset>();
        try
            {
            silverFuturesData->ImportCSV(L"datasets/Silver Futures.csv",
                ImportInfo().
                ContinuousColumns({ L"Open", L"High", L"Low", L"Close/Last" }).
                DateColumns({ { L"Date" } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto candlestickChart = std::make_shared<CandlestickPlot>(subframe->m_canvas);
        // Chart's left axis will start at zero by default so that the scale
        // isn't misleading; you can, however, turn that off like this
        // to better see the daily activity.
        // This should be done before calling SetData() so that it bases
        // axis range on the data.
        candlestickChart->GetLeftYAxis().StartAtZero(false);

        // Uncomment this to fit the entire year onto the canvas
        // so that there isn't a scrollbar.
        // candlestickChart->SetPointsPerDefaultCanvasSize(365);

        candlestickChart->SetData(silverFuturesData,
            L"Date", L"Open", L"High", L"Low", L"Close/Last");

        candlestickChart->GetTitle().SetText(_(L"Silver COMEX 2021 Trend"));

        candlestickChart->SetCanvasMargins(5, 5, 5, 5);
        subframe->m_canvas->SetFixedObject(0, 0, candlestickChart);
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

        plot->AddBar(BarChart::Bar(1,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(92).Brush(barColor))
            },
            _(L""), Label(_(L"Bugs")), BoxEffect::Solid) );

        plot->AddBar(BarChart::Bar(2,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor))
            },
            _(L""), Label(_(L"Pending feature requests")), BoxEffect::Solid));

        plot->AddBar(BarChart::Bar(3,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor))
            },
            _(L""), Label(_(L"Unfinished help topics")), BoxEffect::Solid));

        plot->AddBar(BarChart::Bar(4,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor))
            },
            _(L""), Label(_(L"Missing unit tests")), BoxEffect::Solid));

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

        plot->AddBar(BarChart::Bar(1,
            {
            // this bar will have two sections to it, where a red section
            // refers to the more critical bugs
            BarChart::BarBlock(BarChart::BarBlockInfo(22).Brush(*wxRED)),
            BarChart::BarBlock(BarChart::BarBlockInfo(72).Brush(barColor))
            },
            _(L""), Label(_(L"Bugs")), BoxEffect::Glassy,
            // we will make the width of the bar twice as wide as the others
            // to show how important it is
            wxALPHA_OPAQUE, 2));

        // Note that because the first bar has an unusual width, this will offset
        // the positions of the following bars. Therefore, we need to place them
        // at positions like 2.5, 3.5, etc. Normally, they would just go on points like 2 or 3.
        plot->AddBar(BarChart::Bar(2.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor))
            },
            _(L""), Label(_(L"Pending feature requests")), BoxEffect::Glassy,
            // this bar will be translucent
            75, 1));

        plot->AddBar(BarChart::Bar(3.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor))
            },
            _(L""), Label(_(L"Unfinished help topics")), BoxEffect::Glassy,
            wxALPHA_OPAQUE, 1));

        plot->AddBar(BarChart::Bar(4.5,
            {
             BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor))
            },
            _(L""), Label(_(L"Missing unit tests")), BoxEffect::Glassy,
            wxALPHA_OPAQUE, 1));

        // only show the labels on the axis
        plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        // force the custom labels set at points like 2.5 to be shown
        const auto [rangeStart, rangeEnd] = plot->GetBarAxis().GetRange();
        plot->GetBarAxis().SetRange(rangeStart, rangeEnd, 1, 0.5, 1);

        plot->GetBarAxis().GetTitle().GetGraphItemInfo().
            Text(L"ISSUES").Orient(Orientation::Horizontal).Padding(5, 10, 0, 0).
            LabelAlignment(TextAlignment::Centered);
        plot->GetBarAxis().GetTitle().SplitTextByCharacter();

        // align the axis labels over to the left
        plot->GetBarAxis().SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::AlignWithBoundary);

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
        plot->SetBarOrientation(Orientation::Horizontal);

        plot->SetCommonBarImage(std::make_shared<wxImage>(
            // Photo by ThisisEngineering RAEng on Unsplash
            GraphItems::Image::LoadImageWithCorrection(L"res/thisisengineering-raeng-64YrPKiguAE-unsplash.jpg")),
            *wxWHITE);

        auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

        plot->AddBar(BarChart::Bar(1,
            {
            // this bar will have two sections to it, where a red section
            // refers to the more critical bugs
            BarChart::BarBlock(BarChart::BarBlockInfo(22).Brush(*wxRED)),
            BarChart::BarBlock(BarChart::BarBlockInfo(72).Brush(barColor))
            },
            _(L""), Label(_(L"Bugs")), BoxEffect::CommonImage,
            // we will make the width of the bar twice as wide as the others
            // to show how important it is
            wxALPHA_OPAQUE, 2));

        // Note that because the first bar has an unusual width, this will offset
        // the positions of the following bars. Therefore, we need to place them
        // at positions like 2.5, 3.5, etc. Normally, they would just go on points like 2 or 3.
        plot->AddBar(BarChart::Bar(2.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor))
            },
            _(L""), Label(_(L"Pending feature requests")), BoxEffect::CommonImage,
            wxALPHA_OPAQUE, 1));

        plot->AddBar(BarChart::Bar(3.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor))
            },
            _(L""), Label(_(L"Unfinished help topics")), BoxEffect::CommonImage,
            wxALPHA_OPAQUE, 1));

        plot->AddBar(BarChart::Bar(4.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor))
            },
            _(L""), Label(_(L"Missing unit tests")), BoxEffect::CommonImage,
            wxALPHA_OPAQUE, 1));

        // only show the labels on the axis
        plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        // force the custom labels set at points like 2.5 to be shown
        const auto [rangeStart, rangeEnd] = plot->GetBarAxis().GetRange();
        plot->GetBarAxis().SetRange(rangeStart, rangeEnd, 1, 0.5, 1);

        plot->GetBarAxis().GetTitle().GetGraphItemInfo().
            Text(L"ISSUES").Padding(5, 10, 0, 0);

        // align the axis labels over to the left
        plot->GetBarAxis().SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::AlignWithBoundary);

        plot->SetCanvasMargins(5, 5, 5, 5);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart, using the a dataset interface to aggregate labels
    else if (event.GetId() == MyApp::ID_NEW_CATEGORICAL_BARCHART)
        {
        subframe->SetTitle(_(L"Bar Chart (Categorical Data)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(L"datasets/mpg.csv",
                ImportInfo().
                CategoricalColumns({
                    { L"manufacturer", CategoricalImportMethod::ReadAsStrings },
                    { L"model", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<CategoricalBarChart>(subframe->m_canvas,
            std::make_shared<Colors::Schemes::Decade1980s>());

        plot->SetData(mpgData, L"manufacturer");

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        }
    // Bar Chart, using the a dataset interface to aggregate labels,
    // along with a grouping column
    else if (event.GetId() == MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED)
        {
        subframe->SetTitle(_(L"Bar Chart (Categorical Data, Grouped)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto mpgData = std::make_shared<Data::Dataset>();
        try
            {
            mpgData->ImportCSV(L"datasets/mpg.csv",
                ImportInfo().
                CategoricalColumns({
                    { L"manufacturer", CategoricalImportMethod::ReadAsStrings },
                    { L"class", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

        auto plot = std::make_shared<CategoricalBarChart>(subframe->m_canvas,
            std::make_shared<Colors::Schemes::Decade1980s>());

        plot->SetData(mpgData, L"manufacturer", std::nullopt, L"class");
        plot->SetBarOpacity(220);
        plot->SetBarEffect(BoxEffect::Glassy);

        subframe->m_canvas->SetFixedObject(0, 0, plot);

        subframe->m_canvas->SetFixedObject(0, 1,
            plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true));
        }
    // Pie Chart
    else if (event.GetId() == MyApp::ID_NEW_PIECHART)
        {
        subframe->SetTitle(_(L"Pie Chart"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 1);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"Course", CategoricalImportMethod::ReadAsStrings },
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(), PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            { foundSlice->SetDescription(_(L"Includes both literary and composition courses")); }

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
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"Course", CategoricalImportMethod::ReadAsStrings },
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(), PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            { foundSlice->SetDescription(_(L"Includes both literary and composition courses")); }

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);
        // add a donut hole
        plot->IncludeDonutHole(true);
        plot->GetDonutHoleLabel().SetText(L"Enrollment\nFall 2023");
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
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"Course", CategoricalImportMethod::ReadAsStrings },
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                    { L"Course", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        // find a group from the outer ring and add a description to it
        auto foundSlice = std::find(plot->GetOuterPie().begin(), plot->GetOuterPie().end(), PieChart::SliceInfo{ L"English" });
        if (foundSlice != plot->GetOuterPie().end())
            { foundSlice->SetDescription(_(L"Includes both literary and composition courses")); }
        // turn off all but one of the outer labels for the inner ring
        // to draw attention to it
        std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
            [](auto& slice) noexcept
                {
                if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") != 0)
                    { slice.ShowGroupLabel(false); }
                }
            );

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // add a legend for the inner ring (i.e., the subgroup column,
        // which will also show headers for their parent groups)
        subframe->m_canvas->SetFixedObject(0, 1,
            plot->CreateInnerPieLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        }
    // Donut Chart (with Subgroup)
    else if (event.GetId() == MyApp::ID_NEW_DONUTCHART_GROUPED)
        {
        subframe->SetTitle(_(L"Donut Chart (with Subgroup)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                    { L"Course", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto plot = std::make_shared<PieChart>(subframe->m_canvas);
        plot->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        // hide all outer labels for the main (i.e., outer) ring
        std::for_each(plot->GetOuterPie().begin(), plot->GetOuterPie().end(),
            [](auto& slice) noexcept
                { slice.ShowGroupLabel(false); }
            );
        // turn off all but one of the outer labels for the inner ring,
        // and also add a custom description to one of the inner slices
        std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
            [](auto& slice) noexcept
                {
                if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") != 0)
                    { slice.ShowGroupLabel(false); }
                else
                    { slice.SetDescription(_(L"Drop this from the catalog?")); }
                }
            );

        // apply the slice's colors to its respective outside label
        plot->UseColorLabels(true);
        // add a donut hole
        plot->IncludeDonutHole(true);
        plot->GetDonutHoleLabel().SetText(L"Enrollment\nFall 2023");
        plot->SetDonutHoleProportion(.8);

        subframe->m_canvas->SetFixedObject(0, 0, plot);
        // add a legend for the inner ring (i.e., the subgroup column,
        // which will also show headers for their parent groups)
        subframe->m_canvas->SetFixedObject(0, 1,
            plot->CreateInnerPieLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        }
    // W-Curve plot
    else if (event.GetId() == MyApp::ID_NEW_WCURVE)
        {
        subframe->SetTitle(_(L"W-Curve Plot"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 2);
        auto wcurveData = std::make_shared<Data::Dataset>();
        try
            {
            wcurveData->ImportCSV(L"datasets/Sense of Belonging.csv",
                ImportInfo().
                ContinuousColumns({ L"Year", L"Belong" }).
                CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
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

        /* auto storyNote = std::make_shared<Label>(
            GraphItemInfo(_(L"Frank reported that he experienced a"
                " \u201Cdownward spiral\u201D during his first year on campus.")).
            Anchoring(Anchoring::BottomLeftCorner).
            FontBackgroundColor(ColorBrewer::GetColor(Color::Canary)).
            LabelAlignment(TextAlignment::RaggedRight).
            LabelStyling(LabelStyle::DottedLinedPaper).Padding(4, 4, 4, 4));
        storyNote->GetFont().MakeSmaller();
        storyNote->SplitTextToFitLength(25);

        WCurve->AddEmbeddedObject(storyNote,
            wxPoint(1, WCurve->GetLeftYAxis().GetRange().first));*/

        // add the line plot and its legend to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, WCurve);
        subframe->m_canvas->SetFixedObject(0, 1,
            WCurve->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, false));
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
            surveyData->ImportCSV(L"datasets/Graph Library Survey.csv",
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
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        // Original data has a scale going from 1-7, but we want to simplify
        // it to 1-3. To do this, we will collapse all the positive levels
        // into one, and all negative levels into another level.
        auto categoricalNames{ surveyData->GetCategoricalColumnNames() };
        Dataset::RemoveColumnNamesFromList(categoricalNames, { L"Gender" });
        const auto responsesScale = LikertChart::Simplify(surveyData,
                                                 categoricalNames,
                                                 LikertChart::LikertSurveyQuestionFormat::SevenPoint);

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

        auto likertChart = std::make_shared<LikertChart>(subframe->m_canvas,
            // Simplify() will return LikertChart::LikertSurveyQuestionFormat::ThreePoint
            responsesScale);
        likertChart->SetData(surveyData,
            categoricalNames,
            // passing in a grouping column will change it from ThreePoint -> ThreePointCategorized
            L"Gender");

        // groups with lower responses will have narrower bars
        likertChart->SetBarSizesToRespondentSize(true);

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
            surveyData->ImportCSV(L"datasets/Graph Library Survey.csv",
                Data::ImportInfo().
                CategoricalColumns(
                    {
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
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

        // Because the responses in the dataset were coded 1-7, we will need to
        // add meaningful labels to the dataset. The following will add stock
        // labels to represent the responses.
        LikertChart::SetLabels(surveyData,
            surveyData->GetCategoricalColumnNames(),
            LikertChart::CreateLabels(LikertChart::LikertSurveyQuestionFormat::SevenPoint));

        auto likertChart = std::make_shared<LikertChart>(subframe->m_canvas,
            LikertChart::LikertSurveyQuestionFormat::SevenPoint);
        likertChart->SetData(surveyData, surveyData->GetCategoricalColumnNames());

        subframe->m_canvas->SetFixedObject(0, 0, likertChart);
        subframe->m_canvas->SetFixedObject(0, 1,
            likertChart->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        }
    // Multiple plots
    else if (event.GetId() == MyApp::ID_NEW_MULTIPLOT)
        {
        subframe->SetTitle(_(L"Multiple Plots"));
        subframe->m_canvas->SetFixedObjectsGridSize(2, 2);
        auto pieData = std::make_shared<Data::Dataset>();
        try
            {
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                    { L"Course", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        auto donutChart = std::make_shared<PieChart>(subframe->m_canvas);
        donutChart->SetData(pieData, L"Enrollment", L"COLLEGE");

        // apply the slice's colors to its respective outside label
        donutChart->UseColorLabels(true);
        // add a donut hole
        donutChart->IncludeDonutHole(true);
        donutChart->GetDonutHoleLabel().SetText(L"Enrollment\nFall 2023");
        donutChart->SetDonutHoleProportion(.5);

        subframe->m_canvas->SetFixedObject(0, 0, donutChart);

        // add a pie chart on the side, that will fill up the whole right side
        auto groupedPieChart = std::make_shared<PieChart>(subframe->m_canvas);
        groupedPieChart->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

        // turn off all outer ring labels
        std::for_each(groupedPieChart->GetOuterPie().begin(), groupedPieChart->GetOuterPie().end(),
            [](auto& slice) noexcept
                { slice.ShowGroupLabel(false); }
            );
        // turn off all but one of the outer labels for the inner ring
        // to draw attention to it
        std::for_each(groupedPieChart->GetInnerPie().begin(), groupedPieChart->GetInnerPie().end(),
            [](auto& slice) noexcept
                {
                if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") != 0)
                    { slice.ShowGroupLabel(false); }
                }
            );

        // apply the slice's colors to its respective outside label
        groupedPieChart->UseColorLabels(true);

        groupedPieChart->GetGraphItemInfo().CanvasHeightProportion(1);

        subframe->m_canvas->SetFixedObject(0, 1, groupedPieChart);

        // add a large note to the canvas
        // (into the second row, beneath the donut chart)
        auto note = std::make_shared<Label>(GraphItemInfo(
            _(L"NOTE\n"
               "Should we consider dropping VB.NET from the catalog?\n"
               "Enrollment has been really low the last few years.")).
            Padding(4, 4, 4, 4).
            Scaling(2).
            DPIScaling(subframe->m_canvas->GetDPIScaleFactor()).
            Pen(wxNullPen));
        note->GetHeaderInfo().Enable(true).FontColor(*wxBLUE).GetFont().MakeBold();
        subframe->m_canvas->SetFixedObject(1, 0, note);

        // in the first column (the donut chart and the not beneath it),
        // set the proportions of the rows based on how tall the note is
        subframe->m_canvas->SetRowProportion(0,
            1 - subframe->m_canvas->CalcMinHeightProportion(note));
        subframe->m_canvas->SetRowProportion(1,
            subframe->m_canvas->CalcMinHeightProportion(note));
        }
    // Multiple plots with a common axis
    else if (event.GetId() == MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS)
        {
        subframe->SetTitle(_(L"Multiple Plots (Common Axis)"));
        subframe->m_canvas->SetFixedObjectsGridSize(1, 3);
        auto spellingData = std::make_shared<Data::Dataset>();
        try
            {
            spellingData->ImportCSV(L"datasets/Spelling Grades.csv",
                ImportInfo().
                // different order, first the X column, then the Y
                ContinuousColumns({ L"Week", L"AVG_GRADE" }).
                CategoricalColumns({ { L"Gender", CategoricalImportMethod::ReadAsStrings } }));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
        // create your own color scheme
        const auto colors = std::make_shared<Colors::Schemes::ColorScheme>
            (Colors::Schemes::ColorScheme{
                ColorBrewer::GetColor(Colors::Color::GrannySmithApple),
                ColorBrewer::GetColor(Colors::Color::CadmiumRed) });

        auto linePlot = std::make_shared<LinePlot>(subframe->m_canvas, colors,
            // use custom markers
            std::make_shared<IconShapeScheme>(IconShapeScheme
                { IconShape::DiamondIcon, IconShape::HexagonIcon }));

        // set the data and use the grouping column from the dataset to create separate lines
        linePlot->SetData(spellingData, L"AVG_GRADE", L"WeeK", L"Gender");

        // customize the X axis labels
        for (int i = 1; i < 6; ++i)
            {
            linePlot->GetBottomXAxis().SetCustomLabel(i,
                Label(wxString::Format(_(L"Week %i"), i)));
            }

        // instead of adding the legend to the canvas, place it on top
        // of the line plot
        auto lineLegend = linePlot->CreateLegend(LegendCanvasPlacementHint::EmbeddedOnGraph, false);
        lineLegend->SetAnchoring(Anchoring::BottomRightCorner);
        linePlot->AddEmbeddedObject(lineLegend,
            wxPoint(linePlot->GetBottomXAxis().GetRange().second,
                    linePlot->GetLeftYAxis().GetRange().first));

        // add the line plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 0, linePlot);

        // create a box plot with the same data
        auto boxPlot = std::make_shared<BoxPlot>(subframe->m_canvas, colors);

        boxPlot->SetData(spellingData, L"AVG_GRADE", std::nullopt);

        // customize the box appearance
        boxPlot->SetBoxCorners(BoxCorners::Rounded);

        // copy the left axis range from the line plot to the box plot,
        // then turn off the labels and pen
        boxPlot->GetLeftYAxis().CopySettings(linePlot->GetLeftYAxis());
        boxPlot->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        boxPlot->GetLeftYAxis().GetAxisLinePen() = wxNullPen;

        // add the box plot to the canvas
        subframe->m_canvas->SetFixedObject(0, 1, boxPlot);

        // create a common axis, also copied from the line plot's
        // left axis
        auto commonAxis = std::make_shared<Axis>(AxisType::RightYAxis);
        commonAxis->SetDPIScaleFactor(subframe->m_canvas->GetDPIScaleFactor());
        commonAxis->CopySettings(linePlot->GetLeftYAxis());
        // Get the canvas size of the axis and add it to the canvas.
        // Note that we need to multiple the calculated size by two because
        // axes are centered when drawn
        commonAxis->SetCanvasWidthProportion(subframe->m_canvas->CalcMinWidthProportion(commonAxis)*2);
        subframe->m_canvas->SetFixedObject(0, 2, commonAxis);

        // now that we are done copying the left axis from the line plot,
        // hide the line plot's axis labels
        linePlot->GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        // tell the canvas to align the plots and stand-alone axes across
        // each row
        subframe->m_canvas->AlignRowContent(true);

        // add a centered title and subtitle on the canvas
        // (above the plots)
        subframe->m_canvas->GetTopTitles().emplace_back(_(L"Average Grades"));
        subframe->m_canvas->GetTopTitles().emplace_back(
            Label(GraphItemInfo(_(L"Average grades taken from last 5 weeks' spelling tests.")).
                FontColor(ColorBrewer::GetColor(Color::DarkGray)).Pen(wxNullPen).
                Font(subframe->m_canvas->GetTopTitles().back().GetFont().MakeSmaller())));
        }

    subframe->Maximize(true);
    subframe->Show(true);
    }

void MyFrame::OnSaveWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        { return; }

    pChild->m_canvas->OnSave(event);
    }

void MyFrame::OnPrintWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        { return; }

    pChild->m_canvas->OnPrint(event);
    }

void MyFrame::OnCopyWindow(wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        { return; }

    pChild->m_canvas->OnCopy(event);
    }

void MyFrame::OnCloseAll([[maybe_unused]] wxCommandEvent& event)
    {
    for (auto child : GetChildren())
        {
        if (child->IsKindOf(wxCLASSINFO(wxMDIChildFrame)) )
            { child->Close(); }
        }
    }

void MyFrame::OnClose([[maybe_unused]] wxCommandEvent& event)
    {
    MyChild* pChild = dynamic_cast<MyChild*>(GetActiveChild());
    if (pChild == nullptr)
        { return; }

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
    toolBar->AddTool(MyApp::ID_NEW_CATEGORICAL_BARCHART_GROUPED, _(L"Bar Chart (Categorical Data, Grouped)"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/barchart.svg", iconSize),
        _(L"Bar Chart (Categorical Data, Grouped)"));

    toolBar->AddTool(MyApp::ID_NEW_PIECHART, _(L"Pie Chart"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/piechart.svg", iconSize),
        _(L"Pie Chart"));
    toolBar->AddTool(MyApp::ID_NEW_PIECHART_GROUPED, _(L"Pie Chart (with Subgroup)"),
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

    toolBar->AddTool(MyApp::ID_NEW_HISTOGRAM_UNIQUE_VALUES, _(L"Histogram (Discrete Category Counts)"),
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
    toolBar->AddSeparator();

    toolBar->AddTool(MyApp::ID_NEW_MULTIPLOT, _(L"Multiple Plots"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot.svg", iconSize),
        _(L"Multiple Plots"));
    toolBar->AddTool(MyApp::ID_NEW_MULTIPLOT_COMMON_AXIS, _(L"Multiple Plots (Common Axis)"),
        wxBitmapBundle::FromSVGFile(appDir + L"/res/multiplot-common-axis.svg", iconSize),
        _(L"Multiple Plots (Common Axis)"));

    toolBar->Realize();
    }

// ---------------------------------------------------------------------------
// MyChild
// ---------------------------------------------------------------------------

MyChild::MyChild(wxMDIParentFrame *parent)
       : wxMDIChildFrame(parent, wxID_ANY, L"")
    {
    m_canvas = new Wisteria::Canvas{ this };

    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    const wxSize iconSize = Image::GetSVGSize(appDir + L"/res/wisteria.svg");

    SetIcon(wxBitmapBundle::FromSVGFile(appDir + L"/res/wisteria.svg", iconSize).GetIcon(iconSize));

    // create our menu bar and associate it with the frame
    SetMenuBar(MyFrame::CreateMainMenubar());

    // this should work for MDI frames as well as for normal ones, provided
    // they can be resized at all
    if (!IsAlwaysMaximized())
        { SetSizeHints(200, 200); }
    }
