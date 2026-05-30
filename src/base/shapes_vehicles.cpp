///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_vehicles.cpp
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
    void ShapeRenderer::DrawCar(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect dcRect{ rect };
        dcRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                           ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                           0);
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedHeight{ dcRect.GetHeight() * math_constants::three_quarters };
            const auto adjustTop{ (dcRect.GetHeight() - adjustedHeight) * math_constants::half };
            dcRect.SetHeight(adjustedHeight);
            dcRect.Offset(wxPoint(0, adjustTop));
            }

        const GraphicsContextFallback gcf{ &dc, dcRect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for car icon!");
        if (gc != nullptr)
            {
            const wxPen outlinePen(wxTransparentColour, ScaleToScreenAndCanvas(1));

            const wxColour bodyColor{ TintIfUsingOpacity(wxColour{ L"#171721" }) }; // dark blue

            const auto bodyBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(dcRect, -math_constants::full),
                GetYPosFromTop(dcRect, math_constants::half),
                GetXPosFromLeft(dcRect, math_constants::full),
                GetYPosFromTop(dcRect, math_constants::half),
                Colors::ColorContrast::Shade(bodyColor, 0.4), bodyColor);
            gc->SetPen(outlinePen);
            gc->SetBrush(bodyBrush);
            // body of car
            wxRect2DDouble bodyRect{ dcRect };
            bodyRect.Deflate(ScaleToScreenAndCanvas(1));
            bodyRect.SetHeight(bodyRect.GetHeight() * 0.35);
            bodyRect.Offset(0, dcRect.GetHeight() - (bodyRect.GetHeight() * 1.5));
            // lower half (bumper area)
            wxRect2DDouble lowerBodyRect{ bodyRect };
            lowerBodyRect.MoveTopTo(lowerBodyRect.GetTop() + (lowerBodyRect.GetHeight() / 2));
            lowerBodyRect.SetHeight(lowerBodyRect.GetHeight() / 2);

            // upper half (headlights area)
            // (this is drawn later, after the top area of the car so that it
            //  covers up any seams)
            const double backBumperOffset = bodyRect.GetWidth() * 0.025;
            wxRect2DDouble upperBodyRect{ bodyRect };
            upperBodyRect.SetWidth(upperBodyRect.GetWidth() * 0.95);
            upperBodyRect.Offset(wxPoint2DDouble(backBumperOffset, 0));

            // top of car
            wxRect2DDouble carTopRect{ bodyRect };
            carTopRect.SetWidth(carTopRect.GetWidth() * 0.65);
            carTopRect.MoveTopTo(bodyRect.GetTop() - carTopRect.GetHeight() +
                                 ScaleToScreenAndCanvas(2));
            carTopRect.SetHeight(carTopRect.GetHeight() + ScaleToScreenAndCanvas(2));
            carTopRect.Offset(wxPoint2DDouble(backBumperOffset, 0));
            gc->DrawRoundedRectangle(carTopRect.GetX(), carTopRect.GetY(), carTopRect.GetWidth(),
                                     carTopRect.GetHeight(), ScaleToScreenAndCanvas(2));

            // windshield
            std::array<wxPoint2DDouble, 4> windshieldSection = {
                carTopRect.GetRightTop(),
                wxPoint2DDouble(
                    carTopRect.GetRight() +
                        ((bodyRect.GetWidth() - carTopRect.GetWidth()) * math_constants::third),
                    carTopRect.GetBottom()),
                wxPoint2DDouble(carTopRect.GetRightBottom().m_x -
                                    (carTopRect.GetWidth() * math_constants::quarter),
                                carTopRect.GetRightBottom().m_y),
                wxPoint2DDouble(carTopRect.GetRightTop().m_x -
                                    (carTopRect.GetWidth() * math_constants::quarter),
                                carTopRect.GetRightTop().m_y)
            };
            std::array<wxPoint2DDouble, 2> windshield = {
                wxPoint2DDouble(windshieldSection[0].m_x,
                                windshieldSection[0].m_y + ScaleToScreenAndCanvas(1)),
                windshieldSection[1]
            };
            auto windshieldAreaPath = gc->CreatePath();
            windshieldAreaPath.MoveToPoint(windshieldSection[0]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[1]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[2]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[3]);
            gc->DrawPath(windshieldAreaPath);

            gc->SetPen(wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray),
                                 ScaleToScreenAndCanvas(1))
                           .Cap(wxCAP_BUTT));
            auto windshieldPath = gc->CreatePath();
            windshieldPath.MoveToPoint(windshield[0]);
            windshieldPath.AddLineToPoint(windshield[1]);
            gc->StrokePath(windshieldPath);
            gc->SetPen(outlinePen);

            // side windows
            auto windowBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(dcRect, -math_constants::half),
                GetYPosFromTop(dcRect, math_constants::half),
                GetXPosFromLeft(dcRect, math_constants::three_quarters),
                GetYPosFromTop(dcRect, math_constants::half),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)));
            gc->SetBrush(windowBrush);
            auto sideWindowPath = gc->CreatePath();
            sideWindowPath.MoveToPoint(
                wxPoint2DDouble(windshield[0].m_x - ScaleToScreenAndCanvas(2),
                                windshield[0].m_y + ScaleToScreenAndCanvas(1)));
            sideWindowPath.AddLineToPoint(
                wxPoint2DDouble(windshield[1].m_x - ScaleToScreenAndCanvas(2),
                                windshield[1].m_y + ScaleToScreenAndCanvas(1)));
            sideWindowPath.AddLineToPoint(
                wxPoint2DDouble(upperBodyRect.GetX() + ScaleToScreenAndCanvas(2),
                                windshield[1].m_y + ScaleToScreenAndCanvas(1)));
            sideWindowPath.AddLineToPoint(
                wxPoint2DDouble(upperBodyRect.GetX() + ScaleToScreenAndCanvas(2),
                                windshield[0].m_y + ScaleToScreenAndCanvas(1)));
            gc->FillPath(sideWindowPath);
            gc->SetBrush(bodyBrush);

            // divider between windows
            gc->SetPen(wxPenInfo(Colors::ColorContrast::Shade(bodyColor),
                                 dcRect.GetWidth() <= ScaleToScreenAndCanvas(32) ?
                                     ScaleToScreenAndCanvas(1) :
                                     ScaleToScreenAndCanvas(2))
                           .Cap(wxCAP_BUTT));
            gc->SetBrush(wxColour{ 0, 0, 0, 0 });
            wxRect2DDouble windowRect{ carTopRect };
            windowRect.SetWidth(windowRect.GetWidth() * 0.4);
            windowRect.SetHeight(windowRect.GetHeight() * 0.9);
            windowRect.Offset(wxPoint2DDouble(carTopRect.GetWidth() * math_constants::fifth,
                                              windowRect.GetHeight() * 0.1));
            gc->StrokeLine(windowRect.GetX(), windowRect.GetY(), windowRect.GetX(),
                           windowRect.GetY() + windowRect.GetHeight());
            gc->StrokeLine(windowRect.GetX() + windowRect.GetWidth(), windowRect.GetY(),
                           windowRect.GetX() + windowRect.GetWidth(),
                           windowRect.GetY() + windowRect.GetHeight());
            gc->SetBrush(bodyBrush);
            gc->SetPen(outlinePen);

            // draw upper body part on top of windshield
            gc->DrawRoundedRectangle(upperBodyRect.GetX(), upperBodyRect.GetY(),
                                     upperBodyRect.GetWidth(), upperBodyRect.GetHeight(),
                                     ScaleToScreenAndCanvas(2));

            // headlights
            wxRect2DDouble headlightsRect{ upperBodyRect };
            headlightsRect.SetWidth(headlightsRect.GetWidth() * 0.05);
            headlightsRect.SetHeight(headlightsRect.GetHeight() * math_constants::quarter);
            headlightsRect.Offset(upperBodyRect.GetWidth() - headlightsRect.GetWidth(),
                                  upperBodyRect.GetHeight() * math_constants::quarter);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                headlightsRect.GetLeft(), headlightsRect.GetTop() * math_constants::half,
                headlightsRect.GetRight(), headlightsRect.GetTop() * math_constants::half,
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::OrangeYellow)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::AntiqueWhite))));
            gc->DrawRectangle(headlightsRect.GetX(), headlightsRect.GetY(),
                              headlightsRect.GetWidth(), headlightsRect.GetHeight());
            gc->SetBrush(bodyBrush);

            // draw bumper area now, to overlay any headlight overlap
            gc->DrawRoundedRectangle(lowerBodyRect.GetX(), lowerBodyRect.GetY(),
                                     lowerBodyRect.GetWidth(), lowerBodyRect.GetHeight(),
                                     ScaleToScreenAndCanvas(2));

            // the tires
            wxRect tireRect{ dcRect };
            tireRect.SetWidth(dcRect.GetWidth() * math_constants::quarter);
            tireRect.SetHeight(tireRect.GetWidth());
            tireRect.SetTop(dcRect.GetTop() + (dcRect.GetHeight() - tireRect.GetHeight()));
            tireRect.SetLeft(dcRect.GetLeft() + (dcRect.GetWidth() * math_constants::tenth));

            DrawTire(tireRect, gc);

            tireRect.SetLeft((dcRect.GetRight() - tireRect.GetWidth()) -
                             (dcRect.GetWidth() * math_constants::tenth));
            DrawTire(tireRect, gc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTire(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for tire icon!");
        DrawTire(rect, gc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTire(wxRect rect, wxGraphicsContext* gc) const
        {
        if (gc != nullptr)
            {
            const wxPen scaledPen(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray),
                                  rect.GetWidth() <= ScaleToScreenAndCanvas(32) ?
                                      ScaleToScreenAndCanvas(1) :
                                      ScaleToScreenAndCanvas(2));
            gc->SetPen(scaledPen);
            // the tire
            const wxRect tireRect = wxRect(rect).Deflate(ScaleToScreenAndCanvas(1));
            const auto tireBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(tireRect, 0), GetYPosFromTop(tireRect, 0),
                GetXPosFromLeft(tireRect, 1.5), GetYPosFromTop(tireRect, 1.5),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)));
            gc->SetBrush(tireBrush);

            gc->DrawEllipse(tireRect.GetTopLeft().x, tireRect.GetTopLeft().y, tireRect.GetWidth(),
                            tireRect.GetHeight());

            // hubcap
            wxRect hubCapRect = wxRect(rect).Deflate(rect.GetWidth() * math_constants::quarter);
            const auto hubCapBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(hubCapRect, 0), GetYPosFromTop(hubCapRect, 0),
                GetXPosFromLeft(hubCapRect, 1.5), GetYPosFromTop(hubCapRect, 1.5),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Silver)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::CoolGrey)));
            gc->SetBrush(hubCapBrush);

            gc->DrawEllipse(hubCapRect.GetTopLeft().x, hubCapRect.GetTopLeft().y,
                            hubCapRect.GetWidth(), hubCapRect.GetHeight());

            hubCapRect.Deflate(hubCapRect.GetWidth() * math_constants::eighth);
            const wxPen blackPen(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                scaledPen.GetWidth());
            gc->SetPen(blackPen);

            DrawAsterisk(hubCapRect, gc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTractorTire(wxRect rect, wxGraphicsContext* gc, const bool isRear) const
        {
        if (gc == nullptr)
            {
            return;
            }

        const auto penWidth = std::max<int>(1, ScaleToScreenAndCanvas(0.5));
        const wxColour rubberBlack{ wxColour{ 25, 25, 25 } };
        const wxColour rubberMid{ wxColour{ 65, 65, 65 } };
        const wxColour hubFace{ wxColour{ 195, 195, 200 } };
        const wxColour hubEdge{ wxColour{ 135, 135, 142 } };
        const wxColour hubDark{ wxColour{ 80, 80, 88 } };
        const wxColour boltColor{ wxColour{ 50, 50, 55 } };

        const double cx = rect.GetX() + rect.GetWidth() * 0.5;
        const double cy = rect.GetY() + rect.GetHeight() * 0.5;
        const double rx = rect.GetWidth() * 0.5;
        const double ry = rect.GetHeight() * 0.5;

        // outer rubber fill
        const auto tireBrush = gc->CreateRadialGradientBrush(
            cx - rx * 0.25, cy - ry * 0.25, cx, cy, std::max(rx, ry), rubberMid, rubberBlack);
        gc->SetPen(wxPen{ rubberBlack, penWidth });
        gc->SetBrush(tireBrush);
        gc->DrawEllipse(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

        // tread groove lines around the perimeter
        const int numGrooves = isRear ? 36 : 24;
        constexpr double GROOVE_OUTER = 0.98;
        constexpr double GROOVE_INNER = 0.82;
        gc->SetPen(wxPen{ rubberBlack, std::max<int>(1, ScaleToScreenAndCanvas(1)) });
        for (int i = 0; i < numGrooves; ++i)
            {
            const auto a = safe_divide<double>(2.0 * std::numbers::pi * i, numGrooves);
            gc->StrokeLine(
                cx + rx * GROOVE_INNER * std::cos(a), cy + ry * GROOVE_INNER * std::sin(a),
                cx + rx * GROOVE_OUTER * std::cos(a), cy + ry * GROOVE_OUTER * std::sin(a));
            }

        // sidewall ring
        const double swR = isRear ? 0.72 : 0.66;
        const auto swBrush = gc->CreateRadialGradientBrush(cx - rx * 0.12, cy - ry * 0.12, cx, cy,
                                                           rx * swR, rubberMid, rubberBlack);
        gc->SetPen(wxPen{ rubberBlack, penWidth });
        gc->SetBrush(swBrush);
        gc->DrawEllipse(cx - rx * swR, cy - ry * swR, rx * swR * 2, ry * swR * 2);

        // hub disc
        const double hubR = isRear ? 0.42 : 0.46;
        const auto hubBrush = gc->CreateRadialGradientBrush(
            cx - rx * hubR * 0.25, cy - ry * hubR * 0.25, cx, cy, rx * hubR, hubFace, hubEdge);
        gc->SetPen(wxPen{ hubDark, penWidth });
        gc->SetBrush(hubBrush);
        gc->DrawEllipse(cx - rx * hubR, cy - ry * hubR, rx * hubR * 2, ry * hubR * 2);

        // inner hub ring
        gc->SetPen(wxPen{ hubDark, penWidth });
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        const double ringR = hubR * 0.70;
        gc->DrawEllipse(cx - rx * ringR, cy - ry * ringR, rx * ringR * 2, ry * ringR * 2);

        // hub bolts
        const double boltRingR = hubR * 0.50;
        const double boltR = rx * hubR * 0.10;
        const int numBolts = isRear ? 8 : 6;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ boltColor });
        for (int i = 0; i < numBolts; ++i)
            {
            const auto a = safe_divide<double>(2.0 * std::numbers::pi * i, numBolts);
            const double bx = cx + rx * boltRingR * std::cos(a);
            const double by = cy + ry * boltRingR * std::sin(a);
            gc->DrawEllipse(bx - boltR, by - boltR, boltR * 2, boltR * 2);
            }

        // center axle
        const double axleR = rx * hubR * 0.18;
        gc->SetPen(wxPen{ hubDark, penWidth });
        gc->SetBrush(wxBrush{ boltColor });
        gc->DrawEllipse(cx - axleR, cy - axleR, axleR * 2, axleR * 2);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTractor(const wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect dcRect{ rect };
        dcRect.Deflate(ScaleToScreenAndCanvas(3));

        const GraphicsContextFallback gcf{ &dc, dcRect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for tractor icon!");
        if (gc == nullptr)
            {
            return;
            }

        // color palette
        const wxColour tractorRedDark{ Colors::ColorBrewer::GetColor(Colors::Color::TractorRed) };
        const wxColour tractorRedLight{ 253, 110, 120 };
        const wxColour darkOutline{ wxColour{ 40, 22, 22 } };
        const wxColour darkMetal{ wxColour{ 48, 48, 52 } };
        const wxColour lightMetal{ wxColour{ 180, 183, 190 } };
        const wxColour glassBlue{ wxColour{ 145, 200, 220 } };
        const wxColour glassDark{ wxColour{ 70, 115, 145 } };
        const wxColour grilleSlot{ wxColour{ 28, 16, 16 } };
        const wxColour headlightYellow{ Colors::ColorBrewer::GetColor(
            Colors::Color::AntiqueWhite) };
        const wxColour headlightGlow{ Colors::ColorBrewer::GetColor(Colors::Color::OrangeYellow) };

        const wxPen hairPen(darkOutline, std::min(1.0, ScaleToScreenAndCanvas(0.5)));

        const double width = dcRect.GetWidth();
        const double height = dcRect.GetHeight();
        const double rectLeft = dcRect.GetLeft();
        const double rectTop = dcRect.GetTop();
        const double rectBottom = rectTop + height;

        // tire geometry
        const double rearR = height * 0.28;
        const double rearCx = rectLeft + width * 0.25;
        const double rearCy = rectBottom - rearR;

        const double frontR = height * 0.16;
        const double frontCx = rectLeft + width * 0.79;
        const double frontCy = rectBottom - frontR;

        // body reference points
        const double fenderPeakY = rearCy - rearR - height * 0.04;
        const double ucY = rearCy + rearR * 0.15;
        const double cabRearX = rectLeft + width * 0.08;
        const double cabTopY = rectTop + height * 0.10;
        const double roofRightX = rectLeft + width * 0.48;
        const double pillarBotX = rectLeft + width * 0.52;
        const double pillarBotY = rectTop + height * 0.42;
        const double noseX = rectLeft + width * 0.93;
        const double noseTopY = pillarBotY + height * 0.05;
        const double noseBotY = frontCy - frontR * 0.20;

        // body shell
        wxGraphicsPath bodyPath = gc->CreatePath();

        const double ucLeft = rearCx + rearR * 0.55;
        bodyPath.MoveToPoint(ucLeft, ucY);

        // rear fender
        bodyPath.AddCurveToPoint(rearCx + rearR * 0.20, ucY + height * 0.01, rearCx - rearR * 0.20,
                                 rearCy + rearR * 0.45, rearCx - rearR * 0.65,
                                 rearCy + rearR * 0.10);
        bodyPath.AddCurveToPoint(rearCx - rearR * 0.90, rearCy - rearR * 0.40,
                                 rearCx - rearR * 0.60, fenderPeakY + height * 0.01,
                                 rearCx - rearR * 0.15, fenderPeakY);

        // fender top to cab rear
        bodyPath.AddCurveToPoint(rearCx - rearR * 0.35, fenderPeakY - height * 0.01,
                                 cabRearX + width * 0.02, fenderPeakY - height * 0.01, cabRearX,
                                 fenderPeakY);

        // cab rear wall up
        bodyPath.AddLineToPoint(cabRearX, cabTopY);

        // roof
        bodyPath.AddCurveToPoint(cabRearX, cabTopY - height * 0.015, roofRightX - width * 0.06,
                                 cabTopY - height * 0.015, roofRightX, cabTopY + height * 0.005);

        // front pillar
        bodyPath.AddCurveToPoint(roofRightX + width * 0.01, cabTopY + height * 0.02,
                                 pillarBotX - width * 0.015, pillarBotY - height * 0.08, pillarBotX,
                                 pillarBotY);

        // hood
        bodyPath.AddCurveToPoint(pillarBotX + width * 0.06, pillarBotY - height * 0.005,
                                 noseX - width * 0.10, noseTopY - height * 0.01, noseX, noseTopY);

        // nose curves down
        bodyPath.AddCurveToPoint(noseX + width * 0.015, noseTopY + height * 0.03,
                                 noseX + width * 0.015, noseBotY - height * 0.03,
                                 noseX - width * 0.005, noseBotY);

        // front wheel well
        bodyPath.AddCurveToPoint(noseX - width * 0.02, noseBotY + height * 0.03,
                                 frontCx + frontR * 0.50, frontCy - frontR * 0.35,
                                 frontCx + frontR * 0.25, ucY);

        // undercarriage
        bodyPath.AddLineToPoint(ucLeft, ucY);
        bodyPath.CloseSubpath();

        // body gradient: light at top fading to dark at bottom
        gc->SetPen(hairPen);
        gc->SetBrush(gc->CreateLinearGradientBrush(rectLeft, cabTopY, rectLeft, ucY,
                                                   tractorRedLight, tractorRedDark));
        gc->FillPath(bodyPath);
        gc->StrokePath(bodyPath);

        // body sheens
        gc->SetPen(*wxTRANSPARENT_PEN);

        // hood sheen
        wxGraphicsPath hoodSheenPath = gc->CreatePath();
        const double hsTop = noseTopY + height * 0.008;
        const double hsMid = noseTopY + height * 0.04;
        hoodSheenPath.MoveToPoint(pillarBotX + width * 0.03, hsTop);
        hoodSheenPath.AddCurveToPoint(pillarBotX + width * 0.10, hsTop - height * 0.003,
                                      noseX - width * 0.10, hsTop, noseX - width * 0.03,
                                      hsTop + height * 0.005);
        hoodSheenPath.AddCurveToPoint(noseX - width * 0.10, hsMid, pillarBotX + width * 0.10, hsMid,
                                      pillarBotX + width * 0.03, hsMid);
        hoodSheenPath.CloseSubpath();
        gc->SetBrush(gc->CreateLinearGradientBrush(
            rectLeft, hsTop, rectLeft, hsMid,
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 150),
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 50)));
        gc->FillPath(hoodSheenPath);

        // windshield geometry
        const double wsTopL = roofRightX - width * 0.0175;
        const double wsTopR = roofRightX;
        const double wsTopY = cabTopY + height * 0.08;
        const double wsBotL = wsTopL;
        const double wsBotR = wsBotL + (pillarBotX - width * 0.0125 - wsBotL) * 0.75;
        const double wsBotY = pillarBotY - height * 0.05;

        // cab front sheen
        wxGraphicsPath cabFrontSheenPath = gc->CreatePath();
        const double wrM = (wsTopL + wsTopR) * 0.48;
        cabFrontSheenPath.MoveToPoint(wsTopL + width * 0.002, wsTopY + height * 0.004);
        cabFrontSheenPath.AddLineToPoint(wrM, wsTopY + height * 0.004);
        cabFrontSheenPath.AddLineToPoint(wrM - width * 0.012, wsBotY - height * 0.008);
        cabFrontSheenPath.AddLineToPoint(wsBotL + width * 0.002, wsBotY - height * 0.008);
        cabFrontSheenPath.CloseSubpath();
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(gc->CreateLinearGradientBrush(
            wsTopL, wsTopY, wrM, wsBotY,
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 150),
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 50)));
        gc->FillPath(cabFrontSheenPath);

        // cab left-edge sheen
        wxGraphicsPath cabSheenPath = gc->CreatePath();
        const double csL = cabRearX + width * 0.008;
        const double csR = cabRearX + width * 0.04;
        cabSheenPath.MoveToPoint(csL, cabTopY + height * 0.05);
        cabSheenPath.AddLineToPoint(csR, cabTopY + height * 0.05);
        cabSheenPath.AddLineToPoint(csR, pillarBotY - height * 0.04);
        cabSheenPath.AddLineToPoint(csL, pillarBotY - height * 0.04);
        cabSheenPath.CloseSubpath();
        gc->SetBrush(gc->CreateLinearGradientBrush(
            csL, cabTopY, csR, cabTopY,
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 150),
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 50)));
        gc->FillPath(cabSheenPath);

        // fender sheen
        wxGraphicsPath fenderSheenPath = gc->CreatePath();
        const double fsOff = height * 0.010;
        fenderSheenPath.MoveToPoint(rearCx - rearR * 0.55, rearCy + rearR * 0.15);
        fenderSheenPath.AddCurveToPoint(rearCx - rearR * 0.80, rearCy - rearR * 0.30,
                                        rearCx - rearR * 0.48, fenderPeakY + height * 0.015,
                                        rearCx - rearR * 0.10, fenderPeakY + height * 0.005);
        fenderSheenPath.AddCurveToPoint(rearCx - rearR * 0.48, fenderPeakY + height * 0.015 + fsOff,
                                        rearCx - rearR * 0.80, rearCy - rearR * 0.30 + fsOff,
                                        rearCx - rearR * 0.55, rearCy + rearR * 0.15 + fsOff);
        fenderSheenPath.CloseSubpath();
        gc->SetBrush(gc->CreateLinearGradientBrush(
            rearCx - rearR * 0.55, rearCy, rearCx - rearR * 0.10, fenderPeakY,
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 150),
            Colors::ColorContrast::ChangeOpacity(wxColour{ 255, 255, 255 }, 50)));
        gc->FillPath(fenderSheenPath);

        // windshield
        wxGraphicsPath wsPath = gc->CreatePath();
        wsPath.MoveToPoint(wsBotL, wsBotY);
        wsPath.AddLineToPoint(wsTopL, wsTopY);
        wsPath.AddCurveToPoint(wsTopL + width * 0.003, wsTopY - height * 0.002,
                               wsTopR - width * 0.003, wsTopY - height * 0.002, wsTopR,
                               wsTopY + height * 0.003);
        wsPath.AddLineToPoint(wsBotR, wsBotY);
        wsPath.CloseSubpath();

        gc->SetPen(hairPen);
        gc->SetBrush(
            gc->CreateLinearGradientBrush(wsTopL, wsTopY, wsBotR, wsBotY, glassDark, glassBlue));
        gc->FillPath(wsPath);
        gc->StrokePath(wsPath);

        // side window
        const double swL = cabRearX + width * 0.04;
        const double swR = wsTopL - width * 0.03;
        const double swT = cabTopY + height * 0.10;
        const double swB = pillarBotY - height * 0.10;

        wxGraphicsPath swPath = gc->CreatePath();
        swPath.MoveToPoint(swL, swB);
        swPath.AddLineToPoint(swL, swT);
        swPath.AddCurveToPoint(swL + width * 0.003, swT - height * 0.001, swR - width * 0.003,
                               swT - height * 0.001, swR, swT + height * 0.002);
        swPath.AddLineToPoint(swR, swB);
        swPath.CloseSubpath();
        gc->SetPen(hairPen);
        gc->SetBrush(gc->CreateLinearGradientBrush(swL, swT, swR, swB, glassDark, glassBlue));
        gc->FillPath(swPath);
        gc->StrokePath(swPath);

        // grille slots
        const double grL = pillarBotX + width * 0.025;
        const double grR = frontCx - frontR * 0.80;
        const double grT = pillarBotY + height * 0.08;
        const double grB = pillarBotY + height * 0.24;
        constexpr int GRILLE_SLOTS{ 4 };
        const double slotW = std::max(1.0, width * 0.012);
        const double grSpan = grR - grL;
        const double slotSp = safe_divide(grSpan, static_cast<double>(GRILLE_SLOTS + 1));

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ grilleSlot });
        for (int i = 1; i <= GRILLE_SLOTS; ++i)
            {
            const double sx = grL + slotSp * i - slotW * 0.5;
            const double sr = slotW * 0.3;
            wxGraphicsPath sp = gc->CreatePath();
            sp.MoveToPoint(sx + sr, grT);
            sp.AddLineToPoint(sx + slotW - sr, grT);
            sp.AddCurveToPoint(sx + slotW, grT, sx + slotW, grT + sr, sx + slotW, grT + sr);
            sp.AddLineToPoint(sx + slotW, grB - sr);
            sp.AddCurveToPoint(sx + slotW, grB, sx + slotW - sr, grB, sx + slotW - sr, grB);
            sp.AddLineToPoint(sx + sr, grB);
            sp.AddCurveToPoint(sx, grB, sx, grB - sr, sx, grB - sr);
            sp.AddLineToPoint(sx, grT + sr);
            sp.AddCurveToPoint(sx, grT, sx + sr, grT, sx + sr, grT);
            sp.CloseSubpath();
            gc->FillPath(sp);
            }

        // exhaust stack
        const double exW = width * 0.022;
        const double exX = pillarBotX + width * 0.035;
        const double exBot = noseTopY - height * 0.005;
        const double exTop = rectTop + height * 0.02;

        wxGraphicsPath exPath = gc->CreatePath();
        exPath.MoveToPoint(exX, exBot);
        exPath.AddLineToPoint(exX, exTop + height * 0.03);
        exPath.AddCurveToPoint(exX, exTop, exX + exW * 0.3, exTop, exX + exW,
                               exTop + height * 0.008);
        exPath.AddLineToPoint(exX + exW, exBot);
        exPath.CloseSubpath();
        // gradient goes left-to-right: light on front face, dark on back
        gc->SetPen(hairPen);
        gc->SetBrush(
            gc->CreateLinearGradientBrush(exX, exTop, exX + exW, exTop, lightMetal, darkMetal));
        gc->FillPath(exPath);
        gc->StrokePath(exPath);

        // exhaust cap
        gc->SetBrush(wxBrush{ darkMetal });
        gc->DrawEllipse(exX - exW * 0.12, exTop - height * 0.004, exW * 1.24, height * 0.010);

        // headlight
        const double hlR = height * 0.028;
        const double hlCx = noseX - width * 0.008;
        const double hlCy = noseTopY + height * 0.08;
        gc->SetPen(hairPen);
        gc->SetBrush(gc->CreateRadialGradientBrush(hlCx - hlR * 0.25, hlCy - hlR * 0.25, hlCx, hlCy,
                                                   hlR, headlightYellow, headlightGlow));
        gc->DrawEllipse(hlCx - hlR, hlCy - hlR, hlR * 2, hlR * 2);

        // rear tire
        const wxRect rearTireRect{ static_cast<int>(rearCx - rearR),
                                   static_cast<int>(rearCy - rearR), static_cast<int>(rearR * 2),
                                   static_cast<int>(rearR * 2) };
        DrawTractorTire(rearTireRect, gc, true);

        // front tire
        const wxRect frontTireRect{ static_cast<int>(frontCx - frontR),
                                    static_cast<int>(frontCy - frontR),
                                    static_cast<int>(frontR * 2), static_cast<int>(frontR * 2) };
        DrawTractorTire(frontTireRect, gc, false);
        }
    } // namespace Wisteria::GraphItems
