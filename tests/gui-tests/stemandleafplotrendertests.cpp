///////////////////////////////////////////////////////////////////////////////
// Name:        stemandleafplotrendertests.cpp
// Purpose:     Characterization tests for StemAndLeafPlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of StemAndLeafPlot: left-Y
// and bottom-X axis ranges and intervals, axis slot counts, whether grouping is
// in use, the subgroup count, and the number of render objects produced.
// StemAndLeafPlot derives from GroupGraph2D and draws a colorful table-like
// layout of stems and leaves. An optional two-level grouping column produces a
// back-to-back display. The assertions are invariance based; a separate
// exact-value guard compares against a recorded baseline.

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

#include "../../src/graphs/stemandleafplot.h"
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
        bool m_usingGrouping{ false };
        size_t m_groupCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_leftYAxis == that.m_leftYAxis && m_bottomXAxis == that.m_bottomXAxis &&
                   m_usingGrouping == that.m_usingGrouping && m_groupCount == that.m_groupCount &&
                   m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "leftY" << m_leftYAxis.ToString() << " | bottomX" << m_bottomXAxis.ToString()
                 << " | grouping=" << (m_usingGrouping ? 1 : 0) << " groups=" << m_groupCount
                 << " objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        std::vector<double> m_scores{ 12, 14, 15, 15, 17, 21, 23, 24, 28, 33 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    [[nodiscard]]
    std::shared_ptr<StemAndLeafPlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        const bool grouped{ !spec.m_groupCodes.empty() };
        auto dataset = grouped ? MakeGroupedContinuousDataset(L"score", spec.m_scores, L"group",
                                                             spec.m_groupCodes, spec.m_groupLabels) :
                                 MakeContinuousDataset(L"score", spec.m_scores);

        auto chart = std::make_shared<StemAndLeafPlot>(canvas);
        chart->SetData(dataset, L"score",
                       grouped ? std::optional<wxString>{ L"group" } : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<StemAndLeafPlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_usingGrouping = chart->IsUsingGrouping();
        print.m_groupCount = chart->GetGroupCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "single-series", ChartSpec{} });
        specs.push_back(
            { "spread", ChartSpec{ .m_scores = { 5, 9, 14, 22, 31, 38, 44, 52, 61, 70 } } });
        specs.push_back({ "back-to-back",
                          ChartSpec{ .m_scores = { 12, 25, 14, 28, 17, 22 },
                                     .m_groupCodes = { 0, 1, 0, 1, 0, 1 },
                                     .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back({ "tight", ChartSpec{ .m_scores = { 40, 41, 42, 43, 44, 45 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to StemAndLeafPlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         using-grouping, group count, object count
        // StemAndLeafPlot draws a table-like layout, so both axes stay at their
        // default empty [0, 0] state.
        if (specName == "single-series")
            {
            return LayoutFingerprint{ { 0, 0, 1, 0, false, 0 },
                                      { 0, 0, 1, 0, false, 0 },
                                      false, 0, 16 };
            }
        if (specName == "spread")
            {
            return LayoutFingerprint{ { 0, 0, 1, 0, false, 0 },
                                      { 0, 0, 1, 0, false, 0 },
                                      false, 0, 26 };
            }
        if (specName == "back-to-back")
            {
            return LayoutFingerprint{ { 0, 0, 1, 0, false, 0 },
                                      { 0, 0, 1, 0, false, 0 },
                                      true, 2, 19 };
            }
        if (specName == "tight")
            {
            return LayoutFingerprint{ { 0, 0, 1, 0, false, 0 },
                                      { 0, 0, 1, 0, false, 0 },
                                      false, 0, 12 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("StemAndLeafPlot layout is deterministic and idempotent", "[stemandleafplot][render]")
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

TEST_CASE("StemAndLeafPlot layout matches the recorded baseline", "[stemandleafplot][render]")
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

TEST_CASE("StemAndLeafPlot goes back-to-back only with a two-level column",
          "[stemandleafplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(chart->IsUsingGrouping() == !spec.m_groupCodes.empty());
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("StemAndLeafPlot characterization dump", "[stemandleafplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/stemandleafplot_characterization.txt" };
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
