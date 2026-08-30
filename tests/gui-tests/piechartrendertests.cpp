///////////////////////////////////////////////////////////////////////////////
// Name:        piechartrendertests.cpp
// Purpose:     Characterization tests for PieChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of PieChart: outer- and
// inner-ring slice counts and the number of render objects produced. PieChart is
// radial and exposes no continuous data axes, so the fingerprint is slice counts
// plus the render object count. The assertions are invariance based; a separate
// exact-value guard compares against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/piechart.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    // One drawn snapshot of everything a test can observe about a laid-out pie chart.
    struct LayoutFingerprint
        {
        size_t m_outerSliceCount{ 0 };
        size_t m_innerSliceCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_outerSliceCount == that.m_outerSliceCount &&
                   m_innerSliceCount == that.m_innerSliceCount &&
                   m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "outer=" << m_outerSliceCount << " inner=" << m_innerSliceCount
                 << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a pie chart under test.
    struct ChartSpec
        {
        ColumnWithStringTable::StringTableType m_group1Labels{ { 0, L"Alpha" },
                                                              { 1, L"Beta" },
                                                              { 2, L"Gamma" } };
        std::vector<GroupIdType> m_group1Codes{ 0, 0, 0, 1, 1, 2, 2, 2, 2 };
        std::vector<double> m_weights;
        ColumnWithStringTable::StringTableType m_group2Labels;
        std::vector<GroupIdType> m_group2Codes;
        };

    [[nodiscard]]
    std::shared_ptr<PieChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddCategoricalColumn(L"group1", spec.m_group1Labels);
        if (!spec.m_weights.empty())
            {
            dataset->AddContinuousColumn(L"weight");
            }
        if (!spec.m_group2Codes.empty())
            {
            dataset->AddCategoricalColumn(L"group2", spec.m_group2Labels);
            }

        for (size_t idx = 0; idx < spec.m_group1Codes.size(); ++idx)
            {
            RowInfo row;
            row.Id(wxString::Format(L"obs%d", static_cast<int>(idx)));
            std::vector<GroupIdType> categoricals{ spec.m_group1Codes[idx] };
            if (!spec.m_group2Codes.empty())
                {
                categoricals.push_back(spec.m_group2Codes[idx]);
                }
            row.Categoricals(categoricals);
            if (!spec.m_weights.empty())
                {
                row.Continuous({ spec.m_weights[idx] });
                }
            dataset->AddRow(row);
            }

        auto chart = std::make_shared<PieChart>(canvas);
        chart->SetData(
            dataset,
            spec.m_weights.empty() ? std::nullopt : std::optional<wxString>{ L"weight" }, L"group1",
            spec.m_group2Codes.empty() ? std::nullopt : std::optional<wxString>{ L"group2" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<PieChart>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_outerSliceCount = chart->GetOuterPie().size();
        print.m_innerSliceCount = chart->GetInnerPie().size();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    size_t DistinctOuterCount(const ChartSpec& spec)
        {
        std::set<GroupIdType> codes{ spec.m_group1Codes.cbegin(), spec.m_group1Codes.cend() };
        return codes.size();
        }

    [[nodiscard]]
    size_t DistinctPairCount(const ChartSpec& spec)
        {
        if (spec.m_group2Codes.empty())
            {
            return 0;
            }
        std::set<std::pair<GroupIdType, GroupIdType>> pairs;
        for (size_t idx = 0; idx < spec.m_group1Codes.size(); ++idx)
            {
            pairs.insert({ spec.m_group1Codes[idx], spec.m_group2Codes[idx] });
            }
        return pairs.size();
        }

    // The matrix of pie-chart recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "three-slices", ChartSpec{} });
        specs.push_back({ "weighted-three",
                          ChartSpec{ .m_weights = { 5, 5, 5, 10, 10, 2, 2, 2, 2 } } });
        specs.push_back(
            { "two-ring",
              ChartSpec{ .m_group1Codes = { 0, 0, 1, 1, 2, 2 },
                         .m_group2Labels = { { 0, L"X" }, { 1, L"Y" } },
                         .m_group2Codes = { 0, 1, 0, 1, 0, 1 } } });
        specs.push_back({ "single-slice",
                          ChartSpec{ .m_group1Labels = { { 0, L"Only" } },
                                     .m_group1Codes = { 0, 0, 0, 0 } } });
        specs.push_back(
            { "many-slices",
              ChartSpec{ .m_group1Labels = { { 0, L"C0" }, { 1, L"C1" }, { 2, L"C2" },
                                            { 3, L"C3" }, { 4, L"C4" }, { 5, L"C5" } },
                         .m_group1Codes = { 0, 1, 1, 2, 2, 2, 3, 4, 4, 5, 5, 5 } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to PieChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: outer slice count, inner slice count, object count
        if (specName == "three-slices" || specName == "weighted-three")
            {
            return LayoutFingerprint{ 3, 0, 16 };
            }
        if (specName == "two-ring")
            {
            return LayoutFingerprint{ 3, 6, 28 };
            }
        if (specName == "single-slice")
            {
            return LayoutFingerprint{ 1, 0, 8 };
            }
        if (specName == "many-slices")
            {
            return LayoutFingerprint{ 6, 0, 28 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("PieChart layout is deterministic and idempotent", "[piechart][render]")
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

// Exact-value guard: locks the layout output to what the pre-refactor code
// produced. Update ExpectedFingerprint() only for a deliberate, reviewed change
// to PieChart layout.
TEST_CASE("PieChart layout matches the recorded baseline", "[piechart][render]")
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

TEST_CASE("PieChart outer ring has one slice per top-level group", "[piechart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_outerSliceCount == DistinctOuterCount(spec));
            CHECK(print.m_innerSliceCount == DistinctPairCount(spec));
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("PieChart characterization dump", "[piechart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/piechart_characterization.txt" };
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
