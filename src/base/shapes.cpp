///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "shapes.h"
#include "label.h"

namespace Wisteria::GraphItems
    {
    void Shapes::DrawCircularSign(const wxRect rect, wxDC& dc)
        {
        const auto radius =
            safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 2);
        wxDCPenChanger pc(dc,
                          wxPen(*wxBLACK,
                                // use smaller outline if sign is small
                                (radius >= 6 ? 2 : 1)));
        wxDCBrushChanger bc(dc, m_graphInfo.m_brush);

        const auto circleCenter = rect.GetLeftTop() +
            wxSize(rect.GetWidth() / 2, rect.GetHeight() / 2);

        dc.DrawCircle(circleCenter, radius);

        // GO label
        Label goLabel(GraphItemInfo(m_graphInfo.m_text).Pen(wxNullPen).
            AnchorPoint(circleCenter).Anchoring(Anchoring::Center).
            LabelAlignment(TextAlignment::Centered).
            DPIScaling(GetDPIScaleFactor()));
        goLabel.SetFontColor(*wxWHITE);
        wxPoint goLabelLabelCorner{ circleCenter };
        auto rectWithinCircleWidth =
            geometry::radius_to_inner_rect_width(radius);
        goLabelLabelCorner.x -= rectWithinCircleWidth / 2;
        goLabelLabelCorner.y -= rectWithinCircleWidth / 2;
        goLabel.SetBoundingBox(
            wxRect(goLabelLabelCorner,
                wxSize(rectWithinCircleWidth, rectWithinCircleWidth)),
            dc, GetScaling());
        goLabel.Draw(dc);
        }
    }
