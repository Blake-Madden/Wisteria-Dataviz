///////////////////////////////////////////////////////////////////////////////
// Name:        proconroadmaprendertests.cpp
// Purpose:     Characterization tests for ProConRoadmap layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of ProConRoadmap: the number
// of render objects produced. ProConRoadmap derives from Roadmap (a Graph2D) and
// draws a winding road whose curves and road-stop sizes encode how often each
// pro or con was mentioned; it exposes no data axes and no public road-stop
// count, so the fingerprint is just the render object count. The assertions are
// invariance based; a separate exact-value guard compares against a recorded
// baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/proconroadmap.h"
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
        std::vector<wxString> m_pros{ L"New features", L"New features", L"Better UI",
                                     L"Support",      L"Speed",        L"Speed" };
        std::vector<wxString> m_cons{ L"Cost", L"Cost", L"Cost", L"Retraining", L"Downtime", L"Bugs" };
        std::vector<double> m_conValues;
        std::optional<size_t> m_minimumCount;
        Roadmap::RoadStopTheme m_theme{ Roadmap::RoadStopTheme::LocationMarkers };
        };

    [[nodiscard]]
    std::map<wxString, GroupIdType> BuildCodes(const std::vector<wxString>& values,
                                               ColumnWithStringTable::StringTableType& table)
        {
        std::map<wxString, GroupIdType> codes;
        for (const auto& value : values)
            {
            if (codes.find(value) == codes.cend())
                {
                const auto nextCode{ static_cast<GroupIdType>(codes.size()) };
                codes.insert({ value, nextCode });
                table.insert({ nextCode, value });
                }
            }
        return codes;
        }

    [[nodiscard]]
    std::shared_ptr<ProConRoadmap> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        ColumnWithStringTable::StringTableType proTable;
        ColumnWithStringTable::StringTableType conTable;
        const auto proCodes = BuildCodes(spec.m_pros, proTable);
        const auto conCodes = BuildCodes(spec.m_cons, conTable);

        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"pro", proTable);
        dataset->AddCategoricalColumn(L"con", conTable);
        const bool hasConValues{ !spec.m_conValues.empty() };
        if (hasConValues)
            {
            dataset->AddContinuousColumn(L"conTotal");
            }

        for (size_t idx = 0; idx < spec.m_pros.size(); ++idx)
            {
            auto row = RowInfo()
                           .Categoricals({ proCodes.at(spec.m_pros[idx]),
                                           conCodes.at(spec.m_cons[idx]) })
                           .Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
            if (hasConValues)
                {
                row.Continuous({ spec.m_conValues[idx] });
                }
            dataset->AddRow(row);
            }

        auto chart = std::make_shared<ProConRoadmap>(canvas);
        chart->SetRoadStopTheme(spec.m_theme);
        chart->SetData(dataset, L"pro", std::nullopt, L"con",
                       hasConValues ? std::optional<wxString>{ L"conTotal" } : std::nullopt,
                       spec.m_minimumCount);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<ProConRoadmap>& chart)
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
        specs.push_back({ "pros-cons", ChartSpec{} });
        specs.push_back({ "min-count-2", ChartSpec{ .m_minimumCount = size_t{ 2 } } });
        specs.push_back({ "con-totals",
                          ChartSpec{ .m_conValues = { 22, 22, 22, 12, 8, 5 } } });
        specs.push_back({ "road-signs",
                          ChartSpec{ .m_theme = Roadmap::RoadStopTheme::RoadSigns } });
        specs.push_back(
            { "net-sentiment",
              ChartSpec{ .m_pros = { L"Cost", L"New features", L"Better UI", L"Support", L"Speed",
                                     L"Speed" },
                         .m_cons = { L"Cost", L"Cost", L"Cost", L"Retraining", L"Downtime",
                                     L"Bugs" } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to ProConRoadmap layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "pros-cons" || specName == "con-totals" || specName == "road-signs" ||
            specName == "net-sentiment")
            {
            return LayoutFingerprint{ 23 };
            }
        if (specName == "min-count-2")
            {
            return LayoutFingerprint{ 13 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("ProConRoadmap layout is deterministic and idempotent", "[proconroadmap][render]")
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

TEST_CASE("ProConRoadmap layout matches the recorded baseline", "[proconroadmap][render]")
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

TEST_CASE("ProConRoadmap emits render objects for every sentiment recipe", "[proconroadmap][render]")
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
TEST_CASE("ProConRoadmap characterization dump", "[proconroadmap][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/proconroadmap_characterization.txt" };
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
