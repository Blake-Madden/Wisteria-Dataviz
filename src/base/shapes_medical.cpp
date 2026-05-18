///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_medical.cpp
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
    void ShapeRenderer::DrawThermometer(const wxRect rect, wxDC& dc, const Temperature temp) const
        {
        wxPen scaledPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                         static_cast<int>(
                             ScaleToScreenAndCanvas(rect.GetWidth() <= ScaleToScreenAndCanvas(32) ?
                                                        math_constants::half :
                                                        math_constants::full)) };
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        wxRect drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(1));
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ drawRect.GetWidth() * 0.4 };
            const auto adjustLeft{ (drawRect.GetWidth() - adjustedWidth) * math_constants::half };
            drawRect.SetWidth(adjustedWidth);
            drawRect.Offset(wxPoint(adjustLeft, 0));
            }
            // add padding
            {
            const auto adjustedWidth{ drawRect.GetWidth() * 0.8 };
            const auto adjustLeft{ (drawRect.GetWidth() - adjustedWidth) * math_constants::half };
            drawRect.SetWidth(adjustedWidth);
            drawRect.Offset(wxPoint(adjustLeft, 0));
            }

        wxRect sunOrSnowRect{ rect };
        sunOrSnowRect.SetHeight(sunOrSnowRect.GetHeight() * math_constants::half);
        sunOrSnowRect.SetWidth(sunOrSnowRect.GetWidth() * math_constants::half);
        if (temp == Temperature::Hot)
            {
            DrawSun(sunOrSnowRect, dc);
            }
        else
            {
            DrawSnowflake(sunOrSnowRect, dc);
            }

            // stem
            {
                {
                const wxDCBrushChanger bc(
                    dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White)));
                dc.DrawRoundedRectangle(drawRect, ScaleToScreenAndCanvas(2));
                drawRect.Deflate(static_cast<int>(ScaleToScreenAndCanvas(1.5)));
                }
                // mercury
                {
                wxRect mercuryRect{ drawRect };
                if (temp == Temperature::Cold)
                    {
                    mercuryRect.SetHeight(mercuryRect.GetHeight() * math_constants::third);
                    mercuryRect.Offset(0, drawRect.GetHeight() * math_constants::two_thirds);
                    const wxDCBrushChanger bc(
                        dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Ice)));
                    dc.DrawRectangle(mercuryRect);
                    }
                else
                    {
                    const wxDCBrushChanger bc(dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(
                                                      Colors::Color::TractorRed)));
                    dc.DrawRectangle(mercuryRect);
                    }
                }
            }

        if (temp == Temperature::Hot)
            {
            scaledPen.SetColour(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::LightGray)));
            }
        const DCPenChangerIfDifferent pc2(dc, scaledPen);
        // measuring lines along stem
        wxRect clipRect{ rect };
        clipRect.SetHeight(clipRect.GetHeight() * 0.90);
        wxRect currentClipRect;
        wxRegion clipRegion(clipRect);
        if (dc.GetClippingBox(currentClipRect))
            {
            clipRegion.Intersect(currentClipRect);
            }
        const wxDCClipper clip{ dc, clipRegion };
        int currentY{ drawRect.GetTop() + static_cast<int>(ScaleToScreenAndCanvas(2)) };
        int currentLine{ 0 };
        while (currentY < drawRect.GetBottom())
            {
            dc.DrawLine(
                { drawRect.GetLeft() +
                      static_cast<int>(drawRect.GetWidth() * ((currentLine % 4 == 0) ?
                                                                  math_constants::half :
                                                                  math_constants::three_fourths)),
                  currentY },
                { drawRect.GetRight(), currentY });
            currentY += ScaleToScreenAndCanvas(GetScaling() <= 2.0 ? 2 : 1);
            ++currentLine;
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawColdThermometer(const wxRect rect, wxDC& dc) const
        {
        DrawThermometer(rect, dc, Temperature::Cold);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHotThermometer(const wxRect rect, wxDC& dc) const
        {
        DrawThermometer(rect, dc, Temperature::Hot);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPill(wxRect rect, wxDC& dc) const
        {
        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        // increase deflation to ensure tilted diagonal edges don't clip in high-res PDF
        rect.Deflate(ScaleToScreenAndCanvas(4));

        // colors
        const wxColour outlineColor{ 80, 30, 30 };
        const wxColour whiteBase{ 240, 240, 240 };
        const wxColour whiteShadow{ 180, 180, 180 };
        const wxColour whiteSheen{ 255, 255, 255, 180 };
        const wxColour redBase{ 200, 60, 60 };
        const wxColour redShadow{ 140, 35, 35 };
        const wxColour redSheen{ 255, 150, 150, 160 };
        const wxColour dividerLine{ 60, 20, 20 };

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double cy = rect.GetY() + (rect.GetHeight() / 2.0);

        const double diagonal = std::min(rect.GetWidth(), rect.GetHeight());
        const double pillLength = diagonal * 0.85;
        const double pillWidth = pillLength * 0.40;
        const double r = pillWidth / 2.0;
        const double halfBody = (pillLength / 2.0) - r;

        gc->PushState();
        gc->Translate(cx, cy);
        gc->Rotate(-45.0 * std::numbers::pi / 180.0);

        // single path for the whole pill
        wxGraphicsPath capsulePath = gc->CreatePath();
        capsulePath.AddArc(0, -halfBody, r, -std::numbers::pi, 0, true);
        capsulePath.AddArc(0, halfBody, r, 0, std::numbers::pi, true);
        capsulePath.CloseSubpath();

        // build red half path
        wxGraphicsPath redHalfPath = gc->CreatePath();
        redHalfPath.AddArc(0, halfBody, r, 0, std::numbers::pi, true);
        redHalfPath.AddLineToPoint(-r, 0);
        redHalfPath.AddLineToPoint(r, 0);
        redHalfPath.CloseSubpath();

        // build white half path
        wxGraphicsPath whiteHalfPath = gc->CreatePath();
        whiteHalfPath.AddArc(0, -halfBody, r, -std::numbers::pi, 0, true);
        whiteHalfPath.AddLineToPoint(r, 0);
        whiteHalfPath.AddLineToPoint(-r, 0);
        whiteHalfPath.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);

        // fill white (top) half
        gc->SetBrush(gc->CreateLinearGradientBrush(-r, 0, r, 0, whiteBase, whiteShadow));
        gc->FillPath(whiteHalfPath);

        // fill red (bottom) half
        gc->SetBrush(gc->CreateLinearGradientBrush(-r, 0, r, 0, redBase, redShadow));
        gc->FillPath(redHalfPath);

        // stroke the outline
        const auto outlinePenWidth = std::max<double>(1.0, ScaleToScreenAndCanvas(1));
        gc->SetPen(wxPen{ outlineColor, static_cast<int>(outlinePenWidth) });
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(capsulePath);

        // divider
        gc->SetPen(wxPen{ dividerLine, static_cast<int>(outlinePenWidth) });
        gc->StrokeLine(-r, 0, r, 0);

        // sheens
        const double sheenW = r * 0.35;
        const double sheenH = halfBody * 0.9;
        const double sheenX = -r * 0.5;

        auto drawSheen = [&](double centerY, const wxColour& color)
        {
            gc->SetBrush(wxBrush{ color });
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawEllipse(sheenX - sheenW / 2.0, centerY - sheenH / 2.0, sheenW, sheenH);
        };

        drawSheen(-(halfBody * 0.6 + r * 0.2), whiteSheen);
        drawSheen(halfBody * 0.6 + r * 0.2, redSheen);

        gc->PopState();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawIVBag(const wxRect rect, wxDC& dc) const
        {
        const wxPen scaledPen{
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
            static_cast<int>(ScaleToScreenAndCanvas(
                rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? math_constants::half : 1.0))
        };
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);
        drawRect.SetHeight(drawRect.GetHeight() * math_constants::three_fourths);
            // add padding
            {
            const auto adjustedWidth{ drawRect.GetWidth() * math_constants::two_thirds };
            const auto adjustLeft{ (drawRect.GetWidth() - adjustedWidth) * math_constants::half };
            drawRect.SetWidth(adjustedWidth);
            drawRect.Offset(wxPoint(adjustLeft, 0));
            }

            // outside bag
            {
            const wxDCBrushChanger bc(
                dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White)));
            dc.DrawRoundedRectangle(drawRect, ScaleToScreenAndCanvas(2));
            }

            // IV line going from bag
            {
            const wxDCPenChanger pc2(
                dc, wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                           static_cast<int>(drawRect.GetWidth() * math_constants::fifth) });
            wxPoint lineTop{ rect.GetLeftTop() };
            lineTop.x += rect.GetWidth() * 0.6;
            wxPoint lineBottom{ rect.GetLeftBottom() };
            lineBottom.x += rect.GetWidth() * 0.6;
            lineBottom.y -= ScaleToScreenAndCanvas(2.5);
            wxRect lineRect{ rect };
            lineRect.SetHeight(lineRect.GetHeight() * math_constants::half);
            lineRect.Offset(0, lineRect.GetHeight());
            wxRect currentClipRect;
            wxRegion clipRegion(lineRect);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            dc.DrawLine(lineTop, lineBottom);
                {
                const wxDCPenChanger pc3(dc, wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(
                                                        Colors::Color::RedTomato)),
                                                    static_cast<int>(drawRect.GetWidth() * 0.15) });
                dc.DrawLine(lineTop, lineBottom);
                }
            }

        // fill the bag with blood
        drawRect.Deflate(static_cast<wxCoord>(ScaleToScreenAndCanvas(1.5)));
            {
            const wxDCBrushChanger bc(
                dc, TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::RedTomato)));
            wxRect liquidRect{ drawRect };
            liquidRect.SetHeight(liquidRect.GetHeight() * math_constants::half);
            liquidRect.Offset(0, liquidRect.GetHeight());
            wxRect currentClipRect;
            wxRegion clipRegion(liquidRect);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            dc.DrawRoundedRectangle(drawRect, ScaleToScreenAndCanvas(2));
            }

        // ruler lines along the side of the bag
        int currentY{ drawRect.GetTop() + static_cast<int>(ScaleToScreenAndCanvas(2)) };
        int currentLine{ 0 };
        while (currentY < drawRect.GetBottom())
            {
            dc.DrawLine(
                { drawRect.GetLeft() +
                      static_cast<int>(drawRect.GetWidth() * ((currentLine % 4 == 0) ?
                                                                  math_constants::half :
                                                                  math_constants::three_fourths)),
                  currentY },
                { drawRect.GetRight(), currentY });
            currentY += ScaleToScreenAndCanvas(2);
            ++currentLine;
            }
        }
    } // namespace Wisteria::GraphItems
