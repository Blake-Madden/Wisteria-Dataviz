///////////////////////////////////////////////////////////////////////////////
// Name:        serpentinebarrendertests.cpp
// Purpose:     Characterization tests for BarChart serpentine (folded bar) layout
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable output of serpentine folding: the scaling-
// and bar-axis fingerprints, the bar count, the render-object count, the extra
// blank-row count, and which bars report as folded. The assertions are invariance
// based: layout is checked for determinism and idempotence, and against the
// documented folding rules (threshold, fold cap, three-bar and two-length
// preconditions, reversibility).

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

#include "../../src/graphs/barchart.h"
#include "../../src/graphs/categoricalbarchart.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out
    // serpentine bar chart.
    struct LayoutFingerprint
        {
        AxisFingerprint m_scalingAxis;
        AxisFingerprint m_barAxis;
        size_t m_barCount{ 0 };
        size_t m_objectCount{ 0 };
        size_t m_extraRowCount{ 0 };
        size_t m_foldedBarCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_scalingAxis == that.m_scalingAxis && m_barAxis == that.m_barAxis &&
                   m_barCount == that.m_barCount && m_objectCount == that.m_objectCount &&
                   m_extraRowCount == that.m_extraRowCount &&
                   m_foldedBarCount == that.m_foldedBarCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "scaling" << m_scalingAxis.ToString() << " | bar" << m_barAxis.ToString()
                 << " | bars=" << m_barCount << " objects=" << m_objectCount
                 << " extraRows=" << m_extraRowCount << " folded=" << m_foldedBarCount;
            return text.str();
            }
        };

    // A minimal recipe for a serpentine bar chart under test.
    struct SerpSpec
        {
        Orientation m_orientation{ Orientation::Vertical };
        std::vector<double> m_barLengths{ 5, 6, 4, 30 };
        BarChart::SerpentineMode m_mode{ BarChart::SerpentineMode::Serpentine };
        double m_threshold{ 3.0 };
        bool m_reverseScalingAxis{ false };
        };

    [[nodiscard]]
    std::shared_ptr<BarChart> BuildBarChart(Canvas* canvas, const SerpSpec& spec)
        {
        auto chart = std::make_shared<BarChart>(canvas);
        chart->SetBarOrientation(spec.m_orientation);

        const wxColour barColor{ ColorBrewer::GetColor(Color::OceanBoatBlue) };
        double axisPosition{ 1 };
        for (const auto barLength : spec.m_barLengths)
            {
            BarChart::Bar theBar(
                axisPosition,
                { BarChart::BarBlock{ BarChart::BarBlockInfo(barLength).Brush(barColor) } },
                wxString{}, Label(wxString::Format(L"Bar %d", static_cast<int>(axisPosition))),
                BoxEffect::Solid);
            chart->AddBar(theBar);
            axisPosition += 1;
            }

        // reverse after the bars are added, since AddBar() sets the axis ranges
        if (spec.m_reverseScalingAxis)
            {
            chart->GetScalingAxis().Reverse(true);
            }

        chart->SetSerpentineThreshold(spec.m_threshold);
        chart->SetSerpentineMode(spec.m_mode);
        return chart;
        }

    [[nodiscard]]
    size_t CountFoldedBars(const std::shared_ptr<BarChart>& chart)
        {
        size_t folded{ 0 };
        for (size_t i = 0; i < chart->GetBars().size(); ++i)
            {
            if (chart->IsBarSerpentine(i))
                {
                ++folded;
                }
            }
        return folded;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<BarChart>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_scalingAxis = CaptureAxis(chart->GetScalingAxis());
        print.m_barAxis = CaptureAxis(chart->GetBarAxis());
        print.m_barCount = chart->GetBars().size();
        print.m_objectCount = chart->GetObjectCount();
        print.m_extraRowCount = chart->GetSerpentineExtraRowCount();
        print.m_foldedBarCount = CountFoldedBars(chart);
        return print;
        }

    [[nodiscard]]
    double LongestBar(const std::shared_ptr<BarChart>& chart)
        {
        double longest{ 0 };
        for (const auto& bar : chart->GetBars())
            {
            longest = std::max(longest, bar.GetLength());
            }
        return longest;
        }

    // The matrix of serpentine recipes exercised by the determinism tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, SerpSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, SerpSpec>> specs;
        specs.push_back({ "vert-none", SerpSpec{ .m_mode = BarChart::SerpentineMode::None } });
        specs.push_back({ "vert-serpentine-folds",
                          SerpSpec{ .m_mode = BarChart::SerpentineMode::Serpentine } });
        specs.push_back({ "horz-serpentine-folds",
                          SerpSpec{ .m_orientation = Orientation::Horizontal,
                                    .m_mode = BarChart::SerpentineMode::Serpentine } });
        specs.push_back({ "vert-aggressive-folds",
                          SerpSpec{ .m_mode = BarChart::SerpentineMode::AggressiveSerpentine } });
        specs.push_back({ "horz-aggressive-folds",
                          SerpSpec{ .m_orientation = Orientation::Horizontal,
                                    .m_mode = BarChart::SerpentineMode::AggressiveSerpentine } });
        // a length and neighbor that let aggressive folding turn the last run into
        // a shorter bar's trailing space
        specs.push_back({ "vert-aggressive-eat",
                          SerpSpec{ .m_barLengths = { 4, 61, 1, 20 },
                                    .m_mode = BarChart::SerpentineMode::AggressiveSerpentine } });
        // 14 is only 2.3x the reference bar, below the default threshold of 3
        specs.push_back({ "vert-serpentine-no-fold",
                          SerpSpec{ .m_barLengths = { 5, 6, 4, 14 },
                                    .m_mode = BarChart::SerpentineMode::Serpentine } });
        // 300 is 50x the reference bar, past the 40-fold cap
        specs.push_back({ "vert-serpentine-cap",
                          SerpSpec{ .m_barLengths = { 5, 6, 4, 300 },
                                    .m_mode = BarChart::SerpentineMode::Serpentine } });
        return specs;
        }
    } // namespace

TEST_CASE("BarChart serpentine mode setter round-trips", "[barchart][serpentine]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<BarChart>(canvas);

    CHECK(chart->GetSerpentineMode() == BarChart::SerpentineMode::None);
    chart->SetSerpentineMode(BarChart::SerpentineMode::Serpentine);
    CHECK(chart->GetSerpentineMode() == BarChart::SerpentineMode::Serpentine);
    chart->SetSerpentineMode(BarChart::SerpentineMode::AggressiveSerpentine);
    CHECK(chart->GetSerpentineMode() == BarChart::SerpentineMode::AggressiveSerpentine);
    chart->SetSerpentineMode(BarChart::SerpentineMode::None);
    CHECK(chart->GetSerpentineMode() == BarChart::SerpentineMode::None);
    }

TEST_CASE("BarChart serpentine threshold is clamped to two through forty", "[barchart][serpentine]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<BarChart>(canvas);

    CHECK(chart->GetSerpentineThreshold() == 3.0);
    chart->SetSerpentineThreshold(1.0);
    CHECK(chart->GetSerpentineThreshold() == 2.0);
    chart->SetSerpentineThreshold(-5.0);
    CHECK(chart->GetSerpentineThreshold() == 2.0);
    chart->SetSerpentineThreshold(100.0);
    CHECK(chart->GetSerpentineThreshold() == 40.0);
    chart->SetSerpentineThreshold(7.5);
    CHECK(chart->GetSerpentineThreshold() == 7.5);
    }

TEST_CASE("BarChart serpentine queries are neutral before the first layout",
          "[barchart][serpentine]")
    {
    auto* canvas = MakeCanvas();
    auto chart = BuildBarChart(canvas, SerpSpec{});

    CHECK(chart->GetSerpentineExtraRowCount() == 0);
    for (size_t i = 0; i < chart->GetBars().size(); ++i)
        {
        CHECK_FALSE(chart->IsBarSerpentine(i));
        CHECK(chart->GetSerpentineFoldCount(i) == 0);
        }
    }

TEST_CASE("BarChart serpentine layout is deterministic and idempotent", "[barchart][serpentine]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvasA = MakeCanvas();
            auto chartA = BuildBarChart(canvasA, spec);
            const auto firstPass = LayOutAndCapture(canvasA, chartA);
            const auto secondPass = LayOutAndCapture(canvasA, chartA);

            auto* canvasB = MakeCanvas();
            auto chartB = BuildBarChart(canvasB, spec);
            const auto freshPass = LayOutAndCapture(canvasB, chartB);

            INFO("fingerprint: " << firstPass.ToString());
            CHECK(secondPass == firstPass);
            CHECK(freshPass == firstPass);
            }
        }
    }

TEST_CASE("BarChart folds a bar that dwarfs the others", "[barchart][serpentine]")
    {
    for (const auto orientation : { Orientation::Vertical, Orientation::Horizontal })
        {
        SECTION(orientation == Orientation::Vertical ? "vertical" : "horizontal")
            {
            SerpSpec spec;
            spec.m_orientation = orientation;
            spec.m_barLengths = { 5, 6, 4, 30 };
            spec.m_mode = BarChart::SerpentineMode::Serpentine;

            auto* canvas = MakeCanvas();
            auto chart = BuildBarChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(chart->IsBarSerpentine(3));
            CHECK(chart->GetSerpentineFoldCount(3) >= 2);
            CHECK(print.m_foldedBarCount == 1);
            CHECK(print.m_extraRowCount > 0);
            // the value axis spans a single fold, not the whole giant bar
            CHECK(print.m_scalingAxis.m_rangeEnd < 30.0);
            }
        }
    }

TEST_CASE("BarChart serpentine keeps the scaling axis short", "[barchart][serpentine]")
    {
    SerpSpec plain;
    plain.m_mode = BarChart::SerpentineMode::None;
    auto* canvasPlain = MakeCanvas();
    const auto plainPrint = LayOutAndCapture(canvasPlain, BuildBarChart(canvasPlain, plain));

    SerpSpec folded;
    folded.m_mode = BarChart::SerpentineMode::Serpentine;
    auto* canvasFold = MakeCanvas();
    const auto foldPrint = LayOutAndCapture(canvasFold, BuildBarChart(canvasFold, folded));

    INFO("plain:  " << plainPrint.ToString());
    INFO("folded: " << foldPrint.ToString());
    CHECK(plainPrint.m_foldedBarCount == 0);
    CHECK(plainPrint.m_extraRowCount == 0);
    CHECK(foldPrint.m_foldedBarCount == 1);
    CHECK(foldPrint.m_scalingAxis.m_rangeEnd < plainPrint.m_scalingAxis.m_rangeEnd);
    // the folds need extra rows on the bar axis
    CHECK(foldPrint.m_barAxis.m_pointCount > plainPrint.m_barAxis.m_pointCount);
    }

TEST_CASE("BarChart serpentine threshold gates folding", "[barchart][serpentine]")
    {
    SerpSpec highThreshold;
    highThreshold.m_barLengths = { 5, 6, 4, 14 };
    highThreshold.m_threshold = 3.0;
    highThreshold.m_mode = BarChart::SerpentineMode::Serpentine;
    auto* canvasHigh = MakeCanvas();
    const auto highPrint = LayOutAndCapture(canvasHigh, BuildBarChart(canvasHigh, highThreshold));

    SerpSpec lowThreshold{ highThreshold };
    lowThreshold.m_threshold = 2.0;
    auto* canvasLow = MakeCanvas();
    const auto lowPrint = LayOutAndCapture(canvasLow, BuildBarChart(canvasLow, lowThreshold));

    INFO("threshold 3: " << highPrint.ToString());
    INFO("threshold 2: " << lowPrint.ToString());
    CHECK(highPrint.m_foldedBarCount == 0);
    CHECK(highPrint.m_extraRowCount == 0);
    CHECK(lowPrint.m_foldedBarCount == 1);
    CHECK(lowPrint.m_extraRowCount > 0);
    }

TEST_CASE("BarChart serpentine leaves an un-foldably long bar alone", "[barchart][serpentine]")
    {
    SerpSpec spec;
    spec.m_barLengths = { 5, 6, 4, 300 };
    spec.m_threshold = 3.0;
    spec.m_mode = BarChart::SerpentineMode::Serpentine;

    auto* canvas = MakeCanvas();
    auto chart = BuildBarChart(canvas, spec);
    const auto print = LayOutAndCapture(canvas, chart);

    INFO("fingerprint: " << print.ToString());
    CHECK(print.m_foldedBarCount == 0);
    CHECK(print.m_extraRowCount == 0);
    // nothing folded, so the axis still has to span the giant bar
    CHECK(print.m_scalingAxis.m_rangeEnd >= 300.0);
    }

TEST_CASE("BarChart serpentine needs three bars, two lengths, and a forward axis",
          "[barchart][serpentine]")
    {
    SECTION("fewer than three bars")
        {
        SerpSpec spec;
        spec.m_barLengths = { 6, 30 };
        spec.m_mode = BarChart::SerpentineMode::Serpentine;
        auto* canvas = MakeCanvas();
        const auto print = LayOutAndCapture(canvas, BuildBarChart(canvas, spec));
        CHECK(print.m_foldedBarCount == 0);
        CHECK(print.m_extraRowCount == 0);
        }
    SECTION("only one distinct bar length")
        {
        SerpSpec spec;
        spec.m_barLengths = { 10, 10, 10, 10 };
        spec.m_mode = BarChart::SerpentineMode::Serpentine;
        auto* canvas = MakeCanvas();
        const auto print = LayOutAndCapture(canvas, BuildBarChart(canvas, spec));
        CHECK(print.m_foldedBarCount == 0);
        CHECK(print.m_extraRowCount == 0);
        }
    SECTION("reversed scaling axis")
        {
        SerpSpec spec;
        spec.m_reverseScalingAxis = true;
        spec.m_mode = BarChart::SerpentineMode::Serpentine;
        auto* canvas = MakeCanvas();
        const auto print = LayOutAndCapture(canvas, BuildBarChart(canvas, spec));
        CHECK(print.m_scalingAxis.m_reversed);
        CHECK(print.m_foldedBarCount == 0);
        CHECK(print.m_extraRowCount == 0);
        }
    }

TEST_CASE("BarChart clearing serpentine mode restores the layout", "[barchart][serpentine]")
    {
    SerpSpec folding;
    folding.m_mode = BarChart::SerpentineMode::Serpentine;
    auto* canvas = MakeCanvas();
    auto chart = BuildBarChart(canvas, folding);

    const auto foldedPrint = LayOutAndCapture(canvas, chart);
    REQUIRE(foldedPrint.m_foldedBarCount == 1);

    chart->SetSerpentineMode(BarChart::SerpentineMode::None);
    const auto clearedPrint = LayOutAndCapture(canvas, chart);

    // a chart with the same bars that was never folded
    SerpSpec neverFolded{ folding };
    neverFolded.m_mode = BarChart::SerpentineMode::None;
    auto* freshCanvas = MakeCanvas();
    const auto neverFoldedPrint =
        LayOutAndCapture(freshCanvas, BuildBarChart(freshCanvas, neverFolded));

    INFO("folded:       " << foldedPrint.ToString());
    INFO("cleared:      " << clearedPrint.ToString());
    INFO("never folded: " << neverFoldedPrint.ToString());
    CHECK(clearedPrint.m_foldedBarCount == 0);
    CHECK(clearedPrint.m_extraRowCount == 0);
    // the axis was shrunk to one fold while folded; it must grow back to span the long bar
    CHECK(clearedPrint.m_scalingAxis.m_rangeEnd > foldedPrint.m_scalingAxis.m_rangeEnd);
    CHECK(clearedPrint.m_scalingAxis.m_rangeEnd >= LongestBar(chart));
    // both axes and the object count return all the way to the never-folded layout
    CHECK(clearedPrint == neverFoldedPrint);
    }

TEST_CASE("BarChart raising the serpentine threshold un-folds a bar", "[barchart][serpentine]")
    {
    SerpSpec spec;
    spec.m_barLengths = { 5, 6, 4, 14 };
    spec.m_threshold = 2.0;
    spec.m_mode = BarChart::SerpentineMode::Serpentine;
    auto* canvas = MakeCanvas();
    auto chart = BuildBarChart(canvas, spec);

    const auto foldedPrint = LayOutAndCapture(canvas, chart);
    REQUIRE(foldedPrint.m_foldedBarCount == 1);

    chart->SetSerpentineThreshold(10.0);
    const auto print = LayOutAndCapture(canvas, chart);

    INFO("folded:   " << foldedPrint.ToString());
    INFO("unfolded: " << print.ToString());
    CHECK(print.m_foldedBarCount == 0);
    CHECK(print.m_extraRowCount == 0);
    }

TEST_CASE("BarChart aggressive serpentine lays out and folds", "[barchart][serpentine]")
    {
    SerpSpec spec;
    spec.m_barLengths = { 4, 61, 1, 20 };
    spec.m_mode = BarChart::SerpentineMode::AggressiveSerpentine;
    auto* canvas = MakeCanvas();
    auto chart = BuildBarChart(canvas, spec);
    const auto print = LayOutAndCapture(canvas, chart);

    INFO("fingerprint: " << print.ToString());
    CHECK(chart->IsBarSerpentine(1));
    CHECK(chart->GetSerpentineFoldCount(1) >= 2);
    CHECK(print.m_foldedBarCount == 1);
    CHECK(print.m_scalingAxis.m_rangeEnd < 61.0);
    }

TEST_CASE("CategoricalBarChart supports serpentine folding", "[categoricalbarchart][serpentine]")
    {
    std::vector<GroupIdType> codes;
    codes.insert(codes.end(), 24, 0);
    codes.insert(codes.end(), 6, 1);
    codes.insert(codes.end(), 5, 2);
    codes.insert(codes.end(), 4, 3);
    auto dataset = MakeCategoricalDataset(
        L"cat", { { 0, L"A" }, { 1, L"B" }, { 2, L"C" }, { 3, L"D" } }, codes);

    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<CategoricalBarChart>(canvas);
    chart->SetData(dataset, L"cat", std::nullopt, std::nullopt, BinLabelDisplay::BinValue);
    chart->SetSerpentineMode(BarChart::SerpentineMode::Serpentine);

    LayOutOffscreen(canvas, chart);

    size_t foldedBars{ 0 };
    for (size_t i = 0; i < chart->GetBars().size(); ++i)
        {
        if (chart->IsBarSerpentine(i))
            {
            ++foldedBars;
            }
        }

    INFO("bars=" << chart->GetBars().size()
                 << " extraRows=" << chart->GetSerpentineExtraRowCount());
    CHECK(chart->GetBars().size() == 4);
    CHECK(foldedBars == 1);
    CHECK(chart->GetSerpentineExtraRowCount() > 0);
    CHECK(chart->GetScalingAxis().GetRange().second < 24.0);

    // a second pass converges on the same layout
    const auto firstExtra = chart->GetSerpentineExtraRowCount();
    LayOutOffscreen(canvas, chart);
    CHECK(chart->GetSerpentineExtraRowCount() == firstExtra);
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the output can be diffed by hand as
// an extra safety check.
TEST_CASE("BarChart serpentine characterization dump", "[barchart][serpentine][dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/serpentinebar_characterization.txt" };
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
        auto chart = BuildBarChart(canvas, spec);
        const auto print = LayOutAndCapture(canvas, chart);
        textStream << wxString::Format(L"%s\t%s\n", wxString::FromUTF8(name),
                                      wxString::FromUTF8(print.ToString()));
        }
    SUCCEED("wrote " << outPath.ToStdString());
    }
