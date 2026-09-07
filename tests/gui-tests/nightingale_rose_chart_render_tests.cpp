///////////////////////////////////////////////////////////////////////////////
// Name:        nightingale_rose_chart_render_tests.cpp
// Purpose:     Characterization tests for NightingaleRoseChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of NightingaleRoseChart: the
// number of render objects, angular slices, wedges, and series produced.
// NightingaleRoseChart derives from GroupGraph2D and is circular (equal-width
// angular slices around a common center); it exposes no continuous data axes, so
// the fingerprint is counts rather than pixel coordinates. The assertions are
// invariance based; a separate exact-value guard compares against a recorded
// baseline.

#include "../../src/graphs/nightingale_rose_chart.h"
#include "graphrenderharness.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    struct LayoutFingerprint
        {
        size_t m_objectCount{ 0 };
        size_t m_sliceCount{ 0 };
        size_t m_wedgeCount{ 0 };
        size_t m_groupCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_objectCount == that.m_objectCount && m_sliceCount == that.m_sliceCount &&
                   m_wedgeCount == that.m_wedgeCount && m_groupCount == that.m_groupCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "objects=" << m_objectCount << " slices=" << m_sliceCount
                 << " wedges=" << m_wedgeCount << " groups=" << m_groupCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        std::vector<wxString> m_categories;
        std::vector<wxString> m_series; // empty when the chart is ungrouped
        // outer index is the category, inner index is the series (a single
        // element per category when ungrouped)
        std::vector<std::vector<double>> m_values;
        NightingaleRoseChart::RadialScaling m_radialScaling{
            NightingaleRoseChart::RadialScaling::AreaProportional
        };
        NightingaleRoseChart::SeriesDisplay m_seriesDisplay{
            NightingaleRoseChart::SeriesDisplay::Overlaid
        };
        bool m_showLabels{ true };
        };

    [[nodiscard]]
    std::shared_ptr<NightingaleRoseChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        const bool grouped = !spec.m_series.empty();

        auto dataset = std::make_shared<Dataset>();
        ColumnWithStringTable::StringTableType categoryTable;
        for (size_t catIdx = 0; catIdx < spec.m_categories.size(); ++catIdx)
            {
            categoryTable.insert({ static_cast<GroupIdType>(catIdx), spec.m_categories[catIdx] });
            }
        dataset->AddCategoricalColumn(L"category", categoryTable);

        if (grouped)
            {
            ColumnWithStringTable::StringTableType seriesTable;
            for (size_t seriesIdx = 0; seriesIdx < spec.m_series.size(); ++seriesIdx)
                {
                seriesTable.insert(
                    { static_cast<GroupIdType>(seriesIdx), spec.m_series[seriesIdx] });
                }
            dataset->AddCategoricalColumn(L"series", seriesTable);
            }
        dataset->AddContinuousColumn(L"value");

        size_t rowIdx{ 0 };
        for (size_t catIdx = 0; catIdx < spec.m_categories.size(); ++catIdx)
            {
            for (size_t seriesIdx = 0; seriesIdx < spec.m_values[catIdx].size(); ++seriesIdx)
                {
                RowInfo row;
                row.Id(wxString::Format(L"obs%d", static_cast<int>(rowIdx++)));
                row.Categoricals(
                    grouped ? std::vector<GroupIdType>{ static_cast<GroupIdType>(catIdx),
                                                        static_cast<GroupIdType>(seriesIdx) } :
                              std::vector<GroupIdType>{ static_cast<GroupIdType>(catIdx) });
                row.Continuous({ spec.m_values[catIdx][seriesIdx] });
                dataset->AddRow(row);
                }
            }

        auto chart = std::make_shared<NightingaleRoseChart>(canvas);
        chart->SetRadialScaling(spec.m_radialScaling);
        chart->SetSeriesDisplay(spec.m_seriesDisplay);
        chart->ShowLabels(spec.m_showLabels);
        chart->SetData(dataset, L"value", L"category",
                       grouped ? std::optional<wxString>(L"series") : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                       const std::shared_ptr<NightingaleRoseChart>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_objectCount = chart->GetObjectCount();
        print.m_sliceCount = chart->GetSliceCount();
        print.m_wedgeCount = chart->GetWedgeCount();
        print.m_groupCount = chart->GetGroupCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::vector<double>> SingleSeries(const std::vector<double>& values)
        {
        std::vector<std::vector<double>> wrapped;
        wrapped.reserve(values.size());
        for (const auto value : values)
            {
            wrapped.push_back({ value });
            }
        return wrapped;
        }

    [[nodiscard]]
    std::vector<std::vector<double>> ThreeSeriesTwelveMonths()
        {
        // twelve slices, three series each; magnitudes loosely echo the
        // Nightingale mortality shape without reproducing it exactly
        return { { 1, 0, 5 },        { 12, 0, 9 },      { 11, 0, 6 },      { 359, 0, 23 },
                 { 828, 1, 30 },     { 788, 81, 70 },   { 503, 132, 128 }, { 844, 287, 106 },
                 { 1725, 114, 131 }, { 2761, 83, 324 }, { 2120, 42, 361 }, { 1205, 32, 172 } };
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;

        const std::vector<wxString> sixCategories{ L"Alpha", L"Beta",    L"Gamma",
                                                   L"Delta", L"Epsilon", L"Zeta" };
        const std::vector<double> sixValues{ 10, 25, 40, 15, 30, 20 };

        specs.push_back({ "single-series-area",
                          ChartSpec{ .m_categories = sixCategories,
                                     .m_values = SingleSeries(sixValues),
                                     .m_radialScaling =
                                         NightingaleRoseChart::RadialScaling::AreaProportional } });
        specs.push_back(
            { "single-series-radius",
              ChartSpec{ .m_categories = sixCategories,
                         .m_values = SingleSeries(sixValues),
                         .m_radialScaling =
                             NightingaleRoseChart::RadialScaling::RadiusProportional } });

        const std::vector<wxString> twelveMonths{ L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep",
                                                  L"Oct", L"Nov", L"Dec", L"Jan", L"Feb", L"Mar" };
        const std::vector<wxString> threeCauses{ L"Zymotic diseases", L"Wounds & injuries",
                                                 L"All other causes" };

        specs.push_back(
            { "nightingale-overlaid",
              ChartSpec{ .m_categories = twelveMonths,
                         .m_series = threeCauses,
                         .m_values = ThreeSeriesTwelveMonths(),
                         .m_seriesDisplay = NightingaleRoseChart::SeriesDisplay::Overlaid } });
        specs.push_back(
            { "nightingale-stacked",
              ChartSpec{ .m_categories = twelveMonths,
                         .m_series = threeCauses,
                         .m_values = ThreeSeriesTwelveMonths(),
                         .m_seriesDisplay = NightingaleRoseChart::SeriesDisplay::Stacked } });
        specs.push_back(
            { "grouped-radius-stacked",
              ChartSpec{ .m_categories = twelveMonths,
                         .m_series = threeCauses,
                         .m_values = ThreeSeriesTwelveMonths(),
                         .m_radialScaling = NightingaleRoseChart::RadialScaling::RadiusProportional,
                         .m_seriesDisplay = NightingaleRoseChart::SeriesDisplay::Stacked } });
        specs.push_back({ "no-labels", ChartSpec{ .m_categories = twelveMonths,
                                                  .m_series = threeCauses,
                                                  .m_values = ThreeSeriesTwelveMonths(),
                                                  .m_seriesDisplay =
                                                      NightingaleRoseChart::SeriesDisplay::Stacked,
                                                  .m_showLabels = false } });

        specs.push_back({ "two-categories", ChartSpec{ .m_categories = { L"North", L"South" },
                                                       .m_series = { L"X", L"Y" },
                                                       .m_values = { { 3, 7 }, { 5, 2 } } } });

        return specs;
        }

    // Exact layout output recorded per spec name.
    // Update only for a deliberate, reviewed change to NightingaleRoseChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count, slice count, wedge count, group count
        if (specName == "single-series-area")
            {
            return LayoutFingerprint{ 16, 6, 6, 0 };
            }
        if (specName == "single-series-radius")
            {
            return LayoutFingerprint{ 17, 6, 6, 0 };
            }
        if (specName == "nightingale-overlaid" || specName == "nightingale-stacked" ||
            specName == "grouped-radius-stacked")
            {
            return LayoutFingerprint{ 53, 12, 36, 3 };
            }
        if (specName == "no-labels")
            {
            return LayoutFingerprint{ 40, 12, 36, 3 };
            }
        if (specName == "two-categories")
            {
            return LayoutFingerprint{ 10, 2, 4, 2 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("NightingaleRoseChart layout is deterministic and idempotent",
          "[nightingalerosechart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvasA = MakeCanvas();
            auto chartA = BuildChart(canvasA, spec);
            const auto firstPass = LayOutAndCapture(canvasA, chartA);
            const auto secondPass = LayOutAndCapture(canvasA, chartA);

            auto* canvasB = MakeCanvas();
            auto chartB = BuildChart(canvasB, spec);
            const auto freshPass = LayOutAndCapture(canvasB, chartB);

            INFO("fingerprint: " << firstPass.ToString());
            CHECK(secondPass == firstPass);
            CHECK(freshPass == firstPass);
            }
        }
    }

TEST_CASE("NightingaleRoseChart layout matches the recorded baseline",
          "[nightingalerosechart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto actual = LayOutAndCapture(canvas, chart);
            const auto expected = ExpectedFingerprint(name);

            INFO("expected: " << expected.ToString());
            INFO("actual:   " << actual.ToString());
            CHECK(actual == expected);
            }
        }
    }

TEST_CASE("NightingaleRoseChart layout invariants", "[nightingalerosechart][render]")
    {
    const auto specs = AllSpecs();
    const auto findSpec = [&specs](const std::string& name) -> const ChartSpec&
    {
        for (const auto& [specName, spec] : specs)
            {
            if (specName == name)
                {
                return spec;
                }
            }
        FAIL("spec not found: " << name);
        static const ChartSpec empty;
        return empty;
    };

    for (const auto& [name, spec] : specs)
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_objectCount > 0);
            CHECK(print.m_sliceCount == spec.m_categories.size());

            const size_t seriesPerSlice = spec.m_series.empty() ? 1 : spec.m_series.size();
            CHECK(print.m_wedgeCount == spec.m_categories.size() * seriesPerSlice);
            }
        }

    SECTION("hiding labels reduces the object count for the same data")
        {
        auto* canvasWithLabels = MakeCanvas();
        auto chartWithLabels = BuildChart(canvasWithLabels, findSpec("nightingale-stacked"));
        const auto withLabels = LayOutAndCapture(canvasWithLabels, chartWithLabels);

        auto* canvasNoLabels = MakeCanvas();
        auto chartNoLabels = BuildChart(canvasNoLabels, findSpec("no-labels"));
        const auto withoutLabels = LayOutAndCapture(canvasNoLabels, chartNoLabels);

        CHECK(withoutLabels.m_objectCount < withLabels.m_objectCount);
        CHECK(withoutLabels.m_sliceCount == withLabels.m_sliceCount);
        CHECK(withoutLabels.m_wedgeCount == withLabels.m_wedgeCount);
        }

    SECTION("area and radius scaling produce the same slice and wedge structure")
        {
        auto* canvasArea = MakeCanvas();
        auto chartArea = BuildChart(canvasArea, findSpec("single-series-area"));
        const auto areaPrint = LayOutAndCapture(canvasArea, chartArea);

        auto* canvasRadius = MakeCanvas();
        auto chartRadius = BuildChart(canvasRadius, findSpec("single-series-radius"));
        const auto radiusPrint = LayOutAndCapture(canvasRadius, chartRadius);

        CHECK(areaPrint.m_sliceCount == radiusPrint.m_sliceCount);
        CHECK(areaPrint.m_wedgeCount == radiusPrint.m_wedgeCount);
        CHECK(areaPrint.m_groupCount == radiusPrint.m_groupCount);
        }

    SECTION("overlaid and stacked series produce the same fingerprint for the same shape")
        {
        auto* canvasOverlaid = MakeCanvas();
        auto chartOverlaid = BuildChart(canvasOverlaid, findSpec("nightingale-overlaid"));
        const auto overlaidPrint = LayOutAndCapture(canvasOverlaid, chartOverlaid);

        auto* canvasStacked = MakeCanvas();
        auto chartStacked = BuildChart(canvasStacked, findSpec("nightingale-stacked"));
        const auto stackedPrint = LayOutAndCapture(canvasStacked, chartStacked);

        CHECK(overlaidPrint == stackedPrint);
        }
    }
