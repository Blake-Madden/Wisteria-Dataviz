/////////////////////////////////////////////////////////////////////////////
// Name:        demo.h
// Purpose:     Wisteria Library Demo
// Author:      Blake Madden
// Created:     01/23/2022
// Copyright:   (c) Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/mdi.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/sysopt.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/aboutdlg.h>
#include <wx/uilocale.h>
#include "../src/base/canvas.h"
#include "../src/base/commonaxisbuilder.h"
#include "../src/base/image.h"
#include "../src/base/label.h"
#include "../src/base/reportbuilder.h"
#include "../src/base/reportprintout.h"
#include "../src/data/subset.h"
#include "../src/graphs/barchart.h"
#include "../src/graphs/boxplot.h"
#include "../src/graphs/candlestickplot.h"
#include "../src/graphs/categoricalbarchart.h"
#include "../src/graphs/ganttchart.h"
#include "../src/graphs/heatmap.h"
#include "../src/graphs/histogram.h"
#include "../src/graphs/likertchart.h"
#include "../src/graphs/lineplot.h"
#include "../src/graphs/lrroadmap.h"
#include "../src/graphs/piechart.h"
#include "../src/graphs/proconroadmap.h"
#include "../src/graphs/table.h"
#include "../src/graphs/wcurveplot.h"
#include "../src/import/text_matrix.h"
#include "../src/util/logfile.h"

// Define a new application
class MyApp final : public wxApp
    {
public:
    virtual bool OnInit() final;
    static constexpr int ID_NEW_BOXPLOT{ wxID_HIGHEST+1 };
    static constexpr int ID_NEW_HISTOGRAM{ wxID_HIGHEST+2 };
    static constexpr int ID_NEW_GANTT{ wxID_HIGHEST+3 };
    static constexpr int ID_NEW_LINEPLOT{ wxID_HIGHEST+4 };
    static constexpr int ID_NEW_BARCHART{ wxID_HIGHEST+5 };
    static constexpr int ID_NEW_WCURVE{ wxID_HIGHEST+6 };
    static constexpr int ID_NEW_LIKERT_3POINT{ wxID_HIGHEST+7 };
    static constexpr int ID_NEW_HEATMAP{ wxID_HIGHEST+8 };
    static constexpr int ID_NEW_HEATMAP_GROUPED{ wxID_HIGHEST+9 };
    static constexpr int ID_NEW_PIECHART{ wxID_HIGHEST+10 };
    static constexpr int ID_NEW_PIECHART_GROUPED{ wxID_HIGHEST+11 };
    static constexpr int ID_NEW_DONUTCHART{ wxID_HIGHEST+12 };
    static constexpr int ID_NEW_DONUTCHART_GROUPED{ wxID_HIGHEST+13 };
    static constexpr int ID_NEW_LINEPLOT_CUSTOMIZED{ wxID_HIGHEST+14 };
    static constexpr int ID_NEW_BARCHART_STYLIZED{ wxID_HIGHEST+15 };
    static constexpr int ID_NEW_LIKERT_7POINT{ wxID_HIGHEST+16 };
    static constexpr int ID_NEW_MULTIPLOT{ wxID_HIGHEST+17 };
    static constexpr int ID_NEW_MULTIPLOT_COMMON_AXIS{ wxID_HIGHEST+18 };
    static constexpr int ID_NEW_CANDLESTICK_AXIS{ wxID_HIGHEST+19 };
    static constexpr int ID_NEW_BARCHART_IMAGE{ wxID_HIGHEST+20 };
    static constexpr int ID_NEW_HISTOGRAM_UNIQUE_VALUES{ wxID_HIGHEST+21 };
    static constexpr int ID_NEW_CATEGORICAL_BARCHART{ wxID_HIGHEST+22 };
    static constexpr int ID_NEW_CATEGORICAL_BARCHART_GROUPED{ wxID_HIGHEST+23 };
    static constexpr int ID_NEW_CATEGORICAL_BARCHART_STIPPLED{ wxID_HIGHEST+24 };
    static constexpr int ID_NEW_LR_ROADMAP_GRAPH{ wxID_HIGHEST+25 };
    static constexpr int ID_NEW_PROCON_ROADMAP_GRAPH{ wxID_HIGHEST+26 };
    static constexpr int ID_NEW_TABLE{ wxID_HIGHEST+27 };
    static constexpr int ID_PRINT_ALL{ wxID_HIGHEST+28 };
    };

// Define a new frame
class MyFrame final : public wxMDIParentFrame
    {
public:
    MyFrame();
    static wxMenuBar* CreateMainMenubar();

private:
    void InitToolBar(wxToolBar* toolBar);

    void OnAbout([[maybe_unused]]wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnQuit([[maybe_unused]]wxCommandEvent& event);
    void OnCloseAll([[maybe_unused]]wxCommandEvent& event);
    void OnClose([[maybe_unused]]wxCommandEvent& event);
    void OnSaveWindow(wxCommandEvent& event);
    void OnPrintWindow(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnCopyWindow(wxCommandEvent& event);
    };

class MyChild final : public wxMDIChildFrame
    {
    friend MyFrame;
public:
    MyChild(wxMDIParentFrame* parent);
private:
    Wisteria::Canvas* m_canvas{ nullptr };
    };
