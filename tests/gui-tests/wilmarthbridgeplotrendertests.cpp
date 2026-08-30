///////////////////////////////////////////////////////////////////////////////
// Name:        wilmarthbridgeplotrendertests.cpp
// Purpose:     Characterization tests for WilmarthBridgePlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of WilmarthBridgePlot:
// bottom-X, left-Y, and right-Y axis ranges and intervals, axis slot counts,
// and the number of render objects produced. WilmarthBridgePlot derives from
// Graph2D and draws a tapering character-aligned grid whose silhouette traces a
// survival curve, optionally with at-risk / survival-percent statistics down the
// right axis. The assertions are invariance based; a separate exact-value guard
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

#include "../../src/graphs/wilmarth_bridge_plot.h"
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
        AxisFingerprint m_bottomXAxis;
        AxisFingerprint m_leftYAxis;
        AxisFingerprint m_rightYAxis;
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_bottomXAxis == that.m_bottomXAxis && m_leftYAxis == that.m_leftYAxis &&
                   m_rightYAxis == that.m_rightYAxis && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "bottomX" << m_bottomXAxis.ToString() << " | leftY" << m_leftYAxis.ToString()
                 << " | rightY" << m_rightYAxis.ToString() << " | objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        std::vector<wxString> m_labels{ L"F", L"O", L"R", L"S", L"U", L"W", L"A", L"N" };
        std::vector<double> m_entered{ 1963, 1963, 1963, 1963, 1964, 1964, 1965, 1965 };
        std::vector<double> m_faded{ 1967, 1963, 1971, 1968, 1969, 1965, 1971, 1966 };
        std::vector<int> m_status;
        WilmarthBridgePlot::SurvivalDisplay m_survivalDisplay{
            WilmarthBridgePlot::SurvivalDisplay::None
        };
        WilmarthBridgePlot::FadeEffect m_fadeEffect{ WilmarthBridgePlot::FadeEffect::None };
        wxString m_terminalRowLabel;
        };

    [[nodiscard]]
    std::shared_ptr<WilmarthBridgePlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        ColumnWithStringTable::StringTableType letterTable;
        for (size_t idx = 0; idx < spec.m_labels.size(); ++idx)
            {
            letterTable.insert({ static_cast<GroupIdType>(idx), spec.m_labels[idx] });
            }

        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"letter", letterTable);
        dataset->AddContinuousColumn(L"entered");
        dataset->AddContinuousColumn(L"faded");
        const bool hasStatus{ !spec.m_status.empty() };
        if (hasStatus)
            {
            dataset->AddContinuousColumn(L"status");
            }

        for (size_t idx = 0; idx < spec.m_labels.size(); ++idx)
            {
            std::vector<double> continuous{ spec.m_entered[idx], spec.m_faded[idx] };
            if (hasStatus)
                {
                continuous.push_back(static_cast<double>(spec.m_status[idx]));
                }
            dataset->AddRow(RowInfo()
                                .Categoricals({ static_cast<GroupIdType>(idx) })
                                .Continuous(continuous)
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<WilmarthBridgePlot>(canvas);
        chart->SetFadeEffect(spec.m_fadeEffect);
        chart->SetSurvivalDisplay(spec.m_survivalDisplay);
        if (!spec.m_terminalRowLabel.empty())
            {
            chart->ShowTerminalRow(spec.m_terminalRowLabel);
            }
        chart->SetData(dataset, L"letter", L"faded", std::optional<wxString>{ L"entered" },
                       hasStatus ? std::optional<wxString>{ L"status" } : std::nullopt,
                       std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<WilmarthBridgePlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_rightYAxis = CaptureAxis(chart->GetRightYAxis());
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "letters", ChartSpec{} });
        specs.push_back({ "at-risk",
                          ChartSpec{ .m_survivalDisplay =
                                         WilmarthBridgePlot::SurvivalDisplay::AtRiskCount } });
        specs.push_back({ "survival-percent",
                          ChartSpec{ .m_survivalDisplay =
                                         WilmarthBridgePlot::SurvivalDisplay::SurvivalPercent } });
        specs.push_back({ "with-censoring",
                          ChartSpec{ .m_status = { 1, 1, 0, 1, 1, 0, 1, 1 } } });
        specs.push_back(
            { "faded-ink",
              ChartSpec{ .m_fadeEffect = WilmarthBridgePlot::FadeEffect::ElapsedTime } });
        specs.push_back({ "terminal-row", ChartSpec{ .m_terminalRowLabel = L"Oct 1971" } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to WilmarthBridgePlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: bottom-X axis start/end/interval/precision/reversed/points,
        //         left-Y axis start/end/interval/precision/reversed/points,
        //         right-Y axis start/end/interval/precision/reversed/points,
        //         object count
        // The bottom-X is a fixed [0, 8] observation scale and the left-Y a
        // reversed [1962, 1972] period scale, the same for every spec. The
        // right-Y only carries a range when a survival statistic is displayed.
        const AxisFingerprint bottomX{ 0, 8, 1, 0, false, 9 };
        const AxisFingerprint leftY{ 1962, 1972, 1, 0, true, 10 };
        const AxisFingerprint rightYEmpty{ 0, 0, 1, 0, false, 0 };
        const AxisFingerprint rightYPeriods{ 1962, 1972, 1, 0, true, 10 };
        if (specName == "letters" || specName == "faded-ink" || specName == "terminal-row")
            {
            return LayoutFingerprint{ bottomX, leftY, rightYEmpty, 40 };
            }
        if (specName == "at-risk" || specName == "survival-percent")
            {
            return LayoutFingerprint{ bottomX, leftY, rightYPeriods, 40 };
            }
        if (specName == "with-censoring")
            {
            return LayoutFingerprint{ bottomX, leftY, rightYEmpty, 41 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("WilmarthBridgePlot layout is deterministic and idempotent", "[wilmarthbridgeplot][render]")
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

TEST_CASE("WilmarthBridgePlot layout matches the recorded baseline", "[wilmarthbridgeplot][render]")
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

TEST_CASE("WilmarthBridgePlot draws a non-reversed period axis and render objects",
          "[wilmarthbridgeplot][render]")
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
            CHECK_FALSE(print.m_bottomXAxis.m_reversed);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("WilmarthBridgePlot characterization dump", "[wilmarthbridgeplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/wilmarthbridgeplot_characterization.txt" };
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
