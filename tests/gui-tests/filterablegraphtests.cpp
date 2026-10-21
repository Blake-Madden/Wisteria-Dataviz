///////////////////////////////////////////////////////////////////////////////
// Name:        filterablegraphtests.cpp
// Purpose:     Tests for FilterableGraph and its HTML dashboard export
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

#include "../../src/graphs/chernoffplot.h"
#include "../../src/graphs/filterablegraph.h"
#include "../../src/graphs/piechart.h"
#include "../../src/reporting/htmldashboardprintout.h"
#include "../../src/reporting/reportbuilder.h"
#include "graphrenderharness.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include <wx/file.h>
#include <wx/filename.h>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

// Regions are stored out of alphabetical order to check how variants are sorted.
// East: Apples x2, Bananas
// North: Bananas x2, Cherries x2
// West: Apples, Cherries x2
[[nodiscard]]
static std::shared_ptr<Dataset> MakeRegionDataset(const bool includeWest = true)
    {
    auto dataset = std::make_shared<Dataset>();
    dataset->AddCategoricalColumn(L"Region", { { 0, L"West" }, { 1, L"East" }, { 2, L"North" } });
    dataset->AddCategoricalColumn(L"Product",
                                  { { 0, L"Apples" }, { 1, L"Bananas" }, { 2, L"Cherries" } });
    const std::vector<std::pair<GroupIdType, GroupIdType>> rows{ { 1, 0 }, { 1, 0 }, { 1, 1 },
                                                                 { 2, 1 }, { 2, 1 }, { 2, 2 },
                                                                 { 2, 2 }, { 0, 0 }, { 0, 2 },
                                                                 { 0, 2 } };
    for (size_t idx = 0; idx < rows.size(); ++idx)
        {
        if (!includeWest && rows[idx].first == 0)
            {
            continue;
            }
        dataset->AddRow(RowInfo()
                            .Categoricals({ rows[idx].first, rows[idx].second })
                            .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
        }
    return dataset;
    }

[[nodiscard]]
static FilterableGraph::GraphFactory MakePieFactory(Canvas* canvas)
    {
    return [canvas](const std::shared_ptr<const Dataset>& subset)
    {
        auto chart = std::make_shared<PieChart>(canvas);
        chart->SetData(subset, std::nullopt, L"Product", std::nullopt);
        return chart;
    };
    }

[[nodiscard]]
static size_t ActiveRowCount(const FilterableGraph& filterableGraph)
    {
    return filterableGraph.GetActiveGraph()->GetDataset()->GetRowCount();
    }

TEST_CASE("FilterableGraph builds a sorted variant per filter value", "[filterablegraph]")
    {
    auto* canvas = MakeCanvas();
    const FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"North",
                                           MakePieFactory(canvas) };

    REQUIRE(filterableGraph.GetVariants().size() == 4);
    CHECK(filterableGraph.GetVariants()[0].m_label == L"(All)");
    CHECK(filterableGraph.GetVariants()[1].m_label == L"East");
    CHECK(filterableGraph.GetVariants()[2].m_label == L"North");
    CHECK(filterableGraph.GetVariants()[3].m_label == L"West");
    CHECK(filterableGraph.GetFilterColumn() == L"Region");

    CHECK(filterableGraph.GetVariants()[0].m_graph->GetDataset()->GetRowCount() == 10);
    CHECK(filterableGraph.GetVariants()[1].m_graph->GetDataset()->GetRowCount() == 3);
    CHECK(filterableGraph.GetVariants()[2].m_graph->GetDataset()->GetRowCount() == 4);
    CHECK(filterableGraph.GetVariants()[3].m_graph->GetDataset()->GetRowCount() == 3);
    }

TEST_CASE("FilterableGraph unfiltered option", "[filterablegraph]")
    {
    auto* canvas = MakeCanvas();
    FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"(All)",
                                     MakePieFactory(canvas) };

    REQUIRE(filterableGraph.GetVariants().size() == 4);
    CHECK(filterableGraph.GetVariants()[0].m_label == L"(All)");
    CHECK(filterableGraph.GetActiveIndex() == 0);
    CHECK(ActiveRowCount(filterableGraph) == 10);

    filterableGraph.SetAllLabel(L"Everywhere");
    CHECK(filterableGraph.GetVariants()[0].m_label == L"Everywhere");
    CHECK(filterableGraph.FindVariant(L"everywhere") == std::optional<size_t>{ 0 });
    }

TEST_CASE("FilterableGraph default and active values", "[filterablegraph]")
    {
    auto* canvas = MakeCanvas();

    SECTION("Default value is shown first")
        {
        const FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"north",
                                               MakePieFactory(canvas) };
        CHECK(filterableGraph.GetActiveIndex() == 2);
        CHECK(ActiveRowCount(filterableGraph) == 4);
        }

    SECTION("Missing default value falls back to the unfiltered variant")
        {
        const FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"South",
                                               MakePieFactory(canvas) };
        CHECK(filterableGraph.GetActiveIndex() == 0);
        CHECK(ActiveRowCount(filterableGraph) == 10);
        }

    SECTION("Changing the active value")
        {
        FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"East",
                                         MakePieFactory(canvas) };
        const auto firstGraph = filterableGraph.GetActiveGraph();
        CHECK(filterableGraph.SetActiveValue(L"West"));
        CHECK(filterableGraph.GetActiveIndex() == 3);
        CHECK(filterableGraph.GetActiveGraph() != firstGraph);

        CHECK_FALSE(filterableGraph.SetActiveValue(L"South"));
        CHECK(filterableGraph.GetActiveIndex() == 3);

        // out of range is ignored
        filterableGraph.SetActiveIndex(10);
        CHECK(filterableGraph.GetActiveIndex() == 3);
        }
    }

TEST_CASE("FilterableGraph invalid input", "[filterablegraph]")
    {
    auto* canvas = MakeCanvas();

    CHECK_THROWS_AS(
        FilterableGraph(MakeRegionDataset(), L"Country", L"East", MakePieFactory(canvas)),
        std::runtime_error);
    CHECK_THROWS_AS(FilterableGraph(nullptr, L"Region", L"East", MakePieFactory(canvas)),
                    std::runtime_error);
    CHECK_THROWS_AS(FilterableGraph(MakeRegionDataset(), L"Region", L"East",
                                    [](const std::shared_ptr<const Dataset>&) { return nullptr; }),
                    std::runtime_error);
    }

TEST_CASE("FilterableGraph on a canvas", "[filterablegraph]")
    {
    wxLogNull noLog;
    auto* canvas = MakeCanvas();
    auto filterableGraph = std::make_shared<FilterableGraph>(MakeRegionDataset(), L"Region",
                                                             L"East", MakePieFactory(canvas));
    auto legend = filterableGraph->CreateLegend(
        LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
    REQUIRE(legend != nullptr);
    canvas->SetFixedObjectsGridSize(1, 2);
    canvas->SetFixedObject(0, 0, filterableGraph);
    canvas->SetFixedObject(0, 1, legend);

    wxBitmap bmp{ CANVAS_WIDTH, CANVAS_HEIGHT, 32 };
    wxMemoryDC memDc{ bmp };
    wxGCDC gcdc{ memDc };
    canvas->CalcAllSizes(gcdc);

    SECTION("Layout goes to the active graph")
        {
        const wxRect graphBox{ filterableGraph->GetBoundingBox(gcdc) };
        CHECK_FALSE(graphBox.IsEmpty());
        const GraphItemBase& activeGraph{ *filterableGraph->GetActiveGraph() };
        CHECK(graphBox == activeGraph.GetBoundingBox(gcdc));
        }

    SECTION("Chart IDs are assigned to the graph and its legend")
        {
        canvas->FillChartIds();
        CHECK(filterableGraph->GetId() != wxID_ANY);
        CHECK(legend->GetId() == filterableGraph->GetId());
        }

    SECTION("Legend follows the active variant")
        {
        CHECK(legend->GetText().Contains(L"Apples"));
        CHECK_FALSE(legend->GetText().Contains(L"Cherries"));
        filterableGraph->SetActiveValue(L"North");
        CHECK(legend->GetText().Contains(L"Cherries"));
        CHECK_FALSE(legend->GetText().Contains(L"Apples"));
        // the temporary legends built while switching aren't left registered on the graphs
        for (const auto& filteredGraph : filterableGraph->GetVariants())
            {
            CHECK(filteredGraph.m_graph->GetCreatedLegends().empty());
            }
        }

    SECTION("Canvas finds the filterable graph")
        {
        const auto filterableGraphs = canvas->GetFilterableGraphs();
        REQUIRE(filterableGraphs.size() == 1);
        CHECK(filterableGraphs.front() == filterableGraph);
        }
    }

TEST_CASE("FilterableGraph rebuilds a specialized legend in its canvas cell", "[filterablegraph]")
    {
    wxLogNull noLog;
    auto* canvas = MakeCanvas();
    auto dataset = std::make_shared<Dataset>();
    dataset->AddCategoricalColumn(L"Region", { { 0, L"East" }, { 1, L"West" } });
    dataset->AddContinuousColumn(L"Width");
    dataset->AddContinuousColumn(L"Height");
    for (size_t idx = 0; idx < 6; ++idx)
        {
        dataset->AddRow(
            RowInfo()
                .Categoricals({ static_cast<GroupIdType>(idx % 2) })
                .Continuous({ static_cast<double>(idx * 10), static_cast<double>(60 - idx * 5) })
                .Id(wxString::Format(L"face%d", static_cast<int>(idx))));
        }

    auto filterableGraph = std::make_shared<FilterableGraph>(
        dataset, L"Region", L"East",
        [canvas](const std::shared_ptr<const Dataset>& subset)
        {
            auto chart = std::make_shared<ChernoffFacesPlot>(canvas);
            chart->SetData(subset, L"Width", L"Height");
            chart->SetPropertyTemplate(L"enhanced-legend", L"true");
            return chart;
        });
    const auto legend = filterableGraph->CreateLegend(
        LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph),
        &ReportBuilder::CreateGraphLegend);
    REQUIRE(legend != nullptr);
    CHECK(legend->IsKindOf(wxCLASSINFO(ChernoffFacesPlot::ChernoffLegend)));

    canvas->SetFixedObjectsGridSize(1, 2);
    canvas->SetFixedObject(0, 0, filterableGraph);
    canvas->SetFixedObject(0, 1, legend);
    canvas->FillChartIds();
    const auto legendId{ legend->GetId() };

    // the legend can't be copied, so switching puts a new one in the same cell
    REQUIRE(filterableGraph->SetActiveValue(L"West"));
    const auto newLegend = canvas->GetFixedObject(0, 1);
    REQUIRE(newLegend != nullptr);
    CHECK(newLegend != legend);
    CHECK(newLegend->IsKindOf(wxCLASSINFO(ChernoffFacesPlot::ChernoffLegend)));
    CHECK(newLegend == filterableGraph->GetLegend());
    CHECK(newLegend->GetId() == legendId);
    }

TEST_CASE("FilterableGraph dashboard export", "[filterablegraph][dashboard]")
    {
    wxLogNull noLog;
    auto* canvas = MakeCanvas();
    auto filterableGraph = std::make_shared<FilterableGraph>(MakeRegionDataset(), L"Region",
                                                             L"North", MakePieFactory(canvas));
    canvas->SetFixedObjectsGridSize(1, 1);
    canvas->SetFixedObject(0, 0, filterableGraph);

    const wxString filePath{ wxFileName::CreateTempFileName(L"wisteria-dashboard") };
    HtmlDashboardOptions options{ filePath };
    options.m_title = L"Filterable graph test";
    const HtmlDashboardPrintout printout{ std::vector<Canvas*>{ canvas }, options };

    wxString html;
    wxFile htmlFile{ filePath };
    REQUIRE(htmlFile.IsOpened());
    REQUIRE(htmlFile.ReadAll(&html, wxConvUTF8));
    htmlFile.Close();
    wxRemoveFile(filePath);

    // one version of the page per region, plus the unfiltered one
    size_t choiceCount{ 0 };
    for (size_t pos = html.find(L"class=\"page-variant\""); pos != wxString::npos;
         pos = html.find(L"class=\"page-variant\"", pos + 1))
        {
        ++choiceCount;
        }
    CHECK(choiceCount == 4);
    CHECK(html.Contains(L"<div class=\"page-filter\" data-column=\"Region\" data-link=\"\" "
                        "hidden><span>(All)</span><span>East</span><span>North</span>"
                        "<span>West</span></div>"));
    CHECK(html.Contains(L"data-variant-choices=\"2\" data-variant-default>"));
    CHECK(html.Contains(L"data-variant-choices=\"0\" hidden>"));

    // each choice is captioned below the graph, in room added to the page
    CHECK(html.Contains(L"Filters: <tspan font-weight=\"bold\">Region</tspan> filtered by "
                        "<tspan font-style=\"italic\">North</tspan></text>"));
    CHECK(html.Contains(L"Filters: <tspan font-weight=\"bold\">Region</tspan> unfiltered</text>"));
    CHECK(html.Contains(L"--page-w:1280;--page-h:748"));
    CHECK(html.Contains(L"viewBox=\"0 0 1280 748\""));

    // every filter choice is a separate render, but element IDs must still be unique
    std::set<wxString> elementIds;
    const wxString idAttr{ L" id=\"" };
    for (size_t pos = html.find(idAttr); pos != wxString::npos; pos = html.find(idAttr, pos + 1))
        {
        const size_t nameStart{ pos + idAttr.length() };
        const size_t nameEnd{ html.find(L'"', nameStart) };
        REQUIRE(nameEnd != wxString::npos);
        const wxString elementId{ html.substr(nameStart, nameEnd - nameStart) };
        CHECK(elementIds.insert(elementId).second);
        }

    // the canvas is left showing the variant it had before exporting
    CHECK(filterableGraph->GetActiveIndex() == 2);
    }

TEST_CASE("FilterableGraph dashboard export keeps different datasets in separate groups",
          "[filterablegraph][dashboard]")
    {
    wxLogNull noLog;
    auto* canvas = MakeCanvas();
    auto firstGraph = std::make_shared<FilterableGraph>(MakeRegionDataset(), L"Region", L"East",
                                                        MakePieFactory(canvas));
    // this graph has no West data
    auto secondGraph = std::make_shared<FilterableGraph>(
        MakeRegionDataset(false), L"Region", L"East",
        [canvas](const std::shared_ptr<const Dataset>& subset)
        {
            auto chart = std::make_shared<PieChart>(canvas);
            chart->SetData(subset, std::nullopt, L"Product", std::nullopt);
            chart->GetTitle().SetText(L"SecondGraphTitle");
            return chart;
        });
    REQUIRE(secondGraph->GetVariants().size() == 3);
    canvas->SetFixedObjectsGridSize(1, 2);
    canvas->SetFixedObject(0, 0, firstGraph);
    canvas->SetFixedObject(0, 1, secondGraph);

    const wxString filePath{ wxFileName::CreateTempFileName(L"wisteria-dashboard") };
    HtmlDashboardOptions options{ filePath };
    const HtmlDashboardPrintout printout{ std::vector<Canvas*>{ canvas }, options };

    wxString html;
    wxFile htmlFile{ filePath };
    REQUIRE(htmlFile.IsOpened());
    REQUIRE(htmlFile.ReadAll(&html, wxConvUTF8));
    htmlFile.Close();
    wxRemoveFile(filePath);

    // the graphs filter the same column of different datasets, so each has its own group
    CHECK(html.Contains(L"<div class=\"page-filter\" data-column=\"Region (1)\" data-link=\"\" "
                        "hidden><span>(All)</span><span>East</span><span>North</span>"
                        "<span>West</span></div>"));
    CHECK(html.Contains(L"<div class=\"page-filter\" data-column=\"Region (2)\" data-link=\"\" "
                        "hidden><span>(All)</span><span>East</span><span>North</span></div>"));

    // a version of the page for every combination of the two groups' choices
    size_t choiceCount{ 0 };
    for (size_t pos = html.find(L"class=\"page-variant\""); pos != wxString::npos;
         pos = html.find(L"class=\"page-variant\"", pos + 1))
        {
        ++choiceCount;
        }
    CHECK(choiceCount == 12);
    CHECK(html.Contains(L"data-variant-choices=\"1-1\" data-variant-default>"));

    // the second graph is shown with West chosen in the first group
    const size_t westPos{ html.find(L"data-variant-choices=\"3-1\"") };
    REQUIRE(westPos != wxString::npos);
    CHECK(html.substr(westPos, html.find(L"</div>", westPos) - westPos)
              .Contains(L"SecondGraphTitle"));

    // the canvas is restored after exporting
    CHECK(secondGraph->IsShown());
    CHECK(secondGraph->GetActiveIndex() == 1);
    CHECK(firstGraph->GetActiveIndex() == 1);
    }

TEST_CASE("FilterableGraph dashboard export links pages filtering the same data",
          "[filterablegraph][dashboard]")
    {
    wxLogNull noLog;
    const auto sharedData = MakeRegionDataset();
    auto* firstCanvas = MakeCanvas();
    auto* secondCanvas = MakeCanvas();
    auto* thirdCanvas = MakeCanvas();
    auto firstGraph = std::make_shared<FilterableGraph>(sharedData, L"Region", L"North",
                                                        MakePieFactory(firstCanvas));
    // a different default than the page it is linked to
    auto secondGraph = std::make_shared<FilterableGraph>(sharedData, L"Region", L"West",
                                                         MakePieFactory(secondCanvas));
    // the same column, but of a different dataset
    auto thirdGraph = std::make_shared<FilterableGraph>(MakeRegionDataset(), L"Region", L"East",
                                                        MakePieFactory(thirdCanvas));
    firstCanvas->SetFixedObjectsGridSize(1, 1);
    firstCanvas->SetFixedObject(0, 0, firstGraph);
    secondCanvas->SetFixedObjectsGridSize(1, 1);
    secondCanvas->SetFixedObject(0, 0, secondGraph);
    thirdCanvas->SetFixedObjectsGridSize(1, 1);
    thirdCanvas->SetFixedObject(0, 0, thirdGraph);

    const wxString filePath{ wxFileName::CreateTempFileName(L"wisteria-dashboard") };
    HtmlDashboardOptions options{ filePath };
    const HtmlDashboardPrintout printout{
        std::vector<Canvas*>{ firstCanvas, secondCanvas, thirdCanvas }, options
    };

    wxString html;
    wxFile htmlFile{ filePath };
    REQUIRE(htmlFile.IsOpened());
    REQUIRE(htmlFile.ReadAll(&html, wxConvUTF8));
    htmlFile.Close();
    wxRemoveFile(filePath);

    const auto getPageMarkup = [&html](const size_t pageIndex)
    {
        const size_t startPos{ html.find(wxString::Format(L"id=\"page-%zu\"", pageIndex)) };
        if (startPos == wxString::npos)
            {
            return wxString{};
            }
        const size_t endPos{ html.find(L"</section>", startPos) };
        return html.substr(startPos, endPos - startPos);
    };

    const wxString firstPage{ getPageMarkup(0) };
    const wxString secondPage{ getPageMarkup(1) };
    const wxString thirdPage{ getPageMarkup(2) };
    CHECK(firstPage.Contains(L"data-column=\"Region\" data-link=\"0\""));
    CHECK(secondPage.Contains(L"data-column=\"Region\" data-link=\"0\""));
    CHECK(thirdPage.Contains(L"data-column=\"Region\" data-link=\"\""));

    // the linked page opens on the first page's choice (North), not its own (West)
    CHECK(firstPage.Contains(L"data-variant-choices=\"2\" data-variant-default>"));
    CHECK(secondPage.Contains(L"data-variant-choices=\"2\" data-variant-default>"));
    CHECK(secondPage.Contains(L"data-variant-choices=\"3\" hidden>"));
    // the unlinked page keeps its own choice (East)
    CHECK(thirdPage.Contains(L"data-variant-choices=\"1\" data-variant-default>"));

    // the canvas is left showing the variant it had before exporting
    CHECK(secondGraph->GetActiveIndex() == 3);
    }

TEST_CASE("Dashboard export scopes checkbox filters to their page", "[dashboard]")
    {
    wxLogNull noLog;
    auto* firstCanvas = MakeCanvas();
    auto* secondCanvas = MakeCanvas();
    // the same labels on both pages, but from different datasets
    auto firstPie = std::make_shared<PieChart>(firstCanvas);
    firstPie->SetData(MakeRegionDataset(), std::nullopt, L"Product", std::nullopt);
    auto secondPie = std::make_shared<PieChart>(secondCanvas);
    secondPie->SetData(MakeRegionDataset(false), std::nullopt, L"Product", std::nullopt);
    firstCanvas->SetFixedObjectsGridSize(1, 1);
    firstCanvas->SetFixedObject(0, 0, firstPie);
    secondCanvas->SetFixedObjectsGridSize(1, 1);
    secondCanvas->SetFixedObject(0, 0, secondPie);

    const wxString filePath{ wxFileName::CreateTempFileName(L"wisteria-dashboard") };
    HtmlDashboardOptions options{ filePath };
    // a missing page is skipped and doesn't take up a page index
    const HtmlDashboardPrintout printout{
        std::vector<Canvas*>{ firstCanvas, nullptr, secondCanvas }, options
    };

    wxString html;
    wxFile htmlFile{ filePath };
    REQUIRE(htmlFile.IsOpened());
    REQUIRE(htmlFile.ReadAll(&html, wxConvUTF8));
    htmlFile.Close();
    wxRemoveFile(filePath);

    const auto getPageMarkup = [&html](const size_t pageIndex)
    {
        const size_t startPos{ html.find(wxString::Format(L"id=\"page-%zu\"", pageIndex)) };
        if (startPos == wxString::npos)
            {
            return wxString{};
            }
        const size_t endPos{ html.find(L"</section>", startPos) };
        return html.substr(startPos, endPos - startPos);
    };

    // chart IDs restart on each page, so the chart ID alone can't tell the pies apart
    const wxString firstPage{ getPageMarkup(0) };
    const wxString secondPage{ getPageMarkup(1) };
    CHECK(getPageMarkup(2).empty());
    CHECK(firstPage.Contains(L"data-index=\"0\""));
    CHECK(secondPage.Contains(L"data-index=\"1\""));
    CHECK(firstPage.Contains(L"data-chart-id=\"0\""));
    CHECK(secondPage.Contains(L"data-chart-id=\"0\""));
    CHECK(firstPage.Contains(L"data-filter=\"Apples\""));
    CHECK(secondPage.Contains(L"data-filter=\"Apples\""));

    // filter keys include the page GUID (or index) along with the chart ID
    CHECK(html.Contains(L"return (page ? (page.dataset.guid || page.dataset.index) : '') + ':' + "
                        "scopeIdFor(el);"));
    CHECK(html.Contains(L"box.dataset.filterKey = pages[pageIndex].guid + ':' + section.chartId + "
                        "'|' + entry.key;"));
    CHECK(html.Contains(L"const key = filterScopeFor(el) + '|' + el.getAttribute('data-filter');"));
    CHECK(html.Contains(L"const scope = filterScopeFor(hit);"));
    CHECK_FALSE(html.Contains(L"const key = scopeIdFor(el) + '|'"));

    // the group checkboxes are left out by default
    CHECK(html.Contains(L"const hasGroupToggles = false;"));
    CHECK(html.Contains(L"if (!hasGroupToggles) return;"));
    }

TEST_CASE("Dashboard export includes group visibility toggles when requested", "[dashboard]")
    {
    wxLogNull noLog;
    auto* canvas = MakeCanvas();
    auto pie = std::make_shared<PieChart>(canvas);
    pie->SetData(MakeRegionDataset(), std::nullopt, L"Product", std::nullopt);
    canvas->SetFixedObjectsGridSize(1, 1);
    canvas->SetFixedObject(0, 0, pie);

    const wxString filePath{ wxFileName::CreateTempFileName(L"wisteria-dashboard") };
    HtmlDashboardOptions options{ filePath };
    CHECK_FALSE(options.m_includeGroupVisibilityToggles);
    options.IncludeGroupVisibilityToggles(true);
    const HtmlDashboardPrintout printout{ std::vector<Canvas*>{ canvas }, options };

    wxString html;
    wxFile htmlFile{ filePath };
    REQUIRE(htmlFile.IsOpened());
    REQUIRE(htmlFile.ReadAll(&html, wxConvUTF8));
    htmlFile.Close();
    wxRemoveFile(filePath);

    CHECK(html.Contains(L"const hasGroupToggles = true;"));
    CHECK_FALSE(html.Contains(L"{{GROUPTOGGLES}}"));
    // the graph's groups are still marked, so they can be hidden
    CHECK(html.Contains(L"data-filter=\"Apples\""));
    }

TEST_CASE("FilterableGraph from a report configuration", "[filterablegraph][reportbuilder]")
    {
    wxLogNull noLog;
    const wxString tempDir{ wxFileName::GetTempDir() + wxFileName::GetPathSeparator() };
    const wxString csvPath{ tempDir + L"wisteria-filterable-graph-test.csv" };
    const wxString configPath{ tempDir + L"wisteria-filterable-graph-test.json" };

        {
        wxFile csvFile{ csvPath, wxFile::write };
        REQUIRE(csvFile.IsOpened());
        csvFile.Write(L"Region,Product\nEast,Apples\nEast,Apples\nEast,Bananas\n"
                      "North,Bananas\nNorth,Bananas\nNorth,Cherries\nNorth,Cherries\n"
                      "West,Apples\nWest,Cherries\nWest,Cherries\n");
        }
        {
        wxFile configFile{ configPath, wxFile::write };
        REQUIRE(configFile.IsOpened());
        configFile.Write(LR"({
  "name": "Filterable graph test",
  "datasets": [
    {
      "name": "Sales",
      "path": "wisteria-filterable-graph-test.csv",
      "categorical-columns": [ { "name": "Region" }, { "name": "Product" } ]
    }
  ],
  "pages": [
    {
      "name": "Sales by region",
      "rows": [
        {
          "items": [
            {
              "type": "pie-chart",
              "dataset": "Sales",
              "variables": { "group-1": "Product" },
              "legend": { "placement": "right" },
              "filter": {
                "column": "Region",
                "default": "North",
                "all-label": "Everywhere"
              }
            }
          ]
        }
      ]
    }
  ]
})");
        }

    ReportBuilder builder;
    const auto pages = builder.LoadConfigurationFile(configPath, wxTheApp->GetTopWindow());
    wxRemoveFile(csvPath);
    wxRemoveFile(configPath);

    REQUIRE(pages.size() == 1);
    const auto filterableGraphs = pages.front()->GetFilterableGraphs();
    REQUIRE(filterableGraphs.size() == 1);
    const auto& filterableGraph = filterableGraphs.front();

    CHECK(filterableGraph->GetFilterColumn() == L"Region");
    CHECK(filterableGraph->GetPropertyTemplate(L"filter.column") == L"Region");
    REQUIRE(filterableGraph->GetVariants().size() == 4);
    CHECK(filterableGraph->GetVariants()[0].m_label == L"Everywhere");
    CHECK(filterableGraph->GetVariants()[filterableGraph->GetActiveIndex()].m_label == L"North");

    // each graph is built from its own subset, and the source dataset is left in place
    CHECK(filterableGraph->GetVariants()[0].m_graph->GetDataset()->GetRowCount() == 10);
    CHECK(ActiveRowCount(*filterableGraph) == 4);
    for (const auto& filteredGraph : filterableGraph->GetVariants())
        {
        CHECK(dynamic_cast<const PieChart*>(filteredGraph.m_graph.get()) != nullptr);
        CHECK(filteredGraph.m_graph->GetPropertyTemplate(L"dataset") == L"Sales");
        REQUIRE(filteredGraph.m_graph->GetLegendInfo().has_value());
        }
    CHECK(builder.GetDatasets().at(L"Sales")->GetRowCount() == 10);

    // the legend is placed to the right of the graph
    CHECK(pages.front()->GetFixedObject(0, 0) == filterableGraph);
    const auto* legend = dynamic_cast<const Label*>(pages.front()->GetFixedObject(0, 1).get());
    REQUIRE(legend != nullptr);
    CHECK(legend->IsLegend());
    CHECK(legend == filterableGraph->GetLegend().get());
    }
