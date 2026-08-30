///////////////////////////////////////////////////////////////////////////////
// Name:        candlestickplotrendertests.cpp
// Purpose:     Characterization tests for CandlestickPlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of CandlestickPlot: the
// price (left-Y) and date (bottom-X) axis ranges and intervals, axis slot
// counts, and the number of render objects produced. CandlestickPlot derives
// from Graph2D and draws one candle or OHLC hinge per trading day. The
// assertions are invariance based; a separate exact-value guard compares
// against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/datetime.h>
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

#include "../../src/graphs/candlestickplot.h"
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
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_leftYAxis == that.m_leftYAxis && m_bottomXAxis == that.m_bottomXAxis &&
                   m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "leftY" << m_leftYAxis.ToString() << " | bottomX" << m_bottomXAxis.ToString()
                 << " | objects=" << m_objectCount;
            return text.str();
            }
        };

    [[nodiscard]]
    wxDateTime On(const int day, const wxDateTime::Month month, const int year)
        {
        return wxDateTime(day, month, year);
        }

    struct ChartSpec
        {
        std::vector<wxDateTime> m_dates{ On(2, wxDateTime::Jan, 2024), On(3, wxDateTime::Jan, 2024),
                                        On(4, wxDateTime::Jan, 2024), On(5, wxDateTime::Jan, 2024),
                                        On(8, wxDateTime::Jan, 2024) };
        std::vector<double> m_open{ 20.0, 21.0, 22.5, 21.4, 23.6 };
        std::vector<double> m_high{ 22.0, 23.0, 22.8, 24.0, 25.0 };
        std::vector<double> m_low{ 19.0, 20.5, 21.0, 21.0, 23.0 };
        std::vector<double> m_close{ 21.0, 22.5, 21.4, 23.6, 24.2 };
        CandlestickPlot::PlotType m_plotType{ CandlestickPlot::PlotType::Candlestick };
        bool m_startAtZero{ true };
        };

    [[nodiscard]]
    std::shared_ptr<CandlestickPlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddDateColumn(L"date");
        dataset->AddContinuousColumn(L"open");
        dataset->AddContinuousColumn(L"high");
        dataset->AddContinuousColumn(L"low");
        dataset->AddContinuousColumn(L"close");
        for (size_t idx = 0; idx < spec.m_dates.size(); ++idx)
            {
            dataset->AddRow(RowInfo()
                                .Dates({ spec.m_dates[idx] })
                                .Continuous({ spec.m_open[idx], spec.m_high[idx], spec.m_low[idx],
                                              spec.m_close[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<CandlestickPlot>(canvas);
        // StartAtZero has to be set before SetData so the axis range is based on the data.
        chart->GetLeftYAxis().StartAtZero(spec.m_startAtZero);
        chart->SetPlotType(spec.m_plotType);
        chart->SetData(dataset, L"date", L"open", L"high", L"low", L"close");
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<CandlestickPlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    double HighestHigh(const ChartSpec& spec)
        {
        return *std::max_element(spec.m_high.cbegin(), spec.m_high.cend());
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "week-candles", ChartSpec{} });
        specs.push_back({ "week-ohlc",
                          ChartSpec{ .m_plotType = CandlestickPlot::PlotType::Ohlc } });
        specs.push_back({ "single-day",
                          ChartSpec{ .m_dates = { On(2, wxDateTime::Jan, 2024) },
                                     .m_open = { 20.0 },
                                     .m_high = { 22.0 },
                                     .m_low = { 19.0 },
                                     .m_close = { 21.0 } } });
        specs.push_back(
            { "two-weeks",
              ChartSpec{ .m_dates = { On(2, wxDateTime::Jan, 2024), On(3, wxDateTime::Jan, 2024),
                                     On(4, wxDateTime::Jan, 2024), On(5, wxDateTime::Jan, 2024),
                                     On(8, wxDateTime::Jan, 2024), On(9, wxDateTime::Jan, 2024),
                                     On(10, wxDateTime::Jan, 2024), On(11, wxDateTime::Jan, 2024),
                                     On(12, wxDateTime::Jan, 2024), On(15, wxDateTime::Jan, 2024) },
                         .m_open = { 20.0, 21.0, 22.5, 21.4, 23.6, 24.2, 23.8, 24.5, 25.1, 24.0 },
                         .m_high = { 22.0, 23.0, 22.8, 24.0, 25.0, 25.4, 24.9, 26.0, 26.2, 25.3 },
                         .m_low = { 19.0, 20.5, 21.0, 21.0, 23.0, 23.6, 22.9, 24.1, 24.4, 23.5 },
                         .m_close = { 21.0, 22.5, 21.4, 23.6, 24.2, 23.8, 24.5, 25.1, 24.0, 24.8 } } });
        specs.push_back({ "no-zero-base", ChartSpec{ .m_startAtZero = false } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to CandlestickPlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         object count
        if (specName == "week-candles")
            {
            return LayoutFingerprint{ { 0, 25, 5, 0, false, 6 },
                                      { 0, 31, 1, 0, false, 32 },
                                      15 };
            }
        if (specName == "week-ohlc")
            {
            return LayoutFingerprint{ { 0, 25, 5, 0, false, 6 },
                                      { 0, 31, 1, 0, false, 32 },
                                      10 };
            }
        if (specName == "single-day")
            {
            return LayoutFingerprint{ { 0, 25, 5, 0, false, 6 },
                                      { 0, 31, 1, 0, false, 32 },
                                      7 };
            }
        if (specName == "two-weeks")
            {
            return LayoutFingerprint{ { 0, 35, 5, 0, false, 8 },
                                      { 0, 31, 1, 0, false, 32 },
                                      25 };
            }
        if (specName == "no-zero-base")
            {
            return LayoutFingerprint{ { 19, 25, 1, 0, false, 7 },
                                      { 0, 31, 1, 0, false, 32 },
                                      15 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("CandlestickPlot layout is deterministic and idempotent", "[candlestickplot][render]")
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

TEST_CASE("CandlestickPlot layout matches the recorded baseline", "[candlestickplot][render]")
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

TEST_CASE("CandlestickPlot draws a non-reversed price axis that encloses the highs",
          "[candlestickplot][render]")
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
            CHECK(print.m_leftYAxis.m_interval > 0.0);
            CHECK(print.m_leftYAxis.m_rangeEnd >= HighestHigh(spec));
            CHECK_FALSE(print.m_bottomXAxis.m_reversed);
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("CandlestickPlot characterization dump", "[candlestickplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/candlestickplot_characterization.txt" };
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
