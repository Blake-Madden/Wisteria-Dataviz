///////////////////////////////////////////////////////////////////////////////
// Name:        lines.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "points.h"
#include "label.h"
#include "polygon.h"
#include "shapes.h"

using namespace Wisteria::Colors;

namespace Wisteria::GraphItems
    {
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
            // update previous selections
            // (this is needed if the parent graph needed to recreate this collection)
            for (auto& pt : m_points)
                {
                if (GetSelectedIds().find(pt.GetId()) != GetSelectedIds().cend())
                    { pt.SetSelected(selected); }
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

        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

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
        for (const auto& point : m_points)
            {
            /*if all points selected, then the current pen is the selected one already*/
            if (!areAllPointsSelected &&
                point.IsSelected())
                {
                Point2D pt = point;
                pt.GetPen().SetStyle(wxPENSTYLE_DOT);
                pt.Draw(dc);
                }
            else
                { point.Draw(dc); }
            }

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }
        return GetBoundingBox(dc);
        }

    //-------------------------------------------
    wxRect Point2D::Draw(wxDC& dc) const
        {
        if (!IsShown() || !IsOk())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }

        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

        if (GetAnchorPoint().IsFullySpecified())
            {
            wxDCBrushChanger bc(dc, GetBrush());
            wxPen scaledPen{ GetPen() };
            if (scaledPen.IsOk())
                { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth())); }
            wxDCPenChanger pc(dc, scaledPen);
            const auto boundingBox = GetBoundingBox(dc);
            auto boxRect = boundingBox;
            const auto midPoint = wxPoint(boundingBox.GetLeftTop()+(boundingBox.GetSize()/2));
            const auto iconRadius{ ScaleToScreenAndCanvas(GetRadius())};
            wxPoint polygonPoints[6];
            // object that can handle drawing various shapes for the icons.
            Shapes sh(GraphItemInfo().Brush(GetBrush()).
                Scaling(GetScaling()).
                DPIScaling(GetDPIScaleFactor()));
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
                                ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSizeDIPs()));
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
                case IconShape::WarningRoadSign:
                    // sign post
                        {
                        wxPoint pt[2] =
                            {
                            boundingBox.GetTopLeft() +
                                // top of post is in the middle of the sign
                                // so that pen cap doesn't appear above sign
                                wxSize(boundingBox.GetWidth() / 2, iconRadius),
                            // bottom
                            boundingBox.GetBottomLeft() +
                                wxSize(boundingBox.GetWidth() / 2, 0)
                            };
                        // white outline of sign post
                            {
                            wxDCPenChanger pc2(dc,
                                wxPen(*wxWHITE, ScaleToScreenAndCanvas((GetRadius() / 5) + 1)));
                            dc.DrawLine(pt[0], pt[1]);
                            }
                        // actual sign post
                            {
                            wxDCPenChanger pc2(dc,
                                wxPen(*wxBLACK, ScaleToScreenAndCanvas(GetRadius() / 5)));
                            dc.DrawLine(pt[0], pt[1]);
                            }
                        }
                    // sign
                        {
                        wxDCPenChanger pc2(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2)));
                        const auto circleCenter = boundingBox.GetLeftTop() +
                            wxSize(boundingBox.GetWidth() / 2, boundingBox.GetHeight() * .33);
                        polygonPoints[0] = circleCenter + wxPoint(0, -iconRadius);
                        polygonPoints[1] = circleCenter + wxPoint(iconRadius, 0);
                        polygonPoints[2] = circleCenter + wxPoint(0, iconRadius);
                        polygonPoints[3] = circleCenter + wxPoint(-iconRadius, 0);
                        dc.DrawPolygon(4, polygonPoints);
                        // ! label
                        Label bangLabel(GraphItemInfo(L"!").Pen(wxNullPen).
                            AnchorPoint(circleCenter).Anchoring(Anchoring::Center).
                            LabelAlignment(TextAlignment::Centered).
                            DPIScaling(GetDPIScaleFactor()));
                        bangLabel.SetFontColor(*wxBLACK);
                        bangLabel.GetFont().MakeBold();
                        bangLabel.SetBoundingBox(
                            wxRect(boundingBox.GetLeftTop(),
                                wxSize(boundingBox.GetWidth(), boundingBox.GetHeight() * .66)),
                            dc, GetScaling());
                        bangLabel.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                        bangLabel.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                        bangLabel.Draw(dc);
                        }
                    break;
                case IconShape::GoRoadSign:
                    // sign post
                        {
                        wxPoint pt[2] =
                            {
                            boundingBox.GetTopLeft() +
                                wxSize(boundingBox.GetWidth() / 2, iconRadius),
                            // bottom
                            boundingBox.GetBottomLeft() +
                                wxSize(boundingBox.GetWidth() / 2, 0)
                            };
                        // white outline of sign post
                            {
                            wxDCPenChanger pc2(dc,
                                wxPen(*wxWHITE, ScaleToScreenAndCanvas((GetRadius() / 5) + 1)));
                            dc.DrawLine(pt[0], pt[1]);
                            }
                        // actual sign post
                            {
                            wxDCPenChanger pc2(dc,
                                wxPen(*wxBLACK, ScaleToScreenAndCanvas(GetRadius() / 5)));
                            dc.DrawLine(pt[0], pt[1]);
                            }
                        }
                    // sign
                        {
                        const auto signRect = wxRect(boundingBox.GetLeftTop(),
                                                     wxSize(boundingBox.GetWidth(),
                                                            boundingBox.GetHeight() * .66));
                        sh.GetGraphItemInfo().Text(_(L"GO"));
                        sh.DrawCircularSign(signRect, dc);
                        }
                    break;
                case IconShape::SunIcon:
                        sh.DrawSun(boundingBox, dc);
                        break;
                case IconShape::LocationMarker:
                    // pin
                        {
                        wxDCBrushChanger bc2(dc, *wxBLACK_BRUSH);
                        wxDCPenChanger pc2(dc, wxPen(*wxWHITE, ScaleToScreenAndCanvas(1)));
                        wxPoint pt[3] =
                            {
                            boundingBox.GetTopLeft() +
                                        wxSize(boundingBox.GetWidth() * .25, boundingBox.GetHeight() / 2),
                            boundingBox.GetTopLeft() +
                                        wxSize(boundingBox.GetWidth() * .75, boundingBox.GetHeight() / 2),
                            // bottom
                            boundingBox.GetBottomLeft() + wxSize(boundingBox.GetWidth() / 2, 0)
                            };
                        dc.DrawPolygon(std::size(pt), pt);
                        }
                    // outer ring
                    dc.DrawCircle(boundingBox.GetLeftTop() +
                        wxSize(boundingBox.GetWidth() / 2, boundingBox.GetHeight() * .33),
                        ScaleToScreenAndCanvas(GetRadius()));
                    // secondary ring
                        {
                        wxDCPenChanger pc2(dc,
                            wxPen(ColorContrast::ShadeOrTint(GetBrush().GetColour(), .4),
                                  ScaleToScreenAndCanvas(1)));
                        wxDCBrushChanger bc2(dc, ColorContrast::ShadeOrTint(GetBrush().GetColour(), .4));
                        dc.DrawCircle(boundingBox.GetLeftTop() +
                            wxSize(boundingBox.GetWidth() / 2, boundingBox.GetHeight() * .33),
                            // 1/3 the size of outer ring
                            ScaleToScreenAndCanvas(GetRadius() * .5));
                        }
                    // white center
                    // (using a contrasting color just doesn't seem right here, these usually have white
                    //  centers, regardless of background color, on most real-world maps and whatnot)
                        {
                        wxDCPenChanger pc2(dc, *wxWHITE_PEN);
                        wxDCBrushChanger bc2(dc, *wxWHITE_BRUSH);
                        dc.DrawCircle(boundingBox.GetLeftTop() +
                            wxSize(boundingBox.GetWidth() / 2, boundingBox.GetHeight() * .33),
                            // 1/3 the size of outer ring
                            ScaleToScreenAndCanvas(GetRadius() * .33));
                        }
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
                            std::make_pair<double, double>(m_iconImage->GetDefaultSize().GetWidth(),
                                                           m_iconImage->GetDefaultSize().GetHeight()),
                            std::make_pair<double, double>(boundingBox.GetWidth(),
                                                           boundingBox.GetHeight()));
                        const wxBitmap scaledImg = m_iconImage->GetBitmap(wxSize(downScaledSize.first,
                                                                                 downScaledSize.second));
                        dc.DrawBitmap(scaledImg, boundingBox.GetTopLeft());
                        }
                    break;
                default:
                    dc.DrawCircle(GetBoundingBox(dc).GetLeftTop()+(GetBoundingBox(dc).GetSize()/2),
                                  ScaleToScreenAndCanvas(GetRadius()));
                };
            }
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
            {
            wxPoint debugOutline[5];
            GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), debugOutline);
            debugOutline[4] = debugOutline[0];
            wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
            dc.DrawLines(std::size(debugOutline), debugOutline);
            }

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }
        return GetBoundingBox(dc);
        }

    //-------------------------------------------
    wxRect Point2D::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        if (!IsOk())
            { return wxRect(); }
        wxPoint cp(GetAnchorPoint());
        if (IsFreeFloating())
            {
            cp.x *= GetScaling();
            cp.y *= GetScaling();
            }
        // convert center point to top left corner of area
        cp -= wxSize(ScaleToScreenAndCanvas(GetRadius()),
                        ScaleToScreenAndCanvas(GetRadius()));
        wxRect boundingBox(cp,
                            wxSize((ScaleToScreenAndCanvas(GetRadius())) * 2,
                                   (ScaleToScreenAndCanvas(GetRadius())) * 2));
        if (m_shape == IconShape::LocationMarker ||
            m_shape == IconShape::GoRoadSign ||
            m_shape == IconShape::WarningRoadSign)
            {
            boundingBox.SetTop(boundingBox.GetTop() - boundingBox.GetHeight());
            boundingBox.SetHeight(boundingBox.GetHeight() * 1.5);
            }
        return boundingBox;
        }
    }
