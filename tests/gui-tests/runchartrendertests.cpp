///////////////////////////////////////////////////////////////////////////////
// Name:        runchartrendertests.cpp
// Purpose:     Tests for RunChart sequence layout and median line
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests check that RunChart places observations at their position in the
// sequence (1..N), computes the median while ignoring missing values, and keeps
// exactly one median reference line across repeated SetData() calls.

#include "../../src/graphs/runchart.h"
#include "graphrenderharness.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace wisteria_render_tests;

constexpr static double MISSING{ std::numeric_limits<double>::quiet_NaN() };

static std::shared_ptr<RunChart> BuildRunChart(Canvas* canvas, const std::vector<double>& values)
    {
    auto chart = std::make_shared<RunChart>(canvas);
    chart->SetData(MakeContinuousDataset(L"value", values), L"value", std::nullopt);
    return chart;
    }

static std::shared_ptr<Dataset> MakeLabeledDataset(const std::vector<double>& values)
    {
    auto dataset = std::make_shared<Dataset>();
    dataset->AddContinuousColumn(L"value");
    dataset->AddCategoricalColumn(L"batch", { { 0, L"Mon" }, { 1, L"Tue" }, { 2, L"Wed" } });
    for (size_t idx = 0; idx < values.size(); ++idx)
        {
        dataset->AddRow(RowInfo()
                            .Continuous({ values[idx] })
                            .Categoricals({ static_cast<GroupIdType>(idx % 3) })
                            .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
        }
    return dataset;
    }

TEST_CASE("RunChart median ignores missing values", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();

    SECTION("odd count")
        {
        const auto chart = BuildRunChart(canvas, { 7, 1, 5 });
        REQUIRE(chart->GetMedian().has_value());
        CHECK(chart->GetMedian().value() == 5.0);
        }
    SECTION("even count averages the middle pair")
        {
        const auto chart = BuildRunChart(canvas, { 1, 3, 5, 9 });
        REQUIRE(chart->GetMedian().has_value());
        CHECK(chart->GetMedian().value() == 4.0);
        }
    SECTION("missing values are skipped")
        {
        const auto chart = BuildRunChart(canvas, { 5, MISSING, 1, 3, 9 });
        REQUIRE(chart->GetMedian().has_value());
        CHECK(chart->GetMedian().value() == 4.0);
        }
    SECTION("all missing has no median")
        {
        const auto chart = BuildRunChart(canvas, { MISSING, MISSING });
        CHECK_FALSE(chart->GetMedian().has_value());
        CHECK(chart->GetReferenceLines().empty());
        }
    }

TEST_CASE("RunChart x-axis is the observation sequence", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    const auto chart = BuildRunChart(canvas, { 4, 8, 6, MISSING, 5, 7 });
    LayOutOffscreen(canvas, chart);

    const auto xAxis = CaptureAxis(chart->GetBottomXAxis());
    CHECK(xAxis.m_rangeStart == 1.0);
    CHECK(xAxis.m_rangeEnd == 6.0);
    CHECK(chart->GetLineCount() == 1);
    }

TEST_CASE("RunChart gives constant data a Y range", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();

    SECTION("single row")
        {
        const auto chart = BuildRunChart(canvas, { 5 });
        LayOutOffscreen(canvas, chart);
        const auto yAxis = CaptureAxis(chart->GetLeftYAxis());
        CHECK(yAxis.m_rangeStart < 5.0);
        CHECK(yAxis.m_rangeEnd > 5.0);
        }
    SECTION("identical values")
        {
        const auto chart = BuildRunChart(canvas, { 5, 5, 5, 5 });
        LayOutOffscreen(canvas, chart);
        const auto yAxis = CaptureAxis(chart->GetLeftYAxis());
        CHECK(yAxis.m_rangeStart < 5.0);
        CHECK(yAxis.m_rangeEnd > 5.0);
        }
    SECTION("identical zeros")
        {
        const auto chart = BuildRunChart(canvas, { 0, 0, 0 });
        LayOutOffscreen(canvas, chart);
        const auto yAxis = CaptureAxis(chart->GetLeftYAxis());
        CHECK(yAxis.m_rangeStart < 0.0);
        CHECK(yAxis.m_rangeEnd > 0.0);
        }
    }

TEST_CASE("RunChart labels do not change the spacing", "[runchart][render]")
    {
    auto* plainCanvas = MakeCanvas();
    const auto plain = BuildRunChart(plainCanvas, { 4, 8, 6, 5, 7 });
    LayOutOffscreen(plainCanvas, plain);

    auto* labeledCanvas = MakeCanvas();
    auto labeled = std::make_shared<RunChart>(labeledCanvas);
    labeled->SetData(MakeLabeledDataset({ 4, 8, 6, 5, 7 }), L"value",
                     std::optional<wxString>{ L"batch" });
    LayOutOffscreen(labeledCanvas, labeled);

    CHECK(CaptureAxis(labeled->GetBottomXAxis()) == CaptureAxis(plain->GetBottomXAxis()));
    }

TEST_CASE("RunChart keeps a single median reference line", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = BuildRunChart(canvas, { 4, 8, 6, 5, 7 });
    CHECK(chart->GetReferenceLines().size() == 1);

    chart->SetData(MakeContinuousDataset(L"value", { 1, 2, 3 }), L"value", std::nullopt);
    REQUIRE(chart->GetReferenceLines().size() == 1);
    CHECK(chart->GetReferenceLines().front().GetAxisPosition() == 2.0);

    chart->ShowMedianLine(false);
    CHECK(chart->GetReferenceLines().empty());

    chart->ShowMedianLine(true);
    CHECK(chart->GetReferenceLines().size() == 1);
    }

TEST_CASE("RunChart layout is idempotent", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    const auto chart = BuildRunChart(canvas, { 4, 8, 6, MISSING, 5, 7, 9, 3 });

    LayOutOffscreen(canvas, chart);
    const auto firstY = CaptureAxis(chart->GetLeftYAxis());
    const auto firstX = CaptureAxis(chart->GetBottomXAxis());
    const auto firstObjects = chart->GetObjectCount();

    LayOutOffscreen(canvas, chart);
    CHECK(CaptureAxis(chart->GetLeftYAxis()) == firstY);
    CHECK(CaptureAxis(chart->GetBottomXAxis()) == firstX);
    CHECK(chart->GetObjectCount() == firstObjects);
    }

static std::shared_ptr<Dataset> MakeSampleDataset(const std::vector<double>& values,
                                                  const std::vector<GroupIdType>& sampleCodes)
    {
    auto dataset = std::make_shared<Dataset>();
    dataset->AddContinuousColumn(L"value");
    dataset->AddCategoricalColumn(L"sample", { { 0, L"A" }, { 1, L"B" }, { 2, L"C" } });
    for (size_t idx = 0; idx < values.size(); ++idx)
        {
        dataset->AddRow(RowInfo()
                            .Continuous({ values[idx] })
                            .Categoricals({ sampleCodes[idx] })
                            .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
        }
    return dataset;
    }

TEST_CASE("RunChart plots one point per sample", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);

    SECTION("fixed size with mean")
        {
        chart->SetSampleData(MakeContinuousDataset(L"value", { 1, 2, 9, 4, 5, 6 }), L"value", 3);
        REQUIRE(chart->GetMedian().has_value());
        // sample means are 4 and 5
        CHECK(chart->GetMedian().value() == 4.5);
        }
    SECTION("fixed size with median")
        {
        chart->SetSampleData(MakeContinuousDataset(L"value", { 1, 2, 9, 4, 5, 6 }), L"value", 3,
                             std::nullopt, SampleStatistic::Median);
        REQUIRE(chart->GetMedian().has_value());
        // sample medians are 2 and 5
        CHECK(chart->GetMedian().value() == 3.5);
        }
    SECTION("last fixed-size sample can be smaller")
        {
        chart->SetSampleData(MakeContinuousDataset(L"value", { 2, 4, 6, 8, 10 }), L"value", 2);
        LayOutOffscreen(canvas, chart);
        // samples are (2,4), (6,8), and (10)
        CHECK(CaptureAxis(chart->GetBottomXAxis()).m_rangeEnd == 3.0);
        }
    SECTION("zero sample size is rejected")
        {
        CHECK_THROWS_AS(
            chart->SetSampleData(MakeContinuousDataset(L"value", { 1, 2, 3 }), L"value", 0),
            std::runtime_error);
        }
    }

TEST_CASE("RunChart sample IDs define adjacent runs", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);

    // A, A, B, B, A, A is three samples because the second A run is not adjacent to the first
    chart->SetSampleData(MakeSampleDataset({ 1, 3, 10, 14, 5, 7 }, { 0, 0, 1, 1, 0, 0 }), L"value",
                         L"sample");
    LayOutOffscreen(canvas, chart);

    CHECK(CaptureAxis(chart->GetBottomXAxis()).m_rangeEnd == 3.0);
    REQUIRE(chart->GetMedian().has_value());
    // sample means are 2, 12, and 6
    CHECK(chart->GetMedian().value() == 6.0);
    }

TEST_CASE("RunChart numeric sample IDs must be whole numbers", "[runchart][render]")
    {
    const auto makeDataset = [](const std::vector<double>& sampleIds)
    {
        auto dataset = std::make_shared<Dataset>();
        dataset->AddContinuousColumn(L"value");
        dataset->AddContinuousColumn(L"sample");
        for (size_t idx = 0; idx < sampleIds.size(); ++idx)
            {
            dataset->AddRow(RowInfo()
                                .Continuous({ static_cast<double>(idx), sampleIds[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
    };

    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);

    SECTION("whole numbers define adjacent runs")
        {
        chart->SetSampleData(makeDataset({ 1, 1, 2, 2, 3, 3 }), L"value", L"sample");
        LayOutOffscreen(canvas, chart);
        CHECK(CaptureAxis(chart->GetBottomXAxis()).m_rangeEnd == 3.0);
        }
    SECTION("a fractional ID is rejected")
        {
        CHECK_THROWS_AS(chart->SetSampleData(makeDataset({ 1, 1, 2.5, 2.5 }), L"value", L"sample"),
                        std::runtime_error);
        }
    }

TEST_CASE("RunChart sample gaps and missing values", "[runchart][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);

    // second sample is entirely missing, third has one missing value
    chart->SetSampleData(
        MakeSampleDataset({ 2, 4, MISSING, MISSING, 6, MISSING }, { 0, 0, 1, 1, 2, 2 }), L"value",
        L"sample");

    REQUIRE(chart->GetMedian().has_value());
    // sample means are 3, (gap), and 6
    CHECK(chart->GetMedian().value() == 4.5);
    }

TEST_CASE("RunChart can overlay individual observations", "[runchart][render]")
    {
    const auto dataset = MakeContinuousDataset(L"value", { 1, 2, 9, 4, 5, 6 });

    auto* hiddenCanvas = MakeCanvas();
    auto hidden = std::make_shared<RunChart>(hiddenCanvas);
    hidden->SetSampleData(dataset, L"value", 3);
    LayOutOffscreen(hiddenCanvas, hidden);

    auto* shownCanvas = MakeCanvas();
    auto shown = std::make_shared<RunChart>(shownCanvas);
    shown->SetSampleData(dataset, L"value", 3);
    shown->ShowIndividualPoints(true);
    LayOutOffscreen(shownCanvas, shown);

    // the overlay adds one set of points, and the axes are unchanged
    CHECK(shown->GetObjectCount() == hidden->GetObjectCount() + 1);
    CHECK(CaptureAxis(shown->GetBottomXAxis()) == CaptureAxis(hidden->GetBottomXAxis()));

    // the y-axis grows to include the observations (1 through 9),
    // while the sample means (4 and 5) alone need a much smaller range
    const auto shownY = CaptureAxis(shown->GetLeftYAxis());
    CHECK(shownY.m_rangeStart <= 1.0);
    CHECK(shownY.m_rangeEnd >= 9.0);
    CHECK(CaptureAxis(hidden->GetLeftYAxis()).m_rangeEnd < 9.0);
    }

TEST_CASE("RunChart does not overlay without samples", "[runchart][render]")
    {
    const auto dataset = MakeContinuousDataset(L"value", { 1, 2, 9, 4, 5, 6 });

    auto* hiddenCanvas = MakeCanvas();
    auto hidden = std::make_shared<RunChart>(hiddenCanvas);
    hidden->SetData(dataset, L"value", std::nullopt);
    LayOutOffscreen(hiddenCanvas, hidden);

    auto* shownCanvas = MakeCanvas();
    auto shown = std::make_shared<RunChart>(shownCanvas);
    shown->SetData(dataset, L"value", std::nullopt);
    shown->ShowIndividualPoints(true);
    LayOutOffscreen(shownCanvas, shown);

    CHECK(shown->GetObjectCount() == hidden->GetObjectCount());
    }

TEST_CASE("RunChart samples use the first row's label", "[runchart][render]")
    {
    auto* plainCanvas = MakeCanvas();
    auto plain = std::make_shared<RunChart>(plainCanvas);
    plain->SetSampleData(MakeSampleDataset({ 1, 3, 10, 14 }, { 0, 0, 1, 1 }), L"value", L"sample");
    LayOutOffscreen(plainCanvas, plain);

    auto* labeledCanvas = MakeCanvas();
    auto labeled = std::make_shared<RunChart>(labeledCanvas);
    labeled->SetSampleData(MakeSampleDataset({ 1, 3, 10, 14 }, { 0, 0, 1, 1 }), L"value", L"sample",
                           std::optional<wxString>{ L"sample" });
    LayOutOffscreen(labeledCanvas, labeled);

    CHECK(CaptureAxis(labeled->GetBottomXAxis()) == CaptureAxis(plain->GetBottomXAxis()));
    }

TEST_CASE("RunChart rules find patterns about the median", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    // median is 4.5, so the first four points are below it and the last four above
    const auto chart = BuildRunChart(canvas, { 1, 2, 3, 4, 5, 6, 7, 8 });
    CHECK(chart->GetRuleViolations().empty());

    chart->EnableRule(statistics::spc_rule::trend, true);
    REQUIRE(chart->GetRuleViolations().size() == 1);
    CHECK(chart->GetRuleViolations()[0].rule == statistics::spc_rule::trend);
    CHECK(chart->GetRuleViolations()[0].points.size() == 8);

    // four points on each side is not a shift of 6, but is a shift of 4
    chart->EnableRule(statistics::spc_rule::shift, true);
    CHECK(chart->GetRuleViolations().size() == 1);
    chart->SetShiftLength(4);
    CHECK(chart->GetRuleViolations().size() == 3);

    chart->EnableRule(statistics::spc_rule::trend, false);
    chart->EnableRule(statistics::spc_rule::shift, false);
    CHECK(chart->GetRuleViolations().empty());

    CHECK_THROWS(chart->SetTrendLength(2));
    CHECK_THROWS(chart->SetAlternatingLength(2));
    CHECK_THROWS(chart->SetShiftLength(1));
    CHECK(chart->GetTrendLength() == 5);
    CHECK(chart->GetAlternatingLength() == 14);
    CHECK(chart->GetShiftLength() == 4);
    }

TEST_CASE("RunChart rules are re-evaluated for new data", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    auto chart = BuildRunChart(canvas, { 1, 2, 3, 4, 5 });
    chart->EnableRule(statistics::spc_rule::trend, true);
    CHECK(chart->GetRuleViolations().size() == 1);

    chart->SetData(MakeContinuousDataset(L"value", { 1, 3, 2, 4, 3 }), L"value", std::nullopt);
    CHECK(chart->GetRuleViolations().empty());

    chart->SetData(nullptr, L"value", std::nullopt);
    CHECK(chart->GetRuleViolations().empty());
    CHECK_FALSE(chart->GetRunsSummary().has_value());
    }

TEST_CASE("RunChart rules apply to sample statistics", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);
    // the raw values zigzag, but the sample means (2, 4, 6, 8, 10) rise steadily
    chart->SetSampleData(MakeContinuousDataset(L"value", { 3, 1, 5, 3, 7, 5, 9, 7, 11, 9 }),
                         L"value", 2);
    chart->EnableRule(statistics::spc_rule::trend, true);
    REQUIRE(chart->GetRuleViolations().size() == 1);
    CHECK(chart->GetRuleViolations()[0].points == std::vector<size_t>{ 0, 1, 2, 3, 4 });
    }

TEST_CASE("RunChart runs summary", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    // ten points below the median and ten above, in two runs
    std::vector<double> values(10, 1.0);
    values.insert(values.end(), 10, 9.0);
    const auto chart = BuildRunChart(canvas, values);
    CHECK_FALSE(chart->GetRunsSummary().has_value());

    chart->EnableRule(statistics::spc_rule::runs, true);
    REQUIRE(chart->GetRunsSummary().has_value());
    CHECK(chart->GetRunsSummary()->runs == 2);
    CHECK(chart->GetRunsSummary()->has_too_few_runs());
    // the runs test does not circle any points
    CHECK(chart->GetRuleViolations().empty());
    CHECK(chart->GetRunsNote().Contains(L"Too few runs"));

    // the results are shown in the legend, and the caption is left alone
    chart->GetCaption().SetText(L"My caption");
    // without the median line, the runs note is the only thing in the legend
    chart->ShowMedianLine(false);
    const auto legend =
        chart->CreateLegend(LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText().Contains(L"Too few runs"));
    // the runs note has no icon, so the legend is not indented for one
    CHECK(legend->GetLegendIcons().empty());
    CHECK(legend->GetLeftPadding() == 0);
    LayOutOffscreen(canvas, chart);
    CHECK(chart->GetCaption().GetText() == L"My caption");
    }

TEST_CASE("RunChart legend explains the circles", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    const auto chart = BuildRunChart(canvas, { 1, 2, 3, 4, 5, 6, 7, 8 });
    const auto options = LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph);

    // only the median
    auto legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText() == L"Median: 4.5");
    REQUIRE(legend->GetLegendIcons().size() == 1);
    CHECK(legend->GetLegendIcons().front().m_shape == Icons::IconShape::HorizontalLine);

    // nothing to show
    chart->ShowMedianLine(false);
    CHECK(chart->CreateLegend(options) == nullptr);

    // the rules come first, then the median in its own section
    chart->ShowMedianLine(true);
    chart->EnableRule(statistics::spc_rule::trend, true);
    legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText() == L"Trend (5 or more points rising or falling)\n \nMedian: 4.5");
    REQUIRE(legend->GetLegendIcons().size() == 3);
    CHECK(legend->GetLegendIcons()[0].m_shape == Icons::IconShape::Circle);
    CHECK(legend->GetLegendIcons()[1].m_shape == Icons::IconShape::HorizontalSeparator);
    CHECK(legend->GetLegendIcons()[2].m_shape == Icons::IconShape::HorizontalLine);

    // the median and the end of the baseline share a section, before the runs results
    chart->SetBaselineLength(4);
    chart->EnableRule(statistics::spc_rule::runs, true);
    legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText().starts_with(L"Trend (5 or more points rising or falling)\n \n"
                                        "Median: 2.5\nEnd of baseline (first 4 observations)"
                                        "\n \nRuns about the median"));
    REQUIRE(legend->GetLegendIcons().size() == 5);
    CHECK(legend->GetLegendIcons()[0].m_shape == Icons::IconShape::Circle);
    CHECK(legend->GetLegendIcons()[1].m_shape == Icons::IconShape::HorizontalSeparator);
    CHECK(legend->GetLegendIcons()[2].m_shape == Icons::IconShape::HorizontalLine);
    CHECK(legend->GetLegendIcons()[3].m_shape == Icons::IconShape::VerticalLine);
    CHECK(legend->GetLegendIcons()[4].m_shape == Icons::IconShape::HorizontalSeparator);
    }

TEST_CASE("RunChart rules each have their own color", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    // six points below the median, then six rising points above it
    const auto chart = BuildRunChart(canvas, { 1, 1, 1, 1, 1, 1, 5, 6, 7, 8, 9, 10 });
    chart->EnableRule(statistics::spc_rule::shift, true);
    chart->EnableRule(statistics::spc_rule::trend, true);
    // leave the median out of the legend, so that only the rules are listed
    chart->ShowMedianLine(false);

    CHECK(chart->GetRuleColor(statistics::spc_rule::shift) ==
          SpcChart::GetDefaultRuleColor(statistics::spc_rule::shift));
    CHECK(SpcChart::GetDefaultRuleColor(statistics::spc_rule::shift) !=
          SpcChart::GetDefaultRuleColor(statistics::spc_rule::trend));
    CHECK(SpcChart::GetDefaultRuleColor(statistics::spc_rule::trend) !=
          SpcChart::GetDefaultRuleColor(statistics::spc_rule::alternating));

    const auto options = LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph);
    auto legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText() == L"Shift (6 or more points on one side)\n"
                               "Trend (5 or more points rising or falling)");
    REQUIRE(legend->GetLegendIcons().size() == 2);
    // the rings are translucent so that overlapping rings can be seen
    const wxColour shiftIconColor{ legend->GetLegendIcons()[0].m_pen.GetColour() };
    const wxColour shiftColor{ chart->GetRuleColor(statistics::spc_rule::shift) };
    CHECK(shiftIconColor.GetRGB() == shiftColor.GetRGB());
    CHECK(shiftIconColor.Alpha() < wxALPHA_OPAQUE);
    CHECK(legend->GetLegendIcons()[1].m_pen.GetColour().GetRGB() ==
          chart->GetRuleColor(statistics::spc_rule::trend).GetRGB());

    // a custom color is used for that rule only
    chart->SetRuleColor(statistics::spc_rule::trend, *wxBLUE);
    legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetLegendIcons()[0].m_pen.GetColour().GetRGB() == shiftColor.GetRGB());
    CHECK(legend->GetLegendIcons()[1].m_pen.GetColour().GetRGB() == wxBLUE->GetRGB());

    // a rule without a color is not circled or listed
    chart->SetRuleColor(statistics::spc_rule::trend, wxNullColour);
    legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText() == L"Shift (6 or more points on one side)");
    CHECK(legend->GetLegendIcons().size() == 1);
    }

TEST_CASE("RunChart legend includes reference lines", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    const auto chart = BuildRunChart(canvas, { 1, 2, 3, 4, 5, 6, 7, 8 });
    const auto options = LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph);
    chart->AddReferenceLine(GraphItems::ReferenceLine(
        AxisType::LeftYAxis, 7, L"Goal", wxPen{ *wxBLUE, 2 }, ReferenceLabelPlacement::Legend));
    // leave the median out of the legend, so that the reference lines can be checked alone
    chart->ShowMedianLine(false);

    SECTION("Only reference lines")
        {
        const auto legend = chart->CreateLegend(options);
        REQUIRE(legend != nullptr);
        CHECK(legend->GetText() == L"Goal");
        REQUIRE(legend->GetLegendIcons().size() == 1);
        CHECK(legend->GetLegendIcons().front().m_shape == Icons::IconShape::HorizontalLine);
        }
    SECTION("Reference lines after the circles")
        {
        chart->EnableRule(statistics::spc_rule::trend, true);
        const auto legend = chart->CreateLegend(options);
        REQUIRE(legend != nullptr);
        CHECK(legend->GetText().starts_with(L"Trend ("));
        CHECK(legend->GetText().ends_with(L"Goal"));
        REQUIRE(legend->GetLegendIcons().size() == 3);
        CHECK(legend->GetLegendIcons()[0].m_shape == Icons::IconShape::Circle);
        CHECK(legend->GetLegendIcons()[1].m_shape == Icons::IconShape::HorizontalSeparator);
        CHECK(legend->GetLegendIcons()[2].m_shape == Icons::IconShape::HorizontalLine);
        }
    SECTION("Header comes before everything")
        {
        const auto legend =
            chart->CreateLegend(LegendOptions{}.IncludeHeader(true).Title(L"Key").PlacementHint(
                LegendCanvasPlacementHint::RightOfGraph));
        REQUIRE(legend != nullptr);
        CHECK(legend->GetText() == L"Key\nGoal");
        }
    }

TEST_CASE("RunChart runs note is always shown", "[runchart][render][rules]")
    {
    auto* canvas = MakeCanvas();
    // a random-looking sequence, with no signal
    const auto chart =
        BuildRunChart(canvas, { 1, 9, 9, 1, 1, 1, 9, 1, 9, 9, 1, 9, 1, 1, 9, 9, 9, 1, 9, 1 });
    CHECK(chart->GetRunsNote().empty());

    chart->EnableRule(statistics::spc_rule::runs, true);
    REQUIRE(chart->GetRunsSummary().has_value());
    CHECK_FALSE(chart->GetRunsSummary()->has_too_few_runs());
    CHECK_FALSE(chart->GetRunsSummary()->has_too_many_runs());
    CHECK(chart->GetRunsNote().Contains(L"Runs about the median"));
    CHECK_FALSE(chart->GetRunsNote().Contains(L"Too"));

    // too few points to test still says so
    chart->SetData(MakeContinuousDataset(L"value", { 1, 9, 1, 9 }), L"value", std::nullopt);
    CHECK_FALSE(chart->GetRunsSummary().has_value());
    CHECK(chart->GetRunsNote().Contains(L"too few points"));
    }

TEST_CASE("RunChart baseline freezes the median", "[runchart][render][baseline]")
    {
    auto* canvas = MakeCanvas();
    const auto chart = BuildRunChart(canvas, { 1, 2, 3, 4, 5, 10, 11, 12, 13, 14 });
    REQUIRE(chart->GetMedian().has_value());
    CHECK(chart->GetMedian().value() == 7.5);
    chart->EnableRule(statistics::spc_rule::shift, true);
    CHECK(chart->GetRuleViolations().empty());

    // the median comes from the first five points, and the rules use it for all points
    chart->SetBaselineLength(5);
    CHECK(chart->GetBaselineLength() == 5);
    REQUIRE(chart->GetMedian().has_value());
    CHECK(chart->GetMedian().value() == 3.0);
    REQUIRE(chart->GetRuleViolations().size() == 1);
    CHECK(chart->GetRuleViolations()[0].points ==
          std::vector<size_t>{ 3, 4, 5, 6, 7, 8, 9 });

    const auto options = LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph);
    const auto legend = chart->CreateLegend(options);
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText().Contains(L"End of baseline (first 5 observations)"));
    CHECK(std::ranges::any_of(legend->GetLegendIcons(), [](const auto& icon)
                              { return icon.m_shape == Icons::IconShape::VerticalLine; }));

    // a baseline covering every point is the same as no baseline
    chart->SetBaselineLength(10);
    CHECK(chart->GetMedian().value() == 7.5);
    chart->SetBaselineLength(0);
    CHECK(chart->GetMedian().value() == 7.5);
    CHECK(chart->GetRuleViolations().empty());
    }

TEST_CASE("RunChart baseline draws a divider", "[runchart][render][baseline]")
    {
    const auto dataset = MakeContinuousDataset(L"value", { 1, 2, 3, 4, 5, 10, 11, 12, 13, 14 });

    auto* plainCanvas = MakeCanvas();
    auto plain = std::make_shared<RunChart>(plainCanvas);
    plain->SetData(dataset, L"value", std::nullopt);
    LayOutOffscreen(plainCanvas, plain);

    auto* frozenCanvas = MakeCanvas();
    auto frozen = std::make_shared<RunChart>(frozenCanvas);
    frozen->SetData(dataset, L"value", std::nullopt);
    frozen->SetBaselineLength(5);
    LayOutOffscreen(frozenCanvas, frozen);
    CHECK(frozen->GetObjectCount() == plain->GetObjectCount() + 1);

    // the baseline is kept for new data
    frozen->SetData(MakeContinuousDataset(L"value", { 9, 9, 1, 1, 1, 1 }), L"value",
                    std::nullopt);
    CHECK(frozen->GetBaselineLength() == 5);
    REQUIRE(frozen->GetMedian().has_value());
    CHECK(frozen->GetMedian().value() == 1.0);
    }

TEST_CASE("RunChart baseline counts samples", "[runchart][render][baseline]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<RunChart>(canvas);
    // sample means are 2, 4, 6, 8, 10
    chart->SetSampleData(MakeContinuousDataset(L"value", { 1, 3, 3, 5, 5, 7, 7, 9, 9, 11 }),
                         L"value", 2);
    chart->SetBaselineLength(3);
    REQUIRE(chart->GetMedian().has_value());
    CHECK(chart->GetMedian().value() == 4.0);

    const auto legend = chart->CreateLegend(
        LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
    REQUIRE(legend != nullptr);
    CHECK(legend->GetText().Contains(L"End of baseline (first 3 samples)"));
    }

TEST_CASE("RunChart circles rule violations", "[runchart][render][rules]")
    {
    const auto dataset = MakeContinuousDataset(L"value", { 1, 2, 3, 4, 5, 6, 7, 8 });

    auto* plainCanvas = MakeCanvas();
    auto plain = std::make_shared<RunChart>(plainCanvas);
    plain->SetData(dataset, L"value", std::nullopt);
    LayOutOffscreen(plainCanvas, plain);

    auto* flaggedCanvas = MakeCanvas();
    auto flagged = std::make_shared<RunChart>(flaggedCanvas);
    flagged->SetData(dataset, L"value", std::nullopt);
    flagged->EnableRule(statistics::spc_rule::trend, true);
    LayOutOffscreen(flaggedCanvas, flagged);

    // the circles are one set of points, and the axes are unchanged
    CHECK(flagged->GetObjectCount() == plain->GetObjectCount() + 1);
    CHECK(CaptureAxis(flagged->GetLeftYAxis()) == CaptureAxis(plain->GetLeftYAxis()));

    // an enabled rule that finds nothing draws nothing
    auto* quietCanvas = MakeCanvas();
    auto quiet = std::make_shared<RunChart>(quietCanvas);
    quiet->SetData(dataset, L"value", std::nullopt);
    quiet->EnableRule(statistics::spc_rule::alternating, true);
    LayOutOffscreen(quietCanvas, quiet);
    CHECK(quiet->GetObjectCount() == plain->GetObjectCount());
    }
