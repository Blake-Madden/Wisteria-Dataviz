///////////////////////////////////////////////////////////////////////////////
// Name:        bulletchartrendertests.cpp
// Purpose:     Characterization tests for BulletChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of BulletChart: row/bar
// counts, range round-tripping, axis-bracket counts, and overall render-object
// counts. BulletChart derives from BarChart, but each row's bar is just an
// invisible placeholder (for slotting and axis auto-fit) -- the visible range
// band is hand-drawn per row as one or more RangeSegment objects (so the outer
// left/right ends can be rounded while internal seams stay sharp), plus a
// per-row actual-value Polygon and a single shared target-tick Lines object,
// all added during RecalcSizes().
//
// Unlike some of the other *rendertests.cpp files, there is no "matches a
// recorded baseline" exact-object-count test here: that pattern requires an
// actual run to capture the true baseline numbers first. Instead, the tests
// below check things directly derivable from the public API (SetRanges()
// round-tripping, bracket counts) and structural/relative invariants
// (determinism, exactly one added object per finite-actual row, exactly N
// extra range-segment objects per extra range) that hold by construction.

#include "../../src/graphs/bulletchart.h"
#include "graphrenderharness.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace wisteria_render_tests;

namespace
    {
    struct KpiRow
        {
        wxString m_label;
        double m_actual{ std::numeric_limits<double>::quiet_NaN() };
        double m_target{ std::numeric_limits<double>::quiet_NaN() };
        };

    struct ChartSpec
        {
        std::vector<KpiRow> m_rows;
        std::vector<BulletChart::Range> m_ranges;
        bool m_showRangeLabels{ true };
        bool m_showValueCallouts{ true };
        };

    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const std::vector<KpiRow>& rows)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddContinuousColumn(L"Actual");
        dataset->AddContinuousColumn(L"Target");
        for (const auto& row : rows)
            {
            dataset->AddRow(RowInfo().Id(row.m_label).Continuous({ row.m_actual, row.m_target }));
            }
        dataset->GetIdColumn().SetName(L"Label");
        return dataset;
        }

    [[nodiscard]]
    std::shared_ptr<BulletChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<BulletChart>(canvas);
        chart->SetData(BuildDataset(spec.m_rows), L"Label", L"Actual", L"Target");
        if (!spec.m_ranges.empty())
            {
            chart->SetRanges(spec.m_ranges);
            }
        chart->ShowRangeLabels(spec.m_showRangeLabels);
        chart->ShowValueCallouts(spec.m_showValueCallouts);
        return chart;
        }

    [[nodiscard]]
    std::vector<BulletChart::Range> ThreeBandRanges()
        {
        return { { 50, L"Poor" }, { 80, L"Satisfactory" }, { 100, L"Good" } };
        }

    [[nodiscard]]
    ChartSpec ThreeSatisfactionRows()
        {
        return ChartSpec{ .m_rows = { { L"Employee Satisfaction", 82, 85 },
                                      { L"Community Satisfaction", 74, 80 },
                                      { L"Customer Satisfaction", 91, 90 } },
                          .m_ranges = ThreeBandRanges() };
        }
    } // namespace

TEST_CASE("BulletChart SetRanges round-trips the given ranges", "[bulletchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = BuildChart(canvas, ThreeSatisfactionRows());

    const auto& ranges = chart->GetRanges();
    REQUIRE(ranges.size() == 3);
    CHECK(ranges[0].m_end == 50);
    CHECK(ranges[0].m_label == L"Poor");
    CHECK(ranges[1].m_end == 80);
    CHECK(ranges[2].m_end == 100);
    }

TEST_CASE("BulletChart draws one range-segment object per range per row", "[bulletchart][render]")
    {
    // range bands are hand-drawn (not real BarBlocks -- see RangeSegment), so their
    // count is only observable indirectly via the total object count. Axis brackets
    // aren't included in GetObjectCount() (they live on the Axis, not the plot's own
    // object list), so only the range segments show up here. Holding everything else
    // fixed (same rows, same actual/target values), going from 1 range to 3 ranges
    // should add exactly 2 extra range-segment objects per row (2 rows here): (3-1)*2 = 4
    auto oneRangeSpec = ThreeSatisfactionRows();
    oneRangeSpec.m_rows.resize(2);
    oneRangeSpec.m_ranges = { { 100, L"OK" } };

    auto threeRangeSpec = ThreeSatisfactionRows();
    threeRangeSpec.m_rows.resize(2);

    auto* canvasOneRange = MakeCanvas();
    auto chartOneRange = BuildChart(canvasOneRange, oneRangeSpec);
    LayOutOffscreen(canvasOneRange, chartOneRange);
    const size_t oneRangeCount{ chartOneRange->GetObjectCount() };

    auto* canvasThreeRanges = MakeCanvas();
    auto chartThreeRanges = BuildChart(canvasThreeRanges, threeRangeSpec);
    LayOutOffscreen(canvasThreeRanges, chartThreeRanges);
    const size_t threeRangeCount{ chartThreeRanges->GetObjectCount() };

    CHECK(threeRangeCount - oneRangeCount == 4);
    }

TEST_CASE("BulletChart adds one axis bracket per range when range labels are shown",
          "[bulletchart][render]")
    {
    auto* canvas = MakeCanvas();

    SECTION("range labels shown")
        {
        auto chart = BuildChart(canvas, ThreeSatisfactionRows());
        CHECK(chart->GetScalingAxis().GetBrackets().size() == 3);
        }

    SECTION("range labels hidden")
        {
        auto spec = ThreeSatisfactionRows();
        spec.m_showRangeLabels = false;
        auto chart = BuildChart(canvas, spec);
        CHECK(chart->GetScalingAxis().GetBrackets().empty());
        }

    SECTION("no ranges set")
        {
        ChartSpec spec{ .m_rows = { { L"New Customers", 219,
                                      std::numeric_limits<double>::quiet_NaN() } } };
        auto chart = BuildChart(canvas, spec);
        CHECK(chart->GetScalingAxis().GetBrackets().empty());
        CHECK(chart->GetRanges().empty());
        REQUIRE(chart->GetBars().size() == 1);
        }
    }

TEST_CASE("BulletChart SetData throws on an unknown column name", "[bulletchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<BulletChart>(canvas);
    auto dataset = BuildDataset(ThreeSatisfactionRows().m_rows);

    CHECK_THROWS_AS(chart->SetData(dataset, L"NotAColumn", L"Actual", L"Target"),
                   std::runtime_error);
    CHECK_THROWS_AS(chart->SetData(dataset, L"Label", L"NotAColumn", L"Target"),
                   std::runtime_error);
    CHECK_THROWS_AS(chart->SetData(dataset, L"Label", L"Actual", L"NotAColumn"),
                   std::runtime_error);
    }

TEST_CASE("BulletChart lays out without throwing when a row is missing its target or actual",
          "[bulletchart][render]")
    {
    ChartSpec spec{ .m_rows = { { L"New Customers", 219, std::numeric_limits<double>::quiet_NaN() },
                                { L"No Actual Yet", std::numeric_limits<double>::quiet_NaN(),
                                  250 } },
                    .m_ranges = { { 150, L"Poor" }, { 225, L"Satisfactory" }, { 300, L"Good" } } };

    auto* canvas = MakeCanvas();
    auto chart = BuildChart(canvas, spec);

    CHECK_NOTHROW(LayOutOffscreen(canvas, chart));
    CHECK(chart->GetObjectCount() > 0);
    }

TEST_CASE("BulletChart layout is deterministic and idempotent", "[bulletchart][render]")
    {
    auto* canvasA = MakeCanvas();
    auto chartA = BuildChart(canvasA, ThreeSatisfactionRows());
    LayOutOffscreen(canvasA, chartA);
    const size_t firstPass{ chartA->GetObjectCount() };
    LayOutOffscreen(canvasA, chartA);
    const size_t secondPass{ chartA->GetObjectCount() };

    auto* canvasB = MakeCanvas();
    auto chartB = BuildChart(canvasB, ThreeSatisfactionRows());
    LayOutOffscreen(canvasB, chartB);
    const size_t freshPass{ chartB->GetObjectCount() };

    CHECK(secondPass == firstPass);
    CHECK(freshPass == firstPass);
    }

TEST_CASE("BulletChart adds one actual-value object per finite-actual row",
          "[bulletchart][render]")
    {
    auto* canvasWithActual = MakeCanvas();
    auto chartWithActual = BuildChart(canvasWithActual, ThreeSatisfactionRows());
    LayOutOffscreen(canvasWithActual, chartWithActual);
    const size_t withActualCount{ chartWithActual->GetObjectCount() };

    auto spec = ThreeSatisfactionRows();
    spec.m_rows[0].m_actual = std::numeric_limits<double>::quiet_NaN();
    auto* canvasMissingActual = MakeCanvas();
    auto chartMissingActual = BuildChart(canvasMissingActual, spec);
    LayOutOffscreen(canvasMissingActual, chartMissingActual);
    const size_t missingActualCount{ chartMissingActual->GetObjectCount() };

    // exactly one fewer render object (the missing actual-value bar) than the
    // otherwise-identical chart
    CHECK(missingActualCount == withActualCount - 1);
    }
