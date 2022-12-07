/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_POLYGON_H__
#define __WISTERIA_POLYGON_H__

#include "graphitems.h"

namespace Wisteria::GraphItems
    {
    /** @brief A polygon that can be drawn on a canvas.
        @todo Add shadow support.
        @par Scaling
        The scaling controls how the polygon grows when a free floating object.
        The following demonstrates this (anchoring here is set to Anchoring::TopLeftCorner):

        @par
        @htmlonly
        <?xml version="1.0" encoding="UTF-8"?>
        <svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
            <text x="0" y="20">0,0</text>
            <line x1="10" y1="30" x2="10" y2="115" style="stroke-width:=2;stroke:black"/>
            <line x1="10" y1="30" x2="70" y2="30" style="stroke-width:=2;stroke:black"/>
            <rect x="20" y="40" style="fill:#800080;fill-opacity:1;fill-rule:nonzero;stroke:none" width="50" height="75"/>

            <text x="130" y="20">0,0</text>
            <line x1="140" y1="30" x2="140" y2="190" style="stroke-width:=2;stroke:black"/>
            <line x1="140" y1="30" x2="250" y2="30" style="stroke-width:=2;stroke:black"/>
            <rect x="150" y="40" style="fill:#800080;fill-opacity:1;fill-rule:nonzero;stroke:none" width="100" height="150"/>
        </svg>
        @endhtmlonly

        By setting the scaling, the polygon will stretch itself so that its size adjusts
        to the canvas's scaling if free floating.

        If bound to a canvas or plot (the norm), then scaling will only affect the pen width.
        When canvas bound, the points of the polygon exclusively control where and how large
        the polygon is drawn.*/
    class Polygon final : public GraphItemBase
        {
        friend class Graphs::Graph2D;
        friend class Wisteria::Canvas;
    public:
        /// @brief Hints for how to draw the polygon.
        enum class PolygonShape
            {
            /// @brief A curved line.
            Spline,
            /// @brief A rectangle or square.
            Rectangle,
            /// @brief A rectangle or square with a glassy sheen.
            /// @note If using this shape, call SetBackgroundFill() to set the color and direction
            ///     of the glass effect. (The brush will be ignored.)
            GlassyRectangle,
            /// @brief A watercolor-like filled rectangle, where fill color is warped and the
            ///     rectangle looks like it was filled in with watercolor paint (or a marker).
            WaterColorRectangle,
            /// @brief No real shape.
            Irregular
            };
        /// @private
        Polygon()
            { GetGraphItemInfo().Outline(true, true, true, true); };
        /** @brief Constructor.
            @param itemInfo Base information for the plot object.
            @param points The points of the polygon.
            @param N The number of points.*/
        Polygon(const GraphItemInfo& itemInfo,
                const wxPoint* points, const int N) : GraphItemBase(itemInfo)
            { SetPoints(points, N); }

        /** @name Point & Shape Functions
            @brief Functions related to setting the points and shape of the polygon.*/
        /// @{

        /** @brief Specifies how to draw the polygon.
            @details Basically, this is a hint to optimize the drawing.
            @param shape The shape of the polygon.
            @note If using GlassyRectangle, call SetBackgroundFill() to set the color and
                direction of the glass effect. (The brush will be ignored.) */
        void SetShape(const PolygonShape shape) noexcept
            { m_polygonShape = shape; }
        /// @returns The polygon's shape.
        [[nodiscard]] PolygonShape GetShape() const noexcept
            { return m_polygonShape; }

        /** @brief Sets the points of the polygon.
            @param polygon The points in the polygon.
            @param N The number of points in the polygon.*/
        void SetPoints(const wxPoint* polygon, const size_t N);
        /// @returns The points in the polygon.
        [[nodiscard]] const std::vector<wxPoint>& GetPoints() const noexcept
            { return m_points; }
        /// @}

        /** @name Visual Effect Functions
            @brief Functions related to the polygon's visual effects.*/
        /// @{

        /** @brief Sets the "canvas" color of the shape.
            @details This is useful if you are painting with a translucent or hatched
                brush and you need a specific color to show underneath it
                (other than what is on the underlying DC).\n
                This also can be useful if using a color gradient, rather than a brush.
            @param fill The color information (a single color or gradient) of the polygon's canvas.
            @note If this is not specified, then whatever is being drawn under the
                polygon will appear under it (this would be the usual behavior).
            @sa GetBrush().*/
        void SetBackgroundFill(const Colors::GradientFill& fill) noexcept
            { m_backgroundFill = fill; }
        /// @returns The color underneath the polygon's brush.
        /// @sa GetBrush().
        [[nodiscard]] const Colors::GradientFill& GetBackgroundFill() const noexcept
            { return m_backgroundFill; }
        /// @returns How the corners are drawn.
        [[nodiscard]] BoxCorners GetBoxCorners() const noexcept
            { return m_boxCorners; }
        /** @brief Sets how the corners are drawn.
            @details Only relevant if shape is set to @c Rectangle and painting with a solid color.
            @param boxCorners The corner display to use.*/
        void SetBoxCorners(const BoxCorners boxCorners) noexcept
            { m_boxCorners = boxCorners; }
        /// @}

        /** @name Utility Functions
            @brief Helper functions for collision detection and shape calculations.*/
        /// @{

        /** @brief Alexander Motrichuk's implementation of determining if a point is
                inside of a polygon.
            @details Tests if a point is within a polygon (or on an edge or vertex)
                by shooting a ray along the X axis.
            @param p The point.
            @param polygon The polygon's points.
            @param N The number of points in the polygon.
            @returns Whether the point is inside of the polygon.*/
        [[nodiscard]] static bool IsInsidePolygon(const wxPoint p, const wxPoint* polygon,
                                                  const int N);
        /** @brief Determines if a rectangle entirely fits another rectangle.
            @param innerRect The smaller rect.
            @param outerRect the larger rect.
            @returns @c true if @c innerRect is inside of @c outerRect.*/
        [[nodiscard]] static bool IsRectInsideRect(const wxRect innerRect,
                                                   const wxRect outerRect);
        /** @brief Determines how much of a rectangle fits into another rectangle.
            @param innerRect The smaller rect.
            @param outerRect the larger rect.
            @returns A pair containing the percent of the width and height of @c innerRect
                that fits inside of @c outerRect.\n
                For example, if 3/4 of the smaller rect's width is inside of the larger rect
                and 1/2 of its height fits, then this will return @c 0.75 and @c 0.5.*/
        [[nodiscard]] static std::pair<double, double> GetPercentInsideRect(const wxRect innerRect,
                                                       const wxRect outerRect);
        /** @brief Draws a line from @c pt1 to @c pt2 with an arrowhead pointing at pt2.
            @details The line is drawn with the current pen and the arrowhead is filled
                with the current brush. Adapted from code by Adrian McCarthy.
            @param dc The device context to draw on.
            @param pt1 The starting point of the line.
            @param pt2 The ending point of the line
                (and where the arrowhead will be pointing at).
            @param arrowHeadSize The width and height of the arrowhead.*/
        static void DrawArrow(wxDC& dc, const wxPoint pt1, const wxPoint pt2,
                              const wxSize arrowHeadSize);
        /** @brief Shrinks a rectangle by a given scaling.
            @param theRect The rectangle to downscale.
            @param scaling Scale factor to scale it down. For example, 2 will
                downscale the rectangle to half its original size.
            @returns The downscaled rectangle.*/
        [[nodiscard]] inline static wxRect DownScaleRect(const wxRect& theRect,
                                                         const double scaling)
            {
            return wxRect(
                wxSize(safe_divide<double>(theRect.GetWidth(), scaling),
                       safe_divide<double>(theRect.GetHeight(), scaling)));
            }
        /** @brief Determines the four corners of a rectangle.
            @param rect The rectangle to analyze.
            @param[out] points The (4) points to store the rectangle's points into.
            @warning Make sure that @c points has 4 items in it.*/
        static void GetRectPoints(const wxRect& rect, wxPoint* points);
        /** @brief Determines the bounding box that a polygon requires to fit inside of.
            @param polygon The polygon's points.
            @param N The number of points in the polygon.
            @returns The rectangle that the polygon would need to fit in.
            @todo needs unit testing*/
        [[nodiscard]] static wxRect GetPolygonBoundingBox(const wxPoint* polygon,
                                                          const size_t N);
        /** @brief Determines the bounding box that a polygon requires to fit inside of.
            @param polygon The polygon's points.
            @returns The rectangle that the polygon would need to fit in.
            @todo needs unit testing*/
        [[nodiscard]] static wxRect GetPolygonBoundingBox(const std::vector<wxPoint>& polygon);

        /** @brief Converts a pair of doubles to a @c wxPoint.
            @param coordPair The two double values representing a point.
            @returns The double values as a @c wxPoint.*/
        [[nodiscard]] static wxPoint PairToPoint(const std::pair<double,
                                                    double>& coordPair) noexcept
            { return wxPoint(coordPair.first, coordPair.second); }
        /// @}
    private:
        /** @returns @c true if the given point is inside of this polygon.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final;
        /** @brief Draws the polygon.
            @param dc The canvas to draw the point on.
            @returns The box that the polygon is being drawn within.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            { return GetPolygonBoundingBox(&m_scaledPoints[0], m_scaledPoints.size()); }
        /** @brief Moves the polygon by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final;
        /** @brief Bounds the polygon to the given rectangle.
            @param rect The rectangle to bound the polygon to.
            @param parentScaling This parameter is not used in this implementation.
            @todo Add support for this; not currently implemented.*/
        void SetBoundingBox([[maybe_unused]] const wxRect& rect,
                            [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;
        /** @returns A rectangle from four points.
            @param points The four points to construct the rectangle.
            @warning It is assumed that there are four elements in @c points.*/
        [[nodiscard]] static wxRect GetRectFromPoints(const wxPoint* points);
        void UpdatePointPositions();
        std::vector<wxPoint> m_points;
        // secondary cache used for actual (i.e., scaled) bounding box
        std::vector<wxPoint> m_scaledPoints;
        Colors::GradientFill m_backgroundFill;
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        PolygonShape m_polygonShape{ PolygonShape::Irregular };
        };
    };

/** @}*/

#endif //__WISTERIA_POLYGON_H__
