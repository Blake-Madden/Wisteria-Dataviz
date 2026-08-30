///////////////////////////////////////////////////////////////////////////////
// Name:        tablerendertests.cpp
// Purpose:     Characterization tests for Table layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of Table: the reported row
// and column counts and the number of render objects produced. Table is a grid
// with no data axes. The assertions are invariance based; a separate exact-value
// guard compares against a recorded baseline.

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

#include "../../src/graphs/table.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace wisteria_render_tests;

namespace
    {
    struct LayoutFingerprint
        {
        size_t m_rowCount{ 0 };
        size_t m_columnCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_rowCount == that.m_rowCount && m_columnCount == that.m_columnCount &&
                   m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "rows=" << m_rowCount << " cols=" << m_columnCount
                 << " objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        size_t m_rows{ 3 };
        size_t m_cols{ 3 };
        bool m_fillValues{ false };
        };

    [[nodiscard]]
    std::shared_ptr<Table> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<Table>(canvas);
        chart->SetTableSize(spec.m_rows, spec.m_cols);
        if (spec.m_fillValues)
            {
            for (size_t row = 0; row < spec.m_rows; ++row)
                {
                for (size_t col = 0; col < spec.m_cols; ++col)
                    {
                    chart->GetCell(row, col).SetValue(
                        static_cast<double>((row * spec.m_cols) + col));
                    }
                }
            }
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<Table>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_rowCount = chart->GetRowCount();
        print.m_columnCount = chart->GetColumnCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "one-cell", ChartSpec{ .m_rows = 1, .m_cols = 1 } });
        specs.push_back({ "three-by-four", ChartSpec{ .m_rows = 3, .m_cols = 4 } });
        specs.push_back({ "five-by-two", ChartSpec{ .m_rows = 5, .m_cols = 2 } });
        specs.push_back({ "four-by-four-filled",
                          ChartSpec{ .m_rows = 4, .m_cols = 4, .m_fillValues = true } });
        specs.push_back({ "two-by-eight", ChartSpec{ .m_rows = 2, .m_cols = 8 } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to Table layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: row count, column count, object count
        if (specName == "one-cell")
            {
            return LayoutFingerprint{ 1, 1, 7 };
            }
        if (specName == "three-by-four")
            {
            return LayoutFingerprint{ 3, 4, 18 };
            }
        if (specName == "five-by-two")
            {
            return LayoutFingerprint{ 5, 2, 16 };
            }
        if (specName == "four-by-four-filled")
            {
            return LayoutFingerprint{ 4, 4, 22 };
            }
        if (specName == "two-by-eight")
            {
            return LayoutFingerprint{ 2, 8, 22 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("Table layout is deterministic and idempotent", "[table][render]")
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

TEST_CASE("Table layout matches the recorded baseline", "[table][render]")
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

TEST_CASE("Table reports the requested grid size", "[table][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_rowCount == spec.m_rows);
            CHECK(print.m_columnCount == spec.m_cols);
            CHECK(print.m_objectCount > 0);
            }
        }
    }

TEST_CASE("Table with more cells emits more render objects", "[table][render]")
    {
    ChartSpec fewCells;
    fewCells.m_rows = 2;
    fewCells.m_cols = 2;
    ChartSpec manyCells;
    manyCells.m_rows = 6;
    manyCells.m_cols = 6;

    auto* canvasA = MakeCanvas();
    const auto fewPrint = LayOutAndCapture(canvasA, BuildChart(canvasA, fewCells));
    auto* canvasB = MakeCanvas();
    const auto manyPrint = LayOutAndCapture(canvasB, BuildChart(canvasB, manyCells));

    INFO("few:  " << fewPrint.ToString());
    INFO("many: " << manyPrint.ToString());
    CHECK(manyPrint.m_objectCount > fewPrint.m_objectCount);
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("Table characterization dump", "[table][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/table_characterization.txt" };
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
