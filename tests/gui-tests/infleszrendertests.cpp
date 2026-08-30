///////////////////////////////////////////////////////////////////////////////
// Name:        infleszrendertests.cpp
// Purpose:     Characterization tests for InfleszScale layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of InfleszScale: scaling-
// and bar-axis ranges and intervals, axis slot counts, bar count, and the
// number of render objects produced. InfleszScale derives from ScaleChart and
// pre-populates the Inflesz scale (optionally alongside the Szigriszt and Flesch
// Reading Ease scales) in its constructor; scores are supplied through a
// dataset. The assertions are invariance based; a separate exact-value guard
// compares against a recorded baseline.

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

#include "../../src/graphs/inflesz.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
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

    struct ChartSpec
        {
        std::vector<double> m_scores{ 45, 58, 66, 72 };
        bool m_includeSzigriszt{ true };
        bool m_includeFlesch{ true };
        };

    [[nodiscard]]
    std::shared_ptr<InfleszScale> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<InfleszScale>(canvas, nullptr, nullptr,
                                                    spec.m_includeSzigriszt, spec.m_includeFlesch);
        chart->SetData(MakeContinuousDataset(L"score", spec.m_scores), L"score");
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<InfleszScale>& chart)
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
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "all-scales", ChartSpec{} });
        specs.push_back({ "inflesz-only",
                          ChartSpec{ .m_scores = { 40, 55, 65 },
                                     .m_includeSzigriszt = false,
                                     .m_includeFlesch = false } });
        specs.push_back({ "single-score", ChartSpec{ .m_scores = { 55 } } });
        specs.push_back(
            { "many-scores", ChartSpec{ .m_scores = { 20, 35, 45, 55, 62, 70, 78, 85 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to InfleszScale layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, object count
        if (specName == "inflesz-only")
            {
            return LayoutFingerprint{ { -5, 105, 10, 0, true, 19 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 39 };
            }
        if (specName == "all-scales" || specName == "single-score" ||
            specName == "many-scores")
            {
            return LayoutFingerprint{ { -5, 105, 10, 0, true, 19 },
                                      { 0.5, 5.5, 1, 1, false, 6 },
                                      5, 69 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("InfleszScale layout is deterministic and idempotent", "[inflesz][render]")
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

TEST_CASE("InfleszScale layout matches the recorded baseline", "[inflesz][render]")
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

TEST_CASE("InfleszScale scaling axis spans the 0-100 readability range", "[inflesz][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            // The Inflesz scale is drawn top-down, so the scaling axis is reversed.
            CHECK(print.m_scalingAxis.m_reversed);
            CHECK(print.m_scalingAxis.m_interval > 0.0);
            CHECK(print.m_scalingAxis.m_rangeStart <= 0.0);
            CHECK(print.m_scalingAxis.m_rangeEnd >= 100.0);
            CHECK_FALSE(print.m_barAxis.m_reversed);
            CHECK(print.m_objectCount > 0);
            }
        }
    }

TEST_CASE("InfleszScale adds comparison scales as extra bars", "[inflesz][render]")
    {
    auto* canvasAll = MakeCanvas();
    auto chartAll = BuildChart(canvasAll, ChartSpec{});
    const auto withComparisons = LayOutAndCapture(canvasAll, chartAll);

    auto* canvasOne = MakeCanvas();
    auto chartOne = BuildChart(
        canvasOne,
        ChartSpec{ .m_includeSzigriszt = false, .m_includeFlesch = false });
    const auto infleszOnly = LayOutAndCapture(canvasOne, chartOne);

    INFO("all: " << withComparisons.ToString());
    INFO("inflesz-only: " << infleszOnly.ToString());
    CHECK(withComparisons.m_barCount > infleszOnly.m_barCount);
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("InfleszScale characterization dump", "[inflesz][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/inflesz_characterization.txt" };
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
