///////////////////////////////////////////////////////////////////////////////
// Name:        likertchartrendertests.cpp
// Purpose:     Characterization tests for LikertChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of LikertChart: scaling- and
// bar-axis ranges and intervals, axis slot counts, bar count, and the number of
// render objects produced. LikertChart derives from BarChart; each survey
// question is a horizontal stacked bar. The assertions are invariance based; a
// separate exact-value guard compares against a recorded baseline.

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

#include "../../src/graphs/likertchart.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    using Format = LikertChart::LikertSurveyQuestionFormat;

    struct LayoutFingerprint
        {
        AxisFingerprint m_scalingAxis;
        AxisFingerprint m_barAxis;
        size_t m_barCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_scalingAxis == that.m_scalingAxis && m_barAxis == that.m_barAxis &&
                   m_barCount == that.m_barCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "scaling" << m_scalingAxis.ToString() << " | bar" << m_barAxis.ToString()
                 << " | bars=" << m_barCount << " objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        Format m_format{ Format::FivePoint };
        std::vector<wxString> m_questionColumns{ L"Q1", L"Q2", L"Q3" };
        // One inner vector per respondent, one code per question (1..levels; 0 = no response).
        std::vector<std::vector<GroupIdType>> m_responses{
            { 1, 3, 5 }, { 2, 2, 4 }, { 5, 1, 3 }, { 4, 3, 2 },
            { 3, 4, 5 }, { 2, 5, 1 }, { 4, 2, 3 }, { 5, 3, 4 }
        };
        };

    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const ChartSpec& spec)
        {
        const auto codes = LikertChart::CreateLabels(spec.m_format);
        auto dataset = std::make_shared<Dataset>();
        for (const auto& questionName : spec.m_questionColumns)
            {
            dataset->AddCategoricalColumn(questionName, codes);
            }
        for (size_t row = 0; row < spec.m_responses.size(); ++row)
            {
            dataset->AddRow(RowInfo()
                                .Categoricals(spec.m_responses[row])
                                .Id(wxString::Format(L"resp%d", static_cast<int>(row))));
            }
        return dataset;
        }

    [[nodiscard]]
    std::shared_ptr<LikertChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<LikertChart>(canvas, spec.m_format);
        chart->SetData(BuildDataset(spec), spec.m_questionColumns, std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<LikertChart>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_scalingAxis = CaptureAxis(chart->GetScalingAxis());
        print.m_barAxis = CaptureAxis(chart->GetBarAxis());
        print.m_barCount = chart->GetBars().size();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "three-questions-fivepoint", ChartSpec{} });
        specs.push_back(
            { "two-questions-threepoint",
              ChartSpec{ .m_format = Format::ThreePoint,
                         .m_questionColumns = { L"Q1", L"Q2" },
                         .m_responses = { { 1, 3 }, { 2, 2 }, { 3, 1 }, { 1, 2 },
                                          { 2, 3 }, { 3, 1 }, { 1, 3 }, { 2, 2 } } } });
        specs.push_back(
            { "five-questions-fivepoint",
              ChartSpec{ .m_format = Format::FivePoint,
                         .m_questionColumns = { L"Q1", L"Q2", L"Q3", L"Q4", L"Q5" },
                         .m_responses = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 1 },
                                          { 3, 4, 5, 1, 2 }, { 4, 5, 1, 2, 3 },
                                          { 5, 1, 2, 3, 4 }, { 1, 3, 5, 2, 4 },
                                          { 2, 4, 1, 3, 5 }, { 3, 5, 2, 4, 1 } } } });
        specs.push_back(
            { "one-question-fivepoint",
              ChartSpec{ .m_format = Format::FivePoint,
                         .m_questionColumns = { L"Q1" },
                         .m_responses = { { 1 }, { 2 }, { 3 }, { 4 },
                                          { 5 }, { 3 }, { 2 }, { 4 } } } });
        return specs;
        }

    // Exact layout output recorded from the [dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to LikertChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, object count
        if (specName == "three-questions-fivepoint")
            {
            return LayoutFingerprint{ { 0, 230, 10, 0, false, 24 },
                                      { 0.5, 3.5, 1, 1, false, 4 },
                                      3, 51 };
            }
        if (specName == "two-questions-threepoint")
            {
            return LayoutFingerprint{ { 0, 220, 10, 0, false, 23 },
                                      { 0.5, 2.5, 1, 1, false, 3 },
                                      2, 29 };
            }
        if (specName == "five-questions-fivepoint")
            {
            return LayoutFingerprint{ { 0, 230, 10, 0, false, 24 },
                                      { 0.5, 5.5, 1, 1, false, 6 },
                                      5, 79 };
            }
        if (specName == "one-question-fivepoint")
            {
            return LayoutFingerprint{ { 0, 210, 10, 0, false, 22 },
                                      { 0.5, 1.5, 1, 1, false, 2 },
                                      1, 23 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("LikertChart layout is deterministic and idempotent", "[likertchart][render]")
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

TEST_CASE("LikertChart layout matches the recorded baseline", "[likertchart][render]")
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

TEST_CASE("LikertChart builds at least one bar per question", "[likertchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_barCount >= spec.m_questionColumns.size());
            CHECK(print.m_objectCount > 0);
            CHECK_FALSE(print.m_scalingAxis.m_reversed);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("LikertChart characterization dump", "[likertchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/likertchart_characterization.txt" };
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
