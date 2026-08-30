///////////////////////////////////////////////////////////////////////////////
// Name:        danielsonbryan2plotrendertests.cpp
// Purpose:     Characterization tests for DanielsonBryan2Plot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of DanielsonBryan2Plot:
// left-Y and bottom-X axis ranges and intervals, axis slot counts, whether
// grouping is in use, the subgroup count, and the number of render objects
// produced. DanielsonBryan2Plot derives from GroupGraph2D and draws a fixed
// Flesch Reading Ease derivative scale with the documents' scores plotted onto
// it. The assertions are invariance based; a separate exact-value guard compares
// against a recorded baseline.

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

#include "../../src/graphs/danielsonbryan2plot.h"
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
        std::vector<double> m_scores{ 45, 58, 62 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    [[nodiscard]]
    std::shared_ptr<DanielsonBryan2Plot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        const bool grouped{ !spec.m_groupCodes.empty() };
        auto dataset = grouped ? MakeGroupedContinuousDataset(L"score", spec.m_scores, L"group",
                                                             spec.m_groupCodes, spec.m_groupLabels) :
                                 MakeContinuousDataset(L"score", spec.m_scores);

        auto chart = std::make_shared<DanielsonBryan2Plot>(canvas);
        chart->SetData(dataset, L"score",
                       grouped ? std::optional<wxString>{ L"group" } : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<DanielsonBryan2Plot>& chart)
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
        specs.push_back({ "single-band", ChartSpec{} });
        specs.push_back(
            { "spread", ChartSpec{ .m_scores = { 20, 35, 48, 60, 72, 55, 40 } } });
        specs.push_back({ "two-groups",
                          ChartSpec{ .m_scores = { 30, 65, 45, 70, 38, 60 },
                                     .m_groupCodes = { 0, 1, 0, 1, 0, 1 },
                                     .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back({ "tight", ChartSpec{ .m_scores = { 50, 51, 52 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to DanielsonBryan2Plot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         using-grouping, group count, object count
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 2, 1, 0, false, 3 },
                                      true, 2, 8 };
            }
        if (specName == "single-band" || specName == "spread" || specName == "tight")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 2, 1, 0, false, 3 },
                                      false, 0, 8 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("DanielsonBryan2Plot layout is deterministic and idempotent",
          "[danielsonbryan2plot][render]")
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

TEST_CASE("DanielsonBryan2Plot layout matches the recorded baseline",
          "[danielsonbryan2plot][render]")
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

TEST_CASE("DanielsonBryan2Plot lays out a non-reversed scale with a positive interval",
          "[danielsonbryan2plot][render]")
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
            CHECK(print.m_objectCount > 0);
            }
        }
    }

TEST_CASE("DanielsonBryan2Plot grouping flag tracks the group column",
          "[danielsonbryan2plot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            LayOutOffscreen(canvas, chart);

            INFO("spec: " << name);
            CHECK(chart->IsUsingGrouping() == !spec.m_groupCodes.empty());
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("DanielsonBryan2Plot characterization dump", "[danielsonbryan2plot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/danielsonbryan2plot_characterization.txt" };
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
