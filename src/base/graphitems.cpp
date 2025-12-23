///////////////////////////////////////////////////////////////////////////////
// Name:        graphitems.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "graphitems.h"
#include "image.h"
#include "label.h"

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    double GraphItemBase::GetDPIScaleFactor() const
        {
        wxASSERT_MSG(m_itemInfo.m_dpiScaleFactor.has_value(),
                     L"Graph item should have a proper DPI scaling.");
        return (m_itemInfo.m_dpiScaleFactor.has_value() ? m_itemInfo.m_dpiScaleFactor.value() : 1);
        }

    //-------------------------------------------
    void GraphItemBase::DrawSelectionLabel(wxDC& dc, const double scaling,
                                           const wxRect boundingBox /*= wxRect()*/) const
        {
        if (IsSelected() && IsShowingLabelWhenSelected() && !GetText().empty())
            {
            const wxRect itemBoundingBox(GetBoundingBox(dc));
            GraphItems::Label selectionLabel(
                GraphItemInfo(GetGraphItemInfo())
                    .Scaling(scaling)
                    .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                    .DPIScaling(GetDPIScaleFactor())
                    .Padding(2, 2, 2, 2)
                    .FontBackgroundColor(Colors::ColorBrewer::GetColor(Colors::Color::White))
                    .Anchoring(Anchoring::Center)
                    .AnchorPoint(
                        itemBoundingBox.GetTopLeft() +
                        wxPoint(itemBoundingBox.GetWidth() / 2, itemBoundingBox.GetHeight() / 2)));
            selectionLabel.GetFont().MakeSmaller();
            const wxRect selectionLabelBox = selectionLabel.GetBoundingBox(dc);
            // if going out of the bottom of the bounding box then move it up to fit
            if (!boundingBox.IsEmpty() && selectionLabelBox.GetBottom() > boundingBox.GetBottom())
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

    //-------------------------------------------
    wxBitmap GraphItemBase::ToBitmap(wxDC& dc) const
        {
        const wxRect boundingBox = GetBoundingBox(dc).Inflate(ScaleToScreenAndCanvas(3));
        wxBitmap bmp{ boundingBox.GetWidth(), boundingBox.GetHeight(), 32 };
        Image::SetOpacity(bmp, wxALPHA_TRANSPARENT, false);

            {
            wxMemoryDC memDc(bmp);
            memDc.SetLogicalOrigin(boundingBox.GetPosition().x, boundingBox.GetPosition().y);
            Draw(memDc);
            }

        return bmp;
        }
    } // namespace Wisteria::GraphItems
