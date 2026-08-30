///////////////////////////////////////////////////////////////////////////////
// Name:        heatmaprendertests.cpp
// Purpose:     Characterization tests for HeatMap layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of HeatMap: left-Y and
// bottom-X axis slot counts and the number of render objects produced (one cell
// per observation). The assertions are invariance based; a separate exact-value
// guard compares against a recorded baseline.

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

#include "../../src/graphs/heatmap.h"
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

    struct ChartSpec
        {
        std::vector<double> m_values{ 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        std::optional<size_t> m_groupColumnCount;
        };

    [[nodiscard]]
    std::shared_ptr<HeatMap> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset =
            spec.m_groupCodes.empty() ?
                MakeContinuousDataset(L"values", spec.m_values) :
                MakeGroupedContinuousDataset(L"values", spec.m_values, L"group",
                                             spec.m_groupCodes, spec.m_groupLabels);

        auto chart = std::make_shared<HeatMap>(canvas);
        chart->SetData(dataset, L"values",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" },
                       spec.m_groupColumnCount);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<HeatMap>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "twelve-ungrouped", ChartSpec{} });
        specs.push_back(
            { "twentyfive-ungrouped",
              ChartSpec{ .m_values = { 2,  4,  6,  8,  10, 12, 14, 16, 18, 20, 22, 24, 26,
                                      28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50 } } });
        specs.push_back(
            { "two-groups",
              ChartSpec{ .m_values = { 1, 3, 5, 7, 9, 11, 2, 4, 6, 8, 10, 12 },
                         .m_groupCodes = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back(
            { "three-groups-two-cols",
              ChartSpec{ .m_values = { 1, 4, 7, 10, 2, 5, 8, 11, 3, 6, 9, 12 },
                         .m_groupCodes = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" }, { 2, L"C" } },
                         .m_groupColumnCount = size_t{ 2 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to HeatMap layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         object count
        if (specName == "twelve-ungrouped")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 10, 1, 0, false, 11 },
                                      28 };
            }
        if (specName == "twentyfive-ungrouped")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 10, 1, 0, false, 11 },
                                      54 };
            }
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 10, 1, 0, false, 11 },
                                      31 };
            }
        if (specName == "three-groups-two-cols")
            {
            return LayoutFingerprint{ { 0, 10, 1, 0, false, 11 },
                                      { 0, 10, 1, 0, false, 11 },
                                      33 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("HeatMap layout is deterministic and idempotent", "[heatmap][render]")
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

TEST_CASE("HeatMap layout matches the recorded baseline", "[heatmap][render]")
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

TEST_CASE("HeatMap emits at least one render object per observation", "[heatmap][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_objectCount >= spec.m_values.size());
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("HeatMap characterization dump", "[heatmap][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/heatmap_characterization.txt" };
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
