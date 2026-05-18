///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_nature.cpp
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
    void ShapeRenderer::DrawBaseFlower(const wxRect rect, wxDC& dc, const wxColour& foregroundColor,
                                       const wxColour& backgroundColor) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        wxGraphicsContext* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        wxRect drawRect{ rect };
        const int pad = static_cast<int>(
            std::floor(std::min(drawRect.GetWidth(), drawRect.GetHeight()) * 0.03));
        drawRect.Deflate(pad, pad);

        const double centerX = drawRect.GetX() + (drawRect.GetWidth() * math_constants::half);
        const double centerY = drawRect.GetY() + (drawRect.GetHeight() * math_constants::half);
        const double radius =
            std::min(drawRect.GetWidth(), drawRect.GetHeight()) * math_constants::half;

        const wxColour foregroundColorWarm{ Colors::ColorContrast::Tint(foregroundColor, 0.48) };
        const wxColour darkBackgroundColor{ Colors::ColorContrast::Shade(backgroundColor, 0.40) };
        const wxColour receptacleColor{ 70, 50, 35 };

        // Core + overlap (tuck petals under to avoid fringe)
        const double coreR = radius * 0.30;
        const double overlap = std::min(radius * 0.15, ScaleToScreenAndCanvas(4.0));

        // Petal geometry
        constexpr int PETAL_COUNT{ 10 };       // fuller look
        const double innerLen = radius * 0.70; // ellipse height (radial)
        const double innerWid = radius * 0.22; // ellipse width  (tangential)
        const double innerCtr = coreR + (innerLen * math_constants::half) - overlap;

        // Back ring: slightly longer/slimmer, but clamp tip == front tip
        const double outerLen = innerLen * 1.08;
        const double outerWid = innerWid * 0.92;
        const double innerTip = innerCtr + (innerLen * math_constants::half);
        const double outerCtr =
            innerTip - (outerLen * math_constants::half); // makes outer tip == inner tip

        // Helpers
        const auto clampU8 = [](int val)
        { return static_cast<wxColourBase::ChannelType>(std::clamp(val, 0, 255)); };

        const auto outlineFrom = [&](const wxColour& col)
        {
            const wxColour outlineColor{ clampU8(col.Red() - 60), clampU8(col.Green() - 60),
                                         clampU8(col.Blue() - 60), clampU8(170) };

            wxPen outlinePen{ outlineColor, std::max<int>(1, static_cast<int>(radius * 0.015)) };
            outlinePen.SetJoin(wxJOIN_ROUND);
            outlinePen.SetCap(wxCAP_ROUND);
            return outlinePen;
        };

        // Draw one ring of outward-pointing elliptical petals.
        const auto drawPetalRing = [&](auto colorAtIndex, const wxPen& outline, const int count,
                                       const double petalWidth, const double petalLen,
                                       const double centerRadius, const double rotationOffsetDeg,
                                       const bool drawVeins)
        {
            // Ring-scope sandbox
            gc->PushState();

            const int veinWidth = std::max<int>(1, static_cast<int>(radius * 0.012));
            wxPen veinPen{ wxColour(darkBackgroundColor.Red(), darkBackgroundColor.Green(),
                                    darkBackgroundColor.Blue(), 50),
                           veinWidth };
            veinPen.SetCap(wxCAP_ROUND);

            for (int i = 0; i < count; ++i)
                {
                const double angleDeg = rotationOffsetDeg + (safe_divide<double>(360.0, count) * i);
                const double angleRad = geometry::degrees_to_radians(angleDeg);

                const double px = centerX + (centerRadius * std::cos(angleRad));
                const double py = centerY + (centerRadius * std::sin(angleRad));

                // Per-petal sandbox
                gc->PushState();
                gc->Translate(px, py);
                // Long axis outward (radial)
                gc->Rotate(angleRad + (std::numbers::pi / 2.0));

                const wxColour petalColor = colorAtIndex(i);

                const wxColour baseShade{ clampU8(petalColor.Red() - 18),
                                          clampU8(petalColor.Green() - 18),
                                          clampU8(petalColor.Blue() - 18) };

                // base -> tip gradient
                const auto grad = gc->CreateLinearGradientBrush(
                    0, petalLen * math_constants::half,  // base (near core)
                    0, -petalLen * math_constants::half, // tip
                    baseShade, petalColor);

                gc->SetBrush(grad);
                gc->SetPen(outline);

                // petal ellipse (centered in this local space)
                gc->DrawEllipse(-petalWidth * math_constants::half,
                                -petalLen * math_constants::half, petalWidth, petalLen);

                // two thin, non-touching vein curves with slight deterministic jitter
                if (drawVeins)
                    {
                    // stroke-only setup and opaque pen
                    gc->SetBrush(*wxTRANSPARENT_BRUSH);

                    const double h = petalLen;
                    const double w = petalWidth;
                    const double y0 = h * 0.45;  // start near base (+Y)
                    const double y1 = -h * 0.05; // end near mid

                    // small index-based jitter (no RNG state)
                    const double jitterLeft = ((i * 37) % 7 - 3) * (w * 0.01);
                    const double jitterRight = ((i * 53) % 7 - 3) * (w * 0.01);

                    const auto crease = [&](double x0, double x1, double bulge)
                    {
                        // set pen right before stroke; reset after to avoid leaks
                        gc->SetPen(veinPen);

                        wxGraphicsPath path = gc->CreatePath();
                        path.MoveToPoint(x0, y0);
                        path.AddCurveToPoint(x0 + bulge, y0 - (h * math_constants::quarter),
                                             x1 + (bulge * math_constants::half),
                                             y1 + (h * math_constants::tenth), x1, y1);
                        gc->StrokePath(path);
                    };

                    // left and right creases — thin, separated, slightly different curves
                    crease((-w * 0.17) + jitterLeft, (-w * 0.06) + jitterLeft,
                           -w * math_constants::tenth);
                    crease((w * 0.17) + jitterRight, (w * 0.06) + jitterRight,
                           w * math_constants::tenth);
                    }

                gc->PopState();
                }

            gc->PopState();
        };

        // back ring first (backgroundColor), tips clamped to match front tips
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        drawPetalRing([&](int) { return backgroundColor; }, outlineFrom(backgroundColor),
                      PETAL_COUNT, outerWid, outerLen, outerCtr, 0.0,
                      rect.GetWidth() > ScaleToScreenAndCanvas(18));

        // front ring (alternate foregroundColor / warm-foregroundColor), staggered half-step
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        drawPetalRing([&](int i) { return ((i & 1) != 0) ? foregroundColorWarm : foregroundColor; },
                      outlineFrom(foregroundColor), PETAL_COUNT, innerWid, innerLen, innerCtr,
                      safe_divide(180, PETAL_COUNT), rect.GetWidth() > ScaleToScreenAndCanvas(18));

            // center disk (radial gradient)
            {
            const wxColour coreLite{ clampU8(receptacleColor.Red() + 35),
                                     clampU8(receptacleColor.Green() + 35),
                                     clampU8(receptacleColor.Blue() + 35) };

            gc->SetPen(*wxTRANSPARENT_PEN);
            const auto coreGrad = gc->CreateRadialGradientBrush(centerX, centerY, centerX, centerY,
                                                                coreR, coreLite, receptacleColor);
            gc->SetBrush(coreGrad);
            gc->DrawEllipse(centerX - coreR, centerY - coreR, coreR * 2.0, coreR * 2.0);
            }

        // seeds
        if (rect.GetWidth() > ScaleToScreenAndCanvas(16))
            {
            const double maxSeedR = coreR * 0.92;
            const int nSeeds = std::clamp<int>(static_cast<int>(radius * 0.7), 70, 170);
            const double golden = std::numbers::pi * (3.0 - std::sqrt(5.0));
            const double dotR = std::max(1.0, ScaleToScreenAndCanvas(0.7));

            const wxBrush seedA{ wxColour{ 120, 90, 70 } };
            const wxBrush seedB{ wxColour{ 170, 135, 110 } };

            gc->SetPen(*wxTRANSPARENT_PEN);

            for (int i = 0; i < nSeeds; ++i)
                {
                const auto t = safe_divide<double>(i + 0.5, nSeeds);
                const double rs = std::sqrt(t) * maxSeedR;
                const double th = i * golden;

                const double px = centerX + (rs * std::cos(th));
                const double py = centerY + (rs * std::sin(th));

                gc->SetBrush(((i & 1) != 0) ? seedA : seedB);
                gc->DrawEllipse(px - dotR, py - dotR, dotR * 2.0, dotR * 2.0);
                }
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawButterfly(const wxRect rect, wxDC& dc) const
        {
        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        gc->PushState();
        gc->Translate(rect.x, rect.y);

        const wxRect2DDouble drawRect(0, 0, rect.width, rect.height);
        const double centerX = drawRect.m_width / 2.0;
        const double centerY = drawRect.m_height / 2.0;

        const wxColour frameColor = ApplyColorOpacity(*wxBLACK);
        const wxColour bodyColor = *wxBLACK;
        const wxColour cellColor1 = ApplyColorOpacity(wxColour(200, 140, 230)); // light purple/pink
        const wxColour cellColor2 = ApplyColorOpacity(wxColour(160, 100, 210)); // mid purple
        const wxColour cellColor3 = ApplyColorOpacity(wxColour(120, 60, 180));  // dark purple

        const auto drawWings = [&](double scaleX)
        {
            gc->PushState();
            gc->Translate(centerX, centerY);
            gc->Scale(scaleX, 1.0);
            gc->Translate(-centerX, -centerY);

            const double bodyWidth = drawRect.m_width * 0.08;

            // wing paths
            wxGraphicsPath upperWingFrame = gc->CreatePath();
            upperWingFrame.MoveToPoint(centerX + (bodyWidth / 3),
                                       centerY - (drawRect.m_height * 0.05));
            upperWingFrame.AddCurveToPoint(
                centerX + (drawRect.m_width * 0.15), centerY - (drawRect.m_height * 0.48),
                centerX + (drawRect.m_width * 0.48), centerY - (drawRect.m_height * 0.48),
                centerX + (drawRect.m_width * 0.48), centerY - (drawRect.m_height * 0.05));
            upperWingFrame.AddCurveToPoint(
                centerX + (drawRect.m_width * 0.48), centerY + (drawRect.m_height * 0.15),
                centerX + (drawRect.m_width * 0.25), centerY + (drawRect.m_height * 0.20),
                centerX + (bodyWidth / 3), centerY + (drawRect.m_height * 0.05));
            upperWingFrame.CloseSubpath();

            wxGraphicsPath lowerWingFrame = gc->CreatePath();
            lowerWingFrame.MoveToPoint(centerX + (bodyWidth / 3),
                                       centerY + (drawRect.m_height * 0.10));
            lowerWingFrame.AddCurveToPoint(
                centerX + (drawRect.m_width * 0.45), centerY + (drawRect.m_height * 0.15),
                centerX + (drawRect.m_width * 0.48), centerY + (drawRect.m_height * 0.48),
                centerX + (drawRect.m_width * 0.25), centerY + (drawRect.m_height * 0.48));
            lowerWingFrame.AddCurveToPoint(
                centerX + (drawRect.m_width * 0.05), centerY + (drawRect.m_height * 0.48),
                centerX + (bodyWidth / 3), centerY + (drawRect.m_height * 0.30),
                centerX + (bodyWidth / 3), centerY + (drawRect.m_height * 0.10));
            lowerWingFrame.CloseSubpath();

            // fill wings with gradients — outer-tip light, inner-base darker
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                centerX + (drawRect.m_width * 0.40), centerY - (drawRect.m_height * 0.38),
                centerX + (drawRect.m_width * 0.10), centerY + (drawRect.m_height * 0.12),
                cellColor1, cellColor2));
            gc->FillPath(upperWingFrame);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                centerX + (drawRect.m_width * 0.20), centerY + (drawRect.m_height * 0.13),
                centerX + (drawRect.m_width * 0.35), centerY + (drawRect.m_height * 0.46),
                cellColor2, cellColor3));
            gc->FillPath(lowerWingFrame);

            // inset accent ovals — radial gradient: bright gold highlight → deep amber edge
            const wxColour amberLight{ ApplyColorOpacity(wxColour(255, 235, 120)) };
            const wxColour amberDark{ ApplyColorOpacity(wxColour(230, 175, 50)) };

            // upper wing oval — gradient defined in post-translate local space (center = 0,0)
            gc->PushState();
            gc->Translate(centerX + (drawRect.m_width * 0.26),
                          centerY - (drawRect.m_height * 0.13));
            gc->SetBrush(gc->CreateRadialGradientBrush(
                0.0, 0.0, -drawRect.m_width * 0.03, -drawRect.m_height * 0.06,
                drawRect.m_height * 0.15, amberDark, amberLight));
            gc->DrawEllipse(-drawRect.m_width * 0.10, -drawRect.m_height * 0.15,
                            drawRect.m_width * 0.20, drawRect.m_height * 0.30);
            gc->PopState();

            // lower wing oval
            gc->PushState();
            gc->Translate(centerX + (drawRect.m_width * 0.26),
                          centerY + (drawRect.m_height * 0.30));
            gc->SetBrush(gc->CreateRadialGradientBrush(
                0.0, 0.0, -drawRect.m_width * 0.03, -drawRect.m_height * 0.03,
                drawRect.m_height * 0.10, amberDark, amberLight));
            gc->DrawEllipse(-drawRect.m_width * 0.09, -drawRect.m_height * 0.09,
                            drawRect.m_width * 0.18, drawRect.m_height * 0.18);
            gc->PopState();

            // thick black border stroke on top — defines wing outline and hides any edge artifacts
            const wxPen borderPen(frameColor,
                                  std::max(1, static_cast<int>(drawRect.m_width * 0.02)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(borderPen);
            gc->StrokePath(upperWingFrame);
            gc->StrokePath(lowerWingFrame);

            gc->PopState();
        };

        drawWings(1.0);  // right wings
        drawWings(-1.0); // left wings

        // body — radial gradient lit from upper-left
        const double bodyWidth = drawRect.m_width * 0.06;
        const double bodyHeight = drawRect.m_height * 0.55;
        const double bodyCenterX = centerX;
        const double bodyCenterY =
            centerY - (bodyHeight * 0.4) + (bodyHeight * math_constants::half);
        gc->SetBrush(gc->CreateRadialGradientBrush(
            bodyCenterX, bodyCenterY, bodyCenterX - (bodyWidth * 0.18),
            bodyCenterY - (bodyHeight * 0.18), bodyHeight * math_constants::half,
            wxColour{ 0, 0, 0 }, wxColour{ 75, 75, 75 }));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(centerX - (bodyWidth / 2), centerY - (bodyHeight * 0.4), bodyWidth,
                        bodyHeight);

        // head — same radial highlight
        const double headSize = bodyWidth * 1.3;
        const double headCenterX = centerX;
        const double headCenterY =
            centerY - (bodyHeight * 0.4) - (headSize / 1.5) + (headSize * math_constants::half);
        gc->SetBrush(gc->CreateRadialGradientBrush(
            headCenterX, headCenterY, headCenterX - (headSize * 0.18),
            headCenterY - (headSize * 0.18), headSize * math_constants::half, wxColour{ 0, 0, 0 },
            wxColour{ 75, 75, 75 }));
        gc->DrawEllipse(centerX - (headSize / 2), centerY - (bodyHeight * 0.4) - (headSize / 1.5),
                        headSize, headSize);

        // antennae
        const double antLen = drawRect.m_height * 0.18;
        wxGraphicsPath antennae = gc->CreatePath();
        antennae.MoveToPoint(centerX - (headSize * 0.2),
                             centerY - (bodyHeight * 0.4) - (headSize / 4));
        antennae.AddCurveToPoint(centerX - (headSize * 0.4), centerY - (bodyHeight * 0.5),
                                 centerX - (headSize * 0.6), centerY - (bodyHeight * 0.4) - antLen,
                                 centerX - (headSize * 0.8), centerY - (bodyHeight * 0.4) - antLen);
        antennae.MoveToPoint(centerX + (headSize * 0.2),
                             centerY - (bodyHeight * 0.4) - (headSize / 4));
        antennae.AddCurveToPoint(centerX + (headSize * 0.4), centerY - (bodyHeight * 0.5),
                                 centerX + (headSize * 0.6), centerY - (bodyHeight * 0.4) - antLen,
                                 centerX + (headSize * 0.8), centerY - (bodyHeight * 0.4) - antLen);
        gc->SetPen(wxPen(bodyColor, ScaleToScreenAndCanvas(0.8)));
        gc->StrokePath(antennae);

        // antennae tips
        const double tipSize = ScaleToScreenAndCanvas(1.5);
        gc->SetBrush(bodyColor);
        gc->DrawEllipse(centerX - (headSize * 0.8) - (tipSize / 2),
                        centerY - (bodyHeight * 0.4) - antLen - (tipSize / 2), tipSize, tipSize);
        gc->DrawEllipse(centerX + (headSize * 0.8) - (tipSize / 2),
                        centerY - (bodyHeight * 0.4) - antLen - (tipSize / 2), tipSize, tipSize);

        gc->PopState();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSunFlower(const wxRect rect, wxDC& dc) const
        {
        DrawBaseFlower(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::Sunflower),
                       Colors::ColorBrewer::GetColor(Colors::Color::Gamboge));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFlower(const wxRect rect, wxDC& dc) const
        {
        DrawBaseFlower(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::Wisteria),
                       Colors::ColorBrewer::GetColor(Colors::Color::Goldenrod));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFallLeaf(const wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        wxGraphicsContext* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for leaf icon!");
        if (gc == nullptr)
            {
            return;
            }

        // rotate 45° about center
        const wxPoint centerPoint =
            rect.GetTopLeft() + wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);
        gc->PushState();
        gc->Translate(centerPoint.x, centerPoint.y);
        gc->Rotate(geometry::degrees_to_radians(45));
        gc->Translate(-centerPoint.x, -centerPoint.y);

        // leaf fill (red -> orange)
        gc->SetPen(*wxTRANSPARENT_PEN);
        const auto leafBrush = gc->CreateLinearGradientBrush(
            GetXPosFromLeft(rect, 0.00), GetYPosFromTop(rect, math_constants::half),
            GetXPosFromLeft(rect, math_constants::three_fourths),
            GetYPosFromTop(rect, math_constants::half),
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::ChineseRed)),
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SunsetOrange)));
        gc->SetBrush(leafBrush);

        wxGraphicsPath leafPath = gc->CreatePath();
        // left edge (bottom -> tip)
        leafPath.MoveToPoint(GetXPosFromLeft(rect, math_constants::half),
                             GetYPosFromTop(rect, math_constants::three_quarters));
        leafPath.AddQuadCurveToPoint(GetXPosFromLeft(rect, 0.00), GetYPosFromTop(rect, 0.60),
                                     GetXPosFromLeft(rect, math_constants::half),
                                     GetYPosFromTop(rect, 0.00)); // tip
        // right edge (tip -> bottom)
        leafPath.AddQuadCurveToPoint(GetXPosFromLeft(rect, 1.00), GetYPosFromTop(rect, 0.60),
                                     GetXPosFromLeft(rect, math_constants::half),
                                     GetYPosFromTop(rect, math_constants::three_quarters));
        leafPath.CloseSubpath();
        gc->FillPath(leafPath);

        // key points
        const wxPoint2DDouble leafTipPoint{ GetXPosFromLeft(rect, math_constants::half),
                                            GetYPosFromTop(rect, 0.00) };
        const wxPoint2DDouble leafBottomPoint{ GetXPosFromLeft(rect, math_constants::half),
                                               GetYPosFromTop(rect,
                                                              math_constants::three_quarters) };
        const wxPoint2DDouble stemCurlEndPoint{
            GetXPosFromLeft(rect, 0.40), GetYPosFromTop(rect, math_constants::full - 0.025)
        };

        // stem styling
        const wxColour stemDarkBrown = Colors::ColorBrewer::GetColor(Colors::Color::DarkBrown);
        const int stemWidthPx = std::max<int>(
            1, ScaleToScreenAndCanvas(rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2));

        // inside stem: same brown at 50% opacity
        const wxPen insideStemPen(
            Colors::ColorContrast::ChangeOpacity(stemDarkBrown, static_cast<uint8_t>(0.50 * 255)),
            stemWidthPx);
        // outside/curl: opaque brown
        const wxPen outsideStemPen(stemDarkBrown, stemWidthPx);

            // inside stem (just below tip -> bottom)
            {
            // shorten at the tip end (so the inside stem doesn't poke past the leaf tip)
            // move the start point down a hair from the tip; proportional with a small cap
            const double shortenFromTipPx =
                std::min<double>(rect.GetHeight() * 0.02, ScaleToScreenAndCanvas(3.0));
            const double insideStartY = leafTipPoint.m_y + shortenFromTipPx;
            wxGraphicsPath insideStemPath = gc->CreatePath();
            insideStemPath.MoveToPoint(leafTipPoint.m_x, insideStartY);
            insideStemPath.AddLineToPoint(leafBottomPoint.m_x, leafBottomPoint.m_y);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(insideStemPen);
            gc->StrokePath(insideStemPath);
            }

            // outside curl (starts at the true bottom so it covers the seam)
            {
            wxGraphicsPath outsideStemPath = gc->CreatePath();
            outsideStemPath.MoveToPoint(leafBottomPoint.m_x, leafBottomPoint.m_y);
            outsideStemPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::half),
                GetYPosFromTop(rect, math_constants::three_quarters +
                                         (math_constants::quarter * math_constants::half)),
                stemCurlEndPoint.m_x, stemCurlEndPoint.m_y);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(outsideStemPen);
            gc->StrokePath(outsideStemPath);
            }

        gc->PopState();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSnowflake(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for leaf icon!");
        if (gc != nullptr)
            {
            const wxPen bodyPen(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Ice)),
                ScaleToScreenAndCanvas(1));
            const wxPen crystalPen(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Ice)),
                ScaleToScreenAndCanvas(1));

            const auto centerPt =
                rect.GetTopLeft() + wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

            constexpr auto PEN_CAP_WIGGLE_ROOM = 0.05;

            // a line going from the middle of the left side to the middle of the right
            const std::array<wxPoint2DDouble, 2> points = {
                wxPoint(GetXPosFromLeft(rect, PEN_CAP_WIGGLE_ROOM),
                        GetYPosFromTop(rect, math_constants::half)),
                wxPoint(GetXPosFromLeft(rect, math_constants::full - PEN_CAP_WIGGLE_ROOM),
                        GetYPosFromTop(rect, math_constants::half))
            };
            // save current transform matrix state
            gc->PushState();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the lines, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle = 0.0;
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over from
                // the translated origin
                gc->SetPen(bodyPen);
                gc->StrokeLine(points[0].m_x - centerPt.x, points[0].m_y - centerPt.y,
                               points[1].m_x - centerPt.x, points[1].m_y - centerPt.y);
                // outer leaf branch
                gc->StrokeLine(
                    GetXPosFromLeft(rect,
                                    math_constants::three_fourths + math_constants::twentieth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - (PEN_CAP_WIGGLE_ROOM * 2)) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half - math_constants::tenth) -
                        centerPt.y);
                gc->StrokeLine(
                    GetXPosFromLeft(rect,
                                    math_constants::three_fourths + math_constants::twentieth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - (PEN_CAP_WIGGLE_ROOM * 2)) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half + math_constants::tenth) -
                        centerPt.y);
                // inner leaf branch
                gc->SetPen(crystalPen);
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths - math_constants::tenth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - PEN_CAP_WIGGLE_ROOM -
                                              math_constants::fifth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half - math_constants::tenth) -
                        centerPt.y);
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths - math_constants::tenth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - PEN_CAP_WIGGLE_ROOM -
                                              math_constants::fifth) -
                        centerPt.x,
                    GetYPosFromTop(rect, math_constants::half + math_constants::tenth) -
                        centerPt.y);
                angle += 45;
                }
            // restore transform matrix
            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFlame(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        // Increase deflation to provide a safer buffer for high-res PDF clipping
        drawRect.Deflate(static_cast<int>(ScaleToScreenAndCanvas(2)));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for flame!");
        if (gc != nullptr)
            {
            gc->PushState();
            gc->Translate(rect.x, rect.y);
            const wxRect2DDouble localRect(0, 0, rect.width, rect.height);
            const wxRect2DDouble localDrawRect(drawRect.x - rect.x, drawRect.y - rect.y,
                                               drawRect.width, drawRect.height);

            const auto drawFlameInner = [&gc, this](const wxRect2DDouble& drawingRect,
                                                    const wxColour& color1, const wxColour& color2)
            {
                gc->SetBrush(gc->CreateLinearGradientBrush(
                    GetXPosFromLeft(drawingRect, math_constants::half),
                    GetYPosFromTop(drawingRect, math_constants::full),
                    GetXPosFromLeft(drawingRect, math_constants::half),
                    GetYPosFromTop(drawingRect, math_constants::fifth), ApplyColorOpacity(color1),
                    ApplyColorOpacity(color2)));
                gc->SetPen(wxColour{ 0, 0, 0, 0 });

                wxGraphicsPath flamePath = gc->CreatePath();

                flamePath.MoveToPoint(GetXPosFromLeft(drawingRect, math_constants::half),
                                      GetYPosFromTop(drawingRect, math_constants::full));
                flamePath.AddCurveToPoint(
                    GetXPosFromLeft(drawingRect, 0.05), GetYPosFromTop(drawingRect, 0.9),
                    GetXPosFromLeft(drawingRect, 0.4), GetYPosFromTop(drawingRect, 0.45),
                    GetXPosFromLeft(drawingRect, math_constants::quarter),
                    GetYPosFromTop(drawingRect, 0.4));
                flamePath.AddQuadCurveToPoint(
                    GetXPosFromLeft(drawingRect, 0.4), GetYPosFromTop(drawingRect, 0.4),
                    GetXPosFromLeft(drawingRect, 0.35), GetYPosFromTop(drawingRect, 0.525));
                // Tallest tongue - lowered further for PDF headroom
                flamePath.AddQuadCurveToPoint(GetXPosFromLeft(drawingRect, 0.6),
                                              GetYPosFromTop(drawingRect, 0.32),
                                              GetXPosFromLeft(drawingRect, math_constants::half),
                                              GetYPosFromTop(drawingRect, 0.22));
                flamePath.AddQuadCurveToPoint(GetXPosFromLeft(drawingRect, 0.65),
                                              GetYPosFromTop(drawingRect, 0.3),
                                              GetXPosFromLeft(drawingRect, 0.58),
                                              GetYPosFromTop(drawingRect, math_constants::half));
                flamePath.AddQuadCurveToPoint(
                    GetXPosFromLeft(drawingRect, 0.75), GetYPosFromTop(drawingRect, 0.4),
                    GetXPosFromLeft(drawingRect, 0.75), GetYPosFromTop(drawingRect, 0.28));
                flamePath.AddCurveToPoint(
                    GetXPosFromLeft(drawingRect, 0.85), GetYPosFromTop(drawingRect, 0.6),
                    GetXPosFromLeft(drawingRect, 0.82), GetYPosFromTop(drawingRect, 0.97),
                    GetXPosFromLeft(drawingRect, math_constants::half),
                    GetYPosFromTop(drawingRect, math_constants::full));

                flamePath.CloseSubpath();
                gc->FillPath(flamePath);
                gc->StrokePath(flamePath);
            };

            drawFlameInner(localDrawRect, Colors::ColorBrewer::GetColor(Colors::Color::OrangeRed),
                           Colors::ColorBrewer::GetColor(Colors::Color::Orange));

            // draw inner flames
            wxRect2DDouble innerFlameRect{ localDrawRect };
            innerFlameRect.Inset(localDrawRect.m_width * math_constants::fifth, 0);
            innerFlameRect.m_height = localDrawRect.m_height * 0.8;
            innerFlameRect.m_y = localDrawRect.GetBottom() - innerFlameRect.m_height;

            drawFlameInner(innerFlameRect,
                           Colors::ColorBrewer::GetColor(Colors::Color::OrangeYellow),
                           Colors::ColorBrewer::GetColor(Colors::Color::YellowPepper));

            innerFlameRect.Inset(innerFlameRect.m_width * math_constants::fifth, 0);
            innerFlameRect.m_height *= 0.8;
            innerFlameRect.m_y = localDrawRect.GetBottom() - innerFlameRect.m_height;
            drawFlameInner(innerFlameRect,
                           Colors::ColorBrewer::GetColor(Colors::Color::PastelOrange),
                           Colors::ColorBrewer::GetColor(Colors::Color::OutrageousOrange));

            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSun(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        // if small, then just draw a simple circle
        if (rect.GetWidth() <= ScaleToScreenAndCanvas(10))
            {
            const wxDCPenChanger pc(dc, *wxTRANSPARENT_PEN);
            const wxDCBrushChanger bc(
                dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Sunglow)));
            dc.DrawEllipse(rect);
            return;
            }

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for sun icon!");
        if (gc != nullptr)
            {
            gc->PushState();
            gc->Translate(rect.x, rect.y);

            const wxRect localRect(0, 0, rect.width, rect.height);
            // a sun with a sunset (deeper orange) color blended near the bottom
            gc->SetPen(wxColour{ 0, 0, 0, 0 });
            auto sunBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(localRect, 0), GetYPosFromTop(localRect, 0),
                GetXPosFromLeft(localRect, 1.5), GetYPosFromTop(localRect, 1.5),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Sunglow)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SunsetOrange)));
            gc->SetBrush(sunBrush);

            const wxRect sunRect = wxRect(localRect).Deflate(ScaleToScreenAndCanvas(1));
            gc->DrawEllipse(sunRect.GetTopLeft().x, sunRect.GetTopLeft().y, sunRect.GetWidth(),
                            sunRect.GetHeight());
            gc->PopState();
            }
        }
    } // namespace Wisteria::GraphItems
