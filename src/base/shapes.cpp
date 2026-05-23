///////////////////////////////////////////////////////////////////////////////
// Name:        shapes.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "shapes.h"
#include "image.h"
#include "label.h"
#include "lines.h"
#include <wx/dcgraph.h>
#include <wx/graphics.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::GraphItems::Shape, Wisteria::GraphItems::GraphItemBase);

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    GraphicsContextFallback::GraphicsContextFallback(wxDC* dc, const wxRect rect)
        {
        wxASSERT_MSG(dc, L"Invalid DC for graphics context!");
        if (dc == nullptr)
            {
            return;
            }
        m_rect = rect;
        m_dc = dc;
        m_gc = m_dc->GetGraphicsContext();
        // DC doesn't support GetGraphicsContext(), so fallback to
        // drawing to a bitmap that we will blit later
        if (m_gc == nullptr)
            {
            m_drawingToBitmap = true;
            m_bmp = wxBitmap(m_rect.GetSize(), 32);
            Image::SetOpacity(m_bmp, wxALPHA_TRANSPARENT, false);
            m_memDC.SelectObject(m_bmp);
            m_memDC.SetDeviceOrigin(-m_rect.x, -m_rect.y);

            m_gc = wxGraphicsContext::Create(m_memDC);
            }
        wxASSERT_MSG(m_gc, L"Failed to get graphics context!");
        }

    //---------------------------------------------------
    GraphicsContextFallback::~GraphicsContextFallback()
        {
        if (m_gc == nullptr)
            {
            return;
            }
        // flush drawing commands to bitmap and then blit it
        // onto the original DC
        if (m_drawingToBitmap)
            {
            delete m_gc;
            m_gc = nullptr;
            m_memDC.SelectObject(wxNullBitmap);
            m_dc->DrawBitmap(m_bmp, m_rect.GetTopLeft());
            }
        else
            {
            m_gc->Flush();
            }
        }

    //---------------------------------------------------
    void Shape::SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                               [[maybe_unused]] const double parentScaling)
        {
        m_sizeDIPs.x = (IsFixedWidthOnCanvas() ?
                            std::min<int>(m_shapeSizeDIPs.GetWidth(),
                                          DownscaleFromScreenAndCanvas(rect.GetSize().GetWidth())) :
                            DownscaleFromScreenAndCanvas(rect.GetSize().GetWidth()));
        m_sizeDIPs.y = DownscaleFromScreenAndCanvas(rect.GetSize().GetHeight());

        if (GetAnchoring() == Anchoring::TopLeftCorner)
            {
            SetAnchorPoint(rect.GetTopLeft());
            }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            {
            SetAnchorPoint(rect.GetBottomLeft());
            }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            {
            SetAnchorPoint(rect.GetTopRight());
            }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            {
            SetAnchorPoint(rect.GetBottomRight());
            }
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
            {
            dc.SetClippingRegion(GetClippingRect().value());
            }

        const auto bBox = GetBoundingBox(dc);
        auto drawRect = wxRect(ScaleToScreenAndCanvas(m_shapeSizeDIPs));
        // keep drawing area inside the full area, maintaining aspect ratio
        if (drawRect.GetWidth() > bBox.GetWidth() || drawRect.GetHeight() > bBox.GetHeight())
            {
            const double scale =
                std::min(safe_divide<double>(bBox.GetWidth(), drawRect.GetWidth()),
                         safe_divide<double>(bBox.GetHeight(), drawRect.GetHeight()));
            drawRect.SetSize(drawRect.GetSize() * scale);
            }

        // position the shape inside its (possibly) larger box
        wxPoint shapeTopLeftCorner(bBox.GetLeftTop());
        // horizontal page alignment
        if (GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned)
            { /*noop*/
            }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered)
            {
            shapeTopLeftCorner.x += safe_divide<double>(bBox.GetWidth(), 2) -
                                    safe_divide<double>(drawRect.GetWidth(), 2);
            }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned)
            {
            shapeTopLeftCorner.x += bBox.GetWidth() - drawRect.GetWidth();
            }
        // vertical page alignment
        if (GetPageVerticalAlignment() == PageVerticalAlignment::TopAligned)
            { /*noop*/
            }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::Centered)
            {
            shapeTopLeftCorner.y += safe_divide<double>(bBox.GetHeight(), 2) -
                                    safe_divide<double>(drawRect.GetHeight(), 2);
            }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned)
            {
            shapeTopLeftCorner.y += bBox.GetHeight() - drawRect.GetHeight();
            }

        drawRect.SetTopLeft(shapeTopLeftCorner);

        Draw(drawRect, dc);

        // draw the outline
        if (IsSelected())
            {
            const wxDCBrushChanger bc{ dc, wxColour{ 0, 0, 0, 0 } };
            const wxDCPenChanger pc{ dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                               ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT) };
            dc.DrawRectangle(bBox);
            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                const wxDCPenChanger pcDebug{
                    dc, wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Red),
                              ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT)
                };
                dc.DrawRectangle(drawRect);
                }
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }

        return bBox;
        }

    //---------------------------------------------------
    Shape::Shape(const GraphItemInfo& itemInfo, const Icons::IconShape shape, const wxSize sz,
                 const std::shared_ptr<wxBitmapBundle>& img /*= nullptr*/)
        : GraphItemBase(itemInfo), m_shapeSizeDIPs(sz), m_sizeDIPs(sz), m_shape(shape),
          m_renderer(itemInfo, img)
        {
        static const std::map<Icons::IconShape, ShapeRenderer::DrawFunction> shapeMap = {
            { Icons::IconShape::Blank, nullptr },
            { Icons::IconShape::ArrowRight, &ShapeRenderer::DrawRightArrow },
            { Icons::IconShape::HorizontalLine, &ShapeRenderer::DrawHorizontalLine },
            { Icons::IconShape::VerticalLine, &ShapeRenderer::DrawVerticalLine },
            { Icons::IconShape::CrossedOut, &ShapeRenderer::DrawCrossedOut },
            { Icons::IconShape::Circle, &ShapeRenderer::DrawCircle },
            { Icons::IconShape::Square, &ShapeRenderer::DrawSquare },
            { Icons::IconShape::Asterisk, &ShapeRenderer::DrawAsterisk },
            { Icons::IconShape::Plus, &ShapeRenderer::DrawPlus },
            { Icons::IconShape::TriangleUpward, &ShapeRenderer::DrawUpwardTriangle },
            { Icons::IconShape::TriangleDownward, &ShapeRenderer::DrawDownwardTriangle },
            { Icons::IconShape::TriangleRight, &ShapeRenderer::DrawRightTriangle },
            { Icons::IconShape::TriangleLeft, &ShapeRenderer::DrawLeftTriangle },
            { Icons::IconShape::Diamond, &ShapeRenderer::DrawDiamond },
            { Icons::IconShape::Hexagon, &ShapeRenderer::DrawHexagon },
            { Icons::IconShape::BoxPlot, &ShapeRenderer::DrawBoxPlot },
            { Icons::IconShape::Sun, &ShapeRenderer::DrawSun },
            { Icons::IconShape::Flower, &ShapeRenderer::DrawFlower },
            { Icons::IconShape::Sunflower, &ShapeRenderer::DrawSunFlower },
            { Icons::IconShape::FallLeaf, &ShapeRenderer::DrawFallLeaf },
            { Icons::IconShape::WarningRoadSign, &ShapeRenderer::DrawWarningRoadSign },
            { Icons::IconShape::LocationMarker, &ShapeRenderer::DrawGeoMarker },
            { Icons::IconShape::GoRoadSign, &ShapeRenderer::DrawGoSign },
            { Icons::IconShape::Image, &ShapeRenderer::DrawImage },
            { Icons::IconShape::LeftCurlyBrace, &ShapeRenderer::DrawLeftCurlyBrace },
            { Icons::IconShape::RightCurlyBrace, &ShapeRenderer::DrawRightCurlyBrace },
            { Icons::IconShape::TopCurlyBrace, &ShapeRenderer::DrawTopCurlyBrace },
            { Icons::IconShape::BottomCurlyBrace, &ShapeRenderer::DrawBottomCurlyBrace },
            { Icons::IconShape::Man, &ShapeRenderer::DrawMan },
            { Icons::IconShape::Woman, &ShapeRenderer::DrawWoman },
            { Icons::IconShape::BusinessWoman, &ShapeRenderer::DrawBusinessWoman },
            { Icons::IconShape::ChevronDownward, &ShapeRenderer::DrawChevronDownward },
            { Icons::IconShape::ChevronUpward, &ShapeRenderer::DrawChevronUpward },
            { Icons::IconShape::Text, &ShapeRenderer::DrawText },
            { Icons::IconShape::Tack, &ShapeRenderer::DrawTack },
            { Icons::IconShape::Banner, &ShapeRenderer::DrawBanner },
            { Icons::IconShape::WaterColorRectangle, &ShapeRenderer::DrawWaterColorRectangle },
            { Icons::IconShape::ThickWaterColorRectangle,
              &ShapeRenderer::DrawThickWaterColorRectangle },
            { Icons::IconShape::MarkerRectangle, &ShapeRenderer::DrawMarkerRectangle },
            { Icons::IconShape::PencilRectangle, &ShapeRenderer::DrawPencilRectangle },
            { Icons::IconShape::GraduationCap, &ShapeRenderer::DrawGraduationCap },
            { Icons::IconShape::Book, &ShapeRenderer::DrawBook },
            { Icons::IconShape::Tire, &ShapeRenderer::DrawTire },
            { Icons::IconShape::Snowflake, &ShapeRenderer::DrawSnowflake },
            { Icons::IconShape::Newspaper, &ShapeRenderer::DrawNewspaper },
            { Icons::IconShape::Car, &ShapeRenderer::DrawCar },
            { Icons::IconShape::Blackboard, &ShapeRenderer::DrawBlackboard },
            { Icons::IconShape::Clock, &ShapeRenderer::DrawClock },
            { Icons::IconShape::Ruler, &ShapeRenderer::DrawRuler },
            { Icons::IconShape::IVBag, &ShapeRenderer::DrawIVBag },
            { Icons::IconShape::ColdThermometer, &ShapeRenderer::DrawColdThermometer },
            { Icons::IconShape::HotThermometer, &ShapeRenderer::DrawHotThermometer },
            { Icons::IconShape::Apple, &ShapeRenderer::DrawRedApple },
            { Icons::IconShape::GrannySmithApple, &ShapeRenderer::DrawGrannySmithApple },
            { Icons::IconShape::Heart, &ShapeRenderer::DrawHeart },
            { Icons::IconShape::ImmaculateHeart, &ShapeRenderer::DrawImmaculateHeart },
            { Icons::IconShape::ImmaculateHeartWithSword,
              &ShapeRenderer::DrawImmaculateHeartWithSword },
            { Icons::IconShape::Flame, &ShapeRenderer::DrawFlame },
            { Icons::IconShape::Office, &ShapeRenderer::DrawOffice },
            { Icons::IconShape::Factory, &ShapeRenderer::DrawFactory },
            { Icons::IconShape::House, &ShapeRenderer::DrawHouse },
            { Icons::IconShape::Barn, &ShapeRenderer::DrawBarn },
            { Icons::IconShape::Farm, &ShapeRenderer::DrawFarm },
            { Icons::IconShape::HundredDollarBill, &ShapeRenderer::DrawHundredDollarBill },
            { Icons::IconShape::Monitor, &ShapeRenderer::DrawMonitor },
            { Icons::IconShape::Sword, &ShapeRenderer::DrawSword },
            { Icons::IconShape::CrescentTop, &ShapeRenderer::DrawCrescentTop },
            { Icons::IconShape::CrescentBottom, &ShapeRenderer::DrawCrescentBottom },
            { Icons::IconShape::CrescentRight, &ShapeRenderer::DrawCrescentRight },
            { Icons::IconShape::CurvingRoad, &ShapeRenderer::DrawCurvingRoad },
            { Icons::IconShape::Pumpkin, &ShapeRenderer::DrawPumpkin },
            { Icons::IconShape::JackOLantern, &ShapeRenderer::DrawJackOLantern },
            { Icons::IconShape::NumberRange, &ShapeRenderer::DrawNumberRange },
            { Icons::IconShape::CheesePizza, &ShapeRenderer::DrawCheesePizza },
            { Icons::IconShape::PepperoniPizza, &ShapeRenderer::DrawPepperoniPizza },
            { Icons::IconShape::HawaiianPizza, &ShapeRenderer::DrawHawaiianPizza },
            { Icons::IconShape::ChocolateChipCookie, &ShapeRenderer::DrawChocolateChipCookie },
            { Icons::IconShape::CoffeeShopCup, &ShapeRenderer::DrawCoffeeShopCup },
            { Icons::IconShape::Pill, &ShapeRenderer::DrawPill },
            { Icons::IconShape::Tractor, &ShapeRenderer::DrawTractor },
            { Icons::IconShape::Butterfly, &ShapeRenderer::DrawButterfly },
            { Icons::IconShape::Star, &ShapeRenderer::DrawStar }
        };

        // connect the rendering function to the shape
        const auto foundShape = shapeMap.find(m_shape);
        m_drawFunction = (foundShape != shapeMap.cend()) ? foundShape->second : nullptr;
        }

    //---------------------------------------------------
    Shape::Shape(const Shape& that)
        : GraphItemBase(that), m_shapeSizeDIPs(that.m_shapeSizeDIPs), m_sizeDIPs(that.m_sizeDIPs),
          m_shape(that.m_shape), m_renderer(that.m_renderer), m_rendererNeedsUpdating(true),
          m_drawFunction(that.m_drawFunction)
        {
        }

    //---------------------------------------------------
    Shape& Shape::operator=(const Shape& that)
        {
        if (this != &that)
            {
            GraphItemBase::operator=(that);
            m_shapeSizeDIPs = that.m_shapeSizeDIPs;
            m_sizeDIPs = that.m_sizeDIPs;
            m_shape = that.m_shape;
            m_renderer = that.m_renderer;
            // force re-sync of renderer's render function on first Draw()
            m_rendererNeedsUpdating = true;
            m_drawFunction = that.m_drawFunction;
            }
        return *this;
        }

    //---------------------------------------------------
    void Shape::Draw(const wxRect& drawRect, wxDC& dc) const // cppcheck-suppress constParameter
        {
        // apply any brush, pen, etc. changes if necessary
        if (m_rendererNeedsUpdating)
            {
            m_renderer.m_graphInfo = GraphItemBase::GetGraphItemInfo();
            }
        m_rendererNeedsUpdating = false;

        wxASSERT_MSG((m_shape == Icons::IconShape::Blank || m_drawFunction),
                     L"Shape failed to set drawing function!");
        if (m_drawFunction != nullptr)
            {
            (m_renderer.*m_drawFunction)(drawRect, dc);
            }
        }

    //---------------------------------------------------
    void Shape::SetAutoAccessibilityAttributes()
        {
        const wxString shapeName =
            ShapeInfo::GetReadableShapeName(m_shape, GetGraphItemInfo().GetText());
        wxString label;
        if (m_shape == Icons::IconShape::NumberRange)
            {
            label = shapeName;
            if (label.length() > 0)
                {
                label[0] = wxToupper(label[0]);
                }
            label += L".";
            }
        else
            {
            label = wxString::Format(_(L"A %s shape."), shapeName);
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }

    //---------------------------------------------------
    wxRect Shape::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        wxRect rect(ScaleToScreenAndCanvas(m_sizeDIPs));
        if (GetAnchoring() == Anchoring::TopLeftCorner)
            {
            rect.SetTopLeft(GetAnchorPoint());
            }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            {
            rect.SetBottomLeft(GetAnchorPoint());
            }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            {
            rect.SetTopRight(GetAnchorPoint());
            }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            {
            rect.SetBottomRight(GetAnchorPoint());
            }
        else if (GetAnchoring() == Anchoring::Center)
            {
            rect.SetTopLeft(GetAnchorPoint());
            rect.Offset(-(rect.GetWidth() / 2), -(rect.GetHeight() / 2));
            }
        return rect;
        }

    // random number engine for water color and other "hand drawn" effects
    thread_local std::mt19937 ShapeRenderer::m_mt{ std::random_device{}() };

    //---------------------------------------------------
    void ShapeRenderer::DrawWithBaseColorAndBrush(wxDC& dc,
                                                  const std::function<void(void)>& fn) const
        {
        if (GetGraphItemInfo().GetBaseColor())
            {
            const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBaseColor().value());
            fn();
            }
        const DCBrushChangerIfDifferent bc(dc, GetGraphItemInfo().GetBrush());
        fn();
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawText(const wxRect rect, wxDC& dc) const
        {
        Label theLabel(GraphItemInfo{ GetGraphItemInfo().GetText() }
                           .Pen(wxNullPen)
                           .AnchorPoint(GetMidPoint(rect))
                           .Anchoring(Anchoring::Center)
                           .LabelAlignment(TextAlignment::Centered)
                           .DPIScaling(GetDPIScaleFactor()));
        theLabel.SetFontColor(GetGraphItemInfo().GetFontColor());
        theLabel.GetFont().MakeBold();
        theLabel.SetBoundingBox(rect, dc, GetScaling());
        theLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        theLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
        theLabel.Draw(dc);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawImage(const wxRect rect, wxDC& dc) const
        {
        if (m_iconImage != nullptr && m_iconImage->IsOk())
            {
            const auto downScaledSize = geometry::downscaled_size(
                std::pair<double, double>(m_iconImage->GetDefaultSize().GetWidth(),
                                          m_iconImage->GetDefaultSize().GetHeight()),
                std::pair<double, double>(rect.GetWidth(), rect.GetHeight()));
            const wxBitmap scaledImg =
                m_iconImage->GetBitmap(wxSize(downScaledSize.first, downScaledSize.second));
            dc.DrawBitmap(scaledImg, rect.GetTopLeft());
            }
        }

    //---------------------------------------------------
    wxString ShapeInfo::GetReadableShapeName(const Icons::IconShape shape,
                                             const wxString& text /*= wxString{}*/)
        {
        switch (shape)
            {
        case Icons::IconShape::Blank:
            return _(L"blank");
        case Icons::IconShape::ArrowRight:
            return _(L"right arrow");
        case Icons::IconShape::HorizontalLine:
            return _(L"horizontal line");
        case Icons::IconShape::VerticalLine:
            return _(L"vertical line");
        case Icons::IconShape::Circle:
            return _(L"circle");
        case Icons::IconShape::Square:
            return _(L"square");
        case Icons::IconShape::BoxPlot:
            return _(L"box plot");
        case Icons::IconShape::TriangleUpward:
            return _(L"upward triangle");
        case Icons::IconShape::TriangleDownward:
            return _(L"downward triangle");
        case Icons::IconShape::TriangleRight:
            return _(L"right-pointing triangle");
        case Icons::IconShape::TriangleLeft:
            return _(L"left-pointing triangle");
        case Icons::IconShape::Diamond:
            return _(L"diamond");
        case Icons::IconShape::Plus:
            return _(L"plus sign");
        case Icons::IconShape::Asterisk:
            return _(L"asterisk");
        case Icons::IconShape::Hexagon:
            return _(L"hexagon");
        case Icons::IconShape::WarningRoadSign:
            return _(L"warning road sign");
        case Icons::IconShape::GoRoadSign:
            return _(L"go road sign");
        case Icons::IconShape::LocationMarker:
            return _(L"location marker");
        case Icons::IconShape::Image:
            return _(L"image");
        case Icons::IconShape::LeftCurlyBrace:
            return _(L"left curly brace");
        case Icons::IconShape::RightCurlyBrace:
            return _(L"right curly brace");
        case Icons::IconShape::TopCurlyBrace:
            return _(L"top curly brace");
        case Icons::IconShape::BottomCurlyBrace:
            return _(L"bottom curly brace");
        case Icons::IconShape::Man:
            return _(L"male");
        case Icons::IconShape::Woman:
            return _(L"female");
        case Icons::IconShape::BusinessWoman:
            return _(L"businesswoman");
        case Icons::IconShape::ChevronDownward:
            return _(L"downward chevron");
        case Icons::IconShape::ChevronUpward:
            return _(L"upward chevron");
        case Icons::IconShape::Text:
            return _(L"text");
        case Icons::IconShape::Tack:
            return _(L"tack");
        case Icons::IconShape::Banner:
            return _(L"banner");
        case Icons::IconShape::WaterColorRectangle:
            return _(L"watercolor rectangle");
        case Icons::IconShape::ThickWaterColorRectangle:
            return _(L"thick watercolor rectangle");
        case Icons::IconShape::MarkerRectangle:
            return _(L"marker rectangle");
        case Icons::IconShape::PencilRectangle:
            return _(L"pencil rectangle");
        case Icons::IconShape::GraduationCap:
            return _(L"graduation cap");
        case Icons::IconShape::Book:
            return _(L"book");
        case Icons::IconShape::Tire:
            return _(L"tire");
        case Icons::IconShape::Newspaper:
            return _(L"newspaper");
        case Icons::IconShape::Car:
            return _(L"car");
        case Icons::IconShape::Blackboard:
            return _(L"blackboard");
        case Icons::IconShape::Clock:
            return _(L"clock");
        case Icons::IconShape::Ruler:
            return _(L"ruler");
        case Icons::IconShape::IVBag:
            return _(L"IV bag");
        case Icons::IconShape::Heart:
            return _(L"heart");
        case Icons::IconShape::ImmaculateHeart:
            return _(L"Immaculate Heart");
        case Icons::IconShape::ImmaculateHeartWithSword:
            return _(L"Immaculate Heart with sword");
        case Icons::IconShape::Star:
            return _(L"star");
        case Icons::IconShape::Sun:
            return _(L"sun");
        case Icons::IconShape::Flower:
            return _(L"flower");
        case Icons::IconShape::Sunflower:
            return _(L"sunflower");
        case Icons::IconShape::FallLeaf:
            return _(L"fall leaf");
        case Icons::IconShape::Apple:
            return _(L"apple");
        case Icons::IconShape::GrannySmithApple:
            return _(L"Granny Smith apple");
        case Icons::IconShape::Snowflake:
            return _(L"snowflake");
        case Icons::IconShape::Office:
            return _(L"office building");
        case Icons::IconShape::Factory:
            return _(L"factory");
        case Icons::IconShape::House:
            return _(L"house");
        case Icons::IconShape::Barn:
            return _(L"barn");
        case Icons::IconShape::Farm:
            return _(L"farm");
        case Icons::IconShape::HundredDollarBill:
            return _(L"hundred dollar bill");
        case Icons::IconShape::Monitor:
            return _(L"computer monitor");
        case Icons::IconShape::Pumpkin:
            return _(L"pumpkin");
        case Icons::IconShape::JackOLantern:
            return _(L"jack-o'-lantern");
        case Icons::IconShape::Butterfly:
            return _(L"butterfly");
        case Icons::IconShape::Flame:
            return _(L"flame");
        case Icons::IconShape::Sword:
            return _(L"sword");
        case Icons::IconShape::Pill:
            return _(L"pill");
        case Icons::IconShape::ColdThermometer:
            return _(L"cold thermometer");
        case Icons::IconShape::HotThermometer:
            return _(L"hot thermometer");
        case Icons::IconShape::Tractor:
            return _(L"tractor");
        case Icons::IconShape::CheesePizza:
            return _(L"cheese pizza");
        case Icons::IconShape::PepperoniPizza:
            return _(L"pepperoni pizza");
        case Icons::IconShape::HawaiianPizza:
            return _(L"Hawaiian pizza");
        case Icons::IconShape::ChocolateChipCookie:
            return _(L"chocolate chip cookie");
        case Icons::IconShape::CoffeeShopCup:
            return _(L"coffee cup");
        case Icons::IconShape::CurvingRoad:
            return _(L"curving road");
        case Icons::IconShape::CrossedOut:
            return _(L"crossed-out");
        case Icons::IconShape::CrescentTop:
        case Icons::IconShape::CrescentBottom:
        case Icons::IconShape::CrescentRight:
            return _(L"crescent");
        case Icons::IconShape::NumberRange:
            {
            if (text.empty())
                {
                return _(L"number range");
                }
            const wxArrayString parts = wxSplit(text, L':');
            if (parts.size() >= 2)
                {
                wxString rangeLabel = wxString::Format(_(L"number range %s to %s"),
                                                       wxString{ parts[0] }.Trim(false).Trim(),
                                                       wxString{ parts[1] }.Trim(false).Trim());
                if (parts.size() >= 3)
                    {
                    rangeLabel +=
                        wxString::Format(_(L", %s"), wxString{ parts[2] }.Trim(false).Trim());
                    }
                return rangeLabel;
                }
            return _(L"number range");
            }
        default:
            return _(L"shape");
            }
        }
    } // namespace Wisteria::GraphItems
