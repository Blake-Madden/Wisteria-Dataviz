///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "shapes.h"
#include "label.h"
#include "image.h"
#include "polygon.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Icons;

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    GraphicsContextFallback::GraphicsContextFallback(
        wxDC* dc, const wxRect rect)
        {
        m_gc = nullptr;
        m_drawingToBitmap = false; // reset
        wxASSERT_MSG(dc, L"Invalid DC for graphics context!");
        if (dc == nullptr)
            { return; }
        m_rect = rect;
        m_dc = dc;
        m_gc = m_dc->GetGraphicsContext();
        // DC doesn't support GetGraphicsContext(), so fallback to
        // drawing to a bitmap that we will blit later
        if (m_gc == nullptr)
            {
            m_drawingToBitmap = true;
            m_bmp = wxBitmap(m_dc->GetSize());
            Image::SetOpacity(m_bmp, wxALPHA_OPAQUE);
            m_memDC.SelectObject(m_bmp);
            m_memDC.Clear();

            m_gc = wxGraphicsContext::Create(m_memDC);
            }
        wxASSERT_MSG(m_gc, L"Failed to get graphics context!");
        }

    //---------------------------------------------------
    GraphicsContextFallback::~GraphicsContextFallback()
        {
        // flush drawing commands to bitmap and then blit it
        // onto the original DC
        if (m_drawingToBitmap)
            {
            delete m_gc; m_gc = nullptr;
            m_memDC.SelectObject(wxNullBitmap);
            m_bmp = m_bmp.GetSubBitmap(m_rect);
            m_dc->DrawBitmap(m_bmp, m_rect.GetTopLeft());
            }
        else
            { m_gc->Flush(); }
        }

    //---------------------------------------------------
    void Shape::SetBoundingBox(const wxRect& rect,
        [[maybe_unused]] wxDC& dc,
        [[maybe_unused]] const double parentScaling)
        {
        m_sizeDIPs.x = (IsFixedWidthOnCanvas() ?
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
        // vertical page alignment
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

        Draw(drawRect, dc);

        // draw the outline
        if (IsSelected())
            {
            wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawRectangle(GetBoundingBox(dc));
            if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2),
                                       wxPENSTYLE_DOT));
                dc.DrawRectangle(drawRect);
                }
            }

        return bBox;
        }

    //---------------------------------------------------
    Shape::Shape(const GraphItems::GraphItemInfo& itemInfo,
        const Icons::IconShape shape,
        const wxSize sz,
        const wxBitmapBundle* img /*= nullptr*/) :
        GraphItemBase(itemInfo), m_shapeSizeDIPs(sz),
        m_sizeDIPs(sz), m_shape(shape), m_renderer(itemInfo, img)
        {
        // connect the rendering function to the shape
        switch (m_shape)
            {
            case IconShape::Blank:
                // nothing to draw
                m_drawFunction = nullptr;
                break;
            case IconShape::ArrowRight:
                m_drawFunction = &ShapeRenderer::DrawRightArrow;
                break;
            case IconShape::HorizontalLine:
                m_drawFunction = &ShapeRenderer::DrawHorizontalLine;
                break;
            case IconShape::Circle:
                m_drawFunction = &ShapeRenderer::DrawCircle;
                break;
            case IconShape::Square:
                m_drawFunction = &ShapeRenderer::DrawSquare;
                break;
            case IconShape::Asterisk:
                m_drawFunction = &ShapeRenderer::DrawAsterisk;
                break;
            case IconShape::Plus:
                m_drawFunction = &ShapeRenderer::DrawPlus;
                break;
            case IconShape::TriangleUpward:
                m_drawFunction = &ShapeRenderer::DrawUpwardTriangle;
                break;
            case IconShape::TriangleDownward:
                m_drawFunction = &ShapeRenderer::DrawDownwardTriangle;
                break;
            case IconShape::TriangleRight:
                m_drawFunction = &ShapeRenderer::DrawRightTriangle;
                break;
            case IconShape::TriangleLeft:
                m_drawFunction = &ShapeRenderer::DrawLeftTriangle;
                break;
            case IconShape::Diamond:
                m_drawFunction = &ShapeRenderer::DrawDiamond;
                break;
            case IconShape::Hexagon:
                m_drawFunction = &ShapeRenderer::DrawHexagon;
                break;
            case IconShape::BoxPlot:
                m_drawFunction = &ShapeRenderer::DrawBoxPlot;
                break;
            case IconShape::Sun:
                m_drawFunction = &ShapeRenderer::DrawSun;
                break;
            case IconShape::Flower:
                m_drawFunction = &ShapeRenderer::DrawFlower;
                break;
            case IconShape::FallLeaf:
                m_drawFunction = &ShapeRenderer::DrawFallLeaf;
                break;
            case IconShape::WarningRoadSign:
                m_drawFunction = &ShapeRenderer::DrawWarningRoadSign;
                break;
            case IconShape::LocationMarker:
                m_drawFunction = &ShapeRenderer::DrawGeoMarker;
                break;
            case IconShape::GoRoadSign:
                m_drawFunction = &ShapeRenderer::DrawGoSign;
                break;
            case IconShape::Image:
                m_drawFunction = &ShapeRenderer::DrawImage;
                break;
            case IconShape::LeftCurlyBrace:
                m_drawFunction = &ShapeRenderer::DrawLeftCurlyBrace;
                break;
            case IconShape::RightCurlyBrace:
                m_drawFunction = &ShapeRenderer::DrawRightCurlyBrace;
                break;
            case IconShape::TopCurlyBrace:
                m_drawFunction = &ShapeRenderer::DrawTopCurlyBrace;
                break;
            case IconShape::BottomCurlyBrace:
                m_drawFunction = &ShapeRenderer::DrawBottomCurlyBrace;
                break;
            case IconShape::Man:
                m_drawFunction = &ShapeRenderer::DrawMan;
                break;
            case IconShape::Woman:
                m_drawFunction = &ShapeRenderer::DrawWoman;
                break;
            case IconShape::BusinessWoman:
                m_drawFunction = &ShapeRenderer::DrawBusinessWoman;
                break;
            case IconShape::ChevronDownward:
                m_drawFunction = &ShapeRenderer::DrawChevronDownward;
                break;
            case IconShape::ChevronUpward:
                m_drawFunction = &ShapeRenderer::DrawChevronUpward;
                break;
            case IconShape::Text:
                m_drawFunction = &ShapeRenderer::DrawText;
                break;
            case IconShape::Tack:
                m_drawFunction = &ShapeRenderer::DrawTack;
                break;
            case IconShape::Banner:
                m_drawFunction = &ShapeRenderer::DrawBanner;
                break;
            case IconShape::WaterColorRectangle:
                m_drawFunction = &ShapeRenderer::DrawWaterColorRectangle;
                break;
            default:
                m_drawFunction = nullptr;
                break;
            }
        }

    //---------------------------------------------------
    void Shape::Draw(const wxRect& drawRect, wxDC& dc) const // cppcheck-suppress constParameter
        {
        // apply any brush, pen, etc. changes if necessary
        if (m_rendererNeedsUpdating)
            { m_renderer.m_graphInfo = GraphItemBase::GetGraphItemInfo(); }
        m_rendererNeedsUpdating = false;

        wxASSERT_LEVEL_2_MSG(m_shape == IconShape::Blank || m_drawFunction,
            L"Shape failed to set drawing function!");
        if (m_drawFunction != nullptr)
            {
            (m_renderer.*m_drawFunction)(drawRect, dc);
            }
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

    // random number engine for water color and other "hand drawn" effects
    std::mt19937 ShapeRenderer::m_mt{ std::random_device{}() };

    //---------------------------------------------------
    void ShapeRenderer::DrawWithBaseColorAndBrush(wxDC& dc,
                                                  const std::function<void(void)>& fn) const
        {
        if (GetGraphItemInfo().GetBaseColor())
            {
            DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBaseColor().value());
            fn();
            }
        DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        fn();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCircularSign(const wxRect rect, const wxBrush& brush,
                                         const wxString& text, wxDC& dc) const
        {
        const auto signOutlineWidth = rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2;
        wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(signOutlineWidth)));
        wxDCBrushChanger bc(dc, brush);

        const auto radius = GetRadius(rect);
        const auto circleCenter = GetMidPoint(rect);

        dc.DrawCircle(circleCenter, radius);

        // lettering on the sign
        Label theLabel(GraphItemInfo(text).Pen(wxNullPen).
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
    void ShapeRenderer::DrawSun(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const auto centerPt = rect.GetTopLeft() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for sun icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::SunsetOrange),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::SunsetOrange));
            // a line going from the middle of the left side to the middle of the right
            const std::array<wxPoint2DDouble, 2> points =
                {
                wxPoint(rect.GetLeft(), rect.GetTop() + (rect.GetHeight() / 2)),
                wxPoint(rect.GetRight(), rect.GetTop() + (rect.GetHeight() / 2))
                };
            // save current transform matrix state
            gc->PushState();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the sun beams, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle = 0.0;
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over from
                // the translated origin
                gc->StrokeLine(points[0].m_x - centerPt.x, points[0].m_y - centerPt.y,
                               points[1].m_x - centerPt.x, points[1].m_y - centerPt.y);
                angle += 45;
                }
            // restore transform matrix
            gc->PopState();
            // draw the sun
            const wxRect sunRect = wxRect(rect).Deflate(rect.GetWidth()/4);
            gc->DrawEllipse(sunRect.GetTopLeft().x, sunRect.GetTopLeft().y,
                            sunRect.GetWidth(), sunRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFlower(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const auto centerPt = rect.GetTopLeft() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for flower icon!");
        if (gc)
            {
            gc->SetPen(wxPen(ColorContrast::Shade(ColorBrewer::GetColor(Color::Wisteria)),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::Wisteria));
            // a line going from the middle of the left side to the middle of the right
            wxRect petalRect(
                wxPoint(rect.GetLeft() + (rect.GetWidth()/2), rect.GetTop() + (rect.GetHeight()/2)),
                wxSize(rect.GetWidth()/2, rect.GetHeight()/6));
            petalRect.Offset(wxPoint(0, petalRect.GetHeight() / 2));
            // save current transform matrix state
            gc->PushState();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the sun beams, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle{ 0.0 };
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over from
                // the translated origin
                gc->DrawEllipse(petalRect.GetTopLeft().x - centerPt.x,
                                petalRect.GetTopLeft().y - centerPt.y,
                                petalRect.GetWidth(), petalRect.GetHeight());
                angle += 45;
                }
            // restore transform matrix
            gc->PopState();
            // draw the middle of flower
            gc->SetBrush(ColorBrewer::GetColor(Color::BabyBlue));
            const wxRect flowerRect = wxRect(rect).Deflate(rect.GetWidth()/4);
            gc->DrawEllipse(flowerRect.GetTopLeft().x, flowerRect.GetTopLeft().y,
                            flowerRect.GetWidth(), flowerRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBoxPlot(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        // whisker
        dc.DrawLine(wxPoint(rect.GetLeft()+(rect.GetWidth()/2),
                            rect.GetTop()),
                    wxPoint(rect.GetLeft()+(rect.GetWidth()/2),
                            rect.GetBottom()));
        dc.DrawLine(wxPoint(rect.GetLeft()+(rect.GetWidth()/2) -
                                rect.GetWidth()/4,
                            rect.GetTop()),
                    wxPoint(rect.GetLeft()+(rect.GetWidth()/2) +
                                rect.GetWidth()/4,
                            rect.GetTop()));
        dc.DrawLine(wxPoint(rect.GetLeft()+(rect.GetWidth()/2) -
                                rect.GetWidth()/4,
                            rect.GetBottom()),
                    wxPoint(rect.GetLeft()+(rect.GetWidth()/2) +
                                rect.GetWidth()/4,
                            rect.GetBottom()));
        rect.y += (rect.GetHeight()/2) - (rect.GetHeight()/4); // center
        rect.SetHeight(rect.GetHeight()/2);
        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawRectangle(rect); });
        // median line
        dc.DrawLine(wxPoint(rect.GetLeft(), rect.GetTop() +
                            (rect.GetHeight()/2)),
                    wxPoint(rect.GetRight(), rect.GetTop() +
                            (rect.GetHeight()/2)));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSquare(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawRectangle(rect); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCircle(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawCircle(GetMidPoint(rect), GetRadius(rect)); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawText(const wxRect rect, wxDC& dc) const
        {
        Label theLabel(GraphItemInfo(GetGraphItemInfo().GetText()).Pen(wxNullPen).
            AnchorPoint(GetMidPoint(rect)).Anchoring(Anchoring::Center).
            LabelAlignment(TextAlignment::Centered).
            DPIScaling(GetDPIScaleFactor()));
        theLabel.SetFontColor(GetGraphItemInfo().GetPen().GetColour());
        theLabel.GetFont().MakeBold();
        theLabel.SetBoundingBox(rect, dc, GetScaling());
        theLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        theLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        theLabel.Draw(dc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTack(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const std::array<wxPoint, 11> points =
            {
            // the needle
            wxPoint(GetXPosFromLeft(rect, 0),
                    GetYPosFromTop(rect, math_constants::half)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::half)),
            // top half of tack's handle
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, 0)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, 0.90),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, safe_divide(math_constants::third, 2.0))),
            // bottom half
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, math_constants::half +
                                         (safe_divide(math_constants::third, 2.0) * 2))),
            wxPoint(GetXPosFromLeft(rect, 0.90),
                    GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::full)),
            wxPoint(GetXPosFromLeft(rect, math_constants::third),
                    GetYPosFromTop(rect, math_constants::half)),
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawChevronDownward(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth() * 2) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(-iconRadius, 0),
            midPoint + wxPoint(0, iconRadius),
            midPoint + wxPoint(iconRadius, 0)
            };

        std::for_each(points.begin(), points.end(),
            [&iconRadius, this](auto& pt)
            { pt.y -= ScaleToScreenAndCanvas(2 + (iconRadius/2)); });
        dc.DrawLines(points.size(), &points[0]);

        std::for_each(points.begin(), points.end(),
            [&iconRadius, this](auto& pt)
            { pt.y += ScaleToScreenAndCanvas(4); });
        dc.DrawLines(points.size(), &points[0]);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawChevronUpward(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth() * 2) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(-iconRadius, 0),
            midPoint + wxPoint(0, -iconRadius),
            midPoint + wxPoint(iconRadius, 0)
            };

        std::for_each(points.begin(), points.end(),
            [&iconRadius, this](auto& pt)
            { pt.y += ScaleToScreenAndCanvas(-2 + (iconRadius/2)); });
        dc.DrawLines(points.size(), &points[0]);

        std::for_each(points.begin(), points.end(),
            [&iconRadius, this](auto& pt)
            { pt.y += ScaleToScreenAndCanvas(4); });
        dc.DrawLines(points.size(), &points[0]);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawDiamond(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 4> points =
            {
            midPoint + wxPoint(0, -iconRadius),
            midPoint + wxPoint(iconRadius, 0),
            midPoint + wxPoint(0, iconRadius),
            midPoint + wxPoint(-iconRadius, 0)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawImage(wxRect rect, wxDC& dc) const
        {
        if (m_iconImage && m_iconImage->IsOk())
            {
            const auto downScaledSize = geometry::downscaled_size(
                std::make_pair<double, double>(m_iconImage->GetDefaultSize().GetWidth(),
                    m_iconImage->GetDefaultSize().GetHeight()),
                std::make_pair<double, double>(rect.GetWidth(), rect.GetHeight()));
            const wxBitmap scaledImg = m_iconImage->GetBitmap(wxSize(downScaledSize.first,
                                                             downScaledSize.second));
            dc.DrawBitmap(scaledImg, rect.GetTopLeft());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGeoMarker(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const wxRect dcRect(rect);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for geo marker!");
        if (gc)
            {
            wxPen scaledPen = GetGraphItemInfo().GetPen();
            if (scaledPen.IsOk())
                { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }

            gc->SetPen(scaledPen);
            gc->SetBrush(GetGraphItemInfo().GetBrush());
            auto marker = gc->CreatePath();
            // bottom middle, stretched out to both top corners
            marker.MoveToPoint(GetXPosFromLeft(dcRect, math_constants::half),
                               GetYPosFromTop(dcRect, 1));
            marker.AddCurveToPoint(
                GetXPosFromLeft(dcRect, -.75), GetYPosFromTop(dcRect, -math_constants::quarter),
                GetXPosFromLeft(dcRect, 1.75), GetYPosFromTop(dcRect, -math_constants::quarter),
                GetXPosFromLeft(dcRect, math_constants::half),
                GetYPosFromTop(dcRect, math_constants::full));

            marker.CloseSubpath();
            gc->FillPath(marker);
            gc->StrokePath(marker);

            // outer ring in center of head
            wxRect topRect = dcRect;
            topRect.SetHeight(topRect.GetHeight() * math_constants::third);
            topRect.SetWidth(topRect.GetHeight()); // make it a square
            topRect.SetX(topRect.GetX() +
                ((dcRect.GetWidth() / 2) - (topRect.GetWidth() / 2)));
            topRect.SetY(topRect.GetY() + (topRect.GetHeight() * math_constants::two_thirds));

            gc->SetBrush(wxBrush(ColorContrast::ShadeOrTint(
                GetGraphItemInfo().GetBrush().GetColour())));
            gc->SetPen(wxPen(ColorContrast::ShadeOrTint(
                GetGraphItemInfo().GetBrush().GetColour())));
            gc->DrawEllipse(topRect.GetTopLeft().x, topRect.GetTopLeft().y,
                topRect.GetWidth(), topRect.GetHeight());

            topRect.Deflate(topRect.GetWidth() * math_constants::third);
            gc->SetBrush(*wxWHITE);
            gc->SetPen(*wxWHITE);
            gc->DrawEllipse(topRect.GetTopLeft().x, topRect.GetTopLeft().y,
                topRect.GetWidth(), topRect.GetHeight());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGoSign(const wxRect rect, wxDC& dc) const
        {
        wxDCBrushChanger bc(dc, ColorBrewer::GetColor(Color::SchoolBusYellow));

        const auto iconRadius = GetRadius(rect);

        // sign post
            {
            const wxPoint pt[2] =
                {
                rect.GetTopLeft() +
                    wxSize(rect.GetWidth() / 2, iconRadius),
                // bottom
                rect.GetBottomLeft() +
                    wxSize(rect.GetWidth() / 2, 0)
                };
            const auto signPostWidth = std::min<int>(ScaleToScreenAndCanvas(4),
                                                     (rect.GetWidth() / 5));
            // white outline of sign post
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(*wxWHITE, signPostWidth + ScaleToScreenAndCanvas(1)).
                            Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
            // actual sign post
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(ColorBrewer::GetColor(Color::SlateGray),
                        signPostWidth).Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
            }
        // sign
            {
            const auto signRect = wxRect(rect.GetLeftTop(),
                                         wxSize(rect.GetWidth(),
                                                rect.GetHeight() * math_constants::two_thirds));
            DrawCircularSign(signRect, ColorBrewer::GetColor(Color::KellyGreen), _(L"GO"), dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBanner(const wxRect rect, wxDC& dc) const
        {
        // sign posts
            {
            wxPoint pt[2] =
                {
                rect.GetTopLeft(),
                rect.GetBottomLeft()
                };
            const auto signPostWidth = std::min<int>(ScaleToScreenAndCanvas(8),
                                                     (rect.GetWidth() / 5));
            pt[0].x += signPostWidth/2;
            pt[1].x += signPostWidth/2;

            const auto drawPost = [&]()
                {
                // white outline of sign post used to contrast black sign post
                // against a possibly dark background
                    {
                    wxDCPenChanger pc(dc,
                        wxPen(wxPenInfo(*wxWHITE, signPostWidth + ScaleToScreenAndCanvas(1)).
                                Cap(wxPenCap::wxCAP_BUTT)));
                    dc.DrawLine(pt[0], pt[1]);
                    }
                // actual sign post
                    {
                    wxDCPenChanger pc(dc,
                        wxPen(wxPenInfo(ColorBrewer::GetColor(Color::SlateGray),
                            signPostWidth).Cap(wxPenCap::wxCAP_BUTT)));
                    dc.DrawLine(pt[0], pt[1]);
                    }
                };

            drawPost();
            pt[0].x = rect.GetRight() - signPostWidth/2;
            pt[1].x = rect.GetRight() - signPostWidth/2;
            drawPost();
            }
        // sign
            {
            auto anchorPt = rect.GetTopLeft();
            anchorPt.y += rect.GetHeight() * math_constants::twentieth;
            Label bannerLabel(GraphItemInfo(GetGraphItemInfo().GetText()).
                Pen(wxPen(wxPenInfo(*wxBLACK, 1))).
                FontBackgroundColor(GetGraphItemInfo().GetBrush().GetColour()).
                FontColor(GetGraphItemInfo().GetPen().GetColour()).
                Anchoring(Anchoring::TopLeftCorner).
                LabelAlignment(TextAlignment::Centered).
                DPIScaling(GetDPIScaleFactor()));
            bannerLabel.GetFont().MakeBold();
            bannerLabel.SetBoundingBox(
                wxRect(anchorPt,
                    wxSize(rect.GetWidth(), rect.GetHeight() * math_constants::third)),
                dc, GetScaling());
            bannerLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            bannerLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            bannerLabel.Draw(dc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawWarningRoadSign(const wxRect rect, wxDC& dc) const
        {
        wxDCBrushChanger bc(dc, ColorBrewer::GetColor(Color::SchoolBusYellow));

        const auto iconRadius = GetRadius(rect);

        // sign post
            {
            wxPoint pt[2] =
                {
                rect.GetTopLeft() +
                    // top of post is in the middle of the sign
                    // so that pen cap doesn't appear above sign
                    wxSize(rect.GetWidth() / 2, iconRadius),
                // bottom
                rect.GetBottomLeft() +
                    wxSize(rect.GetWidth() / 2, 0)
                };
            const auto signPostWidth = std::min<int>(ScaleToScreenAndCanvas(4),
                                                     (rect.GetWidth() / 5));
            // white outline of sign post used to contrast black sign post
            // against a possibly dark background
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(*wxWHITE, signPostWidth + ScaleToScreenAndCanvas(1)).
                            Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
            // actual sign post
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(ColorBrewer::GetColor(Color::SlateGray),
                        signPostWidth).Cap(wxPenCap::wxCAP_BUTT)));
                dc.DrawLine(pt[0], pt[1]);
                }
            }
        // sign
            {
            const auto signOutlineWidth = rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2;
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(signOutlineWidth)));
            const auto signHeight = rect.GetHeight() * math_constants::third;
            const auto signRadius = std::min(signHeight, iconRadius);
            const auto circleCenter = rect.GetLeftTop() +
                wxSize(rect.GetWidth() / 2, signRadius);
            const std::array<wxPoint, 4> points =
                {
                circleCenter + wxPoint(0, -signRadius),
                circleCenter + wxPoint(signRadius, 0),
                circleCenter + wxPoint(0, signRadius),
                circleCenter + wxPoint(-signRadius, 0)
                };
            dc.DrawPolygon(points.size(), &points[0]);
            // ! label
            Label bangLabel(GraphItemInfo(L"!").Pen(wxNullPen).
                AnchorPoint(circleCenter).Anchoring(Anchoring::Center).
                LabelAlignment(TextAlignment::Centered).
                DPIScaling(GetDPIScaleFactor()));
            bangLabel.SetFontColor(*wxBLACK);
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
    void ShapeRenderer::DrawRightArrow(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DrawWithBaseColorAndBrush(dc,
            [&]()
            {
            Polygon::DrawArrow(dc,
                wxPoint(rect.GetLeft(), rect.GetTop()+rect.GetHeight()/2),
                wxPoint(rect.GetRight(), rect.GetTop()+rect.GetHeight()/2),
                wxSize(
                    ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSizeDIPs().GetWidth()),
                    ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSizeDIPs().GetHeight())) );
            }
            );
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHexagon(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 6> points =
            {
            midPoint + wxPoint(-iconRadius / 2, -iconRadius),
            midPoint + wxPoint(-iconRadius, 0),
            midPoint + wxPoint(-iconRadius / 2, iconRadius),
            midPoint + wxPoint(iconRadius / 2, iconRadius),
            midPoint + wxPoint(iconRadius, 0),
            midPoint + wxPoint(iconRadius / 2, -iconRadius)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawUpwardTriangle(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(0, -iconRadius),
            midPoint + wxPoint(-iconRadius, iconRadius),
            midPoint + wxPoint(iconRadius, iconRadius)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawDownwardTriangle(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(0, iconRadius),
            midPoint + wxPoint(-iconRadius, -iconRadius),
            midPoint + wxPoint(iconRadius, -iconRadius)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawRightTriangle(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(iconRadius, 0),
            midPoint + wxPoint(-iconRadius, iconRadius),
            midPoint + wxPoint(-iconRadius, -iconRadius)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawLeftTriangle(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()) ); }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        const std::array<wxPoint, 3> points =
            {
            midPoint + wxPoint(-iconRadius, 0),
            midPoint + wxPoint(iconRadius, iconRadius),
            midPoint + wxPoint(iconRadius, -iconRadius)
            };

        DrawWithBaseColorAndBrush(dc, [&](){ dc.DrawPolygon(points.size(), &points[0]); });
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawAsterisk(wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const auto centerPt = rect.GetTopLeft() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for asterisk icon!");
        if (gc)
            {
            wxPen scaledPen = GetGraphItemInfo().GetPen();
            if (scaledPen.IsOk())
                {
                scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)) );
                }
            gc->SetPen(scaledPen);
            // a line going from the middle of the left side to the middle of the right
            const std::array<wxPoint2DDouble, 2> points =
                {
                wxPoint(rect.GetLeft(), rect.GetTop() + (rect.GetHeight() / 2)),
                wxPoint(rect.GetRight(), rect.GetTop() + (rect.GetHeight() / 2))
                };
            // save current transform matrix state
            gc->PushState();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the lines, which will be the horizontal line going across the middle,
            // but rotated 45 degrees around the center
            double angle = 0.0;
            while (angle < 360)
                {
                gc->Rotate(geometry::degrees_to_radians(angle));
                // note that because we translated to the middle of the drawing area,
                // we need to adjust the points of our middle line back and over from
                // the translated origin
                gc->StrokeLine(points[0].m_x - centerPt.x, points[0].m_y - centerPt.y,
                               points[1].m_x - centerPt.x, points[1].m_y - centerPt.y);
                angle += 45;
                }
            // restore transform matrix
            gc->PopState();
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawPlus(wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)) );
            }
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const auto iconRadius = GetRadius(rect);
        const auto midPoint = GetMidPoint(rect);

        dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                    wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
        dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                    wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawHorizontalLine(const wxRect rect, wxDC& dc) const
        {
        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk())
            {
            // for a line icon, make it a minimum of 2 pixels wide
            scaledPen.SetWidth(
                ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)) );
            }
        DCPenChangerIfDifferent pc(dc, scaledPen);
        DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        dc.DrawLine(wxPoint(rect.GetLeft(), rect.GetTop() + (rect.GetHeight()/2)),
                    wxPoint(rect.GetRight(), rect.GetTop() + (rect.GetHeight()/2)) );
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFallLeaf(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for leaf icon!");
        if (gc)
            {
            // draw the leaf
            gc->SetPen(wxPen(ColorContrast::Shade(ColorBrewer::GetColor(Color::ChineseRed)),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ColorBrewer::GetColor(Color::ChineseRed));
            auto leafPath = gc->CreatePath();
            // left side of leaf
            leafPath.MoveToPoint(GetXPosFromLeft(rect, math_constants::half),
                                 GetYPosFromTop(rect, math_constants::three_quarters));
            leafPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, .6),
                // top
                GetXPosFromLeft(rect, math_constants::half), GetYPosFromTop(rect, 0));

            // right side
            leafPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, 1), GetYPosFromTop(rect, .6),
                // top
                GetXPosFromLeft(rect, math_constants::half),
                GetYPosFromTop(rect, math_constants::three_quarters));
            leafPath.CloseSubpath();
            gc->FillPath(leafPath);
            gc->StrokePath(leafPath);

            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::DarkBrown),
                       ScaleToScreenAndCanvas(1)));

            // draw the stem
            auto stemPath = gc->CreatePath();
            // start of middle of bottom
            stemPath.MoveToPoint(GetXPosFromLeft(rect, math_constants::half),
                                                 rect.GetBottom());
            // draw to the top middle
            stemPath.AddLineToPoint(GetXPosFromLeft(rect, math_constants::half),
                                                    rect.GetTop());
            gc->StrokePath(stemPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawWaterColorRectangle(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for water color effect!");
        if (gc)
            {
            const auto strayLinesAlongTopBottom =
                std::max<size_t>(safe_divide<size_t>(rect.GetWidth(),
                    ScaleToScreenAndCanvas(100)), 1);
            const auto strayLinesAlongLeftRight =
                std::max<size_t>(safe_divide<size_t>(rect.GetHeight(),
                    ScaleToScreenAndCanvas(100)), 1);

            // get the min percent of the height needed, which is the lesser of 3 DIPs or 33%
            const auto heightMinDIPsPercent = std::min(
                safe_divide<double>(ScaleToScreenAndCanvas(3), rect.GetHeight()),
                                    math_constants::third);
            // ...and use the larger between that or 10 DIPs (or 20%) are the height
            const auto wiggleTopBottom = std::max(std::min(
                safe_divide<double>(ScaleToScreenAndCanvas(10), rect.GetHeight()),
                math_constants::twentieth),
                heightMinDIPsPercent);
            std::uniform_real_distribution<> wiggleDistroTopBottom(-wiggleTopBottom, wiggleTopBottom);

            const auto widthMinDIPsPercent = std::min(
                safe_divide<double>(ScaleToScreenAndCanvas(3), rect.GetWidth()),
                math_constants::third);
            const auto wiggleLeftRight = std::max(std::min(
                safe_divide<double>(ScaleToScreenAndCanvas(10), rect.GetWidth()),
                math_constants::twentieth),
                widthMinDIPsPercent);
            std::uniform_real_distribution<> wiggleDistroLeftRight(-wiggleLeftRight, wiggleLeftRight);

            // "watercolor" fill of rectangle
            gc->SetPen(*wxTRANSPARENT_PEN);
            wxBrush br{ GetGraphItemInfo().GetBrush() };
            // make the brush translucent (it not already so) to make it a watercolor brush
            if (br.GetColour().Alpha() == wxALPHA_OPAQUE)
                {
                br.SetColour(ColorContrast::ChangeOpacity(br.GetColour(),
                                                          Settings::GetTranslucencyValue()));
                }
            gc->SetBrush(br);
            auto fillPath = gc->CreatePath();

            // top
            //----
            fillPath.MoveToPoint(GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, 0)); // top left
            // "outside of the lines" points along the top
            double previousXPos{ 0.0 };
            for (size_t i = 1; i <= strayLinesAlongTopBottom; ++i)
                {
                auto xPos = safe_divide<double>(math_constants::full, strayLinesAlongTopBottom + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, previousXPos +
                                          safe_divide<double>(xPos - previousXPos, 2)),
                    GetYPosFromTop(rect, wiggleDistroTopBottom(m_mt)),
                    GetXPosFromLeft(rect, xPos),
                    GetYPosFromTop(rect, wiggleDistroTopBottom(m_mt)));
                previousXPos = xPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, wiggleDistroTopBottom(m_mt)),
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, wiggleDistroTopBottom(m_mt))); // top right

            // right
            //------
            double previousYPos{ 0.0 };
            for (size_t i = 1; i <= strayLinesAlongLeftRight; ++i)
                {
                auto yPos = safe_divide<double>(math_constants::full, strayLinesAlongLeftRight + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                    GetYPosFromTop(rect, previousYPos +
                                         safe_divide<double>(yPos - previousYPos, 2)),
                    GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                    GetYPosFromTop(rect, yPos));
                previousYPos = yPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(m_mt)),
                GetXPosFromLeft(rect, math_constants::full + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(m_mt))); // bottom right

            // bottom
            //-------
            // "outside of the lines" points along the bottom
            previousXPos = math_constants::full;
            for (long i = static_cast<long>(strayLinesAlongTopBottom); i > 0; --i)
                {
                auto xPos = safe_divide<double>(math_constants::full, strayLinesAlongTopBottom + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, xPos + safe_divide<double>(previousXPos - xPos, 2)),
                    GetYPosFromTop(rect, math_constants::full - wiggleDistroTopBottom(m_mt)),
                    GetXPosFromLeft(rect, xPos),
                    GetYPosFromTop(rect, math_constants::full - wiggleDistroTopBottom(m_mt)));
                previousXPos = xPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(m_mt)),
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, math_constants::full + wiggleDistroTopBottom(m_mt))); // bottom left

            // left
            //-----
            previousYPos = math_constants::full;
            for (long i = static_cast<long>(strayLinesAlongLeftRight); i > 0; --i)
                {
                auto yPos = safe_divide<double>(math_constants::full, strayLinesAlongLeftRight + 1) * i;
                fillPath.AddQuadCurveToPoint(
                    GetXPosFromLeft(rect, wiggleDistroLeftRight(m_mt)),
                    GetYPosFromTop(rect, yPos +
                                         safe_divide<double>(previousYPos - yPos, 2)),
                    GetXPosFromLeft(rect, wiggleDistroLeftRight(m_mt)),
                    GetYPosFromTop(rect, yPos));
                previousYPos = yPos;
                }
            fillPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, 0 + wiggleDistroTopBottom(m_mt)),
                GetXPosFromLeft(rect, 0 + wiggleDistroLeftRight(m_mt)),
                GetYPosFromTop(rect, 0 + wiggleDistroTopBottom(m_mt)));

            fillPath.CloseSubpath();
            gc->FillPath(fillPath);
            gc->StrokePath(fillPath);

            // draw the hard outline on top
            if (GetGraphItemInfo().GetPen().IsOk())
                {
                wxPen scaledPen(GetGraphItemInfo().GetPen());
                scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
                gc->SetPen(scaledPen);
                gc->SetBrush(*wxTRANSPARENT_BRUSH);

                gc->DrawRectangle(rect.GetX(), rect.GetY(),
                    rect.GetWidth(), rect.GetHeight());
                }
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawCurlyBrace(const wxRect rect, wxDC& dc, const Side side) const
        {
        wxASSERT_MSG(GetGraphItemInfo().GetPen().IsOk(),
                     L"Pen should be set in Shape for curly braces!");
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);

        wxRect drawRect(rect);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for curly braces!");
        if (gc && (side == Side::Left || side == Side::Right))
            {
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
            }
        else if (gc && (side == Side::Bottom || side == Side::Top))
            {
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
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawMan(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const wxRect dcRect(rect);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for male outline!");
        if (gc)
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
            headRect.SetHeight(headRect.GetHeight() * .15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ dcRect };
            const auto neckHeight{ (dcRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(wxPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                            bodyRect.GetTop()));

            constexpr auto collarWidth{ 0.3 };
            constexpr auto shoulderWidth{ 0.1 };
            constexpr auto shoulderHeight{ 0.1 };
            constexpr auto armLength{ 0.3 };
            constexpr auto armWidth{ 0.15 };
            constexpr auto armpitWidth{ 0.05 };
            constexpr auto crotchWidth{ 0.05 };
            constexpr auto sideLength{ 0.9 };
            constexpr auto lengthBetweenArmAndLegs{ 0.05 };
            constexpr auto legWidth{ 0.175 };
            constexpr auto yControlPointOffset{ 0.05 };
            // left collar and shoulder
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - collarWidth),
                                       GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth)),
                                       GetYPosFromTop(bodyRect, 0),
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth)),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // left arm (left side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                            (math_constants::half - collarWidth - shoulderWidth)),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            (armWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength + yControlPointOffset),
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) + armWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // inside of left arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) + armWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // left armpit
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            armWidth + armpitWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // left side, down to left foot
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            armWidth + armpitWidth),
                                       GetYPosFromTop(bodyRect, sideLength));
            // left foot
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            armWidth + armpitWidth + (legWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, sideLength + yControlPointOffset),
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            armWidth + armpitWidth + legWidth),
                                       GetYPosFromTop(bodyRect, sideLength));
            // inside of left leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           (math_constants::half - collarWidth - shoulderWidth) +
                                            armWidth + armpitWidth + legWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength + lengthBetweenArmAndLegs));
            // left half of crotch
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength + lengthBetweenArmAndLegs));

            // right half of crotch
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength + lengthBetweenArmAndLegs));
            // inside of right leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, sideLength));
            // right foot
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           (legWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, sideLength + yControlPointOffset),
                                       GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) + legWidth),
                                       GetYPosFromTop(bodyRect, sideLength));
            // right side, up to armpit
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                        math_constants::half + (crotchWidth * math_constants::half) + legWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // right armpit
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           legWidth + armpitWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // inside of right arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           legWidth + armpitWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           legWidth + armpitWidth + (armWidth * math_constants::half)),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength + yControlPointOffset),
                                       GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           legWidth + armpitWidth + armWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // right arm (right side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                                           math_constants::half + (crotchWidth * math_constants::half) +
                                           legWidth + armpitWidth + armWidth),
                                       GetYPosFromTop(bodyRect, shoulderHeight));
            // right shoulder and collar
            outlinePath.AddQuadCurveToPoint(
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half + collarWidth + shoulderWidth)),
                                       GetYPosFromTop(bodyRect, 0),
                                       GetXPosFromLeft(bodyRect,
                                           (math_constants::half + collarWidth)),
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
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        wxRect dcRect(rect);
        dcRect.Deflate(
            GetGraphItemInfo().GetPen().IsOk() ?
                ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                0);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for female outline!");
        if (gc)
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
            headRect.SetHeight(headRect.GetHeight() * .15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ dcRect };
            const auto neckHeight{ (dcRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(wxPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                            bodyRect.GetTop()));

            constexpr auto collarWidth{ 0.25 };
            constexpr auto collarShortWidth{ 0.15 };
            constexpr auto shoulderWidth{ 0.1 };
            constexpr auto shoulderHeight{ 0.1 };
            constexpr auto armLength{ 0.25 };
            constexpr auto armShortLength{ 0.225 };
            constexpr auto armWidth{ 0.1 };
            constexpr auto armpitWidth{ 0.05 };
            constexpr auto waistWidth{ 0.125 };
            constexpr auto thoraxHeight{ 0.2 };
            constexpr auto legWidth{ 0.125 };
            constexpr auto dressWidth{ 0.3 };
            constexpr auto dressBottom{ 0.675 };
            constexpr auto ankleWidth{ 0.075 };
            constexpr auto yControlPointOffset{ 0.05 };
            constexpr auto xControlPointRightShoulderOffset{ 0.125 };
            constexpr auto xControlPointLeftShoulderOffset{ shoulderWidth * math_constants::quarter };
            // left collar and shoulder
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - collarShortWidth),
                                       GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - xControlPointLeftShoulderOffset)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth)),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // left arm (left side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, 0),
                GetYPosFromTop(bodyRect, shoulderHeight + armShortLength));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, (armWidth * math_constants::quarter)),
                GetYPosFromTop(bodyRect, shoulderHeight + armShortLength + yControlPointOffset),
                GetXPosFromLeft(bodyRect, armWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // inside of left arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth) + armWidth + armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // left armpit to waist
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - waistWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight));
            // left waist to bottom of dress
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - dressWidth),
                GetYPosFromTop(bodyRect, dressBottom));
            // dress bottom to leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - legWidth),
                GetYPosFromTop(bodyRect, dressBottom));
            // left leg to ankle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - ankleWidth),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle to middle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                GetYPosFromTop(bodyRect, 0.9));

            // right side
            //-----------
            // ankle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + ankleWidth),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle up right leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + legWidth),
                GetYPosFromTop(bodyRect, dressBottom));
            // dress bottom
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + dressWidth),
                GetYPosFromTop(bodyRect, dressBottom));
            // bottom of dress to right waist
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + waistWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight));
            // waist to right armpit
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    (math_constants::half + collarWidth + shoulderWidth) - armWidth - armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // inside of right arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::full - armWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect, math_constants::full - (armWidth * math_constants::quarter)),
                GetYPosFromTop(bodyRect, shoulderHeight + armShortLength + yControlPointOffset),
                GetXPosFromLeft(bodyRect, math_constants::full),
                GetYPosFromTop(bodyRect, shoulderHeight + armShortLength));
            // right arm (right side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    math_constants::half + collarWidth + shoulderWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // shoulder and right collar
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    math_constants::half + collarShortWidth + xControlPointRightShoulderOffset),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect,
                    math_constants::half + collarShortWidth),
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
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        wxRect dcRect(rect);
        dcRect.Deflate(
            GetGraphItemInfo().GetPen().IsOk() ?
            ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
            0);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for female outline!");
        if (gc)
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
            headRect.SetHeight(headRect.GetHeight() * .15);
            const auto headMiddle{ GetMidPoint(headRect) };
            outlinePath.AddCircle(headMiddle.x, headMiddle.y, GetRadius(headRect));

            // move to the middle of the shoulders
            wxRect bodyRect{ dcRect };
            const auto neckHeight{ (dcRect.GetHeight() * 0.025) };
            bodyRect.SetHeight(bodyRect.GetHeight() - headRect.GetHeight() - neckHeight);
            bodyRect.SetTop(headRect.GetBottom() + neckHeight);
            outlinePath.MoveToPoint(wxPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                                            bodyRect.GetTop()));

            constexpr auto collarWidth{ 0.25 };
            constexpr auto collarShortWidth{ 0.15 };
            constexpr auto shoulderWidth{ 0.06 };
            constexpr auto shoulderHeight{ 0.1 };
            constexpr auto armLength{ 0.25 };
            constexpr auto armWidth{ 0.06 };
            constexpr auto armpitWidth{ 0.05 };
            constexpr auto waistWidth{ 0.125 };
            constexpr auto thoraxHeight{ 0.2 };
            constexpr auto legWidth{ 0.125 };
            constexpr auto skirtWidth{ legWidth + 0.05 };
            constexpr auto hipWidth{ skirtWidth * 1.6 };
            constexpr auto skirtBottom{ 0.675 };
            constexpr auto ankleWidth{ 0.075 };
            constexpr auto yControlPointOffset{ 0.025 };
            constexpr auto xControlPointRightShoulderOffset{ 0.15 };
            constexpr auto xControlPointLeftShoulderOffset{ shoulderWidth };
            // left collar and shoulder
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - collarShortWidth),
                GetYPosFromTop(bodyRect, 0));
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - xControlPointLeftShoulderOffset)),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth)),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // left arm (left side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, (math_constants::half - collarWidth - shoulderWidth)),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // left hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth) +
                    (armWidth)),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength + yControlPointOffset),
                GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth) + armWidth + armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // inside of left arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    (math_constants::half - collarWidth - shoulderWidth) + armWidth + armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // left armpit to waist
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - waistWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight));
            // left waist to bottom of dress
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    math_constants::half - hipWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight +
                                (skirtBottom - (shoulderHeight + thoraxHeight)) * math_constants::quarter),
                GetXPosFromLeft(bodyRect, math_constants::half - skirtWidth),
                GetYPosFromTop(bodyRect, skirtBottom));
            // dress bottom to leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - legWidth),
                GetYPosFromTop(bodyRect, skirtBottom));
            // left leg to ankle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half - ankleWidth),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle to middle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                GetYPosFromTop(bodyRect, 0.9));

            // right side
            //-----------
            // ankle
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + ankleWidth),
                GetYPosFromTop(bodyRect, 0.9));
            // ankle up right leg
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + legWidth),
                GetYPosFromTop(bodyRect, skirtBottom));
            // dress bottom
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half + skirtWidth),
                GetYPosFromTop(bodyRect, skirtBottom));
            // bottom of dress to right waist
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    math_constants::half + hipWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight +
                                (skirtBottom - (shoulderHeight + thoraxHeight)) * math_constants::quarter),
                GetXPosFromLeft(bodyRect, math_constants::half + waistWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + thoraxHeight));
            // waist to right armpit
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    (math_constants::half + collarWidth + shoulderWidth) - armWidth - armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // inside of right arm
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    (math_constants::half + collarWidth + shoulderWidth) - armWidth - armpitWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // right hand
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    (math_constants::half + collarWidth + shoulderWidth) - armWidth),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength + yControlPointOffset),
                GetXPosFromLeft(bodyRect,
                    (math_constants::half + collarWidth + shoulderWidth)),
                GetYPosFromTop(bodyRect, shoulderHeight + armLength));
            // right arm (right side)
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect,
                    math_constants::half + collarWidth + shoulderWidth),
                GetYPosFromTop(bodyRect, shoulderHeight));
            // shoulder and right collar
            outlinePath.AddQuadCurveToPoint(
                GetXPosFromLeft(bodyRect,
                    math_constants::half + collarShortWidth + xControlPointRightShoulderOffset),
                GetYPosFromTop(bodyRect, 0),
                GetXPosFromLeft(bodyRect,
                    math_constants::half + collarShortWidth),
                GetYPosFromTop(bodyRect, 0));
            // collar
            outlinePath.AddLineToPoint(GetXPosFromLeft(bodyRect, math_constants::half),
                GetYPosFromTop(bodyRect, 0));

            gc->FillPath(outlinePath);
            gc->StrokePath(outlinePath);
            }
        }
    }
