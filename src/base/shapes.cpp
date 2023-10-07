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
        assert(dc && L"Invalid DC for graphics context!");
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
        assert(m_gc && L"Failed to get graphics context!");
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
        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

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

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }

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
        const static std::map<Icons::IconShape, ShapeRenderer::DrawFunction> shapeMap =
            {
                { IconShape::Blank, nullptr },
                { IconShape::ArrowRight, &ShapeRenderer::DrawRightArrow },
                { IconShape::HorizontalLine, &ShapeRenderer::DrawHorizontalLine },
                { IconShape::Circle, &ShapeRenderer::DrawCircle },
                { IconShape::Square, &ShapeRenderer::DrawSquare },
                { IconShape::Asterisk, &ShapeRenderer::DrawAsterisk },
                { IconShape::Plus, &ShapeRenderer::DrawPlus },
                { IconShape::TriangleUpward, &ShapeRenderer::DrawUpwardTriangle },
                { IconShape::TriangleDownward, &ShapeRenderer::DrawDownwardTriangle },
                { IconShape::TriangleRight, &ShapeRenderer::DrawRightTriangle },
                { IconShape::TriangleLeft, &ShapeRenderer::DrawLeftTriangle },
                { IconShape::Diamond, &ShapeRenderer::DrawDiamond },
                { IconShape::Hexagon, &ShapeRenderer::DrawHexagon },
                { IconShape::BoxPlot, &ShapeRenderer::DrawBoxPlot },
                { IconShape::Sun, &ShapeRenderer::DrawSun },
                { IconShape::Flower, &ShapeRenderer::DrawFlower },
                { IconShape::FallLeaf, &ShapeRenderer::DrawFallLeaf },
                { IconShape::WarningRoadSign, &ShapeRenderer::DrawWarningRoadSign },
                { IconShape::LocationMarker, &ShapeRenderer::DrawGeoMarker },
                { IconShape::GoRoadSign, &ShapeRenderer::DrawGoSign },
                { IconShape::Image, &ShapeRenderer::DrawImage },
                { IconShape::LeftCurlyBrace, &ShapeRenderer::DrawLeftCurlyBrace },
                { IconShape::RightCurlyBrace, &ShapeRenderer::DrawRightCurlyBrace },
                { IconShape::TopCurlyBrace, &ShapeRenderer::DrawTopCurlyBrace },
                { IconShape::BottomCurlyBrace, &ShapeRenderer::DrawBottomCurlyBrace },
                { IconShape::Man, &ShapeRenderer::DrawMan },
                { IconShape::Woman, &ShapeRenderer::DrawWoman },
                { IconShape::BusinessWoman, &ShapeRenderer::DrawBusinessWoman },
                { IconShape::ChevronDownward, &ShapeRenderer::DrawChevronDownward },
                { IconShape::ChevronUpward, &ShapeRenderer::DrawChevronUpward },
                { IconShape::Text, &ShapeRenderer::DrawText },
                { IconShape::Tack, &ShapeRenderer::DrawTack },
                { IconShape::Banner, &ShapeRenderer::DrawBanner },
                { IconShape::WaterColorRectangle, &ShapeRenderer::DrawWaterColorRectangle },
                { IconShape::GraduationCap, &ShapeRenderer::DrawGraduationCap },
                { IconShape::Book, &ShapeRenderer::DrawBook },
                { IconShape::Tire, &ShapeRenderer::DrawTire },
                { IconShape::Snowflake, &ShapeRenderer::DrawSnowflake },
                { IconShape::Newspaper, &ShapeRenderer::DrawNewspaper },
                { IconShape::Car, &ShapeRenderer::DrawCar },
            };

        // connect the rendering function to the shape
        const auto foundShape = shapeMap.find(m_shape);
        m_drawFunction = (foundShape != shapeMap.cend()) ?
            foundShape->second : nullptr;
        }

    //---------------------------------------------------
    void Shape::Draw(const wxRect& drawRect, wxDC& dc) const // cppcheck-suppress constParameter
        {
        // apply any brush, pen, etc. changes if necessary
        if (m_rendererNeedsUpdating)
            { m_renderer.m_graphInfo = GraphItemBase::GetGraphItemInfo(); }
        m_rendererNeedsUpdating = false;

        assert((m_shape == IconShape::Blank || m_drawFunction) &&
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

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for sun icon!");
        if (gc != nullptr)
            {
            // a sun with a sunset (deeper orange) color blended near the bottom
            gc->SetPen(*wxTRANSPARENT_PEN);
            auto sunBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, 0),
                GetXPosFromLeft(rect, 1.5), GetYPosFromTop(rect, 1.5),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::Sunglow)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SunsetOrange)));
            gc->SetBrush(sunBrush);

            const wxRect sunRect = wxRect(rect).Deflate(ScaleToScreenAndCanvas(1));
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
        assert(gc && L"Failed to get graphics context for flower icon!");
        if (gc != nullptr)
            {
            gc->SetPen(wxPen(ColorContrast::Shade(ColorBrewer::GetColor(Color::Wisteria)),
                       ScaleToScreenAndCanvas(1)));
            gc->SetBrush(ApplyParentColorOpacity(ColorBrewer::GetColor(Color::Wisteria)));
            // a line going from the middle of the left side to the middle of the right
            wxRect petalRect(centerPt,
                wxSize(rect.GetWidth()/2 - ScaleToScreenAndCanvas(1), rect.GetHeight()/6));
            petalRect.Offset(wxPoint(0, -(petalRect.GetHeight() / 2)));

            // save current transform matrix state
            gc->PushState();
            // move matrix to center of drawing area
            gc->Translate(centerPt.x, centerPt.y);
            // draw the petals, which will be the horizontal line going across the middle,
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
            gc->SetBrush(ApplyParentColorOpacity(ColorBrewer::GetColor(Color::GoldenYellow)));
            const wxRect flowerRect = wxRect(rect).Deflate(rect.GetWidth()/3);
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
        theLabel.SetFontColor(GetGraphItemInfo().GetFontColor());
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
    void ShapeRenderer::DrawBook(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pcReset(dc, *wxBLACK_PEN);
        wxDCBrushChanger bcReset(dc, *wxBLACK_BRUSH);

        const std::array<wxPoint, 4> bookCover =
            {
            wxPoint(GetXPosFromLeft(rect, 0.1),
                    GetYPosFromTop(rect, math_constants::half)),
            wxPoint(GetXPosFromLeft(rect, 0.6),
                    GetYPosFromTop(rect, .1)),
            wxPoint(GetXPosFromLeft(rect, 0.9),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, 0.4),
                    GetYPosFromTop(rect, math_constants::three_quarters))
            };

        std::array<wxPoint, 4> bookCoverBottom =
            { bookCover };
        const double yOffset = GetYPosFromTop(rect, 0.9) - bookCover[3].y;
        for (auto& pt : bookCoverBottom)
            { pt.y += yOffset; }

        std::array<wxPoint, 4> spine =
            { bookCover[0], bookCover[1], bookCoverBottom[1], bookCoverBottom[0] };

        // the pages
        auto frontOfPagesTopLeft = geometry::point_along_line(
            std::make_pair(bookCover[0].x, bookCover[0].y),
            std::make_pair(bookCover[3].x, bookCover[3].y),
            .1);
        auto frontOfPagesTopRight = geometry::point_along_line(
            std::make_pair(bookCover[0].x, bookCover[0].y),
            std::make_pair(bookCover[3].x, bookCover[3].y),
            .95);
        auto frontOfPagesBottomLeft = geometry::point_along_line(
            std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
            std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y),
            .1);
        auto frontOfPagesBottomRight = geometry::point_along_line(
            std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
            std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y),
            .95);
        std::array<wxPoint, 4> pagesFront =
            {
            wxPoint(frontOfPagesTopLeft.first, frontOfPagesTopLeft.second),
            wxPoint(frontOfPagesTopRight.first, frontOfPagesTopRight.second),
            wxPoint(frontOfPagesBottomRight.first, frontOfPagesBottomRight.second),
            wxPoint(frontOfPagesBottomLeft.first, frontOfPagesBottomLeft.second),
            };

        auto sideOfPagesTopRight = geometry::point_along_line(
            std::make_pair(bookCover[1].x, bookCover[1].y),
            std::make_pair(bookCover[2].x, bookCover[2].y),
            .95);
        auto sideOfPagesBottomRight = geometry::point_along_line(
            std::make_pair(bookCoverBottom[1].x, bookCoverBottom[1].y),
            std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
            .95);
        std::array<wxPoint, 4> pagesSide =
            {
            pagesFront[1],
            wxPoint(sideOfPagesTopRight.first, sideOfPagesTopRight.second),
            wxPoint(sideOfPagesBottomRight.first, sideOfPagesBottomRight.second),
            pagesFront[2]
            };

        wxPen scaledPenMain(*wxRED, ScaleToScreenAndCanvas(1));
        scaledPenMain.SetCap(wxPenCap::wxCAP_BUTT);
        DCPenChangerIfDifferent pcMain(dc, scaledPenMain);

        // draw the bottom of the book
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            DCPenChangerIfDifferent pc(dc, scaledPen);
            DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(bookCoverBottom.size(), &bookCoverBottom[0]); });
            // a highlight along the bottom edge
            scaledPen.SetColour(ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), .4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            DCPenChangerIfDifferent pc2(dc, scaledPen);
            dc.DrawLine(bookCoverBottom[0], bookCoverBottom[3]);

            // gold trim on edges of book
            scaledPen.SetWidth(ScaleToScreenAndCanvas(1));
            scaledPen.SetColour(ColorBrewer::GetColor(Color::GoldLeaf));
            DCPenChangerIfDifferent pc3(dc, scaledPen);
            auto topCornerLeft = geometry::point_along_line(
                std::make_pair(bookCoverBottom[1].x, bookCoverBottom[1].y),
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
                .9);
            auto topCornerRight = geometry::point_along_line(
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y),
                .1);
            std::array<wxPoint, 3> topLeftGoldLeaf =
                {
                wxPoint(topCornerLeft.first, topCornerLeft.second),
                bookCoverBottom[2],
                wxPoint(topCornerRight.first, topCornerRight.second)
                };
            auto bottomCornerLeft = geometry::point_along_line(
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y),
                .9);
            auto bottomCornerRight = geometry::point_along_line(
                std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y),
                .9);
            std::array<wxPoint, 3> bottomLeftGoldLeaf =
                {
                wxPoint(bottomCornerLeft.first, bottomCornerLeft.second),
                bookCoverBottom[3],
                wxPoint(bottomCornerRight.first, bottomCornerRight.second)
                };
            dc.DrawLines(topLeftGoldLeaf.size(), &topLeftGoldLeaf[0]);
            dc.DrawLines(bottomLeftGoldLeaf.size(), &bottomLeftGoldLeaf[0]);

            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetColour(ColorBrewer::GetColor(Color::Gold));
            DCPenChangerIfDifferent pc4(dc, scaledPen);
            dc.DrawLines(topLeftGoldLeaf.size(), &topLeftGoldLeaf[0]);
            dc.DrawLines(bottomLeftGoldLeaf.size(), &bottomLeftGoldLeaf[0]);
            }

        // draw the spine
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            DCPenChangerIfDifferent pc(dc, scaledPen);
            DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(spine.size(), &spine[0]); });
            // a highlight along the edge
            scaledPen.SetColour(ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), .4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            DCPenChangerIfDifferent pc2(dc, scaledPen);
            dc.DrawLine(spine[0], spine[3]);
            }

        // draw the pages
            {
            DCBrushChangerIfDifferent bc(dc, ApplyParentColorOpacity(ColorBrewer::GetColor(Color::AntiqueWhite)));
            DCPenChangerIfDifferent pc(dc, *wxTRANSPARENT_PEN);
            dc.DrawPolygon(pagesFront.size(), &pagesFront[0]);
            }

            {
            DCBrushChangerIfDifferent bc(dc, ApplyParentColorOpacity(ColorBrewer::GetColor(Color::LightGray)));
            DCPenChangerIfDifferent pc(dc, *wxTRANSPARENT_PEN);
            dc.DrawPolygon(pagesSide.size(), &pagesSide[0]);
            }

        // draw the cover
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            DCPenChangerIfDifferent pc(dc, scaledPen);
            DrawWithBaseColorAndBrush(dc, [&]() { dc.DrawPolygon(bookCover.size(), &bookCover[0]); });
            // a highlight along the bottom edge
            scaledPen.SetColour(ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), .4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            DCPenChangerIfDifferent pc2(dc, scaledPen);
            dc.DrawLine(bookCover[0], bookCover[3]);

            // gold leaf on cover of book
                {
                std::array<std::pair<double, double>, 4> goldLeafPoints =
                    {
                    std::make_pair(bookCover[0].x, bookCover[0].y),
                    std::make_pair(bookCover[1].x, bookCover[1].y),
                    std::make_pair(bookCover[2].x, bookCover[2].y),
                    std::make_pair(bookCover[3].x, bookCover[3].y)
                    };
                geometry::deflate_rect(goldLeafPoints[0], goldLeafPoints[1],
                                       goldLeafPoints[2], goldLeafPoints[3], .8);
                std::array<wxPoint, 5> goldLeafPointsPt =
                    {
                    wxPoint(goldLeafPoints[0].first, goldLeafPoints[0].second),
                    wxPoint(goldLeafPoints[1].first, goldLeafPoints[1].second),
                    wxPoint(goldLeafPoints[2].first, goldLeafPoints[2].second),
                    wxPoint(goldLeafPoints[3].first, goldLeafPoints[3].second),
                    wxPoint(goldLeafPoints[0].first, goldLeafPoints[0].second)
                    };
                scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
                scaledPen.SetColour(ColorBrewer::GetColor(Color::GoldLeaf));
                DCPenChangerIfDifferent pc3(dc, scaledPen);
                dc.DrawLines(goldLeafPointsPt.size(), &goldLeafPointsPt[0]);
                }

            // gold trim on edges of book
            scaledPen.SetWidth(ScaleToScreenAndCanvas(1));
            scaledPen.SetColour(ColorBrewer::GetColor(Color::GoldLeaf));
            DCPenChangerIfDifferent pc3(dc, scaledPen);
            auto topCornerLeft = geometry::point_along_line(
                std::make_pair(bookCover[1].x, bookCover[1].y),
                std::make_pair(bookCover[2].x, bookCover[2].y),
                .9);
            auto topCornerRight = geometry::point_along_line(
                std::make_pair(bookCover[2].x, bookCover[2].y),
                std::make_pair(bookCover[3].x, bookCover[3].y),
                .1);
            std::array<wxPoint, 3> topLeftGoldLeaf =
                {
                wxPoint(topCornerLeft.first, topCornerLeft.second),
                bookCover[2],
                wxPoint(topCornerRight.first, topCornerRight.second)
                };
            auto bottomCornerLeft = geometry::point_along_line(
                std::make_pair(bookCover[2].x, bookCover[2].y),
                std::make_pair(bookCover[3].x, bookCover[3].y),
                .9);
            auto bottomCornerRight = geometry::point_along_line(
                std::make_pair(bookCover[0].x, bookCover[0].y),
                std::make_pair(bookCover[3].x, bookCover[3].y),
                .9);
            std::array<wxPoint, 3> bottomLeftGoldLeaf =
                {
                wxPoint(bottomCornerLeft.first, bottomCornerLeft.second),
                bookCover[3],
                wxPoint(bottomCornerRight.first, bottomCornerRight.second)
                };
            dc.DrawLines(topLeftGoldLeaf.size(), &topLeftGoldLeaf[0]);
            dc.DrawLines(bottomLeftGoldLeaf.size(), &bottomLeftGoldLeaf[0]);

            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::quarter));
            scaledPen.SetColour(ColorBrewer::GetColor(Color::Gold));
            DCPenChangerIfDifferent pc4(dc, scaledPen);
            dc.DrawLines(topLeftGoldLeaf.size(), &topLeftGoldLeaf[0]);
            dc.DrawLines(bottomLeftGoldLeaf.size(), &bottomLeftGoldLeaf[0]);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGraduationCap(const wxRect rect, wxDC& dc) const
        {
        if (rect.GetWidth() == rect.GetHeight())
            { SetYOffsetPercentage(0.05); }

        wxPen scaledPen(ColorBrewer::GetColor(Colors::Color::DarkGray),
            std::min(1.0, ScaleToScreenAndCanvas(math_constants::half)));
        DCPenChangerIfDifferent pc(dc, scaledPen);

        const std::array<wxPoint, 4> hatTop =
            {
            wxPoint(GetXPosFromLeft(rect, 0),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, 0)),
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::two_thirds))
            };

        const auto hatTopLeftMidPoint = geometry::point_along_line(
            std::make_pair(hatTop[0].x, hatTop[0].y),
            std::make_pair(hatTop[3].x, hatTop[3].y), math_constants::third);
        const auto hatTopRightMidPoint = geometry::point_along_line(
            std::make_pair(hatTop[3].x, hatTop[3].y),
            std::make_pair(hatTop[2].x, hatTop[2].y), math_constants::two_thirds);

        const std::array<wxPoint, 6> hatStem =
            {
            wxPoint(hatTopLeftMidPoint.first, hatTopLeftMidPoint.second),
            hatTop[3],
            wxPoint(hatTopRightMidPoint.first, hatTopRightMidPoint.second),
            wxPoint(hatTopRightMidPoint.first, GetYPosFromTop(rect, math_constants::three_fourths - .1)),
            wxPoint(hatTop[3].x,
                    GetYPosFromTop(rect, math_constants::full - .1)),
            wxPoint(hatTopLeftMidPoint.first, GetYPosFromTop(rect, math_constants::three_fourths - .1)),
            };

            {
            wxBrush shadowedBrush(ApplyParentColorOpacity(*wxBLACK));
            DCBrushChangerIfDifferent bc(dc, shadowedBrush);
            dc.DrawPolygon(hatStem.size(), &hatStem[0]);
            }

            {
            DCBrushChangerIfDifferent bc(dc,
                ApplyParentColorOpacity(ColorBrewer::GetColor(Colors::Color::SmokyBlack)));
            dc.DrawPolygon(hatTop.size(), &hatTop[0]);
            }

        scaledPen.SetColour(ApplyParentColorOpacity(ColorBrewer::GetColor(Colors::Color::HarvestGold)));
        DCPenChangerIfDifferent pc2(dc, scaledPen);
        DCBrushChangerIfDifferent bc(dc,
            ApplyParentColorOpacity(ColorBrewer::GetColor(Colors::Color::HarvestGold)));
        wxPoint hatTopMidPoint(GetXPosFromLeft(rect, math_constants::half),
                               GetYPosFromTop(rect, math_constants::third));

        // button holding the thread to the top of the hat
        const auto threadWidth{ std::ceil(safe_divide<double>(rect.GetWidth(), 32)) };
        dc.DrawEllipticArc(
            hatTopMidPoint - wxPoint((threadWidth*1.5), (threadWidth*1.5)),
            wxSize((threadWidth*3), (threadWidth*3)), 0, 180);
        dc.DrawEllipticArc(
            hatTopMidPoint - wxPoint((threadWidth*1.5), (threadWidth)),
            wxSize((threadWidth*3), (threadWidth*1.5)), 180, 360);

        // thread dangling over the hat
        scaledPen.SetWidth(threadWidth);
        DCPenChangerIfDifferent pc3(dc, scaledPen);
        dc.DrawLine(hatTopMidPoint,
            wxPoint(GetXPosFromLeft(rect, .98),
                    GetYPosFromTop(rect, math_constants::third)));
        dc.DrawLine(
            wxPoint(GetXPosFromLeft(rect, .98),
                GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, .98),
                GetYPosFromTop(rect, math_constants::two_thirds)));

        // tassel
        const std::array<wxPoint, 3> tassel =
            {
            wxPoint(GetXPosFromLeft(rect, .98),
                    GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, .99),
                    GetYPosFromTop(rect, math_constants::two_thirds + .1)),
            wxPoint(GetXPosFromLeft(rect, .94),
                    GetYPosFromTop(rect, math_constants::two_thirds + .1))
            };
        dc.DrawPolygon(tassel.size(), &tassel[0]);

        scaledPen.SetColour(
            ApplyParentColorOpacity(ColorContrast::Shade(ColorBrewer::GetColor(Colors::Color::Silver))));
        scaledPen.SetCap(wxPenCap::wxCAP_BUTT);
        scaledPen.SetWidth(scaledPen.GetWidth() + ScaleToScreenAndCanvas(1.5));
        DCPenChangerIfDifferent pc4(dc, scaledPen);
        dc.DrawLine(
            wxPoint(GetXPosFromLeft(rect, .98),
                GetYPosFromTop(rect, math_constants::two_thirds - .05)),
            wxPoint(GetXPosFromLeft(rect, .98),
                GetYPosFromTop(rect, math_constants::two_thirds)));

        SetYOffsetPercentage(0.0);
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
            { pt.y -= ScaleToScreenAndCanvas(2 + (iconRadius / 2)); });
        dc.DrawLines(points.size(), &points[0]);

        std::for_each(points.begin(), points.end(),
            [this](auto& pt)
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
            [this](auto& pt)
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
        assert(gc && L"Failed to get graphics context for geo marker!");
        if (gc != nullptr)
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
                GetXPosFromLeft(dcRect, -math_constants::three_quarters),
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
        wxDCBrushChanger bc(dc, ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SchoolBusYellow)));

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
            const auto signPostWidth = std::min<int>(ScaleToScreenAndCanvas(3),
                                                     (rect.GetWidth() / 15));
            // dark gray outline of sign post
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(
                        ColorBrewer::GetColor(Colors::Color::DarkGray),
                            signPostWidth + ScaleToScreenAndCanvas(1)).
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
        wxDCBrushChanger bc(dc, ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SchoolBusYellow)));

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
            const auto signPostWidth = std::min<int>(ScaleToScreenAndCanvas(3),
                                                     (rect.GetWidth() / 15));
            // dark gray outline of sign post used to contrast black sign post
            // against a possibly dark background
                {
                wxDCPenChanger pc(dc,
                    wxPen(wxPenInfo(
                            ColorBrewer::GetColor(Colors::Color::DarkGray),
                                signPostWidth + ScaleToScreenAndCanvas(1)).
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
    void ShapeRenderer::DrawCar(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        wxRect dcRect(rect);
        dcRect.Deflate(
            GetGraphItemInfo().GetPen().IsOk() ?
            ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
            0);
        // adjust to center it horizontally inside of square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedHeight{ dcRect.GetHeight() * math_constants::three_quarters };
            const auto adjustTop{ (dcRect.GetHeight() - adjustedHeight) * math_constants::half };
            dcRect.SetHeight(adjustedHeight);
            dcRect.Offset(wxPoint(0, adjustTop));
            }

        GraphicsContextFallback gcf{ &dc, dcRect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for car icon!");
        if (gc != nullptr)
            {
            const wxPen outlinePen(wxTransparentColour, ScaleToScreenAndCanvas(1));

            const auto bodyBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(dcRect, -math_constants::full), GetYPosFromTop(dcRect, math_constants::half),
                GetXPosFromLeft(dcRect, math_constants::full), GetYPosFromTop(dcRect, math_constants::half),
                ApplyParentColorOpacity(ColorContrast::ShadeOrTint(ColorBrewer::GetColor(Color::SmokyBlack))),
                ApplyParentColorOpacity(GetGraphItemInfo().GetBrush().GetColour()));
            gc->SetPen(outlinePen);
            gc->SetBrush(bodyBrush);
            // body of car
            wxRect bodyRect{ dcRect };
            bodyRect.Deflate(ScaleToScreenAndCanvas(1));
            bodyRect.SetHeight(bodyRect.GetHeight() * .35);
            bodyRect.Offset(0, dcRect.GetHeight() - (bodyRect.GetHeight() * 1.5));
            // lower half (bumper area)
            wxRect lowerBodyRect{ bodyRect };
            lowerBodyRect.SetTop(lowerBodyRect.GetTop() + (lowerBodyRect.GetHeight() / 2));
            lowerBodyRect.SetHeight(lowerBodyRect.GetHeight() / 2);

            // upper half (headlights area)
            // (this is drawn later, after the top area of the car so that it
            //  covers up any seams)
            const double backBumperOffset = bodyRect.GetWidth() * 0.025;
            wxRect upperBodyRect{ bodyRect };
            upperBodyRect.SetWidth(upperBodyRect.GetWidth() * 0.95);
            upperBodyRect.Offset(wxPoint(backBumperOffset, 0));

            // top of car
            wxRect carTopRect{ bodyRect };
            carTopRect.SetWidth(carTopRect.GetWidth() * 0.65);
            carTopRect.SetTop(bodyRect.GetTop() - carTopRect.GetHeight() + ScaleToScreenAndCanvas(2));
            carTopRect.SetHeight(carTopRect.GetHeight() + ScaleToScreenAndCanvas(2));
            carTopRect.Offset(wxPoint(backBumperOffset, 0));
            gc->DrawRoundedRectangle(
                carTopRect.GetX(), carTopRect.GetY(), carTopRect.GetWidth(), carTopRect.GetHeight(),
                ScaleToScreenAndCanvas(2));

            // windshield
            std::array<wxPoint2DDouble, 4> windshieldSection =
                {
                carTopRect.GetTopRight(),
                wxPoint(carTopRect.GetRight() +
                    (bodyRect.GetWidth() - carTopRect.GetWidth()) * math_constants::third,
                    carTopRect.GetBottom()),
                wxPoint(carTopRect.GetBottomRight().x - (carTopRect.GetWidth() / 4),
                        carTopRect.GetBottomRight().y),
                wxPoint(carTopRect.GetTopRight().x - (carTopRect.GetWidth() / 4),
                        carTopRect.GetTopRight().y)
                };
            std::array<wxPoint2DDouble, 2> windshield =
                {
                wxPoint(windshieldSection[0].m_x, windshieldSection[0].m_y + ScaleToScreenAndCanvas(1)),
                windshieldSection[1]
                };
            auto windshieldAreaPath = gc->CreatePath();
            windshieldAreaPath.MoveToPoint(windshieldSection[0]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[1]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[2]);
            windshieldAreaPath.AddLineToPoint(windshieldSection[3]);
            gc->DrawPath(windshieldAreaPath);

            gc->SetPen(wxPenInfo(ColorBrewer::GetColor(Colors::Color::DarkGray),
                                 ScaleToScreenAndCanvas(1)).Cap(wxCAP_BUTT));
            auto windshieldPath = gc->CreatePath();
            windshieldPath.MoveToPoint(windshield[0]);
            windshieldPath.AddLineToPoint(windshield[1]);
            gc->StrokePath(windshieldPath);
            gc->SetPen(outlinePen);

            // side windows
            auto windowBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(dcRect, -math_constants::half), GetYPosFromTop(dcRect, math_constants::half),
                GetXPosFromLeft(dcRect, math_constants::three_quarters), GetYPosFromTop(dcRect, math_constants::half),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SmokyBlack)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::DarkGray)));
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
            gc->SetPen(wxPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            auto windowRect{ carTopRect };
            windowRect.SetWidth(windowRect.GetWidth() * 0.4);
            windowRect.Offset(wxPoint(carTopRect.GetWidth() * 0.2, ScaleToScreenAndCanvas(2)));
            gc->StrokeLine(windowRect.GetX(), windowRect.GetY(),
                           windowRect.GetX(), windowRect.GetY() + windowRect.GetHeight());
            gc->StrokeLine(windowRect.GetX() + windowRect.GetWidth(), windowRect.GetY(),
                           windowRect.GetX() + windowRect.GetWidth(), windowRect.GetY() + windowRect.GetHeight());
            gc->SetBrush(bodyBrush);
            gc->SetPen(outlinePen);

            // draw upper body part on top of windshield
            gc->DrawRoundedRectangle(
                upperBodyRect.GetX(), upperBodyRect.GetY(), upperBodyRect.GetWidth(), upperBodyRect.GetHeight(),
                ScaleToScreenAndCanvas(2));

            // headlights
            auto headlightsRect{ upperBodyRect };
            headlightsRect.SetWidth(headlightsRect.GetWidth() * .05);
            headlightsRect.SetHeight(headlightsRect.GetHeight() * .25);
            headlightsRect.Offset(upperBodyRect.GetWidth() - headlightsRect.GetWidth(),
                upperBodyRect.GetHeight() * .25);
            gc->SetBrush(gc->CreateLinearGradientBrush(headlightsRect.GetLeft(), headlightsRect.GetTop() / 2,
                headlightsRect.GetRight(), headlightsRect.GetTop() / 2,
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::OrangeYellow)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::AntiqueWhite))));
            gc->DrawRectangle(headlightsRect.GetX(), headlightsRect.GetY(),
                              headlightsRect.GetWidth(), headlightsRect.GetHeight());
            gc->SetBrush(bodyBrush);

            // draw bumper area now, to overlay any headlight overlap
            gc->DrawRoundedRectangle(
                lowerBodyRect.GetX(), lowerBodyRect.GetY(), lowerBodyRect.GetWidth(), lowerBodyRect.GetHeight(),
                ScaleToScreenAndCanvas(2));

            // the tires
            wxRect tireRect{ dcRect };
            tireRect.SetWidth(dcRect.GetWidth() * .25);
            tireRect.SetHeight(tireRect.GetWidth());
            tireRect.SetTop(dcRect.GetTop() + (dcRect.GetHeight() - tireRect.GetHeight()));
            tireRect.SetLeft(dcRect.GetLeft() + dcRect.GetWidth() * math_constants::tenth);

            DrawTire(tireRect, gc);

            tireRect.SetLeft((dcRect.GetRight() - tireRect.GetWidth()) -
                             dcRect.GetWidth() * math_constants::tenth);
            DrawTire(tireRect, gc);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTire(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for tire icon!");
        DrawTire(rect, gc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawTire(wxRect rect, wxGraphicsContext* gc) const
        {
        if (gc != nullptr)
            {
            wxPen scaledPen(ColorBrewer::GetColor(Colors::Color::DarkGray),
                            std::min(1.0, ScaleToScreenAndCanvas(math_constants::half)));
            gc->SetPen(scaledPen);
            // the tire
            const wxRect tireRect = wxRect(rect).Deflate(ScaleToScreenAndCanvas(1));
            auto tireBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(tireRect, 0), GetYPosFromTop(tireRect, 0),
                GetXPosFromLeft(tireRect, 1.5), GetYPosFromTop(tireRect, 1.5),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SmokyBlack)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::DarkGray)));
            gc->SetBrush(tireBrush);

            gc->DrawEllipse(tireRect.GetTopLeft().x, tireRect.GetTopLeft().y,
                tireRect.GetWidth(), tireRect.GetHeight());

            // hubcap
            wxRect hubCapRect = wxRect(rect).Deflate(rect.GetWidth() * math_constants::quarter);
            auto hubCapBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(hubCapRect, 0), GetYPosFromTop(hubCapRect, 0),
                GetXPosFromLeft(hubCapRect, 1.5), GetYPosFromTop(hubCapRect, 1.5),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::Silver)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::CoolGrey)));
            gc->SetBrush(hubCapBrush);

            gc->DrawEllipse(hubCapRect.GetTopLeft().x, hubCapRect.GetTopLeft().y,
                hubCapRect.GetWidth(), hubCapRect.GetHeight());

            hubCapRect.Deflate(hubCapRect.GetWidth() * math_constants::eighth);
            wxPen blackPen(*wxBLACK, ScaleToScreenAndCanvas(1));
            gc->SetPen(blackPen);

            DrawAsterisk(hubCapRect, gc);
            }
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

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for asterisk icon!");

        wxPen scaledPen = GetGraphItemInfo().GetPen();
        if (scaledPen.IsOk() && gc != nullptr)
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(std::max(scaledPen.GetWidth(), 2)) );
            gc->SetPen(scaledPen);
            }

        DrawAsterisk(rect, gc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawAsterisk(wxRect rect, wxGraphicsContext* gc) const
        {
        if (gc != nullptr)
            {
            const auto centerPt = rect.GetTopLeft() +
                wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

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
    void ShapeRenderer::DrawNewspaper(const wxRect rect, wxDC& dc) const
        {
        wxDCPenChanger pc(dc,
            wxPen(ColorBrewer::GetColor(Colors::Color::DarkGray), ScaleToScreenAndCanvas(1)));
        wxDCBrushChanger bc(dc, *wxWHITE_BRUSH);

        wxRect frontPageRect{ rect };
        frontPageRect.Deflate(frontPageRect.GetSize() * .1);

        auto backPage{ frontPageRect };
        backPage.SetWidth(frontPageRect.GetWidth() * 1.1);
        backPage.SetHeight(frontPageRect.GetHeight() * math_constants::three_fourths);
        backPage.Offset(0, frontPageRect.GetHeight() - backPage.GetHeight());

        // draw the lower (folded) section of the backpage
            {
            auto bottomRect{ backPage };
            bottomRect.SetHeight(bottomRect.GetHeight() * math_constants::half);
            bottomRect.Offset(0, backPage.GetHeight() - bottomRect.GetHeight());
            wxDCClipper clip(dc, bottomRect);
            dc.DrawRoundedRectangle(backPage, ScaleToScreenAndCanvas(3));
            }
        // draw the upper half of the backpage
            {
            auto topRect{ backPage };
            topRect.SetHeight(topRect.GetHeight() *
                // avoid a gap in the lines
                (math_constants::half + .05));
            wxDCClipper clip(dc, topRect);
            dc.DrawRectangle(backPage);
            }
        // draw the font page
            {
            auto topRect{ frontPageRect };
            topRect.SetHeight(topRect.GetHeight() * .9);
            wxDCClipper clip(dc, topRect);
            dc.DrawRectangle(frontPageRect);
            }

        // headline
        wxDCPenChanger pc2(dc,
            wxPen(ColorBrewer::GetColor(Colors::Color::WarmGray), ScaleToScreenAndCanvas(math_constants::half)));
        auto headlineBox{ frontPageRect };
        headlineBox.SetHeight(headlineBox.GetHeight() * math_constants::third);
        headlineBox.Deflate(ScaleToScreenAndCanvas(2));
        Label headline(GraphItemInfo(_("DAYTON TIMES")).
            DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
            Pen(wxNullPen));
        headline.SetBoundingBox(headlineBox, dc, GetScaling());
        headline.Draw(dc);
        headlineBox.Offset(wxPoint(0, ScaleToScreenAndCanvas(1)));
        dc.DrawLine(headlineBox.GetBottomLeft(), headlineBox.GetBottomRight());

        // picture on the front page
        auto pictureBox{ frontPageRect };
        pictureBox.SetHeight(frontPageRect.GetHeight() * math_constants::third);
        pictureBox.SetWidth(frontPageRect.GetWidth() * math_constants::fourth);
        pictureBox.SetTop(headlineBox.GetBottom() + ScaleToScreenAndCanvas(1));
        pictureBox.Offset(wxPoint(ScaleToScreenAndCanvas(2), 0));
        dc.DrawRectangle(pictureBox);
        dc.GradientFillLinear(pictureBox, ColorBrewer::GetColor(Colors::Color::Afternoon),
            ColorBrewer::GetColor(Colors::Color::BlueSky));
        wxRect sunRect{ pictureBox };
        sunRect.SetWidth(sunRect.GetWidth() * math_constants::half);
        sunRect.SetHeight(sunRect.GetWidth());
        DrawSun(sunRect, dc);

        // column separator
        wxPoint columnTop(pictureBox.GetTopRight());
        columnTop.x += ScaleToScreenAndCanvas(1);
        wxPoint columnBottom(columnTop.x, frontPageRect.GetBottom() - ScaleToScreenAndCanvas(2));
        dc.DrawLine(columnTop, columnBottom);

        // text on the right side
        auto rightTextRect{ frontPageRect };
        rightTextRect.SetWidth(frontPageRect.GetRight() - columnTop.x - ScaleToScreenAndCanvas(4));
        headlineBox.Offset(wxPoint(0, ScaleToScreenAndCanvas(1)));
        rightTextRect.SetHeight(frontPageRect.GetBottom() - headlineBox.GetBottom() - ScaleToScreenAndCanvas(4));
        rightTextRect.SetTopLeft(columnTop);
        rightTextRect.Offset(ScaleToScreenAndCanvas(2), ScaleToScreenAndCanvas(2));
        wxPoint textLeft{ rightTextRect.GetTopLeft() };
        wxPoint textRight{ rightTextRect.GetTopRight() };
        // use a random selection of pen dash styles for each line to simulate text
        std::uniform_int_distribution<> randPenStyle(wxPENSTYLE_LONG_DASH, wxPENSTYLE_DOT_DASH);
        size_t currentLine{ 0 };
        while (textLeft.y < rightTextRect.GetBottom())
            {
            wxDCPenChanger pc3(dc,
                wxPen(ColorBrewer::GetColor(Colors::Color::SmokyBlack),
                    ScaleToScreenAndCanvas(rect.GetWidth() <= ScaleToScreenAndCanvas(16) ? 1 : .5),
                    static_cast<wxPenStyle>(randPenStyle(m_mt))));
            if ((currentLine % 10) > 0)
                { dc.DrawLine(textLeft, textRight); }
            // indent every 10th line
            else
                {
                dc.DrawLine(
                    wxPoint(textLeft.x + rightTextRect.GetWidth() * math_constants::fifth, textLeft.y),
                    textRight);
                }
            textLeft.y += ScaleToScreenAndCanvas(2);
            textRight.y += ScaleToScreenAndCanvas(2);
            ++currentLine;
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawFallLeaf(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for leaf icon!");
        if (gc != nullptr)
            {
            // draw the leaf
            gc->SetPen(*wxTRANSPARENT_PEN);
            auto leafBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, math_constants::half),
                GetXPosFromLeft(rect, math_constants::three_fourths), GetYPosFromTop(rect, math_constants::half),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::ChineseRed)),
                ApplyParentColorOpacity(ColorBrewer::GetColor(Color::SunsetOrange)));
            gc->SetBrush(leafBrush);

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

            const auto stemWidth = rect.GetWidth() <= ScaleToScreenAndCanvas(32) ? 1 : 2;
            gc->SetPen(wxPen(ColorBrewer::GetColor(Color::DarkBrown),
                       ScaleToScreenAndCanvas(stemWidth)));

            // draw the stem
            auto stemPath = gc->CreatePath();
            // start at the top middle top middle
            stemPath.MoveToPoint(GetXPosFromLeft(rect, math_constants::half),
                GetYPosFromTop(rect, .025));
            // draw to the bottom middle of leaf
            stemPath.AddLineToPoint(GetXPosFromLeft(rect, math_constants::half),
                                    GetYPosFromTop(rect, math_constants::three_quarters));
            // draw a curled stem at the end of the leaf
            stemPath.AddQuadCurveToPoint(
                GetXPosFromLeft(rect, math_constants::half),
                GetYPosFromTop(rect, math_constants::three_quarters +
                    (math_constants::quarter * math_constants::half)),
                GetXPosFromLeft(rect, .4),
                GetYPosFromTop(rect, math_constants::full - .025));

            gc->StrokePath(stemPath);
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawSnowflake(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for leaf icon!");
        if (gc != nullptr)
            {
            const wxPen bodyPen(ColorBrewer::GetColor(Colors::Color::Ice),
                                ScaleToScreenAndCanvas(2));
            const wxPen leafPen(ColorBrewer::GetColor(Colors::Color::Ice),
                                ScaleToScreenAndCanvas(1));

            const auto centerPt = rect.GetTopLeft() +
                wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

            constexpr auto penCapWiggleRoom = 0.05;

            // a line going from the middle of the left side to the middle of the right
            const std::array<wxPoint2DDouble, 2> points =
                {
                wxPoint(GetXPosFromLeft(rect, penCapWiggleRoom),
                        GetYPosFromTop(rect, math_constants::half)),
                wxPoint(GetXPosFromLeft(rect, math_constants::full - penCapWiggleRoom),
                        GetYPosFromTop(rect, math_constants::half))
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
                gc->SetPen(bodyPen);
                gc->StrokeLine(points[0].m_x - centerPt.x, points[0].m_y - centerPt.y,
                               points[1].m_x - centerPt.x, points[1].m_y - centerPt.y);
                // outer leaf branch
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths + math_constants::twentieth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - (penCapWiggleRoom * 2)) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half - math_constants::tenth) - centerPt.y);
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths + math_constants::twentieth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full - (penCapWiggleRoom * 2)) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half + math_constants::tenth) - centerPt.y);
                // inner leaf branch
                gc->SetPen(leafPen);
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths - math_constants::tenth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full -
                        penCapWiggleRoom - math_constants::fifth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half - math_constants::tenth) - centerPt.y);
                gc->StrokeLine(
                    GetXPosFromLeft(rect, math_constants::three_fourths - math_constants::tenth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half) - centerPt.y,
                    GetXPosFromLeft(rect, math_constants::full -
                        penCapWiggleRoom - math_constants::fifth) - centerPt.x,
                    GetYPosFromTop(rect, math_constants::half + math_constants::tenth) - centerPt.y);
                angle += 45;
                }
            // restore transform matrix
            gc->PopState();
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
        assert(gc && L"Failed to get graphics context for water color effect!");
        if (gc != nullptr)
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
        assert(GetGraphItemInfo().GetPen().IsOk() &&
               L"Pen should be set in Shape for curly braces!");
        // just to reset when we are done
        wxDCPenChanger pc(dc, *wxBLACK_PEN);

        wxRect drawRect(rect);

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for curly braces!");
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

        wxRect dcRect(rect);
        // adjust to center it horizontally inside of square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ dcRect.GetWidth() * .6 };
            const auto adjustLeft{ (dcRect.GetWidth() - adjustedWidth) * math_constants::half };
            dcRect.SetWidth(adjustedWidth);
            dcRect.Offset(wxPoint(adjustLeft, 0));
            }

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for male outline!");
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
        // adjust to center it horizontally inside of square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ dcRect.GetWidth() * .6 };
            const auto adjustLeft{ (dcRect.GetWidth() - adjustedWidth) * math_constants::half };
            dcRect.SetWidth(adjustedWidth);
            dcRect.Offset(wxPoint(adjustLeft, 0));
            }

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for female outline!");
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
        // adjust to center it horizontally inside of square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ dcRect.GetWidth() * .6 };
            const auto adjustLeft{ (dcRect.GetWidth() - adjustedWidth) * math_constants::half };
            dcRect.SetWidth(adjustedWidth);
            dcRect.Offset(wxPoint(adjustLeft, 0));
            }

        GraphicsContextFallback gcf{ &dc, rect };
        auto gc = gcf.GetGraphicsContext();
        assert(gc && L"Failed to get graphics context for female outline!");
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
