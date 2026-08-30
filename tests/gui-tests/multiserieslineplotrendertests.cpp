///////////////////////////////////////////////////////////////////////////////
// Name:        multiserieslineplotrendertests.cpp
// Purpose:     Characterization tests for MultiSeriesLinePlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of MultiSeriesLinePlot:
// left-Y and bottom-X axis ranges and intervals, axis slot counts, line count,
// and the number of render objects produced. MultiSeriesLinePlot derives from
// LinePlot and draws one line per continuous Y column instead of per group. The
// assertions are invariance based; a separate exact-value guard compares against
// a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/multi_series_lineplot.h"
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
        std::vector<wxString> m_yColumnNames{ L"y1", L"y2" };
        // One inner vector per Y column, each the same length as m_xValues.
        std::vector<std::vector<double>> m_ySeries{ { 3, 8, 5, 11, 7, 14, 9, 6 },
                                                    { 5, 6, 9, 8, 12, 10, 15, 13 } };
        std::vector<double> m_xValues{ 1, 2, 3, 4, 5, 6, 7, 8 };
        };

    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        for (const auto& yName : spec.m_yColumnNames)
            {
            dataset->AddContinuousColumn(yName);
            }
        dataset->AddContinuousColumn(L"x");
        for (size_t row = 0; row < spec.m_xValues.size(); ++row)
            {
            std::vector<double> continuousValues;
            continuousValues.reserve(spec.m_yColumnNames.size() + 1);
            for (const auto& series : spec.m_ySeries)
                {
                continuousValues.push_back(series[row]);
                }
            continuousValues.push_back(spec.m_xValues[row]);
            dataset->AddRow(RowInfo()
                                .Continuous(std::move(continuousValues))
                                .Id(wxString::Format(L"obs%d", static_cast<int>(row))));
            }
        return dataset;
        }

    [[nodiscard]]
    std::shared_ptr<MultiSeriesLinePlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<MultiSeriesLinePlot>(canvas);
        chart->SetData(BuildDataset(spec), spec.m_yColumnNames, L"x");
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<MultiSeriesLinePlot>& chart)
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
    double SeriesMin(const ChartSpec& spec)
        {
        double smallest{ spec.m_ySeries.front().front() };
        for (const auto& series : spec.m_ySeries)
            {
            smallest = std::min(smallest, *std::min_element(series.cbegin(), series.cend()));
            }
        return smallest;
        }

    [[nodiscard]]
    double SeriesMax(const ChartSpec& spec)
        {
        double largest{ spec.m_ySeries.front().front() };
        for (const auto& series : spec.m_ySeries)
            {
            largest = std::max(largest, *std::max_element(series.cbegin(), series.cend()));
            }
        return largest;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "two-series", ChartSpec{} });
        specs.push_back(
            { "three-series",
              ChartSpec{ .m_yColumnNames = { L"y1", L"y2", L"y3" },
                         .m_ySeries = { { 3, 8, 5, 11, 7, 14, 9, 6 },
                                        { 5, 6, 9, 8, 12, 10, 15, 13 },
                                        { 1, 4, 2, 7, 5, 9, 6, 3 } },
                         .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 } } });
        specs.push_back({ "single-series",
                          ChartSpec{ .m_yColumnNames = { L"y1" },
                                     .m_ySeries = { { 3, 8, 5, 11, 7, 14, 9, 6 } },
                                     .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 } } });
        specs.push_back(
            { "wide-range-series",
              ChartSpec{ .m_yColumnNames = { L"y1", L"y2" },
                         .m_ySeries = { { 2, 90, 15, 400, 60, 720, 33, 210 },
                                        { 50, 300, 120, 650, 210, 800, 140, 500 } },
                         .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 } } });
        return specs;
        }

    // Exact layout output recorded from the [dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to MultiSeriesLinePlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         line count, object count
        if (specName == "two-series")
            {
            return LayoutFingerprint{ { 0, 16, 2, 0, false, 9 },
                                      { 1, 8, 1, 0, false, 8 },
                                      2, 7 };
            }
        if (specName == "three-series")
            {
            return LayoutFingerprint{ { 0, 16, 2, 0, false, 9 },
                                      { 1, 8, 1, 0, false, 8 },
                                      3, 8 };
            }
        if (specName == "single-series")
            {
            return LayoutFingerprint{ { 0, 14, 2, 0, false, 8 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 6 };
            }
        if (specName == "wide-range-series")
            {
            return LayoutFingerprint{ { 0, 800, 80, 0, false, 11 },
                                      { 1, 8, 1, 0, false, 8 },
                                      2, 7 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("MultiSeriesLinePlot layout is deterministic and idempotent",
          "[multiserieslineplot][render]")
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

TEST_CASE("MultiSeriesLinePlot layout matches the recorded baseline",
          "[multiserieslineplot][render]")
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

TEST_CASE("MultiSeriesLinePlot axes enclose the data", "[multiserieslineplot][render]")
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
            CHECK(print.m_leftYAxis.m_rangeStart <= SeriesMin(spec));
            CHECK(print.m_leftYAxis.m_rangeEnd >= SeriesMax(spec));
            CHECK(print.m_leftYAxis.m_interval > 0.0);
            CHECK(print.m_bottomXAxis.m_interval > 0.0);
            }
        }
    }

TEST_CASE("MultiSeriesLinePlot builds one line per Y column", "[multiserieslineplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_lineCount == spec.m_yColumnNames.size());
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("MultiSeriesLinePlot characterization dump", "[multiserieslineplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/multiserieslineplot_characterization.txt" };
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
