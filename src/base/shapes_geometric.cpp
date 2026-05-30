///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_geometric.cpp
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
    void ShapeRenderer::DrawTack(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const std::array<wxPoint, 11> points = {
            // the needle
            wxPoint(GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, math_constants::half)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::half)),
            // top half of tack's handle
            wxPoint(GetXPosFromLeft(rect, math_constants::third), GetYPosFromTop(rect, 0)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, 0.90), GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, safe_divide(math_constants::third, 2.0))),
            // bottom half
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, math_constants::half +
                                             (safe_divide(math_constants::third, 2.0) * 2))),
            wxPoint(GetXPosFromLeft(rect, 0.90), GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::full)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::half)),
        };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHeart(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        // Aggressive deflation (4 DIPs) to ensure we are nowhere near the PDF clipping boundaries
        drawRect.Deflate(static_cast<int>(ScaleToScreenAndCanvas(4)));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for heart!");
        if (gc != nullptr)
            {
            gc->PushState();
            gc->Translate(rect.x, rect.y);
            const wxRect2DDouble localRect(0, 0, rect.width, rect.height);
            const wxRect2DDouble localDrawRect(drawRect.x - rect.x, drawRect.y - rect.y,
                                               drawRect.width, drawRect.height);

            gc->SetPen(wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black),
                              static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(localRect, 0.08), GetYPosFromTop(localRect, math_constants::half),
                GetXPosFromLeft(localRect, 0.92), GetYPosFromTop(localRect, math_constants::half),
                ApplyColorOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::CandyApple),
                    math_constants::three_fourths)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::CandyApple))));

            wxGraphicsPath heartPath = gc->CreatePath();

            heartPath.MoveToPoint(GetXPosFromLeft(localDrawRect, math_constants::half),
                                  GetYPosFromTop(localDrawRect, 0.25));
            // left side - broaden back out (0.08)
            heartPath.AddCurveToPoint(
                GetXPosFromLeft(localDrawRect, 0.08), GetYPosFromTop(localDrawRect, 0.0),
                GetXPosFromLeft(localDrawRect, 0.08), GetYPosFromTop(localDrawRect, 0.6),
                GetXPosFromLeft(localDrawRect, math_constants::half),
                GetYPosFromTop(localDrawRect, 0.95));
            // right side - broaden back out (0.92)
            heartPath.AddCurveToPoint(
                GetXPosFromLeft(localDrawRect, 0.92), GetYPosFromTop(localDrawRect, 0.6),
                GetXPosFromLeft(localDrawRect, 0.92), GetYPosFromTop(localDrawRect, 0.0),
                GetXPosFromLeft(localDrawRect, math_constants::half),
                GetYPosFromTop(localDrawRect, 0.25));

            heartPath.CloseSubpath();
            gc->FillPath(heartPath);
            gc->StrokePath(heartPath);

            // shine
            wxGraphicsPath shinePath = gc->CreatePath();

            gc->SetPen(wxPen{ wxColour{ 255, 255, 255, 150 },
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });

            shinePath.MoveToPoint(GetXPosFromLeft(localDrawRect, 0.35),
                                  GetYPosFromTop(localDrawRect, 0.35));
            shinePath.AddQuadCurveToPoint(GetXPosFromLeft(localDrawRect, math_constants::quarter),
                                          GetYPosFromTop(localDrawRect, 0.37),
                                          GetXPosFromLeft(localDrawRect, 0.3),
                                          GetYPosFromTop(localDrawRect, math_constants::half));

            gc->StrokePath(shinePath);

            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawAsterisk(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for asterisk icon!");

        DrawAsterisk(rect, gc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawAsterisk(wxRect rect, wxGraphicsContext* gc) const
        {
        if (gc != nullptr)
            {
            wxPen scaledPen = GetGraphItemInfo().GetPen().IsOk() ? GetGraphItemInfo().GetPen() :
                                                                   wxPen{ *wxBLACK };
            if (scaledPen.IsOk() && gc != nullptr)
                {
                scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(
                    1.0, rect.GetWidth() <= ScaleToScreenAndCanvas(16) ? math_constants::half :
                                                                         math_constants::full)));
                gc->SetPen(scaledPen);
                }

            const wxPoint2DDouble centerPt{
                rect.GetLeft() + (rect.GetWidth() * math_constants::half),
                rect.GetTop() + (rect.GetHeight() * math_constants::half)
            };

            gc->PushState();
            gc->Translate(centerPt.m_x, centerPt.m_y);

            const double halfWidth = rect.GetWidth() * math_constants::half;

            // centered line in local coordinates
            const wxPoint2DDouble p1{ -halfWidth, 0 };
            const wxPoint2DDouble p2{ halfWidth, 0 };

            for (int deg = 0; deg < 180; deg += 45)
                {
                const double angleRad = geometry::degrees_to_radians(static_cast<double>(deg));

                gc->PushState();
                gc->Rotate(angleRad);
                gc->StrokeLine(p1.m_x, p1.m_y, p2.m_x, p2.m_y);
                gc->PopState();
                }

            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawStar(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for star icon!");
        if (gc == nullptr)
            {
            return;
            }

        const wxPoint2DDouble centerPt{ rect.GetLeft() + (rect.GetWidth() * math_constants::half),
                                        rect.GetTop() + (rect.GetHeight() * math_constants::half) };

        const double outerRadius = std::min(rect.GetWidth(), rect.GetHeight()) * 0.45;
        const double innerRadius = outerRadius * 0.4;
        const double startAngle = -(std::numbers::pi / 2);

        std::vector<wxPoint2DDouble> points;
        for (int i = 0; i < 10; ++i)
            {
            const double angle = startAngle + i * (std::numbers::pi * 2.0 / 10.0);
            const double r = (i % 2 == 0) ? outerRadius : innerRadius;
            points.emplace_back(centerPt.m_x + r * std::cos(angle),
                                centerPt.m_y + r * std::sin(angle));
            }

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(points[0]);
        for (size_t i = 1; i < points.size(); ++i)
            {
            path.AddLineToPoint(points[i]);
            }
        path.CloseSubpath();

        const wxColour col = Colors::ColorBrewer::GetColor(Colors::Color::Gold);
        const wxColour shadedCol = Colors::ColorContrast::Shade(col, 0.2);
        gc->SetBrush(gc->CreateLinearGradientBrush(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                                                   rect.GetBottom(), col, shadedCol));

        gc->SetPen(wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black),
                          static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });
        gc->FillPath(path);
        gc->StrokePath(path);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPlus(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                    wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
        dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                    wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHorizontalLine(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            // for a line icon, make it a minimum of 2 pixels wide
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        dc.DrawLine(wxPoint(rect.GetLeft(), rect.GetTop() + (rect.GetHeight() / 2)),
                    wxPoint(rect.GetRight(), rect.GetTop() + (rect.GetHeight() / 2)));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawVerticalLine(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            // for a line icon, make it a minimum of 2 pixels wide
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        dc.DrawLine(wxPoint(rect.GetLeft() + (rect.GetWidth() / 2), rect.GetTop()),
                    wxPoint(rect.GetLeft() + (rect.GetWidth() / 2), rect.GetBottom()));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCrossedOut(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            // for a line icon, make it a minimum of 2 pixels wide
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        dc.DrawLine(wxPoint(rect.GetLeft(), rect.GetTop()),
                    wxPoint(rect.GetRight(), rect.GetBottom()));
        dc.DrawLine(wxPoint(rect.GetRight(), rect.GetTop()),
                    wxPoint(rect.GetLeft(), rect.GetBottom()));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawDownwardTriangle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points = { midPoint + wxPoint(0, iconRadius),
                                                midPoint + wxPoint(-iconRadius, -iconRadius),
                                                midPoint + wxPoint(iconRadius, -iconRadius) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawRightTriangle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points = { midPoint + wxPoint(iconRadius, 0),
                                                midPoint + wxPoint(-iconRadius, iconRadius),
                                                midPoint + wxPoint(-iconRadius, -iconRadius) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawLeftTriangle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points = { midPoint + wxPoint(-iconRadius, 0),
                                                midPoint + wxPoint(iconRadius, iconRadius),
                                                midPoint + wxPoint(iconRadius, -iconRadius) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawRightArrow(const wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        // base color from brush (fallback HunterGreen)
        const wxColour baseColor = (GetGraphItemInfo().GetBrush().IsOk() &&
                                    GetGraphItemInfo().GetBrush().GetColour().IsOk()) ?
                                       GetGraphItemInfo().GetBrush().GetColour() :
                                       Colors::ColorBrewer::GetColor(Colors::Color::HunterGreen);

        const wxColour innerOutlineColor = Colors::ColorContrast::Tint(baseColor, 0.55);
        const wxColour fillColor = Colors::ColorContrast::Tint(baseColor, 0.15);

        const GraphicsContextFallback gcf{ &dc, rect };
        wxGraphicsContext* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for right arrow!");
        if (gc == nullptr)
            {
            return;
            }

        // geometry
        constexpr double SHAFT_RATIO = math_constants::half;
        const int left = rect.GetLeft();
        const int top = rect.GetTop();
        const int right = rect.GetRight();
        const int bottom = rect.GetBottom();
        const int midY = rect.GetTop() + (rect.GetHeight() / 2);
        const int shaftEndX = left + static_cast<int>(rect.GetWidth() * SHAFT_RATIO);

        const int shaftHeight = rect.GetHeight() / 3;
        const int shaftTop = midY - (shaftHeight / 2);
        const int shaftBottom = midY + (shaftHeight / 2);

        // arrow path
        wxGraphicsPath arrowPath = gc->CreatePath();
        arrowPath.MoveToPoint(left, shaftTop);
        arrowPath.AddLineToPoint(shaftEndX, shaftTop);
        arrowPath.AddLineToPoint(shaftEndX, top);
        arrowPath.AddLineToPoint(right, midY);
        arrowPath.AddLineToPoint(shaftEndX, bottom);
        arrowPath.AddLineToPoint(shaftEndX, shaftBottom);
        arrowPath.AddLineToPoint(left, shaftBottom);
        arrowPath.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ fillColor });
        gc->FillPath(arrowPath);

            // sheen: diagonal highlight band within the arrow head only.
            // The head region is geometrically well-defined (bounded by the head diagonal),
            // so no clip region is needed and no pixels escape the arrow boundary.
            {
            const auto width = static_cast<double>(rect.GetWidth());
            const auto height = static_cast<double>(rect.GetHeight());

            const double xShaftEnd = rect.GetLeft() + (rect.GetWidth() * math_constants::half);
            const double yMid = rect.GetTop() + (rect.GetHeight() * math_constants::half);
            const double yTop = rect.GetTop();
            const double xRight = rect.GetRight();

            // head top diagonal: (xShaftEnd, yTop) -> (xRight, yMid)
            const double headSlope =
                safe_divide((yMid - yTop), std::max(1.0, (xRight - xShaftEnd)));
            auto yOnHeadTop = [&](double x) { return yTop + (headSlope * (x - xShaftEnd)); };

            const double bandThickness = std::max(height * 0.22, 2.0);
            const double capRadius = bandThickness * 0.45;
            const double epsPx = std::max(1.0, ScaleToScreenAndCanvas(1.0));

            // left edge: just inside the head's top-left corner
            const double xJ = xShaftEnd + epsPx;
            const double y1J = yTop + epsPx;
            const double y2J = std::min(y1J + bandThickness, yMid - epsPx);

            // right edge: near the tip, glued to the head-top diagonal
            const double xR = xRight - (width * 0.005);
            const double y1R = yOnHeadTop(xR) + epsPx;
            const double y2R = std::min(y1R + bandThickness, yMid - epsPx);

            wxGraphicsPath sheen = gc->CreatePath();
            sheen.MoveToPoint(xJ, y1J);
            sheen.AddLineToPoint(xR, y1R);
            sheen.AddQuadCurveToPoint(xR + (capRadius * 0.70), (y1R + y2R) * math_constants::half,
                                      xR, y2R);
            sheen.AddLineToPoint(xJ, y2J);
            sheen.AddQuadCurveToPoint(xJ - (capRadius * 0.70), (y1J + y2J) * math_constants::half,
                                      xJ, y1J);
            sheen.CloseSubpath();

            const auto sheenBrush = gc->CreateLinearGradientBrush(
                0, y1J, 0, y2J, Colors::ColorContrast::ChangeOpacity(*wxWHITE, 175),
                Colors::ColorContrast::ChangeOpacity(*wxWHITE, 70));

            gc->SetBrush(sheenBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->FillPath(sheen);
            }

            // double outline: outer (base), inner (lighter tint)
            {
            const int outerW = std::max<int>(2, ScaleToScreenAndCanvas(2));
            const int innerW = std::max<int>(1, ScaleToScreenAndCanvas(1));

            wxPen outerPen(baseColor, outerW);
            outerPen.SetJoin(wxJOIN_ROUND);
            outerPen.SetCap(wxCAP_ROUND);
            gc->SetPen(outerPen);
            gc->StrokePath(arrowPath);

            wxPen innerPen(innerOutlineColor, innerW);
            innerPen.SetJoin(wxJOIN_ROUND);
            innerPen.SetCap(wxCAP_ROUND);
            gc->SetPen(innerPen);
            gc->StrokePath(arrowPath);
            }

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHexagon(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 6> points = { midPoint + wxPoint(-iconRadius / 2, -iconRadius),
                                                midPoint + wxPoint(-iconRadius, 0),
                                                midPoint + wxPoint(-iconRadius / 2, iconRadius),
                                                midPoint + wxPoint(iconRadius / 2, iconRadius),
                                                midPoint + wxPoint(iconRadius, 0),
                                                midPoint + wxPoint(iconRadius / 2, -iconRadius) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawUpwardTriangle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points = { midPoint + wxPoint(0, -iconRadius),
                                                midPoint + wxPoint(-iconRadius, iconRadius),
                                                midPoint + wxPoint(iconRadius, iconRadius) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawChevronDownward(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth() * 2));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        std::array<wxPoint, 3> points = { midPoint + wxPoint(-iconRadius, 0),
                                          midPoint + wxPoint(0, iconRadius),
                                          midPoint + wxPoint(iconRadius, 0) };

        std::ranges::for_each(points, [&iconRadius, this](auto& pt)
                              { pt.y -= ScaleToScreenAndCanvas(2 + (iconRadius / 2)); });
        dc.DrawLines(points.size(), points.data());

        std::ranges::for_each(points, [this](auto& pt) { pt.y += ScaleToScreenAndCanvas(4); });
        dc.DrawLines(points.size(), points.data());
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawChevronUpward(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth() * 2));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        std::array<wxPoint, 3> points = { midPoint + wxPoint(-iconRadius, 0),
                                          midPoint + wxPoint(0, -iconRadius),
                                          midPoint + wxPoint(iconRadius, 0) };

        std::ranges::for_each(points, [&iconRadius, this](auto& pt)
                              { pt.y += ScaleToScreenAndCanvas(-2 + (iconRadius / 2)); });
        dc.DrawLines(points.size(), points.data());

        std::ranges::for_each(points, [this](auto& pt) { pt.y += ScaleToScreenAndCanvas(4); });
        dc.DrawLines(points.size(), points.data());
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawDiamond(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 4> points = { midPoint + wxPoint(0, -iconRadius),
                                                midPoint + wxPoint(iconRadius, 0),
                                                midPoint + wxPoint(0, iconRadius),
                                                midPoint + wxPoint(-iconRadius, 0) };

        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(points.size(), points.data()); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSquare(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawRectangle(rect); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCircle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };
        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawCircle(GetMidPoint(rect), GetRadius(rect)); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCrescentTop(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for crescent!");
        if (gc != nullptr)
            {
            gc->SetPen(wxPen{ GetGraphItemInfo().GetBrush().GetColour(),
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->SetBrush(GetGraphItemInfo().GetBrush().GetColour());

            wxGraphicsPath crescentPath = gc->CreatePath();

            const auto startPoint = wxPoint(GetXPosFromLeft(drawRect, 0.8),
                                            GetYPosFromTop(drawRect, math_constants::fifth));
            crescentPath.MoveToPoint(startPoint);
            // outside line
            crescentPath.AddCurveToPoint(
                wxPoint(GetXPosFromLeft(drawRect, math_constants::fifth),
                        GetYPosFromTop(drawRect, 0)),
                wxPoint(GetXPosFromLeft(drawRect, -math_constants::fifth),
                        GetYPosFromTop(drawRect, 0.3)),
                wxPoint(GetXPosFromLeft(drawRect, math_constants::tenth),
                        GetYPosFromTop(drawRect, math_constants::three_quarters)));
            // inside line
            crescentPath.AddCurveToPoint(wxPoint(GetXPosFromLeft(drawRect, 0.05),
                                                 GetYPosFromTop(drawRect, math_constants::fifth)),
                                         wxPoint(GetXPosFromLeft(drawRect, 0.4),
                                                 GetYPosFromTop(drawRect, math_constants::fifth)),
                                         startPoint);

            crescentPath.CloseSubpath();
            gc->FillPath(crescentPath);
            gc->StrokePath(crescentPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCrescentBottom(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for crescent!");
        if (gc != nullptr)
            {
            gc->SetPen(wxPen{ GetGraphItemInfo().GetBrush().GetColour(),
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->SetBrush(GetGraphItemInfo().GetBrush().GetColour());

            wxGraphicsPath crescentPath = gc->CreatePath();

            const auto startPoint = wxPoint(GetXPosFromLeft(drawRect, math_constants::quarter),
                                            GetYPosFromTop(drawRect, 0.4));
            crescentPath.MoveToPoint(startPoint);
            // outside line
            crescentPath.AddCurveToPoint(
                wxPoint(GetXPosFromLeft(drawRect, 0.0), GetYPosFromTop(drawRect, 0.9)),
                wxPoint(GetXPosFromLeft(drawRect, 0.4), GetYPosFromTop(drawRect, 1.0)),
                wxPoint(GetXPosFromLeft(drawRect, 0.8),
                        GetYPosFromTop(drawRect, math_constants::three_quarters)));
            // inside line
            crescentPath.AddCurveToPoint(wxPoint(GetXPosFromLeft(drawRect, math_constants::half),
                                                 GetYPosFromTop(drawRect, 0.8)),
                                         wxPoint(GetXPosFromLeft(drawRect, math_constants::fifth),
                                                 GetYPosFromTop(drawRect, 0.9)),
                                         startPoint);

            crescentPath.CloseSubpath();
            gc->FillPath(crescentPath);
            gc->StrokePath(crescentPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCrescentRight(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for crescent!");
        if (gc != nullptr)
            {
            gc->SetPen(wxPen{ GetGraphItemInfo().GetBrush().GetColour(),
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->SetBrush(GetGraphItemInfo().GetBrush().GetColour());

            wxGraphicsPath crescentPath = gc->CreatePath();

            const auto startPoint =
                wxPoint(GetXPosFromLeft(drawRect, 0.7), GetYPosFromTop(drawRect, 0.29));
            crescentPath.MoveToPoint(startPoint);
            // outside line
            crescentPath.AddCurveToPoint(
                wxPoint(GetXPosFromLeft(drawRect, 0.8), GetYPosFromTop(drawRect, 0.3)),
                wxPoint(GetXPosFromLeft(drawRect, 1.1), GetYPosFromTop(drawRect, 0.65)),
                wxPoint(GetXPosFromLeft(drawRect, 0.35), GetYPosFromTop(drawRect, 0.7)));
            // inside line
            crescentPath.AddCurveToPoint(
                wxPoint(GetXPosFromLeft(drawRect, 1.1), GetYPosFromTop(drawRect, 0.4)),
                wxPoint(GetXPosFromLeft(drawRect, 0.4),
                        GetYPosFromTop(drawRect, math_constants::fifth)),
                startPoint);

            crescentPath.CloseSubpath();
            gc->FillPath(crescentPath);
            gc->StrokePath(crescentPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCurlyBrace(const wxRect rect, wxDC& dc, const Side side) const
        {
        wxASSERT_MSG(GetGraphItemInfo().GetPen().IsOk(),
                     L"Pen should be set in Shape for curly braces!");
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect(rect);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for curly braces!");
        if (gc != nullptr && (side == Side::Left || side == Side::Right))
            {
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);

                // shrink drawing area for wider pens so that they don't
                // go outside it
                drawRect.SetHeight(drawRect.GetHeight() - scaledPen.GetWidth());
                drawRect.SetTop(drawRect.GetTop() + (scaledPen.GetWidth() / 2));
                }

            // cut the rect in half and draw mirrored curls in them
            wxRect upperRect(drawRect), lowerRect(drawRect);
            upperRect.SetHeight(upperRect.GetHeight() / 2);
            lowerRect.SetHeight(lowerRect.GetHeight() / 2);
            lowerRect.SetTop(upperRect.GetBottom());

            if (side == Side::Left)
                {
                // draw the upper curl
                auto upperCurlPath = gc->CreatePath();
                upperCurlPath.MoveToPoint(upperRect.GetTopRight());
                upperCurlPath.AddCurveToPoint(upperRect.GetTopLeft(), upperRect.GetBottomRight(),
                                              upperRect.GetBottomLeft());
                gc->StrokePath(upperCurlPath);

                // draw the lower curl
                auto lowerCurlPath = gc->CreatePath();
                lowerCurlPath.MoveToPoint(lowerRect.GetTopLeft());
                lowerCurlPath.AddCurveToPoint(lowerRect.GetTopRight(), lowerRect.GetBottomLeft(),
                                              lowerRect.GetBottomRight());
                gc->StrokePath(lowerCurlPath);
                }
            else if (side == Side::Right)
                {
                // draw the upper curl
                auto upperCurlPath = gc->CreatePath();
                upperCurlPath.MoveToPoint(upperRect.GetTopLeft());
                upperCurlPath.AddCurveToPoint(upperRect.GetTopRight(), upperRect.GetBottomLeft(),
                                              upperRect.GetBottomRight());
                gc->StrokePath(upperCurlPath);

                // draw the lower curl
                auto lowerCurlPath = gc->CreatePath();
                lowerCurlPath.MoveToPoint(lowerRect.GetTopRight());
                lowerCurlPath.AddCurveToPoint(lowerRect.GetTopLeft(), lowerRect.GetBottomRight(),
                                              lowerRect.GetBottomLeft());
                gc->StrokePath(lowerCurlPath);
                }
            }
        else if (gc != nullptr && (side == Side::Bottom || side == Side::Top))
            {
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);

                // shrink drawing area for wider pens so that they don't
                // go outside it
                drawRect.SetWidth(drawRect.GetWidth() - scaledPen.GetWidth());
                drawRect.SetLeft(drawRect.GetLeft() + (scaledPen.GetWidth() / 2));
                }

            // cut the rect in half and draw mirrored curls in them
            wxRect leftRect(drawRect), rightRect(drawRect);
            leftRect.SetWidth(leftRect.GetWidth() / 2);
            rightRect.SetWidth(rightRect.GetWidth() / 2);
            rightRect.SetLeft(leftRect.GetRight());

            if (side == Side::Bottom)
                {
                // draw the left curl
                auto leftCurlPath = gc->CreatePath();
                leftCurlPath.MoveToPoint(leftRect.GetTopLeft());
                leftCurlPath.AddCurveToPoint(leftRect.GetBottomLeft(), leftRect.GetTopRight(),
                                             leftRect.GetBottomRight());
                gc->StrokePath(leftCurlPath);

                // draw the right curl
                auto rightCurlPath = gc->CreatePath();
                rightCurlPath.MoveToPoint(rightRect.GetBottomLeft());
                rightCurlPath.AddCurveToPoint(rightRect.GetTopLeft(), rightRect.GetBottomRight(),
                                              rightRect.GetTopRight());
                gc->StrokePath(rightCurlPath);
                }
            else if (side == Side::Top)
                {
                // draw the left curl
                auto leftCurlPath = gc->CreatePath();
                leftCurlPath.MoveToPoint(leftRect.GetBottomLeft());
                leftCurlPath.AddCurveToPoint(leftRect.GetTopLeft(), leftRect.GetBottomRight(),
                                             leftRect.GetTopRight());
                gc->StrokePath(leftCurlPath);

                // draw the right curl
                auto lowerCurlPath = gc->CreatePath();
                lowerCurlPath.MoveToPoint(rightRect.GetTopLeft());
                lowerCurlPath.AddCurveToPoint(rightRect.GetBottomLeft(), rightRect.GetTopRight(),
                                              rightRect.GetBottomRight());
                gc->StrokePath(lowerCurlPath);
                }
            }
        }
    } // namespace Wisteria::GraphItems
