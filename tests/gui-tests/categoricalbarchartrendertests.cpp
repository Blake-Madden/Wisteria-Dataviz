///////////////////////////////////////////////////////////////////////////////
// Name:        categoricalbarchartrendertests.cpp
// Purpose:     Characterization tests for CategoricalBarChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of CategoricalBarChart
// (scaling- and bar-axis ranges and intervals, bar-axis slot count, bar count,
// and the number of render objects produced). CategoricalBarChart
// derives from BarChart and is always horizontal, so GetScalingAxis() is the
// bottom axis and GetBarAxis() is the left axis. The assertions are invariance
// based; a separate exact-value guard compares against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/categoricalbarchart.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out
    // categorical bar chart.
    struct LayoutFingerprint
        {
        AxisFingerprint m_scalingAxis;
        AxisFingerprint m_barAxis;
        size_t m_barCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_scalingAxis == that.m_scalingAxis && m_barAxis == that.m_barAxis &&
                   m_barCount == that.m_barCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "scaling" << m_scalingAxis.ToString() << " | bar" << m_barAxis.ToString()
                 << " | bars=" << m_barCount << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a categorical bar chart under test.
    struct ChartSpec
        {
        ColumnWithStringTable::StringTableType m_catLabels{ { 0, L"Alpha" },
                                                           { 1, L"Beta" },
                                                           { 2, L"Gamma" } };
        std::vector<GroupIdType> m_catCodes{ 0, 0, 0, 1, 1, 2, 2, 2, 2 };
        std::vector<double> m_weights;
        ColumnWithStringTable::StringTableType m_groupLabels;
        std::vector<GroupIdType> m_groupCodes;
        BinLabelDisplay m_binLabelDisplay{ BinLabelDisplay::BinValue };
        std::optional<SortDirection> m_sortDirection;
        bool m_sortable{ true };
        };

    [[nodiscard]]
    std::shared_ptr<CategoricalBarChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"cat", spec.m_catLabels);
        if (!spec.m_weights.empty())
            {
            dataset->AddContinuousColumn(L"weight");
            }
        if (!spec.m_groupCodes.empty())
            {
            dataset->AddCategoricalColumn(L"grp", spec.m_groupLabels);
            }

        for (size_t idx = 0; idx < spec.m_catCodes.size(); ++idx)
            {
            RowInfo row;
            row.Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
            std::vector<GroupIdType> categoricals{ spec.m_catCodes[idx] };
            if (!spec.m_groupCodes.empty())
                {
                categoricals.push_back(spec.m_groupCodes[idx]);
                }
            row.Categoricals(categoricals);
            if (!spec.m_weights.empty())
                {
                row.Continuous({ spec.m_weights[idx] });
                }
            dataset->AddRow(row);
            }

        auto chart = std::make_shared<CategoricalBarChart>(canvas);
        chart->SetData(
            dataset, L"cat",
            spec.m_weights.empty() ? std::nullopt : std::optional<wxString>{ L"weight" },
            spec.m_groupCodes.empty() ? std::nullopt : std::optional<wxString>{ L"grp" },
            spec.m_binLabelDisplay);
        chart->SetSortable(spec.m_sortable);
        if (spec.m_sortDirection.has_value())
            {
            chart->SortBars(BarChart::BarSortComparison::SortByBarLength,
                            spec.m_sortDirection.value());
            }
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<CategoricalBarChart>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_scalingAxis = CaptureAxis(chart->GetScalingAxis());
        print.m_barAxis = CaptureAxis(chart->GetBarAxis());
        print.m_barCount = chart->GetBars().size();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    size_t DistinctCount(const std::vector<GroupIdType>& codes)
        {
        std::vector<GroupIdType> sorted{ codes };
        std::sort(sorted.begin(), sorted.end());
        sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
        return sorted.size();
        }

    [[nodiscard]]
    double LongestBar(const std::shared_ptr<CategoricalBarChart>& chart)
        {
        double longest{ 0 };
        for (const auto& bar : chart->GetBars())
            {
            longest = std::max(longest, bar.GetLength());
            }
        return longest;
        }

    // The matrix of categorical-bar-chart recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "freq-three", ChartSpec{} });
        specs.push_back({ "weighted-three",
                          ChartSpec{ .m_weights = { 5, 5, 5, 10, 10, 2, 2, 2, 2 } } });
        specs.push_back(
            { "grouped-three",
              ChartSpec{ .m_groupLabels = { { 0, L"Left" }, { 1, L"Right" } },
                         .m_groupCodes = { 0, 1, 0, 1, 0, 1, 0, 1, 0 } } });
        specs.push_back({ "single-category",
                          ChartSpec{ .m_catLabels = { { 0, L"Only" } },
                                     .m_catCodes = { 0, 0, 0, 0, 0 } } });
        specs.push_back(
            { "many-categories",
              ChartSpec{ .m_catLabels = { { 0, L"C0" }, { 1, L"C1" }, { 2, L"C2" }, { 3, L"C3" },
                                         { 4, L"C4" }, { 5, L"C5" }, { 6, L"C6" }, { 7, L"C7" } },
                         .m_catCodes = { 0, 1, 1, 2, 2, 2, 3, 4, 4, 5, 5, 5, 5, 6, 7, 7 } } });
        specs.push_back({ "sorted-ascending",
                          ChartSpec{ .m_sortDirection = SortDirection::SortAscending } });
        specs.push_back({ "sorted-descending",
                          ChartSpec{ .m_sortDirection = SortDirection::SortDescending } });
        specs.push_back({ "unsorted", ChartSpec{ .m_sortable = false } });
        return specs;
        }

    // Exact layout output recorded from the code before the bar drawing refactor.
    // Keyed by the spec name from AllSpecs(). Any change here must be a deliberate,
    // reviewed change to CategoricalBarChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, object count
        if (specName == "freq-three" || specName == "sorted-ascending" ||
            specName == "sorted-descending" || specName == "unsorted")
            {
            return LayoutFingerprint{ { 0, 5, 1, 0, false, 6 },
                                      { -1, 3, 1, 0, false, 5 },
                                      3, 10 };
            }
        if (specName == "weighted-three")
            {
            return LayoutFingerprint{ { 0, 25, 5, 0, false, 6 },
                                      { -1, 3, 1, 0, false, 5 },
                                      3, 10 };
            }
        if (specName == "grouped-three")
            {
            return LayoutFingerprint{ { 0, 5, 1, 0, false, 6 },
                                      { -1, 3, 1, 0, false, 5 },
                                      3, 13 };
            }
        if (specName == "single-category")
            {
            return LayoutFingerprint{ { 0, 6, 1, 0, false, 7 },
                                      { -1, 1, 1, 0, false, 3 },
                                      1, 6 };
            }
        if (specName == "many-categories")
            {
            return LayoutFingerprint{ { 0, 5, 1, 0, false, 6 },
                                      { -1, 8, 1, 0, false, 10 },
                                      8, 20 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("CategoricalBarChart layout is deterministic and idempotent",
          "[categoricalbarchart][render]")
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

// Exact-value guard: locks the layout output to what the pre-refactor code
// produced. Complements the invariance checks above. Update ExpectedFingerprint()
// only for a deliberate, reviewed change to CategoricalBarChart layout.
TEST_CASE("CategoricalBarChart layout matches the recorded baseline",
          "[categoricalbarchart][render]")
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

TEST_CASE("CategoricalBarChart builds one bar per unique category",
          "[categoricalbarchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_barCount == DistinctCount(spec.m_catCodes));
            CHECK(print.m_barAxis.m_pointCount >= print.m_barCount);
            }
        }
    }

TEST_CASE("CategoricalBarChart scaling axis encloses the largest category total",
          "[categoricalbarchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK_FALSE(print.m_scalingAxis.m_reversed);
            CHECK(print.m_scalingAxis.m_rangeStart == 0.0);
            CHECK(print.m_scalingAxis.m_interval > 0.0);
            CHECK(print.m_scalingAxis.m_rangeEnd >= LongestBar(chart));
            }
        }
    }

TEST_CASE("CategoricalBarChart emits at least one render object per bar",
          "[categoricalbarchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_objectCount > 0);
            CHECK(print.m_objectCount >= print.m_barCount);
            }
        }
    }

TEST_CASE("CategoricalBarChart sort direction keeps bar count and axis ranges",
          "[categoricalbarchart][render]")
    {
    ChartSpec unsorted;
    unsorted.m_sortable = false;
    ChartSpec ascending;
    ascending.m_sortDirection = SortDirection::SortAscending;
    ChartSpec descending;
    descending.m_sortDirection = SortDirection::SortDescending;

    auto* canvasA = MakeCanvas();
    const auto plainPrint = LayOutAndCapture(canvasA, BuildChart(canvasA, unsorted));
    auto* canvasB = MakeCanvas();
    const auto ascPrint = LayOutAndCapture(canvasB, BuildChart(canvasB, ascending));
    auto* canvasC = MakeCanvas();
    const auto descPrint = LayOutAndCapture(canvasC, BuildChart(canvasC, descending));

    INFO("plain:      " << plainPrint.ToString());
    INFO("ascending:  " << ascPrint.ToString());
    INFO("descending: " << descPrint.ToString());
    CHECK(ascPrint.m_barCount == plainPrint.m_barCount);
    CHECK(descPrint.m_barCount == plainPrint.m_barCount);
    CHECK(ascPrint.m_scalingAxis.m_rangeStart == plainPrint.m_scalingAxis.m_rangeStart);
    CHECK(ascPrint.m_scalingAxis.m_rangeEnd == plainPrint.m_scalingAxis.m_rangeEnd);
    CHECK(descPrint.m_scalingAxis.m_rangeStart == plainPrint.m_scalingAxis.m_rangeStart);
    CHECK(descPrint.m_scalingAxis.m_rangeEnd == plainPrint.m_scalingAxis.m_rangeEnd);
    CHECK(ascPrint.m_barAxis.m_rangeEnd == plainPrint.m_barAxis.m_rangeEnd);
    CHECK(descPrint.m_barAxis.m_rangeEnd == plainPrint.m_barAxis.m_rangeEnd);
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("CategoricalBarChart characterization dump", "[categoricalbarchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/categoricalbarchart_characterization.txt" };
    wxFileOutputStream fileStream{ outPath };
    if (!fileStream.IsOk())
        {
        WARN("could not open " << outPath.ToStdString());
        return;
        }
    wxTextOutputStream textStream{ fileStream };

    for (const auto& [name, spec] : AllSpecs())
        {
        auto* canvas = MakeCanvas();
        auto chart = BuildChart(canvas, spec);
        const auto print = LayOutAndCapture(canvas, chart);
        textStream << wxString::Format(L"%s\t%s\n", wxString::FromUTF8(name),
                                      wxString::FromUTF8(print.ToString()));
        }
    SUCCEED("wrote " << outPath.ToStdString());
    }
