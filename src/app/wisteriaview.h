///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_VIEW_H
#define WISTERIA_VIEW_H

#include "../base/reportbuilder.h"
#include "../ui/controls/sidebar.h"
#include "../ui/dialogs/insertgraphdlg.h"
#include "../util/windowcontainer.h"
#include <vector>
#include <wx/docview.h>
#include <wx/grid.h>
#include <wx/menu.h>
#include <wx/ribbon/art.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/splitter.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    class DatasetGridTable;
    }

/// @brief View class for Wisteria Dataviz projects.
class WisteriaView : public wxView
    {
  public:
    WisteriaView() = default;
    WisteriaView(const WisteriaView&) = delete;
    WisteriaView& operator=(const WisteriaView&) = delete;

    /// @brief Shows or hides the sidebar.
    /// @param show @c true to show the sidebar, @c false to hide it.
    void ShowSideBar(const bool show = true);

    /// @returns @c true if the sidebar is currently shown.
    [[nodiscard]]
    bool IsSideBarShown() const noexcept
        {
        return m_sidebarShown;
        }

    /// @returns The canvases/pages in the project.
    [[nodiscard]]
    const std::vector<Wisteria::Canvas*>& GetPages() const noexcept
        {
        return m_pages;
        }

  private:
    void LoadProject();

    [[nodiscard]]
    Wisteria::UI::SideBar* GetSideBar() noexcept
        {
        return m_sideBar;
        }

    [[nodiscard]]
    wxSplitterWindow* GetSplitter() noexcept
        {
        return m_splitter;
        }

    bool OnCreate(wxDocument* doc, long flags) override;

    void OnDraw([[maybe_unused]] wxDC* dc) override {}

    bool OnClose(bool deleteWindow) override;
    void OnSidebarClick(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnSaveProject(wxCommandEvent& event);
    void SaveProject(const wxString& filePath);

    // save helpers
    [[nodiscard]]
    static wxString EscapeJsonStr(const wxString& str);
    static void SaveDatasetImportOptions(wxSimpleJSON::Ptr_t& dsNode,
                                         const Wisteria::Data::Dataset::ColumnPreviewInfo& colInfo,
                                         const Wisteria::Data::ImportInfo& info);
    static void
    SaveTransformOptions(wxSimpleJSON::Ptr_t& dsNode,
                         const Wisteria::ReportBuilder::DatasetTransformOptions& txOpts);
    static void
    SaveFormulas(wxSimpleJSON::Ptr_t& dsNode,
                 const std::vector<Wisteria::ReportBuilder::DatasetFormulaInfo>& formulas);
    static void SaveSubsetFilters(wxSimpleJSON::Ptr_t& subsetNode,
                                  const Wisteria::ReportBuilder::DatasetSubsetOptions& sOpts);
    void SaveSubsets(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;
    void SavePivots(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;
    void SaveMerges(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;

    // page item save helpers
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SavePageItem(const Wisteria::GraphItems::GraphItemBase* item,
                                     const Wisteria::Canvas* canvas) const;
    void SaveItem(wxSimpleJSON::Ptr_t& itemNode, const Wisteria::GraphItems::GraphItemBase* item,
                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxString SavePenToStr(const wxPen& pen) const;
    [[nodiscard]]
    wxString SaveBrushToStr(const wxBrush& brush) const;
    [[nodiscard]]
    wxString SaveLabelPropertiesToStr(const Wisteria::GraphItems::Label& label) const;
    [[nodiscard]]
    wxString ColorToStr(const wxColour& color) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveLabel(const Wisteria::GraphItems::Label* label,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveImage(const Wisteria::GraphItems::Image* image,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveShape(const Wisteria::GraphItems::Shape* shape,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveFillableShape(const Wisteria::GraphItems::FillableShape* shape,
                                          const Wisteria::Canvas* canvas) const;
    void SaveGraph(const Wisteria::Graphs::Graph2D* graph, wxSimpleJSON::Ptr_t& graphNode,
                   const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveGraphByType(const Wisteria::Graphs::Graph2D* graph,
                                        const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    static wxString GetGraphTypeString(const Wisteria::Graphs::Graph2D* graph);
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveCommonAxis(const Wisteria::GraphItems::Axis* axis,
                                       const Wisteria::Canvas* canvas) const;

    void OnInsertDataset(wxCommandEvent& event);
    void OnPivotWider(wxCommandEvent& event);
    void OnPivotLonger(wxCommandEvent& event);
    void OnInsertPage(wxCommandEvent& event);
    void OnEditPage(wxCommandEvent& event);
    void
    AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                        const wxString& name,
                        const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo = {},
                        const Wisteria::ReportBuilder::DatasetImportOptions& importOptions = {});
    void AddPageToProject(size_t rows, size_t columns, const wxString& name);
    void ApplyColumnHeaderIcons(wxGrid* grid, Wisteria::UI::DatasetGridTable* table);
    void AdjustGridColumnsForIcons(wxGrid* grid);

    void BuildGraphMenus();
    void OnGraphDropdown(wxCommandEvent& event);
    void OnInsertChernoffPlot(wxCommandEvent& event);
    void OnInsertScatterPlot(wxCommandEvent& event);
    void OnInsertLinePlot(wxCommandEvent& event);
    void OnInsertWCurvePlot(wxCommandEvent& event);
    void OnInsertLRRoadmap(wxCommandEvent& event);
    void OnInsertProConRoadmap(wxCommandEvent& event);
    void OnInsertHistogram(wxCommandEvent& event);
    void OnInsertWordCloud(wxCommandEvent& event);
    void OnInsertWLSparkline(wxCommandEvent& event);
    void OnEditItem(wxCommandEvent& event);
    void OnDeleteItem(wxCommandEvent& event);
    void OnCanvasDClick(wxCommandEvent& event);
    void EditScatterPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void EditChernoffPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                          size_t graphRow, size_t graphCol);
    void EditLinePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                      size_t graphCol);
    void EditWCurvePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                        size_t graphCol);
    void EditLRRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditProConRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol);
    void EditHistogram(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditWordCloud(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditWLSparkline(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void PlaceGraphWithLegend(Wisteria::Canvas* canvas,
                              const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
                              std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend,
                              size_t graphRow, size_t graphCol,
                              Wisteria::UI::LegendPlacement legendPlacement);
    void UpdateGraphButtonStates();
    [[nodiscard]]
    Wisteria::Canvas* GetActiveCanvas() noexcept;
    [[nodiscard]]
    bool IsPageSelected() const noexcept;

    wxDocChildFrame* m_frame{ nullptr };
    wxSplitterWindow* m_splitter{ nullptr };
    Wisteria::UI::SideBar* m_sideBar{ nullptr };
    wxPanel* m_workArea{ nullptr };
    wxRibbonButtonBar* m_graphButtonBar{ nullptr };
    wxRibbonButtonBar* m_pagesButtonBar{ nullptr };
    bool m_sidebarShown{ true };
    wxString m_projectFilePath;

    Wisteria::ReportBuilder m_reportBuilder;
    std::vector<Wisteria::Canvas*> m_pages;
    WindowContainer m_workWindows;

    wxMenu m_saveMenu;
    wxMenu m_basicGraphMenu;
    wxMenu m_businessGraphMenu;
    wxMenu m_statisticalGraphMenu;
    wxMenu m_surveyGraphMenu;
    wxMenu m_educationGraphMenu;
    wxMenu m_socialGraphMenu;
    wxMenu m_sportsGraphMenu;

    constexpr static size_t DATA_ICON_INDEX{ 0 };
    constexpr static size_t PAGE_ICON_INDEX{ 1 };

    wxDECLARE_DYNAMIC_CLASS(WisteriaView);
    };

#endif // WISTERIA_VIEW_H
