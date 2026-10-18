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
