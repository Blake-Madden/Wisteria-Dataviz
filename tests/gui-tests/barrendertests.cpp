///////////////////////////////////////////////////////////////////////////////
// Name:        barrendertests.cpp
// Purpose:     Characterization tests for BarChart layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of BarChart (axis ranges and
// intervals, bar-axis slot counts, plot rectangle, and the number of render
// objects produced). The assertions are invariance based: layout is
// checked for determinism and idempotence, and against documented structural
// invariants, rather than against hand-picked magic numbers.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/dcgraph.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/base/canvas.h"
#include "../../src/graphs/barchart.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace
    {
    constexpr int CANVAS_WIDTH{ 700 };
    constexpr int CANVAS_HEIGHT{ 500 };

    // One drawn snapshot of everything a test can observe about a laid-out chart.
    struct LayoutFingerprint
        {
        double m_scalingRangeStart{ 0 };
        double m_scalingRangeEnd{ 0 };
        double m_scalingInterval{ 0 };
        int m_scalingPrecision{ 0 };
        bool m_scalingReversed{ false };
        double m_barRangeStart{ 0 };
        double m_barRangeEnd{ 0 };
        double m_barInterval{ 0 };
        size_t m_barAxisPointCount{ 0 };
        size_t m_barCount{ 0 };
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_scalingRangeStart == that.m_scalingRangeStart &&
                   m_scalingRangeEnd == that.m_scalingRangeEnd &&
                   m_scalingInterval == that.m_scalingInterval &&
                   m_scalingPrecision == that.m_scalingPrecision &&
                   m_scalingReversed == that.m_scalingReversed &&
                   m_barRangeStart == that.m_barRangeStart &&
                   m_barRangeEnd == that.m_barRangeEnd && m_barInterval == that.m_barInterval &&
                   m_barAxisPointCount == that.m_barAxisPointCount &&
                   m_barCount == that.m_barCount && m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "scaling[" << m_scalingRangeStart << ", " << m_scalingRangeEnd
                 << "] int=" << m_scalingInterval << " prec=" << m_scalingPrecision
                 << " rev=" << (m_scalingReversed ? 1 : 0) << " | bar[" << m_barRangeStart << ", "
                 << m_barRangeEnd << "] int=" << m_barInterval << " points=" << m_barAxisPointCount
                 << " | bars=" << m_barCount << " objects=" << m_objectCount;
            return text.str();
            }
        };

    // A minimal recipe for a bar chart under test.
    struct ChartSpec
        {
        Orientation m_orientation{ Orientation::Vertical };
        std::vector<double> m_barLengths{ 3, 5, 4 };
        BoxEffect m_effect{ BoxEffect::Solid };
        BarChart::BarShape m_shape{ BarChart::BarShape::Rectangle };
        bool m_spacesBetweenBars{ false };
        // when non-empty, a single bar built from these stacked block lengths is
        // used instead of m_barLengths
        std::vector<double> m_stackedBlocks;
        };

    [[nodiscard]]
    std::shared_ptr<BarChart> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto chart = std::make_shared<BarChart>(canvas);
        chart->SetBarOrientation(spec.m_orientation);
        if (spec.m_spacesBetweenBars)
            {
            chart->IncludeSpacesBetweenBars();
            }

        const wxColour barColor{ ColorBrewer::GetColor(Color::OceanBoatBlue) };

        if (!spec.m_stackedBlocks.empty())
            {
            std::vector<BarChart::BarBlock> blocks;
            blocks.reserve(spec.m_stackedBlocks.size());
            for (const auto blockLength : spec.m_stackedBlocks)
                {
                blocks.push_back(
                    BarChart::BarBlock{ BarChart::BarBlockInfo(blockLength).Brush(barColor) });
                }
            chart->AddBar(
                BarChart::Bar(1.0, blocks, wxString{}, Label(L"Stacked"), spec.m_effect));
            }
        else
            {
            double axisPosition{ 1 };
            for (const auto barLength : spec.m_barLengths)
                {
                BarChart::Bar theBar(
                    axisPosition,
                    { BarChart::BarBlock{ BarChart::BarBlockInfo(barLength).Brush(barColor) } },
                    wxString{}, Label(wxString::Format(L"Bar %d", static_cast<int>(axisPosition))),
                    spec.m_effect);
                theBar.SetShape(spec.m_shape);
                chart->AddBar(theBar);
                axisPosition += 1;
                }
            }

        return chart;
        }

    // Lays the chart out on the canvas with an offscreen DC, exactly as the SVG
    // export path does (Canvas::CalcAllSizes), and returns a fingerprint.
    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas, const std::shared_ptr<BarChart>& chart)
        {
        wxLogNull noLog;

        canvas->SetFixedObjectsGridSize(1, 1);
        canvas->SetFixedObject(0, 0, chart);

        wxBitmap bmp{ CANVAS_WIDTH, CANVAS_HEIGHT, 32 };
        wxMemoryDC memDc{ bmp };
        wxGCDC gcdc{ memDc };
        canvas->CalcAllSizes(gcdc);

        LayoutFingerprint print;
        const auto scalingRange = chart->GetScalingAxis().GetRange();
        print.m_scalingRangeStart = scalingRange.first;
        print.m_scalingRangeEnd = scalingRange.second;
        print.m_scalingInterval = chart->GetScalingAxis().GetInterval();
        print.m_scalingPrecision = chart->GetScalingAxis().GetPrecision();
        print.m_scalingReversed = chart->GetScalingAxis().IsReversed();
        const auto barRange = chart->GetBarAxis().GetRange();
        print.m_barRangeStart = barRange.first;
        print.m_barRangeEnd = barRange.second;
        print.m_barInterval = chart->GetBarAxis().GetInterval();
        print.m_barAxisPointCount = chart->GetBarAxis().GetAxisPoints().size();
        print.m_barCount = chart->GetBars().size();
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    Canvas* MakeCanvas()
        {
        auto* canvas = new Canvas{ wxTheApp->GetTopWindow() };
        canvas->SetSize(CANVAS_WIDTH, CANVAS_HEIGHT);
        canvas->SetCanvasMinWidthDIPs(CANVAS_WIDTH);
        canvas->SetCanvasMinHeightDIPs(CANVAS_HEIGHT);
        return canvas;
        }

    [[nodiscard]]
    double MaxLength(const ChartSpec& spec)
        {
        if (!spec.m_stackedBlocks.empty())
            {
            double total{ 0 };
            for (const auto blockLength : spec.m_stackedBlocks)
                {
                total += blockLength;
                }
            return total;
            }
        double longest{ 0 };
        for (const auto barLength : spec.m_barLengths)
            {
            longest = std::max(longest, barLength);
            }
        return longest;
        }

    // The matrix of chart recipes exercised by several tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "vertical-solid-3bars", ChartSpec{} });
        specs.push_back({ "horizontal-solid-3bars",
                          ChartSpec{ .m_orientation = Orientation::Horizontal } });
        specs.push_back(
            { "vertical-one-bar", ChartSpec{ .m_barLengths = { 42 } } });
        specs.push_back({ "vertical-equal-bars",
                          ChartSpec{ .m_barLengths = { 10, 10, 10, 10 } } });
        specs.push_back({ "vertical-wide-range",
                          ChartSpec{ .m_barLengths = { 5, 6, 4, 900 } } });
        specs.push_back({ "vertical-spaces",
                          ChartSpec{ .m_barLengths = { 7, 3, 9, 5 },
                                     .m_spacesBetweenBars = true } });
        specs.push_back({ "horizontal-stacked",
                          ChartSpec{ .m_orientation = Orientation::Horizontal,
                                     .m_stackedBlocks = { 12, 8, 20 } } });
        return specs;
        }

    // Exact layout output recorded from the code before the bar-block drawing
    // refactor. Keyed by the spec name from AllSpecs(). Any change here must be a
    // deliberate, reviewed change to BarChart layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: scaling start/end/interval/precision/reversed,
        //         bar start/end/interval, bar-axis points, bar count, object count
        if (specName == "vertical-solid-3bars" || specName == "horizontal-solid-3bars")
            {
            return LayoutFingerprint{ 0, 6, 1, 0, false, 0, 4, 1, 5, 3, 11 };
            }
        if (specName == "vertical-one-bar")
            {
            return LayoutFingerprint{ 0, 50, 5, 0, false, 0, 2, 1, 3, 1, 7 };
            }
        if (specName == "vertical-equal-bars")
            {
            return LayoutFingerprint{ 0, 11, 1, 0, false, 0, 5, 1, 6, 4, 13 };
            }
        if (specName == "vertical-wide-range")
            {
            return LayoutFingerprint{ 0, 1000, 100, 0, false, 0, 5, 1, 6, 4, 13 };
            }
        if (specName == "vertical-spaces")
            {
            return LayoutFingerprint{ 0, 10, 1, 0, false, 0, 5, 1, 6, 4, 13 };
            }
        if (specName == "horizontal-stacked")
            {
            return LayoutFingerprint{ 0, 45, 5, 0, false, 0, 2, 1, 3, 1, 9 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }

    // A recipe for a bar chart whose drawn block geometry is under test.
    struct GeometrySpec
        {
        Orientation m_orientation{ Orientation::Vertical };
        std::vector<double> m_barLengths{ 3, 5, 4 };
        BoxEffect m_effect{ BoxEffect::Solid };
        BarChart::BarShape m_shape{ BarChart::BarShape::Rectangle };
        bool m_reversedScale{ false };
        std::optional<double> m_customStart;
        std::optional<double> m_customWidth;
        std::vector<double> m_stackedBlocks;
        bool m_decal{ false };
        std::optional<Icons::IconShape> m_stippleShape;
        };

    [[nodiscard]]
    std::shared_ptr<BarChart> BuildGeometryChart(Canvas* canvas, const GeometrySpec& spec)
        {
        auto chart = std::make_shared<BarChart>(canvas);
        chart->SetBarOrientation(spec.m_orientation);
        if (spec.m_stippleShape.has_value())
            {
            chart->SetStippleShape(spec.m_stippleShape.value());
            }

        const wxColour barColor{ ColorBrewer::GetColor(Color::OceanBoatBlue) };

        const auto makeBlock = [&spec, &barColor](const double blockLength)
        {
            BarChart::BarBlockInfo blockInfo{ blockLength };
            blockInfo.Brush(barColor);
            if (spec.m_decal)
                {
                blockInfo.Decal(Label{ L"Decal" });
                }
            return BarChart::BarBlock{ blockInfo };
        };

        const auto applyBarOptions = [&spec](BarChart::Bar& theBar)
        {
            theBar.SetShape(spec.m_shape);
            if (spec.m_customStart.has_value())
                {
                theBar.SetCustomScalingAxisStartPosition(spec.m_customStart);
                }
            if (spec.m_customWidth.has_value())
                {
                theBar.SetCustomWidth(spec.m_customWidth);
                }
        };

        if (!spec.m_stackedBlocks.empty())
            {
            std::vector<BarChart::BarBlock> blocks;
            blocks.reserve(spec.m_stackedBlocks.size());
            for (const auto blockLength : spec.m_stackedBlocks)
                {
                blocks.push_back(makeBlock(blockLength));
                }
            BarChart::Bar theBar(1.0, blocks, wxString{}, Label(L"Stacked"), spec.m_effect);
            applyBarOptions(theBar);
            chart->AddBar(theBar);
            }
        else
            {
            double axisPosition{ 1 };
            for (const auto barLength : spec.m_barLengths)
                {
                BarChart::Bar theBar(
                    axisPosition, { makeBlock(barLength) }, wxString{},
                    Label(wxString::Format(L"Bar %d", static_cast<int>(axisPosition))),
                    spec.m_effect);
                applyBarOptions(theBar);
                chart->AddBar(theBar);
                axisPosition += 1;
                }
            }

        // reverse after the bars are added, since AddBar() sets the axis ranges
        if (spec.m_reversedScale)
            {
            chart->GetScalingAxis().Reverse(true);
            }

        return chart;
        }

    // Lays the chart out and describes each render object: its bounding box, plus
    // the vertices of any polygon. A bounding box alone cannot tell a rectangular
    // block from an arrow, since an arrowhead reaches the rectangle on every side.
    [[nodiscard]]
    std::vector<std::string> LayOutAndCaptureGeometry(Canvas* canvas,
                                                      const std::shared_ptr<BarChart>& chart)
        {
        wxLogNull noLog;

        canvas->SetFixedObjectsGridSize(1, 1);
        canvas->SetFixedObject(0, 0, chart);

        wxBitmap bmp{ CANVAS_WIDTH, CANVAS_HEIGHT, 32 };
        wxMemoryDC memDc{ bmp };
        wxGCDC gcdc{ memDc };
        canvas->CalcAllSizes(gcdc);

        std::vector<std::string> descriptions;
        descriptions.reserve(chart->GetObjectCount());
        for (const auto& object : chart->GetObjects())
            {
            const wxRect box = object->GetBoundingBox(gcdc);
            std::ostringstream text;
            text << '(' << box.GetX() << ',' << box.GetY() << ',' << box.GetWidth() << ','
                 << box.GetHeight() << ')';
            if (const auto* polygon =
                    dynamic_cast<const Wisteria::GraphItems::Polygon*>(object.get());
                polygon != nullptr)
                {
                text << "pts{";
                for (const auto& point : polygon->GetPoints())
                    {
                    text << ' ' << point.x << ',' << point.y;
                    }
                text << " }";
                }
            descriptions.push_back(text.str());
            }
        return descriptions;
        }

    [[nodiscard]]
    std::string GeometryToString(const std::vector<std::string>& descriptions)
        {
        std::ostringstream text;
        text << descriptions.size() << " objects:";
        for (const auto& description : descriptions)
            {
            text << ' ' << description;
            }
        return text.str();
        }

    // The matrix of geometry recipes exercised by the block-geometry tests below.
    [[nodiscard]]
    std::vector<std::pair<std::string, GeometrySpec>> GeometrySpecs()
        {
        std::vector<std::pair<std::string, GeometrySpec>> specs;
        // the four start-point branches (reversed scale x custom start position),
        // in both orientations
        specs.push_back({ "vert-plain", GeometrySpec{} });
        specs.push_back(
            { "horz-plain", GeometrySpec{ .m_orientation = Orientation::Horizontal } });
        specs.push_back({ "vert-reversed", GeometrySpec{ .m_reversedScale = true } });
        specs.push_back({ "horz-reversed",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_reversedScale = true } });
        specs.push_back({ "vert-custom-start", GeometrySpec{ .m_customStart = 2.0 } });
        specs.push_back({ "horz-custom-start",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_customStart = 2.0 } });
        specs.push_back({ "vert-reversed-custom-start",
                          GeometrySpec{ .m_reversedScale = true, .m_customStart = 2.0 } });
        specs.push_back({ "horz-reversed-custom-start",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_reversedScale = true, .m_customStart = 2.0 } });
        // stacked blocks exercise the running axis offset and the first-block nudge
        specs.push_back({ "vert-stacked", GeometrySpec{ .m_stackedBlocks = { 12, 8, 20 } } });
        specs.push_back({ "horz-stacked",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_stackedBlocks = { 12, 8, 20 } } });
        specs.push_back({ "vert-stacked-reversed",
                          GeometrySpec{ .m_reversedScale = true,
                                        .m_stackedBlocks = { 12, 8, 20 } } });
        specs.push_back({ "horz-stacked-reversed",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_reversedScale = true,
                                        .m_stackedBlocks = { 12, 8, 20 } } });
        // the arrow shapes deflate the block rectangle into a neck
        specs.push_back({ "vert-arrow", GeometrySpec{ .m_shape = BarChart::BarShape::Arrow } });
        specs.push_back({ "horz-arrow",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_shape = BarChart::BarShape::Arrow } });
        specs.push_back(
            { "vert-reverse-arrow", GeometrySpec{ .m_shape = BarChart::BarShape::ReverseArrow } });
        specs.push_back({ "horz-reverse-arrow",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_shape = BarChart::BarShape::ReverseArrow } });
        // custom widths take the other branch of the bar-width calculation
        specs.push_back({ "vert-custom-width", GeometrySpec{ .m_customWidth = 0.5 } });
        specs.push_back({ "horz-custom-width",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_customWidth = 0.5 } });
        // the stipple-shape effect tiles icons across the block rectangle
        specs.push_back({ "vert-stipple-shape", GeometrySpec{ .m_effect = BoxEffect::StippleShape } });
        specs.push_back({ "horz-stipple-shape",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_effect = BoxEffect::StippleShape } });
        // icons whose drawing area is narrowed or shortened rather than square
        specs.push_back({ "vert-stipple-woman",
                          GeometrySpec{ .m_effect = BoxEffect::StippleShape,
                                        .m_stippleShape = Icons::IconShape::Woman } });
        specs.push_back({ "horz-stipple-woman",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_effect = BoxEffect::StippleShape,
                                        .m_stippleShape = Icons::IconShape::Woman } });
        specs.push_back({ "vert-stipple-blackboard",
                          GeometrySpec{ .m_effect = BoxEffect::StippleShape,
                                        .m_stippleShape = Icons::IconShape::Blackboard } });
        specs.push_back({ "horz-stipple-blackboard",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_effect = BoxEffect::StippleShape,
                                        .m_stippleShape = Icons::IconShape::Blackboard } });
        // decals are placed from the (possibly deflated) block rectangle
        specs.push_back({ "vert-decal", GeometrySpec{ .m_decal = true } });
        specs.push_back({ "horz-decal", GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                                      .m_decal = true } });
        specs.push_back({ "vert-arrow-decal",
                          GeometrySpec{ .m_shape = BarChart::BarShape::Arrow, .m_decal = true } });
        specs.push_back({ "horz-arrow-decal",
                          GeometrySpec{ .m_orientation = Orientation::Horizontal,
                                        .m_shape = BarChart::BarShape::Arrow,
                                        .m_decal = true } });
        return specs;
        }
    } // namespace

TEST_CASE("BarChart layout is deterministic and idempotent", "[barchart][render]")
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
// produced. Complements the invariance checks above. Update ExpectedFingerprint()
// only for a deliberate, reviewed change to BarChart layout.
TEST_CASE("BarChart layout matches the recorded baseline", "[barchart][render]")
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

TEST_CASE("BarChart scaling axis encloses every bar", "[barchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK_FALSE(print.m_scalingReversed);
            CHECK(print.m_scalingRangeStart == 0.0);
            CHECK(print.m_scalingRangeEnd >= MaxLength(spec));
            CHECK(print.m_scalingInterval > 0.0);
            }
        }
    }

TEST_CASE("BarChart bar axis has a slot for every bar", "[barchart][render]")
    {
    SECTION("evenly spaced single-block bars")
        {
        for (const size_t barCount : { size_t{ 1 }, size_t{ 3 }, size_t{ 6 } })
            {
            auto* canvas = MakeCanvas();
            ChartSpec spec;
            spec.m_barLengths.assign(barCount, 10.0);
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("barCount=" << barCount << " fingerprint: " << print.ToString());
            CHECK(print.m_barCount == barCount);
            // one axis point under each bar, plus the two outer (unlabeled) points
            CHECK(print.m_barAxisPointCount == barCount + 2);
            }
        }
    }

TEST_CASE("BarChart emits at least one render object per visible bar", "[barchart][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_objectCount >= print.m_barCount);
            CHECK(print.m_barAxisPointCount >= print.m_barCount + 2);
            }
        }
    }

TEST_CASE("BarChart lays out every box effect and bar shape stably", "[barchart][render]")
    {
    const std::array<BoxEffect, 11> effects{ BoxEffect::Solid,
                                             BoxEffect::Glassy,
                                             BoxEffect::FadeFromBottomToTop,
                                             BoxEffect::FadeFromTopToBottom,
                                             BoxEffect::CommonImage,
                                             BoxEffect::Image,
                                             BoxEffect::StippleImage,
                                             BoxEffect::StippleShape,
                                             BoxEffect::WaterColor,
                                             BoxEffect::Marker,
                                             BoxEffect::Pencil };
    const std::array<BarChart::BarShape, 3> shapes{ BarChart::BarShape::Rectangle,
                                                    BarChart::BarShape::Arrow,
                                                    BarChart::BarShape::ReverseArrow };
    for (const auto orientation : { Orientation::Vertical, Orientation::Horizontal })
        {
        for (const auto effect : effects)
            {
            for (const auto shape : shapes)
                {
                ChartSpec spec;
                spec.m_orientation = orientation;
                spec.m_barLengths = { 8, 14, 5, 11 };
                spec.m_effect = effect;
                spec.m_shape = shape;

                auto* canvas = MakeCanvas();
                auto chart = BuildChart(canvas, spec);
                const auto firstPass = LayOutAndCapture(canvas, chart);
                const auto secondPass = LayOutAndCapture(canvas, chart);

                INFO("orientation=" << (orientation == Orientation::Vertical ? "V" : "H")
                                    << " effect=" << static_cast<int>(effect)
                                    << " shape=" << static_cast<int>(shape)
                                    << " fingerprint: " << firstPass.ToString());
                CHECK(secondPass == firstPass);
                CHECK(firstPass.m_objectCount > 0);
                CHECK(firstPass.m_scalingRangeEnd >= 14.0);
                }
            }
        }
    }

TEST_CASE("BarChart spacing toggle keeps axis ranges and bar count", "[barchart][render]")
    {
    ChartSpec noSpaces;
    noSpaces.m_barLengths = { 7, 3, 9, 5 };
    ChartSpec withSpaces{ noSpaces };
    withSpaces.m_spacesBetweenBars = true;

    auto* canvasA = MakeCanvas();
    const auto plainPrint = LayOutAndCapture(canvasA, BuildChart(canvasA, noSpaces));
    auto* canvasB = MakeCanvas();
    const auto spacedPrint = LayOutAndCapture(canvasB, BuildChart(canvasB, withSpaces));

    INFO("plain:  " << plainPrint.ToString());
    INFO("spaced: " << spacedPrint.ToString());
    CHECK(spacedPrint.m_scalingRangeStart == plainPrint.m_scalingRangeStart);
    CHECK(spacedPrint.m_scalingRangeEnd == plainPrint.m_scalingRangeEnd);
    CHECK(spacedPrint.m_barRangeStart == plainPrint.m_barRangeStart);
    CHECK(spacedPrint.m_barRangeEnd == plainPrint.m_barRangeEnd);
    CHECK(spacedPrint.m_barCount == plainPrint.m_barCount);
    CHECK(spacedPrint.m_objectCount == plainPrint.m_objectCount);
    }

TEST_CASE("BarChart stacked blocks add render objects", "[barchart][render]")
    {
    ChartSpec oneBlock;
    oneBlock.m_orientation = Orientation::Horizontal;
    oneBlock.m_stackedBlocks = { 40 };
    ChartSpec threeBlocks{ oneBlock };
    threeBlocks.m_stackedBlocks = { 12, 8, 20 };

    auto* canvasA = MakeCanvas();
    const auto onePrint = LayOutAndCapture(canvasA, BuildChart(canvasA, oneBlock));
    auto* canvasB = MakeCanvas();
    const auto threePrint = LayOutAndCapture(canvasB, BuildChart(canvasB, threeBlocks));

    INFO("one block:    " << onePrint.ToString());
    INFO("three blocks: " << threePrint.ToString());
    CHECK(onePrint.m_barCount == 1);
    CHECK(threePrint.m_barCount == 1);
    CHECK(threePrint.m_objectCount > onePrint.m_objectCount);
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand as an extra safety check.
TEST_CASE("BarChart characterization dump", "[barchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/barrender_characterization.txt" };
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

TEST_CASE("BarChart block geometry is deterministic", "[barchart][render]")
    {
    for (const auto& [name, spec] : GeometrySpecs())
        {
        SECTION(name)
            {
            auto* canvasA = MakeCanvas();
            const auto firstPass =
                LayOutAndCaptureGeometry(canvasA, BuildGeometryChart(canvasA, spec));

            auto* canvasB = MakeCanvas();
            const auto secondPass =
                LayOutAndCaptureGeometry(canvasB, BuildGeometryChart(canvasB, spec));

            INFO("geometry: " << GeometryToString(firstPass));
            CHECK_FALSE(firstPass.empty());
            CHECK(firstPass == secondPass);
            }
        }
    }

TEST_CASE("BarChart block geometry is idempotent across relayouts", "[barchart][render]")
    {
    for (const auto& [name, spec] : GeometrySpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildGeometryChart(canvas, spec);
            const auto firstPass = LayOutAndCaptureGeometry(canvas, chart);
            const auto secondPass = LayOutAndCaptureGeometry(canvas, chart);

            INFO("geometry: " << GeometryToString(firstPass));
            CHECK(firstPass == secondPass);
            }
        }
    }

// Best-effort human-readable dump of the block geometry, written next to the test
// executable. Not an assertion; it exists so the pre-refactor and post-refactor
// output can be diffed by hand, and so the recorded baseline below can be filled in.
TEST_CASE("BarChart block geometry dump", "[barchart][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/bargeometry_characterization.txt" };
    wxFileOutputStream fileStream{ outPath };
    if (!fileStream.IsOk())
        {
        WARN("could not open " << outPath.ToStdString());
        return;
        }
    wxTextOutputStream textStream{ fileStream };

    for (const auto& [name, spec] : GeometrySpecs())
        {
        auto* canvas = MakeCanvas();
        const auto boxes = LayOutAndCaptureGeometry(canvas, BuildGeometryChart(canvas, spec));
        textStream << wxString::Format(L"%s\t%s\n", wxString::FromUTF8(name),
                                      wxString::FromUTF8(GeometryToString(boxes)));
        }
    SUCCEED("wrote " << outPath.ToStdString());
    }
