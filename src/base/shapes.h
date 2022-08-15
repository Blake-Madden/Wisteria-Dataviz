/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_SHAPES_H__
#define __WISTERIA_SHAPES_H__

#include "graphitems.h"
#include "colorbrewer.h"
#include "polygon.h"

namespace Wisteria::GraphItems
    {
    /** @brief Helper class to draw shapes.
        @details This class accepts a GraphItemInfo object, which will be used
            by the @c DrawXX() functions.\n
            This class is not an embeddable object placed on a canvas like
            `GraphItemBase`-deirived classes; rather, it is used by these classes
            as a way to draw commonly used shapes.
        @note This class is used by Point2D and Label objects and not meant to be
            used by client code. Prefer using Point2D for drawing icons on a graph.*/
    class ShapeRenderer
        {
    public:
        /** @brief Constructor.
            @param itemInfo Extended information (e.g., brush, pen) to draw with.\n
                Which features are used are shape dependent.
            @param colorScheme The color scheme to use for shapes reqiring multiple colors.*/
        explicit ShapeRenderer(const GraphItemInfo& itemInfo,
                               std::shared_ptr<Colors::Schemes::ColorScheme> colorScheme = nullptr) :
            m_graphInfo(itemInfo),
            m_colorScheme(colorScheme != nullptr ? colorScheme :
                Settings::GetDefaultColorScheme())
            {}
        /// @brief Gets/sets the shape's underlying information (e.g., brush color, pen, etc.).
        /// @details This is useful for changing the shape's settings when preparing to
        ///     draw different shapes.
        /// @returns The shape's information.
        [[nodiscard]] GraphItemInfo& GetGraphItemInfo() noexcept
            { return m_graphInfo; }

        /// @name Shape Rendering Functions
        /// @brief Functions for drawing shapes within a provided rectangle.
        /// @{

        /// @brief Draws a circle filled with the shape's brush, draws a black
        ///     outline, and draws the shape's text value in the center of it.
        /// @param rect The area to draw the circle within.
        /// @param dc The DC to draw to.
        void DrawCircularSign(const wxRect rect, wxDC& dc);
        /// @brief Draws a yellow sun shape (circle with sunbeams).
        /// @param rect The area to draw the sun within.
        /// @param dc The DC to draw to.
        void DrawSun(const wxRect rect, wxDC& dc);
        /// @brief Draws a purple flower shape (stigma and petals).
        /// @param rect The area to draw the flower within.
        /// @param dc The DC to draw to.
        void DrawFlower(const wxRect rect, wxDC& dc);
        /// @brief Draws a orange red leaf.
        /// @param rect The area to draw the leaf within.
        /// @param dc The DC to draw to.
        void DrawFallLeaf(const wxRect rect, wxDC& dc);
        /// @brief Draws curly braces.
        /// @param rect The area to draw the curly braces within.
        /// @param dc The DC to draw to.
        /// @param side The side of the object that the curly braces are enclosing.\n
        ///     For example, @c Left means that the left curly braces will be drawn,
        ///     where @c Top will draw curly braces that are opening downward
        ///     (enclosing what is beneath it).
        void DrawCurlyBraces(const wxRect rect, wxDC& dc, const Side side);
        /// @brief Draws a square.
        /// @param rect The area to draw the square within.
        /// @param dc The DC to draw to.
        void DrawSquare(const wxRect rect, wxDC& dc);
        /// @}
    private:
        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        [[nodiscard]] double GetXPosFromLeft(const wxRect rect, const double percentFromLeft) noexcept
            {
            wxASSERT_MSG(percentFromLeft >= 0.0 && percentFromLeft <= 1.0,
                         L"Percent should be between 0.0 and 1.0!");
            return rect.GetLeft() + (rect.GetWidth() * percentFromLeft);
            };
        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]] double GetYPosFromTop(const wxRect rect, const double percentFromLeft) noexcept
            {
            wxASSERT_MSG(percentFromLeft >= 0.0 && percentFromLeft <= 1.0,
                         L"Percent should be between 0.0 and 1.0!");
            return rect.GetTop() + (rect.GetHeight() * percentFromLeft);
            };
        [[nodiscard]] double GetScaling() const noexcept
            { return m_graphInfo.GetScaling(); }

        [[nodiscard]] double GetDPIScaleFactor() const noexcept
            {
            wxASSERT_LEVEL_2_MSG(m_graphInfo.GetDPIScaleFactor().has_value(),
                                 L"Shape should have a proper DPI scaling.");
            return (m_graphInfo.GetDPIScaleFactor().has_value() ?
                    m_graphInfo.GetDPIScaleFactor().value() : 1);
            }

        [[nodiscard]] double ScaleToScreenAndCanvas(const double value) const noexcept
            { return value * GetScaling() * GetDPIScaleFactor(); }

        GraphItemInfo m_graphInfo;
        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme{ nullptr };
        };

    /** @brief Draws a shape on a canvas.
        @todo Only a few icons are currently supported.*/
    class Shape final : public GraphItemBase
        {
    public:
         /** @brief Constructor.
             @param itemInfo Base information for the shape.
             @param shape The icon shape to draw.
             @param sz The size of the shape (in DIPs).*/
        Shape(const GraphItems::GraphItemInfo& itemInfo,
              const Icons::IconShape shape,
              const wxSize sz) :
            GraphItemBase(itemInfo), m_shape(shape), m_shapeSizeDIPs(sz), m_sizeDIPs(sz)
            {}
    private:
        /** @brief Draws the shape onto the given DC.
            @param dc The DC to render onto.
            @returns The box that the shape is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the shape would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
        /** @brief Bounds the shape to the given rectangle.
            @param rect The rectangle to bound the shape to.
            @param parentScaling This parameter is ignored.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;
        /** @brief Moves the shaoe by the specified x and y values.
            @param xToMove the amount to move horizontally.
            @param yToMove the amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) noexcept final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /** @returns @c true if the given point is inside of the shape.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            { return GetBoundingBox(dc).Contains(pt); }

        wxSize m_shapeSizeDIPs{ 0, 0 };
        wxSize m_sizeDIPs{ 0, 0 };
        Icons::IconShape m_shape;
        };
    };

/** @}*/

#endif //__WISTERIA_SHAPES_H__
