///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_art.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "image.h"
#include "label.h"
#include "lines.h"
#include "shapes.h"
#include <wx/dcgraph.h>
#include <wx/graphics.h>

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    void ShapeRenderer::DrawThickWaterColorRectangle(const wxRect rect, wxDC& dc) const
        {
        DrawWaterColorRectangle(rect, dc);
        // paint a second coat
        DrawWaterColorRectangle(rect, dc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawWaterColorRectangle(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for water color effect!");
        if (gc != nullptr)
            {
            const auto strayLinesAlongTopBottom =
                std::max<size_t>(safe_divide<int>(rect.GetWidth(), ScaleToScreenAndCanvas(100)), 1);
            const auto strayLinesAlongLeftRight = std::max<size_t>(
                safe_divide<int>(rect.GetHeight(), ScaleToScreenAndCanvas(100)), 1);

            // get the min percent of the height needed, which is the lesser of 3 DIPs or 33%
            const auto heightMinDIPsPercent =
                std::min(safe_divide<double>(ScaleToScreenAndCanvas(3), rect.GetHeight()),
                         math_constants::third);
            // ...and use the larger between that or 10 DIPs (or 20%) are the height
            const auto wiggleTopBottom =
                std::max(std::min(safe_divide<double>(ScaleToScreenAndCanvas(10), rect.GetHeight()),
                                  math_constants::twentieth),
                         heightMinDIPsPercent);
            std::uniform_real_distribution<> wiggleDistroTopBottom(-wiggleTopBottom,
                                                                   wiggleTopBottom);

            const auto widthMinDIPsPercent =
                std::min(safe_divide<double>(ScaleToScreenAndCanvas(3), rect.GetWidth()),
                         math_constants::third);
            const auto wiggleLeftRight =
                std::max(std::min(safe_divide<double>(ScaleToScreenAndCanvas(10), rect.GetWidth()),
                                  math_constants::twentieth),
                         widthMinDIPsPercent);
            std::uniform_real_distribution<> wiggleDistroLeftRight(-wiggleLeftRight,
                                                                   wiggleLeftRight);

            // "watercolor" fill of rectangle
            gc->SetPen(wxColour{ 0, 0, 0, 0 });
            wxBrush br{ GetGraphItemInfo().GetBrush() };
            // make the brush translucent (if not already so) to make it a watercolor brush
            if (br.GetColour().IsOpaque())
                {
                br.SetColour(Colors::ColorContrast::ChangeOpacity(
                    br.GetColour(), Settings::GetTranslucencyValue()));
                }
            gc->SetBrush(br);
            auto fillPath = gc->CreatePath();

            // top
            //----
            fillPath.MoveToPoint(GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, 0)); // top left
            // "outside the lines" points along the top
            double previousXPos{ 0.0 };
            for (size_t i = 1; i <= strayLinesAlongTopBottom; ++i)
                {
                auto xPos =
                    safe_divide<double>(math_constants::full, strayLinesAlongTopBottom + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect,
                                    previousXPos + safe_divide<double>(xPos - previousXPos, 2)),
                    GetYPosFromTop(rect, wiggleDistroTopBottom(GetRNG())),
                    GetXPosFromLeft(rect, xPos),
                    GetYPosFromTop(rect, wiggleDistroTopBottom(GetRNG())));
                previousXPos = xPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect, wiggleDistroTopBottom(GetRNG())),
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect, wiggleDistroTopBottom(GetRNG()))); // top right

            // right
            //------
            double previousYPos{ 0.0 };
            for (size_t i = 1; i <= strayLinesAlongLeftRight; ++i)
                {
                auto yPos =
                    safe_divide<double>(math_constants::full, strayLinesAlongLeftRight + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                    GetYPosFromTop(rect,
                                   previousYPos + safe_divide<double>(yPos - previousYPos, 2)),
                    GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                    GetYPosFromTop(rect, yPos));
                previousYPos = yPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(GetRNG())),
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect,
                               math_constants::full +
                                   wiggleDistroTopBottom(GetRNG()))); // bottom right

            // bottom
            //-------
            // "outside the lines" points along the bottom
            previousXPos = math_constants::full;
            for (long i = static_cast<long>(strayLinesAlongTopBottom); i > 0; --i)
                {
                const auto xPos =
                    safe_divide<double>(math_constants::full, strayLinesAlongTopBottom + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, xPos + safe_divide<double>(previousXPos - xPos, 2)),
                    GetYPosFromTop(rect, math_constants::full - wiggleDistroTopBottom(GetRNG())),
                    GetXPosFromLeft(rect, xPos),
                    GetYPosFromTop(rect, math_constants::full - wiggleDistroTopBottom(GetRNG())));
                previousXPos = xPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(GetRNG())),
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(GetRNG())),
                GetYPosFromTop(rect,
                               math_constants::full +
                                   wiggleDistroTopBottom(GetRNG()))); // bottom left

            // left
            //-----
            previousYPos = math_constants::full;
            for (long i = static_cast<long>(strayLinesAlongLeftRight); i > 0; --i)
                {
                auto yPos =
                    safe_divide<double>(math_constants::full, strayLinesAlongLeftRight + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, wiggleDistroLeftRight(GetRNG())),
                    GetYPosFromTop(rect, yPos + safe_divide<double>(previousYPos - yPos, 2)),
                    GetXPosFromLeft(rect, wiggleDistroLeftRight(GetRNG())),
                    GetYPosFromTop(rect, yPos));
                previousYPos = yPos;
                }
            fillPath.AddQuadCurveToPoint(GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(GetRNG())),
                                         GetYPosFromTop(rect, 0 + wiggleDistroTopBottom(GetRNG())),
                                         GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(GetRNG())),
                                         GetYPosFromTop(rect, 0 + wiggleDistroTopBottom(GetRNG())));

            fillPath.CloseSubpath();
            gc->FillPath(fillPath);
            gc->StrokePath(fillPath);

            // draw the hard outline on top
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);
                gc->SetBrush(wxColour{ 0, 0, 0, 0 });

                gc->DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
                }
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawMarkerRectangle(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for marker effect!");
        if (gc != nullptr)
            {
            const wxColour markerColor = GetGraphItemInfo().GetBrush().IsOk() ?
                                             GetGraphItemInfo().GetBrush().GetColour() :
                                             *wxBLACK;

            // perturbation ranges for the marker wobble, proportional to each dimension
            const auto wiggleH =
                std::max(safe_divide<double>(ScaleToScreenAndCanvas(2), rect.GetWidth()), 0.005);
            const auto wiggleV =
                std::max(safe_divide<double>(ScaleToScreenAndCanvas(2), rect.GetHeight()), 0.005);
            std::uniform_real_distribution<> wiggleDistroH(-wiggleH, wiggleH);
            std::uniform_real_distribution<> wiggleDistroV(-wiggleV, wiggleV);

            // Use the shorter side of the box (the base width) for marker sizing.
            // This keeps all bars with the same base width looking consistent.
            const auto baseWidth = std::min(rect.GetWidth(), rect.GetHeight());
            const auto markerWidth = std::max<double>(baseWidth * math_constants::tenth, 1.0);

            // draw diagonal hatching lines (top-left to bottom-right direction)
            gc->SetBrush(wxColour{ 0, 0, 0, 0 });
            const wxPen hatchPen(
                wxPenInfo{ markerColor, static_cast<int>(markerWidth) }.Cap(wxPenCap::wxCAP_BUTT));

            const auto hatchSpacing = std::max(baseWidth * 0.35, 4.0);
            // Sweep a diagonal line from top-left to bottom-right.
            // The line sweeps across the rectangle via an offset value.
            const int totalSweep = rect.GetWidth() + rect.GetHeight();
            for (int offset = hatchSpacing; offset < totalSweep; offset += hatchSpacing)
                {
                // calculate unclipped start/end of the diagonal line
                double x1 = rect.GetX() + offset;
                auto y1 = static_cast<double>(rect.GetY());
                auto x2 = static_cast<double>(rect.GetX());
                double y2 = rect.GetY() + offset;

                // clip to rectangle bounds
                if (x1 > rect.GetRight())
                    {
                    y1 += (x1 - rect.GetRight());
                    x1 = rect.GetRight();
                    }
                if (y2 > rect.GetBottom())
                    {
                    x2 += (y2 - rect.GetBottom());
                    y2 = rect.GetBottom();
                    }
                if (y1 > rect.GetBottom() || x2 > rect.GetRight())
                    {
                    continue;
                    }

                // draw the line as a wobbly path with a few segments
                auto hatchPath = gc->CreatePath();
                constexpr int SEGMENTS{ 4 };
                const auto dx = safe_divide<double>(x2 - x1, SEGMENTS);
                const auto dy = safe_divide<double>(y2 - y1, SEGMENTS);

                hatchPath.MoveToPoint(x1, y1);
                for (int segment = 1; segment <= SEGMENTS; ++segment)
                    {
                    const double wx = rect.GetWidth() * wiggleDistroH(GetRNG());
                    const double wy = rect.GetHeight() * wiggleDistroV(GetRNG());
                    hatchPath.AddLineToPoint(x1 + dx * segment + wx, y1 + dy * segment + wy);
                    }

                gc->SetPen(hatchPen);
                gc->StrokePath(hatchPath);
                }

            // draw rough marker outline
            wxPen outlinePen(markerColor, markerWidth);
            outlinePen.SetCap(wxPenCap::wxCAP_BUTT);
            gc->SetPen(outlinePen);
            gc->SetBrush(wxColour{ 0, 0, 0, 0 });

            const size_t edgeSegments =
                std::max<size_t>(safe_divide<int>(std::max(rect.GetWidth(), rect.GetHeight()),
                                                  ScaleToScreenAndCanvas(20)),
                                 3);

            auto outlinePath = gc->CreatePath();

            // top edge
            outlinePath.MoveToPoint(rect.GetX(), rect.GetY());
            for (size_t i = 1; i <= edgeSegments; ++i)
                {
                const auto tip = safe_divide<double>(i, edgeSegments);
                outlinePath.AddLineToPoint(rect.GetX() + rect.GetWidth() * tip,
                                           rect.GetY() +
                                               rect.GetHeight() * wiggleDistroV(GetRNG()));
                }
            // right edge
            for (size_t i = 1; i <= edgeSegments; ++i)
                {
                const auto tip = safe_divide<double>(i, edgeSegments);
                outlinePath.AddLineToPoint(rect.GetRight() +
                                               rect.GetWidth() * wiggleDistroH(GetRNG()),
                                           rect.GetY() + rect.GetHeight() * tip);
                }
            // bottom edge
            for (size_t i = 1; i <= edgeSegments; ++i)
                {
                const auto tip = 1.0 - safe_divide<double>(i, edgeSegments);
                outlinePath.AddLineToPoint(rect.GetX() + rect.GetWidth() * tip,
                                           rect.GetBottom() +
                                               rect.GetHeight() * wiggleDistroV(GetRNG()));
                }
            // left edge
            for (size_t i = 1; i <= edgeSegments; ++i)
                {
                const double tip = 1.0 - safe_divide<double>(i, edgeSegments);
                outlinePath.AddLineToPoint(rect.GetX() + rect.GetWidth() * wiggleDistroH(GetRNG()),
                                           rect.GetY() + rect.GetHeight() * tip);
                }

            outlinePath.CloseSubpath();
            gc->StrokePath(outlinePath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPencilRectangle(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        // get the pencil pen for color reference
        wxPen pencilPen = GetGraphItemInfo().GetPen();
        if (!pencilPen.IsOk() || pencilPen.GetColour().IsTransparent())
            {
            pencilPen = *wxBLACK_PEN;
            }
        const wxColour pencilColor = pencilPen.GetColour();
        const auto penWidth = std::max(ScaleToScreenAndCanvas(pencilPen.GetWidth()), 1.0);

        // create a lighter, translucent shade of the pencil color for shading
        const wxColour shadingColor(pencilColor.Red(), pencilColor.Green(), pencilColor.Blue(),
                                    80); // ~30% opacity

        // random distributions for imperfections
        std::uniform_real_distribution<> overflowDist(-penWidth * 2, penWidth * 4);
        std::uniform_real_distribution<> skipDist(0.0, 1.0);
        std::uniform_real_distribution<> gapDist(0.8, 1.5); // random spacing multiplier
        std::uniform_real_distribution<> wiggleDist(-penWidth * 0.5, penWidth * 0.5);

        // use the shorter side for sizing, like Marker effect
        const auto baseWidth = std::min(rect.GetWidth(), rect.GetHeight());
        const auto strokeWidth = std::max<double>(penWidth * 0.8, 1.0);

        // draw diagonal hatching lines (top-left to bottom-right direction)
        gc->SetBrush(wxColour{ 0, 0, 0, 0 });
        wxPen shadingPen(shadingColor, static_cast<int>(strokeWidth));
        shadingPen.SetCap(wxPenCap::wxCAP_ROUND);

        const auto baseSpacing = std::max(baseWidth * 0.03, penWidth * 0.5);
        const int totalSweep = rect.GetWidth() + rect.GetHeight();

        double offset = baseSpacing * gapDist(GetRNG());
        while (offset < totalSweep)
            {
            // randomly skip some strokes to create gaps (about 15% chance)
            if (skipDist(GetRNG()) < 0.15)
                {
                offset += baseSpacing * gapDist(GetRNG());
                continue;
                }

            // calculate unclipped start/end of the diagonal line
            double x1 = rect.GetX() + offset;
            auto y1 = static_cast<double>(rect.GetY());
            auto x2 = static_cast<double>(rect.GetX());
            double y2 = rect.GetY() + offset;

            // add random overflow/underflow at the ends
            const double startOverflow = overflowDist(GetRNG());
            const double endOverflow = overflowDist(GetRNG());

            // apply overflow before clipping (can extend outside)
            x1 += startOverflow * 0.707; // cos(45°)
            y1 -= startOverflow * 0.707; // extend diagonally outward
            x2 -= endOverflow * 0.707;
            y2 += endOverflow * 0.707;

            // clip to rectangle bounds (with some tolerance for overflow)
            const double clipMargin{ penWidth * 3 };
            if (x1 > rect.GetRight() + clipMargin)
                {
                const double excess = x1 - rect.GetRight();
                y1 += excess;
                x1 = rect.GetRight() + std::min(startOverflow, clipMargin);
                }
            if (y2 > rect.GetBottom() + clipMargin)
                {
                const double excess = y2 - rect.GetBottom();
                x2 += excess;
                y2 = rect.GetBottom() + std::min(endOverflow, clipMargin);
                }
            if (y1 > rect.GetBottom() + clipMargin || x2 > rect.GetRight() + clipMargin)
                {
                offset += baseSpacing * gapDist(GetRNG());
                continue;
                }

            // draw the line as a wobbly path with a few segments
            auto hatchPath = gc->CreatePath();
            constexpr int SEGMENTS{ 5 };
            const auto dx = safe_divide<double>(x2 - x1, SEGMENTS);
            const auto dy = safe_divide<double>(y2 - y1, SEGMENTS);

            hatchPath.MoveToPoint(x1, y1);
            for (int segment = 1; segment <= SEGMENTS; ++segment)
                {
                const double wx = wiggleDist(GetRNG());
                const double wy = wiggleDist(GetRNG());
                hatchPath.AddLineToPoint(x1 + dx * segment + wx, y1 + dy * segment + wy);
                }

            gc->SetPen(shadingPen);
            gc->StrokePath(hatchPath);

            offset += baseSpacing * gapDist(GetRNG());
            }

        // draw outline with pencil style
        pencilPen.SetWidth(penWidth);
        gc->SetBrush(wxNullBrush);

        auto outlinePath = gc->CreatePath();
        outlinePath.MoveToPoint(rect.GetTopLeft());
        outlinePath.AddLineToPoint(rect.GetTopRight());
        outlinePath.AddLineToPoint(rect.GetBottomRight());
        outlinePath.AddLineToPoint(rect.GetBottomLeft());
        outlinePath.AddLineToPoint(rect.GetTopLeft());
        outlinePath.CloseSubpath();
        gc->StrokePath(outlinePath);
        }
    } // namespace Wisteria::GraphItems
