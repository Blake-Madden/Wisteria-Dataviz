///////////////////////////////////////////////////////////////////////////////
// Name:        wafflechartrendertests.cpp
// Purpose:     Characterization tests for WaffleChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of WaffleChart: the number
// of render objects produced. WaffleChart derives from Graph2D and, unlike the
// other graphs, is built from a vector of shape definitions rather than a
// dataset; it exposes no data axes, so the fingerprint is just the render
// object count. The assertions are invariance based; a separate exact-value
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

#include "../../src/base/shapes.h"
#include "../../src/graphs/waffle_chart.h"
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
        std::vector<std::pair<Icons::IconShape, size_t>> m_cells{
            { Icons::IconShape::Square, 60 }, { Icons::IconShape::Circle, 40 }
        };
        std::optional<WaffleChart::GridRounding> m_gridRounding;
        std::optional<size_t> m_rowCount;
        };

    [[nodiscard]]
    std::shared_ptr<WaffleChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        std::vector<ShapeInfo> shapes;
        for (const auto& [shape, count] : spec.m_cells)
            {
            shapes.push_back(ShapeInfo{}.Shape(shape).Brush(*wxBLUE_BRUSH).Repeat(count));
            }
        return std::make_shared<WaffleChart>(canvas, shapes, spec.m_gridRounding, spec.m_rowCount);
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<WaffleChart>& chart)
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
        specs.push_back({ "full-grid", ChartSpec{} });
        specs.push_back({ "with-rounding",
                          ChartSpec{ .m_cells = { { Icons::IconShape::Square, 55 },
                                                  { Icons::IconShape::Circle, 40 } },
                                     .m_gridRounding = WaffleChart::GridRounding{
                                         .m_numberOfCells = size_t{ 100 },
                                         .m_shapesIndex = size_t{ 0 } } } });
        specs.push_back({ "explicit-rows", ChartSpec{ .m_rowCount = size_t{ 5 } } });
        specs.push_back({ "few-cells",
                          ChartSpec{ .m_cells = { { Icons::IconShape::Square, 10 },
                                                  { Icons::IconShape::Circle, 6 } } } });
        specs.push_back({ "single-shape",
                          ChartSpec{ .m_cells = { { Icons::IconShape::Square, 50 } } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to WaffleChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "full-grid" || specName == "with-rounding" ||
            specName == "explicit-rows")
            {
            return LayoutFingerprint{ 104 };
            }
        if (specName == "few-cells")
            {
            return LayoutFingerprint{ 20 };
            }
        if (specName == "single-shape")
            {
            return LayoutFingerprint{ 54 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("WaffleChart layout is deterministic and idempotent", "[wafflechart][render]")
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

TEST_CASE("WaffleChart layout matches the recorded baseline", "[wafflechart][render]")
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

TEST_CASE("WaffleChart emits render objects for every grid recipe", "[wafflechart][render]")
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
TEST_CASE("WaffleChart characterization dump", "[wafflechart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/wafflechart_characterization.txt" };
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
