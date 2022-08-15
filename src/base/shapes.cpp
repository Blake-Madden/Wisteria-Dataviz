///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "Shapes.h"
#include "label.h"
#include "image.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Icons;

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    void Shape::SetBoundingBox(const wxRect& rect,
        [[maybe_unused]] wxDC& dc,
        [[maybe_unused]] const double parentScaling)
        {
        m_sizeDIPs.x = (IsFittingContentWidthToCanvas() ?
            std::min<int>(m_shapeSizeDIPs.GetWidth(),
                          DownscaleFromScreenAndCanvas(rect.GetSize().GetWidth())) :
            DownscaleFromScreenAndCanvas(rect.GetSize().GetWidth()) );
        m_sizeDIPs.y = DownscaleFromScreenAndCanvas(rect.GetSize().GetHeight());

        if (GetAnchoring() == Anchoring::TopLeftCorner)
            { SetAnchorPoint(rect.GetTopLeft()); }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            { SetAnchorPoint(rect.GetBottomLeft()); }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            { SetAnchorPoint(rect.GetTopRight()); }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            { SetAnchorPoint(rect.GetBottomRight()); }
        else if (GetAnchoring() == Anchoring::Center)
            {
            wxPoint pt = rect.GetTopLeft();
            pt += wxPoint(rect.GetWidth() / 2, rect.GetHeight() / 2);
            SetAnchorPoint(pt);
            }
        }

    //---------------------------------------------------
    wxRect Shape::Draw(wxDC& dc) const
        {
        auto bBox = GetBoundingBox(dc);
        auto drawRect = wxRect(ScaleToScreenAndCanvas(m_shapeSizeDIPs));
        // keep drawing area inside of the full area
        drawRect.SetWidth(std::min(drawRect.GetWidth(), bBox.GetWidth()));
        drawRect.SetHeight(std::min(drawRect.GetHeight(), bBox.GetHeight()));

        // position the shape inside of its (possibly) larger box
        wxPoint shapeTopLeftCorner(GetBoundingBox(dc).GetLeftTop());
        // horizontal page alignment
        if (GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned)
            { /*noop*/ }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered)
            {
            shapeTopLeftCorner.x += safe_divide<double>(GetBoundingBox(dc).GetWidth(), 2) -
                safe_divide<double>(drawRect.GetWidth(), 2);
            }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned)
            {
            shapeTopLeftCorner.x += GetBoundingBox(dc).GetWidth() - drawRect.GetWidth();
            }
        // vertical page aligment
        if (GetPageVerticalAlignment() == PageVerticalAlignment::TopAligned)
            { /*noop*/ }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::Centered)
            {
            shapeTopLeftCorner.y += safe_divide<double>(GetBoundingBox(dc).GetHeight(), 2) -
                safe_divide<double>(drawRect.GetHeight(), 2);
            }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned)
            {
            shapeTopLeftCorner.y += GetBoundingBox(dc).GetHeight() - drawRect.GetHeight();
            }
        
        drawRect.SetTopLeft(shapeTopLeftCorner);

        ShapeRenderer sh(GetGraphItemInfo());

        switch (m_shape)
            {
            case IconShape::BlankIcon:
                // nothing to draw
                break;
            case IconShape::FallLeafIcon:
                sh.DrawFallLeaf(drawRect, dc);
                break;
            case IconShape::FlowerIcon:
                sh.DrawFlower(drawRect, dc);
                break;
            case IconShape::SunIcon:
                sh.DrawSun(drawRect, dc);
                break;
            case IconShape::SquareIcon:
                sh.DrawSquare(drawRect, dc);
                break;
            }

        // draw the outline
        if (IsSelected())
            {
            wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawRectangle(GetBoundingBox(dc));
            if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2),
                                       wxPENSTYLE_DOT));
                dc.DrawRectangle(drawRect);
                }
            }

        return bBox;
        }

    //---------------------------------------------------
    wxRect Shape::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        wxRect rect(ScaleToScreenAndCanvas(m_sizeDIPs));
        if (GetAnchoring() == Anchoring::TopLeftCorner)
            { rect.SetTopLeft(GetAnchorPoint()); }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            { rect.SetBottomLeft(GetAnchorPoint()); }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            { rect.SetTopRight(GetAnchorPoint()); }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            { rect.SetBottomRight(GetAnchorPoint()); }
        else if (GetAnchoring() == Anchoring::Center)
            {
            rect.SetTopLeft(GetAnchorPoint());
            rect.Offset(rect.GetWidth() / 2, rect.GetHeight() / 2);
            }
        return rect;
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCircularSign(const wxRect rect, wxDC& dc)
        {
        const auto radius =
            safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 2);
        wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(1)));
        wxDCBrushChanger bc(dc, GetGraphItemInfo().GetBrush());

        const auto circleCenter = rect.GetLeftTop() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        dc.DrawCircle(circleCenter, radius);

        // lettering on the sign
        Label theLabel(GraphItemInfo(GetGraphItemInfo().GetText()).Pen(wxNullPen).
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
    void ShapeRenderer::DrawSun(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        const auto centerPt = wxPoint(0, 0) +
            wxSize(memDC.GetSize().GetWidth() / 2, memDC.GetSize().GetHeight() / 2);

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for sun icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::SunsetOrange),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::SunsetOrange));
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
    void ShapeRenderer::DrawFlower(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
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
    void ShapeRenderer::DrawSquare(const wxRect rect, wxDC& dc)
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        wxDCPenChanger pc(dc, scaledPen);
        wxDCBrushChanger bc(dc, GetGraphItemInfo().GetBrush());
        dc.DrawRectangle(rect);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFallLeaf(const wxRect rect, wxDC& dc)
        {
        wxBitmap bmp(rect.GetSize());
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        const wxRect dcRect(memDC.GetSize());

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for leaf icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::DarkBrown),
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

    //---------------------------------------------------
    void ShapeRenderer::DrawCurlyBraces(const wxRect rect, wxDC& dc, const Side side)
        {
        wxASSERT_MSG(GetGraphItemInfo().GetPen().IsOk(),
                     L"Pen should be set in Shape for curly braces!");
        wxBitmap bmp(rect.GetSize());
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);

        auto gc = wxGraphicsContext::Create(memDC);
        wxASSERT_MSG(gc, L"Failed to get graphics context for curly braces!");
        if (gc && (side == Side::Left || side == Side::Right))
            {
            wxRect drawRect(rect.GetSize());
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);

                // shrink drawing area for wider pens so that they don't
                // go outside of it
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

            wxDELETE(gc);
            }
        else if (gc && (side == Side::Bottom || side == Side::Top))
            {
            wxRect drawRect(rect.GetSize());
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);

                // shrink drawing area for wider pens so that they don't
                // go outside of it
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

            wxDELETE(gc);
            }

        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(bmp, rect.GetTopLeft(), true);
        }
    }
