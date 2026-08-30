///////////////////////////////////////////////////////////////////////////////
// Name:        bubbleplotrendertests.cpp
// Purpose:     Characterization tests for BubblePlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of BubblePlot: left-Y and
// bottom-X axis ranges and intervals, axis slot counts, series count, and the
// number of render objects produced. BubblePlot derives from ScatterPlot and
// adds a third continuous column that sizes each point. The assertions are
// invariance based; a separate exact-value guard compares against a recorded
// baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/bubbleplot.h"
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
        size_t m_seriesCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_leftYAxis == that.m_leftYAxis && m_bottomXAxis == that.m_bottomXAxis &&
                   m_seriesCount == that.m_seriesCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "leftY" << m_leftYAxis.ToString() << " | bottomX" << m_bottomXAxis.ToString()
                 << " | series=" << m_seriesCount << " objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        std::vector<double> m_yValues{ 3, 8, 5, 11, 7, 14, 9, 6 };
        std::vector<double> m_xValues{ 1, 2, 3, 4, 5, 6, 7, 8 };
        std::vector<double> m_sizeValues{ 1, 2, 3, 4, 5, 6, 7, 8 };
        std::vector<GroupIdType> m_groupCodes;
        ColumnWithStringTable::StringTableType m_groupLabels;
        };

    // BubblePlot needs y, x, and size continuous columns, plus an optional group
    // column; none of the shared builders carry three continuous columns.
    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset(const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddContinuousColumn(L"y");
        dataset->AddContinuousColumn(L"x");
        dataset->AddContinuousColumn(L"size");
        const bool grouped{ !spec.m_groupCodes.empty() };
        if (grouped)
            {
            dataset->AddCategoricalColumn(L"group", spec.m_groupLabels);
            }
        for (size_t idx = 0; idx < spec.m_yValues.size(); ++idx)
            {
            auto row = RowInfo()
                           .Continuous({ spec.m_yValues[idx], spec.m_xValues[idx],
                                         spec.m_sizeValues[idx] })
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
    std::shared_ptr<BubblePlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<BubblePlot>(canvas);
        chart->SetData(BuildDataset(spec), L"y", L"x", L"size",
                       spec.m_groupCodes.empty() ? std::nullopt :
                                                   std::optional<wxString>{ L"group" });
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<BubblePlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_leftYAxis = CaptureAxis(chart->GetLeftYAxis());
        print.m_bottomXAxis = CaptureAxis(chart->GetBottomXAxis());
        print.m_seriesCount = chart->GetSeriesCount();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    double MinOf(const std::vector<double>& values)
        {
        return *std::min_element(values.cbegin(), values.cend());
        }

    [[nodiscard]]
    double MaxOf(const std::vector<double>& values)
        {
        return *std::max_element(values.cbegin(), values.cend());
        }

    [[nodiscard]]
    size_t DistinctGroupCount(const ChartSpec& spec)
        {
        if (spec.m_groupCodes.empty())
            {
            return 1;
            }
        std::vector<GroupIdType> codes{ spec.m_groupCodes };
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
        return codes.size();
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "linear-eight", ChartSpec{} });
        specs.push_back({ "wide-y-range",
                          ChartSpec{ .m_yValues = { 2, 90, 15, 400, 60, 720, 33, 210 },
                                     .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 },
                                     .m_sizeValues = { 5, 9, 2, 7, 4, 8, 3, 6 } } });
        specs.push_back({ "negative-quadrant",
                          ChartSpec{ .m_yValues = { -8, -3, -12, -1, -6, -15, -4, -9 },
                                     .m_xValues = { -4, -3, -2, -1, 1, 2, 3, 4 },
                                     .m_sizeValues = { 1, 2, 3, 4, 5, 6, 7, 8 } } });
        specs.push_back({ "uniform-size",
                          ChartSpec{ .m_yValues = { 3, 8, 5, 11, 7, 14, 9, 6 },
                                     .m_xValues = { 1, 2, 3, 4, 5, 6, 7, 8 },
                                     .m_sizeValues = { 4, 4, 4, 4, 4, 4, 4, 4 } } });
        specs.push_back(
            { "two-groups",
              ChartSpec{ .m_yValues = { 3, 30, 5, 33, 7, 36, 9, 39, 4, 31 },
                         .m_xValues = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 },
                         .m_sizeValues = { 2, 8, 3, 7, 4, 6, 5, 9, 1, 10 },
                         .m_groupCodes = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
                         .m_groupLabels = { { 0, L"A" }, { 1, L"B" } } } });
        return specs;
        }

    // Exact layout output recorded from the [dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to BubblePlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: left-Y axis start/end/interval/precision/reversed/points,
        //         bottom-X axis start/end/interval/precision/reversed/points,
        //         series count, object count
        if (specName == "linear-eight")
            {
            return LayoutFingerprint{ { 2, 14, 2, 0, false, 7 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 7 };
            }
        if (specName == "wide-y-range")
            {
            return LayoutFingerprint{ { 0, 800, 80, 0, false, 11 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 7 };
            }
        if (specName == "negative-quadrant")
            {
            return LayoutFingerprint{ { -16, 0, 2, 0, false, 9 },
                                      { -4, 4, 1, 0, false, 9 },
                                      1, 7 };
            }
        if (specName == "uniform-size")
            {
            return LayoutFingerprint{ { 2, 14, 2, 0, false, 7 },
                                      { 1, 8, 1, 0, false, 8 },
                                      1, 7 };
            }
        if (specName == "two-groups")
            {
            return LayoutFingerprint{ { 0, 40, 5, 0, false, 9 },
                                      { 1, 5, 1, 0, false, 5 },
                                      2, 10 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("BubblePlot layout is deterministic and idempotent", "[bubbleplot][render]")
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

TEST_CASE("BubblePlot layout matches the recorded baseline", "[bubbleplot][render]")
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

TEST_CASE("BubblePlot axes enclose the data", "[bubbleplot][render]")
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
            CHECK_FALSE(print.m_bottomXAxis.m_reversed);
            CHECK(print.m_leftYAxis.m_rangeStart <= MinOf(spec.m_yValues));
            CHECK(print.m_leftYAxis.m_rangeEnd >= MaxOf(spec.m_yValues));
            CHECK(print.m_bottomXAxis.m_rangeStart <= MinOf(spec.m_xValues));
            CHECK(print.m_bottomXAxis.m_rangeEnd >= MaxOf(spec.m_xValues));
            CHECK(print.m_leftYAxis.m_interval > 0.0);
            CHECK(print.m_bottomXAxis.m_interval > 0.0);
            }
        }
    }

TEST_CASE("BubblePlot builds one series per group", "[bubbleplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_seriesCount == DistinctGroupCount(spec));
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("BubblePlot characterization dump", "[bubbleplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/bubbleplot_characterization.txt" };
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
