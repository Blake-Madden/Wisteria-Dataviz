///////////////////////////////////////////////////////////////////////////////
// Name:        plotitems.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "graphitems.h"
#include "label.h"
#include "image.h"

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    double GraphItemBase::GetDPIScaleFactor() const noexcept
        {
        wxASSERT_LEVEL_2_MSG(m_itemInfo.m_dpiScaleFactor.has_value(),
                             L"Graph item should have a proper DPI scaling.");
        return (m_itemInfo.m_dpiScaleFactor.has_value() ?
                m_itemInfo.m_dpiScaleFactor.value() : 1);
        }

    //-------------------------------------------
    void GraphItemBase::DrawSelectionLabel(wxDC& dc, const double scaling,
                                           const wxRect boundingBox /*= wxRect()*/) const
        {
        if (IsSelected() && IsShowingLabelWhenSelected() && !GetText().empty())
            {
            const wxRect ItemBoundingBox(GetBoundingBox(dc));
            GraphItems::Label selectionLabel(GraphItemInfo(GetGraphItemInfo()).
                Scaling(scaling).Pen(*wxBLACK_PEN).
                DPIScaling(GetDPIScaleFactor()).
                Padding(2, 2, 2, 2).FontBackgroundColor(*wxWHITE).
                Anchoring(Anchoring::Center).
                AnchorPoint(ItemBoundingBox.GetTopLeft() +
                            wxPoint(ItemBoundingBox.GetWidth()/2,
                            ItemBoundingBox.GetHeight()/2)));
            const wxRect selectionLabelBox = selectionLabel.GetBoundingBox(dc);
            // if going out of the bottom of the bounding box then move it up to fit
            if (!boundingBox.IsEmpty() && selectionLabelBox.GetBottom() > boundingBox.GetBottom())
                {
                selectionLabel.SetAnchorPoint(
                    wxPoint(selectionLabel.GetAnchorPoint().x,
                    selectionLabel.GetAnchorPoint().y-(selectionLabelBox.GetBottom()-boundingBox.GetBottom())));
                }
            // if going out of the top of the bounding box then move it down to fit
            if (!boundingBox.IsEmpty() && selectionLabelBox.GetTop() < boundingBox.GetTop())
                {
                selectionLabel.SetAnchorPoint(
                    wxPoint(selectionLabel.GetAnchorPoint().x,
                    selectionLabel.GetAnchorPoint().y+(boundingBox.GetTop()-selectionLabelBox.GetTop())));
                }
            // if the right side is going out of the box then move it to the left to fit
            if (!boundingBox.IsEmpty() && selectionLabelBox.GetRight() > boundingBox.GetRight())
                {
                selectionLabel.SetAnchorPoint(
                    wxPoint(selectionLabel.GetAnchorPoint().x-(selectionLabelBox.GetRight()-boundingBox.GetRight()),
                    selectionLabel.GetAnchorPoint().y));
                }
            // if the left side is going out of the box then move it to the right to fit
            if (!boundingBox.IsEmpty() && selectionLabelBox.GetLeft() < boundingBox.GetLeft())
                {
                selectionLabel.SetAnchorPoint(
                    wxPoint(selectionLabel.GetAnchorPoint().x+(boundingBox.GetLeft()-selectionLabelBox.GetLeft()),
                    selectionLabel.GetAnchorPoint().y));
                }
            selectionLabel.Draw(dc);
            }
        }

    //-------------------------------------------
    wxBitmap GraphItemBase::ToBitmap(wxDC& dc) const
        {
        const wxRect BoundingBox = GetBoundingBox(dc).Inflate(ScaleToScreenAndCanvas(3));
        wxBitmap bmp(BoundingBox.GetWidth(), BoundingBox.GetHeight());
        wxMemoryDC memDc(bmp);
        memDc.SetBackground(*wxTRANSPARENT_BRUSH);
        memDc.Clear();
        memDc.SetLogicalOrigin(BoundingBox.GetPosition().x, BoundingBox.GetPosition().y);
        Draw(memDc);
        memDc.SelectObject(wxNullBitmap);
        wxImage img(bmp.ConvertToImage());
        return wxBitmap(img);
        }

    //-------------------------------------------
    void Polygon::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                 [[maybe_unused]] wxDC& dc,
                                 [[maybe_unused]] const double parentScaling)
        {
        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                             L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            { return; }
        wxFAIL_MSG(L"SetBoundingBox() not currently supported!");
        return;
        }

    //-------------------------------------------
    void Polygon::SetPoints(const wxPoint* polygon, const size_t N)
        {
        if (N > 0)
            {
            m_points.assign(polygon, polygon+N);
            UpdatePointPositions();
            }
        else
            {
            m_points.clear();
            m_scaledPoints.clear();
            }
        }

    //-------------------------------------------
    bool Polygon::IsInsidePolygon(const wxPoint p, const wxPoint* polygon, const int N)
        {
        wxASSERT_LEVEL_2(N > 0 && polygon);
        if (N == 0 || polygon == nullptr)
            { return false; }

        constexpr double __DOUBLE_EPSILON__{ .01f };
        constexpr bool bound{ true };
        // cross points count of x
        int __count{ 0 };

        // neighbour bound vertices
        wxPoint p1, p2;

        // left vertex
        p1 = polygon[0];

        // check all rays
        for (int i = 1; i <= N; ++i)
            {
            // point is a vertex
            if (p == p1) return bound;

            // right vertex
            p2 = polygon[i % N];

            // ray is outside of our interests
            if (p.y < std::min(p1.y, p2.y) || p.y > std::max(p1.y, p2.y))
                {
                // next ray left point
                p1 = p2; continue;
                }

            // ray is crossing over by the algorithm (common part of)
            if (p.y > std::min(p1.y, p2.y) && p.y < std::max(p1.y, p2.y))
                {
                // x is before of ray
                if (p.x <= std::max(p1.x, p2.x))
                    {
                    // overlies on a horizontal ray
                    if (p1.y == p2.y && p.x >= std::min(p1.x, p2.x)) return bound;

                    // ray is vertical
                    if (p1.x == p2.x)
                        {
                        // overlies on a ray
                        if (p1.x == p.x) return bound;
                        // before ray
                        else ++__count;
                        }

                    // cross point on the left side
                    else
                        {
                        // cross point of x
                        const auto xinters = ((p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y)) + p1.x;

                        // overlies on a ray
                        if (std::fabs(p.x - xinters) < __DOUBLE_EPSILON__) return bound;

                        // before ray
                        if (p.x < xinters) ++__count;
                        }
                    }
                }
            // special case when ray is crossing through the vertex
            else
                {
                // p crossing over p2
                if (p.y == p2.y && p.x <= p2.x)
                    {
                    // next vertex
                    const wxPoint& p3 = polygon[(i+1) % N];

                    // p.y lies between p1.y & p3.y
                    if (p.y >= std::min(p1.y, p3.y) && p.y <= std::max(p1.y, p3.y))
                        {
                        ++__count;
                        }
                    else
                        {
                        __count += 2;
                        }
                    }
                }

            // next ray left point
            p1 = p2;
            }

        // EVEN
        if (__count % 2 == 0) return false;
        // ODD
        else return true ;
        }

    //-------------------------------------------
    wxRect Polygon::GetPolygonBoundingBox(const wxPoint* polygon, const size_t N)
        {
        wxASSERT_LEVEL_2(N > 0 && polygon);
        if (N == 0 || polygon == nullptr)
            { return wxRect(); }

        wxCoord minX(polygon[0].x), maxX(polygon[0].x), minY(polygon[0].y), maxY(polygon[0].y);
        for (size_t i = 1; i < N; ++i)
            {
            minX = std::min(polygon[i].x, minX);
            maxX = std::max(polygon[i].x, maxX);
            minY = std::min(polygon[i].y, minY);
            maxY = std::max(polygon[i].y, maxY);
            }
        return wxRect(wxPoint(minX,minY), wxPoint(maxX,maxY));
        }

    //-------------------------------------------
    wxRect Polygon::GetPolygonBoundingBox(const std::vector<wxPoint>& polygon)
        {
        wxASSERT_LEVEL_2(polygon.size());
        if (polygon.size() == 0)
            { return wxRect(); }

        wxCoord minX(polygon[0].x), maxX(polygon[0].x), minY(polygon[0].y), maxY(polygon[0].y);
        for (const auto& pt : polygon)
            {
            minX = std::min(pt.x, minX);
            maxX = std::max(pt.x, maxX);
            minY = std::min(pt.y, minY);
            maxY = std::max(pt.y, maxY);
            }
        return wxRect(wxPoint(minX,minY), wxPoint(maxX,maxY));
        }

    //-------------------------------------------
    void Polygon::UpdatePointPositions()
        {
        m_scaledPoints = GetPoints();
        if (!IsFreeFloating())
            { return; }
        for (auto ptPos = m_scaledPoints.begin(); ptPos != m_scaledPoints.end(); ++ptPos)
            { *ptPos = (*ptPos*GetScaling()); } // grow
        }

    //-------------------------------------------
    bool Polygon::HitTest(const wxPoint pt, wxDC& dc) const
        { return IsInsidePolygon(pt, &m_scaledPoints[0], m_scaledPoints.size()); }

    //-------------------------------------------
    void Polygon::GetRectPoints(const wxRect& rect, wxPoint* points)
        {
        if (!points)
            { return; }
        points[0] = rect.GetTopLeft();
        points[1] = rect.GetTopRight();
        points[2] = rect.GetBottomRight();
        points[3] = rect.GetBottomLeft();
        }

    //-------------------------------------------
    wxRect Polygon::GetRectFromPoints(const wxPoint* points)
        {
        if (!points)
            { return wxRect(); }
        decltype(points[0].x) lowestX{ points[0].x }, lowestY{ points[0].y },
                              highestX{ points[0].x }, highestY{ points[0].y };
        for (size_t i = 0; i < 4; ++i)
            {
            lowestX = std::min(lowestX, points[i].x);
            lowestY = std::min(lowestY, points[i].y);
            highestX = std::max(highestX, points[i].x);
            highestY = std::max(highestY, points[i].y);
            }
        return wxRect(wxPoint(lowestX, lowestY), wxPoint(highestX, highestY));
        }

    //-------------------------------------------
    wxRect Polygon::Draw(wxDC& dc) const
        {
        if (!IsShown())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }
        const wxRect boundingBox = GetBoundingBox(dc);

        wxPen scaledPen(GetPen().IsOk() ? GetPen() : *wxTRANSPARENT_PEN);
        scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
        wxDCPenChanger pc(dc, IsSelected() ?
            wxPen(*wxBLACK, 2*scaledPen.GetWidth(), wxPENSTYLE_DOT) : scaledPen);

        // using a color fill (possibly a gradient)
        if (GetBackgroundFill().IsOk())
            {
            wxDCBrushChanger bc(dc, GetBackgroundFill().GetColor1());
            if (GetBackgroundFill().IsGradient())
                {
                // Optimized for rectangle.
                // Also, this enables the draw commands of the gradient to be
                // translated into SVG properly.
                if (GetShape() == PolygonShape::Rectangle)
                    {
                    const auto theRect{ GetRectFromPoints(&m_scaledPoints[0]) };
                    dc.GradientFillLinear(theRect,
                        GetBackgroundFill().GetColor1(), GetBackgroundFill().GetColor2(),
                        (GetBackgroundFill().GetDirection() == FillDirection::North ? wxNORTH :
                         GetBackgroundFill().GetDirection() == FillDirection::East ? wxEAST :
                         GetBackgroundFill().GetDirection() == FillDirection::West ? wxWEST :
                         wxSOUTH) );
                    wxDCBrushChanger bc2(dc, *wxTRANSPARENT_BRUSH);
                    dc.DrawRectangle(theRect);
                    }
                // a spline doesn't use a brush, so just draw it
                else if (GetShape() == PolygonShape::Spline)
                    { dc.DrawSpline(m_scaledPoints.size(), &m_scaledPoints[0]); }
                // irregular polygon
                // @bug SVG exporting of this will lack the gradient
                else
                    {
                    if (dc.IsKindOf(CLASSINFO(wxGCDC)))
                        {
                        wxGraphicsContext* gc = dynamic_cast<wxGCDC&>(dc).GetGraphicsContext();
                        wxASSERT_LEVEL_2(gc);
                        if (gc)
                            {
                            wxPoint start;
                            wxPoint stop;
                            switch (GetBackgroundFill().GetDirection())
                                {
                            case FillDirection::East:
                                start = boundingBox.GetTopLeft() +
                                        wxPoint(0,(boundingBox.GetHeight()/2));
                                stop = boundingBox.GetTopRight() +
                                        wxPoint(0,(boundingBox.GetHeight()/2));
                                break;
                            case FillDirection::West:
                                start = boundingBox.GetTopRight()+wxPoint(0,(boundingBox.GetHeight()/2));
                                stop = boundingBox.GetTopLeft()+wxPoint(0,(boundingBox.GetHeight()/2));
                                break;
                            case FillDirection::North:
                                start = boundingBox.GetBottomLeft()+wxPoint((boundingBox.GetWidth()/2),0);
                                stop = boundingBox.GetTopLeft()+wxPoint((boundingBox.GetWidth()/2),0);
                                break;
                            case FillDirection::South:
                            default:
                                start = boundingBox.GetTopLeft()+wxPoint((boundingBox.GetWidth()/2),0);
                                stop = boundingBox.GetBottomLeft()+wxPoint((boundingBox.GetWidth()/2),0);
                                break;
                                };
                            gc->SetBrush(
                                gc->CreateLinearGradientBrush(start.x, start.y,
                                    stop.x, stop.y,
                                    GetBackgroundFill().GetColor1(), GetBackgroundFill().GetColor2()));
                            dc.DrawPolygon(m_scaledPoints.size(), &m_scaledPoints[0]);
                            }
                        }
                    else
                        { dc.DrawPolygon(m_scaledPoints.size(), &m_scaledPoints[0]); }
                    }
                }
            else
                { dc.DrawPolygon(m_scaledPoints.size(), &m_scaledPoints[0]); }
            }

        // Using the brush.
        // Note that we can use a brush on top of a color-filled background,
        // like a hatched brush on top of white background.
        if (GetBrush().IsOk() || (IsSelected() && GetSelectionBrush().IsOk()))
            {
            wxDCBrushChanger bc(dc, (IsSelected() && GetSelectionBrush().IsOk()) ?
                                     GetSelectionBrush() : GetBrush());
            if (GetShape() == PolygonShape::Spline)
                { dc.DrawSpline(m_scaledPoints.size(), &m_scaledPoints[0]); }
            else if (GetShape() == PolygonShape::Rectangle &&
                     GetBoxCorners() == BoxCorners::Rounded)
                { dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius()); }
            else
                { dc.DrawPolygon(m_scaledPoints.size(), &m_scaledPoints[0]); }
            }
        // just drawing an outline (hasn't already be drawn with a background color above)
        else if (!GetBackgroundFill().IsOk())
            {
            wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            if (GetShape() == PolygonShape::Spline)
                { dc.DrawSpline(m_scaledPoints.size(), &m_scaledPoints[0]); }
            else
                { dc.DrawPolygon(m_scaledPoints.size(), &m_scaledPoints[0]); }
            }

        // highlight the selected bounding box in debug mode
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
            {
            std::array<wxPoint, 5> debugOutline;
            GetRectPoints(boundingBox, &debugOutline[0]);
            debugOutline[4] = debugOutline[0];
            wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
            dc.DrawLines(debugOutline.size(), &debugOutline[0]);
            }
        return boundingBox;
        }

    //------------------------------------------------------
    void Polygon::DrawArrow(wxDC& dc, const wxPoint pt1, const wxPoint pt2,
                            const wxSize arrowHeadSize)
	    {
        wxASSERT_MSG(arrowHeadSize.IsFullySpecified(), L"Arrowhead size not fully specified.");
        if (!arrowHeadSize.IsFullySpecified())
            { return; }
	    const float dx = static_cast<float>(pt2.x - pt1.x);
	    const float dy = static_cast<float>(pt2.y - pt1.y);
	    const auto length = std::sqrt(dx*dx + dy*dy);

	    // ux,uy is a unit vector parallel to the line.
	    const auto ux = safe_divide(dx, length);
	    const auto uy = safe_divide(dy, length);

	    // vx,vy is a unit vector perpendicular to ux,uy
	    const auto vx = -uy;
	    const auto vy = ux;

	    const auto halfWidth = 0.5f * arrowHeadSize.GetWidth();

	    const std::array<wxPoint, 3> arrowHead
		    {
            { pt2,
              wxPoint(std::round(pt2.x - arrowHeadSize.GetHeight()*ux + halfWidth * vx),
                std::round(pt2.y - arrowHeadSize.GetHeight()*uy + halfWidth * vy)),
              wxPoint(std::round(pt2.x - arrowHeadSize.GetHeight()*ux - halfWidth * vx),
                std::round(pt2.y - arrowHeadSize.GetHeight()*uy - halfWidth * vy)) }
		    };

        // The end of the line should be going underneath the head by just one pixel,
        // so that it doesn't poke out under the point of the arrowhead.
        // Note that this only works if pointing perfectly left or right; otherwise,
        // we just have to connect the end of the line to the end of the arrowhead.
        const wxCoord xAdjustment = (pt1.y == pt2.y && pt1.x <= pt2.x) ?
                                        (-(arrowHeadSize.GetWidth())+1) :
                                    (pt1.y == pt2.y && pt1.x > pt2.x) ?
                                        ((arrowHeadSize.GetWidth())-1) : 0;

	    dc.DrawLine(pt1, wxPoint(pt2.x+xAdjustment, pt2.y));
        // fill the arrowhead with the same color as the line
        wxDCBrushChanger bc(dc, dc.GetPen().GetColour());
        // need to turn off the pen because a thicker pen will cause an odd-looking
        // effect when the two lines converge at the tip of the arrowhead
        wxDCPenChanger pc(dc, *wxTRANSPARENT_PEN);
	    dc.DrawPolygon(arrowHead.size(), &arrowHead[0]);
	    }

    //-------------------------------------------
    void Polygon::Offset(const int xToMove, const int yToMove)
        {
        for (auto pos = m_points.begin(); pos != m_points.end(); ++pos)
            { *pos += wxPoint(xToMove, yToMove); }
        }

    //-------------------------------------------
    void Point2D::SetBoundingBox(const wxRect& rect,
                                 wxDC& dc,
                                 [[maybe_unused]] const double parentScaling)
        {
        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                             L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            { return; }
        SetAnchorPoint(wxPoint(rect.GetLeft()+(rect.GetWidth()/2),
                               rect.GetTop()+(rect.GetHeight()/2)));
        const double upscaleSizeWidth = safe_divide<double>(rect.GetWidth(),
                                                            GetBoundingBox(dc).GetWidth());
        const double upscaleSizeHeight = safe_divide<double>(rect.GetHeight(),
                                                             GetBoundingBox(dc).GetHeight());
        const double upscaleBestFix = std::min(upscaleSizeWidth, upscaleSizeHeight);
        if (upscaleBestFix > 1)
            { SetScaling(GetScaling()*upscaleBestFix); }
        }

    //-------------------------------------------
    void Points2D::SetSelected(const bool selected)
        {
        GraphItemBase::SetSelected(selected);
        
        if (m_singlePointSelection)
            {
            // re-select selected items if necessary
            // (this is needed if the parent graph needed to recreate this collection)
            if (selected)
                {
                for (auto& pt : m_points)
                    {
                    if (GetSelectedIds().find(pt.GetId()) != GetSelectedIds().cend())
                        { pt.SetSelected(true); }
                    }
                }
            if (m_lastHitPointIndex < GetPoints().size())
                {
                // toggle selection on individual point
                m_points[m_lastHitPointIndex].SetSelected(!m_points[m_lastHitPointIndex].IsSelected());
                // update list of selected items
                // (based on whether this is newly selected or just unselected)
                if (m_points[m_lastHitPointIndex].IsSelected())
                    { GetSelectedIds().insert(m_points[m_lastHitPointIndex].GetId()); }
                else
                    {
                    auto unselectedItem = GetSelectedIds().find(m_points[m_lastHitPointIndex].GetId());
                    if (unselectedItem != GetSelectedIds().end())
                        { GetSelectedIds().erase(unselectedItem); }
                    // if last point was unselected, then mark the entire collection as unselected
                    if (!GetSelectedIds().size())
                        { GraphItemBase::SetSelected(false); }
                    }
                }
            }
        else
            {
            for (auto& point : m_points)
                { point.SetSelected(selected); }
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
            { m_boundingBox = ptBoundingBox; }
        else
            {
            // adjust top left corner
            if (ptBoundingBox.GetTopLeft().x < GetBoundingBox(dc).GetTopLeft().x)
                { m_boundingBox.SetTopLeft(wxPoint(ptBoundingBox.GetTopLeft().x, GetBoundingBox(dc).GetTopLeft().y)); }
            if (ptBoundingBox.GetTopLeft().y < GetBoundingBox(dc).GetTopLeft().y)
                { m_boundingBox.SetTopLeft(wxPoint(GetBoundingBox(dc).GetTopLeft().x, ptBoundingBox.GetTopLeft().y)); }
            // adjust bottom left corner
            if (ptBoundingBox.GetBottomLeft().x < GetBoundingBox(dc).GetBottomLeft().x)
                { m_boundingBox.SetBottomLeft(wxPoint(ptBoundingBox.GetBottomLeft().x, GetBoundingBox(dc).GetBottomLeft().y)); }
            if (ptBoundingBox.GetBottomLeft().y > GetBoundingBox(dc).GetBottomLeft().y)
                { m_boundingBox.SetBottomLeft(wxPoint(GetBoundingBox(dc).GetBottomLeft().x, ptBoundingBox.GetBottomLeft().y)); }
            // adjust top right corner
            if (ptBoundingBox.GetTopRight().x > GetBoundingBox(dc).GetTopRight().x)
                { m_boundingBox.SetTopRight(wxPoint(ptBoundingBox.GetTopRight().x, GetBoundingBox(dc).GetTopRight().y)); }
            if (ptBoundingBox.GetTopRight().y < GetBoundingBox(dc).GetTopRight().y)
                { m_boundingBox.SetTopRight(wxPoint(GetBoundingBox(dc).GetTopRight().x, ptBoundingBox.GetTopRight().y)); }
            // adjust bottom right corner
            if (ptBoundingBox.GetBottomRight().x > GetBoundingBox(dc).GetBottomRight().x)
                { m_boundingBox.SetBottomRight(wxPoint(ptBoundingBox.GetBottomRight().x, GetBoundingBox(dc).GetBottomRight().y)); }
            if (ptBoundingBox.GetBottomRight().y > GetBoundingBox(dc).GetBottomRight().y)
                { m_boundingBox.SetBottomRight(wxPoint(GetBoundingBox(dc).GetBottomRight().x, ptBoundingBox.GetBottomRight().y)); }
            }
        }

    //-------------------------------------------
    bool Points2D::HitTest(const wxPoint pt, wxDC& dc) const
        {
        for (auto pointsPos = GetPoints().cbegin(); pointsPos != GetPoints().cend(); ++pointsPos)
            {
            if (pointsPos->HitTest(pt, dc))
                {
                m_lastHitPointIndex = pointsPos-GetPoints().cbegin();
                return true;
                }
            }
        m_lastHitPointIndex = static_cast<std::vector<Point2D>::size_type>(-1);
        return false;
        }

    //-------------------------------------------
    void Points2D::DrawSelectionLabel(wxDC& dc, const double scaling, const wxRect boundingBox) const
        {
        for (const auto& point : m_points)
            {
            if (point.IsSelected() && point.IsShowingLabelWhenSelected() && !point.GetText().empty())
                {
                const wxRect ItemBoundingBox(point.GetBoundingBox(dc));
                GraphItems::Label selectionLabel(
                    GraphItemInfo(point.GetText()).Scaling(scaling).Pen(*wxBLACK_PEN).
                    DPIScaling(GetDPIScaleFactor()).
                    Padding(2, 2, 2, 2).FontBackgroundColor(*wxWHITE).
                    AnchorPoint(ItemBoundingBox.GetTopLeft()+wxPoint(ItemBoundingBox.GetWidth()/2,
                                ItemBoundingBox.GetHeight()/2)));
                const wxRect selectionLabelBox = selectionLabel.GetBoundingBox(dc);
                // if going out of the bottom of the bounding box then move it up to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetBottom() > boundingBox.GetBottom())
                    {
                    selectionLabel.SetAnchorPoint(wxPoint(selectionLabel.GetAnchorPoint().x,
                        selectionLabel.GetAnchorPoint().y -
                        (selectionLabelBox.GetBottom()-boundingBox.GetBottom())));
                    }
                // if going out of the top of the bounding box then move it down to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetTop() < boundingBox.GetTop())
                    {
                    selectionLabel.SetAnchorPoint(wxPoint(selectionLabel.GetAnchorPoint().x,
                        selectionLabel.GetAnchorPoint().y+(boundingBox.GetTop()-selectionLabelBox.GetTop())));
                    }
                // if the right side is going out of the box then move it to the left to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetRight() > boundingBox.GetRight())
                    {
                    selectionLabel.SetAnchorPoint(wxPoint(selectionLabel.GetAnchorPoint().x -
                                                          (selectionLabelBox.GetRight()-boundingBox.GetRight()),
                        selectionLabel.GetAnchorPoint().y));
                    }
                // if the left side is going out of the box then move it to the right to fit
                if (!boundingBox.IsEmpty() && selectionLabelBox.GetLeft() < boundingBox.GetLeft())
                    {
                    selectionLabel.SetAnchorPoint(wxPoint(selectionLabel.GetAnchorPoint().x +
                                                  (boundingBox.GetLeft()-selectionLabelBox.GetLeft()),
                        selectionLabel.GetAnchorPoint().y));
                    }
                selectionLabel.Draw(dc);
                }
            }
        }

    //-------------------------------------------
    wxRect Points2D::Draw(wxDC& dc) const
        {
        if (!IsShown())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }
        // draw connection points
        if (GetPen().IsOk() && m_points.size())
            {
            wxPen scaledPen(GetPen());
            scaledPen.SetWidth(ScaleToScreenAndCanvas(GetPen().GetWidth()));
            wxDCPenChanger pc(dc, scaledPen);

            const auto okPointsCount = std::count_if(m_points.cbegin(), m_points.cend(),
                [](const auto pt) noexcept { return pt.IsOk();  });
            if (okPointsCount == 0)
                { return wxRect(); }
            // just one point, so no line to draw
            // (just draw point if shapes aren't being drawn; if points have a shape,
            //  then it will be drawn later below)
            else if (okPointsCount == 1)
                {
                // find the valid point, draw it, then we're done
                for (size_t i = 0; i < m_points.size(); ++i)
                    {
                    if (m_points[i].IsOk() && m_points[i].m_shape == IconShape::BlankIcon)
                        {
                        wxDCBrushChanger bc(dc, scaledPen.GetColour());
                        dc.DrawCircle(m_points[i].GetAnchorPoint(), m_points[i].GetRadius());
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
                            currentSegment.emplace_back(startPt);
                            continue;
                            }
                        // or we are at the end of the points and there is a previous
                        // valid starting point in the segment
                        else if (i == m_points.size()-1)
                            {
                            currentSegment.emplace_back(m_points[i].GetAnchorPoint());
                            if (currentSegment.size() > 1)
                                { dc.DrawSpline(currentSegment.size(), &currentSegment.front()); }
                            }
                        // just a point in between the first valid one in the segment
                        // and the end
                        else
                            { currentSegment.emplace_back(m_points[i].GetAnchorPoint()); }
                        }
                    // encountered invalid point, so draw the current segment and then reset
                    else
                        {
                        if (currentSegment.size() > 1)
                            { dc.DrawSpline(currentSegment.size(), &currentSegment.front()); }
                        currentSegment.clear();
                        }
                    }
                }
            else
                {
                for (size_t i = 0; i < m_points.size()-1; ++i)
                    {
                    if (m_points[i].IsOk() && m_points[i+1].IsOk())
                        {
                        if (GetLineStyle() == LineStyle::Lines)
                            {
                            dc.DrawLine(m_points[i].GetAnchorPoint(),
                                        m_points[i+1].GetAnchorPoint());
                            }
                        else if (GetLineStyle() == LineStyle::Arrows)
                            {
                            Polygon::DrawArrow(dc, m_points[i].GetAnchorPoint(),
                                m_points[i+1].GetAnchorPoint(),
                                wxSize(ScaleToScreenAndCanvas(10), ScaleToScreenAndCanvas(10)));
                            }
                        }
                    }
                }
            }
        const bool areAllPointsSelected = (!m_singlePointSelection && IsSelected());
        wxPen scaledPen(areAllPointsSelected ?
            wxPen(*wxBLACK,GetPen().GetWidth(),wxPENSTYLE_DOT) : *wxBLACK_PEN);
        scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
        wxDCPenChanger pc(dc, scaledPen);
        for (const auto& point : m_points)
            {
            /*if all points selected, then the current pen is the selected one already*/
            if (!areAllPointsSelected &&
                point.IsSelected())
                {
                wxDCPenChanger pc2(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                point.Draw(dc);
                }
            else
                { point.Draw(dc); }
            }
        return GetBoundingBox(dc);
        }

    //-------------------------------------------
    wxRect Point2D::Draw(wxDC& dc) const
        {
        if (!IsShown() || !IsOk())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }
        if (GetAnchorPoint().IsFullySpecified())
            {
            wxDCBrushChanger bc(dc, GetBrush());
            const auto boundingBox = GetBoundingBox(dc);
            auto boxRect = boundingBox;
            const auto midPoint = wxPoint(boundingBox.GetLeftTop()+(boundingBox.GetSize()/2));
            const auto iconRadius{ ScaleToScreenAndCanvas(GetRadius())};
            wxPoint polygonPoints[6];
            switch (m_shape)
                {
                case IconShape::CircleIcon:
                    dc.DrawCircle(boundingBox.GetLeftTop()+(boundingBox.GetSize()/2),
                                  ScaleToScreenAndCanvas(GetRadius()));
                    break;
                case IconShape::SquareIcon:
                    dc.DrawRectangle(boundingBox);
                    break;
                case IconShape::HorizontalLineIcon:
                    dc.DrawLine(wxPoint(boundingBox.GetLeft(), boundingBox.GetTop()+boundingBox.GetHeight()/2),
                                wxPoint(boundingBox.GetRight(), boundingBox.GetTop()+boundingBox.GetHeight()/2));
                    break;
                case IconShape::ArrowRightIcon:
                    GraphItems::Polygon::DrawArrow(dc,
                                wxPoint(boundingBox.GetLeft(), boundingBox.GetTop()+boundingBox.GetHeight()/2),
                                wxPoint(boundingBox.GetRight(), boundingBox.GetTop()+boundingBox.GetHeight()/2),
                                ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSize()));
                    break;
                case IconShape::TriangleUpwardIcon:
                    polygonPoints[0] = midPoint + wxPoint(0, -iconRadius);
                    polygonPoints[1] = midPoint + wxPoint(-iconRadius, iconRadius);
                    polygonPoints[2] = midPoint + wxPoint(iconRadius, iconRadius);
                    dc.DrawPolygon(3, polygonPoints);
                    break;
                case IconShape::TriangleDownwardIcon:
                    polygonPoints[0] = midPoint + wxPoint(0, iconRadius);
                    polygonPoints[1] = midPoint + wxPoint(-iconRadius, -iconRadius);
                    polygonPoints[2] = midPoint + wxPoint(iconRadius, -iconRadius);
                    dc.DrawPolygon(3, polygonPoints);
                    break;
                case IconShape::TriangleRightIcon:
                    polygonPoints[0] = midPoint + wxPoint(iconRadius, 0);
                    polygonPoints[1] = midPoint + wxPoint(-iconRadius, iconRadius);
                    polygonPoints[2] = midPoint + wxPoint(-iconRadius, -iconRadius);
                    dc.DrawPolygon(3, polygonPoints);
                    break;
                case IconShape::TriangleLeftIcon:
                    polygonPoints[0] = midPoint + wxPoint(-iconRadius, 0);
                    polygonPoints[1] = midPoint + wxPoint(iconRadius, iconRadius);
                    polygonPoints[2] = midPoint + wxPoint(iconRadius, -iconRadius);
                    dc.DrawPolygon(3, polygonPoints);
                    break;
                case IconShape::DiamondIcon:
                    polygonPoints[0] = midPoint + wxPoint(0, -iconRadius);
                    polygonPoints[1] = midPoint + wxPoint(iconRadius, 0);
                    polygonPoints[2] = midPoint + wxPoint(0, iconRadius);
                    polygonPoints[3] = midPoint + wxPoint(-iconRadius, 0);
                    dc.DrawPolygon(4, polygonPoints);
                    break;
                case IconShape::CrossIcon:
                    {
                    wxDCPenChanger dpc2(dc, wxPen(wxPenInfo(dc.GetBrush().GetColour(),
                                        dc.GetPen().GetWidth() * 2)));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                                wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                                wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
                    }
                    break;
                case IconShape::AsteriskIcon:
                    {
                    wxDCPenChanger dpc2(dc, wxPen(wxPenInfo(dc.GetBrush().GetColour(),
                                        dc.GetPen().GetWidth() * 2)));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                        wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                        wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(iconRadius, iconRadius)),
                        wxPoint(wxPoint(midPoint + wxPoint(-iconRadius, -iconRadius))));
                    dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, iconRadius)),
                        wxPoint(wxPoint(midPoint + wxPoint(iconRadius, -iconRadius))));
                    }
                    break;
                case IconShape::HexagonIcon:
                    polygonPoints[0] = midPoint + wxPoint(-iconRadius/2, -iconRadius);
                    polygonPoints[1] = midPoint + wxPoint(-iconRadius, 0);
                    polygonPoints[2] = midPoint + wxPoint(-iconRadius/2, iconRadius);
                    polygonPoints[3] = midPoint + wxPoint(iconRadius/2, iconRadius);
                    polygonPoints[4] = midPoint + wxPoint(iconRadius, 0);
                    polygonPoints[5] = midPoint + wxPoint(iconRadius/2, -iconRadius);
                    dc.DrawPolygon(6, polygonPoints);
                    break;
                case IconShape::BlankIcon:
                    // don't draw anything
                    break;
                case IconShape::BoxPlotIcon:
                    // whisker
                    dc.DrawLine(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetTop()),
                        wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2), boxRect.GetBottom()));
                    dc.DrawLine(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2) - boxRect.GetWidth() / 4, boxRect.GetTop()),
                        wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2) + boxRect.GetWidth() / 4, boxRect.GetTop()));
                    dc.DrawLine(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2) - boxRect.GetWidth() / 4, boxRect.GetBottom()),
                        wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2) + boxRect.GetWidth() / 4, boxRect.GetBottom()));
                    boxRect.y += (boxRect.GetHeight() / 2) - (boxRect.GetHeight() / 4); // center
                    boxRect.SetHeight(boxRect.GetHeight() / 2);
                    dc.DrawRectangle(boxRect);
                    // median line
                    dc.DrawLine(wxPoint(boxRect.GetLeft(), boxRect.GetTop() + (boxRect.GetHeight() / 2)),
                        wxPoint(boxRect.GetRight(), boxRect.GetTop() + (boxRect.GetHeight() / 2)));
                    break;
                case IconShape::ImageIcon:
                    if (m_iconImage && m_iconImage->IsOk())
                        {
                        const auto downScaledSize = geometry::calculate_downscaled_size(
                            std::make_pair<double, double>(m_iconImage->GetWidth(), m_iconImage->GetHeight()),
                            std::make_pair<double, double>(boundingBox.GetWidth(), boundingBox.GetHeight()));
                        const wxImage scaledImg = m_iconImage->Scale(downScaledSize.first, downScaledSize.second, wxIMAGE_QUALITY_HIGH);
                        dc.DrawBitmap(wxBitmap(scaledImg), boundingBox.GetTopLeft());
                        }
                    break;
                default:
                    dc.DrawCircle(GetBoundingBox(dc).GetLeftTop()+(GetBoundingBox(dc).GetSize()/2),
                                  ScaleToScreenAndCanvas(GetRadius()));
                };
            }
        return GetBoundingBox(dc);
        }
    }
