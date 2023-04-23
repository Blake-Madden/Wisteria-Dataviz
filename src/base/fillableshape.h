/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_FILLABLE_SHAPE_H__
#define __WISTERIA_FILLABLE_SHAPE_H__

#include "shapes.h"
#include "image.h"

namespace Wisteria::GraphItems
    {
    /** @brief Draws a shape onto a canvas that appears to be partially filled.
        @details The lower portion of the shape (which will be the "filled" percent of it)
            will be drawn as normal. The remaining portion will be drawn above that, but
            will appear heavily translucent (i.e., appearing empty).\n
            The effect is achieved by making the brush translucent, while keeping the
            pen the same. This will result in showing an outline around the entire shape.\n
            Note that because of this, only shapes that make use of a customizable brush are
            recommended (i.e., not shapes like plus).*/
    class FillableShape final : public Shape
        {
    public:
         /** @brief Constructor.
             @param itemInfo Base information for the shape.
             @param shape The icon shape to draw.
             @param sz The size of the shape (in DIPs).
             @param fillPercent How much of the shape should appear filled.
                Should be a percent (@c 0.0 to @c 1.0),
             @param img An image to use for the point if point is using IconShape::ImageIcon.*/
        FillableShape(const GraphItems::GraphItemInfo& itemInfo,
              const Icons::IconShape shape,
              const wxSize sz,
              const double fillPercent,
              const wxBitmapBundle* img = nullptr) :
            Shape(itemInfo, shape, sz, img),
            m_fillPercent(std::clamp(fillPercent, math_constants::empty, math_constants::full))
            {
            // a valid brush is needed, so fall back to black if one is not provided
            if (!itemInfo.GetBrush().IsOk())
                {
                GetBrush() = *wxBLACK_BRUSH;
                GetRenderer().GetGraphItemInfo().Brush(*wxBLACK_BRUSH);
                }
            }
        /// @private
        FillableShape(const FillableShape&) = delete;
        /// @private
        FillableShape(FillableShape&&) = delete;
        /// @private
        FillableShape& operator==(const FillableShape&) = delete;
        /// @private
        FillableShape& operator==(FillableShape&&) = delete;
        /** @brief Draws the shape onto the given DC.
            @param dc The DC to render onto.
            @returns The box that the shape is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
    private:
        double m_fillPercent{ math_constants::empty };
        };
    };

/** @}*/

#endif //__WISTERIA_FILLABLE_SHAPE_H__
