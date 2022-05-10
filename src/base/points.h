/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_POINTS_H__
#define __WISTERIA_POINTS_H__

#include "graphitems.h"

namespace Wisteria::GraphItems
    {
    /** @brief A single point that can be drawn on a canvas.*/
    class Point2D final : public GraphItemBase
        {
        friend class Points2D;
        friend class Graphs::Graph2D;
        friend class Wisteria::Canvas;
    public:
        /// @private
        Point2D() = default;
        /// @brief Constructor.
        /// @param pt The anchor point.
        explicit Point2D(const wxPoint pt)
            { SetAnchorPoint(pt); }
        /** @brief Constructor.
            @param itemInfo Base information for the plot object.
                Should include the center point and optionally the scaling, brush color, and
                selection label.
            @param radius The radius of the point. This is a DIP value that will be scaled
                by the object's scaling and parent window's DPI scale factor.
            @param shape The shape of the point.
            @param img An image to use for the point if point is using IconShape::ImageIcon.
            @warning Some icon shapes (@c ImageWholeLegend, @c ColorGradientIcon) are not
                applicable here and will be drawn as a circle instead if used.*/
        Point2D(const GraphItems::GraphItemInfo& itemInfo,
                const size_t radius,
                const IconShape& shape = IconShape::CircleIcon,
                const wxBitmapBundle* img = nullptr) :
            GraphItemBase(itemInfo), m_shape(shape),
            m_iconImage(img), m_radius(radius)
            {}
        /// @returns The radius of the point. The radius is the distance from the center point
        ///  to outside of the circle.
        /// @warning This needs to be scaled when called for measuring and rendering.
        [[nodiscard]] size_t GetRadius() const noexcept
            { return m_radius; }
        /** @brief Sets the radius of the point.
            @param radius The radius of the point. This is a pixel value that the framework will
                scale to the screen for you.*/
        void SetRadius(const size_t radius) noexcept
            { m_radius = radius; }
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
    private:
        /** @returns `true` if the given point is inside of this point.
            @note This does a hit test within a bounding box of the point, not the point itself.
                So it may return `true` if slightly at the corner outside of the point.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Draws the point.
            @param dc The canvas to draw the point on.
            @returns The box that the point is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns `true` if center point is valid.
        [[nodiscard]] bool IsOk() const noexcept
            { return GetAnchorPoint().IsFullySpecified(); }
        /** @brief Moves the point by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /** @brief Bounds the point to the given rectangle.
            @param rect The rectangle to bound the point to.
            @param parentScaling This parameter is ignored.
            @note The scaling of the point will be adjusted to this box.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;

        IconShape m_shape{ IconShape::CircleIcon };
        const wxBitmapBundle* m_iconImage{ nullptr };
        size_t m_radius{ 4 };
        };

    /** @brief Class to manage and render multiple points.
        @details This is useful for grouping points together (i.e., categorized data)
            or building a line of longitudinal points.
            Whether a line connecting the points can be specified, as well as
            which style to draw the line as.
        @note The points added to this collection can include more individual
            granularity (such as shape and color).*/
    class Points2D final : public GraphItemBase
        {
        friend class Graphs::Graph2D;
        friend class Wisteria::Canvas;
    public:
        /** @brief Constructor.
            @param pen The pen to draw the line connecting the points.
                Set to @c wxNullPen to not connect the points.
            @note The color and shapes of the points are controlled on
                the individual point level.
            @sa AddPoint().*/
        explicit Points2D(const wxPen& pen)
            { GetPen() = pen; }
        /// @returns The points in this collection.
        [[nodiscard]] const std::vector<Point2D>& GetPoints() const noexcept
            { return m_points; }
        /// @brief Reserves memory for a specified number of points.
        /// @param size The number of points to reserve space for.
        void Reserve(const size_t size)
            { m_points.reserve(size); }
        /** @brief Adds a point to the collection.
            @details The point's canvas and scaling will be implicitly set to the
                collection's canvas and scaling.
            @param pt The point to add.
            @param dc Measurement DC. Not used, just needed for API requirements.
            @note To not actually draw the point, set its shape to BlankIcon.*/
        void AddPoint(Point2D pt, wxDC& dc);
        /** @brief Sets whether selecting the points collection will select the
                individual point that was clicked on or all the points.
            @param singlePointSelect Whether to select the last hit point.*/
        void SetSinglePointSelection(const bool singlePointSelect) noexcept
            { m_singlePointSelection = singlePointSelect; }

        /** @brief Sets whether the points should be bound to a plot's coordinate system
                or float on the canvas.
            @details In other words, the points are not connected to axis coordinates
                on a particular plot, but rather sit arbitrarily on the canvas and has to have
                their coordinates adjusted as the canvas gets rescaled.
                This is meant for movable objects on a canvas that a client can manually move.
            @param freeFloat Whether the points should be free floating.*/
        void SetFreeFloating(const bool freeFloat) final
            {
            GraphItemBase::SetFreeFloating(freeFloat);
            for (auto& point : m_points)
                { point.SetFreeFloating(freeFloat); }
            }
        /// @returns How the segments between the points on a line are connected.
        [[nodiscard]] LineStyle GetLineStyle() const noexcept
            { return m_lineStyle; }
        /// @brief How the segments between the points on a line are connected.
        /// @param lineStyle The line style.
        void SetLineStyle(const LineStyle lineStyle) noexcept
            { m_lineStyle = lineStyle; }
        /** @brief Sets the scaling of the points. As a canvas grows or shrinks,
                this can be adjusted to make the rendering of lines/text/etc. fit appropriately.
            @param scaling The scaling to use.*/
        void SetScaling(const double scaling) final
            {
            GraphItemBase::SetScaling(scaling);
            for (auto& point : m_points)
                { point.SetScaling(scaling); }
            }
        /// @brief Sets the DPI scale factor.
        /// @param scaling The DPI scaling.
        void SetDPIScaleFactor(const double scaling) noexcept final
            {
            GraphItemBase::SetDPIScaleFactor(scaling);
            for (auto& point : m_points)
                { point.SetDPIScaleFactor(scaling); }
            }
    private:
        /** @brief Sets whether the points are selected.
            @param selected Whether the last hit point
                (or all points if there was no previous hit) should be selected.*/
        void SetSelected(const bool selected) override;
        /** @brief Draws the selected points' labels.
            @param dc The DC to render with.
            @param scaling The scaling to draw the text with. This may be different from
                the scaling used by the element itself, depending on what the scaling is of the caller.
            @param boundingBox The bounding box to constrain the label inside of.
                Default is an empty rect, which will cause this parameter to be ignored.*/
        void DrawSelectionLabel(wxDC& dc, const double scaling,
                                const wxRect boundingBox = wxRect()) const final;
        /** @warning Should not be called. Points should be explicitly set at
                specific coordinates, and cannot be scaled to fit in an arbitrary bounding box.
                This is only included to fulfill the interface contract.
            @param rect This parameter is ignored.
            @param parentScaling This parameter is ignored.*/
        [[deprecated("Not implemented")]]
        void SetBoundingBox([[maybe_unused]] const wxRect& rect,
                            [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final
            { wxFAIL_MSG(L"SetBoundingBox() not supported for Points2D objects."
                            "Points should be explicitly set at specific coordinates, "
                            "and cannot be scaled to fit in an arbitrary bounding box."); }
        /** @brief Draws the points, using the pen and brush connected to this object.
            @param dc The device context to draw to.
            @returns The area that the points are being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            {
            wxRect boundingBox(m_boundingBox.GetTopLeft(),
                                wxSize(m_boundingBox.GetWidth()*GetScaling(),
                                m_boundingBox.GetHeight()*GetScaling()));
            if (IsFreeFloating())
                {
                boundingBox.Offset((boundingBox.GetLeftTop()*GetScaling()) -
                                    boundingBox.GetLeftTop());
                }
            return boundingBox;
            }
        /** @brief Moves the points by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            {
            for (auto& point : m_points)
                { point.Offset(xToMove,yToMove); }
            m_boundingBox.Offset(wxPoint(xToMove, yToMove));
            }
        /** @returns `true` if the given point is inside any of the points in this collection.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final;
        std::vector<Point2D> m_points;
        mutable std::vector<Point2D>::size_type m_lastHitPointIndex
            { static_cast<std::vector<Point2D>::size_type>(-1) };
        /* Note that we don't use the base class's cached bounding box logic because
            GetBoundingBox() doesn't calculate anything. Instead, we manage a bounding box
            internally whenever a point is added.*/
        wxRect m_boundingBox{ wxPoint(wxDefaultCoord, wxDefaultCoord), wxSize(0, 0) };
        bool m_singlePointSelection{ true };
        LineStyle m_lineStyle{ LineStyle::Lines };

        long m_currentAssignedId{ 0 };
        };
    };

/** @}*/

#endif //__WISTERIA_POINTS_H__
