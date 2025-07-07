///////////////////////////////////////////////////////////////////////////////
// Name:        fillableshape.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "fillableshape.h"

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    wxRect FillableShape::Draw(wxDC& dc) const
        {
        assert(GetBrush().IsOk() && L"Fillable shape must have a valid brush!");
        if (!GetBrush().IsOk())
            {
            return {};
            }

        if (GetClippingRect())
            {
            dc.SetClippingRegion(GetClippingRect().value());
            }

        const auto bBox = GetBoundingBox(dc);
        auto drawRect = wxRect(ScaleToScreenAndCanvas(GetShapeSizeDIPS()));
        // keep drawing area inside the full area
        drawRect.SetWidth(std::min(drawRect.GetWidth(), bBox.GetWidth()));
        drawRect.SetHeight(std::min(drawRect.GetHeight(), bBox.GetHeight()));

        // position the shape inside its (possibly) larger box
        wxPoint shapeTopLeftCorner(GetBoundingBox(dc).GetLeftTop());
        // horizontal page alignment
        if (GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned)
            { /*noop*/
            }
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
            { /*noop*/
            }
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

        // draw the full shape to a bitmap
        wxBitmap bmp(drawRect.GetSize());
        wxBitmap ghostedBmp(drawRect.GetSize());

            // main image
            {
            Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
            wxMemoryDC memDC(bmp);
            wxGCDC gdc(memDC);
            gdc.SetBrush(*wxTRANSPARENT_BRUSH);
            gdc.Clear();

            Shape::Draw(wxRect(drawRect.GetSize()), gdc);
            memDC.SelectObject(wxNullBitmap);
            }
            // ghosted image (brush is translucent, the pen remains the same to show an
            // outline/skeleton of the shape)
            {
            Image::SetOpacity(ghostedBmp, wxALPHA_TRANSPARENT);
            wxMemoryDC memDC(ghostedBmp);
            wxGCDC gdc(memDC);
            gdc.SetBrush(*wxTRANSPARENT_BRUSH);
            gdc.Clear();

            auto shapeInfo{ GraphItemBase::GetGraphItemInfo() };
            shapeInfo.Brush(wxBrush(Wisteria::Colors::ColorContrast::ChangeOpacity(
                                        shapeInfo.GetBrush().GetColour(), 32),
                                    shapeInfo.GetBrush().GetStyle()));
            const Shape ghostShape(shapeInfo, GetShape(), GetSizeDIPS());

            ghostShape.Draw(wxRect(drawRect.GetSize()), gdc);
            memDC.SelectObject(wxNullBitmap);
            }

        // if 100% "filled," then just draw the regular bitmap
        if (compare_doubles_greater_or_equal(m_fillPercent, math_constants::full))
            {
            dc.DrawBitmap(bmp, drawRect.GetLeftTop(), true);
            }
        else
            {
            const auto yCutOff = bmp.GetHeight() * (math_constants::full - m_fillPercent);

            ghostedBmp = ghostedBmp.GetSubBitmap(wxSize(bmp.GetWidth(), yCutOff));
            dc.DrawBitmap(ghostedBmp, drawRect.GetLeftTop(), true);

            // nothing to draw above the ghosted image if empty
            if (compare_doubles_greater(m_fillPercent, math_constants::empty))
                {
                auto filledBmp = bmp.GetSubBitmap(wxRect(
                    wxPoint(0, yCutOff), wxSize(bmp.GetWidth(), bmp.GetHeight() * m_fillPercent)));
                dc.DrawBitmap(filledBmp,
                              drawRect.GetLeftTop() + wxPoint{ 0, static_cast<int>(yCutOff) },
                              true);
                }
            }

        // draw the bounding box outline
        if (IsSelected())
            {
            const wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            const wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawRectangle(drawRect);
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }

        return { dc.GetSize() };
        }
    } // namespace Wisteria::GraphItems
