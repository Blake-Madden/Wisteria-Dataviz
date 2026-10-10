///////////////////////////////////////////////////////////////////////////////
// Name:        reportbuilderdetachedgraphtests.cpp
// Purpose:     Tests for building a single graph from JSON without placing it
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

#include "../../src/graphs/piechart.h"
#include "../../src/reporting/reportbuilder.h"
#include "graphrenderharness.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace wisteria_render_tests;

TEST_CASE("ReportBuilder::LoadGraphDetached", "[reportbuilder][detached]")
    {
    auto dataset = std::make_shared<Dataset>();
    dataset->AddCategoricalColumn(
        L"group1", ColumnWithStringTable::StringTableType{ { 0, L"Alpha" }, { 1, L"Beta" } });
    const std::vector<GroupIdType> codes{ 0, 0, 1 };
    for (size_t idx = 0; idx < codes.size(); ++idx)
        {
        RowInfo row;
        row.Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
        row.Categoricals({ codes[idx] });
        dataset->AddRow(row);
        }

    ReportBuilder builder;
    builder.GetDatasets()[L"survey"] = dataset;
    auto* canvas = MakeCanvas();

    const wxString pieJson{ L"{\"type\": \"pie-chart\", \"dataset\": \"survey\", "
                            L"\"variables\": { \"group-1\": \"group1\" } }" };

    SECTION("Builds the graph without placing it on the canvas")
        {
        const auto graph = builder.LoadGraphDetached(pieJson, canvas);
        REQUIRE(graph != nullptr);

        const auto* pie = dynamic_cast<const PieChart*>(graph.get());
        REQUIRE(pie != nullptr);
        CHECK(pie->GetGroupColumn1Name() == L"group1");
        CHECK(pie->GetPropertyTemplate(L"dataset") == L"survey");

        const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < gridRows; ++row)
            {
            for (size_t col = 0; col < gridCols; ++col)
                {
                CHECK(canvas->GetFixedObject(row, col) == nullptr);
                }
            }
        }

    SECTION("Legend settings are kept without placing a legend")
        {
        const wxString legendJson{ L"{\"type\": \"pie-chart\", \"dataset\": \"survey\", "
                                   L"\"variables\": { \"group-1\": \"group1\" }, "
                                   L"\"legend\": { \"placement\": \"left\", \"title\": \"Groups\", "
                                   L"\"include-header\": false, \"ring\": \"inner\" } }" };
        const auto graph = builder.LoadGraphDetached(legendJson, canvas);
        REQUIRE(graph != nullptr);

        const auto& legendInfo = graph->GetLegendInfo();
        REQUIRE(legendInfo.has_value());
        CHECK(legendInfo->GetPlacement() == Side::Left);
        CHECK(legendInfo->GetTitle() == L"Groups");
        CHECK_FALSE(legendInfo->IsIncludingHeader());
        CHECK(legendInfo->GetRingPerimeter() == Perimeter::Inner);

        const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < gridRows; ++row)
            {
            for (size_t col = 0; col < gridCols; ++col)
                {
                CHECK(canvas->GetFixedObject(row, col) == nullptr);
                }
            }
        }

    SECTION("A missing dataset throws, and a later load still works")
        {
        const wxString missingJson{ L"{\"type\": \"pie-chart\", \"dataset\": \"nope\", "
                                    L"\"variables\": { \"group-1\": \"group1\" } }" };
        CHECK_THROWS_AS(builder.LoadGraphDetached(missingJson, canvas), std::runtime_error);
        CHECK(builder.LoadGraphDetached(pieJson, canvas) != nullptr);
        }

    SECTION("Unsupported or malformed input returns null")
        {
        CHECK(builder.LoadGraphDetached(L"{\"type\": \"label\"}", canvas) == nullptr);
        CHECK(builder.LoadGraphDetached(L"{}", canvas) == nullptr);
        CHECK(builder.LoadGraphDetached(L"not json", canvas) == nullptr);
        CHECK(builder.LoadGraphDetached(pieJson, nullptr) == nullptr);
        }
    }
