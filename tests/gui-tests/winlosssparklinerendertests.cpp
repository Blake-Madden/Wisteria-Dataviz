///////////////////////////////////////////////////////////////////////////////
// Name:        winlosssparklinerendertests.cpp
// Purpose:     Characterization tests for WinLossSparkline layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of WinLossSparkline: the
// number of render objects produced. WinLossSparkline derives from Graph2D and
// draws one sparkline row per season of win/loss/tie marks; it exposes no
// continuous data axes, so the fingerprint is just the render object count. The
// assertions are invariance based; a separate exact-value guard compares
// against a recorded baseline.

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

#include "../../src/graphs/win_loss_sparkline.h"
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
        std::vector<wxString> m_seasons{ L"2022", L"2022", L"2022", L"2022", L"2022",
                                        L"2022", L"2022", L"2022", L"2022", L"2022" };
        std::vector<int> m_results{ 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
        std::vector<int> m_shutouts{ 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
        std::vector<int> m_homeGames{ 1, 1, 1, 0, 0, 1, 1, 1, 0, 0 };
        std::vector<int> m_postseason;
        bool m_highlightBest{ true };
        };

    [[nodiscard]]
    std::shared_ptr<WinLossSparkline> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();

        ColumnWithStringTable::StringTableType seasonTable;
        std::map<wxString, GroupIdType> seasonCodes;
        for (const auto& season : spec.m_seasons)
            {
            if (seasonCodes.find(season) == seasonCodes.cend())
                {
                const auto nextCode{ static_cast<GroupIdType>(seasonCodes.size()) };
                seasonCodes.insert({ season, nextCode });
                seasonTable.insert({ nextCode, season });
                }
            }
        const ColumnWithStringTable::StringTableType boolTable{ { 0, L"0" }, { 1, L"1" } };
        const ColumnWithStringTable::StringTableType resultTable{ { 0, L"0" }, { 1, L"1" },
                                                                 { 2, L"2" } };

        dataset->AddCategoricalColumn(L"season", seasonTable);
        dataset->AddCategoricalColumn(L"result", resultTable);
        dataset->AddCategoricalColumn(L"shutout", boolTable);
        dataset->AddCategoricalColumn(L"home", boolTable);
        const bool hasPostseason{ !spec.m_postseason.empty() };
        if (hasPostseason)
            {
            dataset->AddCategoricalColumn(L"post", boolTable);
            }

        for (size_t idx = 0; idx < spec.m_seasons.size(); ++idx)
            {
            std::vector<GroupIdType> categoricals{
                seasonCodes.at(spec.m_seasons[idx]), static_cast<GroupIdType>(spec.m_results[idx]),
                static_cast<GroupIdType>(spec.m_shutouts[idx]),
                static_cast<GroupIdType>(spec.m_homeGames[idx])
            };
            if (hasPostseason)
                {
                categoricals.push_back(static_cast<GroupIdType>(spec.m_postseason[idx]));
                }
            dataset->AddRow(RowInfo()
                                .Categoricals(categoricals)
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<WinLossSparkline>(canvas);
        chart->HighlightBestRecords(spec.m_highlightBest);
        chart->SetData(dataset, L"season", L"result", L"shutout", L"home",
                       hasPostseason ? std::optional<wxString>{ L"post" } : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<WinLossSparkline>& chart)
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
        specs.push_back({ "one-season", ChartSpec{} });
        specs.push_back(
            { "two-seasons",
              ChartSpec{ .m_seasons = { L"2022", L"2022", L"2022", L"2022", L"2022", L"2023",
                                       L"2023", L"2023", L"2023", L"2023" },
                         .m_results = { 1, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                         .m_shutouts = { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0 },
                         .m_homeGames = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 } } });
        specs.push_back(
            { "with-postseason",
              ChartSpec{ .m_postseason = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 } } });
        specs.push_back(
            { "with-ties",
              ChartSpec{ .m_results = { 1, 0, 2, 1, 0, 2, 1, 1, 0, 1 } } });
        specs.push_back({ "no-highlight", ChartSpec{ .m_highlightBest = false } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to WinLossSparkline layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "one-season")
            {
            return LayoutFingerprint{ 56 };
            }
        if (specName == "two-seasons")
            {
            return LayoutFingerprint{ 61 };
            }
        if (specName == "with-postseason")
            {
            return LayoutFingerprint{ 59 };
            }
        if (specName == "with-ties")
            {
            return LayoutFingerprint{ 54 };
            }
        if (specName == "no-highlight")
            {
            return LayoutFingerprint{ 52 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("WinLossSparkline layout is deterministic and idempotent", "[winlosssparkline][render]")
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

TEST_CASE("WinLossSparkline layout matches the recorded baseline", "[winlosssparkline][render]")
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

TEST_CASE("WinLossSparkline emits render objects for every spec", "[winlosssparkline][render]")
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
TEST_CASE("WinLossSparkline characterization dump", "[winlosssparkline][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/winlosssparkline_characterization.txt" };
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
