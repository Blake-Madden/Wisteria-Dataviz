/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_LINES_H__
#define __WISTERIA_LINES_H__

#include "graphitems.h"
#include <vector>

namespace Wisteria::Graphs
    {
    class Graph2D;
    }

namespace Wisteria::GraphItems
    {
    /** @brief Collection of lines to be drawn together (e.g., an axis's gridlines).
        @details Although meant for axes, this can also be used to draw an arbitrary
         series on lines on a canvas.
        @note The points in this collection are not all connected; rather, each
         pair of points are drawn as a separate line. For example, if there are six
         points, then three separate lines will be drawn.*/
    class Lines final : public GraphItems::GraphItemBase
        {
        /// Friend of Graph2D.
        friend class Wisteria::Graphs::Graph2D;
    public:
        /** @brief Constructor.
            @param pen The pen to draw the lines with.
            @param scaling The scaling factor to use when drawing.*/
        Lines(const wxPen& pen, const double scaling) : GraphItemBase(scaling, wxEmptyString)
            { GetPen() = pen; }
        /// @private
        Lines() = delete;
        /// @brief Removes all lines from the collection.
        void Clear() noexcept
            { m_lines.clear(); }
        /// @brief Reserves enough space for the specified number of lines.
        /// @param size The number of lines to reserve space for.
        void Reserve(const size_t size)
            { m_lines.reserve(size); }
        /** @brief Adds a new line.
            @param pt1 The first point of the line.
            @param pt2 The second point of the line.
            @note The points here refer to the physical coordinates on the parent canvas.*/
        void AddLine(const wxPoint pt1, const wxPoint pt2)
            { m_lines.emplace_back(pt1, pt2); }
        /// @returns Direct access to the lines. This is useful for directly filling them in.
        [[nodiscard]] std::vector<std::pair<wxPoint, wxPoint>>& GetLines() noexcept
            { return m_lines; }
        /// @returns How the segments between the points on a line are connected.
        [[nodiscard]] LineStyle GetLineStyle() const noexcept
            { return m_lineStyle; }
        /// @brief How the segments between the points on a line are connected.
        /// @param lineStyle The line style.
        void SetLineStyle(const LineStyle lineStyle) noexcept
            { m_lineStyle = lineStyle; }
        /** @brief Moves the points by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final;
    private:
        /** @brief Draws the points, using the pen and brush connected to this object.
            @param dc The device context to draw to.
            @returns The area that the points are being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
        /** @returns `true` if the given point is inside any of the points in this collection.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const final;
        /** @warning Do not call this function. It is only included because it's contractually
             required by base class and is not relevant to this object.*/
        [[deprecated("Not implemented")]]
        void SetBoundingBox([[maybe_unused]] const wxRect& rect,
                            [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final
            { wxFAIL_MSG(L"SetBoundingBox() not supported for Lines objects. "
                          "Points should be explicitly set at specific coordinates, "
                          "and cannot be scaled to fit in an arbitrary bounding box."); }
        std::vector<std::pair<wxPoint,wxPoint>> m_lines;
        LineStyle m_lineStyle{ LineStyle::Lines };
        };
    };

/** @}*/

#endif //__WISTERIA_LINES_H__
