///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_people.cpp
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
    void ShapeRenderer::DrawMan(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect dcRect{ rect };
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ dcRect.GetWidth() * 0.6 };
            const auto adjustLeft{ (dcRect.GetWidth() - adjustedWidth) * math_constants::half };
            dcRect.SetWidth(adjustedWidth);
            dcRect.Offset(wxPoint(adjustLeft, 0));
            }

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for male outline!");
        if (gc != nullptr)
            {
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);
                }

            gc->SetBrush(GetGraphItemInfo().GetBrush());

            auto outlinePath = gc->CreatePath();
            // draw the head
            wxRect headRect{ dcRect };
            headRect.SetHeight(headRect.GetHeight() * 0.15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ dcRect };
            const auto neckHeight{ (dcRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(
                wxPoint(GetXPosFromLeft(bodyRect, math_constants::half), bodyRect.GetTop()));

            constexpr auto COLLAR_WIDTH{ 0.3 };
            constexpr auto SHOULDER_WIDTH{ math_constants::tenth };
            constexpr auto SHOULDER_HEIGHT{ math_constants::tenth };
            constexpr auto ARM_LENGTH{ 0.3 };
            constexpr auto ARM_WIDTH{ 0.15 };
            constexpr auto ARMPIT_WIDTH{ 0.05 };
            constexpr auto CROTCH_WIDTH{ 0.05 };
            constexpr auto SIDE_LENGTH{ 0.9 };
            constexpr auto LENGTH_BETWEEN_ARM_AND_LEGS{ 0.05 };
            constexpr auto LEG_WIDTH{ 0.175 };
            constexpr auto Y_CONTROL_POINT_OFFSET{ 0.05 };
            // left collar and shoulder
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - COLLAR_WIDTH),
                GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left arm (left side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              (ARM_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect,
                                (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) + ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // inside left arm
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect,
                                (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) + ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left armpit
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left side, down to left foot
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SIDE_LENGTH));
            // left foot
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH +
                                              (LEG_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect, SIDE_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH + LEG_WIDTH),
                GetYPosFromTop(bodyRect, SIDE_LENGTH));
            // inside left leg
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH + LEG_WIDTH),
                GetYPosFromTop(bodyRect,
                               SHOULDER_HEIGHT + ARM_LENGTH + LENGTH_BETWEEN_ARM_AND_LEGS));
            // left half of crotch
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH +
                                                                    LENGTH_BETWEEN_ARM_AND_LEGS));

            // right half of crotch
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect,
                                math_constants::half + (CROTCH_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect,
                               SHOULDER_HEIGHT + ARM_LENGTH + LENGTH_BETWEEN_ARM_AND_LEGS));
            // inside right leg
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect,
                                math_constants::half + (CROTCH_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect, SIDE_LENGTH));
            // right foot
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) +
                                              (LEG_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect, SIDE_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH),
                GetYPosFromTop(bodyRect, SIDE_LENGTH));
            // right side, up to armpit
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // right armpit
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH +
                                              ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // inside right arm
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH +
                                              ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH +
                                              ARMPIT_WIDTH + (ARM_WIDTH * math_constants::half)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH +
                                              ARMPIT_WIDTH + ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // right arm (right side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half +
                                              (CROTCH_WIDTH * math_constants::half) + LEG_WIDTH +
                                              ARMPIT_WIDTH + ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // right shoulder and collar
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH)),
                GetYPosFromTop(bodyRect, 0));
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, 0));

            gc->FillPath(outlinePath);
            gc->StrokePath(outlinePath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawWoman(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ drawRect.GetWidth() * 0.6 };
            const auto adjustLeft{ (drawRect.GetWidth() - adjustedWidth) * math_constants::half };
            drawRect.SetWidth(adjustedWidth);
            drawRect.Offset(wxPoint(adjustLeft, 0));
            }

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for female outline!");
        if (gc != nullptr)
            {
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);
                }

            gc->SetBrush(GetGraphItemInfo().GetBrush());

            auto outlinePath = gc->CreatePath();
            // draw the head
            wxRect headRect{ drawRect };
            headRect.SetHeight(headRect.GetHeight() * 0.15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ drawRect };
            const auto neckHeight{ (drawRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(
                wxPoint(GetXPosFromLeft(bodyRect, math_constants::half), bodyRect.GetTop()));

            constexpr auto COLLAR_WIDTH{ math_constants::quarter };
            constexpr auto COLLAR_SHORT_WIDTH{ 0.15 };
            constexpr auto SHOULDER_WIDTH{ math_constants::tenth };
            constexpr auto SHOULDER_HEIGHT{ math_constants::tenth };
            constexpr auto ARM_LENGTH{ math_constants::quarter };
            constexpr auto ARM_SHORT_LENGTH{ 0.225 };
            constexpr auto ARM_WIDTH{ math_constants::tenth };
            constexpr auto ARMPIT_WIDTH{ 0.05 };
            constexpr auto WAIST_WIDTH{ 0.125 };
            constexpr auto THORAX_HEIGHT{ math_constants::fifth };
            constexpr auto LEG_WIDTH{ 0.125 };
            constexpr auto DRESS_WIDTH{ 0.3 };
            constexpr auto DRESS_BOTTOM{ 0.675 };
            constexpr auto ANKLE_WIDTH{ 0.075 };
            constexpr auto Y_CONTROL_POINT_OFFSET{ 0.05 };
            constexpr auto X_CONTROL_POINT_RIGHT_SHOULDER_OFFSET{ 0.125 };
            constexpr auto X_CONTROL_POINT_LEFT_SHOULDER_OFFSET{ SHOULDER_WIDTH *
                                                                 math_constants::quarter };
            // left collar and shoulder
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - COLLAR_SHORT_WIDTH),
                GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH -
                                           X_CONTROL_POINT_LEFT_SHOULDER_OFFSET)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left arm (left side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, 0),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_SHORT_LENGTH));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (ARM_WIDTH * math_constants::quarter)),
                GetYPosFromTop(bodyRect,
                               SHOULDER_HEIGHT + ARM_SHORT_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // inside left arm
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left armpit to waist
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - WAIST_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT));
            // left waist to bottom of dress
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - DRESS_WIDTH),
                GetYPosFromTop(bodyRect, DRESS_BOTTOM));
            // dress bottom to leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - LEG_WIDTH),
                                       GetYPosFromTop(bodyRect, DRESS_BOTTOM));
            // left leg to ankle
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - ANKLE_WIDTH),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle to middle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, 0.9));

            // right side
            //-----------
            // ankle
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + ANKLE_WIDTH),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle up right leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + LEG_WIDTH),
                                       GetYPosFromTop(bodyRect, DRESS_BOTTOM));
            // dress bottom
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + DRESS_WIDTH),
                GetYPosFromTop(bodyRect, DRESS_BOTTOM));
            // bottom of dress to right waist
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + WAIST_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT));
            // waist to right armpit
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH) -
                                              ARM_WIDTH - ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // inside right arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::full - ARM_WIDTH),
                                       GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                                math_constants::full - (ARM_WIDTH * math_constants::quarter)),
                GetYPosFromTop(bodyRect,
                               SHOULDER_HEIGHT + ARM_SHORT_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, math_constants::full),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_SHORT_LENGTH));
            // right arm (right side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // shoulder and right collar
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_SHORT_WIDTH +
                                              X_CONTROL_POINT_RIGHT_SHOULDER_OFFSET),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_SHORT_WIDTH),
                GetYPosFromTop(bodyRect, 0));
            // collar
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, 0));

            gc->FillPath(outlinePath);
            gc->StrokePath(outlinePath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBusinessWoman(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        wxRect drawRect{ rect };
        drawRect.Deflate(GetGraphItemInfo().GetPen().IsOk() ?
                             ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                             0);
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ drawRect.GetWidth() * 0.6 };
            const auto adjustLeft{ (drawRect.GetWidth() - adjustedWidth) * math_constants::half };
            drawRect.SetWidth(adjustedWidth);
            drawRect.Offset(wxPoint(adjustLeft, 0));
            }

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for female outline!");
        if (gc != nullptr)
            {
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);
                }

            gc->SetBrush(GetGraphItemInfo().GetBrush());

            auto outlinePath = gc->CreatePath();
            // draw the head
            wxRect headRect{ drawRect };
            headRect.SetHeight(headRect.GetHeight() * 0.15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ drawRect };
            const auto neckHeight{ (drawRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(
                wxPoint(GetXPosFromLeft(bodyRect, math_constants::half), bodyRect.GetTop()));

            constexpr auto COLLAR_WIDTH{ math_constants::quarter };
            constexpr auto COLLAR_SHORT_WIDTH{ 0.15 };
            constexpr auto SHOULDER_WIDTH{ 0.06 };
            constexpr auto SHOULDER_HEIGHT{ math_constants::tenth };
            constexpr auto ARM_LENGTH{ math_constants::quarter };
            constexpr auto ARM_WIDTH{ 0.06 };
            constexpr auto ARMPIT_WIDTH{ 0.05 };
            constexpr auto WAIST_WIDTH{ 0.125 };
            constexpr auto THORAX_HEIGHT{ math_constants::fifth };
            constexpr auto LEG_WIDTH{ 0.125 };
            constexpr auto SKIRT_WIDTH{ LEG_WIDTH + 0.05 };
            constexpr auto HIP_WIDTH{ SKIRT_WIDTH * 1.6 };
            constexpr auto SKIRT_BOTTOM{ 0.675 };
            constexpr auto ANKLE_WIDTH{ 0.075 };
            constexpr auto Y_CONTROL_POINT_OFFSET{ 0.025 };
            constexpr auto X_CONTROL_POINT_RIGHT_SHOULDER_OFFSET{ 0.15 };
            constexpr auto X_CONTROL_POINT_LEFT_SHOULDER_OFFSET{ SHOULDER_WIDTH };
            // left collar and shoulder
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - COLLAR_SHORT_WIDTH),
                GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH -
                                           X_CONTROL_POINT_LEFT_SHOULDER_OFFSET)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left arm (left side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              (ARM_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // inside left arm
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half - COLLAR_WIDTH - SHOULDER_WIDTH) +
                                              ARM_WIDTH + ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // left armpit to waist
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - WAIST_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT));
            // left waist to bottom of dress
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - HIP_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT +
                                             ((SKIRT_BOTTOM - (SHOULDER_HEIGHT + THORAX_HEIGHT)) *
                                              math_constants::quarter)),
                GetXPosFromLeft(bodyRect, math_constants::half - SKIRT_WIDTH),
                GetYPosFromTop(bodyRect, SKIRT_BOTTOM));
            // dress bottom to leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - LEG_WIDTH),
                                       GetYPosFromTop(bodyRect, SKIRT_BOTTOM));
            // left leg to ankle
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half - ANKLE_WIDTH),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle to middle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, 0.9));

            // right side
            //-----------
            // ankle
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + ANKLE_WIDTH),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle up right leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + LEG_WIDTH),
                                       GetYPosFromTop(bodyRect, SKIRT_BOTTOM));
            // dress bottom
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + SKIRT_WIDTH),
                GetYPosFromTop(bodyRect, SKIRT_BOTTOM));
            // bottom of dress to right waist
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + HIP_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT +
                                             ((SKIRT_BOTTOM - (SHOULDER_HEIGHT + THORAX_HEIGHT)) *
                                              math_constants::quarter)),
                GetXPosFromLeft(bodyRect, math_constants::half + WAIST_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + THORAX_HEIGHT));
            // waist to right armpit
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH) -
                                              ARM_WIDTH - ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // inside right arm
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH) -
                                              ARM_WIDTH - ARMPIT_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                                (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH) - ARM_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH + Y_CONTROL_POINT_OFFSET),
                GetXPosFromLeft(bodyRect, (math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH)),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT + ARM_LENGTH));
            // right arm (right side)
            outlinePath.AddLineToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_WIDTH + SHOULDER_WIDTH),
                GetYPosFromTop(bodyRect, SHOULDER_HEIGHT));
            // shoulder and right collar
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_SHORT_WIDTH +
                                              X_CONTROL_POINT_RIGHT_SHOULDER_OFFSET),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect, math_constants::half + COLLAR_SHORT_WIDTH),
                GetYPosFromTop(bodyRect, 0));
            // collar
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, 0));

            gc->FillPath(outlinePath);
            gc->StrokePath(outlinePath);
            }
        }
    } // namespace Wisteria::GraphItems
