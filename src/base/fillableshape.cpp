///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
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
        const auto drawArea = GetBoundingBox(dc);
        wxBitmap bmp(drawArea.GetSize());
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(bmp);
        Shape::Draw(wxRect(drawArea.GetSize()), memDC);
        memDC.SelectObject(wxNullBitmap);

        const auto yCutOff = bmp.GetHeight() * (1.0 - m_fillPercent);

        auto ghostedBmp = bmp.GetSubBitmap(
            wxSize(bmp.GetWidth(), yCutOff));
        Image::SetOpacity(ghostedBmp, 32, true);
        dc.DrawBitmap(ghostedBmp, drawArea.GetLeftTop(), true);

        auto filledBmp = bmp.GetSubBitmap(
            wxRect(wxPoint(0, yCutOff),
                   wxSize(bmp.GetWidth(), bmp.GetHeight() * m_fillPercent)) );
        dc.DrawBitmap(filledBmp,
                      drawArea.GetLeftTop() + wxPoint(0, yCutOff), true);

        // draw the outline
        if (IsSelected())
            {
            wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawRectangle(drawArea);
            }

        return wxRect(dc.GetSize());
        }
    }
