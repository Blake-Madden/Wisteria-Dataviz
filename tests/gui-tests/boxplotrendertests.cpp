///////////////////////////////////////////////////////////////////////////////
// Name:        boxplotrendertests.cpp
// Purpose:     Characterization tests for BoxPlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of BoxPlot (value-axis range
// and interval, category-axis slot count, box count, and the number of render
// objects produced). The assertions are invariance based: layout is
// checked for determinism and idempotence, and against documented structural
// invariants, rather than against hand-picked magic numbers. A separate
// exact-value guard compares against a baseline recorded from the "[.dump]" test.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/boxplot.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out box plot.
    struct LayoutFingerprint
        {
        AxisFingerprint m_valueAxis;
        size_t m_categoryAxisPointCount{ 0 };
        size_t m_boxCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_valueAxis == that.m_valueAxis &&
                   m_categoryAxisPointCount == that.m_categoryAxisPointCount &&
                   m_boxCount == that.m_boxCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "value" << m_valueAxis.ToString() << " | categoryPoints="
                 << m_categoryAxisPointCount << " boxes=" << m_boxCount
                 << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a box plot under test.
    struct ChartSpec
        {
        std::vector<double> m_values{ 10, 11, 12, 12, 13, 13, 13, 14, 14, 15 };
        std::vector<Data::GroupIdType> m_groupCodes;
        Data::ColumnWithStringTable::StringTableType m_groupLabels;
        BoxEffect m_effect{ BoxEffect::Solid };
        BoxCorners m_corners{ BoxCorners::Straight };
        };

    [[nodiscard]]
    std::shared_ptr<BoxPlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset =
            spec.m_groupCodes.empty() ?
                MakeContinuousDataset(L"values", spec.m_values) :
                MakeGroupedContinuousDataset(L"values", spec.m_values, L"group",
                                             spec.m_groupCodes, spec.m_groupLabels);

        auto chart = std::make_shared<BoxPlot>(canvas);
        chart->SetData(dataset, L"values",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" });
        for (size_t idx = 0; idx < chart->GetBoxCount(); ++idx)
            {
            chart->GetBox(idx).SetBoxEffect(spec.m_effect);
            chart->GetBox(idx).SetBoxCorners(spec.m_corners);
            }
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<BoxPlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_valueAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_categoryAxisPointCount = chart->GetBottomXAxis().GetAxisPointsCount();
        print.m_boxCount = chart->GetBoxCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    double MinValue(const ChartSpec& spec)
        {
        return *std::min_element(spec.m_values.cbegin(), spec.m_values.cend());
        }

    [[nodiscard]]
    double MaxValue(const ChartSpec& spec)
        {
        return *std::max_element(spec.m_values.cbegin(), spec.m_values.cend());
        }

    [[nodiscard]]
    size_t DistinctGroupCount(const ChartSpec& spec)
        {
        if (spec.m_groupCodes.empty())
            {
            return 1;
            }
        std::vector<Data::GroupIdType> codes{ spec.m_groupCodes };
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
        return codes.size();
        }

    // The matrix of box-plot recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "single-tight", ChartSpec{} });
        specs.push_back({ "single-wide",
                          ChartSpec{ .m_values = { 2, 20, 45, 60, 61, 62, 90, 130, 175, 200 } } });
        specs.push_back(
            { "single-with-outliers",
              ChartSpec{ .m_values = { 5, 50, 51, 52, 53, 54, 55, 56, 57, 140 } } });
        specs.push_back({ "single-negative",
                          ChartSpec{ .m_values = { -30, -20, -15, -10, -5, 0, 5, 10, 20 } } });
        specs.push_back({ "two-groups",
                          ChartSpec{ .m_values = { 10, 40, 12, 44, 14, 46, 13, 48, 15, 50 },
                                     .m_groupCodes = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
                                     .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back(
            { "three-groups",
              ChartSpec{ .m_values = { 10, 40, 70, 12, 44, 74, 14, 46, 76, 13, 48, 78 },
                         .m_groupCodes = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" }, { 2, L"C" } } } });

        ChartSpec largeN;
        largeN.m_values.clear();
        for (int step = 0; step < 60; ++step)
            {
            largeN.m_values.push_back(20.0 + ((step * 7) % 33));
            }
        specs.push_back({ "large-n", largeN });

        return specs;
        }

    // Exact layout output recorded from the code before the box drawing refactor.
    // Keyed by the spec name from AllSpecs(). Any change here must be a deliberate,
    // reviewed change to BoxPlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: value axis start/end/interval/precision/reversed/points,
        //         category-axis points, box count, object count
        if (specName == "single-tight")
            {
            return LayoutFingerprint{ { 0, 16, 2, 0, false, 9 }, 5, 1, 13 };
            }
        if (specName == "single-wide")
            {
            return LayoutFingerprint{ { 0, 200, 20, 0, false, 11 }, 5, 1, 13 };
            }
        if (specName == "single-with-outliers")
            {
            return LayoutFingerprint{ { 0, 140, 20, 0, false, 8 }, 5, 1, 13 };
            }
        if (specName == "single-negative")
            {
            return LayoutFingerprint{ { -30, 20, 5, 0, false, 11 }, 5, 1, 13 };
            }
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 0, 50, 5, 0, false, 11 }, 4, 2, 20 };
            }
        if (specName == "three-groups")
            {
            return LayoutFingerprint{ { 0, 80, 5, 0, false, 17 }, 5, 3, 28 };
            }
        if (specName == "large-n")
            {
            return LayoutFingerprint{ { 0, 55, 5, 0, false, 12 }, 5, 1, 13 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("BoxPlot layout is deterministic and idempotent", "[boxplot][render]")
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
// only for a deliberate, reviewed change to BoxPlot layout.
TEST_CASE("BoxPlot layout matches the recorded baseline", "[boxplot][render]")
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

TEST_CASE("BoxPlot value axis encloses the data", "[boxplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK_FALSE(print.m_valueAxis.m_reversed);
            CHECK(print.m_valueAxis.m_rangeStart <= MinValue(spec));
            CHECK(print.m_valueAxis.m_rangeEnd >= MaxValue(spec));
            CHECK(print.m_valueAxis.m_interval > 0.0);
            }
        }
    }

TEST_CASE("BoxPlot builds one box per group", "[boxplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_boxCount == DistinctGroupCount(spec));
            CHECK(print.m_categoryAxisPointCount >= print.m_boxCount);
            }
        }
    }

TEST_CASE("BoxPlot emits at least one render object per box", "[boxplot][render]")
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
            CHECK(print.m_objectCount >= print.m_boxCount);
            }
        }
    }

TEST_CASE("BoxPlot lays out every box effect and corner style stably", "[boxplot][render]")
    {
    const std::array<BoxEffect, 8> effects{ BoxEffect::Solid,
                                            BoxEffect::Glassy,
                                            BoxEffect::FadeFromBottomToTop,
                                            BoxEffect::FadeFromTopToBottom,
                                            BoxEffect::WaterColor,
                                            BoxEffect::ThickWaterColor,
                                            BoxEffect::Marker,
                                            BoxEffect::Pencil };
    const std::array<BoxCorners, 2> corners{ BoxCorners::Straight, BoxCorners::Rounded };

    for (const auto effect : effects)
        {
        for (const auto corner : corners)
            {
            ChartSpec spec;
            spec.m_values = { 8, 14, 5, 11, 9, 7, 12, 6, 13, 10 };
            spec.m_effect = effect;
            spec.m_corners = corner;

            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto firstPass = LayOutAndCapture(canvas, chart);
            const auto secondPass = LayOutAndCapture(canvas, chart);

            INFO("effect=" << static_cast<int>(effect) << " corner=" << static_cast<int>(corner)
                           << " fingerprint: " << firstPass.ToString());
            CHECK(secondPass == firstPass);
            CHECK(firstPass.m_objectCount > 0);
            CHECK(firstPass.m_valueAxis.m_rangeEnd >= 14.0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("BoxPlot characterization dump", "[boxplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/boxplot_characterization.txt" };
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
