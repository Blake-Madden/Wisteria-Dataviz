///////////////////////////////////////////////////////////////////////////////
// Name:        lineplotrendertests.cpp
// Purpose:     Characterization tests for LinePlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of LinePlot: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, line count, and the
// number of render objects produced. The assertions are invariance based; a
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

#include "../../src/graphs/lineplot.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out line plot.
    struct LayoutFingerprint
        {
        AxisFingerprint m_leftYAxis;
        AxisFingerprint m_bottomXAxis;
        size_t m_lineCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_leftYAxis == that.m_leftYAxis && m_bottomXAxis == that.m_bottomXAxis &&
                   m_lineCount == that.m_lineCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "leftY" << m_leftYAxis.ToString() << " | bottomX" << m_bottomXAxis.ToString()
                 << " | lines=" << m_lineCount << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a line plot under test.
    struct ChartSpec
        {
        std::vector<double> m_yValues{ 3, 8, 5, 11, 7, 14, 9, 6 };
        std::vector<double> m_xValues{ 1, 2, 3, 4, 5, 6, 7, 8 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    [[nodiscard]]
    std::shared_ptr<LinePlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset =
            spec.m_groupCodes.empty() ?
                MakeXYDataset(L"y", spec.m_yValues, L"x", spec.m_xValues) :
                MakeGroupedXYDataset(L"y", spec.m_yValues, L"x", spec.m_xValues, L"group",
                                     spec.m_groupCodes, spec.m_groupLabels);

        auto chart = std::make_shared<LinePlot>(canvas);
        chart->SetData(dataset, L"y", L"x",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<LinePlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_lineCount = chart->GetLineCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    double MinOf(const std::vector<double>& values)
        {
        return *std::min_element(values.cbegin(), values.cend());
        }

    [[nodiscard]]
    double MaxOf(const std::vector<double>& values)
        {
        return *std::max_element(values.cbegin(), values.cend());
        }

    [[nodiscard]]
    size_t DistinctGroupCount(const ChartSpec& spec)
        {
        if (spec.m_groupCodes.empty())
            {
            return 1;
            }
        std::vector<GroupIdType> codes{ spec.m_groupCodes };
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
        return codes.size();
        }

    // The matrix of line-plot recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "single-eight", ChartSpec{} });
        specs.push_back({ "wide-y-range",
                          ChartSpec{ .m_yValues = { 2, 90, 15, 400, 60, 720, 33, 210 },
                                     .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 } } });
        specs.push_back({ "negative-quadrant",
                          ChartSpec{ .m_yValues = { -8, -3, -12, -1, -6, -15, -4, -9 },
                                     .m_xValues = { -4, -3, -2, -1, 1, 2, 3, 4 } } });
        specs.push_back(
            { "three-lines",
              ChartSpec{ .m_yValues = { 3, 30, 60, 5, 33, 63, 7, 36, 66, 9, 39, 69 },
                         .m_xValues = { 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4 },
                         .m_groupCodes = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" }, { 2, L"C" } } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to LinePlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         line count, object count
        if (specName == "single-eight")
            {
            return LayoutFingerprint{ { 0, 14, 2, 0, false, 8 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 6 };
            }
        if (specName == "wide-y-range")
            {
            return LayoutFingerprint{ { 0, 800, 80, 0, false, 11 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 6 };
            }
        if (specName == "negative-quadrant")
            {
            return LayoutFingerprint{ { -16, 0, 2, 0, false, 9 },
                                      { -4, 4, 1, 0, false, 9 },
                                      1, 6 };
            }
        if (specName == "three-lines")
            {
            return LayoutFingerprint{ { 0, 70, 5, 0, false, 15 },
                                      { 1, 4, 1, 0, false, 4 },
                                      3, 8 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("LinePlot layout is deterministic and idempotent", "[lineplot][render]")
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
// produced. Update ExpectedFingerprint() only for a deliberate, reviewed change
// to LinePlot layout.
TEST_CASE("LinePlot layout matches the recorded baseline", "[lineplot][render]")
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

TEST_CASE("LinePlot axes enclose the data", "[lineplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK_FALSE(print.m_leftYAxis.m_reversed);
            CHECK_FALSE(print.m_bottomXAxis.m_reversed);
            CHECK(print.m_leftYAxis.m_rangeStart <= MinOf(spec.m_yValues));
            CHECK(print.m_leftYAxis.m_rangeEnd >= MaxOf(spec.m_yValues));
            CHECK(print.m_bottomXAxis.m_rangeStart <= MinOf(spec.m_xValues));
            CHECK(print.m_bottomXAxis.m_rangeEnd >= MaxOf(spec.m_xValues));
            CHECK(print.m_leftYAxis.m_interval > 0.0);
            CHECK(print.m_bottomXAxis.m_interval > 0.0);
            }
        }
    }

TEST_CASE("LinePlot builds one line per group", "[lineplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_lineCount == DistinctGroupCount(spec));
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("LinePlot characterization dump", "[lineplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/lineplot_characterization.txt" };
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
