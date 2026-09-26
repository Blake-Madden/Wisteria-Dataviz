///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_VIEW_H
#define WISTERIA_VIEW_H

#include "../reporting/reportbuilder.h"
#include "../ui/controls/sidebar.h"
#include "../ui/dialogs/editors/insertgraphdlg.h"
#include "../util/windowcontainer.h"
#include <functional>
#include <optional>
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
    void ShowSideBar(bool show = true);

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

    /// @brief Re-applies the global print settings (orientation, paper size) to all pages
    ///     and refreshes their on-screen aspect ratios.
    void RefreshPagePrintSettings();

    /// @returns The report builder, which stores all data for serialization.
    [[nodiscard]]
    const Wisteria::ReportBuilder& GetReportBuilder() const noexcept
        {
        return m_reportBuilder;
        }

    /// @private
    [[nodiscard]]
    Wisteria::ReportBuilder& GetReportBuilder() noexcept
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

    /// @brief Builds the project UI from @p filename.
    /// @param filename The project file to load (empty for a new project).
    /// @returns @c true if the project loaded successfully.
    bool LoadProject(const wxString& filename);

    bool OnCreate(wxDocument* doc, long flags) final;

    void OnDraw([[maybe_unused]] wxDC* dc) final {}

    bool OnClose(bool deleteWindow) final;
    void OnSidebarClick(const wxCommandEvent& event);
    void OnCopyItem(wxCommandEvent& event);
    void OnPasteItem(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnPrintSetup(wxCommandEvent& event);
    void OnSvgExport(wxCommandEvent& event);
    void OnHtmlExport(wxCommandEvent& event);
    void OnPdfExport(wxCommandEvent& event);
    void OnPptxExport(wxCommandEvent& event);
    void OnProjectSettings(wxCommandEvent& event);
    void OnInsertDataset(wxCommandEvent& event);
    void OnEditDataset(wxCommandEvent& event);
    void OnDeleteDataset(wxCommandEvent& event);
    void OnPivotWider(wxCommandEvent& event);
    void OnPivotLonger(wxCommandEvent& event);
    void OnSubsetDataset(wxCommandEvent& event);
    void OnJoinDataset(wxCommandEvent& event);
    void OnInsertPage(wxCommandEvent& event);
    void OnEditPage(wxCommandEvent& event);
    void OnDeletePage(wxCommandEvent& event);
    void OnRearrangePages(wxCommandEvent& event);
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
    void OnInsertChoroplethMap(wxCommandEvent& event);
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
    void EditWaffleChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void OnInsertRaceTrackChart(wxCommandEvent& event);
    void EditRaceTrackChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                            size_t graphRow, size_t graphCol) const;
    void OnInsertNightingaleRoseChart(wxCommandEvent& event);
    void EditNightingaleRoseChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                  size_t graphRow, size_t graphCol) const;
    void OnInsertDuBoisSpiralChart(wxCommandEvent& event);
    void EditDuBoisSpiralChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                               size_t graphRow, size_t graphCol) const;
    void OnInsertPictograph(wxCommandEvent& event);
    void EditPictograph(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                        size_t graphRow, size_t graphCol) const;
    void OnInsertBulletChart(wxCommandEvent& event);
    void EditBulletChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void OnInsertWaterfallChart(wxCommandEvent& event);
    void EditWaterfallChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                            size_t graphRow, size_t graphCol) const;
    void OnInsertFunnelChart(wxCommandEvent& event);
    void EditFunnelChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void OnInsertWilmarthBridgePlot(wxCommandEvent& event);
    void EditWilmarthBridgePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                size_t graphRow, size_t graphCol) const;
    void OnInsertScaleChart(wxCommandEvent& event);
    void EditScaleChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                        size_t graphRow, size_t graphCol) const;
    void OnInsertTable(wxCommandEvent& event);
    void OnInsertCatBarChart(wxCommandEvent& event);
    void EditCatBarChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void OnInsertLabel(wxCommandEvent& event);
    void EditLabel(const Wisteria::GraphItems::Label& label, Wisteria::Canvas* canvas,
                   size_t labelRow, size_t labelCol) const;
    void OnInsertKpiCard(wxCommandEvent& event);
    void OnInsertSpacer(wxCommandEvent& event);
    void OnDividerDropdown(wxCommandEvent& event);
    void OnInsertDivider(wxCommandEvent& event);
    void OnInsertImage(wxCommandEvent& event);
    void EditImage(Wisteria::GraphItems::Image& image, Wisteria::Canvas* canvas, size_t imageRow,
                   size_t imageCol) const;
    void OnInsertShape(wxCommandEvent& event);
    void EditShape(const Wisteria::GraphItems::Shape& shape, Wisteria::Canvas* canvas,
                   size_t shapeRow, size_t shapeCol) const;
    void EditFillableShape(const Wisteria::GraphItems::FillableShape& shape,
                           Wisteria::Canvas* canvas, size_t shapeRow, size_t shapeCol) const;
    void OnInsertCommonAxis(wxCommandEvent& event);
    void EditCommonAxis(Wisteria::GraphItems::Axis& axis, Wisteria::Canvas* canvas, size_t axisRow,
                        size_t axisCol);
    void OnEditItem(wxCommandEvent& event);
    void OnDeleteItem(wxCommandEvent& event);
    void OnGoToDatasource(wxCommandEvent& event);
    void OnCanvasDClick(wxCommandEvent& event);
    void EditScatterPlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void EditBubblePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                        size_t graphRow, size_t graphCol) const;
    void EditChernoffPlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                          size_t graphRow, size_t graphCol) const;
    void EditLinePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                      size_t graphRow, size_t graphCol) const;
    void EditMultiSeriesLinePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                                 size_t graphRow, size_t graphCol) const;
    void EditWCurvePlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                        size_t graphRow, size_t graphCol) const;
    void EditLRRoadmap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                       size_t graphRow, size_t graphCol) const;
    void EditProConRoadmap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol) const;
    void EditBoxPlot(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                     size_t graphCol) const;
    void EditLikertChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void EditHeatMap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                     size_t graphRow, size_t graphCol) const;
    void EditHistogram(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                       size_t graphRow, size_t graphCol) const;
    void EditWordCloud(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                       size_t graphRow, size_t graphCol) const;
    void EditChoroplethMap(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol) const;
    void EditWLSparkline(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void EditStemAndLeaf(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                         size_t graphRow, size_t graphCol) const;
    void EditPieChart(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                      size_t graphRow, size_t graphCol) const;
    void EditGanttChart(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                        size_t graphCol) const;
    void EditCandlestickPlot(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                             size_t graphRow, size_t graphCol) const;
    void EditSankeyDiagram(const Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas,
                           size_t graphRow, size_t graphCol) const;
    void EditTable(Wisteria::Graphs::Graph2D& graph, Wisteria::Canvas* canvas, size_t graphRow,
                   size_t graphCol);
    void PlaceGraphWithLegend(Wisteria::Canvas* canvas,
                              const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
                              std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend,
                              size_t graphRow, size_t graphCol,
                              Wisteria::UI::LegendPlacement legendPlacement) const;

  public:
    /// @brief Places a graph and its optional legend into a canvas's grid, growing the
    ///     grid to make room for the legend if needed.
    /// @details This is the canvas/grid-mutating core of PlaceGraphWithLegend(),
    ///     without the live-view refresh (UpdateCanvas()) or document-modified flagging.
    ///     So it can also be used against a disposable staging canvas (e.g., the object gallery).
    /// @param canvas The canvas whose grid to place into.
    /// @param plot The graph to place.
    /// @param legend The graph's legend, or @c nullptr for none.
    /// @param graphRow The row to place the graph at.
    /// @param graphCol The column to place the graph at.
    /// @param legendPlacement Where the legend goes, relative to the graph.
    static void PlaceGraphAndLegendInGrid(
        Wisteria::Canvas* canvas, const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& plot,
        std::unique_ptr<Wisteria::GraphItems::GraphItemBase> legend, size_t graphRow,
        size_t graphCol, Wisteria::UI::LegendPlacement legendPlacement);

  private:
    void UpdateGraphButtonStates() const;
    void UpdateDatasetButtonStates() const;
    void AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                             const wxString& name,
                             const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo,
                             const Wisteria::ReportBuilder::DatasetImportOptions& importOptions);
    // non-imported datasets (e.g., subsets or pivots from imported data)
    void AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                             const wxString& name);
    void ApplyGlobalPrintSettings(Wisteria::Canvas* canvas);
    Wisteria::Canvas* AddPageToProject(size_t rows, size_t columns, const wxString& name,
                                       std::optional<size_t> position = std::nullopt);
    void RemovePageFromProject(Wisteria::Canvas* canvas);
    static std::optional<size_t> ParseDefaultPageNumber(const wxString& name);
    static void ApplyColumnHeaderIcons(const wxGrid* grid, Wisteria::UI::DatasetGridTable* table);
    static void AdjustGridColumnsForIcons(wxGrid* grid);
    /// @brief Destroys all work-area windows (canvases, dataset grids, constants grid),
    ///     drops the sidebar tree, and resets the report model.
    void TearDownProject();
    /// @brief Serializes the in-memory project, tears it down, and rebuilds it from
    ///     that serialization, preserving the current sidebar selection and sash position.
    void ReloadProject();
    void PopulateConstantsGrid();
    void BuildGraphMenus();

  public:
    /// @brief Sets an icon (from an SVG) for a dialog.
    /// @param dlg The dialog to set the icon for.
    /// @param svgName The SVG resource name.
    static void SetDialogIcon(wxDialog& dlg, const wxString& svgName);

    /// @brief Helper to get the side and hint for a legend's placement.
    /// @param placement The legend's placement.
    /// @returns The side and hint for the legend's placement.
    [[nodiscard]]
    static std::pair<Wisteria::Side, Wisteria::LegendCanvasPlacementHint>
    GetLegendSideAndHint(Wisteria::UI::LegendPlacement placement);

    /// @brief Builds a graph's legend (via Graph2D::CreateLegend()), based on the
    ///     dialog's legend settings.
    /// @param dlg The graph dialog containing the legend settings.
    /// @param plot The graph to create the legend from.
    /// @param legendPlacement The placement the legend was requested at.
    /// @returns The legend, or @c nullptr if @p legendPlacement is LegendPlacement::None.
    [[nodiscard]]
    static std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
    BuildLegend(const Wisteria::UI::InsertGraphDlg& dlg, Wisteria::Graphs::Graph2D& plot,
                Wisteria::UI::LegendPlacement legendPlacement);

    /// @brief Builds a graph's legend from a caller-supplied factory, based on the
    ///     dialog's legend settings.
    /// @details Use this overload for graphs that offer more than one kind of legend
    ///     (e.g., Chernoff faces' enhanced legend, or a choropleth map's symbol legend),
    ///     where the caller needs to pick which @c Create*Legend() method to call.
    /// @param dlg The graph dialog containing the legend settings.
    /// @param legendPlacement The placement the legend was requested at.
    /// @param createLegend Builds the legend from the resolved legend options.
    /// @returns The legend, or @c nullptr if @p legendPlacement is LegendPlacement::None.
    [[nodiscard]]
    static std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
    BuildLegend(const Wisteria::UI::InsertGraphDlg& dlg,
                Wisteria::UI::LegendPlacement legendPlacement,
                const std::function<std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                    const Wisteria::Graphs::LegendOptions&)>& createLegend);

    /// @brief Clears a graph and its legend from a canvas.
    /// @param canvas The canvas to clear from.
    /// @param graph The graph to clear.
    /// @param graphRow The row of the graph.
    /// @param graphCol The column of the graph.
    static void ClearGraphAndLegend(Wisteria::Canvas* canvas,
                                    const Wisteria::Graphs::Graph2D& graph, size_t graphRow,
                                    size_t graphCol);

    /// @brief Carries forward a property template from an old graph to a new one.
    /// @param oldGraph The old graph.
    /// @param newGraph The new graph.
    /// @param prop The property name.
    /// @param newVal The new value for the property.
    /// @param oldExpanded The old expanded value of the property.
    static void CarryForwardProperty(const Wisteria::Graphs::Graph2D& oldGraph,
                                     Wisteria::Graphs::Graph2D& newGraph, const wxString& prop,
                                     const wxString& newVal, const wxString& oldExpanded);

    /// @brief Builds the JSON for an aggregate's start/end position.
    /// @param pos The position (bare name, number, or "last-row"/"last-column").
    /// @param dimension The "row"/"column" prefix to apply to a named position.
    /// @param offset The offset applied to the position (0 = none).
    /// @returns The JSON string for the position.
    [[nodiscard]]
    static wxString BuildAggPosJson(const wxString& pos, const wxString& dimension, int offset = 0);

  private:
    /// @returns The sidebar icon that displays where a dataset comes from
    ///     (i.e., imported vs. a pivot operation).
    [[nodiscard]]
    size_t GetDatasetIconFromName(const wxString& name) const;

    [[nodiscard]]
    Wisteria::Canvas* GetActiveCanvas() const noexcept;

    /// @brief Returns the active canvas, or prompts the user to select a page if none is active.
    /// @returns The active canvas, or @c nullptr if no pages exist or the user cancels.
    [[nodiscard]]
    Wisteria::Canvas* EnsureActivePage();

    [[nodiscard]]
    bool IsPageSelected() const noexcept;

    [[nodiscard]]
    bool IsDatasetSelected() const noexcept;

    [[nodiscard]]
    bool IsGraphSelected() const noexcept;

    [[nodiscard]]
    bool IsCanvasItemSelected() const noexcept;

    static void UpdateCanvas(Wisteria::Canvas* canvas);

    wxDocChildFrame* m_frame{ nullptr };
    wxSplitterWindow* m_splitter{ nullptr };
    Wisteria::UI::SideBar* m_sideBar{ nullptr };
    wxPanel* m_workArea{ nullptr };
    wxRibbonButtonBar* m_datasetButtonBar{ nullptr };
    wxRibbonButtonBar* m_graphButtonBar{ nullptr };
    wxRibbonButtonBar* m_pagesButtonBar{ nullptr };
    wxRibbonButtonBar* m_objectsButtonBar{ nullptr };
    wxRibbonButtonBar* m_sourcesButtonBar{ nullptr };
    wxGrid* m_constantsGrid{ nullptr };
    bool m_sidebarShown{ true };

    Wisteria::ReportBuilder m_reportBuilder;
    std::vector<Wisteria::Canvas*> m_pages;
    WindowContainer m_workWindows;

    wxMenu m_saveMenu;
    wxMenu m_dividerMenu;
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
