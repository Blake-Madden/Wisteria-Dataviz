///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_business.cpp
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
    void ShapeRenderer::DrawHundredDollarBill(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCFontChanger fc{ dc };

        wxRect2DDouble drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(1));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for dollar icon!");
        if (gc != nullptr)
            {
            gc->SetPen(
                wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                       static_cast<int>(ScaleToScreenAndCanvas(math_constants::quarter)) });

            // background of bill
            wxRect2DDouble billRect{ drawRect };
            billRect.SetHeight(billRect.GetHeight() * math_constants::half);
            billRect.Offset(0, drawRect.GetHeight() * math_constants::quarter);

            // fill entire bill with muted blue-gray base
            const wxColour blueGray = ApplyColorOpacity(wxColour{ L"#BCC5D3" });
            gc->SetBrush(blueGray);
            gc->SetPen(wxPen{ wxColour(0, 0, 0, 0) });
            gc->DrawRectangle(billRect);

            // metallic gold "flare" band
            const double glowStart =
                billRect.GetX() + (billRect.GetWidth() * math_constants::tenth);
            const double glowCenter =
                billRect.GetX() + (billRect.GetWidth() * math_constants::half);
            const double glowEnd = billRect.GetX() + (billRect.GetWidth() * math_constants::half);
            const wxColour goldTone = ApplyColorOpacity(wxColour{ L"#E5C07B" });

                // left half of flare
                // transparent -> gold
                {
                const wxGraphicsBrush leftFlare = gc->CreateLinearGradientBrush(
                    glowStart, billRect.GetY(), glowCenter, billRect.GetY(),
                    wxColour{ blueGray.Red(), blueGray.Green(), blueGray.Blue(), 0 }, goldTone);

                gc->SetBrush(leftFlare);
                gc->DrawRectangle(glowStart, billRect.GetY(), glowCenter - glowStart,
                                  billRect.GetHeight());
                }

                // right half of flare:
                // gold -> transparent
                {
                const wxGraphicsBrush rightFlare = gc->CreateLinearGradientBrush(
                    glowCenter, billRect.GetY(),
                    glowEnd + (billRect.GetWidth() * math_constants::fifth), billRect.GetY(),
                    goldTone, wxColour(blueGray.Red(), blueGray.Green(), blueGray.Blue(), 0));

                gc->SetBrush(rightFlare);
                gc->DrawRectangle(glowCenter, billRect.GetY(), (billRect.GetWidth() * 0.30),
                                  billRect.GetHeight());
                }

            // portrait
            //---------
            wxRect2DDouble innerBillRect{ billRect };
            innerBillRect.Deflate(ScaleToScreenAndCanvas(1));

            wxRect2DDouble portraitRect{ innerBillRect };
            portraitRect.SetWidth(portraitRect.GetWidth() * math_constants::third);
            portraitRect.Offset(billRect.GetWidth() * math_constants::fifth, 0);

            gc->PushState();
            gc->Clip(portraitRect.GetX(), portraitRect.GetY(), portraitRect.GetWidth(),
                     portraitRect.GetHeight());

            const wxRect2DDouble faceRect{ portraitRect.GetX() +
                                               (portraitRect.GetWidth() * math_constants::third),
                                           portraitRect.GetY() + (portraitRect.GetHeight() * 0.275),
                                           portraitRect.GetWidth() * math_constants::half,
                                           portraitRect.GetHeight() * math_constants::half };

            const auto fillHairShaded = [&](const wxGraphicsPath& hp, const wxRect2DDouble& hr)
            {
                const wxColour baseHair = Colors::ColorBrewer::GetColor(Colors::Color::SlateGray);
                gc->SetBrush(baseHair);
                gc->SetPen(wxPen{ wxColour{ 0, 0, 0, 0 },
                                  std::max<int>(1, ScaleToScreenAndCanvas(math_constants::full)) });
                gc->FillPath(hp);

                const wxColour highlight =
                    Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack, 150);

                // angled gradient: top-right to bottom-left
                const wxGraphicsBrush grad = gc->CreateLinearGradientBrush(
                    hr.GetX() + hr.GetWidth(), hr.GetY(), hr.GetX(), hr.GetY() + hr.GetHeight(),
                    highlight, wxColour{ 0, 0, 0, 0 });

                gc->SetBrush(grad);
                gc->FillPath(hp);
            };

                // hair ribbon on right side
                {
                wxRect2DDouble ribbon{ faceRect };

                ribbon.SetWidth(ribbon.GetWidth() * 0.80);
                ribbon.SetHeight(ribbon.GetHeight() * 1.40);

                ribbon.Offset(faceRect.GetWidth() * 0.4,
                              -ribbon.GetHeight() * math_constants::tenth);

                wxGraphicsPath hairPath = gc->CreatePath();

                const double topX = ribbon.GetX() + (ribbon.GetWidth() * math_constants::fifth);
                const double topY = ribbon.GetY() + (ribbon.GetHeight() * 0.05);

                const double yBase = topY;

                const auto sy = [&](double y)
                { return yBase + ((y - yBase) * math_constants::three_quarters); };

                const double mid1X = ribbon.GetX() + (ribbon.GetWidth() * 1.15);
                const double mid1Y = sy(ribbon.GetY() + (ribbon.GetHeight() * 0.35));

                const double mid2X = ribbon.GetX() + (ribbon.GetWidth() * 0.90);
                const double mid2Y = sy(ribbon.GetY() + (ribbon.GetHeight() * 0.60));

                const double brX = ribbon.GetX() + (ribbon.GetWidth() * 1.05);
                const double brY = sy(ribbon.GetY() + ribbon.GetHeight());

                const double blX = ribbon.GetX() + (ribbon.GetWidth() * math_constants::quarter);
                const double blY = sy(ribbon.GetY() + ribbon.GetHeight());

                const double bcX = ribbon.GetX() + (ribbon.GetWidth() * 0.65);
                const double bcY = sy(ribbon.GetY() + (ribbon.GetHeight() * 1.20));

                hairPath.MoveToPoint(topX, topY);

                hairPath.AddQuadCurveToPoint(mid1X, mid1Y, mid2X, mid2Y);

                hairPath.AddQuadCurveToPoint(ribbon.GetX() + (ribbon.GetWidth() * 1.15),
                                             sy(ribbon.GetY() + (ribbon.GetHeight() * 0.80)), brX,
                                             brY);

                hairPath.AddQuadCurveToPoint(bcX, bcY, blX, blY);

                hairPath.AddQuadCurveToPoint(
                    ribbon.GetX() - (ribbon.GetWidth() * math_constants::tenth),
                    sy(ribbon.GetY() + (ribbon.GetHeight() * 0.40)), topX, topY);

                hairPath.CloseSubpath();

                fillHairShaded(hairPath, ribbon);
                }

                // body
                {
                const wxColour bodyPen = Colors::ColorBrewer::GetColor(Colors::Color::Black);
                const wxColour bodyBrush =
                    Colors::ColorBrewer::GetColor(Colors::Color::ClassicFrenchGray);

                gc->SetPen(wxPenInfo{
                    bodyPen, std::max<int>(1, ScaleToScreenAndCanvas(math_constants::half)) });
                gc->SetBrush(bodyBrush);

                // meeting point on left jaw
                const double meetX =
                    faceRect.GetX() + (faceRect.GetWidth() * math_constants::fifth);
                const double meetY = faceRect.GetY() + (faceRect.GetHeight() * 0.90);

                // bottom of body
                const double bottomY = portraitRect.GetBottom();

                // left bottom anchor
                const double leftBottomX = portraitRect.GetX() + (portraitRect.GetWidth() * 0.05);

                // shoulder bump control point
                const double ctrlX =
                    leftBottomX + (portraitRect.GetWidth() * math_constants::tenth);
                const double ctrlY = meetY + ((bottomY - meetY) * math_constants::tenth);

                // under-chin
                const double rightMeetX = faceRect.GetX() + (faceRect.GetWidth() * 0.80);

                wxGraphicsPath body = gc->CreatePath();

                // left side (shoulder)
                body.MoveToPoint(meetX, meetY);
                body.AddQuadCurveToPoint(ctrlX, ctrlY, leftBottomX, bottomY);

                // right side (upwards)
                body.AddLineToPoint(portraitRect.GetRight() +
                                        (portraitRect.GetWidth() * math_constants::tenth),
                                    bottomY);
                body.AddQuadCurveToPoint(rightMeetX - (portraitRect.GetWidth() * 0.05),
                                         meetY + ((bottomY - meetY) * math_constants::fifth),
                                         rightMeetX, meetY);

                // close across the chin back to left meet
                body.CloseSubpath();

                gc->FillPath(body);
                }

                // white overcoat stripe
                {
                const wxColour stripeColor{ Colors::ColorBrewer::GetColor(Colors::Color::Cream) };
                gc->SetPen(wxPenInfo{
                    stripeColor, std::max<int>(1, ScaleToScreenAndCanvas(math_constants::half)) });
                gc->SetBrush(stripeColor);

                // collar area
                const double startX = faceRect.GetX() + (faceRect.GetWidth() * 0.4);
                const double startY = faceRect.GetY() + (faceRect.GetHeight() * 0.9);

                // bottom of shirt
                const double endX = portraitRect.GetX() +
                                    (portraitRect.GetWidth() * math_constants::three_quarters);
                const double endY = portraitRect.GetBottom();

                const double stripeW = portraitRect.GetWidth() * math_constants::tenth;

                wxGraphicsPath stripe = gc->CreatePath();

                stripe.MoveToPoint(startX - (stripeW * math_constants::half), startY);
                stripe.AddLineToPoint(startX + (stripeW * math_constants::half), startY);
                stripe.AddQuadCurveToPoint(((startX + endX) * math_constants::half) +
                                               (stripeW * 0.4),
                                           startY + ((endY - startY) * math_constants::fifth),
                                           endX + (stripeW * math_constants::half), endY);
                stripe.AddLineToPoint(endX - (stripeW * math_constants::half), endY);
                stripe.CloseSubpath();

                gc->FillPath(stripe);
                }

            // face
            gc->SetPen(
                wxPenInfo{ Colors::ColorContrast::ShadeOrTint(wxColour{ L"#ADADAD" }) }.Width(
                    std::max<int>(1, ScaleToScreenAndCanvas(math_constants::tenth))));
            gc->SetBrush(Colors::ColorContrast::Tint(wxColour{ L"#BCC5D3" }));
            gc->DrawEllipse(faceRect);

                // face features
                {
                const wxColour faintBlack = Colors::ColorContrast::ChangeOpacity(
                    Colors::ColorBrewer::GetColor(Colors::Color::Black), 60);

                const int browPenWidth =
                    std::max<int>(1, ScaleToScreenAndCanvas(math_constants::quarter));

                const double faceCenterX =
                    faceRect.GetX() + (faceRect.GetWidth() * math_constants::half);
                const double midY = faceRect.GetY() + (faceRect.GetHeight() * math_constants::half);

                const double eyeSpacing = faceRect.GetWidth() * math_constants::fifth;
                const double eyeW = faceRect.GetWidth() * 0.18;
                const double eyeH = faceRect.GetHeight() * 0.07;
                const double pupilR = (std::min)(eyeW, eyeH) * 0.30;

                const double leftEyeX = faceCenterX - eyeSpacing;
                const double rightEyeX = faceCenterX + eyeSpacing;
                const double eyeTopY = midY - eyeH;

                // eyes
                gc->SetPen(wxPenInfo{
                    faintBlack, std::max<int>(1, ScaleToScreenAndCanvas(math_constants::fifth)) });
                gc->SetBrush(*wxTRANSPARENT_BRUSH);

                auto drawEye = [&](double ex)
                {
                    // looking slightly right
                    const double pupilOffsetX = eyeW * 0.15;
                    gc->DrawEllipse(ex - (eyeW * math_constants::half), eyeTopY, eyeW, eyeH);

                    gc->SetBrush(faintBlack);
                    gc->SetPen(*wxTRANSPARENT_PEN);

                    gc->DrawEllipse(ex - pupilR + pupilOffsetX,
                                    eyeTopY + (eyeH * math_constants::half) - pupilR, pupilR * 2.0,
                                    pupilR * 2.0);

                    gc->SetPen(wxPenInfo{
                        faintBlack,
                        std::max<int>(1, ScaleToScreenAndCanvas(math_constants::fifth)) });
                    gc->SetBrush(*wxTRANSPARENT_BRUSH);
                };

                drawEye(leftEyeX);
                drawEye(rightEyeX);

                // eyebrows
                gc->SetPen(wxPenInfo{ faintBlack, browPenWidth });

                const double browY = eyeTopY - (faceRect.GetHeight() * 0.06);
                const double browW = eyeW * 0.85;

                auto drawBrow = [&](const double ex)
                {
                    gc->StrokeLine(ex - (browW * math_constants::half), browY,
                                   ex + (browW * math_constants::half), browY);
                };

                drawBrow(leftEyeX);
                drawBrow(rightEyeX);

                    // nose (two-line profile, pointing right)
                    {
                    gc->SetPen(wxPenInfo{
                        faintBlack,
                        std::max<int>(1, ScaleToScreenAndCanvas(math_constants::quarter)) });
                    const double cx =
                        faceRect.GetX() + (faceRect.GetWidth() * math_constants::half);

                    const double noseBottomY = faceRect.GetY() +
                                               (faceRect.GetHeight() * math_constants::half) +
                                               (faceRect.GetHeight() * 0.12);

                    const double noseTopY = noseBottomY - (faceRect.GetHeight() * 0.11);

                    const double xOut = cx + (faceRect.GetWidth() * 0.015);

                    gc->StrokeLine(cx, noseTopY, xOut, noseBottomY); // downward diagonal
                    }

                    // mouth
                    {
                    gc->SetPen(wxPenInfo{
                        faintBlack,
                        std::max<int>(1, ScaleToScreenAndCanvas(math_constants::quarter)) });

                    const double cx =
                        faceRect.GetX() + (faceRect.GetWidth() * math_constants::half);
                    const double mouthY = faceRect.GetY() + (faceRect.GetHeight() * 0.72);

                    const double mouthW = faceRect.GetWidth() * 0.22;
                    const double mouthH = faceRect.GetHeight() * 0.07;

                    wxGraphicsPath mouth = gc->CreatePath();

                    mouth.MoveToPoint(cx - (mouthW * math_constants::half),
                                      mouthY - (mouthH * math_constants::quarter));
                    mouth.AddQuadCurveToPoint(cx + (mouthW * math_constants::fifth),
                                              mouthY + mouthH, cx + (mouthW * math_constants::half),
                                              mouthY);

                    gc->StrokePath(mouth);
                    }
                }

                // hair on top of head
                {
                const double xLeft =
                    faceRect.GetX() + (faceRect.GetWidth() * math_constants::tenth);
                const double xRight =
                    faceRect.GetRight() - (faceRect.GetWidth() * math_constants::tenth);
                const double yBase = faceRect.GetY() + (faceRect.GetHeight() * 0.15);
                const double xCtrl = (xLeft + xRight) * math_constants::half;

                const double yCtrl = yBase - (faceRect.GetHeight() * 0.42);

                wxGraphicsPath arc = gc->CreatePath();
                arc.MoveToPoint(xLeft, yBase);
                arc.AddQuadCurveToPoint(xCtrl, yCtrl, xRight, yBase);
                arc.CloseSubpath();

                fillHairShaded(arc, faceRect);
                }

                // left ribbon of hair
                {
                wxRect2DDouble ribbon{ faceRect };

                ribbon.SetWidth(ribbon.GetWidth() * 0.55);
                ribbon.SetHeight(ribbon.GetHeight() * 1.20);

                ribbon.Offset(-ribbon.GetWidth() * 0.35, -ribbon.GetHeight() * 0.07);

                wxGraphicsPath hairPath = gc->CreatePath();

                const double topX = ribbon.GetX() + (ribbon.GetWidth() * 0.90);
                const double topY = ribbon.GetY() + (ribbon.GetHeight() * 0.05);

                const auto sy = [&](double y) { return topY + ((y - topY) * 0.82); };

                hairPath.MoveToPoint(topX, topY);

                // first curve outward
                hairPath.AddQuadCurveToPoint(
                    ribbon.GetX() - (ribbon.GetWidth() * 0.30),
                    sy(ribbon.GetY() + (ribbon.GetHeight() * 0.40)),
                    ribbon.GetX() + (ribbon.GetWidth() * math_constants::fifth),
                    sy(ribbon.GetY() + (ribbon.GetHeight() * math_constants::three_quarters)));

                // stronger second wave
                hairPath.AddQuadCurveToPoint(ribbon.GetX() - (ribbon.GetWidth() * 0.40),
                                             sy(ribbon.GetY() + ribbon.GetHeight()),
                                             ribbon.GetX() + (ribbon.GetWidth() * 0.35),
                                             sy(ribbon.GetY() + (ribbon.GetHeight() * 1.15)));

                hairPath.CloseSubpath();

                fillHairShaded(hairPath, ribbon);
                }

            gc->PopState();

            // inner frame
            //------------
            const wxColour frameColor = wxColour{ L"#525B54" };

            const double tbThickness = billRect.GetHeight() * 0.085;
            const double lrThickness = billRect.GetHeight() * 0.035;

            const double innerBillX = innerBillRect.GetX();
            const double innerBillY = innerBillRect.GetY();
            const double innerBillWidth = innerBillRect.GetWidth();
            const double innerBillHeight = innerBillRect.GetHeight();

            gc->SetBrush(wxColour{ 0, 0, 0, 0 });

            // top and bottom side of inner frame
            gc->SetPen(wxPenInfo{ Colors::ColorContrast::ChangeOpacity(frameColor, 125),
                                  static_cast<int>(tbThickness) }
                           .Cap(wxCAP_BUTT));
            gc->StrokeLine(innerBillX, innerBillY + (tbThickness * math_constants::half),
                           innerBillX + innerBillWidth,
                           innerBillY + (tbThickness * math_constants::half));
            gc->StrokeLine(innerBillX,
                           innerBillY + innerBillHeight - (tbThickness * math_constants::half),
                           innerBillX + innerBillWidth,
                           innerBillY + innerBillHeight - (tbThickness * math_constants::half));
            // left and right side of inner frame
            gc->SetPen(wxPenInfo{ Colors::ColorContrast::ChangeOpacity(frameColor, 125),
                                  static_cast<int>(lrThickness) }
                           .Cap(wxCAP_BUTT));
            gc->StrokeLine(innerBillX + (lrThickness * math_constants::half), innerBillY,
                           innerBillX + (lrThickness * math_constants::half),
                           innerBillY + innerBillHeight);
            gc->StrokeLine(innerBillX + innerBillWidth - (lrThickness * math_constants::half),
                           innerBillY,
                           innerBillX + innerBillWidth - (lrThickness * math_constants::half),
                           innerBillY + innerBillHeight);
            gc->SetPen(wxPenInfo{ Colors::ColorContrast::ChangeOpacity(frameColor, 125),
                                  static_cast<int>(lrThickness) }
                           .Cap(wxCAP_BUTT)
                           .Style(wxPenStyle::wxPENSTYLE_SHORT_DASH));
            gc->StrokeLine(innerBillX + lrThickness, innerBillY, innerBillX + lrThickness,
                           innerBillY + innerBillHeight);
            gc->StrokeLine(innerBillX + innerBillWidth - lrThickness, innerBillY,
                           innerBillX + innerBillWidth - lrThickness, innerBillY + innerBillHeight);

            // Corner ornament
            gc->SetPen(wxPen{ wxColour(0, 0, 0, 0) });
            gc->SetBrush(Colors::ColorContrast::ChangeOpacity(frameColor, 175));

            auto drawCorner =
                [&](const double cx, const double cy, const bool flipX, const bool flipY)
            {
                wxGraphicsPath p = gc->CreatePath();

                const double cs = ScaleToScreenAndCanvas(4.0);

                // corner size horizontally
                const double w = cs;
                // half-height vertically
                const double h = cs * math_constants::half;

                // x-pos direction
                const double dx = flipX ? -1.0 : 1.0;
                // y-pos direction
                const double dy = flipY ? -1.0 : 1.0;

                // Base anchor (corner)
                const double x0 = cx;
                const double y0 = cy;

                // Horizontal extent
                const double x1 = cx + (dx * w);
                const double y1 = cy;

                // Vertical extent (half height)
                const double x2 = cx;
                const double y2 = cy + (dy * h);

                // Midpoint for curve control (concave inward)
                const double ctrlX = cx + (dx * (w * math_constants::half));
                const double ctrlY = cy + (dy * (h * math_constants::half));

                // Build shape
                p.MoveToPoint(x0, y0);
                p.AddLineToPoint(x1, y1);

                // Concave inward curve from (x1,y1) → (x2,y2)
                p.AddQuadCurveToPoint(ctrlX, ctrlY, x2, y2);

                p.CloseSubpath();

                gc->FillPath(p);
            };

            // top-Left
            drawCorner(innerBillX, innerBillY, false, false);
            // top-Right
            drawCorner(innerBillX + innerBillWidth, innerBillY, true, false);
            // bottom-Left
            drawCorner(innerBillX, innerBillY + innerBillHeight, false, true);
            // bottom-Right
            drawCorner(innerBillX + innerBillWidth, innerBillY + innerBillHeight, true, true);

            // security strip
            //---------------
            wxRect2DDouble securityStripRect{ billRect };
            securityStripRect.SetWidth(securityStripRect.GetWidth() * 0.05);
            securityStripRect.MoveLeftTo(portraitRect.GetRight());
            gc->SetPen(wxColour{ 0, 0, 0, 0 });
            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(securityStripRect, 0.0), GetYPosFromTop(securityStripRect, 0.0),
                GetXPosFromLeft(securityStripRect, 0.0), GetYPosFromTop(securityStripRect, 1.0),
                Colors::ColorBrewer::GetColor(Colors::Color::Blue, 125),
                Colors::ColorBrewer::GetColor(Colors::Color::Gray, 125)));
            gc->DrawRectangle(securityStripRect);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(securityStripRect, 0.0), GetYPosFromTop(securityStripRect, 0.0),
                GetXPosFromLeft(securityStripRect, 0.0), GetYPosFromTop(securityStripRect, 1.0),
                Colors::ColorBrewer::GetColor(Colors::Color::Gray, 50),
                Colors::ColorBrewer::GetColor(Colors::Color::OutrageousOrange, 50)));
            gc->DrawRectangle(securityStripRect);

            // orange 100 in the bottom corner
            wxRect2DDouble monetaryAmountRect{ billRect };
            monetaryAmountRect.SetWidth(billRect.GetWidth() * math_constants::fifth);
            monetaryAmountRect.SetHeight(billRect.GetHeight() * math_constants::quarter);
            monetaryAmountRect.MoveRightTo(billRect.GetRight() - (billRect.GetWidth() * 0.05));
            monetaryAmountRect.MoveBottomTo(billRect.GetBottom() -
                                            (billRect.GetHeight() * math_constants::tenth));

            auto drawAmount = [&](const wxRect2DDouble& r, const wxColour& color)
            {
                Label amountLabel{ GraphItemInfo{ L"100" }
                                       .Pen(wxNullPen)
                                       .FontColor(color)
                                       .LabelAlignment(TextAlignment::Centered)
                                       .DPIScaling(GetDPIScaleFactor()) };
                amountLabel.SetBoundingBox(r.ToRect(), dc, GetScaling());
                amountLabel.Draw(dc);
            };

            drawAmount(monetaryAmountRect,
                       Colors::ColorBrewer::GetColor(Colors::Color::OutrageousOrange));

            // dark green 100 in upper corner
            monetaryAmountRect.MoveTopTo(billRect.GetTop() +
                                         (billRect.GetHeight() * math_constants::tenth));
            monetaryAmountRect.Deflate(monetaryAmountRect.GetWidth() * math_constants::tenth);
            drawAmount(monetaryAmountRect, frameColor);

            // top left corner
            monetaryAmountRect.MoveLeftTo(billRect.GetLeft() + (billRect.GetHeight() * 0.15));
            drawAmount(monetaryAmountRect, frameColor);

            // bottom left corner
            monetaryAmountRect.MoveBottomTo(billRect.GetBottom() -
                                            (billRect.GetHeight() * math_constants::tenth));
            drawAmount(monetaryAmountRect, frameColor);

                // Seals
                //------

                // left seal
                {
                const double sealDiameter = innerBillRect.GetHeight() * math_constants::third;
                const double sealX = GetXPosFromLeft(innerBillRect, math_constants::tenth);
                const double sealY = GetYPosFromTop(innerBillRect, math_constants::third);

                const double outerRingThickness = sealDiameter * math_constants::tenth;
                const double innerGap = sealDiameter * 0.04;

                    // outer ring
                    {
                    gc->SetPen(wxPenInfo{ ApplyColorOpacity(wxColour{ 0, 0, 0 }),
                                          static_cast<int>(outerRingThickness) }
                                   .Cap(wxCAP_BUTT));
                    gc->SetBrush(wxColour{ 0, 0, 0, 0 });

                    gc->DrawEllipse(sealX, sealY, sealDiameter, sealDiameter);
                    }

                // inner shield area
                const wxRect2DDouble shieldRect{
                    sealX + outerRingThickness + innerGap, sealY + outerRingThickness + innerGap,
                    sealDiameter - (2 * (outerRingThickness + innerGap)),
                    sealDiameter - (2 * (outerRingThickness + innerGap))
                };

                const double lineWidth =
                    std::max(1.0, ScaleToScreenAndCanvas(math_constants::quarter));

                gc->SetPen(wxPenInfo{ wxColour{ L"#4A4A4A" }, static_cast<int>(lineWidth) });
                gc->SetBrush(wxColour{ 0, 0, 0, 0 });

                constexpr int LINE_COUNT{ 4 };
                const double spacing = shieldRect.GetWidth() / (LINE_COUNT + 1);

                const double insetTop = shieldRect.GetHeight() * math_constants::tenth;
                const double insetBottom = shieldRect.GetHeight() * math_constants::tenth;
                const double bottomExtra = shieldRect.GetHeight() * math_constants::quarter;

                for (int i = 1; i <= LINE_COUNT; ++i)
                    {
                    const bool isOuterLine = (i == 1 || i == LINE_COUNT);

                    const double topY = shieldRect.GetY() + insetTop;

                    // outer lines end higher (taper bottom)
                    const double bottomY =
                        shieldRect.GetY() + shieldRect.GetHeight() -
                        (isOuterLine ? (insetBottom + bottomExtra) : insetBottom);

                    const double xPos = shieldRect.GetX() + (spacing * i);

                    gc->StrokeLine(xPos, topY, xPos, bottomY);
                    }
                }

                // right seal
                {
                const wxRect2DDouble rightSealRect{
                    GetXPosFromLeft(
                        innerBillRect,
                        0.9 - safe_divide<double>(innerBillRect.GetHeight() * math_constants::third,
                                                  innerBillRect.GetWidth())),
                    GetYPosFromTop(innerBillRect, math_constants::third),
                    innerBillRect.GetHeight() * math_constants::third,
                    innerBillRect.GetHeight() * math_constants::third
                };

                gc->SetPen(wxPenInfo{ ApplyColorOpacity(wxColour{ L"#689E80" }),
                                      static_cast<int>(billRect.GetHeight() * 0.05) });
                gc->SetBrush(wxColour{ 0, 0, 0, 0 });
                gc->DrawEllipse(rightSealRect);

                // balance scales icon inside of seal
                const double outerThickness = billRect.GetHeight() * 0.05;
                wxRect2DDouble scaleRect = rightSealRect;
                scaleRect.Deflate(outerThickness * 1.1);

                const double cx = scaleRect.GetX() + (scaleRect.GetWidth() / 2.0);
                const double cy = scaleRect.GetY() + (scaleRect.GetHeight() / 2.0);

                const double poleHeight = scaleRect.GetHeight() * 0.55;
                const double barWidth = scaleRect.GetWidth() * 0.60;

                // how far cups hang
                const double armDrop = scaleRect.GetHeight() * 0.22;
                const double cupWidth = scaleRect.GetWidth() * 0.22;
                const double cupHeight = scaleRect.GetHeight() * 0.08;

                const wxColour scaleColor = Colors::ColorContrast::ShadeOrTint(
                    ApplyColorOpacity(wxColour{ L"#689E80" }), -20);

                const wxPen scalePen{ scaleColor,
                                      static_cast<int>(std::max(
                                          1.0, ScaleToScreenAndCanvas(math_constants::half))) };

                gc->SetPen(scalePen);
                gc->SetBrush(wxColour{ 0, 0, 0, 0 });

                // vertical pole (center post)
                gc->StrokeLine(cx, cy - (poleHeight * math_constants::half), cx,
                               cy + (poleHeight * math_constants::half));

                // crossbar (top beam)
                gc->StrokeLine(cx - (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half),
                               cx + (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half));

                // suspension lines for the cups

                // left drop
                gc->StrokeLine(cx - (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half),
                               cx - (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half) + armDrop);

                // right drop
                gc->StrokeLine(cx + (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half),
                               cx + (barWidth * math_constants::half),
                               cy - (poleHeight * math_constants::half) + armDrop);

                // scale pans (simple small ellipses)

                // left cup
                gc->DrawEllipse(
                    cx - (barWidth * math_constants::half) - (cupWidth * math_constants::half),
                    cy - (poleHeight * math_constants::half) + armDrop, cupWidth, cupHeight);

                // right cup
                gc->DrawEllipse(
                    cx + (barWidth * math_constants::half) - (cupWidth * math_constants::half),
                    cy - (poleHeight * math_constants::half) + armDrop, cupWidth, cupHeight);
                }

                // inkwell
                {
                wxRect2DDouble inkwellRect{ billRect };
                inkwellRect.SetWidth(billRect.GetWidth() * 0.14);
                inkwellRect.SetHeight(billRect.GetHeight() * 0.22);
                inkwellRect.MoveLeftTo(billRect.GetLeft() + (billRect.GetWidth() * 0.59));
                inkwellRect.MoveTopTo(billRect.GetTop() + (billRect.GetHeight() * 0.55));

                const double leftX = inkwellRect.GetX();
                const double topY = inkwellRect.GetY();
                const double width = inkwellRect.GetWidth();
                const double height = inkwellRect.GetHeight();

                // top (lid), neck (narrow part), and body proportions
                const double lidHeight = height * 0.12;
                const double neckHeight = height * math_constants::fifth;
                const double bodyTopY = topY + lidHeight + neckHeight;
                const double bottomY = topY + height;

                const double lidLeftX = leftX + (width * 0.30);
                const double lidRightX = leftX + (width * 0.70);

                const double neckLeftX = leftX + (width * 0.32);
                const double neckRightX = leftX + (width * 0.68);

                const double bodyLeftX = leftX + (width * math_constants::quarter);
                const double bodyRightX = leftX + (width * math_constants::three_quarters);

                // gradient from copper-orange to yellow-orange
                const wxGraphicsBrush gradient = gc->CreateLinearGradientBrush(
                    leftX, topY, leftX, bottomY,
                    Colors::ColorBrewer::GetColor(Colors::Color::OutrageousOrange, 200),
                    Colors::ColorBrewer::GetColor(Colors::Color::OrangeYellow, 200));

                gc->SetBrush(gradient);
                gc->SetPen(
                    wxPenInfo{ Colors::ColorBrewer::GetColor(Colors::Color::OutrageousOrange, 200),
                               std::max<int>(1, ScaleToScreenAndCanvas(math_constants::quarter)) });

                wxGraphicsPath inkwellPath = gc->CreatePath();

                // lid
                inkwellPath.MoveToPoint(lidLeftX, topY);
                inkwellPath.AddLineToPoint(lidRightX, topY);
                inkwellPath.AddLineToPoint(lidRightX, topY + lidHeight);

                // right neck side
                inkwellPath.AddQuadCurveToPoint(
                    leftX + (width * 0.78), topY + lidHeight + (neckHeight * math_constants::half),
                    neckRightX, bodyTopY);
                // right side
                inkwellPath.AddLineToPoint(bodyRightX, bottomY);
                // bottom
                inkwellPath.AddLineToPoint(bodyLeftX, bottomY);
                // left bdy
                inkwellPath.AddLineToPoint(neckLeftX, bodyTopY);
                // left neck side
                inkwellPath.AddQuadCurveToPoint(
                    leftX + (width * 0.22), topY + lidHeight + (neckHeight * math_constants::half),
                    lidLeftX, topY + lidHeight);
                // back at lid
                inkwellPath.AddLineToPoint(lidLeftX, topY);
                inkwellPath.CloseSubpath();

                gc->FillPath(inkwellPath);
                }
            }
        }
    } // namespace Wisteria::GraphItems
