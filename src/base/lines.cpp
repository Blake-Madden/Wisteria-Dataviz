///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lines.h"
#include "polygon.h"
#include <algorithm>

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------------
    void Lines::Offset(const int xToMove, const int yToMove)
        {
        for (auto& [fst, snd] : m_lines)
            {
            fst += wxPoint(xToMove, yToMove);
            snd += wxPoint(xToMove, yToMove);
            }
        }

    //----------------------------------------------------------------
    wxRect Lines::Draw(wxDC& dc) const
        {
        if (GetClippingRect())
            {
            dc.SetClippingRegion(GetClippingRect().value());
            }

        wxPen scaledPen(GetPen());
        if (GetPen().IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(GetPen().GetWidth()));
            }
        const wxDCPenChanger pc(dc, IsSelected() ?
                                        wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                              2 * scaledPen.GetWidth(), wxPENSTYLE_DOT) :
                                        scaledPen);
        for (const auto& line : m_lines)
            {
            if (GetLineStyle() == LineStyle::Arrows)
                {
                Polygon::DrawArrow(dc, line.first, line.second,
                                   wxSize(ScaleToScreenAndCanvas(5), ScaleToScreenAndCanvas(5)));
                }
            else // Lines or Spline
                {
                dc.DrawLine(line.first, line.second);
                }
            }
        // highlight the selected bounding box in debug mode
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
            {
            if (IsSelected())
                {
                std::array<wxPoint, 5> debugOutline;
                GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), debugOutline);
                const wxDCPenChanger pcDebug{ dc, wxPen(*wxRED, 2 * scaledPen.GetWidth(),
                                                        wxPENSTYLE_SHORT_DASH) };
                dc.DrawLines(debugOutline.size(), debugOutline.data());
                }
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }
        return GetBoundingBox(dc);
        }

    //----------------------------------------------------------------
    wxRect Lines::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        if (m_lines.empty())
            {
            return {};
            }

        double minX{ static_cast<double>(m_lines[0].first.x) };
        double maxX{ static_cast<double>(m_lines[0].first.x) };
        double minY{ static_cast<double>(m_lines[0].first.y) };
        double maxY{ static_cast<double>(m_lines[0].first.y) };

        for (const auto& [fst, snd] : m_lines)
            {
            minX =
                std::min<double>({ minX, static_cast<double>(fst.x), static_cast<double>(snd.x) });
            maxX =
                std::max<double>({ maxX, static_cast<double>(fst.x), static_cast<double>(snd.x) });

            minY =
                std::min<double>({ minY, static_cast<double>(fst.y), static_cast<double>(snd.y) });
            maxY =
                std::max<double>({ maxY, static_cast<double>(fst.y), static_cast<double>(snd.y) });
            }
        return { wxRealPoint(minX, minY), wxRealPoint(maxX, maxY) };
        }

    //----------------------------------------------------------------
    bool Lines::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        std::array<wxPoint, 2> pts;
        for (const auto& line : m_lines)
            {
            pts[0] = line.first;
            pts[1] = line.second;
            if (geometry::is_inside_polygon(pt, pts))
                {
                return true;
                }
            }
        return false;
        }
    } // namespace Wisteria::GraphItems
