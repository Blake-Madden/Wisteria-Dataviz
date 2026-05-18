///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_buildings.cpp
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
    void ShapeRenderer::DrawBarn(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(2));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for barn!");
        if (gc != nullptr)
            {
            gc->SetPen(
                wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                       static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, math_constants::half),
                GetXPosFromLeft(rect, math_constants::full),
                GetYPosFromTop(rect, math_constants::half),
                ApplyColorOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::FireEngineRed))),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::FireEngineRed))));

            const wxRect barnRect{ drawRect };
            const std::vector<wxPoint> barnPoints = {
                barnRect.GetBottomLeft(),
                wxPoint(GetXPosFromLeft(barnRect, 0),
                        GetYPosFromTop(barnRect, math_constants::half)),
                wxPoint(GetXPosFromLeft(barnRect, math_constants::tenth),
                        GetYPosFromTop(barnRect, math_constants::fourth)),
                wxPoint(GetXPosFromLeft(barnRect, math_constants::half),
                        GetYPosFromTop(barnRect, 0)),
                wxPoint(GetXPosFromLeft(barnRect, math_constants::full - math_constants::tenth),
                        GetYPosFromTop(barnRect, math_constants::fourth)),
                wxPoint(GetXPosFromLeft(barnRect, math_constants::full),
                        GetYPosFromTop(barnRect, math_constants::half)),
                wxPoint(GetXPosFromLeft(barnRect, math_constants::full),
                        GetYPosFromTop(barnRect, math_constants::full))
            };

            wxGraphicsPath barnPath = gc->CreatePath();

            barnPath.MoveToPoint(barnPoints[0].x, barnPoints[0].y);
            barnPath.AddLineToPoint(barnPoints[1].x, barnPoints[1].y);
            barnPath.AddLineToPoint(barnPoints[2].x, barnPoints[2].y);
            barnPath.AddLineToPoint(barnPoints[3].x, barnPoints[3].y);
            barnPath.AddLineToPoint(barnPoints[4].x, barnPoints[4].y);
            barnPath.AddLineToPoint(barnPoints[5].x, barnPoints[5].y);
            barnPath.AddLineToPoint(barnPoints[6].x, barnPoints[6].y);

            barnPath.CloseSubpath();
            gc->StrokePath(barnPath);
            gc->FillPath(barnPath);

            // draw lines across barn to look like boards
            gc->SetPen(wxPen{ Colors::ColorContrast::ChangeOpacity(
                                  Colors::ColorBrewer::GetColor(Colors::Color::DarkGray), 75),
                              static_cast<int>(ScaleToScreenAndCanvas(math_constants::quarter)) });
            gc->PushState();
            const wxRegion barnRegion{ static_cast<size_t>(barnPoints.size()), barnPoints.data() };
            gc->Clip(barnRegion);
            wxCoord currentY{ barnRect.GetTop() };
            while (currentY < barnRect.GetBottom())
                {
                gc->StrokeLine(barnRect.GetLeft(), currentY, barnRect.GetRight(), currentY);
                currentY +=
                    static_cast<wxCoord>(ScaleToScreenAndCanvas(GetScaling() <= 2.0 ? 4 : 2));
                }
            gc->PopState();

            // roof
            gc->SetPen(
                wxPenInfo{ TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                           static_cast<int>(ScaleToScreenAndCanvas(1.5)) }
                    .Join(wxPenJoin::wxJOIN_MITER));
            gc->StrokeLine(barnPoints[1].x, barnPoints[1].y, barnPoints[2].x, barnPoints[2].y);
            gc->StrokeLine(barnPoints[2].x, barnPoints[2].y, barnPoints[3].x, barnPoints[3].y);
            gc->StrokeLine(barnPoints[3].x, barnPoints[3].y, barnPoints[4].x, barnPoints[4].y);
            gc->StrokeLine(barnPoints[4].x, barnPoints[4].y, barnPoints[5].x, barnPoints[5].y);

            // alley doors
            gc->SetPen(wxPenInfo{ Colors::ColorBrewer::GetColor(Colors::Color::White),
                                  static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) }
                           .Join(wxPenJoin::wxJOIN_MITER));
            gc->SetBrush(wxColour{ 0, 0, 0, 0 });

            wxRect doorRect{ barnRect };
            doorRect.SetWidth(doorRect.GetWidth() * math_constants::half);
            doorRect.SetHeight(doorRect.GetHeight() * 0.4);
            doorRect.Offset((barnRect.GetWidth() * math_constants::half) -
                                (doorRect.GetWidth() * math_constants::half),
                            (barnRect.GetHeight() * 0.6) -
                                static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)));
            gc->DrawRectangle(doorRect.x, doorRect.y, doorRect.GetWidth(), doorRect.GetHeight());

            gc->StrokeLine(doorRect.GetBottomLeft().x, doorRect.GetBottomLeft().y,
                           doorRect.GetTopRight().x, doorRect.GetTopRight().y);
            gc->StrokeLine(doorRect.GetTopLeft().x, doorRect.GetTopLeft().y,
                           doorRect.GetBottomRight().x, doorRect.GetBottomRight().y);
            gc->SetPen(
                wxPenInfo{ Colors::ColorBrewer::GetColor(Colors::Color::White),
                           static_cast<int>(ScaleToScreenAndCanvas(math_constants::quarter)) }
                    .Join(wxPenJoin::wxJOIN_MITER));
            gc->StrokeLine(doorRect.GetTopLeft().x + (doorRect.GetWidth() * math_constants::half),
                           doorRect.GetTopLeft().y,
                           doorRect.GetBottomLeft().x +
                               (doorRect.GetWidth() * math_constants::half),
                           doorRect.GetBottomLeft().y);

            // loft opening
            gc->SetPen(wxColour{ 0, 0, 0, 0 });
            gc->SetBrush(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)));
            const wxSize originalSize{ doorRect.GetSize() };
            doorRect.SetWidth(doorRect.GetWidth() * math_constants::half);
            doorRect.SetHeight(doorRect.GetHeight() * math_constants::half);
            doorRect.Offset((originalSize.GetWidth() * math_constants::half) -
                                (doorRect.GetWidth() * math_constants::half),
                            -(doorRect.GetHeight() * 1.5));
            gc->DrawRectangle(doorRect.x, doorRect.y, doorRect.GetWidth(), doorRect.GetHeight());

            // hay bale
            wxRect hayRect{ doorRect };
            hayRect.SetWidth(hayRect.GetWidth() * math_constants::half);
            hayRect.SetHeight(hayRect.GetHeight() * math_constants::half);
            hayRect.Offset(0, doorRect.GetHeight() - hayRect.GetHeight());

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, math_constants::half),
                GetXPosFromLeft(rect, math_constants::full),
                GetYPosFromTop(rect, math_constants::half),
                ApplyColorOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::Yellow))),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Yellow))));
            gc->DrawRectangle(hayRect.x, hayRect.y, hayRect.GetWidth(), hayRect.GetHeight());

            // draw the loft opening's frame
            gc->SetPen(
                wxPenInfo{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White)),
                           static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) }
                    .Join(wxPenJoin::wxJOIN_MITER));
            gc->SetBrush(wxColour{ 0, 0, 0, 0 });
            gc->DrawRectangle(doorRect.x, doorRect.y, doorRect.GetWidth(), doorRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFarm(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(2));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for farm!");
        if (gc != nullptr)
            {
            // silo
            gc->SetPen(
                wxPenInfo{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                           static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) }
                    .Cap(wxPenCap::wxCAP_BUTT));

            wxRect siloRect{ drawRect };
            siloRect.SetWidth(siloRect.GetWidth() * math_constants::half);

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(siloRect, 0), GetYPosFromTop(siloRect, math_constants::half),
                GetXPosFromLeft(siloRect, math_constants::full),
                GetYPosFromTop(siloRect, math_constants::half),
                ApplyColorOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::LightGray))),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::LightGray))));

            wxRect siloBodyRect{ siloRect };
            siloBodyRect.SetHeight(siloBodyRect.GetHeight() * math_constants::three_fourths);
            siloBodyRect.Offset(0, siloRect.GetHeight() - siloBodyRect.GetHeight());

            gc->DrawRectangle(siloBodyRect.x, siloBodyRect.y, siloBodyRect.GetWidth(),
                              siloBodyRect.GetHeight());

            // ladder
            wxRect ladderRect{ siloBodyRect };
            ladderRect.Offset(siloBodyRect.GetWidth() * math_constants::fifth, 0);
            ladderRect.SetWidth(ladderRect.GetWidth() * math_constants::third);
            gc->StrokeLine(ladderRect.x, ladderRect.y, ladderRect.x,
                           ladderRect.y + ladderRect.GetHeight());
            gc->StrokeLine(ladderRect.x + ladderRect.GetWidth(), ladderRect.y,
                           ladderRect.x + ladderRect.GetWidth(),
                           ladderRect.y + ladderRect.GetHeight());

            int currentY{ ladderRect.GetTop() };
            // clang-format off
            wxCLANG_WARNING_SUPPRESS(unused-but-set-variable);
            // clang-format on
            int currentLine{ 0 };
            while (currentY < ladderRect.GetBottom())
                {
                gc->StrokeLine(ladderRect.GetLeft(), currentY, ladderRect.GetRight(), currentY);
                currentY += ScaleToScreenAndCanvas(GetScaling() <= 2.0 ? 2 : 1);
                ++currentLine;
                }
            // clang-format off
            wxCLANG_WARNING_RESTORE(unused-but-set-variable);
            // clang-format on
            // top of silo
            wxGraphicsPath siloLidPath = gc->CreatePath();

            siloLidPath.MoveToPoint(wxPoint(siloBodyRect.x, siloBodyRect.y));
            // left side
            siloLidPath.AddCurveToPoint(
                wxPoint(siloBodyRect.x, siloRect.y),
                wxPoint(siloBodyRect.x + siloBodyRect.GetWidth(), siloRect.y),
                wxPoint(siloBodyRect.x + siloBodyRect.GetWidth(), siloBodyRect.y));

            siloLidPath.CloseSubpath();
            gc->FillPath(siloLidPath);
            gc->StrokePath(siloLidPath);
            }

        wxRect barnRect{ rect };
        barnRect.SetWidth(barnRect.GetWidth() * math_constants::three_fourths);
        barnRect.SetHeight(barnRect.GetHeight() * math_constants::three_fourths);
        barnRect.Offset(rect.GetWidth() - barnRect.GetWidth(),
                        rect.GetHeight() - barnRect.GetHeight());
        DrawBarn(barnRect, dc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFactory(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for factory!");
        if (gc != nullptr)
            {
            // smoke
            wxRect smokeRect{ rect };
            smokeRect.Deflate(ScaleToScreenAndCanvas(2));
            smokeRect.SetTop(rect.GetTop());
            smokeRect.SetWidth(smokeRect.GetWidth() * math_constants::fifth * 2);
            smokeRect.SetHeight(rect.GetHeight() * math_constants::fifth);

            gc->SetPen(
                wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                       static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });
            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(smokeRect, 0), GetYPosFromTop(smokeRect, math_constants::full),
                GetXPosFromLeft(smokeRect, 0), GetYPosFromTop(smokeRect, 0),
                Colors::ColorContrast::ChangeOpacity(
                    Colors::ColorContrast::ShadeOrTint(
                        Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)),
                    200),
                Colors::ColorContrast::ChangeOpacity(
                    Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack), 50)));

            wxGraphicsPath smokePath = gc->CreatePath();

            smokePath.MoveToPoint(smokeRect.GetBottomLeft());
            smokePath.AddQuadCurveToPoint(GetXPosFromLeft(smokeRect, math_constants::tenth),
                                          GetYPosFromTop(smokeRect, math_constants::half),
                                          GetXPosFromLeft(smokeRect, -math_constants::fifth),
                                          GetYPosFromTop(smokeRect, 0));
            smokePath.AddQuadCurveToPoint(
                GetXPosFromLeft(smokeRect, 1.4), GetYPosFromTop(smokeRect, 0),
                GetXPosFromLeft(smokeRect, 1.4), GetYPosFromTop(smokeRect, 0));
            smokePath.AddQuadCurveToPoint(GetXPosFromLeft(smokeRect, 1.4),
                                          GetYPosFromTop(smokeRect, math_constants::quarter),
                                          GetXPosFromLeft(smokeRect, math_constants::full),
                                          GetYPosFromTop(smokeRect, math_constants::half));
            smokePath.AddQuadCurveToPoint(GetXPosFromLeft(smokeRect, 1.2),
                                          GetYPosFromTop(smokeRect, 0.6),
                                          GetXPosFromLeft(smokeRect, math_constants::full),
                                          GetYPosFromTop(smokeRect, math_constants::full));
            smokePath.AddQuadCurveToPoint(
                GetXPosFromLeft(smokeRect, 1.1), GetYPosFromTop(smokeRect, 1.1),
                GetXPosFromLeft(smokeRect, math_constants::full), GetYPosFromTop(smokeRect, 1.4));
            smokePath.AddQuadCurveToPoint(
                GetXPosFromLeft(smokeRect, 0), GetYPosFromTop(smokeRect, 1.2),
                GetXPosFromLeft(smokeRect, 0), GetYPosFromTop(smokeRect, 1.2));

            smokePath.CloseSubpath();
            gc->FillPath(smokePath);

            // smoke stacks
            wxRect chimneyRect{ rect };
            chimneyRect.Deflate(ScaleToScreenAndCanvas(2));
            chimneyRect.SetWidth(chimneyRect.GetWidth() * math_constants::fifth);
            chimneyRect.SetHeight(chimneyRect.GetHeight() * 0.9);

            auto setChimneyBrush = [&](const wxRect& cr)
            {
                gc->SetBrush(gc->CreateLinearGradientBrush(
                    GetXPosFromLeft(cr, -math_constants::quarter),
                    GetYPosFromTop(cr, math_constants::half),
                    GetXPosFromLeft(cr, math_constants::full),
                    GetYPosFromTop(cr, math_constants::half),
                    TintIfUsingOpacity(Colors::ColorContrast::ShadeOrTint(
                        Colors::ColorBrewer::GetColor(Colors::Color::BrickRed))),
                    TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::BrickRed))));
            };

            setChimneyBrush(chimneyRect);
            gc->DrawRectangle(chimneyRect.GetX(), chimneyRect.GetY(), chimneyRect.GetWidth(),
                              chimneyRect.GetHeight());

            const auto yOffset{ chimneyRect.GetHeight() * math_constants::fifth };
            chimneyRect.SetHeight(chimneyRect.GetHeight() * 0.8);
            chimneyRect.Offset(chimneyRect.GetWidth(), yOffset);

            setChimneyBrush(chimneyRect);
            gc->DrawRectangle(chimneyRect.GetX(), chimneyRect.GetY(), chimneyRect.GetWidth(),
                              chimneyRect.GetHeight());
            }

        DrawBaseBuilding(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::BrickRed));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBaseBuilding(const wxRect rect, wxDC& dc, const wxColour& color) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(2));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for building!");
        if (gc != nullptr)
            {
            const auto drawWindow = [&gc, this](const wxRect drawingRect)
            {
                gc->SetPen(
                    wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                           static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });

                gc->SetBrush(gc->CreateLinearGradientBrush(
                    GetXPosFromLeft(drawingRect, 0),
                    GetYPosFromTop(drawingRect, math_constants::half),
                    GetXPosFromLeft(drawingRect, 2),
                    GetYPosFromTop(drawingRect, math_constants::half),
                    ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::BlizzardBlue)),
                    ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White))));

                gc->DrawRectangle(drawingRect.GetX(), drawingRect.GetY(), drawingRect.GetWidth(),
                                  drawingRect.GetHeight());

                gc->StrokeLine(GetXPosFromLeft(drawingRect, 0),
                               GetYPosFromTop(drawingRect, math_constants::half),
                               GetXPosFromLeft(drawingRect, math_constants::full),
                               GetYPosFromTop(drawingRect, math_constants::half));
                gc->StrokeLine(GetXPosFromLeft(drawingRect, math_constants::half),
                               GetYPosFromTop(drawingRect, 0),
                               GetXPosFromLeft(drawingRect, math_constants::half),
                               GetYPosFromTop(drawingRect, math_constants::full));
            };

            gc->SetPen(
                wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                       static_cast<int>(ScaleToScreenAndCanvas(math_constants::half)) });

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(rect, -math_constants::quarter),
                GetYPosFromTop(rect, math_constants::half),
                GetXPosFromLeft(rect, math_constants::full),
                GetYPosFromTop(rect, math_constants::half),
                TintIfUsingOpacity(Colors::ColorContrast::ShadeOrTint(color)),
                TintIfUsingOpacity(color)));

            wxRect mainBuildingRect{ drawRect };
            const auto yOffset{ mainBuildingRect.GetHeight() * math_constants::third };
            mainBuildingRect.SetHeight(mainBuildingRect.GetHeight() * math_constants::two_thirds);
            mainBuildingRect.Offset(0, yOffset);
            gc->DrawRectangle(mainBuildingRect.GetX(), mainBuildingRect.GetY(),
                              mainBuildingRect.GetWidth(), mainBuildingRect.GetHeight());

            wxRect windowRect{ mainBuildingRect };
            windowRect.SetWidth(mainBuildingRect.GetWidth() * math_constants::third);
            windowRect.SetHeight(mainBuildingRect.GetHeight() * math_constants::third);
            windowRect.SetLeft(mainBuildingRect.GetLeft() +
                               (mainBuildingRect.GetWidth() * math_constants::tenth));
            windowRect.SetTop(mainBuildingRect.GetTop() + (mainBuildingRect.GetHeight() * 0.15));
            drawWindow(windowRect);

            const auto newWindowX{ (mainBuildingRect.GetRight() -
                                    (mainBuildingRect.GetWidth() * math_constants::tenth)) -
                                   windowRect.GetWidth() };
            const auto oldWindowX{ windowRect.GetX() };
            windowRect.Offset(newWindowX - windowRect.GetX(), 0);
            drawWindow(windowRect);

            windowRect.SetX(oldWindowX);
            windowRect.Offset(0, windowRect.GetHeight() +
                                     (mainBuildingRect.GetWidth() * math_constants::tenth));
            drawWindow(windowRect);

            windowRect.Offset(newWindowX - windowRect.GetX(), 0);
            windowRect.SetBottom(mainBuildingRect.GetBottom());
            const auto doorOffset{ windowRect.GetWidth() * math_constants::quarter };
            windowRect.SetWidth(windowRect.GetWidth() * math_constants::half);
            windowRect.Offset(doorOffset, 0);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(windowRect, 0), GetYPosFromTop(windowRect, math_constants::half),
                GetXPosFromLeft(windowRect, 2), GetYPosFromTop(windowRect, math_constants::half),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White))));
            gc->DrawRectangle(windowRect.GetX(), windowRect.GetY(), windowRect.GetWidth(),
                              windowRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawOffice(const wxRect rect, wxDC& dc) const
        {
        DrawBaseBuilding(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::AntiqueWhite));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHouse(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for house!");
        if (gc != nullptr)
            {
            // chimney
            wxRect chimneyRect{ rect };
            chimneyRect.Deflate(ScaleToScreenAndCanvas(2));
            chimneyRect.SetWidth(chimneyRect.GetWidth() * math_constants::fifth);
            chimneyRect.SetHeight(chimneyRect.GetHeight() * 0.9);

            gc->SetPen(
                wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                       static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(chimneyRect, -math_constants::quarter),
                GetYPosFromTop(chimneyRect, math_constants::half),
                GetXPosFromLeft(chimneyRect, math_constants::full),
                GetYPosFromTop(chimneyRect, math_constants::half),
                ApplyColorOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::BrickRed))),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::BrickRed))));
            gc->DrawRectangle(chimneyRect.GetX(), chimneyRect.GetY(), chimneyRect.GetWidth(),
                              chimneyRect.GetHeight());

            // house body
            DrawBaseBuilding(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::PastelYellow));

            // roof
            wxRect roofRect{ rect };
            roofRect.SetHeight((rect.GetHeight() * math_constants::third) +
                               ScaleToScreenAndCanvas(2));

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(roofRect, -math_constants::quarter),
                GetYPosFromTop(roofRect, math_constants::half),
                GetXPosFromLeft(roofRect, math_constants::full),
                GetYPosFromTop(roofRect, math_constants::half),
                TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Brownstone)),
                TintIfUsingOpacity(Colors::ColorContrast::ShadeOrTint(
                    Colors::ColorBrewer::GetColor(Colors::Color::Brownstone)))));

            wxGraphicsPath roofPath = gc->CreatePath();

            roofPath.MoveToPoint(roofRect.GetBottomLeft());
            roofPath.AddLineToPoint(GetXPosFromLeft(roofRect, math_constants::half),
                                    GetYPosFromTop(roofRect, 0));
            roofPath.AddLineToPoint(GetXPosFromLeft(roofRect, math_constants::full),
                                    GetYPosFromTop(roofRect, math_constants::full));

            roofPath.CloseSubpath();
            gc->FillPath(roofPath);
            gc->StrokePath(roofPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawWarningRoadSign(const wxRect rect, wxDC& dc) const
        {
        const wxDCBrushChanger bc(
            dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow)));

        const auto iconRadius = GetRadius(rect);

            // sign post
            {
            const std::array<wxPoint, 2> pt = { rect.GetTopLeft() +
                                                    // top of post is in the middle of the sign
                                                    // so that pen cap doesn't appear above sign
                                                    wxSize(rect.GetWidth() / 2, iconRadius),
                                                // bottom
                                                rect.GetBottomLeft() +
                                                    wxSize(rect.GetWidth() / 2, 0) };
            const auto signPostWidth =
                std::min<int>(ScaleToScreenAndCanvas(3), (rect.GetWidth() / 15));
                // dark gray outline of sign post used to contrast black sign post
                // against a possibly dark background
                {
                const wxDCPenChanger pc{
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray),
                                        signPostWidth + ScaleToScreenAndCanvas(1))
                                  .Cap(wxPenCap::wxCAP_BUTT))
                };
                dc.DrawLine(pt[0], pt[1]);
                }
                // actual sign post
                {
                const wxDCPenChanger pc{
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::SlateGray),
                                        signPostWidth)
                                  .Cap(wxPenCap::wxCAP_BUTT))
                };
                dc.DrawLine(pt[0], pt[1]);
                }
            }
            // sign
            {
            const auto signOutlineWidth = rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2;
            const wxDCPenChanger pc{ dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                               ScaleToScreenAndCanvas(signOutlineWidth)) };
            const auto signHeight = rect.GetHeight() * math_constants::third;
            const auto signRadius = std::min(signHeight, iconRadius);
            const auto circleCenter = rect.GetLeftTop() + wxSize(rect.GetWidth() / 2, signRadius);
            const std::array<wxPoint, 4> points = { circleCenter + wxPoint(0, -signRadius),
                                                    circleCenter + wxPoint(signRadius, 0),
                                                    circleCenter + wxPoint(0, signRadius),
                                                    circleCenter + wxPoint(-signRadius, 0) };
            dc.DrawPolygon(points.size(), points.data());
            // ! label
            Label bangLabel(GraphItemInfo{ L"!" }
                                .Pen(wxNullPen)
                                .AnchorPoint(circleCenter)
                                .Anchoring(Anchoring::Center)
                                .LabelAlignment(TextAlignment::Centered)
                                .DPIScaling(GetDPIScaleFactor()));
            bangLabel.SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black));
            bangLabel.GetFont().MakeBold();
            bangLabel.SetBoundingBox(
                wxRect(rect.GetLeftTop(),
                       wxSize(rect.GetWidth(), rect.GetHeight() * math_constants::two_thirds)),
                dc, GetScaling());
            bangLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            bangLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            bangLabel.Draw(dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGoSign(const wxRect rect, wxDC& dc) const
        {
        const wxDCBrushChanger bc(
            dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow)));

            // sign post
            {
            const auto iconRadius = GetRadius(rect);
            const std::array<wxPoint, 2> pt = {
                rect.GetTopLeft() + wxSize(rect.GetWidth() / 2, iconRadius),
                // bottom
                rect.GetBottomLeft() + wxSize(rect.GetWidth() / 2, 0)
            };
            const auto signPostWidth =
                std::min<int>(ScaleToScreenAndCanvas(3), (rect.GetWidth() / 15));
                // dark gray outline of sign post
                {
                const wxDCPenChanger pc(
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray),
                                        signPostWidth + ScaleToScreenAndCanvas(1))
                                  .Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
                // actual sign post
                {
                const wxDCPenChanger pc(
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::SlateGray),
                                        signPostWidth)
                                  .Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
            }
            // sign
            {
            const auto signRect =
                wxRect(rect.GetLeftTop(),
                       wxSize(rect.GetWidth(), rect.GetHeight() * math_constants::two_thirds));
            DrawCircularSign(signRect, Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen),
                             // TRANSLATORS: A GO sign, as in OK to proceed.
                             _(L"GO"), dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBanner(const wxRect rect, wxDC& dc) const
        {
            // sign posts
            {
            std::array<wxPoint, 2> pt = { rect.GetTopLeft(), rect.GetBottomLeft() };
            const auto signPostWidth =
                std::min<int>(ScaleToScreenAndCanvas(8), (rect.GetWidth() / 5));
            pt[0].x += signPostWidth / 2;
            pt[1].x += signPostWidth / 2;

            const auto drawPost = [&]()
            {
                // white outline of sign post used to contrast black sign post
                // against a possibly dark background
                {
                const wxDCPenChanger pc{
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::White),
                                        signPostWidth + ScaleToScreenAndCanvas(1))
                                  .Cap(wxPenCap::wxCAP_BUTT))
                };
                dc.DrawLine(pt[0], pt[1]);
                }
                // actual sign post
                {
                const wxDCPenChanger pc{
                    dc, wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::SlateGray),
                                        signPostWidth)
                                  .Cap(wxPenCap::wxCAP_BUTT))
                };
                dc.DrawLine(pt[0], pt[1]);
                }
            };

            drawPost();
            pt[0].x = rect.GetRight() - signPostWidth / 2;
            pt[1].x = rect.GetRight() - signPostWidth / 2;
            drawPost();
            }
            // sign
            {
            auto anchorPt = rect.GetTopLeft();
            anchorPt.y += rect.GetHeight() * math_constants::twentieth;
            Label bannerLabel(
                GraphItemInfo{ GetGraphItemInfo().GetText() }
                    .Pen(wxPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::Black), 1)))
                    .FontBackgroundColor(GetGraphItemInfo().GetBrush().GetColour())
                    .FontColor(GetGraphItemInfo().GetPen().GetColour())
                    .Anchoring(Anchoring::TopLeftCorner)
                    .LabelAlignment(TextAlignment::Centered)
                    .DPIScaling(GetDPIScaleFactor()));
            bannerLabel.GetFont().MakeBold();
            bannerLabel.SetBoundingBox(
                wxRect(anchorPt, wxSize(rect.GetWidth(), rect.GetHeight() * math_constants::third)),
                dc, GetScaling());
            bannerLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            bannerLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            bannerLabel.Draw(dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGeoMarker(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const wxRect dcRect{ rect };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for geo marker!");
        if (gc != nullptr)
            {
            wxPen scaledPen = GetGraphItemInfo().GetPen();
            if (scaledPen.IsOk())
                {
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                }

            gc->SetPen(scaledPen);
            gc->SetBrush(GetGraphItemInfo().GetBrush());
            auto marker = gc->CreatePath();
            // bottom middle, stretched out to both top corners
            marker.MoveToPoint(GetXPosFromLeft(dcRect, math_constants::half),
                               GetYPosFromTop(dcRect, 1));
            marker.AddCurveToPoint(GetXPosFromLeft(dcRect, -math_constants::three_quarters),
                                   GetYPosFromTop(dcRect, -math_constants::quarter),
                                   GetXPosFromLeft(dcRect, 1.75),
                                   GetYPosFromTop(dcRect, -math_constants::quarter),
                                   GetXPosFromLeft(dcRect, math_constants::half),
                                   GetYPosFromTop(dcRect, math_constants::full));

            marker.CloseSubpath();
            gc->FillPath(marker);
            gc->StrokePath(marker);

            // outer ring in center of head
            wxRect topRect = dcRect;
            topRect.SetHeight(topRect.GetHeight() * math_constants::third);
            topRect.SetWidth(topRect.GetHeight()); // make it a square
            topRect.SetX(topRect.GetX() + ((dcRect.GetWidth() / 2) - (topRect.GetWidth() / 2)));
            topRect.SetY(topRect.GetY() + (topRect.GetHeight() * math_constants::two_thirds));

            gc->SetBrush(wxBrush(
                Colors::ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour())));
            gc->SetPen(wxPen(
                Colors::ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour())));
            gc->DrawEllipse(topRect.GetTopLeft().x, topRect.GetTopLeft().y, topRect.GetWidth(),
                            topRect.GetHeight());

            topRect.Deflate(topRect.GetWidth() * math_constants::third);
            gc->SetBrush(Colors::ColorBrewer::GetColor(Colors::Color::White));
            gc->SetPen(Colors::ColorBrewer::GetColor(Colors::Color::White));
            gc->DrawEllipse(topRect.GetTopLeft().x, topRect.GetTopLeft().y, topRect.GetWidth(),
                            topRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCurvingRoad(const wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcWrap{ &dc, rect };
        wxGraphicsContext* gc = gcWrap.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        // clip so stroke caps are cut flat at the edges of rect
        gc->PushState();
        gc->Clip(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

        const auto width = static_cast<double>(rect.GetWidth());
        const auto height = static_cast<double>(rect.GetHeight());
        // Base scale factor for size-independent drawing.
        // Derived from the smaller rect dimension (clamped ≥1) so all widths,
        // offsets, and tapers scale proportionally to the available space.
        const double baseScale = std::max(1.0, std::min(width, height));

        // perspective taper
        const double roadWidthNear = baseScale * 0.40;
        const double roadWidthFar = baseScale * 0.12;

        const double shoulderPad = baseScale * 0.035;

        // dashed centerline: thin, constant width
        const double laneWidth = std::max(1.0, (roadWidthNear + roadWidthFar) * 0.05);

        // ---- left->right spline that climbs upward; sway only in X ------------
        constexpr int NODES{ 6 };
        const auto stepX = safe_divide<double>(width, (NODES - 1));

        const double baseY0 = rect.GetBottom() - (height * 0.12); // near
        const double baseY1 = rect.GetTop() + (height * 0.02);    // far

        const double ampMax = std::max(0.0, (width * 0.45) - (roadWidthNear * 0.6));

        // linear interpolation
        const auto lerp = [](double a, double b, double t) noexcept { return a + ((b - a) * t); };

        const auto anchorAt = [&](int i) -> wxPoint2DDouble
        {
            const auto t = safe_divide<double>(i, (NODES - 1)); // 0..1 left->right
            const double baseX =
                rect.GetLeft() - (stepX * math_constants::quarter) + (i * stepX * 1.05);
            const double baseY = lerp(baseY0, baseY1, t); // climbs upward
            const double amp = ampMax * (1.0 - t * 0.55); // sway fades with distance
            const double dir = (i % 2 == 0) ? -1.0 : 1.0;
            return { baseX + (dir * amp), baseY };
        };

        std::vector<wxPoint2DDouble> points;
        points.reserve(NODES + 2);
        points.push_back(anchorAt(0));
        for (int i = 0; i < NODES; ++i)
            {
            points.push_back(anchorAt(i));
            }
        points.push_back(anchorAt(NODES - 1));

        // sample Catmull–Rom into a polyline
        const auto catmullPoint = [&](int i, double u) -> wxPoint2DDouble
        {
            const wxPoint2DDouble& p0 = points[i - 1];
            const wxPoint2DDouble& p1 = points[i];
            const wxPoint2DDouble& p2 = points[i + 1];
            const wxPoint2DDouble& p3 = points[i + 2];

            const double t = 0.55, u2 = u * u, u3 = u2 * u;

            const double m1x = (p2.m_x - p0.m_x) * (t / 2.0);
            const double m1y = (p2.m_y - p0.m_y) * (t / 2.0);
            const double m2x = (p3.m_x - p1.m_x) * (t / 2.0);
            const double m2y = (p3.m_y - p1.m_y) * (t / 2.0);

            const double h00 = ((2 * u3) - (3 * u2) + 1);
            const double h10 = (u3 - (2 * u2) + u);
            const double h01 = ((-2 * u3) + (3 * u2));
            const double h11 = (u3 - u2);

            return { (h00 * p1.m_x) + (h10 * m1x) + (h01 * p2.m_x) + (h11 * m2x),
                     (h00 * p1.m_y) + (h10 * m1y) + (h01 * p2.m_y) + (h11 * m2y) };
        };

        std::vector<wxPoint2DDouble> samples;
        samples.reserve(((NODES - 1) * 18) + 1);
        for (int i = 1; i < static_cast<int>(points.size()) - 2; ++i)
            {
            constexpr int SEGS_PER_SPAN{ 18 };
            for (int j = 0; j < SEGS_PER_SPAN; ++j)
                {
                const auto yVal = safe_divide<double>(j, SEGS_PER_SPAN);
                samples.push_back(catmullPoint(i, yVal));
                }
            }
        samples.push_back(points[points.size() - 2]);

        // build one continuous GC path from samples (for shadow and lane)
        wxGraphicsPath splinePath = gc->CreatePath();
        splinePath.MoveToPoint(samples.front().m_x, samples.front().m_y);
        for (size_t i = 1; i < samples.size(); ++i)
            {
            splinePath.AddLineToPoint(samples[i].m_x, samples[i].m_y);
            }

        // helper to draw tapered strokes as short segments (for shoulder/asphalt/shadow)
        const auto strokeTapered = [&](const wxColour& col, double wNear, double wFar)
        {
            for (size_t i = 1; i < samples.size(); ++i)
                {
                const auto t = safe_divide<double>(i, (samples.size() - 1));
                const double w = lerp(wNear, wFar, t);
                const wxGraphicsPen pen =
                    gc->CreatePen(wxGraphicsPenInfo{ col, w }.Cap(wxCAP_ROUND).Join(wxJOIN_ROUND));
                gc->SetPen(pen);
                wxGraphicsPath seg = gc->CreatePath();
                seg.MoveToPoint(samples[i - 1].m_x, samples[i - 1].m_y);
                seg.AddLineToPoint(samples[i].m_x, samples[i].m_y);
                gc->StrokePath(seg);
                }
        };

            // ---- hard shadow: same as outline, slightly offset to the right ----------
            {
            gc->PushState();

            // push the entire tapered stroke slightly to the right
            const double nudge = baseScale * 0.03; // small offset
            gc->Translate(nudge, 0.0);

            // darker, subtle version of the outline
            const wxColour hardShadowCol{ 0, 0, 0, 20 };

            // just reuse the existing tapered stroke logic
            strokeTapered(hardShadowCol, roadWidthNear + shoulderPad, roadWidthFar + shoulderPad);

            gc->PopState();
            }

        // ---- shoulders (tapered) ----------------------------------------------
        const wxColour shoulderCol{ 226, 232, 242 };
        strokeTapered(shoulderCol, roadWidthNear + shoulderPad, roadWidthFar + shoulderPad);

        // ---- asphalt (tapered) -------------------------------------------------
        const wxColour asphalt{ 28, 31, 38 };
        strokeTapered(asphalt, roadWidthNear * 1.03, roadWidthFar * 1.03);

            // ---- center line: thin, dashed, continuous stroke via GC --------------
            {
            const wxGraphicsPen lanePen = gc->CreatePen(wxGraphicsPenInfo{
                Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow), laneWidth }
                                                            .Style(wxPENSTYLE_SHORT_DASH)
                                                            .Cap(wxCAP_ROUND)
                                                            .Join(wxJOIN_ROUND));
            gc->SetPen(lanePen);
            gc->StrokePath(splinePath);
            }

        gc->PopState();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCircularSign(const wxRect rect, const wxBrush& brush,
                                         const wxString& text, wxDC& dc) const
        {
        const auto signOutlineWidth = rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2;
        const wxDCPenChanger pc(dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                          ScaleToScreenAndCanvas(signOutlineWidth)));
        const wxDCBrushChanger bc(dc, brush);

        const auto radius = GetRadius(rect);
        const auto circleCenter = GetMidPoint(rect);

        dc.DrawCircle(circleCenter, radius);

        // lettering on the sign
        Label theLabel(GraphItemInfo{ text }
                           .Pen(wxNullPen)
                           .AnchorPoint(circleCenter)
                           .Anchoring(Anchoring::Center)
                           .LabelAlignment(TextAlignment::Centered)
                           .DPIScaling(GetDPIScaleFactor()));
        theLabel.SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::White));
        wxPoint theLabelCorner{ circleCenter };
        auto rectWithinCircleWidth = geometry::radius_to_inner_rect_width(radius);
        theLabelCorner.x -= rectWithinCircleWidth / 2;
        theLabelCorner.y -= rectWithinCircleWidth / 2;
        theLabel.SetBoundingBox(
            wxRect(theLabelCorner, wxSize(rectWithinCircleWidth, rectWithinCircleWidth)), dc,
            GetScaling());
        theLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        theLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        theLabel.Draw(dc);
        }
    } // namespace Wisteria::GraphItems
