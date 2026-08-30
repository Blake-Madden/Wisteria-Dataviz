///////////////////////////////////////////////////////////////////////////////
// Name:        fleschchartrendertests.cpp
// Purpose:     Characterization tests for FleschChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of FleschChart: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, whether grouping is in
// use, the subgroup count, and the number of render objects produced.
// FleschChart derives from GroupGraph2D and draws three vertical rulers
// (words-per-sentence, score, syllables-per-word) with points connected across
// them. The assertions are invariance based; a separate exact-value guard
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

#include "../../src/graphs/fleschchart.h"
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
        std::vector<double> m_wordsPerSentence{ 14, 18, 20 };
        std::vector<double> m_scores{ 70, 60, 55 };
        std::vector<double> m_syllablesPerWord{ 1.4, 1.5, 1.55 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    // FleschChart needs three continuous columns (words-per-sentence, score,
    // syllables-per-word) plus an optional group column.
    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddContinuousColumn(L"wps");
        dataset->AddContinuousColumn(L"score");
        dataset->AddContinuousColumn(L"spw");
        const bool grouped{ !spec.m_groupCodes.empty() };
        if (grouped)
            {
            dataset->AddCategoricalColumn(L"group", spec.m_groupLabels);
            }
        for (size_t idx = 0; idx < spec.m_scores.size(); ++idx)
            {
            auto row = RowInfo()
                           .Continuous({ spec.m_wordsPerSentence[idx], spec.m_scores[idx],
                                         spec.m_syllablesPerWord[idx] })
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
    std::shared_ptr<FleschChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<FleschChart>(canvas);
        chart->SetData(BuildDataset(spec), L"wps", L"score", L"spw",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<FleschChart>& chart)
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
                          ChartSpec{ .m_wordsPerSentence = { 10, 14, 18, 22, 25 },
                                     .m_scores = { 85, 72, 60, 50, 42 },
                                     .m_syllablesPerWord = { 1.3, 1.4, 1.5, 1.6, 1.68 } } });
        specs.push_back({ "two-groups",
                          ChartSpec{ .m_wordsPerSentence = { 12, 20, 15, 22, 13, 19 },
                                     .m_scores = { 78, 55, 70, 50, 75, 58 },
                                     .m_syllablesPerWord = { 1.35, 1.55, 1.42, 1.6, 1.38, 1.52 },
                                     .m_groupCodes = { 0, 1, 0, 1, 0, 1 },
                                     .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        specs.push_back({ "tight",
                          ChartSpec{ .m_wordsPerSentence = { 17, 17.5, 18 },
                                     .m_scores = { 62, 61, 60 },
                                     .m_syllablesPerWord = { 1.5, 1.51, 1.52 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to FleschChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         using-grouping, group count, object count
        if (specName == "single-band" || specName == "tight")
            {
            return LayoutFingerprint{ { 0, 110, 10, 0, false, 12 },
                                      { 0, 4, 1, 0, false, 5 },
                                      false, 0, 16 };
            }
        if (specName == "spread")
            {
            return LayoutFingerprint{ { 0, 110, 10, 0, false, 12 },
                                      { 0, 4, 1, 0, false, 5 },
                                      false, 0, 20 };
            }
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 0, 110, 10, 0, false, 12 },
                                      { 0, 4, 1, 0, false, 5 },
                                      true, 2, 22 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("FleschChart layout is deterministic and idempotent", "[fleschchart][render]")
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

TEST_CASE("FleschChart layout matches the recorded baseline", "[fleschchart][render]")
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

TEST_CASE("FleschChart lays out a non-reversed left ruler with a positive interval",
          "[fleschchart][render]")
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

TEST_CASE("FleschChart grouping flag tracks the group column", "[fleschchart][render]")
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
TEST_CASE("FleschChart characterization dump", "[fleschchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/fleschchart_characterization.txt" };
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
