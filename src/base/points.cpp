///////////////////////////////////////////////////////////////////////////////
// Name:        points.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "points.h"
#include "label.h"
#include "polygon.h"
#include "shapes.h"

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    void Points2D::SetSelected(const bool selected)
        {
        GraphItemBase::SetSelected(selected);

        if (m_singlePointSelection)
            {
            // update previous selections
            // (this is needed if the parent graph needed to recreate this collection)
            for (auto& pt : m_points)
                {
                if (GetSelectedIds().contains(pt.GetId()))
                    {
                    pt.SetSelected(selected);
                    }
                }
            if (m_lastHitPointIndex < GetPoints().size())
                {
                // toggle selection on individual point
                m_points[m_lastHitPointIndex].SetSelected(
                    !m_points[m_lastHitPointIndex].IsSelected());
                // update list of selected items
                // (based on whether this is newly selected or just unselected)
                if (m_points[m_lastHitPointIndex].IsSelected())
                    {
                    GetSelectedIds().insert(m_points[m_lastHitPointIndex].GetId());
                    }
                else
                    {
                    auto unselectedItem =
                        GetSelectedIds().find(m_points[m_lastHitPointIndex].GetId());
                    if (unselectedItem != GetSelectedIds().end())
                        {
                        GetSelectedIds().erase(unselectedItem);
                        }
                    // if last point was unselected, then mark the entire collection as unselected
                    if (GetSelectedIds().empty())
                        {
                        GraphItemBase::SetSelected(false);
                        }
                    }
                }
            }
        else
            {
            for (auto& point : m_points)
                {
                point.SetSelected(selected);
                }
            }
        }

    //-------------------------------------------
    void Points2D::AddPoint(Point2D pt, wxDC& dc)
        {
        pt.SetId(m_currentAssignedId++);
        pt.SetDPIScaleFactor(GetDPIScaleFactor());
        pt.SetScaling(GetScaling());
        const wxRect ptBoundingBox = pt.GetBoundingBox(dc);
        m_points.push_back(pt);

        if (GetPoints().size() == 1)
            {
            m_boundingBox = ptBoundingBox;
            }
        else
            {
            // adjust top left corner
            if (ptBoundingBox.GetTopLeft().x < GetBoundingBox(dc).GetTopLeft().x)
                {
                m_boundingBox.SetTopLeft(
                    wxPoint(ptBoundingBox.GetTopLeft().x, GetBoundingBox(dc).GetTopLeft().y));
                }
            if (ptBoundingBox.GetTopLeft().y < GetBoundingBox(dc).GetTopLeft().y)
                {
                m_boundingBox.SetTopLeft(
                    wxPoint(GetBoundingBox(dc).GetTopLeft().x, ptBoundingBox.GetTopLeft().y));
                }
            // adjust bottom left corner
            if (ptBoundingBox.GetBottomLeft().x < GetBoundingBox(dc).GetBottomLeft().x)
                {
                m_boundingBox.SetBottomLeft(
                    wxPoint(ptBoundingBox.GetBottomLeft().x, GetBoundingBox(dc).GetBottomLeft().y));
                }
            if (ptBoundingBox.GetBottomLeft().y > GetBoundingBox(dc).GetBottomLeft().y)
                {
                m_boundingBox.SetBottomLeft(
                    wxPoint(GetBoundingBox(dc).GetBottomLeft().x, ptBoundingBox.GetBottomLeft().y));
                }
            // adjust top right corner
            if (ptBoundingBox.GetTopRight().x > GetBoundingBox(dc).GetTopRight().x)
                {
                m_boundingBox.SetTopRight(
                    wxPoint(ptBoundingBox.GetTopRight().x, GetBoundingBox(dc).GetTopRight().y));
                }
            if (ptBoundingBox.GetTopRight().y < GetBoundingBox(dc).GetTopRight().y)
                {
                m_boundingBox.SetTopRight(
                    wxPoint(GetBoundingBox(dc).GetTopRight().x, ptBoundingBox.GetTopRight().y));
                }
            // adjust bottom right corner
            if (ptBoundingBox.GetBottomRight().x > GetBoundingBox(dc).GetBottomRight().x)
                {
                m_boundingBox.SetBottomRight(wxPoint(ptBoundingBox.GetBottomRight().x,
                                                     GetBoundingBox(dc).GetBottomRight().y));
                }
            if (ptBoundingBox.GetBottomRight().y > GetBoundingBox(dc).GetBottomRight().y)
                {
                m_boundingBox.SetBottomRight(wxPoint(GetBoundingBox(dc).GetBottomRight().x,
                                                     ptBoundingBox.GetBottomRight().y));
                }
            }
        }

    //-------------------------------------------
    bool Points2D::HitTest(const wxPoint pt, wxDC& dc) const
        {
        for (auto pointsPos = GetPoints().cbegin(); pointsPos != GetPoints().cend(); ++pointsPos)
            {
            if (pointsPos->HitTest(pt, dc))
                {
                m_lastHitPointIndex = pointsPos - GetPoints().cbegin();
                return true;
                }
            }
        m_lastHitPointIndex = static_cast<std::vector<Point2D>::size_type>(-1);
        return false;
        }

    //-------------------------------------------
    void Points2D::DrawSelectionLabel(wxDC& dc, const double scaling,
                                      const wxRect boundingBox) const
        {
        for (const auto& point : m_points)
            {
            if (point.IsSelected() && point.IsShowingLabelWhenSelected() &&
                !point.GetText().empty())
                {
                const wxRect itemBoundingBox(point.GetBoundingBox(dc));
                GraphItems::Label selectionLabel(
                    GraphItemInfo(point.GetText())
                        .Scaling(scaling)
                        .Pen(*wxBLACK_PEN)
                        .DPIScaling(GetDPIScaleFactor())
                        .Padding(2, 2, 2, 2)
                        .FontBackgroundColor(*wxWHITE)
                        .AnchorPoint(itemBoundingBox.GetTopLeft() +
                                     wxPoint(itemBoundingBox.GetWidth() / 2,
                                             itemBoundingBox.GetHeight() / 2)));
                const wxRect selectionLabelBox = selectionLabel.GetBoundingBox(dc);
                // if going out of the bottom of the bounding box then move it up to fit
                if (!boundingBox.IsEmpty() &&
                    selectionLabelBox.GetBottom() > boundingBox.GetBottom())
                    {
                    selectionLabel.SetAnchorPoint(
                        wxPoint(selectionLabel.GetAnchorPoint().x,
                                selectionLabel.GetAnchorPoint().y -
                                    (selectionLabelBox.GetBottom() - boundingBox.GetBottom())));
                    }
                // if going out of the top of the bounding box then move it down to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetTop() < boundingBox.GetTop())
                    {
                    selectionLabel.SetAnchorPoint(
                        wxPoint(selectionLabel.GetAnchorPoint().x,
                                selectionLabel.GetAnchorPoint().y +
                                    (boundingBox.GetTop() - selectionLabelBox.GetTop())));
                    }
                // if the right side is going out of the box then move it to the left to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetRight() > boundingBox.GetRight())
                    {
                    selectionLabel.SetAnchorPoint(
                        wxPoint(selectionLabel.GetAnchorPoint().x -
                                    (selectionLabelBox.GetRight() - boundingBox.GetRight()),
                                selectionLabel.GetAnchorPoint().y));
                    }
                // if the left side is going out of the box then move it to the right to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetLeft() < boundingBox.GetLeft())
                    {
                    selectionLabel.SetAnchorPoint(
                        wxPoint(selectionLabel.GetAnchorPoint().x +
                                    (boundingBox.GetLeft() - selectionLabelBox.GetLeft()),
                                selectionLabel.GetAnchorPoint().y));
                    }
                selectionLabel.Draw(dc);
                }
            }
        }

    //-------------------------------------------
    wxRect Points2D::Draw(wxDC& dc) const
        {
        if (!IsShown() || GetPoints().empty())
            {
            return {};
            }
        if (IsInDragState())
            {
            return GetBoundingBox(dc);
            }

        if (GetClippingRect())
            {
            dc.SetClippingRegion(GetClippingRect().value());
            }

        // draw connection points
        if (GetPen().IsOk() && !m_points.empty())
            {
            wxPen scaledPen{ GetPen() };
            scaledPen.SetWidth(ScaleToScreenAndCanvas(GetPen().GetWidth()));
            scaledPen.SetColour(GetMaybeGhostedColor(scaledPen.GetColour()));
            const wxDCPenChanger pc{ dc, scaledPen };

            const auto okPointsCount =
                std::ranges::count_if(m_points, [](const auto& pt) noexcept { return pt.IsOk(); });
            if (okPointsCount == 0)
                {
                return {};
                }
            // just one point, so no line to draw
            // (just draw point if shapes aren't being drawn; if points have a shape,
            // then it will be drawn later below)
            if (okPointsCount == 1)
                {
                // find the valid point, draw it, then we're done
                for (const auto& point : m_points)
                    {
                    if (point.IsOk() && point.m_shape == Wisteria::Icons::IconShape::Blank)
                        {
                        const wxDCBrushChanger bc{ dc, scaledPen.GetColour() };
                        dc.DrawCircle(point.GetAnchorPoint(), point.GetRadius());
                        break;
                        }
                    }
                }
            else if (GetLineStyle() == LineStyle::Spline)
                {
                wxPoint startPt{ wxDefaultCoord, wxDefaultCoord };
                std::vector<wxPoint> currentSegment;
                for (size_t i = 0; i < m_points.size(); ++i)
                    {
                    if (m_points[i].IsOk())
                        {
                        // if first valid point in the segment then mark it as such
                        if (!startPt.IsFullySpecified())
                            {
                            startPt = m_points[i].GetAnchorPoint();
                            currentSegment.push_back(startPt);
                            continue;
                            }
                        // or we are at the end of the points and there is a previous
                        // valid starting point in the segment
                        if (i == m_points.size() - 1)
                            {
                            currentSegment.push_back(m_points[i].GetAnchorPoint());
                            if (currentSegment.size() > 1)
                                {
                                dc.DrawSpline(currentSegment.size(), &currentSegment.front());
                                }
                            }
                        // just a point in between the first valid one in the segment
                        // and the end
                        else
                            {
                            currentSegment.push_back(m_points[i].GetAnchorPoint());
                            }
                        }
                    // encountered invalid point, so draw the current segment and then reset
                    else
                        {
                        if (currentSegment.size() > 1)
                            {
                            dc.DrawSpline(currentSegment.size(), &currentSegment.front());
                            }
                        currentSegment.clear();
                        }
                    }
                }
            else
                {
                for (size_t i = 0; i < m_points.size() - 1; ++i)
                    {
                    if (m_points[i].IsOk() && m_points[i + 1].IsOk())
                        {
                        if (GetLineStyle() == LineStyle::Lines)
                            {
                            dc.DrawLine(m_points[i].GetAnchorPoint(),
                                        m_points[i + 1].GetAnchorPoint());
                            }
                        else if (GetLineStyle() == LineStyle::Arrows)
                            {
                            Polygon::DrawArrow(
                                dc, m_points[i].GetAnchorPoint(), m_points[i + 1].GetAnchorPoint(),
                                wxSize(ScaleToScreenAndCanvas(10), ScaleToScreenAndCanvas(10)));
                            }
                        }
                    }
                }
            }
        const bool areAllPointsSelected = (!m_singlePointSelection && IsSelected());

        wxBrush firstBrush{ GetPoints().front().GetBrush() };
        firstBrush.SetColour(GetMaybeGhostedColor(firstBrush.GetColour()));
        const wxDCBrushChanger bc{ dc, firstBrush };
        wxPen scaledPen{ GetPoints().front().GetPen() };
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
            }
        scaledPen.SetColour(GetMaybeGhostedColor(scaledPen.GetColour()));
        const wxDCPenChanger pc{ dc, scaledPen };

        for (const auto& point : GetPoints())
            {
            // if all points selected, then the current pen is the selected one already
            if (!areAllPointsSelected && point.IsSelected())
                {
                Point2D pt = point;
                pt.GetPen().SetStyle(wxPENSTYLE_DOT);
                pt.Draw(dc);
                }
            else
                {
                point.Draw(dc);
                }
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }
        return GetBoundingBox(dc);
        }

    //-------------------------------------------
    wxRect Point2D::Draw(wxDC& dc) const
        {
        if (!IsShown() || !IsOk())
            {
            return {};
            }
        if (IsInDragState())
            {
            return GetBoundingBox(dc);
            }

        if (GetClippingRect())
            {
            dc.SetClippingRegion(GetClippingRect().value());
            }

        if (GetAnchorPoint().IsFullySpecified())
            {
            const auto boundingBox = GetBoundingBox(dc);

            // object that can handle drawing various shapes for the icons
            Shape sh(GraphItemInfo()
                         .Brush(GetBrush())
                         .Pen(GetPen())
                         .Anchoring(Anchoring::TopLeftCorner)
                         .Scaling(GetScaling())
                         .DPIScaling(GetDPIScaleFactor()),
                     m_shape, boundingBox.GetSize(), m_iconImage);
            sh.SetBoundingBox(boundingBox, dc, GetScaling());
            sh.Draw(dc);
            }
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
            {
            if (IsSelected())
                {
                wxPoint debugOutline[5]{ { 0, 0 } };
                GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), debugOutline);
                debugOutline[4] = debugOutline[0];
                const wxDCPenChanger pcDebug(
                    dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
                dc.DrawLines(std::size(debugOutline), debugOutline);
                }
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }
        return GetBoundingBox(dc);
        }

    //-------------------------------------------
    void Point2D::SetBoundingBox(const wxRect& rect, wxDC& dc,
                                 [[maybe_unused]] const double parentScaling)
        {
        assert(!IsFreeFloating() && L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            {
            return;
            }
        SetAnchorPoint(wxPoint(rect.GetLeft() + (rect.GetWidth() / 2),
                               rect.GetTop() + (rect.GetHeight() / 2)));
        const auto upscaleSizeWidth =
            safe_divide<double>(rect.GetWidth(), GetBoundingBox(dc).GetWidth());
        const auto upscaleSizeHeight =
            safe_divide<double>(rect.GetHeight(), GetBoundingBox(dc).GetHeight());
        const double upscaleBestFix = std::min(upscaleSizeWidth, upscaleSizeHeight);
        if (upscaleBestFix > 1)
            {
            SetScaling(GetScaling() * upscaleBestFix);
            }
        }

    //-------------------------------------------
    wxRect Point2D::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        if (!IsOk())
            {
            return {};
            }
        wxPoint cp(GetAnchorPoint());
        if (IsFreeFloating())
            {
            cp.x *= GetScaling();
            cp.y *= GetScaling();
            }
        // convert center point to top left corner of area
        cp -= wxSize(ScaleToScreenAndCanvas(GetRadius()), ScaleToScreenAndCanvas(GetRadius()));
        wxRect boundingBox(cp, wxSize((ScaleToScreenAndCanvas(GetRadius())) * 2,
                                      (ScaleToScreenAndCanvas(GetRadius())) * 2));
        if (m_shape == Wisteria::Icons::IconShape::LocationMarker ||
            m_shape == Wisteria::Icons::IconShape::GoRoadSign ||
            m_shape == Wisteria::Icons::IconShape::WarningRoadSign)
            {
            boundingBox.SetTop(boundingBox.GetTop() - boundingBox.GetHeight());
            boundingBox.SetHeight(boundingBox.GetHeight() * 1.5);
            }
        return boundingBox;
        }
    } // namespace Wisteria::GraphItems
