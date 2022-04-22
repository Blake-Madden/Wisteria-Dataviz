/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_SPACER_H__
#define __WISTERIA_SPACER_H__

#include "graphitems.h"

namespace Wisteria::GraphItems
    {
    /** @brief Class for taking up an empty space on a canvas.
         This is useful for fitting other elements into a smaller section of the canvas.
        @note The size of this spacer will be determined by its parent canvas, which fits
         its rows to a uniform height. Also, note that this element should only be used as a fixed
         object on a canvas (refer to Canvas::SetFixedObject()).*/
    class Spacer final : public GraphItems::GraphItemBase
        {
    public:
        /** @brief Default Constructor.
            @note The parent canvas manages the final size and position of this element. All
             that the caller is responsible for is to place it into a Canvas's fixed-object grid.*/
        Spacer() : m_size(wxDefaultSize)
            {
            SetSelectable(false);
            if (Wisteria::Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                SetSelectable(true);
                SetText(L"DEBUG MSG: SPACER");
                }
            }
        /** @returns `true` if the given point is inside of the spacer.
            @param pt The point to check.
            @param dc The DC to measure content with.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Moves the item by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /** @brief Bounds the spacer to the given rectangle.
            @param rect The rectangle to bound the spacer to.
            @param dc The DC to measure content with.
            @param parentScaling This parameter is ignored.*/
        void SetBoundingBox(const wxRect& rect,
                            [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final
            {
            wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                                 L"SetBoundingBox() should only be called on fixed objects!");
            if (IsFreeFloating())
                { return; }
            SetAnchorPoint(rect.GetTopLeft());
            m_size = rect.GetSize()*safe_divide<double>(1.0f, GetScaling());
            }
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            { return GetBoundingBox(); }
        /// @returns The rectangle on the canvas where the spacer would fit in.
        [[nodiscard]] wxRect GetBoundingBox() const final
            {
            wxASSERT_LEVEL_2(m_size.IsFullySpecified());
            return wxRect(GetAnchorPoint(), m_size*GetScaling());
            }
        /** @returns The rectangle that the spacer is occupying.
            @param dc The dc to render the spacer on. Only used in debug mode.
            @note This element merely takes up space, so nothing is actually drawn.*/
        wxRect Draw([[maybe_unused]] wxDC& dc) const final
            {
            // highlight the selected bounding box in debug mode
            if (Wisteria::Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
                {
                wxPoint debugOutline[5]{ {0,0} };
                Polygon::GetRectPoints(GetBoundingBox(), debugOutline);
                debugOutline[4] = debugOutline[0];
                wxDCPenChanger pcBg(dc, wxPen(*wxRED, GetScaling()*2, wxPENSTYLE_SHORT_DASH));
                dc.DrawLines(std::size(debugOutline), debugOutline);
                }
            return GetBoundingBox();
            }
    private:
        wxSize m_size;
        };
    };

/** @}*/

#endif //__WISTERIA_SPACER_H__
