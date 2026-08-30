///////////////////////////////////////////////////////////////////////////////
// Name:        scalechartrendertests.cpp
// Purpose:     Characterization tests for ScaleChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of ScaleChart: scaling- and
// bar-axis ranges and intervals, axis slot counts, bar count, and the number of
// render objects produced. ScaleChart derives from BarChart and is populated
// directly with values (no dataset). The assertions are invariance based; a
// separate exact-value guard compares against a recorded baseline.

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

#include "../../src/graphs/scalechart.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
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
        std::vector<double> m_mainScaleValues{ 10, 20, 30, 40, 50, 60, 70, 80, 90 };
        uint8_t m_precision{ 0 };
        std::vector<double> m_scaleBlockLengths{ 30, 40, 30 };
        std::optional<double> m_scaleStart;
        };

    [[nodiscard]]
    std::shared_ptr<ScaleChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<ScaleChart>(canvas);
        chart->SetMainScaleValues(spec.m_mainScaleValues, spec.m_precision);

        const wxColour blockColor{ ColorBrewer::GetColor(Color::OceanBoatBlue) };
        std::vector<BarChart::BarBlock> blocks;
        blocks.reserve(spec.m_scaleBlockLengths.size());
        for (const auto blockLength : spec.m_scaleBlockLengths)
            {
            blocks.push_back(
                BarChart::BarBlock{ BarChart::BarBlockInfo(blockLength).Brush(blockColor) });
            }
        chart->AddScale(blocks, spec.m_scaleStart, wxString{});
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<ScaleChart>& chart)
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
    double MinValue(const ChartSpec& spec)
        {
        return *std::min_element(spec.m_mainScaleValues.cbegin(), spec.m_mainScaleValues.cend());
        }

    [[nodiscard]]
    double MaxValue(const ChartSpec& spec)
        {
        return *std::max_element(spec.m_mainScaleValues.cbegin(), spec.m_mainScaleValues.cend());
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "ten-to-ninety", ChartSpec{} });
        specs.push_back({ "zero-to-hundred",
                          ChartSpec{ .m_mainScaleValues = { 0, 20, 40, 60, 80, 100 },
                                     .m_scaleBlockLengths = { 50, 50 } } });
        specs.push_back({ "single-block",
                          ChartSpec{ .m_mainScaleValues = { 0, 25, 50, 75, 100 },
                                     .m_scaleBlockLengths = { 100 } } });
        specs.push_back({ "custom-start",
                          ChartSpec{ .m_mainScaleValues = { 20, 40, 60, 80 },
                                     .m_scaleBlockLengths = { 20, 20, 20 },
                                     .m_scaleStart = 20.0 } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to ScaleChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, object count
        if (specName == "ten-to-ninety")
            {
            return LayoutFingerprint{ { 0, 100, 10, 0, false, 11 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 19 };
            }
        if (specName == "zero-to-hundred")
            {
            return LayoutFingerprint{ { 0, 100, 10, 0, false, 11 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 15 };
            }
        if (specName == "single-block")
            {
            return LayoutFingerprint{ { 0, 100, 10, 0, false, 11 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 13 };
            }
        if (specName == "custom-start")
            {
            return LayoutFingerprint{ { 0, 100, 10, 0, false, 11 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 14 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("ScaleChart layout is deterministic and idempotent", "[scalechart][render]")
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

TEST_CASE("ScaleChart layout matches the recorded baseline", "[scalechart][render]")
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

TEST_CASE("ScaleChart scaling axis covers the main scale values", "[scalechart][render]")
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
            CHECK(print.m_scalingAxis.m_rangeStart <= MinValue(spec));
            CHECK(print.m_scalingAxis.m_rangeEnd >= MaxValue(spec));
            CHECK(print.m_scalingAxis.m_interval > 0.0);
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("ScaleChart characterization dump", "[scalechart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/scalechart_characterization.txt" };
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
