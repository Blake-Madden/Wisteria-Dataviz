///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "shapes.h"
#include "label.h"
#include "image.h"

using namespace Wisteria::Colors;

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    void Shapes::DrawCircularSign(const wxRect rect, wxDC& dc)
        {
        const auto radius =
            safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 2);
        wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(1)));
        wxDCBrushChanger bc(dc, m_graphInfo.GetBrush());

        const auto circleCenter = rect.GetLeftTop() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        dc.DrawCircle(circleCenter, radius);

        // lettering on the sign
        Label theLabel(GraphItemInfo(m_graphInfo.GetText()).Pen(wxNullPen).
            AnchorPoint(circleCenter).Anchoring(Anchoring::Center).
            LabelAlignment(TextAlignment::Centered).
            DPIScaling(GetDPIScaleFactor()));
        theLabel.SetFontColor(*wxWHITE);
        wxPoint theLabelCorner{ circleCenter };
        auto rectWithinCircleWidth =
            geometry::radius_to_inner_rect_width(radius);
        theLabelCorner.x -= rectWithinCircleWidth / 2;
        theLabelCorner.y -= rectWithinCircleWidth / 2;
        theLabel.SetBoundingBox(
            wxRect(theLabelCorner,
                wxSize(rectWithinCircleWidth, rectWithinCircleWidth)),
            dc, GetScaling());
        theLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        theLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        theLabel.Draw(dc);
        }

    //---------------------------------------------------
    void Shapes::DrawSun(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        GraphItems::Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        const auto centerPt = wxPoint(0, 0) +
            wxSize(memDC.GetSize().GetWidth() / 2, memDC.GetSize().GetHeight() / 2);

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for sun icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::Sunflower),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::Sunflower));
            // a line going from the middle of the left side to the middle of the right
            const std::array<wxPoint2DDouble, 2> points =
                {
                wxPoint(0, memDC.GetSize().GetHeight() / 2),
                wxPoint(memDC.GetSize().GetWidth(), memDC.GetSize().GetHeight() / 2)
                };
            // save current transform matrix state
            auto gm = gc->GetTransform();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the sun beams, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle = 0.0;
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over to
                // from the translated origin
                gc->StrokeLine(points[0].m_x - centerPt.x, points[0].m_y - centerPt.y,
                               points[1].m_x - centerPt.x, points[1].m_y - centerPt.y);
                angle += 45;
                }
            // restore transform matrix
            gc->SetTransform(gm);
            // draw the sun
            const wxRect sunRect = wxRect(memDC.GetSize()).Deflate(memDC.GetSize().GetWidth()/4);
            gc->DrawEllipse(sunRect.GetTopLeft().x, sunRect.GetTopLeft().y,
                            sunRect.GetWidth(), sunRect.GetHeight());

            wxDELETE(gc);
            }

        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(bmp, rect.GetTopLeft(), true);
        }

    //---------------------------------------------------
    void Shapes::DrawFlower(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        GraphItems::Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        const auto centerPt = wxPoint(0, 0) +
            wxSize(memDC.GetSize().GetWidth() / 2, memDC.GetSize().GetHeight() / 2);

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for flower icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorContrast::Shade(ColorBrewer::GetColor(Color::Wisteria)),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::Wisteria));
            // a line going from the middle of the left side to the middle of the right
            wxRect petalRect(
                wxPoint(memDC.GetSize().GetWidth()/2, memDC.GetSize().GetHeight()/2),
                wxSize(memDC.GetSize().GetWidth()/2, memDC.GetSize().GetHeight()/6));
            petalRect.Offset(wxPoint(0, petalRect.GetHeight() / 2));
            // save current transform matrix state
            auto gm = gc->GetTransform();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the sun beams, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle = 0.0;
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over to
                // from the translated origin
                gc->DrawEllipse(petalRect.GetTopLeft().x - centerPt.x,
                                petalRect.GetTopLeft().y - centerPt.y,
                                petalRect.GetWidth(), petalRect.GetHeight());
                angle += 45;
                }
            // restore transform matrix
            gc->SetTransform(gm);
            // draw the middle of flower
            gc->SetBrush(ColorBrewer::GetColor(Color::BabyBlue));
            const wxRect flowerRect = wxRect(memDC.GetSize()).Deflate(memDC.GetSize().GetWidth()/4);
            gc->DrawEllipse(flowerRect.GetTopLeft().x, flowerRect.GetTopLeft().y,
                            flowerRect.GetWidth(), flowerRect.GetHeight());

            wxDELETE(gc);
            }

        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(bmp, rect.GetTopLeft(), true);
        }

    //---------------------------------------------------
    void Shapes::DrawFallLeaf(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        GraphItems::Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        const wxRect dcRect(wxPoint(0, 0), memDC.GetSize());

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for maple leaf icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::LightBrown),
                       ScaleToScreenAndCanvas(1)));

            // draw the stem
            auto stemPath = gc->CreatePath();
            // start of middle of bottom
            stemPath.MoveToPoint(GetXPosFromLeft(dcRect, .5), dcRect.GetBottom());
            // draw to the top middle
            stemPath.AddLineToPoint(GetXPosFromLeft(dcRect, .5), dcRect.GetTop());
            gc->StrokePath(stemPath);

            // draw the leaf
            gc->SetPen(wxPen(ColorContrast::Shade(ColorBrewer::GetColor(Color::ChineseRed)),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::ChineseRed));
            auto leafPath = gc->CreatePath();
            // left side of leaf
            leafPath.MoveToPoint(GetXPosFromLeft(dcRect, .5), GetYPosFromTop(dcRect, .75));
            leafPath.AddQuadCurveToPoint(
                GetXPosFromLeft(dcRect, 0), GetYPosFromTop(dcRect, .6),
                // top
                GetXPosFromLeft(dcRect, .5), GetYPosFromTop(dcRect, 0));

            // right side
            leafPath.AddQuadCurveToPoint(
                GetXPosFromLeft(dcRect, 1), GetYPosFromTop(dcRect, .6),
                // top
                GetXPosFromLeft(dcRect, .5), GetYPosFromTop(dcRect, .75));
            leafPath.CloseSubpath();
            gc->FillPath(leafPath);
            gc->StrokePath(leafPath);

            wxDELETE(gc);
            }

        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(bmp, rect.GetTopLeft(), true);
        }
    }
