/////////////////////////////////////////////////////////////////////////////
// Name:        demo.h
// Purpose:     Wisteria Library Demo
// Author:      Blake Madden
// Created:     01/23/2022
// Copyright:   (c) Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
/////////////////////////////////////////////////////////////////////////////

#include "../src/base/canvas.h"
#include "../src/base/commonaxisbuilder.h"
#include "../src/base/image.h"
#include "../src/base/label.h"
#include "../src/base/reportbuilder.h"
#include "../src/base/reportprintout.h"
#include "../src/data/pivot.h"
#include "../src/data/subset.h"
#include "../src/data/textclassifier.h"
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
#include "../src/graphs/sankeydiagram.h"
#include "../src/graphs/scalechart.h"
#include "../src/graphs/table.h"
#include "../src/graphs/wcurveplot.h"
#include "../src/graphs/wordcloud.h"
#include "../src/import/text_matrix.h"
#include "../src/ui/dialogs/variableselectdlg.h"
#include "../src/util/logfile.h"
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/choicdlg.h>
#include <wx/filename.h>
#include <wx/mdi.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/toolbar.h>
#include <wx/uilocale.h>
#include <wx/wx.h>

#ifndef __WISTERIA_DEMO_H__
#define __WISTERIA_DEMO_H__

// Define a new application
class MyApp final : public wxApp
    {
  public:
    virtual bool OnInit() final;

    enum ControlIDs
        {
        ID_NEW_BOXPLOT = wxID_HIGHEST,
        ID_NEW_HISTOGRAM,
        ID_NEW_GANTT,
        ID_NEW_LINEPLOT,
        ID_NEW_BARCHART,
        ID_NEW_WCURVE,
        ID_NEW_LIKERT_3POINT,
        ID_NEW_HEATMAP,
        ID_NEW_HEATMAP_GROUPED,
        ID_NEW_PIECHART,
        ID_NEW_PIECHART_GROUPED,
        ID_NEW_DONUTCHART,
        ID_NEW_DONUTCHART_GROUPED,
        ID_NEW_LINEPLOT_CUSTOMIZED,
        ID_NEW_BARCHART_STYLIZED,
        ID_NEW_LIKERT_7POINT,
        ID_NEW_MULTIPLOT,
        ID_NEW_MULTIPLOT_COMMON_AXIS,
        ID_NEW_CANDLESTICK_AXIS,
        ID_NEW_BARCHART_IMAGE,
        ID_NEW_HISTOGRAM_UNIQUE_VALUES,
        ID_NEW_CATEGORICAL_BARCHART,
        ID_NEW_CATEGORICAL_BARCHART_GROUPED,
        ID_NEW_CATEGORICAL_BARCHART_STIPPLED,
        ID_NEW_LR_ROADMAP_GRAPH,
        ID_NEW_PROCON_ROADMAP_GRAPH,
        ID_NEW_SANKEY_DIAGRAM,
        ID_NEW_GROUPED_SANKEY_DIAGRAM,
        ID_NEW_WORD_CLOUD,
        ID_NEW_TABLE,
        ID_PRINT_ALL,
        ID_TEXT_CLASSIFIER,
        ID_NEW_SCALE_CHART
        };
    };

// Define a new frame
class MyFrame final : public wxMDIParentFrame
    {
  public:
    MyFrame();
    static wxMenuBar* CreateMainMenubar();

  private:
    void InitToolBar(wxToolBar* toolBar);

    void OnAbout([[maybe_unused]] wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnQuit([[maybe_unused]] wxCommandEvent& event);
    void OnCloseAll([[maybe_unused]] wxCommandEvent& event);
    void OnClose([[maybe_unused]] wxCommandEvent& event);
    void OnSaveWindow(wxCommandEvent& event);
    void OnPrintWindow(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnCopyWindow(wxCommandEvent& event);
    void OnTextClassifier([[maybe_unused]] wxCommandEvent& event);
    };

class MyChild final : public wxMDIChildFrame
    {
    friend MyFrame;

  public:
    MyChild(wxMDIParentFrame* parent);

  private:
    Wisteria::Canvas* m_canvas{ nullptr };
    };

#endif //__WISTERIA_DEMO_H__
