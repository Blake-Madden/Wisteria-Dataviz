///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "shapes.h"
#include "label.h"

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
        const auto radius =
            safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 3);
        wxDCPenChanger pc(dc, wxPen(m_graphInfo.GetBrush().GetColour(),
                          ScaleToScreenAndCanvas(1)));
        wxDCBrushChanger bc(dc, m_graphInfo.GetBrush());

        dc.DrawLine(rect.GetTopLeft(), rect.GetBottomRight());
        dc.DrawLine(rect.GetTopRight(), rect.GetBottomLeft());
        dc.DrawLine(rect.GetTopLeft() + wxPoint(rect.GetWidth()/2, 0),
                    rect.GetBottomLeft() + wxPoint(rect.GetWidth()/2, 0));
        dc.DrawLine(rect.GetTopLeft() + wxPoint(0, rect.GetHeight()/2),
                    rect.GetBottomRight() - wxPoint(0, rect.GetHeight()/2));

        const auto circleCenter = rect.GetLeftTop() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        dc.DrawCircle(circleCenter, radius);
        }
    }
