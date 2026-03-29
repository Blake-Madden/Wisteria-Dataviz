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
class WisteriaView final : public wxView
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

    /// @returns The rebuilder builder, which stores all data for serialization.
    [[nodiscard]]
    const Wisteria::ReportBuilder& GetReportBuilder() const noexcept
        {
        return m_reportBuilder;
        }

  private:
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

    void LoadProject();

    bool OnCreate(wxDocument* doc, long flags) final;

    void OnDraw([[maybe_unused]] wxDC* dc) final {}

    bool OnClose(bool deleteWindow) final;
    void OnSidebarClick(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnInsertDataset(wxCommandEvent& event);
    void OnPivotWider(wxCommandEvent& event);
    void OnPivotLonger(wxCommandEvent& event);
    void OnInsertPage(wxCommandEvent& event);
    void OnEditPage(wxCommandEvent& event);
    void OnDeletePage(wxCommandEvent& event);
    void OnConstantEdited(wxGridEvent& event);
    void OnAddConstant(wxCommandEvent& event);
    void OnDeleteConstant(wxCommandEvent& event);
    void OnRibbonAddConstant(wxCommandEvent& event);
    void OnGraphDropdown(wxCommandEvent& event);
    void OnInsertChernoffPlot(wxCommandEvent& event);
    void OnInsertScatterPlot(wxCommandEvent& event);
    void OnInsertBubblePlot(wxCommandEvent& event);
    void OnInsertLinePlot(wxCommandEvent& event);
    void OnInsertMultiSeriesLinePlot(wxCommandEvent& event);
    void OnInsertWCurvePlot(wxCommandEvent& event);
    void OnInsertLRRoadmap(wxCommandEvent& event);
    void OnInsertProConRoadmap(wxCommandEvent& event);
    void OnInsertGanttChart(wxCommandEvent& event);
    void OnInsertCandlestickPlot(wxCommandEvent& event);
    void OnInsertSankeyDiagram(wxCommandEvent& event);
    void OnInsertBoxPlot(wxCommandEvent& event);
    void OnInsertLikertChart(wxCommandEvent& event);
    void OnInsertHeatMap(wxCommandEvent& event);
    void OnInsertHistogram(wxCommandEvent& event);
    void OnInsertWordCloud(wxCommandEvent& event);
    void OnInsertWLSparkline(wxCommandEvent& event);
    void OnInsertStemAndLeaf(wxCommandEvent& event);
    void OnInsertPieChart(wxCommandEvent& event);
    void OnInsertWaffleChart(wxCommandEvent& event);
    void EditWaffleChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void OnInsertTable(wxCommandEvent& event);
    void OnInsertCatBarChart(wxCommandEvent& event);
    void EditCatBarChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void OnInsertLabel(wxCommandEvent& event);
    void EditLabel(Wisteria::GraphItems::Label& label, Wisteria::Canvas* canvas, size_t labelRow,
                   size_t labelCol);
    void OnInsertImage(wxCommandEvent& event);
    void EditImage(Wisteria::GraphItems::Image& image, Wisteria::Canvas* canvas, size_t imageRow,
                   size_t imageCol);
    void OnInsertShape(wxCommandEvent& event);
    void EditShape(Wisteria::GraphItems::Shape& shape, Wisteria::Canvas* canvas, size_t shapeRow,
                   size_t shapeCol);
    void EditFillableShape(Wisteria::GraphItems::FillableShape& shape, Wisteria::Canvas* canvas,
                           size_t shapeRow, size_t shapeCol);
    void OnEditItem(wxCommandEvent& event);
    void OnDeleteItem(wxCommandEvent& event);
    void OnCanvasDClick(wxCommandEvent& event);
    void EditScatterPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void EditBubblePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                        size_t graphCol);
    void EditChernoffPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                          size_t graphRow, size_t graphCol);
    void EditLinePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                      size_t graphCol);
    void EditMultiSeriesLinePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 size_t graphRow, size_t graphCol);
    void EditWCurvePlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                        size_t graphCol);
    void EditLRRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditProConRoadmap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol);
    void EditBoxPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                     size_t graphCol);
    void EditLikertChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void EditHeatMap(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                     size_t graphCol);
    void EditHistogram(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditWordCloud(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                       size_t graphCol);
    void EditWLSparkline(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void EditStemAndLeaf(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol);
    void EditPieChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                      size_t graphCol);
    void EditGanttChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                        size_t graphCol);
    void EditCandlestickPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                             size_t graphRow, size_t graphCol);
    void EditSankeyDiagram(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol);
    void EditTable(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                   size_t graphCol);
    void PlaceGraphWithLegend(Wisteria::Canvas* canvas,
                              const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
                              std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend,
                              size_t graphRow, size_t graphCol,
                              Wisteria::UI::LegendPlacement legendPlacement);
    void UpdateGraphButtonStates();
    void
    AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                        const wxString& name,
                        const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo = {},
                        const Wisteria::ReportBuilder::DatasetImportOptions& importOptions = {});
    void AddPageToProject(size_t rows, size_t columns, const wxString& name);
    void ApplyColumnHeaderIcons(wxGrid* grid, Wisteria::UI::DatasetGridTable* table);
    void AdjustGridColumnsForIcons(wxGrid* grid);
    void PopulateConstantsGrid();
    void BuildGraphMenus();

    /// @returns The sidebar icon that displays where a dataset come from
    ///     (i.e., imported vs. a pivot operation).
    [[nodiscard]]
    size_t GetDatasetIconFromName(const wxString& name) const;

    [[nodiscard]]
    Wisteria::Canvas* GetActiveCanvas() noexcept;

    [[nodiscard]]
    bool IsPageSelected() const noexcept;

    static void UpdateCanvas(Wisteria::Canvas* canvas);

    wxDocChildFrame* m_frame{ nullptr };
    wxSplitterWindow* m_splitter{ nullptr };
    Wisteria::UI::SideBar* m_sideBar{ nullptr };
    wxPanel* m_workArea{ nullptr };
    wxRibbonButtonBar* m_graphButtonBar{ nullptr };
    wxRibbonButtonBar* m_pagesButtonBar{ nullptr };
    wxRibbonButtonBar* m_objectsButtonBar{ nullptr };
    wxGrid* m_constantsGrid{ nullptr };
    bool m_sidebarShown{ true };

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
    constexpr static size_t CONSTANTS_ICON_INDEX{ 2 };
    constexpr static size_t DATA_PIVOT_WIDER_ICON_INDEX{ 3 };
    constexpr static size_t DATA_PIVOT_LONGER_ICON_INDEX{ 4 };
    constexpr static size_t DATA_SUBSET_ICON_INDEX{ 5 };
    constexpr static size_t DATA_JOIN_ICON_INDEX{ 6 };

    wxDECLARE_DYNAMIC_CLASS(WisteriaView);
    };

#endif // WISTERIA_VIEW_H
