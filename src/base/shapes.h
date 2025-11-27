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
        GraphicsContextFallback(wxDC* dc, wxRect rect);
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
        [[nodiscard]]
        wxGraphicsContext* GetGraphicsContext() const noexcept
            {
            return m_gc;
            }

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
        explicit ShapeRenderer(GraphItemInfo itemInfo,
                               const std::shared_ptr<wxBitmapBundle>& img = nullptr)
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
        void DrawCircularSign(wxRect rect, const wxBrush& brush, const wxString& text,
                              wxDC& dc) const;
        /// @brief Draws a yellow sun shape (circle with sunbeams).
        /// @param rect The area to draw the sun within.
        /// @param dc The DC to draw to.
        void DrawSun(wxRect rect, wxDC& dc) const;
        /// @brief Draws a purple flower shape (stigma and petals).
        /// @param rect The area to draw the flower within.
        /// @param dc The DC to draw to.
        void DrawFlower(wxRect rect, wxDC& dc) const;
        /// @brief Draws a sunflower shape (stigma and petals).
        /// @param rect The area to draw the flower within.
        /// @param dc The DC to draw to.
        void DrawSunFlower(wxRect rect, wxDC& dc) const;
        /// @brief Draws an orange red leaf.
        /// @param rect The area to draw the leaf within.
        /// @param dc The DC to draw to.
        void DrawFallLeaf(wxRect rect, wxDC& dc) const;
        /// @brief Draws a curly brace.
        /// @param rect The area to draw the curly brace within.
        /// @param dc The DC to draw to.
        /// @param side The side of the object that the curly braces are enclosing.\n
        ///     For example, @c Left means that the left curly braces will be drawn
        ///     (enclosing what is to the right of them),
        ///     where @c Top will draw curly braces that are opening downward
        ///     (enclosing what is beneath it).
        void DrawCurlyBrace(wxRect rect, wxDC& dc, Side side) const;

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
        void DrawSquare(wxRect rect, wxDC& dc) const;
        /// @brief Draws a circle.
        /// @param rect The area to draw the circle within.
        /// @param dc The DC to draw to.
        void DrawCircle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a horizontal line.
        /// @param rect The area to draw the line within.
        /// @param dc The DC to draw to.
        void DrawHorizontalLine(wxRect rect, wxDC& dc) const;
        /// @brief Draws a vertical line.
        /// @param rect The area to draw the line within.
        /// @param dc The DC to draw to.
        void DrawVerticalLine(wxRect rect, wxDC& dc) const;
        /// @brief Draws an 'X' (crossed out symbol).
        /// @param rect The area to draw within.
        /// @param dc The DC to draw to.
        void DrawCrossedOut(wxRect rect, wxDC& dc) const;
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
        /// @brief Draws an upward triangle.
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
        /// @brief Draws a banner sign.
        /// @param rect The area to draw the sign within.
        /// @param dc The DC to draw to.
        void DrawBanner(wxRect rect, wxDC& dc) const;
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
        void DrawMan(wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawWoman(wxRect rect, wxDC& dc) const;
        /// @brief Draws a female outline (business skirt).
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBusinessWoman(wxRect rect, wxDC& dc) const;
        /// @brief Draws a downward-pointing chevron.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawChevronDownward(wxRect rect, wxDC& dc) const;
        /// @brief Draws an upward-pointing chevron.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawChevronUpward(wxRect rect, wxDC& dc) const;
        /// @brief Draws a string.
        /// @note The pen color of the shape controls the font's color.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawText(wxRect rect, wxDC& dc) const;
        /// @brief Draws a tack.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawTack(wxRect rect, wxDC& dc) const;
        /// @brief Draws a rectangle that looks like it was painted with watercolor.
        /// @param rect The area to draw within.
        /// @param dc The DC to draw to.
        /// @note The color will more than likely go outside the provided rectangle,
        ///     as that is the aesthetic that we are going for.\n
        ///     This can be negated, however, by calling `SetClippingRect()` for the Shape
        ///     object using this renderer.
        void DrawWaterColorRectangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a rectangle that looks like it was painted with watercolor,
        ///     painted with a second coat.
        /// @param rect The area to draw within.
        /// @param dc The DC to draw to.
        /// @note The color will more than likely go outside the provided rectangle,
        ///     as that is the aesthetic that we are going for.\n
        ///     This can be negated, however, by calling `SetClippingRect()` for the Shape
        ///     object using this renderer.
        void DrawThickWaterColorRectangle(wxRect rect, wxDC& dc) const;
        /// @brief Draws a graduation cap with tassel.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawGraduationCap(wxRect rect, wxDC& dc) const;
        /// @brief Draws a book.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBook(wxRect rect, wxDC& dc) const;
        /// @brief Draws a car tire.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawTire(wxRect rect, wxDC& dc) const;
        /// @brief Draws a snowflake.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawSnowflake(wxRect rect, wxDC& dc) const;
        /// @brief Draws a newspaper.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawNewspaper(wxRect rect, wxDC& dc) const;
        /// @brief Draws a car.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCar(wxRect rect, wxDC& dc) const;
        /// @brief Draws a blackboard.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBlackboard(wxRect rect, wxDC& dc) const;
        /// @brief Draws a clock.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawClock(wxRect rect, wxDC& dc) const;
        /// @brief Draws a measuring ruler.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawRuler(wxRect rect, wxDC& dc) const;
        /// @brief Draws an IV bag.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawIVBag(wxRect rect, wxDC& dc) const;
        /// @brief Draws a thermometer showing a cold temperature.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawColdThermometer(wxRect rect, wxDC& dc) const;
        /// @brief Draws a thermometer showing a hot temperature.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHotThermometer(wxRect rect, wxDC& dc) const;
        /// @brief Draws a red apple.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawRedApple(wxRect rect, wxDC& dc) const;
        /// @brief Draws a green apple.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawGrannySmithApple(wxRect rect, wxDC& dc) const;
        /// @brief Draws a heart.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHeart(wxRect rect, wxDC& dc) const;
        /// @brief Draws an Immaculate Heart.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawImmaculateHeart(wxRect rect, wxDC& dc) const;
        /// @brief Draws an Immaculate Heart (with sword piercing it).
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawImmaculateHeartWithSword(wxRect rect, wxDC& dc) const;
        /// @brief Draws a flame.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFlame(wxRect rect, wxDC& dc) const;
        /// @brief Draws an office building.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawOffice(wxRect rect, wxDC& dc) const;
        /// @brief Draws a factory building.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFactory(wxRect rect, wxDC& dc) const;
        /// @brief Draws a house.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHouse(wxRect rect, wxDC& dc) const;
        /// @brief Draws a barn.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawBarn(wxRect rect, wxDC& dc) const;
        /// @brief Draws a barn and grain silo.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawFarm(wxRect rect, wxDC& dc) const;
        /// @brief Draws a $100 dollar bill.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawHundredDollarBill(wxRect rect, wxDC& dc) const;
        /// @brief Draws a computer monitor.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawMonitor(wxRect rect, wxDC& dc) const;
        /// @brief Draws a sword.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawSword(wxRect rect, wxDC& dc) const;
        /// @brief Draws a crescent at the top.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCrescentTop(wxRect rect, wxDC& dc) const;
        /// @brief Draws a crescent at the bottom.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCrescentBottom(wxRect rect, wxDC& dc) const;
        /// @brief Draws a crescent at the right.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCrescentRight(wxRect rect, wxDC& dc) const;
        /// @brief Draws a curving.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawCurvingRoad(wxRect rect, wxDC& dc) const;
        /// @brief Draws a pumpkin.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawPumpkin(wxRect rect, wxDC& dc) const;
        /// @brief Draws a jack-o'-lantern.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawJackOLantern(wxRect rect, wxDC& dc) const;
        /// @brief Draws two numbers with a fancy bar between them.
        /// @param rect The area to draw the image within.
        /// @param dc The DC to draw to.
        void DrawNumberRange(wxRect rect, wxDC& dc) const;
        /// @}
      private:
        enum class Temperature
            {
            Hot,
            Cold
            };
        enum class ClippingSection
            {
            Upper,
            Lower,
            None
            };

        void DrawBaseFlower(wxRect rect, wxDC& dc, const wxColour& foregroundColor,
                            const wxColour& backgroundColor) const;
        void DrawBaseBuilding(wxRect rect, wxDC& dc, const wxColour& color) const;
        void DrawThermometer(wxRect rect, wxDC& dc, Temperature temp) const;
        void DrawApple(wxRect rect, wxDC& dc, const wxColour& color) const;
        void DrawSword(wxRect rect, wxDC& dc, ClippingSection clippingSection) const;
        static void DrawAsterisk(wxRect rect, wxGraphicsContext* gc);
        void DrawTire(wxRect rect, wxGraphicsContext* gc) const;

        static void FillCarvedFeature(wxGraphicsContext* gc, const wxGraphicsPath& path,
                                      const wxRect2DDouble& bounds);
        /// @brief Sets the base color (if in use), performs the provided rendering lambda,
        ///     sets the brush, then runs the rendering lambda again.
        void DrawWithBaseColorAndBrush(wxDC& dc, const std::function<void(void)>& fn) const;

        /// @brief Offsets calls to GetXPosFromLeft(). This is useful for centering the rendering
        ///     of irregular (i.e., non-square) shapes inside squares.
        void SetXOffsetPercentage(const double offset) const noexcept
            {
            m_xOffsetPercentage = offset;
            }

        /// @brief Offsets calls to GetYPosFromLeft(). This is useful for centering the rendering
        ///     of irregular (i.e., non-square) shapes inside squares.
        void SetYOffsetPercentage(const double offset) const noexcept
            {
            m_yOffsetPercentage = offset;
            }

        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        /// @note @c percentFromLeft can be negative if using it for Bezier control points
        ///     that need to go a little outside the rect.
        [[nodiscard]]
        double GetXPosFromLeft(const wxRect& rect, const double percentFromLeft) const
            {
            return rect.GetLeft() + (rect.GetWidth() * (percentFromLeft + m_xOffsetPercentage));
            }

        /// @brief Helper to get X coordinate based on percent of width of rect from its left side.
        /// @note @c percentFromLeft can be negative if using it for Bezier control points
        ///     that need to go a little outside the rect.
        [[nodiscard]]
        double GetXPosFromLeft(const wxRect2DDouble& rect, const double percentFromLeft) const
            {
            return rect.GetLeft() + (rect.GetWidth() * (percentFromLeft + m_xOffsetPercentage));
            }

        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]]
        double GetYPosFromTop(const wxRect& rect, const double percentFromTop) const
            {
            return rect.GetTop() + (rect.GetHeight() * (percentFromTop + m_yOffsetPercentage));
            }

        /// @brief Helper to get Y coordinate based on percent of height of rect from its top.
        [[nodiscard]]
        double GetYPosFromTop(const wxRect2DDouble& rect, const double percentFromTop) const
            {
            return rect.GetTop() + (rect.GetHeight() * (percentFromTop + m_yOffsetPercentage));
            }

        /// @brief Mirrors percentages passed to GetXPosFromLeft() or GetYPosFromTop().
        [[nodiscard]]
        constexpr static double Mirror(const double percent) noexcept
            {
            return 1.0 - percent;
            }

        /// @returns The midpoint of a rect.
        [[nodiscard]]
        static wxPoint GetMidPoint(const wxRect& rect)
            {
            return rect.GetLeftTop() + wxPoint(rect.GetWidth() / 2, rect.GetHeight() / 2);
            }

        /// @returns The radius of the largest circle that can fit in a rect.
        /// @note This is floored to be conservative.
        [[nodiscard]]
        static double GetRadius(const wxRect& rect)
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
        wxColour ApplyColorOpacity(const wxColour& col) const
            {
            return Colors::ColorContrast::ChangeOpacity(
                col, GetGraphItemInfo().GetBrush().GetColour().GetAlpha());
            }

        /** @returns The tinted version of a color if the shape's
                brush isn't opaque.
            @details For situations where we can't use true opacity for
                the shape because it is drawn with overlapping sub-shapes.
                Here, we just used washed out colors instead.*/
        [[nodiscard]]
        wxColour TintIfUsingOpacity(const wxColour& color) const
            {
            return (!GetGraphItemInfo().GetBrush().GetColour().IsOpaque()) ?
                       Colors::ColorContrast::Tint(color) :
                       color;
            }

        GraphItemInfo m_graphInfo;
        std::shared_ptr<wxBitmapBundle> m_iconImage{ nullptr };
        mutable double m_xOffsetPercentage{ 0.0 };
        mutable double m_yOffsetPercentage{ 0.0 };
        thread_local static std::mt19937 m_mt;
        };

    /// @brief Simple descriptive information about a shape.
    ///     Only used for storing  and sharing shape info between interfaces,
    ///     as Shape is not copyable.
    class ShapeInfo
        {
      public:
        /** @brief Specifies the shape to render.
            @param shp The shape to draw.
            @returns A self reference.*/
        ShapeInfo& Shape(const Icons::IconShape shp) noexcept
            {
            m_shape = shp;
            return *this;
            }

        /** @brief Specifies the size of the shape (in DIPs).
            @param sz The shape's size.
            @returns A self reference.*/
        ShapeInfo& Size(const wxSize sz) noexcept
            {
            m_sizeDIPs = sz;
            return *this;
            }

        /** @brief Specifies the brush of the shape.
            @param brush The brush.
            @returns A self reference.*/
        ShapeInfo& Brush(const wxBrush& brush) noexcept
            {
            m_brush = brush;
            return *this;
            }

        /** @brief Specifies the pen of the shape.
            @param pen The pen.
            @returns A self reference.*/
        ShapeInfo& Pen(const wxPen& pen) noexcept
            {
            m_pen = pen;
            return *this;
            }

        /** @brief Specifies the text on the shape to render.
            @param text The text to draw.
            @returns A self reference.*/
        ShapeInfo& Text(const wxString& text) noexcept
            {
            m_text = text;
            return *this;
            }

        /** @brief If a fillable shape, how much of it to fill.
            @param fillPercent The percent to fill the shape.
            @returns A self reference.*/
        ShapeInfo& FillPercent(const double fillPercent) noexcept
            {
            m_fillPercent = fillPercent;
            return *this;
            }

        /** @brief The number of times to repeat the shape.
            @details This is only used for special circumstances, such as waffle charts.
            @param repeatCount The number of times to repeat the shape.
            @returns A self reference.*/
        ShapeInfo& Repeat(const size_t repeatCount) noexcept
            {
            m_repeatCount = repeatCount;
            return *this;
            }

        /// @returns The shape.
        [[nodiscard]]
        Icons::IconShape GetShape() const noexcept
            {
            return m_shape;
            }

        /// @returns The shape's size (in DIPs).
        [[nodiscard]]
        wxSize GetSizeDIPs() const noexcept
            {
            return m_sizeDIPs;
            }

        /// @return The shape's brush.
        [[nodiscard]]
        wxBrush GetBrush() const noexcept
            {
            return m_brush;
            }

        /// @returns The shape's pen.
        [[nodiscard]]
        wxPen GetPen() const noexcept
            {
            return m_pen;
            }

        /// @returns The shape's text.
        [[nodiscard]]
        wxString GetText() const noexcept
            {
            return m_text;
            }

        /// @returns The percent that the shape is filled.
        [[nodiscard]]
        double GetFillPercent() const noexcept
            {
            return m_fillPercent;
            }

        /// @returns The number of times the shape should be repeated.
        [[nodiscard]]
        size_t GetRepeatCount() const noexcept
            {
            return m_repeatCount;
            }

      private:
        Icons::IconShape m_shape{ Icons::IconShape::Square };
        wxSize m_sizeDIPs{ 16, 16 };
        wxBrush m_brush;
        wxPen m_pen;
        wxString m_text;
        double m_fillPercent{ math_constants::full };
        size_t m_repeatCount{ 1 };
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
        Shape(const GraphItemInfo& itemInfo, Icons::IconShape shape, wxSize sz,
              const std::shared_ptr<wxBitmapBundle>& img = nullptr);
        /// @private
        /// @internal Is not copyable because the renderer stores a pointer to one of its
        ///     own functions to draw shapes.
        Shape(const Shape&) = delete;
        /// @private
        Shape& operator=(const Shape&) = delete;
        /** @brief Bounds the shape to the given rectangle.
            @param rect The rectangle to bound the shape to.
            @param dc This parameter is ignored.
            @param parentScaling This parameter is ignored.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] double parentScaling) final;
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

        /// @brief The overall size of the shape and its bounding box (in DIPs).
        /// @note This will differ from GetShapeSizeDIPS() (which is protected)
        ///     if SetBoundingBox() is called.
        /// @returns The bounding box size in DIPs.
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
        ShapeRenderer& GetRenderer() const noexcept
            {
            return m_renderer;
            }

        /// @returns The actual shape's size in device independent pixels.
        /// @note This will differ from GetSizeDIPS() if SetBoundingBox() is called.
        [[nodiscard]]
        wxSize GetShapeSizeDIPS() const noexcept
            {
            return m_shapeSizeDIPs;
            }

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

        /** @returns @c true if the given point is inside the shape.
            @param pt The point to check.
            @param dc The rendering DC.*/
        [[nodiscard]]
        bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            {
            return GetBoundingBox(dc).Contains(pt);
            }

        wxSize m_shapeSizeDIPs{ 0, 0 };
        wxSize m_sizeDIPs{ 0, 0 };

        Icons::IconShape m_shape{ Icons::IconShape::Square };
        mutable ShapeRenderer m_renderer;
        mutable bool m_rendererNeedsUpdating{ true };
        ShapeRenderer::DrawFunction m_drawFunction{ nullptr };
        };
    } // namespace Wisteria::GraphItems

/** @}*/

#endif // WISTERIA_SHAPES_H
