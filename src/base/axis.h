/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_AXIS_H
#define WISTERIA_AXIS_H

#include "../data/dataset.h"
#include "currencyformat.h"
#include "label.h"
#include "polygon.h"
#include "shapes.h"
#include <cstdint>
#include <map>
#include <vector>
#include <wx/wx.h>

// forward declares for friendship
namespace Wisteria
    {
    class Canvas;
    }

namespace Wisteria::Graphs
    {
    class Graph2D;
    class LikertChart;
    class GanttChart;
    class Histogram;
    class BarChart;
    } // namespace Wisteria::Graphs

namespace Wisteria
    {
    /// @brief The visual style for a reference area.
    enum class ReferenceAreaStyle
        {
        /// @brief Area is filled solidly and has reference lines on each side.
        Solid,
        /// @brief Area is filled with a gradient of the color to transparent,
        ///     going from left to right. Only the left side has a reference line.\n
        ///     This is useful for showing a reference area with a vague ending point.
        FadeFromLeftToRight,
        /// @brief Area is filled with a gradient of the color to transparent,
        ///     going from bottom to top. Only the bottom side has a reference line.\n
        ///     This is useful for showing a reference area with a vague ending point.
        FadeFromBottomToTop = FadeFromLeftToRight,
        /// @brief Area is filled with a gradient of the color to transparent,
        ///     going from right to left. Only the right side has a reference line.\n
        ///     This is useful for showing a reference area with a vague starting point.
        FadeFromRightToLeft,
        /// @brief Area is filled with a gradient of the color to transparent,
        ///     going from top to bottom. Only the top side has a reference line.\n
        ///     This is useful for showing a reference area with a vague starting point.
        FadeFromTopToBottom = FadeFromRightToLeft
        };

    /// @brief How to draw the line connecting the bracket to the parent axis.
    enum class BracketLineStyle
        {
        /// @brief Regular line.
        Lines,
        /// @brief Arrow pointing at the axis.
        Arrow,
        /// @brief Arrow point at the labels on the bracket.
        ReverseArrow,
        /** @brief Do not draw a line from the axis to the bracket;
                only the bracket itself will be drawn.*/
        NoConnectionLines,
        /** @brief The connecting lines and main bracket line
                will be a curly brace.*/
        CurlyBraces
        };
    } // namespace Wisteria

namespace Wisteria::GraphItems
    {
    /// @brief Draws a line across the graph, showing a reference value on its parent axis.
    /// @details This is used by a Graph2D::AddReferenceLine().
    class ReferenceLine
        {
        friend class Wisteria::Graphs::Graph2D;

      public:
        /// @brief Constructor.
        /// @param axisType The parent axis to anchor the line to.
        /// @param axisPosition Where on the parent axis to start the line.\n
        ///     Note that `Axes::FindCustomLabelPosition()` or `Axes::FindDatePosition()`
        ///     can be used to find the position of a label.
        /// @param label The label to describe what the line represents.
        /// @param pen The pen to use for the line.
        ReferenceLine(const AxisType axisType, const double axisPosition, wxString label,
                      const wxPen& pen = wxPen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH))
            : m_axisType(axisType), m_axisPosition(axisPosition), m_label(std::move(label)),
              m_pen(pen), m_compKey(m_label + pen.GetColour().GetAsString(wxC2S_HTML_SYNTAX))
            {
            }

        /// @private
        /// @brief Compare on axis position.
        [[nodiscard]]
        bool operator<(const ReferenceLine& that) const noexcept
            {
            return (m_axisPosition < that.m_axisPosition);
            }

      private:
        AxisType m_axisType{ AxisType::RightYAxis };
        double m_axisPosition{ 0 };
        wxString m_label;
        wxPen m_pen{ *wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH };
        // used by lambdas to sort by label and color (instead of axis position)
        wxString m_compKey;
        };

    /// @brief Draws two lines across the graph, showing reference values on its parent axis,
    ///     and fills in the area between them with a color. An example of this could be adding
    ///     a recession to a financial plot.
    /// @details This is used by Graph2D::AddReferenceArea().
    class ReferenceArea : public ReferenceLine
        {
        friend class Wisteria::Graphs::Graph2D;

      public:
        /// @brief Constructor.
        /// @param axisType The parent axis to anchor the line to.
        /// @param axisPosition1 Where on the parent axis to start the area.\n
        ///     Note that `Axes::FindCustomLabelPosition()` or `Axes::FindDatePosition()`
        ///     can be used to find the position of a label.
        /// @param axisPosition2 Where on the parent axis to end the area.
        /// @param label The label to describe what the line represents.
        /// @param pen The pen to use for the line.
        /// @param refAreaStyle The visual style of the reference area.
        ReferenceArea(const AxisType axisType, const double axisPosition1,
                      const double axisPosition2, const wxString& label,
                      const wxPen& pen = wxPen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH),
                      const ReferenceAreaStyle refAreaStyle = ReferenceAreaStyle::Solid)
            : ReferenceLine(axisType, axisPosition1, label, pen), m_refAreaStyle(refAreaStyle),
              m_axisPosition2(axisPosition2)
            {
            }

      private:
        ReferenceAreaStyle m_refAreaStyle{ ReferenceAreaStyle::Solid };
        double m_axisPosition2{ 0 };
        };

    /// @brief An axis on a graph.
    /// @sa The [axis](../../Axes.md) overview for more information.
    class Axis final : public GraphItems::GraphItemBase
        {
        friend class Wisteria::Canvas;
        friend class Wisteria::Graphs::Graph2D;
        friend class Wisteria::Graphs::LikertChart;
        friend class Wisteria::Graphs::GanttChart;
        friend class Wisteria::Graphs::Histogram;
        friend class Wisteria::Graphs::BarChart;

      public:
        /// @brief A tickmark on an axis.
        struct TickMark
            {
            /// @brief How a tickmark is displayed.
            enum class DisplayType
                {
                Inner,    /*!< The tickmark is inside the plot.*/
                Outer,    /*!< The tickmark is outside the plot.*/
                Crossed,  /*!< The tickmark is both inside and outside the plot.*/
                NoDisplay /*!< Do not display tickmarks.*/
                };

            /** @brief Constructor.
                @param displayType How the tickmark is displayed.
                @param position The position on the axis for the tickmark.
                @param lineLength The length of the tickmark.*/
            TickMark(const DisplayType displayType, const double position,
                     wxCoord lineLength) noexcept
                : m_displayType(displayType), m_position(position), m_lineLength(lineLength)
                {
                }

            /// @private
            TickMark() = default;

            /// @brief Returns how the tickmark is displayed.
            /// @returns The tickmark display method.
            [[nodiscard]]
            const DisplayType& GetTickMarkDisplay() const noexcept
                {
                return m_displayType;
                }

            /// @brief The display of the tickmark.
            DisplayType m_displayType{ DisplayType::NoDisplay };
            /// @brief Value on the axis.
            double m_position{ 0 };
            /// @brief Length of the line, protruding from the axis.
            wxCoord m_lineLength{ 0 };
            /// @brief Actual pixel coordinate on the graph.
            wxCoord m_physicalCoordinate{ -1 };
            };

        /** @brief Bracket that can be drawn next to an axis,
                showing what a particular range of values on the axis means.*/
        class AxisBracket
            {
            /// Parent axis needs full access to this object.
            friend class Axis;

          public:
            /** @brief Constructor, which sets the range (and positions),
                    along with initial label and line settings.
                @param pos1 The starting position (relative to its parent axis) of
                    where to place the bracket.
                @param pos2 The ending position (relative to its parent axis) of
                    where to place the bracket.
                @param labelPos The position (relative to its axis) of where to
                    place the bracket's label.
                @param label The label to display next to the brackets.
                @param pen The pen for the lines.\n
                    The color of the pen will be applied to the label as well.\n
                @param lineStyle The style to use for the bracket's lines.*/
            AxisBracket(const double pos1, const double pos2, const double labelPos, wxString label,
                        const wxPen& pen = wxPenInfo(*wxBLACK, 2),
                        const BracketLineStyle lineStyle = BracketLineStyle::CurlyBraces)
                : m_startPosition(pos1), m_endPosition(pos2), m_labelPosition(labelPos),
                  m_label(GraphItems::GraphItemInfo(std::move(label)).Pen(wxNullPen)),
                  m_linePen(pen), m_bracketLineStyle(lineStyle)
                {
                m_label.SetFontColor(pen.GetColour());
                m_label.SetTextOrientation(Orientation::Horizontal);
                }

            /// @name Style Functions
            /// @brief Functions relating to the bracket's general appearance.
            /// @{

            /// @brief Sets the bracket to be translucent.
            /// @details This includes the bracket lines, as well as the label.
            /// @param ghost @c true to make the bracket translucent.
            void Ghost(const bool ghost) noexcept { m_ghost = ghost; }

            /// @returns @c true if the bracket is being made translucent.
            [[nodiscard]]
            bool IsGhosted() const noexcept
                {
                return m_ghost;
                }

            /// @returns The opacity level applied if "ghosted".
            [[nodiscard]]
            uint8_t GetGhostOpacity() const noexcept
                {
                return m_ghostOpacity;
                }

            /** @brief Sets the opacity level for "ghosting".
                @param opacity The opacity level (should be between @c 0 and @c 255).
                @sa Ghost().*/
            void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

            /// @returns The width of the padding between the bracket label and the parent axis
            ///     (not including the connection line).
            /// @warning This will need to be scaled when being drawn or measured.
            ///     (This is normally managed by the parent axis.)
            [[nodiscard]]
            wxCoord GetPadding() const noexcept
                {
                return m_padding;
                }

            /// @brief Sets the width of the padding between the bracket label and the parent axis
            ///     (not including the connection line).
            /// @param padding The width of the padding. This is a pixel value that the framework
            ///     will scale to the screen for you.
            ///  (The parent axis will also scale this as the graph's scaling changes.)
            void SetPadding(const wxCoord padding) noexcept { m_padding = padding; }

            /// @}

            /// @name Range Functions
            /// @brief Functions relating to the bracket's starting and ending point
            ///     (relative to its parent axis).
            /// @{

            /// @returns The starting position (relative to the parent axis).
            [[nodiscard]]
            double GetStartPosition() const noexcept
                {
                return m_startPosition;
                }

            /// @returns The ending position (relative to the parent axis).
            [[nodiscard]]
            double GetEndPosition() const noexcept
                {
                return m_endPosition;
                }

            /// @}

            /// @name Label Functions
            /// @brief Functions relating to the bracket's labels.
            /// @{

            /// @returns The bracket's label.
            ///     Call this to edit the label's font, color, etc.
            /// @note If the bracket is ghosted, then any color assigned here
            ///     will have a translucency applied to it dynamically.
            [[nodiscard]]
            Label& GetLabel() noexcept
                {
                return m_label;
                }

            /// @brief The label position (relative to the parent axis).
            /// @returns The label position.
            [[nodiscard]]
            double GetLabelPosition() const noexcept
                {
                return m_labelPosition;
                }

            /// @returns How the labels are aligned, either against the connection
            ///     lines or the outer boundary of the brackets.
            [[nodiscard]]
            const AxisLabelAlignment& GetPerpendicularLabelConnectionLinesAlignment() const noexcept
                {
                return m_axisLabelAlignment;
                }

            /// @brief Sets how the labels are aligned, either against the lines
            ///     (connecting to the parent axis) or the outer boundary.
            /// @param alignment How to alignment the labels within the bracket section.
            /// @note Has no effect if axis labels are parallel to the connection lines.
            void SetPerpendicularLabelConnectionLinesAlignment(
                const AxisLabelAlignment alignment) noexcept
                {
                m_axisLabelAlignment = alignment;
                }

            /// @}

            /// @name Line Functions
            /// @brief Functions relating to the bracket's lines.
            /// @{

            /** @brief Gets/sets the line pen.
                @details Call this to edit the line's color, style, width, etc.
                    (e.g., `GetLinePen().SetStyle(wxPenStyle::wxPENSTYLE_DOT)`)
                @returns The pen used to draw the bracket's line.
                @note If the bracket is ghosted, then any color assigned here
                    will have a translucency applied to it dynamically.
            */
            [[nodiscard]]
            wxPen& GetLinePen() noexcept
                {
                return m_linePen;
                }

            /// @returns The length of the tickmarks between the main line of the
            ///     bracket and the parent axis.
            /// @warning This will need to be scaled when being drawn or measured.
            ///     (This is normally managed by the parent axis.)
            [[nodiscard]]
            wxCoord GetTickmarkLength() const noexcept
                {
                return m_tickMarkLength;
                }

            /// @brief Sets the length of the connection line between the bracket label
            ///     and the parent axis.
            /// @param length The length of the tickmarks.
            ///     This is a pixel value that the framework will scale to the screen for you.\n
            ///     (The parent axis will also scale this as the graph's scaling changes.)
            void SetTickmarkLength(const wxCoord length) noexcept { m_tickMarkLength = length; }

            /// @returns The type of line being draw for the bracket.
            [[nodiscard]]
            BracketLineStyle GetBracketLineStyle() const noexcept
                {
                return m_bracketLineStyle;
                }

            /** @brief Sets the type of line to draw on the bracket.
                @param lineType The line type.*/
            void SetBracketLineStyle(const BracketLineStyle lineType) noexcept
                {
                m_bracketLineStyle = lineType;
                }

            /** @brief Indicates whether the bracket is only pointing to one spot on the axis.
                @returns @c true if the bracket is really just a single line pointing from a
                    bracket label to the axis.
                @note This is @c true if the start and end points of the bracket are the same.
                @sa GetStartPosition(), GetEndPosition().*/
            [[nodiscard]]
            bool IsSingleLine() const noexcept
                {
                return compare_doubles(GetStartPosition(), GetEndPosition());
                }

            /// @returns @c true if the lines being drawn are straight lines
            ///     (as opposed to curly braces).
            [[nodiscard]]
            bool IsStraightLines() const noexcept
                {
                return (m_bracketLineStyle == BracketLineStyle::Arrow ||
                        m_bracketLineStyle == BracketLineStyle::Lines ||
                        m_bracketLineStyle == BracketLineStyle::ReverseArrow);
                }

            /// @}

            /// @private
            [[nodiscard]]
            const Label& GetLabel() const noexcept
                {
                return m_label;
                }

            /// @private
            [[nodiscard]]
            wxPen GetLinePen() const
                {
                return IsGhosted() ? wxPen{ Colors::ColorContrast::ChangeOpacity(
                                                m_linePen.GetColour(), GetGhostOpacity()),
                                            m_linePen.GetWidth() } :
                                     m_linePen;
                }

          private:
            /** @returns The width or height needed for the bracket
                    (including the connection lines).
                @param dc A graphics context to use for measuring the bracket.
                @param parentAxis The parent axis to get scaling and font information from.
                @param parentAxisOrientation The orientation of the bracket's parent axis.*/
            [[nodiscard]]
            wxCoord CalcSpaceRequired(wxDC& dc, const Axis& parentAxis,
                                      const Orientation parentAxisOrientation) const;
            /** @brief Draws the line between the bracket label area and the parent axis.
                @param dc The device context to draw on.
                @param scaling The scaling to apply to shapes (e.g., arrowheads) if applicable.\n
                    This scaling should include the screen and DPI scaling, since the brackets
                    will not have access to that information from the parent axis.\n
                    Also, note that scaling does not relate to the line length along the parent axis
                    since two explicit points are used here. If the line length needs to be scaled,
                    then caller must apply that transformation to the points.
                @param axisPoint The starting point from the axis.
                @param bracketPoint The ending point, connecting to the bracket label area.
                @note This function is only meant to be called from the parent axis when rendering
                    the brackets.*/
            void DrawConnectionLine(wxDC& dc, const double scaling, const wxPoint axisPoint,
                                    const wxPoint bracketPoint) const;

            /// @returns The connecting line length and space between that and the bracket label.
            /// @note If lines aren't being drawn, then will return a recommended amount of padding
            ///     between bracket label and axis labels.
            [[nodiscard]]
            wxCoord GetLineSpacing() const noexcept
                {
                return (GetBracketLineStyle() != BracketLineStyle::NoConnectionLines) ?
                           (GetTickmarkLength() + GetPadding()) :
                           GetPadding();
                }

            double m_startPosition{ -1 };
            double m_endPosition{ -1 };
            double m_labelPosition{ -1 };
            wxCoord m_tickMarkLength{ 15 };
            wxCoord m_padding{ 5 };
            Label m_label;
            wxPen m_linePen{ *wxBLACK, 2 };
            AxisLabelAlignment m_axisLabelAlignment{ AxisLabelAlignment::AlignWithAxisLine };
            BracketLineStyle m_bracketLineStyle{ BracketLineStyle::CurlyBraces };
            bool m_ghost{ false };
            uint8_t m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };
            };

        /// @brief A point on an axis.
        class AxisPoint
            {
            friend class Axis;
            friend class Graphs::Graph2D;

          public:
            /// @private
            AxisPoint() = default;

            /** @brief Constructor.
                @param value The value on the axis that this point should be at.
                @param displayValue The label to be display on the axis for this point.
                @param display Whether to draw this label on the parent axis.*/
            AxisPoint(const double value, wxString displayValue, bool display = true)
                : m_displayValue(std::move(displayValue)), m_show(display), m_value(value)
                {
                }

            /// @returns The string on the axis that this point is displayed as.
            [[nodiscard]]
            const wxString& GetDisplayValue() const noexcept
                {
                return m_displayValue;
                }

            /// @brief Sets the axis display label to label.
            /// @param label The label of the axis point.
            void SetDisplayValue(const wxString& label) { m_displayValue = label; }

            /// @returns Whether the point is being drawn on the axis.
            [[nodiscard]]
            bool IsShown() const noexcept
                {
                return m_show;
                }

            /// @brief Sets whether to display the axis label.
            /// @param display @c true to display the axis label.
            void Show(const bool display) noexcept { m_show = display; }

            /// @brief Positions the point along the parent axis at the specified value.
            /// @param value The value to set the axis point to.
            void SetValue(const double value) noexcept { m_value = value; }

            /// @returns The point's position along the parent axis.
            [[nodiscard]]
            double GetValue() const noexcept
                {
                return m_value;
                }

            /// @brief Sets the point's axis label to be translucent.
            /// @param ghost @c true to make the label translucent.
            void Ghost(const bool ghost) noexcept { m_ghost = ghost; }

            /// @returns @c true if the label is being made translucent.
            [[nodiscard]]
            bool IsGhosted() const noexcept
                {
                return m_ghost;
                }

            /// @returns The opacity level applied to "ghosted" label.
            [[nodiscard]]
            uint8_t GetGhostOpacity() const noexcept
                {
                return m_ghostOpacity;
                }

            /** @brief Sets the opacity level for "ghosted" label.
                @param opacity The opacity level (should be between @c 0 and @c 255).*/
            void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

            /// @private
            [[nodiscard]]
            bool operator==(const AxisPoint& that) const noexcept
                {
                return compare_doubles(m_value, that.m_value);
                }

            /// @private
            [[nodiscard]]
            bool operator==(const double value) const noexcept
                {
                return compare_doubles(m_value, value);
                }

            /// @private
            [[nodiscard]]
            bool operator!=(const AxisPoint& that) const noexcept
                {
                return !compare_doubles(m_value, that.m_value);
                }

            /// @private
            [[nodiscard]]
            bool operator!=(const double value) const noexcept
                {
                return !compare_doubles(m_value, value);
                }

            /// @private
            [[nodiscard]]
            bool operator<(const AxisPoint& that) const noexcept
                {
                return compare_doubles_less(m_value, that.m_value);
                }

            /// @private
            [[nodiscard]]
            bool operator<(const double value) const noexcept
                {
                return compare_doubles_less(m_value, value);
                }

          private:
            /** @brief Sets the point on the parent axis at the specified coordinate.
                @param coordinate The axis coordinate to place the point.
                @note This function is normally handled by the parent plot, use SetValue()
                    (or constructor) to set the axis value where the point should be.*/
            void SetPhysicalCoordinate(const double coordinate) noexcept
                {
                m_physicalCoordinate = coordinate;
                }

            /// @returns The point's physical coordinates along the parent axis.
            [[nodiscard]]
            double GetPhysicalCoordinate() const noexcept
                {
                return m_physicalCoordinate;
                }

            wxString m_displayValue;
            bool m_show{ false };
            double m_physicalCoordinate{ -1 };
            double m_value{ 0 };
            bool m_ghost{ false };
            uint8_t m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };
            };

        /// @brief Constructor.
        /// @param type The type of axis this should be.
        explicit Axis(const AxisType type)
            : m_type(type),
              m_labelOrientation((type == AxisType::BottomXAxis || type == AxisType::TopXAxis) ?
                                     AxisLabelOrientation::Parallel :
                                     AxisLabelOrientation::Perpendicular),
              // label stacking looks a bit odd on vertical axes,
              // so don't enable auto stack for them
              m_enableAutoStacking((type == AxisType::BottomXAxis || type == AxisType::TopXAxis))
            {
            m_invalidLabel.SetOk(false);
            SetTextAlignment(TextAlignment::Centered);
            GetTitle().SetTextOrientation(
                (IsVertical() ? Orientation::Vertical : Orientation::Horizontal));
            GetFont().MakeSmaller();
            }

        /// @private
        Axis(const Axis& that) = default;
        /// @private
        Axis(Axis&& that) noexcept = default;
        /// @private
        Axis() = delete;

        /** @private
            @brief Sets the DPI scaling for the axis.
            @param scaling The DPI scaling to use.*/
        void SetDPIScaleFactor(const double scaling) override final
            {
            GraphItemBase::SetDPIScaleFactor(scaling);
            m_title.SetDPIScaleFactor(scaling);
            m_header.SetDPIScaleFactor(scaling);
            m_footer.SetDPIScaleFactor(scaling);
            m_invalidLabel.SetDPIScaleFactor(scaling);
            m_widestLabel.SetDPIScaleFactor(scaling);
            m_tallestLabel.SetDPIScaleFactor(scaling);
            }

        /** @private
            @brief Sets the scaling of the axis.
            @param scaling The scaling to use.*/
        void SetScaling(const double scaling) override final;

        /// @brief Which parts of the axis should be reset when Reset() is called.
        enum class AxisResetLevel
            {
            CosmeticSettings,    /*!< @brief Resets the fonts, lines, and colors.*/
            RangeAndLabelValues, /*!< @brief Resets the calculated range information and axis
                                       labels.\n This is useful if you need to call
                                       SetRange() multiple times and want to ensure
                                       that all custom labels are cleared
                                    first.*/
            Brackets,            /*!< @brief Removes the brackets.*/
            TitleHeaderFooter,   /*!< @brief Removes the title, header, and footer.*/
            AllSettings          /*!< @brief Resets all settings, calculated range information,
                                       and axis labels.*/
            };
        /** @brief Resets the settings for the axis.
            @param level Which level of settings and values should be reset.*/
        void Reset(const AxisResetLevel level = AxisResetLevel::RangeAndLabelValues);

        /** @name Axis Type Functions
            @brief Functions related to specifying the type of axis that this is.*/
        /// @{

        /// @returns The type of axis that this is.
        [[nodiscard]]
        const AxisType& GetAxisType() const noexcept
            {
            return m_type;
            }

        /// @returns @c true if a left or right Y axis.
        [[nodiscard]]
        bool IsVertical() const noexcept
            {
            return (GetAxisType() == AxisType::LeftYAxis || GetAxisType() == AxisType::RightYAxis);
            }

        /// @returns @c true if a bottom or top X axis.
        [[nodiscard]]
        bool IsHorizontal() const noexcept
            {
            return (GetAxisType() == AxisType::BottomXAxis || GetAxisType() == AxisType::TopXAxis);
            }

        /// @returns Whether the scaling of the axis is reversed.
        ///     If it is reversed, then the values on the axis go from
        ///     largest to smaller from the origin.
        [[nodiscard]]
        bool IsReversed() const noexcept
            {
            return m_scaledReserved;
            }

        /** @brief Flips the axis. Call this after setting up the range and intervals.
            @param reverse @c true to reverse the scale, @c false to reset it.
            @deprecated Use Reverse() instead.*/
        [[deprecated("Use Reverse() instead.")]]
        void ReverseScale(bool reverse)
            {
            Reverse(reverse);
            }

        /** @brief Flips the axis.
            @param reverse @c true to reverse the scale, @c false to reset it.
            @note The range will remain the same (lowest-to-highest value); calling this
                simply reverses the order that the values/labels are rendered along the axis.
            @warning Call this after setting up the range, intervals, brackets, etc.
        */
        void Reverse(bool reverse = true);
        /// @}

        /** @name Range & Interval Functions
            @brief Functions related to setting the range of the axis.*/
        /// @{

        /** @brief Call this to set the range if you know what the interval step size should be.
            @details The range may be adjusted to fix the intervals, but will not be rounded up to
                neat intervals. This function gives you more precise control over the interval
                and range.
            @param rangeStart The start of the range.
            @param rangeEnd The end of the range.
            @param precision The floating-point precision to show on the axis labels.
            @param interval How often a tickmark should be placed along the axis range.
            @param displayInterval How often a label should be shown along the tickmarks.*/
        void SetRange(double rangeStart, double rangeEnd, uint8_t precision, double interval,
                      const size_t displayInterval = 1);
        /** @brief This sets the axis if you are not sure what the interval size should be.
            @details This function will come up with an intelligent interval size for you.
                It will also adjust the range to make them "neat."
            @param rangeStart The start of the range.
            @param rangeEnd The end of the range.
            @param precision The floating-point precision to show on the axis labels.
            @param includeExtraInterval Useful for including point labels that take up unexpected
                space on the plot.*/
        void SetRange(double rangeStart, double rangeEnd, const uint8_t precision,
                      bool includeExtraInterval = false);
        /** @brief Sets the range of data for the axis, using a series of dates.
            @param startDate The start date of the range.
            @param endDate The end date of the range.
            @param displayInterval The date interval to display along the axis.
            @param fyType If @c displayInterval is fiscal-year based, then this is the type of
                fiscal year to use; otherwise, it will be ignored.
            @note The date interval and fiscal year type specified can affect the
                starting and ending points of the dates. For example, using FY quarters will cause
                the dates to start and end at the beginning and end of a fiscal year.
                Also, after calling this, GetRangeDates() will return the (possibly adjusted)
                start and end dates. GetRange(), on the other hand, will return the underlying
                axis values (which may be something like 0-365 if the date range is a year).
            @sa GetRangeDates().*/
        void SetRange(const wxDateTime& startDate, const wxDateTime& endDate,
                      const DateInterval displayInterval, const FiscalYear fyType);
        /** @brief Sets the range of data for the axis, using a series of dates.
            @param startDate The start date of the range.
            @param endDate The end date of the range.
            @note This is simplified version of SetRange() that deduced the best
                interval based on the date range.
            @sa GetRangeDates().*/
        void SetRange(const wxDateTime& startDate, const wxDateTime& endDate);

        /** @returns A pair representing the first and last value on the axis.
            @warning The direction that this range is drawn will be reversed if the
                axis is reversed.*/
        [[nodiscard]]
        auto GetRange() const noexcept
            {
            return std::make_pair(m_rangeStart, m_rangeEnd);
            }

        /** @brief Returns the start and end dates of the range.
            @returns The start and end date of the range.
            @note This will return invalid dates unless the version of SetRange()
                using dates is called.*/
        [[nodiscard]]
        std::pair<wxDateTime, wxDateTime> GetRangeDates() const noexcept
            {
            return std::make_pair(m_firstDay, m_lastDay);
            }

        /** @brief Adds a point at value/label to the axis.
            @details This function should only be called if you need to fill the axis with
             uneven values that have uniform spacing between them.
             This is useful if you want uneven values such as 1, 5, 15, 50, 100 to
             be shown across an axis equidistantly.

             This behaves differently from custom labels in that the custom labels are merely
             cosmetic displays; this will actually set the position on axis to the value.
             In the above example, plotting a value of 75 will place the point in the middle
             of where 50 and 100 are.

             Examples of what this can be used for are logarithmic scales with condensed ranges.
            @param value The position on the axis. Note that this value's position will be spaced
                evenly between the surrounding points, even if they are not equidistant in value.
            @param label The label to display on the axis at this position.\n
                If empty, then the label will be @c value formatted into a string with the current
                precision.
            @note If a value is already at this position, then this function is ignored.
            @warning Call `GetAxisPoints().clear()` prior to calling this to clear any current
                points. Also, call AdjustRangeToLabels() after you are finished adding your
                axis values to adjust the range.
            @sa AdjustRangeToLabels().*/
        void AddUnevenAxisPoint(const double value, const wxString& label = wxString{});
        /// @brief Adjusts the start and end of the range based on min and max range of the labels.
        /// @details This should be called if new points have been added via
        ///     GetAxisPoints() or AddUnevenAxisPoint().
        void AdjustRangeToLabels();

        /// @brief Whether the axis begins at zero.
        /// @returns @c true if set to begin at zero.
        /// @note If the range is set to start at a value less than zero,
        ///     then that value will be used.
        ///     This is only a request for when the starting value of the data is over zero.
        [[nodiscard]]
        bool IsStartingAtZero() const noexcept
            {
            return m_startAtZero;
            }

        /// @brief Sets whether the axis should start at a minimum of zero when
        ///     SetRange() is called.
        /// @param startAtZero @c true to force the axis to start at a minimum of zero.
        void StartAtZero(const bool startAtZero) noexcept
            {
            m_startAtZero = startAtZero;
            // reset the axis, if necessary
            const auto [axisMin, axisMax] = GetRange();
            SetRange(axisMin, axisMax, GetPrecision());
            }

        /// @returns The interval size between ticks on the axis.
        [[nodiscard]]
        double GetInterval() const noexcept
            {
            return m_interval;
            }

        /// @returns How often a label should be displayed on the ticks.
        [[nodiscard]]
        size_t GetDisplayInterval() const noexcept
            {
            return m_displayInterval;
            }

        /** @brief Sets how often a label should be displayed on the ticks.
                Specifying @c 2 means that every other axis tick will have a label on it.
            @param interval How often a label should be displayed on the ticks.
            @param offset The starting point on the axis from where to start labeling ticks.
            @warning This must be called after setting the axis labels. Also, this will override
                ShowDefaultLabels() if it had been set to @c false.*/
        void SetDisplayInterval(const size_t interval, const size_t offset = 0);

        /// @}

        /** @name Label & Value Functions
            @brief Functions related to the labels and axis points spread across the axis.
            @details The axis labels are managed by default labels generated by SetRange() or
                AddUnevenAxisPoint(), and by custom labels. Custom labels can be placed
                anywhere along the axis and can override default axis labels.

                 To set the padding for axis labels, call SetPadding(), SetTopPadding(),
                 SetRightPadding(), SetBottomPadding(), or SetLeftPadding()
                 (this will only affect the axis labels, not the actual axis).

                 To change the font attributes of the axis labels, call @c SetFontBackgroundColor(),
                 @c SetFontColor(), and @c GetFont(). Setting the background color of the labels
                    will display background boxes behind the labels that take up as much space
                    as possible on the axis. Calling SetTextAlignment() will then control
                    how the text is aligned within these boxes.

                 Whether default labels, custom labels, or both are shown can be controlled
                 by SetLabelDisplay().

                 Finally, the axis's title, header, footer, and brackets manage their own font
                 attributes. To change these, edit them via GetHeader(), GetFooter(), GetTitle(),
                 and GetBrackets().*/
        /// @{

        /// @returns The number of axis points along the axis.
        ///     This includes any points that may show tickmarks or labels,
        ///     not just the ones showing them.
        /// @note This, combined with GetDisplayInterval(), can be useful for filling
        ///     the axis with custom labels. It can also be useful when measuring how wide
        ///     objects can be if there are ones being drawn at every axis point.
        [[nodiscard]]
        size_t GetAxisPointsCount() const noexcept
            {
            return m_axisLabels.size();
            }

        /// @returns The major axis points (generated by SetRange()).
        [[nodiscard]]
        std::vector<AxisPoint>& GetAxisPoints() noexcept
            {
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            return m_axisLabels;
            }

        /// @brief Removes all custom labels from the axis.
        void ClearCustomLabels() noexcept
            {
            m_customAxisLabels.clear();
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @brief Sets the text of an axis tick label (overriding any default calculated label).
            @param tickValue The tick on the axis to label.
            @param label The label to show on the tick.*/
        void SetCustomLabel(const double tickValue, const Label& label);
        /** @returns The custom label for a specific tick, or empty string if one hasn't
                been assigned.\n Will return an invalid label object if not found,
                so check it with @c IsOk().
            @param value The tick value to retrieve the custom label for.*/
        [[nodiscard]]
        const Label& GetCustomLabel(const double value) const;

        /** @brief Finds a custom label along the axis, returning its numeric position.
            @param label The string to look for.
            @returns The numeric position on the axis of where the label is,
                or @c std::nullopt if not found.*/
        [[nodiscard]]
        std::optional<double> FindCustomLabelPosition(const wxString& label) const;

        /// @brief Finds a date along the axis, returning its numeric position.
        /// @returns The point on the axis corresponding to the provided date,
        ///     or @c std::nullopt if not found.
        /// @param date The date to search for along the axis.
        /// @note The axis must be initialized with a series of dates from SetRange();
        ///     otherwise, this will return @c std::nullopt.
        [[nodiscard]]
        std::optional<double> FindDatePosition(const wxDateTime& date) const noexcept;

        /// @returns The suggested maximum length for the axis labels.
        [[nodiscard]]
        size_t GetLabelLineLength() const noexcept
            {
            return m_suggestedMaxLengthPerLine;
            }

        /// @brief Sets the length of each line of each axis label.
        /// @param suggestedMaxLengthPerLine The length that each line should be
        ///     shortened to. If any line is longer, then the axis label will be split
        ///     into multiple lines.
        void SetLabelLineLength(const size_t suggestedMaxLengthPerLine);
        /// @brief Attempts to split longer (> 20 characters) axis labels into multiple lines.
        /// @details Attempts will be made to split lengthy labels if they contain various
        ///     separators (e.g., parentheses, slashes), appear to be a comma-separated list, or
        ///     contain conjunctions.\n
        ///     Refer to Label::SplitTextAuto(), Label::SplitTextByListItems(), and
        ///     Label::SplitTextByConjunctions() for how auto-splitting text works.
        /// @param suggestedMaxLength The suggested maximum length that an axis label should be.\n
        ///     If any label is longer, then an attempt to split it into
        ///     multiple lines will be made.
        void SetLabelLengthAuto(const size_t suggestedMaxLength = 20);
        /// @returns Whether the axis labels need to be stacked so that they don't overlap.
        /// @param dc The DC to measure the text with.
        [[nodiscard]]
        bool ShouldLabelsBeStackedToFit(wxDC& dc) const;
        /// @returns @c true if the specified value as a label associated to it.
        ///     This checks for both generated labels and custom labels.
        /// @param value The axis value to check.
        [[nodiscard]]
        bool PointHasLabel(const double value) const;
        /// @brief Sets how the numbers and custom labels should be shown.
        /// @param display How the tick labels should be displayed.
        void SetLabelDisplay(const AxisLabelDisplay display);

        /// @returns How the tick labels are displayed.
        [[nodiscard]]
        AxisLabelDisplay GetLabelDisplay() const noexcept
            {
            return m_labelDisplay;
            }

        /// @returns How the numeric values of the axis points are displayed.
        [[nodiscard]]
        NumberDisplay GetNumberDisplay() const noexcept
            {
            return m_numberLabelDisplay;
            }

        /// @brief Sets how the numeric values of the axis points are displayed.
        /// @param display The numeric display method.
        /// @note This will only apply if GetLabelDisplay() is using a type that includes
        ///     the numeric value of the axis point. In other words, this will not apply
        ///     to custom labels.
        void SetNumerDisplay(const NumberDisplay display)
            {
            m_numberLabelDisplay = display;
            SetLabelDisplay(GetLabelDisplay());
            }

        /// @brief Sets how the labels are aligned, either flush with the axis line,
        ///     flush left against the edge of the axis area, or centered on the axis line itself.
        /// @param alignment How to align the labels against the axis.
        /// @note Has no effect if axis labels are parallel to the axis.
        ///     Also, the double-sided option
        ///     will be ignored if this is set to @c CenterOnAxisLine.
        /// @todo Not currently implemented for horizontal axes.
        void SetPerpendicularLabelAxisAlignment(const AxisLabelAlignment alignment) noexcept
            {
            m_axisLabelAlignment = alignment;
            }

        /// @returns How the labels are aligned, either against the line or the outer boundary.
        [[nodiscard]]
        const AxisLabelAlignment& GetPerpendicularLabelAxisAlignment() const noexcept
            {
            return m_axisLabelAlignment;
            }

        /// @brief How the labels are aligned with its respective axis point.
        /// @returns The axis labels' alignment to their respective points.
        [[nodiscard]]
        RelativeAlignment GetParallelLabelAlignment() const noexcept
            {
            return m_labelAlignment;
            }

        /** @brief How the labels are aligned with its respective axis point.
            @param alignment The alignment.
            @note This only applies when labels are parallel to the axis, see
                SetAxisLabelOrientation(). To control how the text within the
                axis labels are aligned, call SetTextAlignment().*/
        void SetParallelLabelAlignment(const RelativeAlignment alignment) noexcept
            {
            m_labelAlignment = alignment;
            }

        /** @brief Sets whether the first and last axis labels should be shown
                (the outer lines of the plot).
                This should be called after the labels and scaling have been set.
            @param display Whether outer labels should be displayed.*/
        void ShowOuterLabels(const bool display = true) noexcept
            {
            if (!m_axisLabels.empty())
                {
                m_axisLabels.front().Show(display);
                m_axisLabels.back().Show(display);
                }
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @returns Whether the first and last axis labels are being shown
                (the outer lines of the plot).*/
        [[nodiscard]]
        bool IsShowingOuterLabels() const noexcept
            {
            return !m_axisLabels.empty() ? m_axisLabels.front().IsShown() : true;
            }

        /// @returns @c true if the labels are being stacked.
        /// @sa EnableAutoStacking().
        [[nodiscard]]
        bool IsStackingLabels() const noexcept
            {
            return m_stackLabelsToFit;
            }

        /** @brief Stacks the labels, where every other label is below the previous one.
            @details This is useful for when the labels are long and will overlap each other.
            @param stacked Whether to stack the labels.*/
        void StackLabels(bool stacked = true) noexcept { m_stackLabelsToFit = stacked; }

        /** @brief Sets whether to enable auto stacking labels.
            @details If @c true, the axis will set labels to auto stack
                if it is determined that that is the best way to fit them along the axis.
            @param enable Whether to enable auto stacking.
            @note By default, this is turned on for horizontal axes,
                but turned off for vertical axes.\n
                Stacking labels on a vertical axis can look a bit odd
                (especially with text labels), so caller should explicitly enable this
                for vertical axes if they really want it.*/
        void EnableAutoStacking(const bool enable) noexcept { m_enableAutoStacking = enable; }

        /// @returns @c true if axis can override label stacking if it determines that is the
        ///     best way to get them to fit.
        [[nodiscard]]
        bool IsAutoStackingEnabled() const noexcept
            {
            return m_enableAutoStacking;
            }

        /// @returns Whether axis labels are being drawn on both sides of the main axis line.
        [[nodiscard]]
        bool HasDoubleSidedAxisLabels() const noexcept
            {
            return m_doubleSidedAxisLabels;
            }

        /** @brief Specifies whether axis labels should be drawn on both sides of the axis.
            @param doubleSided Whether axis labels should be drawn on both sides of the
                main axis line.*/
        void SetDoubleSidedAxisLabels(const bool doubleSided) noexcept
            {
            m_doubleSidedAxisLabels = doubleSided;
            }

        /// @returns Whether to draw brackets on both sides when axis labels are
        ///     being drawn on both sides of the main axis line.
        [[nodiscard]]
        bool IsMirroringBracketsWhenDoubleSided() const noexcept
            {
            return m_mirrorBracketsWhenDoubleSided;
            }

        /** @brief Specifies whether to draw brackets on both sides when
                axis labels are drawn on both sides of the axis.
            @param mirror Whether axis labels should be drawn on both sides of the
                main axis line.*/
        void MirrorBracketsWhenDoubleSided(const bool mirror) noexcept
            {
            m_mirrorBracketsWhenDoubleSided = mirror;
            }

        /** @returns The precision of the axis labels (if numeric).*/
        [[nodiscard]]
        uint8_t GetPrecision() const noexcept
            {
            return m_displayPrecision;
            }

        /** @brief Sets the precision of the axis labels (if numeric).
            @param precision The precision to display.*/
        void SetPrecision(const uint8_t precision) noexcept { m_displayPrecision = precision; }

        /** @brief Sets the orientation of the axis's labels.
            @param orient The orientation.*/
        void SetAxisLabelOrientation(const AxisLabelOrientation& orient) noexcept;

        /// @returns The orientation of the axis's labels.
        [[nodiscard]]
        const AxisLabelOrientation& GetAxisLabelOrientation() const noexcept
            {
            return m_labelOrientation;
            }

        /** @brief Sets the axis label at the specified position to be translucent.
            @param axisPosition The label's position on the axis.
            @param ghost @c true to make the label translucent.
            @warning This should be called after the axis range (and any custom labels)
                have been assigned.
            @sa Ghost() for ghosting the axis lines.
        */
        void GhostAxisPoint(const double axisPosition, const bool ghost);
        /** @brief Sets all axis labels to be translucent.
            @param ghost @c true to make the labels translucent.
            @warning This should be called after the axis range (and any custom labels)
                have been assigned.
            @sa Ghost() for ghosting the axis lines.
        */
        void GhostAllAxisPoints(const bool ghost);

        /** @brief Sets the specified axis labels (by axis position) to be fully opaque,
                and all others to a lighter opacity.
            @param positions The axis positions of the labels to showcase.
            @warning This should be called after the axis range (and any custom labels)
                have been assigned.
            @sa SetGhostOpacity().
        */
        void ShowcaseAxisPoints(const std::vector<double>& positions);

        /// @}

        /** @name Padding Functions
            @brief Functions related to padding around the outer area of the axis.
            @note These values are only applied to the axis labels, not the actual axis.*/
        /// @{

        /** @brief Sets the padding around the axis, starting at 12 o'clock and going clockwise.
            @details This is applied to the axis labels.
            @param top The top padding.
            @param right The right padding.
            @param bottom The bottom padding.
            @param left The left padding.*/
        void SetPadding(const wxCoord top, const wxCoord right, const wxCoord bottom,
                        const wxCoord left) noexcept override final
            {
            GraphItemBase::SetPadding(top, right, bottom, left);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @brief Sets the bottom padding of the axis.
            @param padding The padding size.
            @note This is a pixel value that the framework will scale to the screen for you.*/
        void SetBottomPadding(const wxCoord padding) noexcept override final
            {
            GraphItemBase::SetBottomPadding(padding);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @brief Sets the top padding of the axis.
            @param padding The padding size.
            @note This is a pixel value that the framework will scale to the screen for you.*/
        void SetTopPadding(const wxCoord padding) noexcept override final
            {
            GraphItemBase::SetTopPadding(padding);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @brief Sets the right padding of the axis.
            @param padding The padding size.
            @note This is a pixel value that the framework will scale to the screen for you.*/
        void SetRightPadding(const wxCoord padding) noexcept override final
            {
            GraphItemBase::SetRightPadding(padding);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /** @brief Sets the left padding of the axis.
            @param padding The padding size.
            @note This is a pixel value that the framework will scale to the screen for you.*/
        void SetLeftPadding(const wxCoord padding) noexcept override final
            {
            GraphItemBase::SetLeftPadding(padding);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /// @}

        /** @name Plotting Functions
            @brief Functions related to plotting points.
            @details These functions are generally only used by `Graph2D`-derived classes,
                and may only be relevant when you are designing a new graph type.*/
        /// @{

        /** @brief Returns the physical point of an axis value, relative to the parent plot.
            @param value The axis value to search for.
            @param[out] result The physical coordinate of where the value is,
                relative to the parent plot.
            @returns @c true if the physical coordinate is found
                (@c false when value isn't on the axis).*/
        bool GetPhysicalCoordinate(const double value, wxCoord& result) const;

        /// @}

        /** @name Line Functions
            @brief Functions related to axis lines and gridlines.*/
        /// @{

        /** @brief Gets/sets the pen used for the main axis line and tickmarks.
            @returns The axis line pen.
            @note Set to @c wxNullPen or transparent to turn off the main axis line and tickmarks.*/
        [[nodiscard]]
        wxPen& GetAxisLinePen() noexcept
            {
            return m_axisLinePen;
            }

        /** @brief Gets/sets the gridline pen.
            @returns The gridline pen.
            @note Set to @c wxNullPen or transparent to turn off gridlines.*/
        [[nodiscard]]
        wxPen& GetGridlinePen() noexcept
            {
            return m_gridlinePen;
            }

        /** @brief Returns how the end of the axis line is being drawn.
            @returns The line's cap style.*/
        [[nodiscard]]
        AxisCapStyle GetCapStyle() const noexcept
            {
            return m_capStyle;
            }

        /** @brief Sets how the end of the axis line is being drawn.
            @param capStyle The cap style to use.*/
        void SetCapStyle(const AxisCapStyle capStyle) noexcept { m_capStyle = capStyle; }

        /// @brief Sets the axis line and tickmarks to be translucent.
        /// @param ghost @c true to make the lines translucent.
        /// @note Titles, headers, footers, labels, and brackets will not be affected.
        /// @sa GhostAxisPoint() and ShowcaseAxisPoints() for ghosting axis labels.
        void Ghost(const bool ghost) noexcept { m_ghost = ghost; }

        /// @returns @c true if the lines are being made translucent.
        [[nodiscard]]
        bool IsGhosted() const noexcept
            {
            return m_ghost;
            }

        /// @returns The opacity level applied to "ghosted" lines.
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /** @brief Sets the opacity level for "ghosted" lines and brackets.
            @param opacity The opacity level (should be between @c 0 and @c 255).*/
        void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

        /// @}

        /** @name Tickmark Functions
            @brief Functions related to tickmarks.*/
        /// @{

        /** @brief Adds a custom tick mark.
            @param displayType How to draw the tick mark.
            @param position The axis point to draw the tick mark on.
            @param length The length of the tick mark.*/
        void AddCustomTickMark(const TickMark::DisplayType displayType, const double position,
                               const double length)
            {
            m_customTickMarks.emplace_back(displayType, position, length);
            }

        /// @returns The interval of the tick marks being shown.
        [[nodiscard]]
        double GetTickMarkInterval() const noexcept
            {
            return m_tickMarkDisplayInterval;
            }

        /// @brief Sets the interval to display tick marks.
        /// @param interval The interval for the tickmarks.
        void SetTickMarkInterval(const double interval) noexcept
            {
            m_tickMarkDisplayInterval = interval;
            }

        /// @returns The length of the minor tick marks (i.e., tick marks between axis labels).
        /// @note This value is not scaled to the screen DPI because it is copied to the tickmarks,
        ///     so the axis will then scale the tickmarks to the DPI and canvas scaling
        ///     at a later stage.
        [[nodiscard]]
        int GetMinorTickMarkLength() const noexcept
            {
            return m_minorTickMarkLength;
            }

        /// @brief Sets the length of the minor tick marks (i.e., tick marks between axis labels).
        /// @param length The length of the minor tickmarks.
        ///     This is a pixel value that the framework will scale to the screen for you.\n
        ///     (The parent axis will also scale this as the graph's scaling changes.)
        void SetMinorTickMarkLength(const int length) noexcept { m_minorTickMarkLength = length; }

        /// @returns The length of the major tick marks (i.e., tick marks between axis labels).
        [[nodiscard]]
        int GetMajorTickMarkLength() const noexcept
            {
            return m_majorTickMarkLength;
            }

        /// @brief Sets the length of the major tick marks
        ///     (i.e., tick marks that correspond to axis labels).
        /// @param length The length of the major tickmarks.
        ///     This is a pixel value that the framework will scale to the screen for you.
        ///     (The parent axis will also scale this as the graph's scaling changes.)
        void SetMajorTickMarkLength(const int length) noexcept { m_majorTickMarkLength = length; }

        /// @returns How the tick marks are being drawn.
        [[nodiscard]]
        const TickMark::DisplayType& GetTickMarkDisplay() const noexcept
            {
            return m_tickMarkDisplayType;
            }

        /// @param displayType Sets how to draw the tick marks.
        void SetTickMarkDisplay(const TickMark::DisplayType displayType) noexcept
            {
            m_tickMarkDisplayType = displayType;
            }

        /// @}

        /** @brief Copies the settings from another axis into this one.
            @details This only copies core settings (e.g., range, label display, precision, fonts,
                colors, etc.), not structural features such as axis type or "add on" features like
                titles, headers, footers, and brackets.
            @param that The other axis to copy settings from.*/
        void CopySettings(const Axis& that);

        /// @brief If colors in the axes are the same as @c bkColor, then set them
        ///     to black or white, contrasting against @c bkColor.
        /// @param bkColor The background color to contrast against.
        /// @note This will not do anything for the text elements of the axis if it
        ///     has a font background color overriding any underlying canvas color.
        void ContrastAgainstColor(const wxColour& bkColor);

        /** @name Title Functions
            @brief Functions related to the axis's title.*/
        /// @{

        /** @brief Gets/sets the title of the axis.
            @details This is a label that appears at the far outside the axis,
                drawn parallel to it. Relative alignment can control where the
                title is aligned against the axis
                (the default is to align it centered to the axis line).
            @note The title manages its own font.
                To customize this font, call `GetTitle().GetLabelDisplayInfo()`.
            @returns The header of the axis.
            @sa SetRelativeAlignment().*/
        [[nodiscard]]
        Label& GetTitle() noexcept
            {
            return m_title;
            }

        /// @}

        /** @name Header & Footer Functions
            @brief Functions related to labels drawn on the top or bottom (or left and right)
                of an axis.*/
        /// @{

        /** @brief Gets/sets the header of the axis.
            @details This is a label that appears at the top or right of the axis
                (depending on the orientation of the axis).

                For X axes, relative alignment can control where the header is aligned vertically
                against the right of axis (the default is to align it centered to the axis line).

                For Y axes, relative alignment controls where the footer is aligned horizontally
                against the top of the axis.
            @note The header manages its own font. The customize this font, call
                `GetHeader().GetLabelDisplayInfo()`.
            @returns The header of the axis.
            @sa SetRelativeAlignment().*/
        [[nodiscard]]
        Label& GetHeader() noexcept
            {
            return m_header;
            }

        /** @brief Gets/sets the footer of the axis.
                This is a label that appears at the bottom or left of the axis
                (depending on the orientation of the axis).

             For X axes, relative alignment controls where the footer is aligned
             vertically against the left of the axis
             (the default is to align it centered to the axis line).

             For Y axes, relative alignment controls where the footer is aligned
             horizontally against the bottom of the axis.
            @note The footer manages its own font. The customize this font, call
                `GetFooter().GetLabelDisplayInfo()`.
            @sa SetRelativeAlignment().
            @returns The footer of the axis.*/
        [[nodiscard]]
        Label& GetFooter() noexcept
            {
            return m_footer;
            }

        /// @}

        /** @name Bracket Functions
            @brief Functions related to brackets drawn alongside the axis to show grouped ranges.*/
        /// @{

        /// @brief Adds a bracket to the axis.
        /// @param bracket The bracket to add.
        /// @note The orientation of the bracket will be adjusted here to match the parent axis.
        void AddBracket(AxisBracket bracket);
        /** @brief Adds a series of brackets, based on the specified bracket type.
            @param bracketType The type of bracket series to add.
            @note If using fiscal year related brackets, you can specify how to define the fiscal
                year's dates via @c SetFiscalYearType() or @c SetFiscalYearStart().*/
        void AddBrackets(const BracketType bracketType);
        /** @brief Adds a series of brackets, based on two columns from a dataset.
            @details The first column is the labels for the brackets, and the second column
                will be the values associated with the labels. Each unique label from the first
                column will be a new bracket, and the bracket will stretch from the lowest and
                highest values from the second column that were associated with the label.
            @param data The dataset to read the columns from.
            @param labelColumn The column containing the labels.
            @param valueColumn The column containing the values to build the brackets from.\n
                This can be either a continuous, categorical, or date column.\n
                Note that this should be the same column that the axis was built from.*/
        void AddBrackets(const std::shared_ptr<const Wisteria::Data::Dataset>& data,
                         const wxString& labelColumn, const wxString& valueColumn);

        /// @brief Removes all the brackets.
        void ClearBrackets() noexcept { m_brackets.clear(); }

        /// @returns The axis's brackets.
        [[nodiscard]]
        std::vector<AxisBracket>& GetBrackets() noexcept
            {
            return m_brackets;
            }

        /** @brief Sets all axis brackets to be translucent.
            @param ghost @c true to make the brackets translucent.
            @warning This should be called after the axis range (and any custom labels)
                have been assigned.
            @sa Ghost() for ghosting the axis lines.
        */
        void GhostAllBrackets(const bool ghost);

        /** @brief Sets the specified brackets (by label) to be fully opaque,
                and all others to a lighter opacity.
            @param labels The brackets to showcase.
            @note Call SetGhostOpacity() prior to this to control how translucent
                the non-showcased (i.e., "ghosted") brackets are.
            @sa SetGhostOpacity().*/
        void ShowcaseBrackets(const std::vector<wxString>& labels);

        /** @brief Sets the specified brackets (by their labels' axis position) to be fully opaque,
                and all others to a lighter opacity.
            @param positions The brackets (by axis position) to showcase.
            @note Call SetGhostOpacity() prior to this to control how translucent
                the non-showcased (i.e., "ghosted") brackets are.
            @sa SetGhostOpacity().*/
        void ShowcaseBrackets(const std::vector<double>& positions);

        /** @brief Simplifies the labels along the brackets.
            @note This should be called after all brackets have been added.
            @details This will be various simplifications, such as the following:
            - If brackets contain a pattern like `1979-80`, `FY1979-1980`, or `AY1979-1980`,
              then the first one will be preserved, but the remaining ones will be
              shortened to `'79-80`.
              (This will only be applied if all dates are in the same century.)*/
        void SimplifyBrackets();

        /// @}

        /** @name Custom Axis Functions
            @brief Functions related to positioning this axis as a custom axis
                (i.e., an axis branching off of the traditional X or Y axes).*/
        /// @{

        /// @returns The custom position (in respect to the main x axes);
        ///     these only relate to custom axes.
        /// @sa SetCustomXPosition().
        [[nodiscard]]
        double GetCustomXPosition() const noexcept
            {
            return m_customXPosition;
            }

        /** @brief Sets the custom position (in respect to the main axes);
                these only relate to custom axes.
            @param xPos The custom (parent) x-axis position to connect this axis to.
            @note For horizontal axes, this should be the right-most point of the X axis.
                In other words, the max value of the X axis, unless it is reversed
                (then use the min of the X axis). For vertical axes, this is where the axis should
                be placed on the bottom axis.*/
        void SetCustomXPosition(const double xPos) noexcept { m_customXPosition = xPos; }

        /// @returns The custom position (in respect to the main y axes);
        ///     these only relate to custom axes.
        /// @sa SetCustomYPosition().
        [[nodiscard]]
        double GetCustomYPosition() const noexcept
            {
            return m_customYPosition;
            }

        /** @brief Sets the custom position (in respect to the main axes);
                these only relate to custom axes.
            @param yPos The custom (parent) x-axis position to connect this axis to.
            @note For vertical axes, this should be the top-most point of the Y axis.
                In other words, the max value of the Y axis, unless Y is reversed
                (then it should be its min value).\n
                For horizontal axes, this is where the axis should be placed on the left axis.*/
        void SetCustomYPosition(const double yPos) noexcept { m_customYPosition = yPos; }

        /// @returns The offset from the parent axis (applies only to custom axes).
        [[nodiscard]]
        double GetOffsetFromParentAxis() const noexcept
            {
            return m_offsetFromParentAxis;
            }

        /// @brief Sets the offset from the parent axis (applies only to custom axes).
        /// @param offset The offset from the parent axis.
        void SetOffsetFromParentAxis(const double offset) noexcept
            {
            m_offsetFromParentAxis = offset;
            }

        /** @brief Sets the physical y position on the canvas.
            @param y The physical y position to place the axis.
            @note This is used by the framework (e.g., @c Graph2D) for layout and should not be
                called in client code.\n
                Use SetCustomYPosition() and SetCustomXPosition() for custom positioning of an
                axis relative to the main axes.*/
        void SetPhysicalCustomYPosition(const double y) noexcept { m_physicalCustomYPosition = y; }

        /// @returns The physical y position on the canvas of the axis.
        ///     (Should only be used if using custom positioning along a parent axis.)
        [[nodiscard]]
        double GetPhysicalCustomYPosition() const noexcept
            {
            return m_physicalCustomYPosition;
            }

        /** @brief Sets the physical x position on the canvas.
            @param x The physical x position to place the axis.
            @note This is used by the framework (e.g., @c Graph2D) for layout and should not be
                called in client code.\n
                Use SetCustomYPosition() and SetCustomXPosition() for custom positioning of an
                axis relative to the main axes.*/
        void SetPhysicalCustomXPosition(const double x) noexcept { m_physicalCustomXPosition = x; }

        /// @returns The physical x position on the canvas of the axis.
        ///     (Should only be used if using custom positioning along a parent axis.)
        [[nodiscard]]
        double GetPhysicalCustomXPosition() const noexcept
            {
            return m_physicalCustomXPosition;
            }

        /// @returns The length of an interval along the axis in pixels.
        [[nodiscard]]
        double GetIntervalPhysicalLength() const
            {
            wxCoord axisPos1{ 0 }, axisPos2{ 0 };
            const auto rangeStart = GetRange().first;
            GetPhysicalCoordinate(rangeStart, axisPos1);
            GetPhysicalCoordinate(rangeStart + GetInterval(), axisPos2);
            return std::abs(axisPos2 - axisPos1);
            }

        /// @}

        /** @name Outlining & Axis Width Functions
            @brief Functions related to controlling how wide the axis is and drawing an
                outline around it.
            @note These functions can be useful for custom axes.*/
        /// @{

        /** @brief Specifies the inflated size around the axis to draw an outline.
            @details By default, this outline is not being drawn and will be activated by
                calling this function.
            @param sz The size around the axis to draw an outline.*/
        void SetOutlineSize(const wxSize sz) noexcept { m_outlineSize = sz; }

        /// @returns The inflated size around the axis that an outline is being drawn.
        /// @note By default, this outline is not being drawn and SetOutlineSize()
        ///     must be called to enable this behavior.
        [[nodiscard]]
        wxSize GetOutlineSize() const noexcept
            {
            return m_outlineSize;
            }

        /// @returns The amount of pixels (scaled to the system's DPI and canvas's scale)
        ///     that the largest tick mark is pushing out from the axis.
        ///     This is useful for see how wide the axis line and its tickmarks are.
        [[nodiscard]]
        wxCoord CalcTickMarkOuterWidth() const;
        /// @returns The amount of pixels (scaled to the system's DPI and canvas's scale)
        ///     that the largest tick mark is pushing in from the axis.
        [[nodiscard]]
        wxCoord CalcTickMarkInnerWidth() const;

        /// @}

        // These are overridden so that the widest and tallest labels are invalidated.
        /// @private
        [[nodiscard]]
        GraphItemInfo& GetGraphItemInfo() noexcept override final
            {
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            return GraphItemBase::GetGraphItemInfo();
            }

        /// @private
        void SetFontBackgroundColor(const wxColour& color) override final
            {
            if (color.IsOk())
                {
                GraphItemBase::SetFontBackgroundColor(color);
                m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
                }
            }

        /// @private
        [[nodiscard]]
        wxFont& GetFont() noexcept override final
            {
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            return GraphItemBase::GetFont();
            }

        /// @private
        [[nodiscard]]
        const wxFont& GetFont() const noexcept override final
            {
            return GraphItemBase::GetFont();
            }

        /// @private
        void SetFont(const wxFont& font) override final
            {
            GraphItemBase::SetFont(font);
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            for (auto& bracket : GetBrackets())
                {
                bracket.GetLabel().SetFont(GetFont());
                }
            }

        // Just hiding these from Doxygen. If these are included inside groupings,
        // then the "private" tag will break the group in the generated help.
        /// @private
        [[nodiscard]]
        const wxPen& GetAxisLinePen() const noexcept
            {
            return m_axisLinePen;
            }

        /// @private
        [[nodiscard]]
        const wxPen& GetGridlinePen() const noexcept
            {
            return m_gridlinePen;
            }

        /// @private
        [[nodiscard]]
        const Label& GetFooter() const noexcept
            {
            return m_footer;
            }

        /// @private
        [[nodiscard]]
        const Label& GetHeader() const noexcept
            {
            return m_header;
            }

        /// @private
        [[nodiscard]]
        const Label& GetTitle() const noexcept
            {
            return m_title;
            }

        /// @private
        [[nodiscard]]
        const std::vector<AxisBracket>& GetBrackets() const noexcept
            {
            return m_brackets;
            }

        /// @private
        [[nodiscard]]
        const std::vector<AxisPoint>& GetAxisPoints() const noexcept
            {
            return m_axisLabels;
            }

        /// @private
        [[deprecated("Use FindDatePosition() instead.")]] [[nodiscard]]
        std::optional<double> GetPointFromDate(const wxDateTime& date) const noexcept
            {
            return FindDatePosition(date);
            }

      private:
        /// @brief Loads the default labels based on the axis values.
        /// @details This will only do anything is the label display is set to use
        ///     the default labels. In other words, if labels are turned off or only
        ///     custom labels are used, then this will not update anything.
        void LoadDefaultLabels();
        /// @brief Simplifies bracket labels if they are years.
        bool SimplifyYearBrackets();

        // Most of the following functionality is only used by Graph2D-derived classes and
        // shouldn't be part of the client API. They are still properly documented here,
        // though, as a reference for when you are designing a new graph type.

        /// @returns Whether labels are set to display AND the axis is actually being drawn.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return IsShown() && (GetLabelDisplay() != AxisLabelDisplay::NoDisplay);
            }

        /// @brief Sets the fiscal year date range.
        /// @details This will be used if the date interval is set to DateInterval::FiscalQuarterly.
        ///     Fiscal years can vary between domains. For example, higher education runs
        ///     July 1st to June 30, while most businesses run from April 1st to March 31st.
        /// @param startMonth The starting month of the FY.
        /// @param startDay The starting day of the FY.
        /// @sa SetFiscalYearType().
        void SetFiscalYearStart(const wxDateTime::Month startMonth,
                                const wxDateTime::wxDateTime_t startDay)
            {
            m_fyQ1.SetMonth(startMonth);
            m_fyQ1.SetDay(startDay);
            // arbitrary placeholder
            m_fyQ1.SetYear(1970);

            m_fyQ2 = m_fyQ3 = m_fyQ4 = m_fyQ1;

            m_fyQ2.Add(wxDateSpan(0, 3));
            m_fyQ3.Add(wxDateSpan(0, 6));
            m_fyQ4.Add(wxDateSpan(0, 9));
            }

        /// @brief Sets the fiscal year date range, based on pre-defined types.
        /// @details Use SetFiscalYearStart() to set a specific start date.
        /// @param FY The fiscal year type.
        /// @sa SetFiscalYearStart().
        void SetFiscalYearType(const FiscalYear FY)
            {
            switch (FY)
                {
            case FiscalYear::USBusiness:
                SetFiscalYearStart(wxDateTime::Apr, 1);
                break;
            case FiscalYear::Education:
                SetFiscalYearStart(wxDateTime::Jul, 1);
                break;
                }
            }

        /// @returns The date intervals shown along the axis.
        [[nodiscard]]
        DateInterval GetDateDisplayInterval() const noexcept
            {
            return m_dateDisplayInterval;
            }

        /// @brief If embedded into canvas (i.e., not part of a plot) then this is
        ///     used by canvas to set the axis to a common size with plots next to it.
        void RecalcSizes(wxDC& dc) override final
            {
            if (IsVertical() && GetContentTop() && GetContentBottom())
                {
                auto topPt = GetTopPoint();
                topPt.y = GetContentTop().value_or(0);

                auto bottomPt = GetTopPoint();
                bottomPt.y = GetContentBottom().value_or(0);
                SetPoints(topPt, bottomPt, dc);
                }
            else if (IsHorizontal() && GetContentLeft() && GetContentRight())
                {
                auto leftPt = GetLeftPoint();
                leftPt.x = GetContentLeft().value_or(0);

                auto rightPt = GetRightPoint();
                rightPt.x = GetContentRight().value_or(0);
                SetPoints(leftPt, rightPt, dc);
                }
            else
                {
                SetPoints(GetPoints().first, GetPoints().second, dc);
                }
            }

        /// @returns The rectangular area of the axis line area.
        ///     This is relative to its parent canvas.
        [[nodiscard]]
        wxRect GetContentRect() const noexcept override final
            {
            if (IsVertical())
                {
                auto lineRect = wxRect(GetTopPoint(), GetBottomPoint());
                lineRect.SetWidth(ScaleToScreenAndCanvas(
                    GetAxisLinePen().IsOk() ? GetAxisLinePen().GetWidth() : 1));
                return lineRect;
                }
            auto lineRect = wxRect(GetLeftPoint(), GetRightPoint());
            lineRect.SetHeight(
                ScaleToScreenAndCanvas(GetAxisLinePen().IsOk() ? GetAxisLinePen().GetWidth() : 1));
            return lineRect;
            }

        /** @brief Sets the interval of the tick marks.
            @param interval The interval for the tick marks.
            @note This will be overridden the next time SetRange() is called and
                should only be used to store a temporary value for the next call to GetInterval().
                Use SetRange() if you intend to truly change the interval.*/
        void SetInterval(const double interval) noexcept
            {
            m_interval = interval;
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /// @brief Sets the scaling used just for the axis labels.
        /// @details The parent plot may set all axes to have a common scaling for the
        ///     axes' label fonts, but that should not affect the scaling of the lines,
        ///     titles, brackets, etc.
        /// @param scaling The scaling for the axis labels.
        void SetAxisLabelScaling(const double scaling)
            {
            m_axisLabelScaling = scaling;
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }

        /// @returns The font scaling used just for the axis labels.
        [[nodiscard]]
        double GetAxisLabelScaling() const noexcept
            {
            return m_axisLabelScaling;
            }

        /// @brief If labels have a background color, then make their boxes wide enough
        ///     to fill the entire place available to them.
        void AdjustLabelSizeIfUsingBackgroundColor(Label& axisLabel, const bool useMaxWidth) const;

        /** @brief Retrieves the value along the axis from a physical
                (relative to parent plot) coordinate.
            @param coordinate The physical coordinate to look up.
            @param[out] value The axis value connected to the coordinate.
            @returns @c true if coordinate is on the axis and a value can be found;
                @c false otherwise.*/
        [[nodiscard]]
        bool GetValueFromPhysicalCoordinate(const wxCoord coordinate, double& value) const noexcept;

        /// @brief Sets the width of spacing between labels.
        /// @param width The width of spacing between labels.
        void SetLabelPhysicalOffset(const double width) noexcept
            {
            m_labelSpacingPhysicalOffset = width;
            }

        /// @returns The spacing between axis labels.
        [[nodiscard]]
        double GetLabelPhysicalOffset() const noexcept
            {
            return m_labelSpacingPhysicalOffset;
            }

        /// @brief For horizontal axes, gets the first and last displayed labels and adjusts
        ///     the axis's corners to fit the labels' overhangs.
        void CalcHorizontalLabelOverhang(wxDC& dc, wxPoint& topLeftCorner,
                                         wxPoint& bottomRightCorner) const;
        /// @brief For vertical axes, gets the first and last displayed labels and adjusts
        ///     the axis's corners to fit the labels' overhangs.
        void CalcVerticalLabelOverhang(wxDC& dc, wxPoint& topLeftCorner,
                                       wxPoint& bottomRightCorner) const;

        /// @brief Draws the axis onto the graphics context dc.
        /// @param dc The DC to draw on.
        /// @returns The bounding box that the axis was drawn in.
        [[nodiscard]]
        wxRect Draw(wxDC& dc) const override final;
        /** @returns The rectangle that the axis would fit in.
            @param dc The DC to measure with.
            @note This version is more optimal if multiple axes need to be measured with
                the same DC.*/
        [[nodiscard]]
        wxRect GetBoundingBox(wxDC& dc) const override final;
        /** @returns The rectangle of the part of the axis that protrudes outside the plot area.
            @param dc The DC to measure with.*/
        [[nodiscard]]
        wxRect GetProtrudingBoundingBox(wxDC& dc) const;

        /** @returns @c true if the given point is inside this point.
            @param pt The point to check.
            @param dc The rendering DC.*/
        [[nodiscard]]
        bool HitTest(const wxPoint pt, wxDC& dc) const override final
            {
            return GetBoundingBox(dc).Contains(pt);
            }

        /** @brief Moves the item by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) override final
            {
            m_points.first += wxPoint(xToMove, yToMove);
            m_points.second += wxPoint(xToMove, yToMove);
            CalcLabelPositions();
            CalcTickMarkPositions();
            }

        /** @brief Sets the physical start and end points of the axis (relative to the
                parent canvas).
            @param pt1 The first point.
            @param pt2 The second point.
            @param dc The rendering DC.*/
        void SetPoints(const wxPoint pt1, const wxPoint pt2, wxDC& dc);

        /// @returns The top physical (relative to the parent canvas) point of the axis line
        ///     (for vertical axes).
        /// @note This is always the top-most point vertically, regardless of whether the axis
        ///     is reversed.
        [[nodiscard]]
        wxPoint GetTopPoint() const
            {
            return { GetPoints().first.x, GetPoints().first.y };
            }

        /// @returns The bottom physical (relative to the parent canvas) point of the axis line
        ///     (for vertical axes).
        /// @note This is always the bottom-most point vertically,
        ///     regardless of whether the axis is reversed.
        [[nodiscard]]
        wxPoint GetBottomPoint() const
            {
            return { GetPoints().second.x, GetPoints().second.y };
            }

        /// @returns The left physical (relative to the parent canvas) point of the axis line
        ///     (for horizontal axes).
        /// @note This is always the left-most point vertically,
        ///     regardless of whether the axis is reversed.
        [[nodiscard]]
        wxPoint GetLeftPoint() const
            {
            return { GetPoints().first.x, GetPoints().first.y };
            }

        /// @returns The right physical (relative to the parent canvas) point of the axis line
        ///     (for horizontal axes).
        /// @note This is always the right-most point vertically,
        ///     regardless of whether the axis is reversed.
        [[nodiscard]]
        wxPoint GetRightPoint() const
            {
            return { GetPoints().second.x, GetPoints().second.y };
            }

        /// @brief Get the first label (and its axis position) that is being displayed.
        /// @returns The first displayed (left or bottom) label along the axis. Also,
        ///     returns the axis position of where the label is.
        ///     If no label is found, then returns an empty label and NaN.
        /// @note This returns the far most left (or bottom) label, regardless of orientation.
        [[nodiscard]]
        std::pair<Label, double> GetFirstDisplayedLabel() const;
        /// @brief Get the last label (and its axis position) that is being displayed.
        /// @returns The last displayed (right or top) label along the axis. Also,
        ///     returns the axis position of where the label is.
        ///     If no label is found, then returns an empty label and NaN.
        /// @note This returns the far most right (or top) label, regardless of orientation.
        [[nodiscard]]
        std::pair<Label, double> GetLastDisplayedLabel() const;

        /// @returns The (const) custom tick marks.
        [[nodiscard]]
        const std::vector<TickMark>& GetCustomTickMarks() const noexcept
            {
            return m_customTickMarks;
            }

        /// @returns The custom tick marks.
        [[nodiscard]]
        std::vector<TickMark>& GetCustomTickMarks() noexcept
            {
            return m_customTickMarks;
            }

        /// @returns The calculated tick marks.
        /// @sa GetCustomTickMarks().
        [[nodiscard]]
        std::vector<TickMark>& GetTickMarks() noexcept
            {
            return m_tickMarks;
            }

        /// @returns The (const) calculated tick marks.
        /// @sa GetCustomTickMarks().
        [[nodiscard]]
        const std::vector<TickMark>& GetTickMarks() const noexcept
            {
            return m_tickMarks;
            }

        /// @returns The space between axis labels and the main axis line.
        [[nodiscard]]
        static wxCoord GetSpacingBetweenLabelsAndLine() noexcept
            {
            return 5;
            }

        /// @returns The widest label;
        [[nodiscard]]
        Label GetWidestTextLabel(wxDC& dc) const;
        /** @returns The tallest label.
            @note This takes multi-line labels into account.*/
        [[nodiscard]]
        Label GetTallestTextLabel(wxDC& dc) const;

        /** @returns The displayed string connected to the specified point on the axis.
                Which label is returned depends on whether custom labels are being used and what's
                available that the given point.
            @param pt The point on the axis to return the associated label.*/
        [[nodiscard]]
        Label GetDisplayableValue(const AxisPoint& pt) const;

        /// @returns How much space is needed to fit the brackets.
        [[nodiscard]]
        wxCoord CalcBracketsSpaceRequired(wxDC& dc) const;
        /// @returns Whether the given axis position will display something.
        /// @note Relying on AxisPoint::IsShown() is not adequate because we
        ///     need to take into account if the point doesn't have a custom label
        ///     but only custom labels are being displayed.
        [[nodiscard]]
        bool IsPointDisplayingLabel(const AxisPoint& point) const;
        /// @returns The best (smallest) scaling so that the labels don't overlap.
        /// @warning This should be called after SetPoints() is called so that the
        ///     physical width of the axis is valid.
        double CalcBestScalingToFitLabels(wxDC& dc);
        /// @brief Calculates how much space is available for the labels within the plot area.
        void CalcMaxLabelWidth();
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) override final;

        /// @returns The physical (relative to the parent canvas) starting and ending points
        ///     for the axis.
        [[nodiscard]]
        const std::pair<wxPoint, wxPoint>& GetPoints() const noexcept
            {
            return m_points;
            }

        /// @brief Calculates where to place the labels.
        void CalcLabelPositions() noexcept;
        /// @brief Calculates where to place the tickmarks.
        void CalcTickMarkPositions();

        Label m_invalidLabel;

        std::vector<AxisBracket> m_brackets;

        std::vector<AxisPoint> m_axisLabels;
        std::map<double, Label, double_less> m_customAxisLabels;
        size_t m_suggestedMaxLengthPerLine{ 100 };

        std::vector<TickMark> m_tickMarks;
        std::vector<TickMark> m_customTickMarks;
        double m_tickMarkDisplayInterval{ 1 };
        double m_minorTickMarkLength{ 5 };
        double m_majorTickMarkLength{ 10 };
        TickMark::DisplayType m_tickMarkDisplayType{ TickMark::DisplayType::NoDisplay };

        std::pair<wxPoint, wxPoint> m_points{ wxPoint(0, 0), wxPoint(0, 0) };

        bool m_doubleSidedAxisLabels{ false };
        bool m_mirrorBracketsWhenDoubleSided{ true };

        Label m_title;
        Label m_header;
        Label m_footer;

        AxisType m_type{ AxisType::TopXAxis };
        AxisLabelOrientation m_labelOrientation{ AxisLabelOrientation::Parallel };
        AxisLabelDisplay m_labelDisplay{ AxisLabelDisplay::DisplayCustomLabelsOrValues };
        AxisLabelAlignment m_axisLabelAlignment{ AxisLabelAlignment::AlignWithAxisLine };
        NumberDisplay m_numberLabelDisplay{ NumberDisplay::Value };
        RelativeAlignment m_labelAlignment{ RelativeAlignment::Centered };
        AxisCapStyle m_capStyle{ AxisCapStyle::NoCap };
        bool m_stackLabelsToFit{ false };
        bool m_enableAutoStacking{ true };
        double m_labelSpacingPhysicalOffset{ 0 };
        wxPen m_axisLinePen{ *wxBLACK_PEN };
        wxPen m_gridlinePen{ wxPen(wxPenInfo(wxColour(211, 211, 211, 200)).Cap(wxCAP_BUTT)) };

        // date information (if being used to display date intervals)
        wxDateTime m_fyQ1{ wxDateTime(1, wxDateTime::Month::Jul,
                                      1970) }; // year is just a placeholder here
        wxDateTime m_fyQ2{ wxDateTime(1, wxDateTime::Month::Oct, 1970) };
        wxDateTime m_fyQ3{ wxDateTime(1, wxDateTime::Month::Jan, 1971) };
        wxDateTime m_fyQ4{ wxDateTime(1, wxDateTime::Month::Apr, 1971) };

        wxDateTime m_firstDay;
        wxDateTime m_lastDay;
        DateInterval m_dateDisplayInterval{ DateInterval::FiscalQuarterly };

        // ghosting settings
        uint8_t m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };
        bool m_ghost{ false };

        // scale information
        bool m_scaledReserved{ false };
        bool m_startAtZero{ false };
        double m_rangeStart{ 0 };
        double m_rangeEnd{ 0 };
        uint8_t m_displayPrecision{ 0 };
        double m_interval{ 1 };
        size_t m_displayInterval{ 1 };

        double m_customXPosition{ 0 };
        double m_customYPosition{ 0 };
        double m_offsetFromParentAxis{ 0 };
        double m_physicalCustomYPosition{ -1 };
        double m_physicalCustomXPosition{ -1 };

        // calculated values for the labels
        double m_axisLabelScaling{ 1 };
        wxCoord m_maxLabelWidth{ 0 };
        // just used when embedded on a canvas
        std::optional<wxCoord> m_maxHeight;
        std::optional<wxCoord> m_maxWidth;
        // cached values
        mutable Label m_widestLabel{ Label(GraphItemInfo().Ok(false)) };
        mutable Label m_tallestLabel{ Label(GraphItemInfo().Ok(false)) };

        wxSize m_outlineSize{ wxDefaultSize };
        };
    } // namespace Wisteria::GraphItems

/** @}*/

#endif // WISTERIA_AXIS_H
