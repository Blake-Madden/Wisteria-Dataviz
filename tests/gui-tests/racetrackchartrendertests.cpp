///////////////////////////////////////////////////////////////////////////////
// Name:        racetrackchartrendertests.cpp
// Purpose:     Characterization tests for RaceTrackChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of RaceTrackChart: the
// number of render objects produced. RaceTrackChart derives from Graph2D and is
// circular (concentric track lanes with a hollow center); it exposes no
// continuous data axes, so the fingerprint is just the render object count. The
// assertions are invariance based; a separate exact-value guard compares
// against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/racetrackchart.h"
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
        std::vector<double> m_values{ 40, 30, 20 };
        std::vector<wxString> m_labels{ L"Alpha", L"Beta", L"Gamma" };
        RaceTrackChart::TrackCount m_trackCount{ RaceTrackChart::TrackCount::Auto };
        bool m_showLabels{ true };
        };

    [[nodiscard]]
    std::shared_ptr<RaceTrackChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        ColumnWithStringTable::StringTableType labelTable;
        for (size_t idx = 0; idx < spec.m_labels.size(); ++idx)
            {
            labelTable.insert({ static_cast<GroupIdType>(idx), spec.m_labels[idx] });
            }
        dataset->AddCategoricalColumn(L"label", labelTable);
        dataset->AddContinuousColumn(L"value");
        for (size_t idx = 0; idx < spec.m_values.size(); ++idx)
            {
            dataset->AddRow(RowInfo()
                                .Categoricals({ static_cast<GroupIdType>(idx) })
                                .Continuous({ spec.m_values[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<RaceTrackChart>(canvas);
        chart->SetTrackCount(spec.m_trackCount);
        chart->ShowLabels(spec.m_showLabels);
        chart->SetData(dataset, L"value", L"label");
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<RaceTrackChart>& chart)
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
        specs.push_back({ "three-lanes", ChartSpec{} });
        specs.push_back(
            { "one-track", ChartSpec{ .m_trackCount = RaceTrackChart::TrackCount::One } });
        specs.push_back(
            { "two-track", ChartSpec{ .m_trackCount = RaceTrackChart::TrackCount::Two } });
        specs.push_back({ "six-lanes",
                          ChartSpec{ .m_values = { 90, 72, 55, 40, 28, 12 },
                                     .m_labels = { L"L1", L"L2", L"L3", L"L4", L"L5", L"L6" } } });
        specs.push_back({ "no-labels", ChartSpec{ .m_showLabels = false } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to RaceTrackChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "three-lanes" || specName == "one-track" || specName == "two-track")
            {
            return LayoutFingerprint{ 14 };
            }
        if (specName == "six-lanes")
            {
            return LayoutFingerprint{ 23 };
            }
        if (specName == "no-labels")
            {
            return LayoutFingerprint{ 7 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("RaceTrackChart layout is deterministic and idempotent", "[racetrackchart][render]")
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

TEST_CASE("RaceTrackChart layout matches the recorded baseline", "[racetrackchart][render]")
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

TEST_CASE("RaceTrackChart emits render objects for every track-count preference",
          "[racetrackchart][render]")
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
TEST_CASE("RaceTrackChart characterization dump", "[racetrackchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/racetrackchart_characterization.txt" };
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
