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

    } // namespace Wisteria::GraphItems
