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
            `GraphItemBase`-derived classes; rather, it is used by these classes
            as a way to draw commonly used shapes.
        @note This class is used by Point2D and Label objects and not meant to be
            used by client code. Prefer using Point2D for drawing icons on a graph
            and Shape or FillableShape for drawing on a Canvas.*/
    class ShapeRenderer
        {
    public:
        /// @private
        using DrawFunction = void (ShapeRenderer::*)(const wxRect rect, wxDC& dc) const;

        /// @private
        friend class Shape;

        /** @brief Constructor.
            @param itemInfo Extended information (e.g., brush, pen) to draw with.\n
                Which features are used are shape dependent.
            @param img An image to use for the shape if using IconShape::ImageIcon.*/
        explicit ShapeRenderer(const GraphItemInfo& itemInfo,
                               const wxBitmapBundle* img = nullptr) :
            m_graphInfo(itemInfo),
            m_iconImage(img)
            {}
        /// @brief Gets/sets the shape's underlying information (e.g., brush color, pen, etc.).
        /// @details This is useful for changing the shape's settings when preparing to
        ///     draw different shapes.
        /// @returns The shape's information.
        [[nodiscard]] GraphItemInfo& GetGraphItemInfo() noexcept
            { return m_graphInfo; }
        /// @private
        [[nodiscard]] const GraphItemInfo& GetGraphItemInfo() const noexcept
            { return m_graphInfo; }

        /// @name Shape Rendering Functions
        /// @brief Functions for drawing shapes within a provided rectangle.
        /// @{

        /// @brief Draws a circle filled with the shape's brush, draws a black
        ///     outline, and draws the shape's text value in the center of it.
        /// @param rect The area to draw the circle within.
        /// 
        /// @param brush The brush for the sign's background.
        /// @param text The text to draw on the sign.
        /// @param dc The DC to draw to.
        void DrawCircularSign(const wxRect rect, const wxBrush& brush, const wxString& text, wxDC& dc) const;
        /// @brief Draws a yellow sun shape (circle with sunbeams).
        /// @param rect The area to draw the sun within.
        /// @param dc The DC to draw to.
        void DrawSun(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a purple flower shape (stigma and petals).
        /// @param rect The area to draw the flower within.
        /// @param dc The DC to draw to.
        void DrawFlower(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a orange red leaf.
        /// @param rect The area to draw the leaf within.
        /// @param dc The DC to draw to.
        void DrawFallLeaf(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        /// @param side The side of the object that the curly braces are enclosing.\n
        ///     For example, @c Left means that the left curly braces will be drawn
        ///     (enclosing what is to the right of them),
        ///     where @c Top will draw curly braces that are opening downward
        ///     (enclosing what is beneath it).
        void DrawCurlyBrace(const wxRect rect, wxDC& dc, const Side side) const;
        /// @brief Draws a left curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawLeftCurlyBrace(const wxRect rect, wxDC& dc) const
            { DrawCurlyBrace(rect, dc, Side::Left); }
        /// @brief Draws a right curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawRightCurlyBrace(const wxRect rect, wxDC& dc) const
            { DrawCurlyBrace(rect, dc, Side::Right); }
        /// @brief Draws a top curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawTopCurlyBrace(const wxRect rect, wxDC& dc) const
            { DrawCurlyBrace(rect, dc, Side::Top); }
        /// @brief Draws a bottom curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawBottomCurlyBrace(const wxRect rect, wxDC& dc) const
            { DrawCurlyBrace(rect, dc, Side::Bottom); }
        /// @brief Draws a square.
        /// @param rect The area to draw the square within.
        /// @param dc The DC to draw to.
        void DrawSquare(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a circle.
        /// @param rect The area to draw the circle within.
        /// @param dc The DC to draw to.
        void DrawCircle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a horizontal line.
        /// @param rect The area to draw the line within.
        /// @param dc The DC to draw to.
        void DrawHorizontalLine(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a box plot.
        /// @param rect The area to draw the box plot within.
        /// @param dc The DC to draw to.
        void DrawBoxPlot(wxRect rect, wxDC& dc) const;
        /// @brief Draws an asterisk.
        /// @param rect The area to draw the asterisk within.
        /// @param dc The DC to draw to.
        void DrawAsterisk(wxRect rect, wxDC& dc) const;
        /// @brief Draws a plus sign.
        /// @param rect The area to draw the plus within.
        /// @param dc The DC to draw to.
        void DrawPlus(wxRect rect, wxDC& dc) const;
        /// @brief Draws a diamond.
        /// @param rect The area to draw the diamond within.
        /// @param dc The DC to draw to.
        void DrawDiamond(wxRect rect, wxDC& dc) const;
        /// @brief Draws a upward triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawUpwardTriangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a downward triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawDownwardTriangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a right triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawRightTriangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a left triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawLeftTriangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a hexagon.
        /// @param rect The area to draw the hexagon within.
        /// @param dc The DC to draw to.
        void DrawHexagon(wxRect rect, wxDC& dc) const;
        /// @brief Draws a right-pointing arrow.
        /// @param rect The area to draw the arrow within.
        /// @param dc The DC to draw to.
        void DrawRightArrow(wxRect rect, wxDC& dc) const;
        /// @brief Draws a warning road sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawWarningRoadSign(wxRect rect, wxDC& dc) const;
        /// @brief Draws a "Go" road sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawGoSign(wxRect rect, wxDC& dc) const;
        /// @brief Draws a geo marker.
        /// @param rect The area to draw the marker within.
        /// @param dc The DC to draw to.
        void DrawGeoMarker(wxRect rect, wxDC& dc) const;
        /// @brief Draws an image.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawImage(wxRect rect, wxDC& dc) const;
        /// @brief Draws a male outline.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawMale(wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFemale(wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline (business skirt).
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFemaleBusiness(wxRect rect, wxDC& dc) const;
        /// @}
    private:
        /// @brief Sets the base color (if in use), performs the provided rendering lambda,
        ///     sets the brush, then runs the rendering lambda again.
        void DrawWithBaseColorAndBrush(wxDC& dc, const std::function<void(void)>& fn) const;
        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        /// @note @c percentFromLeft can be negative if using it for Bezier control points
        ///     that need to go a little outside of the rect.
        [[nodiscard]] double GetXPosFromLeft(const wxRect rect,
                                             const double percentFromLeft) const
            { return rect.GetLeft() + (rect.GetWidth() * percentFromLeft); };
        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]] double GetYPosFromTop(const wxRect rect,
                                            const double percentFromTop) const
            { return rect.GetTop() + (rect.GetHeight() * percentFromTop); };

        /// @brief Mirrors percents passed to GetXPosFromLeft() or GetYPosFromTop().
        [[nodiscard]] constexpr double Mirror(const double percent) const noexcept
            { return 1.0 - percent; }

        /// @returns The midpoint of a rect.
        [[nodiscard]] wxPoint GetMidPoint(const wxRect rect) const
            { return rect.GetLeftTop() + wxPoint(rect.GetWidth()/2, rect.GetHeight()/2); }

        /// @returns The radius of the largest circle that can fit in a rect.
        /// @note This is floored to be conservative.
        [[nodiscard]] double GetRadius(const wxRect rect) const
            { return std::floor(safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 2)); }

        [[nodiscard]] double ScaleToScreenAndCanvas(const double value) const noexcept
            { return value * GetScaling() * GetDPIScaleFactor(); }

        [[nodiscard]] double DownscaleFromScreenAndCanvas(const double value) const noexcept
            { return safe_divide(value, (GetScaling() * GetDPIScaleFactor())); }

        [[nodiscard]] double GetScaling() const noexcept
            { return m_graphInfo.GetScaling(); }

        [[nodiscard]] double GetDPIScaleFactor() const noexcept
            {
            wxASSERT_LEVEL_2_MSG(m_graphInfo.GetDPIScaleFactor().has_value(),
                                 L"Shape should have a proper DPI scaling.");
            return m_graphInfo.GetDPIScaleFactor().value_or(1);
            }

        GraphItemInfo m_graphInfo;
        const wxBitmapBundle* m_iconImage{ nullptr };
        };

    /** @brief Draws a shape onto a canvas.*/
    class Shape : public GraphItemBase
        {
    public:
         /** @brief Constructor.
             @param itemInfo Base information for the shape.
             @param shape The icon shape to draw.
             @param sz The size of the shape (in DIPs).
             @param img An image to use for the shape if using IconShape::ImageIcon.*/
        Shape(const GraphItems::GraphItemInfo& itemInfo,
              const Icons::IconShape shape,
              const wxSize sz,
              const wxBitmapBundle* img = nullptr);
        /// @private
        Shape(const Shape&) = delete;
        /// @private
        Shape(Shape&&) = delete;
        /// @private
        Shape& operator==(const Shape&) = delete;
        /// @private
        Shape& operator==(Shape&&) = delete;
        /** @brief Bounds the shape to the given rectangle.
            @param rect The rectangle to bound the shape to.
            @param dc This parameter is ignored.
            @param parentScaling This parameter is ignored.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;
        /** @brief Draws the shape onto the given DC.
            @param dc The DC to render onto.
            @returns The box that the shape is being drawn in.*/
        wxRect Draw(wxDC& dc) const override;
        /** @brief Draws the shape onto the given DC within a given rect.
            @details This is the main drawing routine and should be used by derived classes.
            @param drawRect The rect to draw within.
            @param dc The DC to render onto.*/
        void Draw(const wxRect& drawRect, wxDC& dc) const;
        /// @returns What type of shape is being drawn.
        Icons::IconShape GetShape() const noexcept
            { return m_shape; }
        /// @returns The size in DIPs.
        wxSize GetSizeDIPS() const noexcept
            { return m_sizeDIPs; }
    protected:
        /// @returns The rectangle on the canvas where the shape would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
        /** @brief Moves the shaoe by the specified x and y values.
            @param xToMove the amount to move horizontally.
            @param yToMove the amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) noexcept final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /// @returns The renderer.
        ShapeRenderer& GetRenderer() noexcept
            { return m_renderer; }
    private:
        [[nodiscard]] GraphItemInfo& GetGraphItemInfo() final
            {
            m_rendererNeedsUpdating = true;
            return GraphItemBase::GetGraphItemInfo();
            }
        void SetDPIScaleFactor(const double scaling) final
            {
            GraphItemBase::SetDPIScaleFactor(scaling);
            m_renderer.m_graphInfo.DPIScaling(scaling);
            }
        void SetScaling(const double scaling) final
            {
            GraphItemBase::SetScaling(scaling);
            m_renderer.m_graphInfo.Scaling(scaling);
            }
        /** @returns @c true if the given point is inside of the shape.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            { return GetBoundingBox(dc).Contains(pt); }

        wxSize m_shapeSizeDIPs{ 0, 0 };
        wxSize m_sizeDIPs{ 0, 0 };
        Icons::IconShape m_shape;
        mutable ShapeRenderer m_renderer;
        mutable bool m_rendererNeedsUpdating{ true };
        ShapeRenderer::DrawFunction m_drawFunction{ nullptr };
        };
    };

/** @}*/

#endif //__WISTERIA_SHAPES_H__
