///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_food.cpp
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
    void ShapeRenderer::DrawRedApple(const wxRect rect, wxDC& dc) const
        {
        DrawApple(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::CandyApple));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGrannySmithApple(const wxRect rect, wxDC& dc) const
        {
        DrawApple(rect, dc, Colors::ColorBrewer::GetColor(Colors::Color::GrannySmithApple));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawApple(const wxRect rect, wxDC& dc, const wxColour& color) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        // Increase deflation even more to provide a safer buffer for high-res PDF clipping
        // boundaries
        drawRect.Deflate(static_cast<int>(ScaleToScreenAndCanvas(2)));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for apple!");
        if (gc != nullptr)
            {
            gc->PushState();
            gc->Translate(rect.x, rect.y);
            const wxRect2DDouble localRect(0, 0, rect.width, rect.height);
            const wxRect2DDouble localDrawRect(drawRect.x - rect.x, drawRect.y - rect.y,
                                               drawRect.width, drawRect.height);

            gc->SetPen(wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black),
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });

            gc->SetBrush(gc->CreateLinearGradientBrush(
                GetXPosFromLeft(localRect, 0), GetYPosFromTop(localRect, math_constants::half),
                GetXPosFromLeft(localRect, math_constants::three_fourths),
                GetYPosFromTop(localRect, math_constants::half),
                ApplyColorOpacity(
                    Colors::ColorContrast::ShadeOrTint(color, math_constants::three_fourths)),
                ApplyColorOpacity(color)));

            wxGraphicsPath applePath = gc->CreatePath();

            applePath.MoveToPoint(GetXPosFromLeft(localDrawRect, math_constants::half),
                                  GetYPosFromTop(localDrawRect, 0.3));
            // left side - symmetric with right side
            applePath.AddCurveToPoint(GetXPosFromLeft(localDrawRect, 0.08),
                                      GetYPosFromTop(localDrawRect, 0),
                                      GetXPosFromLeft(localDrawRect, math_constants::fifth),
                                      GetYPosFromTop(localDrawRect, 0.9),
                                      GetXPosFromLeft(localDrawRect, math_constants::half),
                                      GetYPosFromTop(localDrawRect, 0.7));
            // right side - clamp to avoid clipping boundaries
            applePath.AddCurveToPoint(
                GetXPosFromLeft(localDrawRect, 0.8), GetYPosFromTop(localDrawRect, 0.9),
                GetXPosFromLeft(localDrawRect, 0.92), GetYPosFromTop(localDrawRect, 0),
                GetXPosFromLeft(localDrawRect, math_constants::half),
                GetYPosFromTop(localDrawRect, 0.3));

            applePath.CloseSubpath();
            gc->FillPath(applePath);
            gc->StrokePath(applePath);

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

            // leaf
            gc->SetBrush(Colors::ColorBrewer::GetColor(Colors::Color::JungleGreen));
            gc->SetPen(wxColour{ 0, 0, 0, 0 });

            wxGraphicsPath leafPath = gc->CreatePath();

            leafPath.MoveToPoint(GetXPosFromLeft(localDrawRect, math_constants::half),
                                 GetYPosFromTop(localDrawRect, 0.3));
            leafPath.AddQuadCurveToPoint(GetXPosFromLeft(localDrawRect, 0.325),
                                         GetYPosFromTop(localDrawRect, math_constants::fifth),
                                         GetXPosFromLeft(localDrawRect, math_constants::quarter),
                                         GetYPosFromTop(localDrawRect, math_constants::tenth));
            leafPath.AddQuadCurveToPoint(GetXPosFromLeft(localDrawRect, 0.475),
                                         GetYPosFromTop(localDrawRect, math_constants::tenth),
                                         GetXPosFromLeft(localDrawRect, math_constants::half),
                                         GetYPosFromTop(localDrawRect, 0.3));

            leafPath.CloseSubpath();
            gc->FillPath(leafPath);
            gc->StrokePath(leafPath);

            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::FillCarvedFeature(wxGraphicsContext* gc, const wxGraphicsPath& path,
                                          const wxRect2DDouble& bounds,
                                          const wxPolygonFillMode fillMode /*= wxWINDING_RULE*/)
        {
        // Dimmer, narrower glow
        const wxColour centerCol{ 235, 200, 100, 255 }; // muted candlelight
        const wxColour edgeCol{ 0, 0, 0, 255 };         // solid black edge

        const double cx = bounds.m_x + (bounds.m_width * math_constants::half);
        const double cy = bounds.m_y + (bounds.m_height * 0.4);
        const double radius = std::max(bounds.m_width, bounds.m_height) * 0.30;

        const auto brush =
            gc->CreateRadialGradientBrush(cx, cy, cx, cy, radius, centerCol, edgeCol);

        gc->SetBrush(brush);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->FillPath(path, fillMode);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCheesePizza(wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        rect.Deflate(ScaleToScreenAndCanvas(2));

        // cheese color
        const wxColour cheeseColor{ 255, 235, 190 };
        const wxColour crustColor{ 210, 170, 110 };
        const wxColour crustEdgeColor{ 180, 140, 80 };
        const wxColour toastedSpotColor{ 215, 185, 120, 120 };

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double cy = rect.GetY() + (rect.GetHeight() / 2.0);
        const double radius = GetRadius(rect);
        const double crustThickness = std::max(2.0, radius * 0.12);

        // draw cheese base
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ cheeseColor });
        gc->DrawEllipse(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

        // draw crust ring
        const auto outlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(0.5));
        gc->SetPen(wxPen{ crustEdgeColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ crustColor });

        wxGraphicsPath crustPath = gc->CreatePath();
        crustPath.AddEllipse(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
        const double innerRadius = radius - crustThickness;
        const double innerDiameter = innerRadius * 2;
        crustPath.AddEllipse(cx - innerRadius, cy - innerRadius, innerDiameter, innerDiameter);
        gc->FillPath(crustPath, wxODDEVEN_RULE);
        gc->StrokePath(crustPath);

        // add toasted cheese spots
        constexpr int SPOT_COUNT{ 5 };
        constexpr uint32_t SPOT_SEED{ 0xBEEFCAFE };
        const double maxSpotDistance = innerRadius * 0.75;

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(toastedSpotColor));

        for (int i = 0; i < SPOT_COUNT; ++i)
            {
            const uint32_t seed = SPOT_SEED + static_cast<uint32_t>(i * 733);
            const double angle = (360.0 * i / SPOT_COUNT) + ((seed % 30) - 15);
            const double distance = maxSpotDistance * (0.3 + 0.7 * ((seed % 100) / 100.0));
            const double spotRadius = crustThickness * (0.8 + 0.6 * ((seed % 50) / 50.0));

            const double spotX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double spotY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            gc->DrawEllipse(spotX - spotRadius, spotY - spotRadius, spotRadius * 2, spotRadius * 2);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPepperoniPizza(wxRect rect, wxDC& dc) const
        {
        // draw the cheese pizza base first
        DrawCheesePizza(rect, dc);

        // now add pepperoni on top
        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        rect.Deflate(ScaleToScreenAndCanvas(2));

        const wxColour pepperoniFillColor{ 170, 45, 45 };
        const wxColour pepperoniEdgeColor{ 110, 20, 20 };

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double cy = rect.GetY() + (rect.GetHeight() / 2.0);
        const double radius = GetRadius(rect);
        const double crustThickness = std::max(2.0, radius * 0.12);
        const double innerRadius = radius - crustThickness;

        constexpr int PEPPERONI_COUNT{ 5 };
        const double pepperoniRadius = std::max(2.0, radius * 0.15);
        const double minPepperoniDistance = innerRadius * 0.25;
        const double maxPepperoniDistance = innerRadius * 0.70;

        const auto outlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(0.5));
        gc->SetPen(wxPen{ pepperoniEdgeColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ pepperoniFillColor });

        // 5 pepperoni spread 72° apart, starting from top-right
        constexpr std::array<double, PEPPERONI_COUNT> ANGLE_OFFSETS = { -45.0, 27.0, 99.0, 171.0,
                                                                        243.0 };
        // alternating distances to avoid clustering
        constexpr std::array<double, PEPPERONI_COUNT> DISTANCE_FACTORS = { 0.80, 0.50, 0.85, 0.45,
                                                                           0.75 };

        for (int i = 0; i < PEPPERONI_COUNT; ++i)
            {
            const double angle = ANGLE_OFFSETS[i];
            const double distance =
                minPepperoniDistance +
                (maxPepperoniDistance - minPepperoniDistance) * DISTANCE_FACTORS[i];

            const double pepX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double pepY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            gc->DrawEllipse(pepX - pepperoniRadius, pepY - pepperoniRadius, pepperoniRadius * 2,
                            pepperoniRadius * 2);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHawaiianPizza(const wxRect rect, wxDC& dc) const
        {
        // draw the pepperoni pizza base first
        // (technically, it should be Canadian bacon)
        DrawPepperoniPizza(rect, dc);

        // now add pineapple chunks on top
        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        const wxColour pineappleFillColor{ 240, 220, 80 };
        const wxColour pineappleEdgeColor{ 200, 180, 60 };

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double cy = rect.GetY() + (rect.GetHeight() / 2.0);
        const double radius = GetRadius(rect);
        const double crustThickness = std::max(2.0, radius * 0.12);
        const double innerRadius = radius - crustThickness;

        // pineapple chunks - rectangular, about half pepperoni size, 50% longer
        constexpr int PINEAPPLE_COUNT{ 10 };
        const double pineappleWidth = std::max(2.0, radius * 0.12); // 50% longer
        const double pineappleHeight = std::max(1.5, radius * 0.06);
        const double minPineappleDistance = innerRadius * 0.15;
        const double maxPineappleDistance = innerRadius * 0.65;

        const auto outlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(0.5));
        gc->SetPen(wxPen{ pineappleEdgeColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ pineappleFillColor });

        // offset from pepperoni positions so they don't overlap directly
        constexpr std::array<double, PINEAPPLE_COUNT> ANGLE_OFFSETS = { 0.0,   36.0,  72.0,  108.0,
                                                                        144.0, 180.0, 216.0, 252.0,
                                                                        288.0, 324.0 };
        constexpr std::array<double, PINEAPPLE_COUNT> DISTANCE_FACTORS = { 0.55, 0.85, 0.40, 0.70,
                                                                           0.90, 0.35, 0.75, 0.50,
                                                                           0.80, 0.45 };
        // random rotation angles for each chunk (in radians)
        constexpr std::array<double, PINEAPPLE_COUNT> ROTATION_ANGLES = { 0.4, -0.7, 1.2, -0.3,
                                                                          0.9, -1.1, 0.6, -0.5,
                                                                          1.0, -0.2 };

        for (int i = 0; i < PINEAPPLE_COUNT; ++i)
            {
            const double angle = ANGLE_OFFSETS[i];
            const double distance =
                minPineappleDistance +
                (maxPineappleDistance - minPineappleDistance) * DISTANCE_FACTORS[i];

            const double pineX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double pineY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            // draw rotated rectangle using path and transform
            wxGraphicsPath chunkPath = gc->CreatePath();
            chunkPath.AddRectangle(-pineappleWidth / 2, -pineappleHeight / 2, pineappleWidth,
                                   pineappleHeight);

            wxGraphicsMatrix matrix = gc->CreateMatrix();
            matrix.Translate(pineX, pineY);
            matrix.Rotate(ROTATION_ANGLES[i]);
            chunkPath.Transform(matrix);

            gc->FillPath(chunkPath);
            gc->StrokePath(chunkPath);
            }

        // add cinnamon sprinkles on top
        const wxColour cinnamonColor{ 139, 90, 43, 180 };
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ cinnamonColor });

        constexpr int CINNAMON_COUNT{ 30 };
        constexpr uint32_t CINNAMON_SEED{ 0xC1AA0A };
        const double cinnamonSize = std::max(1.0, radius * 0.02);
        const double maxCinnamonDistance = innerRadius * 0.80;

        for (int i = 0; i < CINNAMON_COUNT; ++i)
            {
            const uint32_t seed = CINNAMON_SEED + static_cast<uint32_t>(i * 557);
            const double angle =
                safe_divide<double>(360.0 * i, CINNAMON_COUNT) + ((seed % 50) - 25);
            const double distance = maxCinnamonDistance * (0.2 + 0.8 * ((seed % 100) / 100.0));

            const double spiceX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double spiceY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            gc->DrawEllipse(spiceX - cinnamonSize, spiceY - cinnamonSize, cinnamonSize * 2,
                            cinnamonSize * 2);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawChocolateChipCookie(wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        rect.Deflate(ScaleToScreenAndCanvas(2));

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double cy = rect.GetY() + (rect.GetHeight() / 2.0);
        const double radius = GetRadius(rect);

        // cookie base - warm golden tan color with gradient
        const wxColour cookieBaseColor{ 210, 170, 110 };
        const wxColour cookieEdgeColor{ 180, 140, 80 };

        auto cookieBrush =
            gc->CreateRadialGradientBrush(cx, cy, cx, cy, radius, cookieBaseColor, cookieEdgeColor);
        gc->SetBrush(cookieBrush);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

        // toasted spots - darker brown spots scattered on the cookie
        const wxColour toastedColor{ 160, 120, 70, 180 };
        gc->SetBrush(wxBrush{ toastedColor });

        constexpr int TOASTED_SPOT_COUNT{ 6 };
        constexpr uint32_t TOASTED_SEED{ 0xC00C1E };
        const double toastedSpotSize = std::max(3.0, radius * 0.4);
        const double maxToastedDistance = radius * 0.85;

        for (int i = 0; i < TOASTED_SPOT_COUNT; ++i)
            {
            const uint32_t seed = TOASTED_SEED + static_cast<uint32_t>(i * 431);
            const double angle =
                safe_divide<double>(360.0 * i, TOASTED_SPOT_COUNT) + ((seed % 30) - 15);
            const double distance = maxToastedDistance * (0.3 + 0.6 * ((seed % 100) / 100.0));

            const double spotX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double spotY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            const double spotW = toastedSpotSize * (0.8 + 0.4 * (((seed * 7) % 100) / 100.0));
            const double spotH = toastedSpotSize * (0.6 + 0.3 * (((seed * 13) % 100) / 100.0));

            gc->DrawEllipse(spotX - (spotW / 2), spotY - (spotH / 2), spotW, spotH);
            }

        // chocolate chips - dark brown ellipses
        const wxColour chipFillColor{ 60, 30, 15 };
        const wxColour chipEdgeColor{ 40, 20, 10 };

        constexpr int CHIP_COUNT{ 7 };
        const double chipRadius = std::max(2.0, radius * 0.14);
        const double minChipDistance = radius * 0.15;
        const double maxChipDistance = radius * 0.70;

        const auto chipOutlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(0.5));
        gc->SetPen(wxPen{ chipEdgeColor, chipOutlinePenWidth });
        gc->SetBrush(wxBrush{ chipFillColor });

        // spread chips around the cookie
        constexpr std::array<double, CHIP_COUNT> CHIP_ANGLE_OFFSETS = { -30.0, 25.0,  80.0, 135.0,
                                                                        190.0, 250.0, 310.0 };
        constexpr std::array<double, CHIP_COUNT> CHIP_DISTANCE_FACTORS = { 0.65, 0.40, 0.75, 0.50,
                                                                           0.80, 0.35, 0.0 };

        const double scaledChipDiameter{ chipRadius * 2.0 };

        for (int i = 0; i < CHIP_COUNT; ++i)
            {
            const double angle = CHIP_ANGLE_OFFSETS[i];
            const double distance =
                minChipDistance + (maxChipDistance - minChipDistance) * CHIP_DISTANCE_FACTORS[i];

            const double chipX = cx + std::cos(angle * std::numbers::pi / 180.0) * distance;
            const double chipY = cy + std::sin(angle * std::numbers::pi / 180.0) * distance;

            gc->DrawEllipse(chipX - chipRadius, chipY - chipRadius, scaledChipDiameter,
                            scaledChipDiameter);

            // add sheen on chips
            const wxColour sheenColor{ 255, 255, 255, 60 };
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush{ sheenColor });

            const double sheenRadius = chipRadius * 0.35;
            const double sheenOffsetX = chipRadius * 0.25;
            const double sheenOffsetY = chipRadius * 0.25;

            gc->DrawEllipse(chipX - sheenOffsetX - sheenRadius, chipY - sheenOffsetY - sheenRadius,
                            sheenRadius * 2, sheenRadius * 2);

            // restore pen and brush for next chip
            gc->SetPen(wxPen{ chipEdgeColor, chipOutlinePenWidth });
            gc->SetBrush(wxBrush{ chipFillColor });
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCoffeeShopCup(wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        rect.Deflate(ScaleToScreenAndCanvas(2));

        // colors
        const wxColour cupColor{ 240, 200, 160 };         // tan/beige cup body
        const wxColour cupShadowColor{ 220, 180, 140 };   // slightly darker for depth
        const wxColour lidColor{ 139, 90, 43 };           // brown lid
        const wxColour lidDarkColor{ 100, 65, 30 };       // darker brown for lid edge
        const wxColour sleeveColor{ 60, 60, 60 };         // dark gray sleeve
        const wxColour logoBackgroundColor{ 50, 50, 50 }; // dark circle background
        const wxColour logoRingColor{ 80, 80, 80 };       // slightly lighter ring
        const wxColour beanColor{ 200, 180, 160 };        // light color for coffee bean
        const wxColour steamColor{ 180, 180, 180, 150 };  // semi-transparent gray steam

        const double width = rect.GetWidth();
        const double height = rect.GetHeight();

        // reserve space for steam at top
        const double steamHeight = height * 0.18;
        const double cupAreaTop = rect.GetY() + steamHeight;
        const double cupAreaHeight = height - steamHeight;

        // cup proportions (relative to cup area, not full rect)
        const double lidHeight = cupAreaHeight * 0.10;
        const double cupTopWidth = width * 0.48;
        const double cupBottomWidth = width * 0.36;
        const double lidWidth = width * 0.52;

        const double cx = rect.GetX() + (width / 2.0);
        const double cupTop = cupAreaTop + lidHeight;
        const double cupBottom = rect.GetY() + height;

        const auto outlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(math_constants::half));

        // draw steam swooshes first (so they appear behind the cup)
        const double steamBaseY = cupAreaTop;
        const double steamTopY = rect.GetY();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ steamColor });

        // large swoosh (left)
        wxGraphicsPath largeSwoosh = gc->CreatePath();
        const double largeX = cx - (width * 0.02);
        largeSwoosh.MoveToPoint(largeX, steamBaseY);
        // outer curve going up and curving right
        largeSwoosh.AddCurveToPoint(largeX - (width * 0.15), steamBaseY - steamHeight * 0.3,
                                    largeX - (width * 0.18), steamBaseY - steamHeight * 0.6,
                                    largeX - (width * 0.05), steamBaseY - steamHeight * 0.75);
        largeSwoosh.AddCurveToPoint(largeX + (width * 0.08), steamBaseY - steamHeight * 0.88,
                                    largeX + (width * 0.02), steamBaseY - steamHeight * 0.95,
                                    largeX - (width * 0.08), steamTopY);
        // inner curve coming back down
        largeSwoosh.AddCurveToPoint(largeX + (width * 0.06), steamBaseY - steamHeight * 0.90,
                                    largeX + (width * 0.12), steamBaseY - steamHeight * 0.80,
                                    largeX + (width * 0.02), steamBaseY - steamHeight * 0.65);
        largeSwoosh.AddCurveToPoint(largeX - (width * 0.10), steamBaseY - steamHeight * 0.50,
                                    largeX - (width * 0.08), steamBaseY - steamHeight * 0.25,
                                    largeX + (width * 0.06), steamBaseY);
        largeSwoosh.CloseSubpath();
        gc->FillPath(largeSwoosh);

        // small swoosh (spooning inside the large one, same direction, half size)
        wxGraphicsPath smallSwoosh = gc->CreatePath();
        const double smallX = cx + (width * 0.06);
        const double smallScale = 0.5;
        const double smallBaseY = steamBaseY - steamHeight * 0.08;
        const double smallSteamHeight = steamHeight * smallScale;
        smallSwoosh.MoveToPoint(smallX, smallBaseY);
        // outer curve going up and curving right (same direction as large)
        smallSwoosh.AddCurveToPoint(smallX - (width * 0.08), smallBaseY - smallSteamHeight * 0.3,
                                    smallX - (width * 0.10), smallBaseY - smallSteamHeight * 0.6,
                                    smallX - (width * 0.02), smallBaseY - smallSteamHeight * 0.75);
        smallSwoosh.AddCurveToPoint(smallX + (width * 0.05), smallBaseY - smallSteamHeight * 0.88,
                                    smallX + (width * 0.02), smallBaseY - smallSteamHeight * 0.95,
                                    smallX - (width * 0.04), smallBaseY - smallSteamHeight);
        // inner curve coming back down
        smallSwoosh.AddCurveToPoint(smallX + (width * 0.04), smallBaseY - smallSteamHeight * 0.90,
                                    smallX + (width * 0.07), smallBaseY - smallSteamHeight * 0.80,
                                    smallX + (width * 0.02), smallBaseY - smallSteamHeight * 0.65);
        smallSwoosh.AddCurveToPoint(smallX - (width * 0.04), smallBaseY - smallSteamHeight * 0.50,
                                    smallX - (width * 0.03), smallBaseY - smallSteamHeight * 0.25,
                                    smallX + (width * 0.04), smallBaseY);
        smallSwoosh.CloseSubpath();
        gc->FillPath(smallSwoosh);

        // draw cup body (tapered trapezoid)
        wxGraphicsPath cupPath = gc->CreatePath();
        cupPath.MoveToPoint(cx - (cupTopWidth / 2), cupTop);
        cupPath.AddLineToPoint(cx + (cupTopWidth / 2), cupTop);
        cupPath.AddLineToPoint(cx + (cupBottomWidth / 2), cupBottom);
        cupPath.AddLineToPoint(cx - (cupBottomWidth / 2), cupBottom);
        cupPath.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ cupColor });
        gc->FillPath(cupPath);

        // add curved shadow on left side of cup for cylindrical effect
        const double shadowWidth = cupTopWidth * 0.20;
        const double bottomShadowWidth = cupBottomWidth * 0.20;
        wxGraphicsPath shadowPath = gc->CreatePath();
        shadowPath.MoveToPoint(cx - (cupTopWidth / 2), cupTop);
        // curved inner edge using quadratic bezier
        const double shadowCtrlX = cx - (cupTopWidth / 2) + shadowWidth * 1.5;
        const double shadowCtrlY = (cupTop + cupBottom) / 2.0;
        shadowPath.AddQuadCurveToPoint(shadowCtrlX, shadowCtrlY,
                                       cx - (cupBottomWidth / 2) + bottomShadowWidth, cupBottom);
        shadowPath.AddLineToPoint(cx - (cupBottomWidth / 2), cupBottom);
        shadowPath.AddLineToPoint(cx - (cupTopWidth / 2), cupTop);
        shadowPath.CloseSubpath();

        gc->SetBrush(wxBrush{ cupShadowColor });
        gc->FillPath(shadowPath);

        // draw cup outline now (before sleeve so sleeve covers it)
        gc->SetPen(wxPen{ cupShadowColor, outlinePenWidth });
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(cupPath);

        // draw sleeve band (middle of cup) - slightly wider to cover cup outline
        const double sleeveTop = cupTop + (cupAreaHeight - lidHeight) * 0.32;
        const double sleeveBottom = cupTop + (cupAreaHeight - lidHeight) * 0.62;
        const double sleeveTopWidth =
            cupTopWidth -
            (cupTopWidth - cupBottomWidth) * ((sleeveTop - cupTop) / (cupBottom - cupTop));
        const double sleeveBottomWidth =
            cupTopWidth -
            (cupTopWidth - cupBottomWidth) * ((sleeveBottom - cupTop) / (cupBottom - cupTop));
        // add small padding to ensure sleeve covers cup outline
        const double sleevePadding = outlinePenWidth + 1;

        wxGraphicsPath sleevePath = gc->CreatePath();
        sleevePath.MoveToPoint(cx - (sleeveTopWidth / 2) - sleevePadding, sleeveTop);
        sleevePath.AddLineToPoint(cx + (sleeveTopWidth / 2) + sleevePadding, sleeveTop);
        sleevePath.AddLineToPoint(cx + (sleeveBottomWidth / 2) + sleevePadding, sleeveBottom);
        sleevePath.AddLineToPoint(cx - (sleeveBottomWidth / 2) - sleevePadding, sleeveBottom);
        sleevePath.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ sleeveColor });
        gc->FillPath(sleevePath);

        // draw circular logo on sleeve
        const double sleeveMiddleY = (sleeveTop + sleeveBottom) / 2.0;
        const double logoRadius = (sleeveBottom - sleeveTop) * 0.42;

        // outer logo circle (dark background)
        gc->SetPen(wxPen{ logoRingColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ logoBackgroundColor });
        gc->DrawEllipse(cx - logoRadius, sleeveMiddleY - logoRadius, logoRadius * 2,
                        logoRadius * 2);

        // inner ring
        const double innerRingRadius = logoRadius * 0.75;
        gc->SetPen(wxPen{ logoRingColor, outlinePenWidth });
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawEllipse(cx - innerRingRadius, sleeveMiddleY - innerRingRadius, innerRingRadius * 2,
                        innerRingRadius * 2);

        // draw coffee bean in center of logo (rotated 45 degrees)
        const double beanWidth = logoRadius * 0.80;
        const double beanHeight = logoRadius * 0.55;
        constexpr double BEAN_ROTATION = 45.0 * std::numbers::pi / 180.0;

        gc->SetPen(wxPen{ beanColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ beanColor });

        // bean ellipse with rotation
        wxGraphicsPath beanPath = gc->CreatePath();
        beanPath.AddEllipse(-beanWidth / 2, -beanHeight / 2, beanWidth, beanHeight);
        wxGraphicsMatrix beanMatrix = gc->CreateMatrix();
        beanMatrix.Translate(cx, sleeveMiddleY);
        beanMatrix.Rotate(BEAN_ROTATION);
        beanPath.Transform(beanMatrix);
        gc->FillPath(beanPath);
        gc->StrokePath(beanPath);

        // draw curved S-shaped crease along the bean's long axis (also rotated)
        gc->SetPen(wxPen{ logoBackgroundColor, outlinePenWidth });
        wxGraphicsPath creasePath = gc->CreatePath();
        const double creaseHalfLen = beanWidth * 0.35;
        creasePath.MoveToPoint(-creaseHalfLen, 0);
        // S-curve along the horizontal axis (bean's length)
        creasePath.AddQuadCurveToPoint(-beanWidth * 0.1, -beanHeight * 0.15, 0, 0);
        creasePath.AddQuadCurveToPoint(beanWidth * 0.1, beanHeight * 0.15, creaseHalfLen, 0);
        wxGraphicsMatrix creaseMatrix = gc->CreateMatrix();
        creaseMatrix.Translate(cx, sleeveMiddleY);
        creaseMatrix.Rotate(BEAN_ROTATION);
        creasePath.Transform(creaseMatrix);
        gc->StrokePath(creasePath);

        // draw lid
        const double lidTop = cupAreaTop;
        const double lidBottom = cupTop;

        wxGraphicsPath lidPath = gc->CreatePath();
        lidPath.MoveToPoint(cx - (lidWidth / 2), lidBottom);
        lidPath.AddLineToPoint(cx + (lidWidth / 2), lidBottom);
        lidPath.AddLineToPoint(cx + (lidWidth / 2) * 0.90, lidTop);
        lidPath.AddLineToPoint(cx - (lidWidth / 2) * 0.90, lidTop);
        lidPath.CloseSubpath();

        gc->SetPen(wxPen{ lidDarkColor, outlinePenWidth });
        gc->SetBrush(wxBrush{ lidColor });
        gc->FillPath(lidPath);
        gc->StrokePath(lidPath);

        // add rim detail on lid
        const double rimY = lidBottom - (lidHeight * 0.25);
        gc->SetPen(wxPen{ lidDarkColor, outlinePenWidth });
        gc->StrokeLine(cx - (lidWidth / 2) * 0.95, rimY, cx + (lidWidth / 2) * 0.95, rimY);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawJackOLantern(const wxRect rect, wxDC& dc) const
        {
        DrawPumpkin(rect, dc);

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for jack-o'-lantern!");
        if (gc == nullptr)
            {
            return;
            }

        gc->SetPen(*wxBLACK_PEN);

        const double cx = rect.GetX() + (rect.GetWidth() / 2.0);
        const double width = rect.GetWidth();
        const double height = rect.GetHeight();

        //--------------------------------------------------
        // Eyes
        //--------------------------------------------------
        const double eyeW = width * 0.14;
        const double eyeH = height * 0.12;
        const double eyeOffsetX = width * 0.12;
        const double eyeTopY = rect.GetY() + (height * 0.45);
        constexpr double EYE_TILT_RAD = 45.0 * (std::numbers::pi / 180.0);

        const auto drawEye = [&](const bool left)
        {
            const double centerX = cx + (left ? -eyeOffsetX : eyeOffsetX);
            const double centerY = eyeTopY + (eyeH / 2.0);

            // build eye in local space, then transform into world space
            wxGraphicsPath eye = gc->CreatePath();
            eye.MoveToPoint(-eyeW / 2.0, 0.0);
            eye.AddLineToPoint(eyeW / 2.0, 0.0);
            eye.AddArc(0.0, 0.0, eyeW / 2.0, 0.0, std::numbers::pi, true);
            eye.CloseSubpath();

            wxGraphicsMatrix matrix = gc->CreateMatrix();
            matrix.Translate(centerX, centerY);
            matrix.Rotate(left ? EYE_TILT_RAD : -EYE_TILT_RAD);
            eye.Transform(matrix);

            FillCarvedFeature(
                gc, eye,
                wxRect2DDouble(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()));
        };

        drawEye(true);
        drawEye(false);

        //--------------------------------------------------
        // Nose
        //--------------------------------------------------
        const double noseW = width * math_constants::tenth;
        const double noseH = height * math_constants::tenth;
        const double noseY = rect.GetY() + (height * 0.53) + noseH;

        wxGraphicsPath nose = gc->CreatePath();
        nose.MoveToPoint(cx, noseY - (noseH / 2.0));
        nose.AddLineToPoint(cx - (noseW / 2.0), noseY + (noseH / 2.0));
        nose.AddLineToPoint(cx + (noseW / 2.0), noseY + (noseH / 2.0));
        nose.CloseSubpath();
        FillCarvedFeature(
            gc, nose, wxRect2DDouble(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()));

        //--------------------------------------------------
        // Mouth
        //--------------------------------------------------
        const double mouthTop = rect.GetY() + (height * 0.63);
        const double mouthW = width * 0.55;
        const double mouthH = height * 0.40;

        wxGraphicsPath mouth = gc->CreatePath();
        mouth.MoveToPoint(cx - (mouthW / 2.0), mouthTop);
        mouth.AddQuadCurveToPoint(cx, mouthTop + mouthH, cx + (mouthW / 2.0), mouthTop);
        mouth.AddQuadCurveToPoint(cx, mouthTop + (mouthH * 0.6), cx - (mouthW / 2.0), mouthTop);
        mouth.CloseSubpath();

        FillCarvedFeature(
            gc, mouth, wxRect2DDouble(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight()));

        //--------------------------------------------------
        // Teeth (erase from mouth by redrawing pumpkin)
        //--------------------------------------------------
        const double toothW = mouthW * 0.08;
        const double toothH = mouthH * math_constants::quarter;
        const double upperOffset = mouthW * math_constants::fifth;

        auto drawTooth = [&](const double x, const double y, const double w, const double h)
        {
            gc->PushState();
            gc->Clip(x, y, w, h);
            DrawPumpkin(rect, dc);
            gc->PopState();
        };

        drawTooth(cx - upperOffset - (toothW / 2.0), mouthTop + (mouthH * math_constants::tenth),
                  toothW, toothH);
        drawTooth(cx + upperOffset - (toothW / 2.0), mouthTop + (mouthH * math_constants::tenth),
                  toothW, toothH);
        drawTooth(cx - (toothW / 2.0), mouthTop + (mouthH * 0.40), toothW, toothH * 1.6);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPumpkin(wxRect rect, wxDC& dc) const
        {
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect bodyRect{ rect };
        if (bodyRect.GetHeight() > bodyRect.GetWidth() * 1.25)
            {
            const int targetHeight = static_cast<int>(bodyRect.GetWidth() * 1.25);
            const int dy = (bodyRect.GetHeight() - targetHeight) / 2;
            bodyRect.SetHeight(targetHeight);
            bodyRect.Offset(0, dy);
            }

        bodyRect.Deflate(static_cast<int>(bodyRect.GetWidth() * 0.06),
                         static_cast<int>(bodyRect.GetHeight() * 0.08));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for pumpkin!");
        if (gc == nullptr)
            {
            return;
            }

        const auto outlinePenWidth = std::max<int>(1, ScaleToScreenAndCanvas(math_constants::half));

        const double leftX = bodyRect.GetLeft();
        const double rightX = bodyRect.GetRight();
        const double topY = bodyRect.GetTop() + (bodyRect.GetHeight() * 0.15);
        const double bottomY = bodyRect.GetTop() + (bodyRect.GetHeight() * 0.95);
        const double midY = (topY + bottomY) / 2.0;
        const double halfHeightBase = (bottomY - topY) / 2.0;
        const double bottomBulge = (bottomY - topY) * 0.08;
        const double cx = (leftX + rightX) / 2.0;

        // gradient: lighter top, very vibrant bottom
        const wxColour topOrange{ TintIfUsingOpacity(wxColour{ 255, 205, 80 }) };
        const wxColour bottomOrange{ TintIfUsingOpacity(wxColour{ 255, 90, 0 }) };
        const wxColour lobeEdge{ TintIfUsingOpacity(wxColour{ 220, 110, 0, 200 }) };

        const wxGraphicsPen lobePen = gc->CreatePen(wxPen{ lobeEdge, outlinePenWidth });

        struct LobeInfo
            {
            double m_centerPercent{ 0.0 };
            double m_foldPercent{ 0.0 };
            double m_widthFactor{ 0.0 };
            double m_heightFactor{ 0.0 };
            };

        constexpr std::array<LobeInfo, 7> LOBES{ { { 0.16, 0.26, 0.80, 0.85 },
                                                   { 0.28, 0.36, 0.95, 0.95 },
                                                   { 0.40, 0.48, 1.05, 1.00 },
                                                   { 0.50, 0.50, 1.15, 1.05 },
                                                   { 0.60, 0.52, 1.05, 1.00 },
                                                   { 0.72, 0.64, 0.95, 0.95 },
                                                   { 0.84, 0.74, 0.80, 0.85 } } };

        // draw order so right-side lobes are underneath the ones to their left
        constexpr std::array DRAW_ORDER{ 6, 5, 4, 0, 1, 2, 3 };

            //--------------------------------------------------
            // back caps at the very top (peek of back lobes)
            //--------------------------------------------------
            {
            // slightly lighter than the main body, but fully opaque
            const wxGraphicsBrush backBrush =
                gc->CreateBrush(wxBrush{ TintIfUsingOpacity(wxColour{ 255, 235, 140 }) });

            // lighter outline than front lobes
            const wxGraphicsPen backPen =
                gc->CreatePen(wxPen{ TintIfUsingOpacity(wxColour{ 235, 170, 90 }), // softer edge
                                     outlinePenWidth });

            gc->SetBrush(backBrush);
            gc->SetPen(backPen);

            for (const auto& lobe : LOBES)
                {
                // pull back caps inward toward center so they don't stick out so far
                const double centerXRaw = leftX + (bodyRect.GetWidth() * lobe.m_centerPercent);
                const double capCenterX = cx + ((centerXRaw - cx) * 0.65); // 0.65 = more inside

                const double halfWidth =
                    (bodyRect.GetWidth() / 7.0) * lobe.m_widthFactor * 0.4;            // narrower
                const double halfHeight = halfHeightBase * lobe.m_heightFactor * 0.18; // shorter

                // lower the caps so they sit into the pumpkin, not above it
                const double capMidY = topY + ((bottomY - topY) * math_constants::fifth);

                wxGraphicsPath cap = gc->CreatePath();
                cap.MoveToPoint(capCenterX - halfWidth, capMidY);
                cap.AddCurveToPoint(wxPoint(capCenterX - (halfWidth * 0.4), capMidY - halfHeight),
                                    wxPoint(capCenterX + (halfWidth * 0.4), capMidY - halfHeight),
                                    wxPoint(capCenterX + halfWidth, capMidY));
                cap.AddCurveToPoint(wxPoint(capCenterX + (halfWidth * 0.4),
                                            capMidY + (halfHeight * math_constants::half)),
                                    wxPoint(capCenterX - (halfWidth * 0.4),
                                            capMidY + (halfHeight * math_constants::half)),
                                    wxPoint(capCenterX - halfWidth, capMidY));
                cap.CloseSubpath();

                gc->FillPath(cap);
                gc->StrokePath(cap);
                }
            }

            //--------------------------------------------------
            // stem – outline only, parametric:
            // wide base -> 15% neck at 50% width ->
            // smooth right-leaning taper to 1/4 width
            //--------------------------------------------------
            {
            const wxColour stemOutlineCol{ TintIfUsingOpacity(wxColour{ 90, 150, 90 }) };
            const wxPen stemPen{ stemOutlineCol, outlinePenWidth };
            gc->SetPen(stemPen);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);

            // --- geometry parameters ---
            const double stemBaseWidth = bodyRect.GetWidth() * 0.22; // wide base
            const double stemHeight = bodyRect.GetHeight() * 0.23;

            const double baseY =
                topY + ((bottomY - topY) * math_constants::fifth); // nestled into lobes
            const double baseCenterX = cx;                         // centered at the pumpkin

            // how far the centerline leans right at the tip
            const double maxCenterOffset = bodyRect.GetWidth() * 0.16;

            // widthFactor(t): how the width changes from base (t=0) to tip (t=1)
            // Adjust these constants to reshape the stem
            constexpr double NECK_T = 0.15; // where the "neck" happens (0..1 of height)
            constexpr double BASE_WIDTH = math_constants::whole; // width at base
            constexpr double NECK_WIDTH = math_constants::half;  // width at the neck
            constexpr double TIP_WIDTH = 0.40;                   // width at the top

            const auto widthFactor = [&](double t) noexcept
            {
                if (t <= NECK_T)
                    {
                    // smoothly transition from 1.0 -> neckWidth
                    const double u = t / NECK_T;
                    return BASE_WIDTH + ((NECK_WIDTH - BASE_WIDTH) * u);
                    }

                // smoothly transition from neckWidth -> tipWidth
                const double u = (t - NECK_T) / (1.0 - NECK_T);
                return NECK_WIDTH + ((TIP_WIDTH - NECK_WIDTH) * u);
            };

            // centerline: mostly vertical then leaning right.
            // t=0: base, t=1: top
            const auto centerX = [&](const double t) noexcept
            {
                // quadratic in t so the bend gets stronger toward the top
                return baseCenterX + (maxCenterOffset * (t * t));
            };
            const auto centerY = [&](const double t) noexcept { return baseY - (stemHeight * t); };

            // sample points along left and right edges
            constexpr int STEPS{ 24 };
            std::array<wxPoint2DDouble, STEPS + 1> leftPts;
            std::array<wxPoint2DDouble, STEPS + 1> rightPts;

            for (int i = 0; i <= STEPS; ++i)
                {
                const double t = static_cast<double>(i) / STEPS;

                const double wFactor = widthFactor(t);
                const double w = stemBaseWidth * wFactor;
                const double halfW = w / 2.0;

                const double cxStem = centerX(t);
                const double cyStem = centerY(t);

                leftPts[i] = wxPoint2DDouble{ cxStem - halfW, cyStem };
                rightPts[i] = wxPoint2DDouble{ cxStem + halfW, cyStem };
                }

            wxGraphicsPath stemPath = gc->CreatePath();

            // start at base left
            stemPath.MoveToPoint(leftPts[0].m_x, leftPts[0].m_y);

            // left edge: go up from base to tip
            for (int i = 1; i <= STEPS; ++i)
                {
                stemPath.AddLineToPoint(leftPts[i].m_x, leftPts[i].m_y);
                }

                // top edge: slight convex join from left tip to right tip
                {
                const auto& lTip = leftPts[STEPS];
                const auto& rTip = rightPts[STEPS];
                const double middleX = (lTip.m_x + rTip.m_x) / 2.0;
                const double middleY = ((lTip.m_y + rTip.m_y) / 2.0) - (stemHeight * 0.05);

                stemPath.AddCurveToPoint(middleX, middleY, middleX, middleY, rTip.m_x, rTip.m_y);
                }

            // right edge: go back down from tip to base
            for (int i = STEPS - 1; i >= 0; --i)
                {
                stemPath.AddLineToPoint(rightPts[i].m_x, rightPts[i].m_y);
                }

                // base: gentle bulge across the bottom back to left
                {
                const double bulgeY = baseY + (stemHeight * 0.03);
                stemPath.AddCurveToPoint(baseCenterX + (stemBaseWidth * math_constants::tenth),
                                         bulgeY,
                                         baseCenterX - (stemBaseWidth * math_constants::tenth),
                                         bulgeY, leftPts[0].m_x, leftPts[0].m_y);
                }

            stemPath.CloseSubpath();

            //--------------------------------------------------
            // Fill the stem with base color
            //--------------------------------------------------
            const wxColour stemBaseCol{ TintIfUsingOpacity(
                wxColour{ Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#526C45") }) };
            const wxColour stemShadowCol{ TintIfUsingOpacity(
                wxColour{ Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#1C3D1C") }) };

            gc->SetBrush(wxBrush(stemBaseCol));
            gc->FillPath(stemPath);

                //--------------------------------------------------
                // Right-side shadow down the full length
                //--------------------------------------------------
                {
                // how far in from the right edge the shadow fades (0..1 from right→left)
                constexpr double INNER_LERP = math_constants::half; // halfway toward center

                wxGraphicsPath shadowPath = gc->CreatePath();

                // start at base outer-right
                shadowPath.MoveToPoint(rightPts[0].m_x, rightPts[0].m_y);

                // go up along the outer-right edge
                for (int i = 1; i <= STEPS; ++i)
                    {
                    shadowPath.AddLineToPoint(rightPts[i].m_x, rightPts[i].m_y);
                    }

                // track gradient bounds while we build the inner edge
                double outerMaxX = rightPts[0].m_x;
                double innerMinX = rightPts[0].m_x;

                // come back down along an inner edge (toward the center)
                for (int i = STEPS; i >= 0; --i)
                    {
                    const double innerX =
                        rightPts[i].m_x + ((leftPts[i].m_x - rightPts[i].m_x) * INNER_LERP);
                    const double innerY =
                        rightPts[i].m_y + ((leftPts[i].m_y - rightPts[i].m_y) * INNER_LERP);

                    shadowPath.AddLineToPoint(innerX, innerY);

                    outerMaxX = std::max(outerMaxX, rightPts[i].m_x);
                    innerMinX = std::min(innerMinX, innerX);
                    }

                shadowPath.CloseSubpath();

                // gradient: outer edge (right) → inner edge (toward center)
                wxDouble bx{ 0.0 }, by{ 0.0 }, bw{ 0.0 }, bh{ 0.0 };
                shadowPath.GetBox(&bx, &by, &bw, &bh);

                wxGraphicsGradientStops stops;
                stops.Add(stemShadowCol, 0.0); // outer edge = dark
                stops.Add(TintIfUsingOpacity(wxColour{ stemShadowCol.Red(), stemShadowCol.Green(),
                                                       stemShadowCol.Blue() }),
                          1.0);

                // use our computed outer/inner x so gradient spans entire strip width
                const wxGraphicsBrush shadowBrush =
                    gc->CreateLinearGradientBrush(outerMaxX, by, // start at outer-right
                                                  innerMinX, by, // end toward center
                                                  stops);

                gc->SetBrush(shadowBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(shadowPath);
                }

            //--------------------------------------------------
            // re-stroke stem outline with base outline color
            //--------------------------------------------------
            const wxPen stemOutlinePen{ stemBaseCol.ChangeLightness(80), outlinePenWidth };
            gc->SetPen(stemOutlinePen);
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->StrokePath(stemPath);
            }

        //--------------------------------------------------
        // main lobes with top→bottom orange gradient
        // (front lobes sink further down now)
        //--------------------------------------------------
        gc->SetPen(lobePen);

        for (const int idx : DRAW_ORDER)
            {
            const auto& lobe = LOBES[idx];

            const double centerX = leftX + (bodyRect.GetWidth() * lobe.m_centerPercent);
            const double foldX = leftX + (bodyRect.GetWidth() * lobe.m_foldPercent);

            const double halfWidth = (bodyRect.GetWidth() / 7.0) * lobe.m_widthFactor;
            const double halfHeight = halfHeightBase * lobe.m_heightFactor;

            // 1 at center, ~0 at outer lobes
            const double centerOffset = 1.0 - (std::abs(lobe.m_centerPercent - 0.50) * 2.0);

            // sink tops more, especially for central lobes
            const double sinkAmount =
                halfHeight * (math_constants::fifth + (math_constants::quarter * centerOffset));

            // raise the bottoms of central lobes a bit to flatten the base
            const double flattenAmount = halfHeight * 0.08 * centerOffset;

            const double topYLocal = midY - halfHeight + sinkAmount;
            const double bottomYLocal = midY + halfHeight - flattenAmount;

            const double leftMidX = centerX - halfWidth;
            const double rightMidX = centerX + halfWidth;

            wxGraphicsPath path = gc->CreatePath();

            // start at top (folded toward center)
            path.MoveToPoint(foldX, topYLocal);

            // top -> right side (curved, bowing out)
            path.AddCurveToPoint(
                wxPoint(foldX + ((rightMidX - foldX) * 0.6), topYLocal - (halfHeight * 0.22)),
                wxPoint(rightMidX + (halfWidth * math_constants::fifth),
                        midY - (halfHeight * 0.12)),
                wxPoint(rightMidX, midY));

            // right side -> bottom (curved)
            path.AddCurveToPoint(
                wxPoint(rightMidX + (halfWidth * 0.18), midY + (halfHeight * 0.40)),
                wxPoint(foldX + ((rightMidX - foldX) * 0.7),
                        bottomYLocal + (bottomBulge * math_constants::half)),
                wxPoint(foldX, bottomYLocal));

            // bottom -> left side (curved back under neighbor)
            path.AddCurveToPoint(wxPoint(foldX - ((foldX - leftMidX) * 0.7),
                                         bottomYLocal + (bottomBulge * math_constants::half)),
                                 wxPoint(leftMidX - (halfWidth * 0.18), midY + (halfHeight * 0.40)),
                                 wxPoint(leftMidX, midY));

            // left side -> top (curved)
            path.AddCurveToPoint(
                wxPoint(leftMidX - (halfWidth * math_constants::fifth), midY - (halfHeight * 0.12)),
                wxPoint(foldX - ((foldX - leftMidX) * 0.6), topYLocal - (halfHeight * 0.22)),
                wxPoint(foldX, topYLocal));

            path.CloseSubpath();

            const wxGraphicsBrush brush = gc->CreateLinearGradientBrush(
                centerX, topYLocal, centerX, bottomYLocal, topOrange, bottomOrange);
            gc->SetBrush(brush);

            gc->FillPath(path);
            gc->StrokePath(path);
            }

            //--------------------------------------------------
            // center crease
            //--------------------------------------------------
            {
            const double centerXLine = leftX + (bodyRect.GetWidth() * math_constants::half);

            const double creaseTopY =
                topY +
                ((bottomY - topY) * math_constants::fifth); // line starts where front lobes sink to
            const double creaseBottomY = bottomY - ((bottomY - topY) * 0.05);

            wxGraphicsPath crease = gc->CreatePath();

            crease.MoveToPoint(centerXLine, creaseTopY);
            crease.AddCurveToPoint(wxPoint(centerXLine + (bodyRect.GetWidth() * 0.02),
                                           midY - (halfHeightBase * math_constants::tenth)),
                                   wxPoint(centerXLine - (bodyRect.GetWidth() * 0.02),
                                           midY + (halfHeightBase * math_constants::fifth)),
                                   wxPoint(centerXLine, creaseBottomY));

            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen{ TintIfUsingOpacity(wxColour{ 220, 110, 0, 200 }), outlinePenWidth });
            gc->StrokePath(crease);
            }
        }
    } // namespace Wisteria::GraphItems
