///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "fillableshape.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Icons;

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    wxRect FillableShape::Draw(wxDC& dc) const
        {
        wxASSERT_MSG(GetBrush().IsOk(), L"Fillable shape must have a valid brush!");
        if (!GetBrush().IsOk())
            { return wxRect(); }
        const auto drawArea = GetBoundingBox(dc);

        // draw the full shape to a bitmap
        wxBitmap bmp(drawArea.GetSize());
        wxBitmap ghostedBmp(drawArea.GetSize());

        // main image
            {
            Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
            wxMemoryDC memDC(bmp);
            wxGCDC gdc(memDC);
            gdc.SetBrush(*wxTRANSPARENT_BRUSH);
            gdc.Clear();

            Shape::Draw(wxRect(drawArea.GetSize()), gdc);
            memDC.SelectObject(wxNullBitmap);
            }
        // ghosted image (brush is translucent, the pen remains the same to show an outline/skeleton of the shape)
            {
            Image::SetOpacity(ghostedBmp, wxALPHA_TRANSPARENT);
            wxMemoryDC memDC(ghostedBmp);
            wxGCDC gdc(memDC);
            gdc.SetBrush(*wxTRANSPARENT_BRUSH);
            gdc.Clear();

            auto shapeInfo{ GraphItemBase::GetGraphItemInfo() };
            shapeInfo.Brush(wxBrush(Colors::ColorContrast::ChangeOpacity(shapeInfo.GetBrush().GetColour(), 32),
                            shapeInfo.GetBrush().GetStyle()));
            Shape ghostShape(shapeInfo, GetShape(), GetSizeDIPS());

            ghostShape.Draw(wxRect(drawArea.GetSize()), gdc);
            memDC.SelectObject(wxNullBitmap);
            }

        // if 100% "filled," then just draw the regular bitmap
        if (compare_doubles_greater_or_equal(m_fillPercent, math_constants::full))
            {
            dc.DrawBitmap(bmp, drawArea.GetLeftTop(), true);
            }
        else
            {
            const auto yCutOff = bmp.GetHeight() * (math_constants::full - m_fillPercent);

            ghostedBmp = ghostedBmp.GetSubBitmap(wxSize(bmp.GetWidth(), yCutOff));
            dc.DrawBitmap(ghostedBmp, drawArea.GetLeftTop(), true);

            // nothing to draw above the ghosted image if empty
            if (compare_doubles_greater(m_fillPercent, math_constants::empty))
                {
                auto filledBmp = bmp.GetSubBitmap(
                    wxRect(wxPoint(0, yCutOff),
                        wxSize(bmp.GetWidth(), bmp.GetHeight() * m_fillPercent)));
                dc.DrawBitmap(filledBmp,
                    drawArea.GetLeftTop() + wxPoint(0, yCutOff), true);
                }
            }

        // draw the bounding box outline
        if (IsSelected())
            {
            wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawRectangle(drawArea);
            }

        return wxRect(dc.GetSize());
        }
    }
