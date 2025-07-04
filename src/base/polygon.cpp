///////////////////////////////////////////////////////////////////////////////
// Name:        polygon.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "polygon.h"
#include "shapes.h"

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    void Polygon::SetPoints(const wxPoint* polygon, const size_t N)
        {
        if (N > 0)
            {
            m_points.assign(polygon, polygon + N);
            UpdatePointPositions();
            }
        else
            {
            m_points.clear();
            m_scaledPoints.clear();
            }
        }

    //-------------------------------------------
    bool Polygon::IsRectInsideRect(const wxRect innerRect, const wxRect outerRect)
        {
        return (outerRect.Contains(innerRect.GetTopLeft()) &&
                outerRect.Contains(innerRect.GetTopRight()) &&
                outerRect.Contains(innerRect.GetBottomLeft()) &&
                outerRect.Contains(innerRect.GetBottomRight()));
        }

    //-------------------------------------------
    std::pair<double, double> Polygon::GetPercentInsideRect(const wxRect innerRect,
                                                            const wxRect outerRect)
        {
        if (IsRectInsideRect(innerRect, outerRect))
            {
            return std::make_pair(1.0, 1.0);
            }

        int widthDiffLeft{ 0 }, widthDiffRight{ 0 };
        int heightDiffTop{ 0 }, heightDiffBottom{ 0 };

        if (innerRect.GetLeft() < outerRect.GetLeft())
            {
            widthDiffLeft = outerRect.GetLeft() - innerRect.GetLeft();
            }
        if (innerRect.GetRight() > outerRect.GetRight())
            {
            widthDiffRight = innerRect.GetRight() - outerRect.GetRight();
            }
        if (innerRect.GetTop() < outerRect.GetTop())
            {
            heightDiffTop = outerRect.GetTop() - innerRect.GetTop();
            }
        if (innerRect.GetBottom() > outerRect.GetBottom())
            {
            heightDiffBottom = innerRect.GetBottom() - outerRect.GetBottom();
            }

        const int widthDiff = widthDiffLeft + widthDiffRight;
        const int heightDiff = heightDiffTop + heightDiffBottom;

        return std::make_pair(
            safe_divide<double>(innerRect.GetWidth() - widthDiff, innerRect.GetWidth()),
            safe_divide<double>(innerRect.GetHeight() - heightDiff, innerRect.GetHeight()));
        }

    //-------------------------------------------
    bool Polygon::IsInsidePolygon(const wxPoint p, const wxPoint* polygon, const int N)
        {
        assert(N > 0 && polygon);
        if (N == 0 || polygon == nullptr)
            {
            return false;
            }

        // cross points count of x
        int crossPointsCount{ 0 };

        // neighbor bound vertices

        // left vertex
        wxPoint p1 = polygon[0];

        // check all rays
        for (int i = 1; i <= N; ++i)
            {
            constexpr bool BOUND{ true };
            // point is a vertex
            if (p == p1)
                {
                return BOUND;
                }

            // right vertex
            wxPoint p2 = polygon[i % N];

            // ray is outside our interests
            if (p.y < std::min(p1.y, p2.y) || p.y > std::max(p1.y, p2.y))
                {
                // next ray left point
                p1 = p2;
                continue;
                }

            // ray is crossing over by the algorithm (common part of)
            if (p.y > std::min(p1.y, p2.y) && p.y < std::max(p1.y, p2.y))
                {
                // x is before of ray
                if (p.x <= std::max(p1.x, p2.x))
                    {
                    // overlies on a horizontal ray
                    if (p1.y == p2.y && p.x >= std::min(p1.x, p2.x))
                        {
                        return BOUND;
                        }

                    // ray is vertical
                    if (p1.x == p2.x)
                        {
                        // overlies on a ray
                        if (p1.x == p.x)
                            {
                            return BOUND;
                            }
                        // before ray
                        ++crossPointsCount;
                        }

                    // cross point on the left side
                    else
                        {
                        // cross point of x
                        const auto xinters = ((p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y)) + p1.x;

                        // overlies on a ray
                        if (constexpr double DOUBLE_EPSILON{ .01f };
                            std::fabs(p.x - xinters) < DOUBLE_EPSILON)
                            {
                            return BOUND;
                            }

                        // before ray
                        if (p.x < xinters)
                            {
                            ++crossPointsCount;
                            }
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
                    const wxPoint& p3 = polygon[(i + 1) % N];

                    // p.y lies between p1.y & p3.y
                    if (p.y >= std::min(p1.y, p3.y) && p.y <= std::max(p1.y, p3.y))
                        {
                        ++crossPointsCount;
                        }
                    else
                        {
                        crossPointsCount += 2;
                        }
                    }
                }

            // next ray left point
            p1 = p2;
            }

        // EVEN
        if (crossPointsCount % 2 == 0)
            {
            return false;
            }
        // ODD
        else
            {
            return true;
            }
        }

    //-------------------------------------------
    wxRect Polygon::GetPolygonBoundingBox(const wxPoint* polygon, const size_t N)
        {
        assert(N > 0 && polygon);
        if (N == 0 || polygon == nullptr)
            {
            return {};
            }

        wxCoord minX(polygon[0].x), maxX(polygon[0].x), minY(polygon[0].y), maxY(polygon[0].y);
        for (size_t i = 1; i < N; ++i)
            {
            minX = std::min(polygon[i].x, minX);
            maxX = std::max(polygon[i].x, maxX);
            minY = std::min(polygon[i].y, minY);
            maxY = std::max(polygon[i].y, maxY);
            }
        return { wxPoint(minX, minY), wxPoint(maxX, maxY) };
        }

    //-------------------------------------------
    wxRect Polygon::GetPolygonBoundingBox(const std::vector<wxPoint>& polygon)
        {
        assert(!polygon.empty());
        if (polygon.empty())
            {
            return {};
            }

        wxCoord minX(polygon[0].x), maxX(polygon[0].x), minY(polygon[0].y), maxY(polygon[0].y);
        for (const auto& pt : polygon)
            {
            minX = std::min(pt.x, minX);
            maxX = std::max(pt.x, maxX);
            minY = std::min(pt.y, minY);
            maxY = std::max(pt.y, maxY);
            }
        return { wxPoint(minX, minY), wxPoint(maxX, maxY) };
        }

    //-------------------------------------------
    void Polygon::UpdatePointPositions()
        {
        m_scaledPoints = GetPoints();
        if (!IsFreeFloating())
            {
            return;
            }
        for (auto& scaledPoint : m_scaledPoints)
            {
            scaledPoint = (scaledPoint * GetScaling());
            } // grow
        }

    //-------------------------------------------
    bool Polygon::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        return IsInsidePolygon(pt, m_scaledPoints.data(), m_scaledPoints.size());
        }

    //-------------------------------------------
    void Polygon::GetRectPoints(const wxRect& rect, wxPoint* points)
        {
        if (points == nullptr)
            {
            return;
            }
        points[0] = rect.GetTopLeft();
        points[1] = rect.GetTopRight();
        points[2] = rect.GetBottomRight();
        points[3] = rect.GetBottomLeft();
        }

    //-------------------------------------------
    void Polygon::GetRectPoints(const wxRect& rect, std::array<wxPoint, 4>& points)
        {
        points[0] = rect.GetTopLeft();
        points[1] = rect.GetTopRight();
        points[2] = rect.GetBottomRight();
        points[3] = rect.GetBottomLeft();
        }

    //-------------------------------------------
    wxRect Polygon::GetRectFromPoints(const wxPoint* points)
        {
        if (points == nullptr)
            {
            return {};
            }
        decltype(points[0].x) lowestX{ points[0].x }, lowestY{ points[0].y },
            highestX{ points[0].x }, highestY{ points[0].y };
        for (size_t i = 0; i < 4; ++i)
            {
            lowestX = std::min(lowestX, points[i].x);
            lowestY = std::min(lowestY, points[i].y);
            highestX = std::max(highestX, points[i].x);
            highestY = std::max(highestY, points[i].y);
            }
        return { wxPoint(lowestX, lowestY), wxPoint(highestX, highestY) };
        }

    //-------------------------------------------
    wxRect Polygon::Draw(wxDC& dc) const
        {
        if (!IsShown())
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

        const wxRect boundingBox = GetBoundingBox(dc);

        wxPen scaledPen(GetPen().IsOk() ? GetPen() : *wxTRANSPARENT_PEN);
        scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));
        const bool penIsLight{ (scaledPen.GetColour().IsOk() &&
                                Wisteria::Colors::ColorContrast::IsLight(scaledPen.GetColour())) };
        const wxDCPenChanger pc(dc, IsSelected() ? wxPen(penIsLight ? *wxWHITE : *wxBLACK,
                                                         2 * scaledPen.GetWidth(), wxPENSTYLE_DOT) :
                                                   scaledPen);

        // don't use manual outline drawing unless one side is explicitly turned off
        const bool usingCustomOutline = (!GetGraphItemInfo().IsShowingTopOutline() ||
                                         !GetGraphItemInfo().IsShowingRightOutline() ||
                                         !GetGraphItemInfo().IsShowingBottomOutline() ||
                                         !GetGraphItemInfo().IsShowingLeftOutline());

        assert(!(GetShape() == PolygonShape::WaterColorRectangle && !GetBrush().IsOk()) &&
               L"Brush must be set when using watercolor-filled rectangle!");
        assert(!(GetShape() == PolygonShape::ThickWaterColorRectangle && !GetBrush().IsOk()) &&
               L"Brush must be set when using watercolor-filled rectangle!");

        // using a color fill (possibly a gradient)
        if (GetBackgroundFill().IsOk() &&
            // use solid color for these
            (GetShape() != PolygonShape::WaterColorRectangle) &&
            (GetShape() != PolygonShape::ThickWaterColorRectangle))
            {
            const wxDCBrushChanger bc(dc, GetBackgroundFill().GetColor1());
            if (GetBackgroundFill().IsGradient())
                {
                const auto theRect{ GetRectFromPoints(m_scaledPoints.data()) };
                // Optimized for rectangle.
                // Also, this enables the draw commands of the gradient to be
                // translated into SVG properly.
                if (GetShape() == PolygonShape::Rectangle)
                    {
                        // draw color area
                        {
                        dc.GradientFillLinear(
                            theRect, GetBackgroundFill().GetColor1(),
                            GetBackgroundFill().GetColor2(),
                            (GetBackgroundFill().GetDirection() == FillDirection::North ? wxNORTH :
                             GetBackgroundFill().GetDirection() == FillDirection::East  ? wxEAST :
                             GetBackgroundFill().GetDirection() == FillDirection::West  ? wxWEST :
                                                                                          wxSOUTH));
                        const wxDCBrushChanger bc2(dc, *wxTRANSPARENT_BRUSH);
                        const wxDCPenChanger pc2(dc,
                                                 (usingCustomOutline ? wxNullPen : dc.GetPen()));
                        dc.DrawRectangle(theRect);
                        }
                    // draw the outline
                    if (usingCustomOutline && dc.GetPen().IsOk())
                        {
                        if (GetGraphItemInfo().IsShowingTopOutline())
                            {
                            dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetTopRight());
                            }
                        if (GetGraphItemInfo().IsShowingRightOutline())
                            {
                            dc.DrawLine(boundingBox.GetTopRight(), boundingBox.GetBottomRight());
                            }
                        if (GetGraphItemInfo().IsShowingBottomOutline())
                            {
                            dc.DrawLine(boundingBox.GetBottomRight(), boundingBox.GetBottomLeft());
                            }
                        if (GetGraphItemInfo().IsShowingLeftOutline())
                            {
                            dc.DrawLine(boundingBox.GetBottomLeft(), boundingBox.GetTopLeft());
                            }
                        }
                    }
                else if (GetShape() == PolygonShape::GlassyRectangle)
                    {
                    const bool isVertical =
                        (GetBackgroundFill().GetDirection() == FillDirection::South ||
                         GetBackgroundFill().GetDirection() == FillDirection::North);
                    // fill with the color
                    dc.GradientFillLinear(theRect, GetBackgroundFill().GetColor1(),
                                          GetBackgroundFill().GetColor1().ChangeLightness(140),
                                          isVertical ? wxSOUTH : wxEAST);
                    // create a shiny overlay
                    dc.GradientFillLinear(
                        wxRect(theRect.GetX(), theRect.GetY(),
                               isVertical ? theRect.GetWidth() :
                                            theRect.GetWidth() * math_constants::quarter,
                               isVertical ? theRect.GetHeight() * math_constants::quarter :
                                            theRect.GetHeight()),
                        GetBackgroundFill().GetColor1().ChangeLightness(115),
                        GetBackgroundFill().GetColor1().ChangeLightness(155),
                        isVertical ? wxSOUTH : wxEAST);
                    }
                // a spline doesn't use a fill color, so just draw it
                else if (GetShape() == PolygonShape::Spline)
                    {
                    dc.DrawSpline(m_scaledPoints.size(), m_scaledPoints.data());
                    }
                // irregular polygon
                else
                    {
                    if (dc.IsKindOf(wxCLASSINFO(wxGCDC)))
                        {
                        wxGraphicsContext* gc = dynamic_cast<wxGCDC&>(dc).GetGraphicsContext();
                        assert(gc && L"Failed to get graphics context from polygon renderer!");
                        if (gc != nullptr)
                            {
                            wxPoint start;
                            wxPoint stop;
                            switch (GetBackgroundFill().GetDirection())
                                {
                            case FillDirection::East:
                                start = boundingBox.GetTopLeft() +
                                        wxPoint(0, (boundingBox.GetHeight() / 2));
                                stop = boundingBox.GetTopRight() +
                                       wxPoint(0, (boundingBox.GetHeight() / 2));
                                break;
                            case FillDirection::West:
                                start = boundingBox.GetTopRight() +
                                        wxPoint(0, (boundingBox.GetHeight() / 2));
                                stop = boundingBox.GetTopLeft() +
                                       wxPoint(0, (boundingBox.GetHeight() / 2));
                                break;
                            case FillDirection::North:
                                start = boundingBox.GetBottomLeft() +
                                        wxPoint((boundingBox.GetWidth() / 2), 0);
                                stop = boundingBox.GetTopLeft() +
                                       wxPoint((boundingBox.GetWidth() / 2), 0);
                                break;
                            case FillDirection::South:
                            default:
                                start = boundingBox.GetTopLeft() +
                                        wxPoint((boundingBox.GetWidth() / 2), 0);
                                stop = boundingBox.GetBottomLeft() +
                                       wxPoint((boundingBox.GetWidth() / 2), 0);
                                break;
                                };
                            gc->SetBrush(gc->CreateLinearGradientBrush(
                                start.x, start.y, stop.x, stop.y, GetBackgroundFill().GetColor1(),
                                GetBackgroundFill().GetColor2()));
                            dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                            }
                        }
                    else
                        {
                        dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                        }
                    }
                }
            else
                {
                dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                }
            }

        // Using the brush.
        // Note that we can use a brush on top of a color-filled background,
        // like a hatched brush on top of white background.
        if (GetBrush().IsOk() || (IsSelected() && GetSelectionBrush().IsOk()))
            {
            const wxDCBrushChanger bc(dc, (IsSelected() && GetSelectionBrush().IsOk()) ?
                                              GetSelectionBrush() :
                                              GetBrush());
            if (GetShape() == PolygonShape::Spline)
                {
                dc.DrawSpline(m_scaledPoints.size(), m_scaledPoints.data());
                }
            else if (GetShape() == PolygonShape::Rectangle &&
                     GetBoxCorners() == BoxCorners::Rounded)
                {
                dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius());
                }
            else if (GetShape() == PolygonShape::Rectangle)
                {
                    // draw brush area
                    {
                    const wxDCPenChanger pc2(dc, (usingCustomOutline ? wxNullPen : dc.GetPen()));
                    dc.DrawRectangle(boundingBox);
                    }
                // draw the outline
                if (usingCustomOutline && dc.GetPen().IsOk())
                    {
                    if (GetGraphItemInfo().IsShowingTopOutline())
                        {
                        dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetTopRight());
                        }
                    if (GetGraphItemInfo().IsShowingRightOutline())
                        {
                        dc.DrawLine(boundingBox.GetTopRight(), boundingBox.GetBottomRight());
                        }
                    if (GetGraphItemInfo().IsShowingBottomOutline())
                        {
                        dc.DrawLine(boundingBox.GetBottomRight(), boundingBox.GetBottomLeft());
                        }
                    if (GetGraphItemInfo().IsShowingLeftOutline())
                        {
                        dc.DrawLine(boundingBox.GetBottomLeft(), boundingBox.GetTopLeft());
                        }
                    }
                }
            else if (GetShape() == PolygonShape::CurvyRectangle && m_scaledPoints.size() == 10)
                {
                GraphicsContextFallback gcf{ &dc, boundingBox };
                auto* gc = gcf.GetGraphicsContext();
                assert(gc && L"Failed to get graphics context for curvy rectangle!");
                // If drawing commands can't be used, then switch to drawing as
                // a regular polygon. These shapes often overlap each other (e.g., Sankey diagram),
                // so using bitmaps won't work.
                if (gc != nullptr && !gcf.IsFallingBackToBitmap())
                    {
                    // save current transform matrix state
                    gc->PushState();

                    auto outlinePath = gc->CreatePath();

                    outlinePath.MoveToPoint(m_scaledPoints[0].x, m_scaledPoints[0].y);
                    outlinePath.AddCurveToPoint(m_scaledPoints[1].x, m_scaledPoints[1].y,
                                                m_scaledPoints[3].x, m_scaledPoints[3].y,
                                                m_scaledPoints[4].x, m_scaledPoints[4].y);
                    outlinePath.AddLineToPoint(m_scaledPoints[5].x, m_scaledPoints[5].y);
                    outlinePath.AddCurveToPoint(m_scaledPoints[6].x, m_scaledPoints[6].y,
                                                m_scaledPoints[8].x, m_scaledPoints[8].y,
                                                m_scaledPoints[9].x, m_scaledPoints[9].y);
                    outlinePath.AddLineToPoint(m_scaledPoints[0].x, m_scaledPoints[0].y);

                    gc->FillPath(outlinePath);
                    gc->StrokePath(outlinePath);

                    // restore transform matrix
                    gc->PopState();
                    }
                else
                    {
                    dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                    }
                }
            else if (GetShape() == PolygonShape::WaterColorRectangle)
                {
                const GraphItems::Shape sh(GetGraphItemInfo(),
                                           Icons::IconShape::WaterColorRectangle,
                                           boundingBox.GetSize());
                sh.Draw(boundingBox, dc);
                }
            else if (GetShape() == PolygonShape::ThickWaterColorRectangle)
                {
                const GraphItems::Shape sh(GetGraphItemInfo(),
                                           Icons::IconShape::ThickWaterColorRectangle,
                                           boundingBox.GetSize());
                sh.Draw(boundingBox, dc);
                }
            else
                {
                dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                }
            }
        // just drawing an outline (hasn't already be drawn with a background color above)
        else if (!GetBackgroundFill().IsOk())
            {
            const wxDCBrushChanger bc(dc, *wxTRANSPARENT_BRUSH);
            if (GetShape() == PolygonShape::Spline)
                {
                dc.DrawSpline(m_scaledPoints.size(), m_scaledPoints.data());
                }
            else
                {
                dc.DrawPolygon(m_scaledPoints.size(), m_scaledPoints.data());
                }
            }

        // highlight the selected bounding box in debug mode
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
            {
            if (IsSelected())
                {
                std::array<wxPoint, 5> debugOutline;
                GetRectPoints(boundingBox, debugOutline.data());
                debugOutline[4] = debugOutline[0];
                const wxDCPenChanger pcDebug(
                    dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
                dc.DrawLines(debugOutline.size(), debugOutline.data());
                }
            }

        if (GetClippingRect())
            {
            dc.DestroyClippingRegion();
            }
        return boundingBox;
        }

    //------------------------------------------------------
    void Polygon::DrawArrow(wxDC& dc, const wxPoint pt1, const wxPoint pt2,
                            const wxSize arrowHeadSize)
        {
        assert(arrowHeadSize.IsFullySpecified() && L"Arrowhead size not fully specified.");
        if (!arrowHeadSize.IsFullySpecified())
            {
            return;
            }
        const auto dx = static_cast<float>(pt2.x - pt1.x);
        const auto dy = static_cast<float>(pt2.y - pt1.y);
        const auto length = std::sqrt(dx * dx + dy * dy);

        // ux,uy is a unit vector parallel to the line.
        const auto ux = safe_divide(dx, length);
        const auto uy = safe_divide(dy, length);

        // vx,vy is a unit vector perpendicular to ux,uy
        const auto vx = -uy;
        const auto vy = ux;

        const auto halfWidth = math_constants::half * arrowHeadSize.GetWidth();

        const std::array<wxPoint, 3> arrowHead{
            { pt2,
              wxPoint(std::round(pt2.x - arrowHeadSize.GetHeight() * ux + halfWidth * vx),
                      std::round(pt2.y - arrowHeadSize.GetHeight() * uy + halfWidth * vy)),
              wxPoint(std::round(pt2.x - arrowHeadSize.GetHeight() * ux - halfWidth * vx),
                      std::round(pt2.y - arrowHeadSize.GetHeight() * uy - halfWidth * vy)) }
        };

        // The end of the line should be going underneath the head by just one pixel,
        // so that it doesn't poke out under the point of the arrowhead.
        // Note that this only works if pointing perfectly left or right; otherwise,
        // we just have to connect the end of the line to the end of the arrowhead.
        const wxCoord xAdjustment =
            (pt1.y == pt2.y && pt1.x <= pt2.x) ? (-(arrowHeadSize.GetWidth()) + 1) :
            (pt1.y == pt2.y && pt1.x > pt2.x)  ? ((arrowHeadSize.GetWidth()) - 1) :
                                                 0;

        dc.DrawLine(pt1, wxPoint(pt2.x + xAdjustment, pt2.y));
        // fill the arrowhead with the same color as the line
        const wxDCBrushChanger bc(dc, dc.GetPen().GetColour());
        // need to turn off the pen because a thicker pen will cause an odd-looking
        // effect when the two lines converge at the tip of the arrowhead
        const wxDCPenChanger pc(dc, *wxTRANSPARENT_PEN);
        dc.DrawPolygon(arrowHead.size(), arrowHead.data());
        }

    //-------------------------------------------
    void Polygon::Offset(const int xToMove, const int yToMove)
        {
        for (auto& point : m_points)
            {
            point += wxPoint(xToMove, yToMove);
            }
        }

    //-------------------------------------------
    void Polygon::SetBoundingBox([[maybe_unused]] const wxRect& rect, [[maybe_unused]] wxDC& dc,
                                 [[maybe_unused]] const double parentScaling)
        {
        assert(!IsFreeFloating() && L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            {
            return;
            }
        wxFAIL_MSG(L"SetBoundingBox() not currently supported!");
        }
    } // namespace Wisteria::GraphItems
