///////////////////////////////////////////////////////////////////////////////
// Name:        sankeydiagramrendertests.cpp
// Purpose:     Characterization tests for SankeyDiagram layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of SankeyDiagram: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, and the number of
// render objects produced. SankeyDiagram derives from Graph2D and draws a
// two-level flow of group boxes connected by ribbons. The assertions are
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

#include "../../src/graphs/sankeydiagram.h"
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
        std::vector<wxString> m_from{ L"Male",   L"Male", L"Female",
                                     L"Female", L"Female", L"Male" };
        std::vector<wxString> m_to{ L"Pass", L"Pass", L"Fail", L"Pass", L"Pass", L"Fail" };
        std::vector<double> m_fromWeights;
        std::vector<double> m_toWeights;
        std::vector<wxString> m_fromSort;
        FlowShape m_flowShape{ FlowShape::Curvy };
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
    std::shared_ptr<SankeyDiagram> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        const bool hasWeights{ !spec.m_fromWeights.empty() };
        const bool hasSort{ !spec.m_fromSort.empty() };

        ColumnWithStringTable::StringTableType fromTable;
        ColumnWithStringTable::StringTableType toTable;
        ColumnWithStringTable::StringTableType sortTable;
        const auto fromCodes = BuildCodes(spec.m_from, fromTable);
        const auto toCodes = BuildCodes(spec.m_to, toTable);
        const auto sortCodes = hasSort ? BuildCodes(spec.m_fromSort, sortTable) :
                                         std::map<wxString, GroupIdType>{};

        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"from", fromTable);
        dataset->AddCategoricalColumn(L"to", toTable);
        if (hasSort)
            {
            dataset->AddCategoricalColumn(L"county", sortTable);
            }
        if (hasWeights)
            {
            dataset->AddContinuousColumn(L"fromWeight");
            dataset->AddContinuousColumn(L"toWeight");
            }

        for (size_t idx = 0; idx < spec.m_from.size(); ++idx)
            {
            std::vector<GroupIdType> categoricals{ fromCodes.at(spec.m_from[idx]),
                                                  toCodes.at(spec.m_to[idx]) };
            if (hasSort)
                {
                categoricals.push_back(sortCodes.at(spec.m_fromSort[idx]));
                }
            auto row = RowInfo()
                           .Categoricals(categoricals)
                           .Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
            if (hasWeights)
                {
                row.Continuous({ spec.m_fromWeights[idx], spec.m_toWeights[idx] });
                }
            dataset->AddRow(row);
            }

        auto chart = std::make_shared<SankeyDiagram>(canvas);
        chart->SetFlowShape(spec.m_flowShape);
        chart->SetData(dataset, L"from", L"to",
                       hasWeights ? std::optional<wxString>{ L"fromWeight" } : std::nullopt,
                       hasWeights ? std::optional<wxString>{ L"toWeight" } : std::nullopt,
                       hasSort ? std::optional<wxString>{ L"county" } : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<SankeyDiagram>& chart)
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
        specs.push_back({ "pass-fail", ChartSpec{} });
        specs.push_back({ "jagged", ChartSpec{ .m_flowShape = FlowShape::Jagged } });
        specs.push_back(
            { "weighted",
              ChartSpec{ .m_fromWeights = { 150, 175, 197, 60, 42, 88 },
                         .m_toWeights = { 13, 2, 0, 9, 7, 4 } } });
        specs.push_back(
            { "three-outcomes",
              ChartSpec{ .m_to = { L"Pass", L"Pass", L"Fail", L"Pass", L"Withdraw", L"Withdraw" } } });
        specs.push_back(
            { "sorted",
              ChartSpec{ .m_fromSort = { L"Berkshire", L"Franklin", L"Berkshire", L"Franklin",
                                        L"Berkshire", L"Franklin" } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to SankeyDiagram layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         object count
        // The left-Y axis is a fixed [0, 100] percent scale and the bottom-X a
        // fixed [0, 10] column scale, the same for every spec.
        const AxisFingerprint leftY{ 0, 100, 1, 0, false, 101 };
        const AxisFingerprint bottomX{ 0, 10, 1, 0, false, 11 };
        if (specName == "pass-fail" || specName == "jagged" || specName == "weighted")
            {
            return LayoutFingerprint{ leftY, bottomX, 16 };
            }
        if (specName == "three-outcomes")
            {
            return LayoutFingerprint{ leftY, bottomX, 19 };
            }
        if (specName == "sorted")
            {
            return LayoutFingerprint{ leftY, bottomX, 22 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("SankeyDiagram layout is deterministic and idempotent", "[sankeydiagram][render]")
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

TEST_CASE("SankeyDiagram layout matches the recorded baseline", "[sankeydiagram][render]")
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

TEST_CASE("SankeyDiagram emits render objects for every flow recipe", "[sankeydiagram][render]")
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
            CHECK_FALSE(print.m_bottomXAxis.m_reversed);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("SankeyDiagram characterization dump", "[sankeydiagram][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/sankeydiagram_characterization.txt" };
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
