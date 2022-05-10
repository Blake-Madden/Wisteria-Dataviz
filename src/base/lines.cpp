///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lines.h"
#include "polygon.h"

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------------
    void Lines::Offset(const int xToMove, const int yToMove)
        {
        for (auto& line : m_lines)
            {
            line.first += wxPoint(xToMove,yToMove);
            line.second += wxPoint(xToMove,yToMove);
            }
        }

    //----------------------------------------------------------------
    wxRect Lines::Draw(wxDC& dc) const
        {
        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

        wxPen scaledPen(GetPen());
        if (GetPen().IsOk())
            { scaledPen.SetWidth(ScaleToScreenAndCanvas(GetPen().GetWidth())); }
        wxDCPenChanger pc(dc, IsSelected() ? wxPen(*wxBLACK, 2*scaledPen.GetWidth(), wxPENSTYLE_DOT) : scaledPen);
        for (const auto& line : m_lines)
            {
            if (GetLineStyle() == LineStyle::Arrows)
                {
                Polygon::DrawArrow(dc, line.first, line.second,
                    wxSize(ScaleToScreenAndCanvas(10), ScaleToScreenAndCanvas(10)));
                }
            else // Lines or Spline
                { dc.DrawLine(line.first, line.second); }
            }
        // highlight the selected bounding box in debug mode
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
            {
            wxPoint debugOutline[5]{ { 0, 0 } };
            GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), debugOutline);
            debugOutline[4] = debugOutline[0];
            wxDCPenChanger pcDebug(dc, wxPen(*wxRED, 2*scaledPen.GetWidth(), wxPENSTYLE_SHORT_DASH));
            dc.DrawLines(std::size(debugOutline), debugOutline);
            }

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }
        return GetBoundingBox(dc);
        }

    //----------------------------------------------------------------
    wxRect Lines::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        if (m_lines.empty())
            { return wxRect(); }

        double minX(m_lines[0].first.x), maxX(m_lines[0].first.x),
               minY(m_lines[0].first.y), maxY((m_lines[0].first.y));

        for (const auto& line : m_lines)
            {
            minX = std::min<double>(minX, std::min(line.first.x, line.second.x));
            maxX = std::max<double>(maxX, std::max(line.first.x, line.second.x));

            minY = std::min<double>(minY, std::min(line.first.y, line.second.y));
            maxY = std::max<double>(maxY, std::max(line.first.y, line.second.y));
            }
        return wxRect(wxRealPoint(minX, minY), wxRealPoint(maxX, maxY));
        }

    //----------------------------------------------------------------
    bool Lines::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        wxPoint pts[2]{ { 0, 0 } };
        for (const auto& line : m_lines)
            {
            pts[0] = line.first; pts[1] = line.second;
            if (GraphItems::Polygon::IsInsidePolygon(pt, pts, 2))
                { return true; }
            }
        return false;
        }
    }
