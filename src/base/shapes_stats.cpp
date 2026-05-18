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
    void ShapeRenderer::DrawNumberRange(wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };

        rect.Deflate(ScaleToScreenAndCanvas(2));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        //--------------------------------------
        // Parse "left:right[:bottom]"
        //--------------------------------------
        const wxArrayString parts = wxSplit(GetGraphItemInfo().GetText(), ':');
        if (parts.size() < 2)
            {
            return;
            }

        const wxString& leftText = parts[0];
        const wxString& rightText = parts[1];

        const bool hasBottomLine = (parts.size() >= 3);
        wxString bottomText;
        if (hasBottomLine)
            {
            bottomText = parts[2];
            }

        //--------------------------------------
        // Split rect into top/bottom halves if needed
        //--------------------------------------
        wxRect topLineRect{ rect };
        wxRect bottomLineRect;

        if (hasBottomLine)
            {
            topLineRect.SetHeight(rect.GetHeight() / 2);
            bottomLineRect = wxRect{ rect.GetX(), rect.GetY() + (rect.GetHeight() / 2),
                                     rect.GetWidth(), rect.GetHeight() / 2 };
            }

        //--------------------------------------
        // Helper: Draw the top line (labels + dots + bar)
        //--------------------------------------
        auto drawTopLine = [&](const wxRect& r)
        {
            //--------------------------------------
            // Scaled stroke & dots
            //--------------------------------------
            const double rawStroke = r.GetHeight() * 0.03;
            const int penWidth =
                std::max<int>(1, ScaleToScreenAndCanvas(static_cast<int>(rawStroke)));

            const wxPen pen = GetGraphItemInfo().GetPen().IsOk() ?
                                  wxPen{ GetGraphItemInfo().GetPen().GetColour(), penWidth,
                                         GetGraphItemInfo().GetPen().GetStyle() } :
                                  wxPen{ *wxBLACK, penWidth };

            gc->SetPen(pen);
            gc->SetBrush(wxBrush{ pen.GetColour() });

            //--------------------------------------
            // Layout
            //--------------------------------------
            const double cy = r.GetY() + (r.GetHeight() * math_constants::half);

            // Dot radius (smaller)
            const double dotRadius = r.GetHeight() * (0.075 / 3);

            // Left/right label regions
            const double leftRegionWidth = r.GetWidth() * math_constants::fifth;
            const double rightRegionWidth = r.GetWidth() * math_constants::fifth;

            // Padding between labels and dots
            const double dotPadding = r.GetWidth() * 0.04;

            // Bar endpoints (account for dot radius + padding)
            const double x1 = r.GetX() + leftRegionWidth + dotRadius + dotPadding;
            const double x2 = r.GetRight() - rightRegionWidth - dotRadius - dotPadding;

            //--------------------------------------
            // Draw bar + dots
            //--------------------------------------
            gc->StrokeLine(x1, cy, x2, cy);
            gc->DrawEllipse(x1 - dotRadius, cy - dotRadius, dotRadius * 2, dotRadius * 2);
            gc->DrawEllipse(x2 - dotRadius, cy - dotRadius, dotRadius * 2, dotRadius * 2);

            //--------------------------------------
            // Label bounding box geometry
            //--------------------------------------
            const int labelWidth = static_cast<int>(leftRegionWidth * 0.85);
            const int labelHeight = static_cast<int>(r.GetHeight() * 0.70);

            //--------------------------------------
            // Left label
            //--------------------------------------
            const wxPoint leftLabelCenter(
                static_cast<int>(r.GetX() + (leftRegionWidth * math_constants::half)),
                static_cast<int>(cy));
            Label leftLabel{ GraphItemInfo{ leftText }
                                 .Pen(wxNullPen)
                                 .FontColor(GetGraphItemInfo().GetFontColor().IsOk() ?
                                                GetGraphItemInfo().GetFontColor() :
                                                *wxBLACK)
                                 .Font(GetGraphItemInfo().GetFont().MakeBold())
                                 .LabelAlignment(TextAlignment::Centered)
                                 .DPIScaling(GetDPIScaleFactor())
                                 .Anchoring(Anchoring::Center)
                                 .AnchorPoint(leftLabelCenter)
                                 .LabelPageVerticalAlignment(PageVerticalAlignment::Centered) };
            leftLabel.SetBoundingBox(wxRect(leftLabelCenter.x - (labelWidth / 2),
                                            leftLabelCenter.y - (labelHeight / 2), labelWidth,
                                            labelHeight),
                                     dc, GetScaling());

            //--------------------------------------
            // Right label
            //--------------------------------------
            const wxPoint rightLabelCenter(
                static_cast<int>((r.GetRight() - rightRegionWidth) +
                                 (rightRegionWidth * math_constants::half)),
                static_cast<int>(cy));
            Label rightLabel{ GraphItemInfo{ rightText }
                                  .Pen(wxNullPen)
                                  .FontColor(GetGraphItemInfo().GetFontColor().IsOk() ?
                                                 GetGraphItemInfo().GetFontColor() :
                                                 *wxBLACK)
                                  .Font(GetGraphItemInfo().GetFont().MakeBold())
                                  .LabelAlignment(TextAlignment::Centered)
                                  .DPIScaling(GetDPIScaleFactor())
                                  .Anchoring(Anchoring::Center)
                                  .AnchorPoint(rightLabelCenter)
                                  .LabelPageVerticalAlignment(PageVerticalAlignment::Centered) };
            rightLabel.SetBoundingBox(wxRect(rightLabelCenter.x - (labelWidth / 2),
                                             rightLabelCenter.y - (labelHeight / 2), labelWidth,
                                             labelHeight),
                                      dc, GetScaling());

            // ensure both labels have the same size (scaling)
            const double labelScaling{ std::min(leftLabel.GetScaling(), rightLabel.GetScaling()) };
            leftLabel.SetScaling(labelScaling);
            rightLabel.SetScaling(labelScaling);
            leftLabel.Draw(dc);
            rightLabel.Draw(dc);

            return labelScaling;
        };

        //--------------------------------------
        // Draw top line
        //--------------------------------------
        const auto labelScaling = drawTopLine(topLineRect);

        //--------------------------------------
        // Draw bottom line (optional)
        //--------------------------------------
        if (hasBottomLine)
            {
            const double cy =
                bottomLineRect.GetY() + (bottomLineRect.GetHeight() * math_constants::half);

            const wxPoint anchor(
                bottomLineRect.GetX() + (bottomLineRect.GetWidth() * math_constants::half), cy);

            Label bottomLabel{ GraphItemInfo{ bottomText }
                                   .Pen(wxNullPen)
                                   .LabelAlignment(TextAlignment::Centered)
                                   .DPIScaling(GetDPIScaleFactor())
                                   .Anchoring(Anchoring::Center)
                                   .AnchorPoint(anchor)
                                   .LabelPageVerticalAlignment(PageVerticalAlignment::Centered) };

            bottomLabel.SetBoundingBox(bottomLineRect, dc, GetScaling());
            bottomLabel.SetScaling(std::min(bottomLabel.GetScaling(), labelScaling));
            bottomLabel.Draw(dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBoxPlot(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen().IsOk() ?
                              GetGraphItemInfo().GetPen() :
                              wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        wxRect drawRect{ rect };

        // whisker
        dc.DrawLine(wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2), drawRect.GetTop()),
                    wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2), drawRect.GetBottom()));
        dc.DrawLine(
            wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2) - (drawRect.GetWidth() / 4),
                    drawRect.GetTop()),
            wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2) + (drawRect.GetWidth() / 4),
                    drawRect.GetTop()));
        dc.DrawLine(
            wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2) - (drawRect.GetWidth() / 4),
                    drawRect.GetBottom()),
            wxPoint(drawRect.GetLeft() + (drawRect.GetWidth() / 2) + (drawRect.GetWidth() / 4),
                    drawRect.GetBottom()));
        drawRect.y += (drawRect.GetHeight() / 2) - (drawRect.GetHeight() / 4); // center
        drawRect.SetHeight(drawRect.GetHeight() / 2);
        DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawRectangle(drawRect); });
        // median line
        dc.DrawLine(wxPoint(drawRect.GetLeft(), drawRect.GetTop() + (drawRect.GetHeight() / 2)),
                    wxPoint(drawRect.GetRight(), drawRect.GetTop() + (drawRect.GetHeight() / 2)));
        }
    } // namespace Wisteria::GraphItems
