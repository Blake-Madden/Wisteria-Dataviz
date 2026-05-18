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
    void ShapeRenderer::DrawImmaculateHeart(const wxRect rect, wxDC& dc) const
        {
        wxRect heartRect{ rect };
        heartRect.SetHeight(heartRect.GetHeight() * math_constants::three_quarters);
        heartRect.Offset(0, rect.GetHeight() * math_constants::quarter);
        DrawHeart(heartRect, dc);

        wxRect flameRect{ rect };
        flameRect.Deflate(flameRect.GetWidth() * 0.24);
        flameRect.SetHeight(rect.GetHeight() * math_constants::half);
        // Anchor to the very top of the bounding box
        flameRect.SetTop(rect.GetTop());
        DrawFlame(flameRect, dc);

        // The heart drawing function uses Bézier curves, meaning it doesn't consume
        // all the rect it was given. Scale down the heart's bounding box to the area
        // that actually has content.
        heartRect.SetWidth(heartRect.GetWidth() * 0.55);
        heartRect.Offset((rect.GetWidth() - heartRect.GetWidth()) / 2, 0);

        wxRect flowerRect{ heartRect };
        flowerRect.SetHeight(heartRect.GetHeight() * math_constants::quarter);
        flowerRect.SetWidth(heartRect.GetWidth() * math_constants::quarter);
        const auto flowerOverlayTolerance{ (flowerRect.GetWidth() * math_constants::half) };
        flowerRect.Offset(-flowerOverlayTolerance,
                          safe_divide<double>(heartRect.GetHeight(), 2) -
                              safe_divide<double>(flowerRect.GetHeight(), 2));
        if (flowerRect.GetWidth() > 0)
            {
            while (flowerRect.GetRight() - flowerOverlayTolerance < heartRect.GetRight())
                {
                DrawBaseFlower(flowerRect, dc,
                               Colors::ColorBrewer::GetColor(Colors::Color::ChapelBlue),
                               Colors::ColorBrewer::GetColor(Colors::Color::Sand));
                flowerRect.Offset(flowerRect.GetWidth() * math_constants::half, 0);
                }
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawImmaculateHeartWithSword(const wxRect rect, wxDC& dc) const
        {
        DrawImmaculateHeart(rect, dc);

        wxRect swordRect{ rect };
        swordRect.Offset(static_cast<int>(swordRect.GetWidth() * math_constants::tenth),
                         static_cast<int>(swordRect.GetHeight() * math_constants::tenth));
        DrawSword(swordRect, dc, ClippingSection::Upper);
        DrawSword(swordRect, dc, ClippingSection::Lower);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSword(const wxRect rect, wxDC& dc) const
        {
        DrawSword(rect, dc, ClippingSection::None);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSword(const wxRect rect, wxDC& dc,
                                  const ClippingSection clippingSection) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCFontChanger fc{ dc };

        const auto centerPt = rect.GetTopLeft() + wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for sword icon!");
        if (gc != nullptr)
            {
            if (clippingSection != ClippingSection::None)
                {
                gc->PushState();
                if (clippingSection == ClippingSection::Upper)
                    {
                    const std::array<wxPoint, 3> corners{
                        wxPoint{ static_cast<int>(GetXPosFromLeft(rect, math_constants::tenth)),
                                 static_cast<int>(GetYPosFromTop(rect, 0.0)) },
                        wxPoint{ static_cast<int>(GetXPosFromLeft(rect, 1.0)),
                                 static_cast<int>(GetYPosFromTop(rect, 0.9)) },
                        rect.GetTopRight()
                    };
                    const wxRegion clipBox(corners.size(), corners.data());
                    gc->Clip(clipBox);
                    }
                else
                    {
                    const std::array<wxPoint, 3> corners{
                        wxPoint{ static_cast<int>(GetXPosFromLeft(rect, 0)),
                                 static_cast<int>(GetYPosFromTop(rect, math_constants::fifth)) },
                        rect.GetBottomLeft(),
                        wxPoint{ static_cast<int>(GetXPosFromLeft(rect, 0.8)),
                                 static_cast<int>(GetYPosFromTop(rect, 1.0)) }
                    };
                    const wxRegion clipBox(corners.size(), corners.data());
                    gc->Clip(clipBox);
                    }
                }

            // blade
            wxRect2DDouble bladeRect{ rect };
            bladeRect.SetHeight(bladeRect.GetHeight() * math_constants::tenth);
            bladeRect.SetWidth(bladeRect.GetWidth() * math_constants::three_quarters);
            bladeRect.Offset(0, (rect.GetHeight() * math_constants::half) -
                                    (bladeRect.GetHeight() * math_constants::half));

            // save current transform matrix state
            gc->PushState();
            gc->Translate(centerPt.x, centerPt.y);

            // this shape is used for other composite shapes, so need to tint instead of
            // using opacity
            gc->SetBrush(TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::AshGrey)));
            // only show outline if larger icon
            gc->SetPen({ TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                         static_cast<int>(ScaleToScreenAndCanvas(1)) });

            gc->Rotate(geometry::degrees_to_radians(-60));

            // note that because we translated to the middle of the drawing area,
            // we need to adjust the points of our middle line back and over from
            // the translated origin
            auto bladePath = gc->CreatePath();
            bladePath.MoveToPoint(
                wxPoint2DDouble{ GetXPosFromLeft(rect, math_constants::fifth) - centerPt.x,
                                 GetYPosFromTop(rect, 0.45) - centerPt.y });
            bladePath.AddLineToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.7) - centerPt.x,
                                                      GetYPosFromTop(rect, 0.45) - centerPt.y });

            bladePath.AddLineToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.7) - centerPt.x,
                                                      GetYPosFromTop(rect, 0.55) - centerPt.y });
            bladePath.AddLineToPoint(
                wxPoint2DDouble{ GetXPosFromLeft(rect, math_constants::fifth) - centerPt.x,
                                 GetYPosFromTop(rect, 0.55) - centerPt.y });
            // tip of the blade
            bladePath.AddLineToPoint(
                wxPoint2DDouble{ GetXPosFromLeft(rect, math_constants::tenth) - centerPt.x,
                                 GetYPosFromTop(rect, math_constants::half) - centerPt.y });

            bladePath.CloseSubpath();
            gc->FillPath(bladePath);
            gc->StrokePath(bladePath);

            // hilt
            gc->SetBrush(
                TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::GoldenYellow)));
            auto hiltPath = gc->CreatePath();
            hiltPath.MoveToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.7) - centerPt.x,
                                                  GetYPosFromTop(rect, 0.45) - centerPt.y });
            hiltPath.AddLineToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.9) - centerPt.x,
                                                     GetYPosFromTop(rect, 0.45) - centerPt.y });

            hiltPath.AddLineToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.9) - centerPt.x,
                                                     GetYPosFromTop(rect, 0.55) - centerPt.y });
            hiltPath.AddLineToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.7) - centerPt.x,
                                                     GetYPosFromTop(rect, 0.55) - centerPt.y });

            hiltPath.CloseSubpath();
            gc->FillPath(hiltPath);
            gc->StrokePath(hiltPath);

            // hilt guard
            auto hiltGuardPath = gc->CreatePath();
            hiltGuardPath.MoveToPoint(wxPoint2DDouble{ GetXPosFromLeft(rect, 0.7) - centerPt.x,
                                                       GetYPosFromTop(rect, 0.35) - centerPt.y });
            hiltGuardPath.AddLineToPoint(
                wxPoint2DDouble{ GetXPosFromLeft(rect, math_constants::three_quarters) - centerPt.x,
                                 GetYPosFromTop(rect, 0.35) - centerPt.y });

            hiltGuardPath.AddLineToPoint(
                wxPoint2DDouble{ GetXPosFromLeft(rect, math_constants::three_quarters) - centerPt.x,
                                 GetYPosFromTop(rect, 0.65) - centerPt.y });
            hiltGuardPath.AddLineToPoint(wxPoint2DDouble{
                GetXPosFromLeft(rect, 0.7) - centerPt.x, GetYPosFromTop(rect, 0.65) - centerPt.y });

            hiltGuardPath.CloseSubpath();
            gc->FillPath(hiltGuardPath);
            gc->StrokePath(hiltGuardPath);

            // restore transform matrix
            gc->PopState();

            if (clippingSection != ClippingSection::None)
                {
                gc->PopState();
                }
            }
        }
    } // namespace Wisteria::GraphItems
