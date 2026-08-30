///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplotrendertests.cpp
// Purpose:     Characterization tests for WCurvePlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of WCurvePlot: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, line count, and the
// number of render objects produced. WCurvePlot derives from LinePlot and always
// groups the data into one line per observation. The assertions are invariance
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

#include "../../src/graphs/wcurveplot.h"
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

    struct ChartSpec
        {
        std::vector<double> m_yValues{ 6, 2, 5, 5, 2, 2.5, 3.2, 5.25 };
        std::vector<double> m_xValues{ 1, 2, 3, 4, 1, 2, 3, 4 };
        std::vector<GroupIdType> m_groupCodes{ 0, 0, 0, 0, 1, 1, 1, 1 };
        ColumnWithStringTable::StringTableType m_groupLabels{ { 0, L"Nancy" }, { 1, L"Tina" } };
        };

    [[nodiscard]]
    std::shared_ptr<WCurvePlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = MakeGroupedXYDataset(L"y", spec.m_yValues, L"x", spec.m_xValues, L"group",
                                            spec.m_groupCodes, spec.m_groupLabels);

        auto chart = std::make_shared<WCurvePlot>(canvas);
        chart->SetData(dataset, L"y", L"x", std::optional<wxString>{ L"group" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<WCurvePlot>& chart)
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
        std::vector<GroupIdType> codes{ spec.m_groupCodes };
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
        return codes.size();
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "two-students", ChartSpec{} });
        specs.push_back(
            { "three-students",
              ChartSpec{ .m_yValues = { 6, 2, 5, 5, 2, 2.5, 3.2, 5.25, 5.75, 1, 4, 2 },
                         .m_xValues = { 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4 },
                         .m_groupCodes = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 },
                         .m_groupLabels = { { 0, L"Nancy" }, { 1, L"Tina" }, { 2, L"Sharry" } } } });
        specs.push_back(
            { "single-student",
              ChartSpec{ .m_yValues = { 4, 1, 3, 2 },
                         .m_xValues = { 1, 2, 3, 4 },
                         .m_groupCodes = { 0, 0, 0, 0 },
                         .m_groupLabels = { { 0, L"Frank" } } } });
        specs.push_back(
            { "four-periods-wide",
              ChartSpec{ .m_yValues = { 7, 1, 6, 2, 3, 7, 2, 6 },
                         .m_xValues = { 1, 2, 3, 4, 1, 2, 3, 4 },
                         .m_groupCodes = { 0, 0, 0, 0, 1, 1, 1, 1 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        return specs;
        }

    // Exact layout output recorded from the [dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to WCurvePlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         line count, object count
        if (specName == "two-students")
            {
            return LayoutFingerprint{ { 0, 6, 1, 1, false, 7 },
                                      { 1, 4, 1, 0, false, 4 },
                                      2, 6 };
            }
        if (specName == "three-students")
            {
            return LayoutFingerprint{ { 0, 6, 1, 1, false, 7 },
                                      { 1, 4, 1, 0, false, 4 },
                                      3, 7 };
            }
        if (specName == "single-student")
            {
            return LayoutFingerprint{ { 0, 4, 1, 0, false, 5 },
                                      { 1, 4, 1, 0, false, 4 },
                                      1, 5 };
            }
        if (specName == "four-periods-wide")
            {
            return LayoutFingerprint{ { 0, 7, 1, 0, false, 8 },
                                      { 1, 4, 1, 0, false, 4 },
                                      2, 6 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("WCurvePlot layout is deterministic and idempotent", "[wcurveplot][render]")
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

TEST_CASE("WCurvePlot layout matches the recorded baseline", "[wcurveplot][render]")
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

TEST_CASE("WCurvePlot axes enclose the data", "[wcurveplot][render]")
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

TEST_CASE("WCurvePlot builds one line per observation", "[wcurveplot][render]")
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
// executable.
TEST_CASE("WCurvePlot characterization dump", "[wcurveplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/wcurveplot_characterization.txt" };
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
