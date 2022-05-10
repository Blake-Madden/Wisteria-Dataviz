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

using namespace Wisteria::Colors;

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
            selectionLabel.GetFont().MakeSmaller();
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
    }
