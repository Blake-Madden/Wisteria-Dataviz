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

    REQUIRE(filterableGraph.GetVariants().size() == 3);
    CHECK(filterableGraph.GetVariants()[0].m_label == L"East");
    CHECK(filterableGraph.GetVariants()[1].m_label == L"North");
    CHECK(filterableGraph.GetVariants()[2].m_label == L"West");
    CHECK_FALSE(filterableGraph.HasAllOption());
    CHECK(filterableGraph.GetFilterColumn() == L"Region");

    CHECK(filterableGraph.GetVariants()[0].m_graph->GetDataset()->GetRowCount() == 3);
    CHECK(filterableGraph.GetVariants()[1].m_graph->GetDataset()->GetRowCount() == 4);
    CHECK(filterableGraph.GetVariants()[2].m_graph->GetDataset()->GetRowCount() == 3);
    }

TEST_CASE("FilterableGraph unfiltered option", "[filterablegraph]")
    {
    auto* canvas = MakeCanvas();
    FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"(All)",
                                     MakePieFactory(canvas), true };

    REQUIRE(filterableGraph.GetVariants().size() == 4);
    CHECK(filterableGraph.HasAllOption());
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
        CHECK(filterableGraph.GetActiveIndex() == 1);
        CHECK(ActiveRowCount(filterableGraph) == 4);
        }

    SECTION("Missing default value falls back to the first variant")
        {
        const FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"South",
                                               MakePieFactory(canvas) };
        CHECK(filterableGraph.GetActiveIndex() == 0);
        }

    SECTION("Changing the active value")
        {
        FilterableGraph filterableGraph{ MakeRegionDataset(), L"Region", L"East",
                                         MakePieFactory(canvas) };
        const auto firstGraph = filterableGraph.GetActiveGraph();
        CHECK(filterableGraph.SetActiveValue(L"West"));
        CHECK(filterableGraph.GetActiveIndex() == 2);
        CHECK(filterableGraph.GetActiveGraph() != firstGraph);

        CHECK_FALSE(filterableGraph.SetActiveValue(L"South"));
        CHECK(filterableGraph.GetActiveIndex() == 2);

        // out of range is ignored
        filterableGraph.SetActiveIndex(10);
        CHECK(filterableGraph.GetActiveIndex() == 2);
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
    auto filterableGraph = std::make_shared<FilterableGraph>(
        MakeRegionDataset(), L"Region", L"North", MakePieFactory(canvas), true);
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
    CHECK(html.Contains(L"data-variant-column=\"Region\""));
    CHECK(html.Contains(L"data-variant-label=\"North\" data-variant-default>"));
    CHECK(html.Contains(L"data-variant-label=\"(All)\" hidden>"));

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

TEST_CASE("FilterableGraph dashboard export blanks graphs without the choice",
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
    REQUIRE(secondGraph->GetVariants().size() == 2);
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

    const auto getChoiceMarkup = [&html](const wxString& label)
    {
        const size_t startPos{ html.find(L"data-variant-label=\"" + label + L"\"") };
        if (startPos == wxString::npos)
            {
            return wxString{};
            }
        const size_t endPos{ html.find(L"</div>", startPos) };
        return html.substr(startPos, endPos - startPos);
    };

    CHECK(getChoiceMarkup(L"East").Contains(L"SecondGraphTitle"));
    CHECK(getChoiceMarkup(L"North").Contains(L"SecondGraphTitle"));
    // West must not show the second graph's East (or North) data
    const wxString westMarkup{ getChoiceMarkup(L"West") };
    REQUIRE_FALSE(westMarkup.empty());
    CHECK_FALSE(westMarkup.Contains(L"SecondGraphTitle"));

    // the canvas is restored after exporting
    CHECK(secondGraph->IsShown());
    CHECK(secondGraph->GetActiveIndex() == 0);
    CHECK(firstGraph->GetActiveIndex() == 0);
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
                "include-all": true,
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
    CHECK(filterableGraph->HasAllOption());
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
