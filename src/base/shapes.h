/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SHAPES_H
#define WISTERIA_SHAPES_H

#include "changers.h"
#include "colorbrewer.h"
#include "graphitems.h"
#include <random>

namespace Wisteria::GraphItems
    {
    /** @brief Gets a @c wxGraphicsContext to draw to.
        @details If a provided @c wxDC supports @c GetGraphicsContext(),
            then that will be returned. Otherwise, a @c wxGraphicsContext
            pointing to an internal @c wxMemoryDC is returned, which
            can be rendered to by the client the same way.\n
            \n
            If using this fallback method, the rendering will be performed
            to a bitmap, which will be blitted to the original @c wxDC
            upon destruction of this object.*/
    class GraphicsContextFallback
        {
      public:
        /** @brief Takes a @c wxDC and a rectangle area within it, and creates a
                @c wxGraphicsContext to render to that area.
            @param dc A pointer to the original @c wxDC that is being drawn to.
            @param rect The rectangle on the @c wxDC that is being drawn to.
            @note Call GetGraphicsContext() after construction to get the graphics
                context that you can being drawing with.*/
        GraphicsContextFallback(wxDC* dc, const wxRect rect);
        /// @private
        GraphicsContextFallback(const GraphicsContextFallback&) = delete;
        /// @private
        GraphicsContextFallback& operator=(const GraphicsContextFallback&) = delete;

        /** @returns The @c wxGraphicsContext to render to.\n
                (May be null if a failure occurred).
            @note The returned @c wxGraphicsContext may point to the original @c wxDC, or it may
                be pointing to an internal bitmap that will be blitted to the @c wxDC when
                this object goes out of scope. Either way, just use the returned
                @c wxGraphicsContext to draw to and the rendering/blitting will be applied when
                this object goes out of scope.*/
        wxGraphicsContext* GetGraphicsContext() noexcept { return m_gc; }

        /// @private
        ~GraphicsContextFallback();

        /// @returns @c true if an advanced graphics context could not be acquired
        ///     and rendering will fall back to drawing to a bitmap.
        [[nodiscard]]
        bool IsFallingBackToBitmap() const noexcept
            {
            return m_drawingToBitmap;
            }

      private:
        wxGraphicsContext* m_gc{ nullptr };
        wxDC* m_dc{ nullptr };
        wxMemoryDC m_memDC;
        wxBitmap m_bmp;
        wxRect m_rect;
        bool m_drawingToBitmap{ false };
        };

    /** @brief Helper class to draw shapes.
        @details This class accepts a GraphItemInfo object, which will be used
            by the various @c Draw___() functions.\n
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
        explicit ShapeRenderer(GraphItemInfo itemInfo, const wxBitmapBundle* img = nullptr)
            : m_graphInfo(std::move(itemInfo)), m_iconImage(img)
            {
            }

        /// @private
        ShapeRenderer() = delete;

        /// @brief Gets/sets the shape's underlying information (e.g., brush color, pen, etc.).
        /// @details This is useful for changing the shape's settings when preparing to
        ///     draw different shapes.
        /// @returns The shape's information.
        [[nodiscard]]
        GraphItemInfo& GetGraphItemInfo() noexcept
            {
            return m_graphInfo;
            }

        /// @private
        [[nodiscard]]
        const GraphItemInfo& GetGraphItemInfo() const noexcept
            {
            return m_graphInfo;
            }

        /// @name Shape Rendering Functions
        /// @brief Functions for drawing shapes within a provided rectangle.
        /// @{

        /// @brief Draws a circle filled with the shape's brush, draws a black
        ///     outline, and draws the shape's text value in the center of it.
        /// @param rect The area to draw the circle within.
        /// @param brush The brush for the sign's background.
        /// @param text The text to draw on the sign.
        /// @param dc The DC to draw to.
        void DrawCircularSign(const wxRect rect, const wxBrush& brush, const wxString& text,
                              wxDC& dc) const;
        /// @brief Draws a yellow sun shape (circle with sunbeams).
        /// @param rect The area to draw the sun within.
        /// @param dc The DC to draw to.
        void DrawSun(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a purple flower shape (stigma and petals).
        /// @param rect The area to draw the flower within.
        /// @param dc The DC to draw to.
        void DrawFlower(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an orange red leaf.
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
            {
            DrawCurlyBrace(rect, dc, Side::Left);
            }

        /// @brief Draws a right curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawRightCurlyBrace(const wxRect rect, wxDC& dc) const
            {
            DrawCurlyBrace(rect, dc, Side::Right);
            }

        /// @brief Draws a top curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawTopCurlyBrace(const wxRect rect, wxDC& dc) const
            {
            DrawCurlyBrace(rect, dc, Side::Top);
            }

        /// @brief Draws a bottom curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        void DrawBottomCurlyBrace(const wxRect rect, wxDC& dc) const
            {
            DrawCurlyBrace(rect, dc, Side::Bottom);
            }

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
        void DrawBoxPlot(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an asterisk.
        /// @param rect The area to draw the asterisk within.
        /// @param dc The DC to draw to.
        void DrawAsterisk(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a plus sign.
        /// @param rect The area to draw the plus within.
        /// @param dc The DC to draw to.
        void DrawPlus(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a diamond.
        /// @param rect The area to draw the diamond within.
        /// @param dc The DC to draw to.
        void DrawDiamond(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an upward triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawUpwardTriangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a downward triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawDownwardTriangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a right triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawRightTriangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a left triangle.
        /// @param rect The area to draw the triangle within.
        /// @param dc The DC to draw to.
        void DrawLeftTriangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a hexagon.
        /// @param rect The area to draw the hexagon within.
        /// @param dc The DC to draw to.
        void DrawHexagon(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a right-pointing arrow.
        /// @param rect The area to draw the arrow within.
        /// @param dc The DC to draw to.
        void DrawRightArrow(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a banner sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawBanner(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a warning road sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawWarningRoadSign(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a "Go" road sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawGoSign(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a geo marker.
        /// @param rect The area to draw the marker within.
        /// @param dc The DC to draw to.
        void DrawGeoMarker(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an image.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawImage(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a male outline.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawMan(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawWoman(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline (business skirt).
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBusinessWoman(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a downward-pointing chevron.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawChevronDownward(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an upward-pointing chevron.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawChevronUpward(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a string.
        /// @note The pen color of the shape controls the font's color.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawText(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a tack.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawTack(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a rectangle that looks like it was painted with watercolor.
        /// @param rect The area to draw within.
        /// @param dc The DC to draw to.
        /// @note The color will more than likely go outside of the provided rectangle,
        ///     as that is the aesthetic that we are going for.\n
        ///     This can be negated, however, by calling `SetClippingRect()` for the Shape
        ///     object using this renderer.
        void DrawWaterColorRectangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a rectangle that looks like it was painted with watercolor,
        ///     painted with a second coat.
        /// @param rect The area to draw within.
        /// @param dc The DC to draw to.
        /// @note The color will more than likely go outside of the provided rectangle,
        ///     as that is the aesthetic that we are going for.\n
        ///     This can be negated, however, by calling `SetClippingRect()` for the Shape
        ///     object using this renderer.
        void DrawThickWaterColorRectangle(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a graduation cap with tassel.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawGraduationCap(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a book.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBook(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a car tire.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawTire(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a snowflake.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawSnowflake(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a newspaper.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawNewspaper(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a car.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCar(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a blackboard.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBlackboard(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a clock.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawClock(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a measuring ruler.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawRuler(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an IV bag.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawIVBag(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a thermometer showing a cold temperature.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawColdThermometer(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a thermometer showing a hot temperature.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHotThermometer(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a red apple.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawRedApple(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a green apple.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawGrannySmithApple(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a heart.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHeart(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an Immaculate Heart.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawImmaculateHeart(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a flame.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFlame(const wxRect rect, wxDC& dc) const;
        /// @brief Draws an office building.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawOffice(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a factory building.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFactory(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a house.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHouse(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a barn.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBarn(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a barn and grain silo.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFarm(const wxRect rect, wxDC& dc) const;
        /// @brief Draws a dollar bill.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawDollar(const wxRect rect, wxDC& dc) const;
        /// @}
      private:
        enum class Temperature
            {
            Hot,
            Cold
            };

        void DrawBaseBuilding(const wxRect rect, wxDC& dc, const wxColour color) const;
        void DrawThermometer(const wxRect rect, wxDC& dc, const Temperature temp) const;
        void DrawApple(const wxRect rect, wxDC& dc, const wxColour color) const;
        void DrawAsterisk(wxRect rect, wxGraphicsContext* gc) const;
        void DrawTire(wxRect rect, wxGraphicsContext* gc) const;
        /// @brief Sets the base color (if in use), performs the provided rendering lambda,
        ///     sets the brush, then runs the rendering lambda again.
        void DrawWithBaseColorAndBrush(wxDC& dc, const std::function<void(void)>& fn) const;

        /// @brief Offsets calls to GetXPosFromLeft(). This is useful for centering the rendering
        ///     of irregular (i.e., non-square) shapes inside of squares.
        void SetXOffsetPercentage(const double offset) const noexcept
            {
            m_xOffsetPercentage = offset;
            }

        /// @brief Offsets calls to GetYPosFromLeft(). This is useful for centering the rendering
        ///     of irregular (i.e., non-square) shapes inside of squares.
        void SetYOffsetPercentage(const double offset) const noexcept
            {
            m_yOffsetPercentage = offset;
            }

        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        /// @note @c percentFromLeft can be negative if using it for Bezier control points
        ///     that need to go a little outside of the rect.
        [[nodiscard]]
        double GetXPosFromLeft(const wxRect rect, const double percentFromLeft) const
            {
            return rect.GetLeft() + (rect.GetWidth() * (percentFromLeft + m_xOffsetPercentage));
            }

        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        /// @note @c percentFromLeft can be negative if using it for Bezier control points
        ///     that need to go a little outside of the rect.
        [[nodiscard]]
        double GetXPosFromLeft(const wxRect2DDouble rect, const double percentFromLeft) const
            {
            return rect.GetLeft() + (rect.GetWidth() * (percentFromLeft + m_xOffsetPercentage));
            }

        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]]
        double GetYPosFromTop(const wxRect rect, const double percentFromTop) const
            {
            return rect.GetTop() + (rect.GetHeight() * (percentFromTop + m_yOffsetPercentage));
            }

        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]]
        double GetYPosFromTop(const wxRect2DDouble rect, const double percentFromTop) const
            {
            return rect.GetTop() + (rect.GetHeight() * (percentFromTop + m_yOffsetPercentage));
            }

        /// @brief Mirrors percentages passed to GetXPosFromLeft() or GetYPosFromTop().
        [[nodiscard]]
        constexpr double Mirror(const double percent) const noexcept
            {
            return 1.0 - percent;
            }

        /// @returns The midpoint of a rect.
        [[nodiscard]]
        wxPoint GetMidPoint(const wxRect rect) const
            {
            return rect.GetLeftTop() + wxPoint(rect.GetWidth() / 2, rect.GetHeight() / 2);
            }

        /// @returns The radius of the largest circle that can fit in a rect.
        /// @note This is floored to be conservative.
        [[nodiscard]]
        double GetRadius(const wxRect rect) const
            {
            return std::floor(safe_divide<double>(std::min(rect.GetWidth(), rect.GetHeight()), 2));
            }

        [[nodiscard]]
        double ScaleToScreenAndCanvas(const double value) const noexcept
            {
            return value * GetScaling() * GetDPIScaleFactor();
            }

        [[nodiscard]]
        double DownscaleFromScreenAndCanvas(const double value) const noexcept
            {
            return safe_divide(value, (GetScaling() * GetDPIScaleFactor()));
            }

        [[nodiscard]]
        double GetScaling() const noexcept
            {
            return m_graphInfo.GetScaling();
            }

        [[nodiscard]]
        double GetDPIScaleFactor() const noexcept
            {
            assert(m_graphInfo.GetDPIScaleFactor().has_value() &&
                   L"Shape should have a proper DPI scaling.");
            return m_graphInfo.GetDPIScaleFactor().value_or(1);
            }

        /// @returns A color with the shape's brush opacity applied to it.
        /// @param col The color to adjust.
        /// @note This should be performed on any custom brush (but *not* outline pen) colors
        ///     that a shape is using that a client isn't provided. This will ensure
        ///     that if being drawn as a fillable shape, the "ghosted" portion of the
        ///     shape will appear as expected.
        [[nodiscard]]
        wxColour ApplyParentColorOpacity(const wxColour col) const
            {
            return Colors::ColorContrast::ChangeOpacity(
                col, GetGraphItemInfo().GetBrush().GetColour().GetAlpha());
            }

        GraphItemInfo m_graphInfo;
        const wxBitmapBundle* m_iconImage{ nullptr };
        mutable double m_xOffsetPercentage{ 0.0 };
        mutable double m_yOffsetPercentage{ 0.0 };
        static std::mt19937 m_mt;
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
        Shape(const GraphItems::GraphItemInfo& itemInfo, const Icons::IconShape shape,
              const wxSize sz, const wxBitmapBundle* img = nullptr);
        /// @private
        Shape(const Shape&) = delete;
        /// @private
        Shape& operator==(const Shape&) = delete;
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
        Icons::IconShape GetShape() const noexcept { return m_shape; }

        /// @returns The size in DIPs.
        [[nodiscard]]
        wxSize GetSizeDIPS() const noexcept
            {
            return m_sizeDIPs;
            }

      protected:
        /// @returns The rectangle on the canvas where the shape would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]]
        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;

        /** @brief Moves the shape by the specified x and y values.
            @param xToMove the amount to move horizontally.
            @param yToMove the amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) noexcept final
            {
            SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove, yToMove));
            }

        /// @returns The renderer.
        [[nodiscard]]
        ShapeRenderer& GetRenderer() noexcept
            {
            return m_renderer;
            }

      protected:
        /// @private
        wxSize m_shapeSizeDIPs{ 0, 0 };
        /// @private
        wxSize m_sizeDIPs{ 0, 0 };

      private:
        [[nodiscard]]
        GraphItemInfo& GetGraphItemInfo() final
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
        [[nodiscard]]
        bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            {
            return GetBoundingBox(dc).Contains(pt);
            }

        Icons::IconShape m_shape{ Icons::IconShape::Square };
        mutable ShapeRenderer m_renderer;
        mutable bool m_rendererNeedsUpdating{ true };
        ShapeRenderer::DrawFunction m_drawFunction{ nullptr };
        };
    } // namespace Wisteria::GraphItems

/** @}*/

#endif // WISTERIA_SHAPES_H
