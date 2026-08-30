///////////////////////////////////////////////////////////////////////////////
// Name:        crawfordgraphrendertests.cpp
// Purpose:     Characterization tests for CrawfordGraph layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of CrawfordGraph: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, whether grouping is in
// use, the subgroup count, and the number of render objects produced.
// CrawfordGraph derives from GroupGraph2D and plots a Spanish readability score
// against a syllables-per-100-words value on a fixed grid. The assertions are
// invariance based; a separate exact-value guard compares against a recorded
// baseline.

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

#include "../../src/graphs/crawfordgraph.h"
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
        std::vector<double> m_scores{ 3.0, 3.8, 4.2 };
        std::vector<double> m_syllables{ 195, 205, 212 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    // CrawfordGraph needs a score column and a syllables-per-100-words column,
    // plus an optional group column.
    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddContinuousColumn(L"score");
        dataset->AddContinuousColumn(L"syl");
        const bool grouped{ !spec.m_groupCodes.empty() };
        if (grouped)
            {
            dataset->AddCategoricalColumn(L"group", spec.m_groupLabels);
            }
        for (size_t idx = 0; idx < spec.m_scores.size(); ++idx)
            {
            auto row = RowInfo()
                           .Continuous({ spec.m_scores[idx], spec.m_syllables[idx] })
                           .Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
            if (grouped)
                {
                row.Categoricals({ spec.m_groupCodes[idx] });
                }
            dataset->AddRow(row);
            }
        return dataset;
        }

    [[nodiscard]]
    std::shared_ptr<CrawfordGraph> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<CrawfordGraph>(canvas);
        chart->SetData(BuildDataset(spec), L"score", L"syl",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<CrawfordGraph>& chart)
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
        specs.push_back({ "spread",
                          ChartSpec{ .m_scores = { 2.2, 2.8, 3.5, 4.0, 4.6 },
                                     .m_syllables = { 175, 185, 198, 208, 220 } } });
        specs.push_back({ "two-groups",
                          ChartSpec{ .m_scores = { 3.0, 4.2, 3.4, 4.5, 2.9, 4.0 },
                                     .m_syllables = { 190, 210, 195, 215, 185, 205 },
                                     .m_groupCodes = { 0, 1, 0, 1, 0, 1 },
                                     .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back({ "tight",
                          ChartSpec{ .m_scores = { 3.5, 3.6, 3.7 },
                                     .m_syllables = { 200, 201, 202 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to CrawfordGraph layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         using-grouping, group count, object count
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 166, 222, 2, 0, false, 29 },
                                      { 0.5, 7, 0.5, 1, false, 14 },
                                      true, 2, 145 };
            }
        if (specName == "single-band" || specName == "spread" || specName == "tight")
            {
            return LayoutFingerprint{ { 166, 222, 2, 0, false, 29 },
                                      { 0.5, 7, 0.5, 1, false, 14 },
                                      false, 0, 145 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("CrawfordGraph layout is deterministic and idempotent", "[crawfordgraph][render]")
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

TEST_CASE("CrawfordGraph layout matches the recorded baseline", "[crawfordgraph][render]")
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

TEST_CASE("CrawfordGraph lays out non-reversed axes with positive intervals",
          "[crawfordgraph][render]")
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
            CHECK(print.m_leftYAxis.m_interval > 0.0);
            CHECK(print.m_bottomXAxis.m_interval > 0.0);
            CHECK(print.m_objectCount > 0);
            }
        }
    }

TEST_CASE("CrawfordGraph grouping flag tracks the group column", "[crawfordgraph][render]")
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
TEST_CASE("CrawfordGraph characterization dump", "[crawfordgraph][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/crawfordgraph_characterization.txt" };
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
