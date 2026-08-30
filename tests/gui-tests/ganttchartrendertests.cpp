///////////////////////////////////////////////////////////////////////////////
// Name:        ganttchartrendertests.cpp
// Purpose:     Characterization tests for GanttChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of GanttChart: scaling- (date)
// and bar-axis ranges and intervals, axis slot counts, bar count, and the number
// of render objects produced. GanttChart derives from BarChart; each task is a
// horizontal bar. The assertions are invariance based; a separate exact-value
// guard compares against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/ganttchart.h"
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

    [[nodiscard]]
    wxDateTime On(const int day, const wxDateTime::Month month, const int year)
        {
        return wxDateTime(day, month, year);
        }

    struct ChartSpec
        {
        std::vector<wxString> m_taskNames{ L"Design", L"Build", L"Ship" };
        std::vector<wxDateTime> m_startDates{ On(1, wxDateTime::Jan, 2024),
                                              On(1, wxDateTime::Mar, 2024),
                                              On(1, wxDateTime::Jun, 2024) };
        std::vector<wxDateTime> m_endDates{ On(1, wxDateTime::Mar, 2024),
                                            On(1, wxDateTime::Jun, 2024),
                                            On(1, wxDateTime::Sep, 2024) };
        DateInterval m_interval{ DateInterval::Monthly };
        };

    [[nodiscard]]
    std::shared_ptr<GanttChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        ColumnWithStringTable::StringTableType taskLabels;
        for (size_t idx = 0; idx < spec.m_taskNames.size(); ++idx)
            {
            taskLabels.insert({ static_cast<GroupIdType>(idx), spec.m_taskNames[idx] });
            }
        dataset->AddCategoricalColumn(L"task", taskLabels);
        dataset->AddDateColumn(L"start");
        dataset->AddDateColumn(L"end");
        for (size_t idx = 0; idx < spec.m_taskNames.size(); ++idx)
            {
            dataset->AddRow(RowInfo()
                                .Categoricals({ static_cast<GroupIdType>(idx) })
                                .Dates({ spec.m_startDates[idx], spec.m_endDates[idx] })
                                .Id(wxString::Format(L"row%d", static_cast<int>(idx))));
            }

        auto chart = std::make_shared<GanttChart>(canvas);
        chart->SetData(dataset, spec.m_interval, FiscalYear::Education, L"task", L"start", L"end");
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<GanttChart>& chart)
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
        specs.push_back({ "three-sequential-monthly", ChartSpec{} });
        specs.push_back(
            { "three-overlapping-weekly",
              ChartSpec{ .m_taskNames = { L"A", L"B", L"C" },
                         .m_startDates = { On(1, wxDateTime::Jan, 2024),
                                           On(8, wxDateTime::Jan, 2024),
                                           On(15, wxDateTime::Jan, 2024) },
                         .m_endDates = { On(21, wxDateTime::Jan, 2024),
                                         On(28, wxDateTime::Jan, 2024),
                                         On(4, wxDateTime::Feb, 2024) },
                         .m_interval = DateInterval::Weekly } });
        specs.push_back({ "one-task",
                          ChartSpec{ .m_taskNames = { L"Solo" },
                                     .m_startDates = { On(1, wxDateTime::Jan, 2024) },
                                     .m_endDates = { On(1, wxDateTime::Apr, 2024) } } });
        specs.push_back(
            { "five-tasks-monthly",
              ChartSpec{ .m_taskNames = { L"T1", L"T2", L"T3", L"T4", L"T5" },
                         .m_startDates = { On(1, wxDateTime::Jan, 2024),
                                           On(1, wxDateTime::Feb, 2024),
                                           On(1, wxDateTime::Mar, 2024),
                                           On(1, wxDateTime::Apr, 2024),
                                           On(1, wxDateTime::May, 2024) },
                         .m_endDates = { On(15, wxDateTime::Feb, 2024),
                                         On(15, wxDateTime::Mar, 2024),
                                         On(15, wxDateTime::Apr, 2024),
                                         On(15, wxDateTime::May, 2024),
                                         On(15, wxDateTime::Jun, 2024) } } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to GanttChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling axis start/end/interval/precision/reversed/points,
        //         bar axis start/end/interval/precision/reversed/points,
        //         bar count, object count
        if (specName == "three-sequential-monthly")
            {
            return LayoutFingerprint{ { 0, 273, 1, 0, false, 274 },
                                      { -1, 3, 1, 0, true, 5 },
                                      3, 13 };
            }
        if (specName == "three-overlapping-weekly")
            {
            return LayoutFingerprint{ { 0, 35, 1, 0, false, 36 },
                                      { -1, 3, 1, 0, true, 5 },
                                      3, 13 };
            }
        if (specName == "one-task")
            {
            return LayoutFingerprint{ { 0, 120, 1, 0, false, 121 },
                                      { -1, 1, 1, 0, true, 3 },
                                      1, 7 };
            }
        if (specName == "five-tasks-monthly")
            {
            return LayoutFingerprint{ { 0, 181, 1, 0, false, 182 },
                                      { -1, 5, 1, 0, true, 7 },
                                      5, 19 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("GanttChart layout is deterministic and idempotent", "[ganttchart][render]")
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

TEST_CASE("GanttChart layout matches the recorded baseline", "[ganttchart][render]")
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

TEST_CASE("GanttChart builds one bar per task", "[ganttchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_barCount == spec.m_taskNames.size());
            CHECK(print.m_objectCount > 0);
            CHECK_FALSE(print.m_scalingAxis.m_reversed);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("GanttChart characterization dump", "[ganttchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/ganttchart_characterization.txt" };
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
