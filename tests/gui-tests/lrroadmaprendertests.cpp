///////////////////////////////////////////////////////////////////////////////
// Name:        lrroadmaprendertests.cpp
// Purpose:     Characterization tests for LRRoadmap layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of LRRoadmap: the number of
// render objects produced. LRRoadmap derives from Roadmap (a Graph2D) and draws
// a winding road whose curves and road-stop sizes encode regression
// coefficients; it exposes no data axes and no public road-stop count, so the
// fingerprint is just the render object count. The assertions are invariance
// based; a separate exact-value guard compares against a recorded baseline.

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

#include "../../src/graphs/lrroadmap.h"
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
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        std::vector<wxString> m_predictors{ L"Being female", L"Being an athlete", L"Being older",
                                           L"First generation", L"Works on campus" };
        std::vector<double> m_coefficients{ 0.19, 0.29, -0.17, -0.08, 0.05 };
        std::vector<double> m_pValues;
        std::optional<double> m_pLevel;
        std::optional<Influence> m_include;
        };

    [[nodiscard]]
    std::shared_ptr<LRRoadmap> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        ColumnWithStringTable::StringTableType factorTable;
        for (size_t idx = 0; idx < spec.m_predictors.size(); ++idx)
            {
            factorTable.insert({ static_cast<GroupIdType>(idx), spec.m_predictors[idx] });
            }

        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"factor", factorTable);
        dataset->AddContinuousColumn(L"coefficient");
        const bool hasPValues{ !spec.m_pValues.empty() };
        if (hasPValues)
            {
            dataset->AddContinuousColumn(L"pvalue");
            }

        for (size_t idx = 0; idx < spec.m_predictors.size(); ++idx)
            {
            std::vector<double> continuous{ spec.m_coefficients[idx] };
            if (hasPValues)
                {
                continuous.push_back(spec.m_pValues[idx]);
                }
            dataset->AddRow(RowInfo()
                                .Categoricals({ static_cast<GroupIdType>(idx) })
                                .Continuous(continuous)
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<LRRoadmap>(canvas);
        chart->SetData(dataset, L"factor", L"coefficient",
                       hasPValues ? std::optional<wxString>{ L"pvalue" } : std::nullopt,
                       spec.m_pLevel, spec.m_include, std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<LRRoadmap>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "all-factors", ChartSpec{} });
        specs.push_back({ "negative-only", ChartSpec{ .m_include = InfluenceNegative } });
        specs.push_back({ "positive-only", ChartSpec{ .m_include = InfluencePositive } });
        specs.push_back({ "p-filtered",
                          ChartSpec{ .m_pValues = { 0.009, 0.001, 0.002, 0.20, 0.30 },
                                     .m_pLevel = 0.05 } });
        specs.push_back({ "single-factor",
                          ChartSpec{ .m_predictors = { L"Being an athlete" },
                                     .m_coefficients = { 0.29 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to LRRoadmap layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "all-factors")
            {
            return LayoutFingerprint{ 17 };
            }
        if (specName == "negative-only")
            {
            return LayoutFingerprint{ 11 };
            }
        if (specName == "positive-only" || specName == "p-filtered")
            {
            return LayoutFingerprint{ 13 };
            }
        if (specName == "single-factor")
            {
            return LayoutFingerprint{ 9 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("LRRoadmap layout is deterministic and idempotent", "[lrroadmap][render]")
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

TEST_CASE("LRRoadmap layout matches the recorded baseline", "[lrroadmap][render]")
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

TEST_CASE("LRRoadmap emits render objects for every predictor filter", "[lrroadmap][render]")
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
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("LRRoadmap characterization dump", "[lrroadmap][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/lrroadmap_characterization.txt" };
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
