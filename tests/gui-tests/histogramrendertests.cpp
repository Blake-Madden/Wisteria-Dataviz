///////////////////////////////////////////////////////////////////////////////
// Name:        histogramrendertests.cpp
// Purpose:     Characterization tests for Histogram layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of Histogram (scaling- and
// bar-axis ranges and intervals, bar-axis slot count, bar count, populated-bin
// count, and the number of render objects produced). Histogram derives
// from BarChart, so the same GetScalingAxis()/GetBarAxis()/GetBars() surface
// used by barrendertests.cpp applies here. The assertions are invariance based;
// a separate exact-value guard compares against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/histogram.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out histogram.
    struct LayoutFingerprint
        {
        AxisFingerprint m_scalingAxis;
        AxisFingerprint m_barAxis;
        size_t m_barCount{ 0 };
        size_t m_binsWithValues{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_scalingAxis == that.m_scalingAxis && m_barAxis == that.m_barAxis &&
                   m_barCount == that.m_barCount && m_binsWithValues == that.m_binsWithValues &&
                   m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "scaling" << m_scalingAxis.ToString() << " | bar" << m_barAxis.ToString()
                 << " | bars=" << m_barCount << " binsWithValues=" << m_binsWithValues
                 << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a histogram under test.
    struct ChartSpec
        {
        std::vector<double> m_values{ 1, 2, 2, 3, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10 };
        Histogram::BinningMethod m_binningMethod{ Histogram::BinningMethod::BinByIntegerRange };
        Histogram::IntervalDisplay m_intervalDisplay{ Histogram::IntervalDisplay::Cutpoints };
        bool m_showFullRangeOfValues{ true };
        std::optional<double> m_startBinsValue;
        std::optional<size_t> m_suggestedBins;
        };

    [[nodiscard]]
    std::shared_ptr<Histogram> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = MakeContinuousDataset(L"values", spec.m_values);

        auto chart = std::make_shared<Histogram>(canvas);
        chart->SetData(dataset, L"values", std::nullopt, spec.m_binningMethod,
                       RoundingMethod::NoRounding, spec.m_intervalDisplay,
                       BinLabelDisplay::BinValue, spec.m_showFullRangeOfValues,
                       spec.m_startBinsValue,
                       std::make_pair(spec.m_suggestedBins, std::optional<size_t>{ std::nullopt }));
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<Histogram>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_scalingAxis = CaptureAxis(chart->GetScalingAxis());
        print.m_barAxis = CaptureAxis(chart->GetBarAxis());
        print.m_barCount = chart->GetBars().size();
        print.m_binsWithValues = chart->GetBinsWithValuesCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    double TotalBarLength(const std::shared_ptr<Histogram>& chart)
        {
        double total{ 0 };
        for (const auto& bar : chart->GetBars())
            {
            total += bar.GetLength();
            }
        return total;
        }

    [[nodiscard]]
    double LongestBar(const std::shared_ptr<Histogram>& chart)
        {
        double longest{ 0 };
        for (const auto& bar : chart->GetBars())
            {
            longest = std::max(longest, bar.GetLength());
            }
        return longest;
        }

    // The matrix of histogram recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "integer-range-default", ChartSpec{} });
        specs.push_back({ "unique-values",
                          ChartSpec{ .m_values = { 1, 1, 1, 2, 2, 3, 3, 3, 3, 4, 5, 5 },
                                     .m_binningMethod =
                                         Histogram::BinningMethod::BinUniqueValues } });
        specs.push_back(
            { "wide-spread",
              ChartSpec{ .m_values = { 1, 5, 12, 33, 47, 68, 91, 140, 200, 260 } } });
        specs.push_back(
            { "explicit-bin-count",
              ChartSpec{ .m_values = { 1, 5, 12, 33, 47, 68, 91, 140, 200, 260 },
                         .m_suggestedBins = size_t{ 4 } } });
        specs.push_back({ "no-full-range",
                          ChartSpec{ .m_values = { 1, 2, 3, 10, 11, 12 },
                                     .m_binningMethod = Histogram::BinningMethod::BinUniqueValues,
                                     .m_showFullRangeOfValues = false } });
        specs.push_back({ "start-bins-value",
                          ChartSpec{ .m_values = { 5, 6, 7, 8, 9, 10 },
                                     .m_startBinsValue = 0.0 } });
        return specs;
        }

    // Exact layout output recorded from the code before the bar drawing refactor.
    // Keyed by the spec name from AllSpecs(). Any change here must be a deliberate,
    // reviewed change to Histogram layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, populated-bin count, object count
        if (specName == "integer-range-default")
            {
            return LayoutFingerprint{ { 0, 7, 1, 0, false, 8 },
                                      { -1, 13, 2, 0, false, 8 },
                                      5, 5, 14 };
            }
        if (specName == "unique-values")
            {
            return LayoutFingerprint{ { 0, 5, 1, 0, false, 6 },
                                      { 0, 6, 1, 0, false, 7 },
                                      5, 5, 14 };
            }
        if (specName == "wide-spread")
            {
            return LayoutFingerprint{ { 0, 6, 1, 0, false, 7 },
                                      { -51, 313, 52, 0, false, 8 },
                                      5, 5, 14 };
            }
        if (specName == "explicit-bin-count")
            {
            return LayoutFingerprint{ { 0, 6, 1, 0, false, 7 },
                                      { -64, 326, 65, 0, false, 7 },
                                      4, 4, 12 };
            }
        if (specName == "no-full-range")
            {
            return LayoutFingerprint{ { 0, 2, 1, 1, false, 3 },
                                      { 0, 7, 1, 0, false, 8 },
                                      6, 6, 16 };
            }
        if (specName == "start-bins-value")
            {
            return LayoutFingerprint{ { 0, 4, 1, 0, false, 5 },
                                      { -3, 15, 3, 0, false, 7 },
                                      4, 4, 11 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("Histogram layout is deterministic and idempotent", "[histogram][render]")
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
// only for a deliberate, reviewed change to Histogram layout.
TEST_CASE("Histogram layout matches the recorded baseline", "[histogram][render]")
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

TEST_CASE("Histogram scaling axis starts at zero and encloses the tallest bin",
          "[histogram][render]")
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

TEST_CASE("Histogram bin counts sum to the observation count", "[histogram][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            (void)LayOutAndCapture(canvas, chart);

            INFO("spec: " << name);
            CHECK(TotalBarLength(chart) == static_cast<double>(spec.m_values.size()));
            }
        }
    }

TEST_CASE("Histogram populated-bin count never exceeds the bar count", "[histogram][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_binsWithValues <= print.m_barCount);
            CHECK(print.m_binsWithValues > 0);
            }
        }
    }

TEST_CASE("Histogram emits at least one render object per bar", "[histogram][render]")
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
            CHECK(print.m_barAxis.m_pointCount >= print.m_barCount);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("Histogram characterization dump", "[histogram][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/histogram_characterization.txt" };
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
