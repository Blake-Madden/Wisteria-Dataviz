/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_CANVAS_ITEMS_H__
#define __WISTERIA_CANVAS_ITEMS_H__

#include <vector>
#include <set>
#include <initializer_list>
#include <array>
#include <memory>
#include <optional>
#include <bitset>
#include <wx/wx.h>
#include <wx/gdicmn.h>
#include <wx/dcgraph.h>
#include <wx/string.h>
#include <wx/uilocale.h>
#include <wx/numformatter.h>
#include "settings.h"
#include "icons.h"
#include "../math/mathematics.h"

// forward declares
namespace Wisteria
    {
    class Canvas;
    class CanvasItemScalingChanger;
    }

namespace Wisteria::Graphs
    {
    class Graph2D;
    class PieChart;
    }

/// @brief Graphics classes and enumerations.
namespace Wisteria
    {
    /// @brief The sorting direction of bars along an axis.
    enum class SortDirection
        {
        SortAscending,  /*!< Sorted smallest to largest.*/
        SortDescending, /*!< Sorted largest to smallest.*/
        NoSort          /*!< Not sorted.*/
        };

    /// @brief The direction to fill (paint) with a gradient brush.
    enum class FillDirection
        {
        /// @brief fill upward.
        North,
        /// @brief fill downward.
        South,
        /// @brief fill to the right.
        East,
        /// @brief fill to the left.
        West,
        /// @brief fill upward.
        Up = North,
        /// @brief fill downward.
        Down = South,
        /// @brief fill to the right.
        Right = East,
        /// @brief fill to the left.
        Left = West
        };

    /// @brief Which side something is on.
    enum class Side
        {
        /// @brief Left side.
        Left,
        /// @brief Right side.
        Right,
        /// @brief Top side.
        Top,
        /// @brief Bottom side.
        Bottom
        };

    /// @brief Where a ring is within a circle.
    enum class Perimeter
        {
        /// @brief Inner ring.
        Inner,
        /// @brief outer ring.
        Outer
        };

    /// @brief The type of influence something can have on a subject.
    /// @details As an example, predictors in a linear regression.
    /// @internal This enum is a bitmask, do not make it strongly typed.
    enum Influence
        {
        /// @brief IVs with coefficients > 0.
        Positive = 1,
        /// @brief IVs with coefficients < 0.
        Negative = 2,
        /// @brief IVs with coefficients = 0.
        Neutral = 4,
        /// @brief All IVs.
        All = 8
        };

    /** @brief How labels are aligned with their parents on a graph.*/
    enum class LabelPlacement
        {
        /// @brief Labels are next to their parents.
        NextToParent,
        /// @brief Labels are flush with the plotting area's left or right side.
        /// @details This is graph-type dependent.
        Flush
        };

    /// @brief How an element is aligned to whatever it is being drawn on.
    /// @note @c FlushRight and @c FlushBottom (and likewise, right and bottom)
    ///     are synonyms for each other. That way, if the parent's orientation changes,
    ///     the relative alignment of the subobject will adjust logically without
    ///     having to be changed.
    enum class RelativeAlignment
        {
        FlushLeft,                /*!< Flush left/ragged right.*/
        FlushRight,               /*!< Flush right/ragged left.*/
        Centered,                 /*!< Centered.*/
        FlushBottom = FlushRight, /*!< Flush to the bottom.*/
        FlushTop = FlushLeft      /*!< Flush to the top.*/
        };

    /// @brief Values for specifying how an element's point controls its anchoring on the canvas.
    enum class Anchoring
        {
        TopLeftCorner,    /*!< Assume that point is the top-left corner.*/
        TopRightCorner,   /*!< Assume that point is the top-right corner.*/
        Center,           /*!< Assume that point is the center point.*/
        BottomLeftCorner, /*!< Assume that point is the bottom-left corner.*/
        BottomRightCorner /*!< Assume that point is the bottom-right corner.*/
        };

    /// @brief Methods for how objects should draw their shadow.
    /// @todo Add support for other sides.
    enum class ShadowType
        {
        NoShadow,                /*!< No shadow should be drawn.*/
        RightSideShadow,         /*!< Draw a shadow on the right side.*/
        RightSideAndBottomShadow /*!< Draw a shadow on the right side and bottom.*/
        };

    /// @brief How the decal's label should be adjusted to fit on its parent.
    enum class LabelFit
        {
        ScaleFontToFit,       /*!< Text's font is scaled to fit inside the parent.*/
        SplitTextToFit,       /*!< Text is split into multiple lines to fit in the parent.\n
                                   May be truncated with an ellipsis if there are too many lines.*/
        SplitTextToFitWidth,  /*!< Text is split into multiple lines to fit in the parent's width.*/
        DisplayAsIs,          /*!< Text is drawn from where it is anchored and is
                                   not scaled or split.\n
                                   May go outside of its parent.*/
        DisplayAsIsAutoFrame, /*!< Text is drawn from where it is anchored and is
                                   not scaled or split.\n
                                   May go outside of its parent. If it does go outside of parent,
                                   will draw a frame around the text.*/
        };

    /// @brief How (single or multi-line) text is aligned.
    /// @sa PageVerticalAlignment, PageHorizontalAlignment
    enum class TextAlignment
        {
        /// @brief Text is flush left/ragged right. (This is the default for Labels.)
        FlushLeft,
        /// @brief Same as FlushLeft.
        RaggedRight = FlushLeft,
        /// @brief Text is flush right/ragged left.
        FlushRight,
        /// @brief Same as FlushRight.
        RaggedLeft = FlushRight,
        /// @brief Text is centered.
        Centered,
        /// @brief Multiline text is tracked (space inserted) to make lines equal width.
        ///     Hair spaces are inserted between each character.
        JustifiedAtCharacter,
        /// @brief Same as JustifiedAtCharacter.
        Justified = JustifiedAtCharacter,
        /// @brief Multiline text is tracked (space inserted) to make lines equal width.
        ///     Hair spaces are inserted between each word.
        JustifiedAtWord
        };

    /// @brief @brief How a label's text is aligned within its user-defined bounding box,
    ///     going from top-to-bottom.
    /// @note This is only relevant if a Label is using a minimum user-defined size,
    ///     and only if the user-defined size is taller than the text.
    /// @sa Wisteria::GraphItems::Label::SetMinimumUserSizeDIPs(), TextAlignment.
    enum class PageVerticalAlignment
        {
        TopAligned,   /*!< Text is aligned to the top of the label's bounding box.*/
        Centered,     /*!< Text is centered label's bounding box. (This is the default.)*/
        BottomAligned /*!< Text is aligned to the bottom of the label's bounding box.*/
        };

    /// @brief @brief How a label's text is aligned within its user-defined bounding box,
    ///     going from left-to-right.
    /// @note This is only relevant if a Label is using a minimum user-defined size,
    ///     and only if the user-defined size is wider than the text.
    /// @sa Wisteria::GraphItems::Label::SetMinimumUserSizeDIPs(), TextAlignment.
    enum class PageHorizontalAlignment
        {
        LeftAligned,   /*!< Text is aligned to the left of the label's bounding box.
                            (This is the default.)*/
        Centered,      /*!< Text is centered label's bounding box.*/
        RightAligned   /*!< Text is aligned to the right of the label's bounding box.*/
        };

    /// @brief Background visual styles to apply a label.
    enum class LabelStyle
        {
        NoLabelStyle,                    /*!< No extra visual style should be applied to the label,
                                              other than possible outlining.*/
        IndexCard,                       /*!< Display the label as an index card.*/
        LinedPaper,                      /*!< Display the label as lined paper
                                              (lines under each text line).*/
        LinedPaperWithMargins,           /*!< Display the label as lined paper
                                              (lines under each text line, within the margins
                                               of the label).*/
        DottedLinedPaper,                /*!< Display the label as dotted lined paper
                                              (lines under each text line).*/
        DottedLinedPaperWithMargins,     /*!< Display the label as dotted lined paper
                                              (lines under each text line, within the margins
                                              of the label).*/
        RightArrowLinedPaper,            /*!< Display the label as lined paper
                                              (right arrow lines under each text line).\n
                                              Will use the same pen as the label's text.*/
        RightArrowLinedPaperWithMargins, /*!< Display the label as lined paper
                                              (right arrow lines under each text line,
                                              within the margins of the label).
                                              Will use the same pen as the label's text.*/
        /// @private
        LABEL_STYLE_COUNT
        };

    /// @brief The orientation of an item (e.g., a vertically drawn label).
    enum class Orientation
        {
        Horizontal, /*!< Horizontal (i.e., left to right).*/
        Vertical,   /*!< Vertical (i.e., top to bottom).*/
        Both,       /*!< Both horizontal and vertical.*/
        /// @private
        ORIENTATION_COUNT
        };

    /// @brief A hint as to where a generated legend may be placed on a canvas.
    ///     These hints are used by a plot to determine how padding, outlining,
    ///     and canvas proportions should be used when creating a legend.
    enum class LegendCanvasPlacementHint
        {
        EmbeddedOnGraph,    /*!< The legend will be on the plot.
                                 This will include outlining on the legend*/
        LeftOfGraph,        /*!< The legend will be on the right or left of the plot.
                                 This will set the legend's canvas width % to a calculated value.*/
        RightOfGraph,       /*!< The legend will be on the right or left of the plot.
                                 This will set the legend's canvas width % to a calculated value.*/
        AboveOrBeneathGraph /*!< The legend will be above or below the plot.
                                 This will set the legend's canvas width % to 1.*/
        };

    /// @brief Date intervals used along axes.
    enum class DateInterval
        {
        FiscalQuarterly, /*!< Fiscal year, by quarters.*/
        Monthly,         /*!< Months.*/
        Weekly,          /*!< Weeks.*/
        Daily            /*!< Days.*/
        };

    /// @brief Types of fiscal years (based on start date).
    enum class FiscalYear
        {
        Education, /*!< K-12 and College FY (July 1st to June 30th).*/
        USBusiness /*!< US Businesses (April 1st to March 31st).*/
        };

    /// @brief Types of brackets to shown along an axis.
    enum class BracketType
        {
        FiscalQuarterly /*!< Fiscal year, by quarters.*/
        };

    /// @brief The type of axis.
    enum class AxisType
        {
        BottomXAxis, /*!< The bottom X axis.*/
        TopXAxis,    /*!< The top X axis.*/
        LeftYAxis,   /*!< The left Y axis.*/
        RightYAxis   /*!< The right Y axis.*/
        };

    /// @brief How to draw the labels in relation to their parent axis.
    enum class AxisLabelOrientation
        {
        Parallel,     /*!< Draw labels parallel to the axis.*/
        Perpendicular /*!< Draw labels perpendicular to axis.*/
        };

    /// @brief How to display axis labels.
    enum class AxisLabelDisplay
        {
        DisplayCustomLabelsOrValues,  /*!< Display either custom label (if available)
                                           or numeric value.*/
        DisplayOnlyCustomLabels,      /*!< Only show as a custom label; nothing will
                                           be displayed if custom label isn't available.*/
        DisplayCustomLabelsAndValues, /*!< Display both numeric value and custom label.*/
        NoDisplay                     /*!< Do not display any label.*/
        };

    /// @brief How to align perpendicular labels with their parent axis or bracket.
    enum class AxisLabelAlignment
        {
        AlignWithAxisLine, /*!< The labels will be flush right against the axis line.*/
        AlignWithBoundary, /*!< If the labels are perpendicular against the axis and some are wider
                                than others, then align the labels against the outer parameter of
                                the axis area.
                                @note Has no effect with parallel axes.*/
        CenterOnAxisLine   /*!< The labels will be centered on the axis line.
                                @note Has no effect on bracket labels.
                                @todo Add support for horizontal axes.*/
        };

    /// @brief The type of cap (i.e., head) that an axis line displays at its ending point
    ///     (right for horizontal, top for vertical).
    enum class AxisCapStyle
        {
        Arrow, /*!< The top or right end of the axis line is an arrow.*/
        NoCap  /*!< Nothing is drawn at the end of the axis line.*/
        };

    /// @brief How the segments between the points on a line are connected.
    /// @note Setting the drawing pen to `wxNullPen` will turn off line drawing.
    enum class LineStyle
        {
        Lines,  /*!< Each pair of points are connected with a regular line.*/
        Arrows, /*!< Each pair of points are connected with a line with a terminal arrow.*/
        Spline  /*!< Consecutive valid points are connected with a spline.*/
        };

    /// @brief Box rendering options (used for bar charts, box plots, etc.)
    enum class BoxEffect
        {
        /// @brief Solid color.
        Solid,
        /// @brief Glass effect.
        Glassy,
        /// @brief Color gradient, bottom-to-top.
        FadeFromBottomToTop,
        /// @brief Color gradient, left-to-right.
        FadeFromLeftToRight = FadeFromBottomToTop,
        /// @brief Color gradient, top-to-bottom.
        FadeFromTopToBottom,
        /// @brief Color gradient, right-to-left.
        FadeFromRightToLeft = FadeFromTopToBottom,
        /// @brief Fill with repeating images.
        Stipple,
        /// @brief A subimage of a larger image shared by all boxes.
        CommonImage,
        /// @brief An image scaled down to fit the box.
        Image,
        /// @brief A watercolor-like effect, where the box is warped and looks
        ///     like it was filled in with watercolor paint (or a marker).\n
        ///     Note that if an outline pen is in use, it will be drawn over the
        ///     fill color, giving the look that it showing through the "watercolor."
        WaterColor,
        /// @private
        EFFECTS_COUNT
        };

    /// @brief Pie slice rendering options (used for pie charts.)
    enum class PieSliceEffect
        {
        /// @brief Solid color.
        Solid,
        /// @brief An image scaled down to fit the slice
        ///     (or repeated as a pattern if smaller than the slice area).
        Image
        };

    /// @brief How the corners of how various boxes are drawn.
    enum class BoxCorners
        {
        Straight, /*!< Straight lines meet at the corner.*/
        Rounded   /*!< Corners are rounded.
                       The roundedness level can be controlled via
                       Settings::SetBoxRoundedCornerRadius().*/
        };

    /// @brief The type of label to display for a bin (i.e., a bar, pie slice, etc.).
    enum class BinLabelDisplay
        {
        /** @brief The number of items in (or aggregated value of) each bin.*/
        BinValue,
        /** @brief The percentage of items in (or aggregated value of) each bin.*/
        BinPercentage,
        /** @brief Both the percentage and number of items in
                (or aggregated value of) each bin.*/
        BinValueAndPercentage,
        /** @brief Don't display labels on the bins.*/
        NoDisplay,
        /** @brief The name of the bin (e.g., the group name).*/
        BinName,
        /** @brief The name of the bin (e.g., the group name) and the value.*/
        BinNameAndValue,
        /** @brief The name of the bin (e.g., the group name) and the
                percentage of items in (or aggregated value of) each bin.*/
        BinNameAndPercentage,
        /// @private
        BIN_LABEL_DISPLAY_COUNT
        };

    /// @brief How to round floating-point values when binning.
    enum class RoundingMethod
        {
        Round,                 /*!< Round up or down.*/
        RoundDown,             /*!< Round down (ceiling).*/
        RoundUp,               /*!< Round up (floor).*/
        NoRounding,            /*!< Do not round.*/
        /// @private
        ROUNDING_METHOD_COUNT
        };

    /// @brief How an object resized to fit into a new bounding box.
    enum class ResizeMethod
        {
        /// @brief Make the item smaller or larger to fit the bounding box.
        DownscaleOrUpscale,
        /// @brief Only make items smaller if necessary.
        DownscaleOnly,
        /// @brief Only make items larger if necessary.
        UpscaleOnly,
        /// @brief Don't rescale the item.
        NoResize
        };

    /// @brief Lamba to return a color if a point's
    ///     X and/or Y values meet a certain set of criteria.\n
    /// @details Should return an invalid color if values do not meet the criteria.
    using PointColorCriteria = std::function<wxColour(double x, double y)>;

    /// @brief Base class for a list of line styles to use for groups.
    /// @details This is used for line plots and includes the line's pen style and
    ///     how points between the lines are connected (e.g., arrow lines, splines, etc.).
    class LineStyleScheme
        {
    public:
        /// @brief Constructor.
        /// @param penStyles The list of pen & line styles to fill the scheme with.
        explicit LineStyleScheme(const std::vector<std::pair<wxPenStyle, LineStyle>>& penStyles) :
            m_lineStyles(penStyles)
            {}
        /// @private
        explicit LineStyleScheme(std::vector<std::pair<wxPenStyle, LineStyle>>&& penStyles) :
            m_lineStyles(std::move(penStyles))
            {}
        /// @brief Constructor.
        /// @param penStyles The initializer list of pen & line styles to fill the scheme with.
        explicit LineStyleScheme(const std::initializer_list<std::pair<wxPenStyle, LineStyle>>& penStyles) :
            m_lineStyles(penStyles)
            {}
        /// @returns The vector of pen & line styles from the scheme.
        [[nodiscard]] const std::vector<std::pair<wxPenStyle, LineStyle>>&
            GetLineStyles() const noexcept
            { return m_lineStyles; }
        /** @returns The line style from a given index.
            @param index The index into the line style list to return. If index is outside
                number of line styles, then it will recycle (i.e., wrap around).
                For example, if there are 2 line styles, index 1 will return 1;
                however, index 2 will wrap around and return line style 0 and
                index 3 will return line style 1.*/
        [[nodiscard]] const std::pair<wxPenStyle,LineStyle>&
            GetLineStyle(const size_t index) const
            { return m_lineStyles.at(index%m_lineStyles.size()); }
        /** @brief Adds a line style to the scheme.
            @param penStyle The line style.
            @param lineStyle The line style.*/
        void AddLineStyle(const wxPenStyle penStyle, const LineStyle lineStyle)
            { m_lineStyles.push_back(std::make_pair(penStyle, lineStyle)); }
        /// @brief Removes all line styles from the collection.
        void Clear() noexcept
            { m_lineStyles.clear(); }
    private:
        std::vector<std::pair<wxPenStyle, LineStyle>> m_lineStyles;
        };

    /// @brief Standard line styles.
    /// @details This iterates through all pen styles with straight connection lines,
    ///     then goes through the pen styles again with arrow connection lines.
    /// @note Splines are not used here in an effort to keep a consistent look of
    ///     straight lines.
    class StandardLineStyles : public LineStyleScheme
        {
    public:
        /// @brief Constructor.
        StandardLineStyles() : LineStyleScheme({
            { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_DOT, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_LONG_DASH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_SHORT_DASH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_DOT_DASH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_BDIAGONAL_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_CROSSDIAG_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_FDIAGONAL_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_CROSS_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_HORIZONTAL_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_VERTICAL_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_FIRST_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_LAST_HATCH, LineStyle::Lines },
            { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_DOT, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_LONG_DASH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_SHORT_DASH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_DOT_DASH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_BDIAGONAL_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_CROSSDIAG_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_FDIAGONAL_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_CROSS_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_HORIZONTAL_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_VERTICAL_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_FIRST_HATCH, LineStyle::Arrows },
            { wxPenStyle::wxPENSTYLE_LAST_HATCH, LineStyle::Arrows }
            })
            {}
        };

    namespace Colors
        {
        /// @brief Structure for describing a gradient flood fill.
        struct GradientFill
            {
        public:
            /// @private
            GradientFill() = default;
            /** @brief Constructor.
                @param col The color to paint with.*/
            explicit GradientFill(const wxColour& col) : m_color1(col)
                {}
            /** @brief Constructor, which will paint with a gradient.
                @param col1 The first color of the gradient.
                @param col2 The second color of the gradient.
                @param dir The direction of the gradient.*/
            GradientFill(const wxColour& col1, const wxColour& col2,
                         const FillDirection dir) noexcept :
                m_color1(col1), m_color2(col2), m_direction(dir)
                {}
            /// @returns @c true if the primary color has been specified.
            [[nodiscard]] bool IsOk() const
                { return m_color1.IsOk(); } // we use either the first color or both
            /// @returns @c true if a gradient is being used.
            [[nodiscard]] bool IsGradient() const
                { return (m_color1.IsOk() && m_color2.IsOk()); }
            /// @returns The primary color. If a gradient, returns the first color of the gradient.
            [[nodiscard]] const wxColour& GetColor1() const noexcept
                { return m_color1; }
            /// @returns The second color of the gradient.
            [[nodiscard]] const wxColour& GetColor2() const noexcept
                { return m_color2; }
            /// @returns The direction of the gradient (if a gradient is being used).
            [[nodiscard]] FillDirection GetDirection() const noexcept
                { return m_direction; }
        private:
            // leave these uninitialized, client must explicitly set these
            wxColour m_color1;
            wxColour m_color2;
            FillDirection m_direction{ FillDirection::South };
            };
        }

    /// @brief Base items that can be drawn on a plot or canvas.
    namespace GraphItems
        {
        /// @brief Options for setting and customizing the top line of a label as its header.
        class HeaderInfo
            {
        public:
            /// @brief Whether the top line of a label object is a header.
            /// @returns @c true if the top line is treated like a header.
            [[nodiscard]] bool IsEnabled() const noexcept
                { return m_enabled; }
            /** @brief Specifies whether to treat the top line of the label as a header.
                @param enable @c true to enable header mode.
                @returns A self reference.*/
            HeaderInfo& Enable(const bool enable) noexcept
                {
                m_enabled = enable;
                return *this;
                }

            /// @brief Gets the text alignment of the header.
            /// @returns The text alignment.
            [[nodiscard]] TextAlignment GetLabelAlignment() const noexcept
                { return m_alignment; }
            /** @brief Specifies align the top line of the label.
                @param alignment How to align the top line.
                @returns A self reference.*/
            HeaderInfo& LabelAlignment(const TextAlignment alignment) noexcept
                {
                m_alignment = alignment;
                return *this;
                }

            /// @brief Gets the font color of the header.
            /// @returns The font color.
            [[nodiscard]] wxColour GetFontColor() const noexcept
                { return m_fontColor; }
            /** @brief Specifies the font color for the top line of the label.
                @param fontColor The font color for the top line.
                @returns A self reference.*/
            HeaderInfo& FontColor(const wxColour fontColor) noexcept
                {
                m_fontColor = fontColor;
                return *this;
                }

            /** @brief Specifies the font for the top line of the label.
                @param font The font for the top line.
                @returns A self reference.*/
            HeaderInfo& Font(const wxFont font) noexcept
                {
                m_font = font;
                return *this;
                }
            /** @brief Gets/sets the top line's font.
                @returns The top line font.*/
            [[nodiscard]] wxFont& GetFont() noexcept
                { return m_font; }

            /** @brief Gets the top line's scaling, relative to the rest of the text.
                @returns The top line's scaling.*/
            [[nodiscard]] double GetRelativeScaling() const noexcept
                { return m_relativeScaling; }
            /** @brief Specifies the top line's scaling, relative to the rest of the text.
                @param scaling The relative scaling for the top line.
                @returns A self reference.*/
            HeaderInfo& RelativeScaling(const double scaling) noexcept
                {
                m_relativeScaling = scaling;
                return *this;
                }

            /// @private
            [[nodiscard]] const wxFont& GetFont() const noexcept
                { return m_font; }
        private:
            TextAlignment m_alignment{ TextAlignment::FlushLeft };
            bool m_enabled{ false };
            wxFont m_font{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
            wxColour m_fontColor{ *wxBLACK };
            double m_relativeScaling{ 1.0 };
            };

        /// @brief Stores base information for an item being drawn on a plot or canvas.
        class GraphItemInfo
            {
            friend class GraphItemBase;
        public:
            /// @brief Constructor.
            /// @param text The string displayed on the item.
            explicit GraphItemInfo(const wxString& text) : m_text(text)
                {}
            /// @private
            GraphItemInfo() = default;
            /// @brief Sets the ID.
            /// @param id The ID to assign to this object.
            /// @returns A self reference.
            GraphItemInfo& Id(const long id) noexcept
                {
                m_id = id;
                return *this;
                }
            /// @brief Sets whether to show the object.
            /// @param show @c true to display the object.
            /// @returns A self reference.
            GraphItemInfo& Show(const bool show) noexcept
                {
                m_show = show;
                return *this;
                }
            /// @brief Sets whether the object is selectable.
            /// @param selectable @c true to show the item as selected with clicked on.
            /// @returns A self reference.
            GraphItemInfo& Selectable(const bool selectable) noexcept
                {
                m_isSelectable = selectable;
                return *this;
                }
            /// @brief Sets whether the object is <b>not</b> bound to its parent canvas.
            /// @param freeFloating @c true to not anchor this object to its parent canvas.
            /// @returns A self reference.
            GraphItemInfo& FreeFloating(const bool freeFloating) noexcept
                {
                m_freeFloating = freeFloating;
                return *this;
                }
            /// @brief Sets whether the object should display a label on itself when
            ///     selected by the mouse.
            /// @param showLabelWhenSelected @c true to show a label on the object when selected.
            /// @returns A self reference.
            GraphItemInfo& ShowLabelWhenSelected(const bool showLabelWhenSelected) noexcept
                {
                m_showLabelWhenSelected = showLabelWhenSelected;
                return *this;
                }
            /** @brief Sets the margins for this object when being used as separate grid
                    in a multi-item canvas, starting at 12 o'clock and going clockwise.
                @param top The top margin.
                @param right The right margin.
                @param bottom The bottom margin.
                @param left The left margin.
                @returns A self reference.
                @note This will have no effect unless it is the only object in a canvas's grid.*/
            GraphItemInfo& CanvasPadding(const wxCoord top, const wxCoord right,
                                         const wxCoord bottom, const wxCoord left) noexcept
                {
                m_topCanvasMargin = top;
                m_rightCanvasMargin = right;
                m_bottomCanvasMargin = bottom;
                m_leftCanvasMargin = left;
                return *this;
                }
            /// @brief Tells the canvas that this object's parent row should be as tall as
            ///     this object's height (at the default scaling) and no more.
            /// @details By default, this is false and canvases will stretch all of its rows
            ///     (and the items in them) equally height-wise.
            /// @param fit @c true for the object to force the row to fit its content height-wise
            ///     (and no more).
            /// @note If two items in a given row have this set to @c true, then the row will
            ///     be the maximum height of the two items. This will result in the smaller
            ///     item being stretched taller.\n
            ///     Also, the client will need to call Canvas::CalcRowDimensions() after setting all
            ///     objects into its grid for this to take effect.
            /// @returns A self reference.
            GraphItemInfo& FitCanvasHeightToContent(const bool fit) noexcept
                {
                m_fitCanvasRowToContent = fit;
                return *this;
                }
            /// @brief Tells the canvas to allocate just the necessary width for this item's width
            ///     (at default scaling) within its row, and nothing more.
            /// @details This is usually used for legends off to the side of a graph.
            /// @details By default, this is false and canvases will stretch all items in a given
            ///     row equally width-wise.
            /// @param fit @c true for the object to consume exactly enough space for its content
            ///     (and no more).
            /// @note Client will need to call Canvas::CalcRowDimensions() after setting all objects
            ///     into its grid for this to take effect.
            /// @returns A self reference.
            GraphItemInfo& FixedWidthOnCanvas(const bool fit) noexcept
                {
                m_fitContentWidthToCanvas = fit;
                return *this;
                }
            /// @brief Sets the percent of the canvas width that this object should consume.
            /// @param canvasWidthProportion The percent of the canvas that this
            ///     object should consume.
            /// @returns A self reference.
            GraphItemInfo& CanvasWidthProportion(const double canvasWidthProportion) noexcept
                {
                m_canvasWidthProportion = canvasWidthProportion;
                return *this;
                }
            /// @brief Sets the percent of the canvas height that this object should consume.
            /// @note The object will be drawn from its row position, so this is recommended
            ///     only for items in the top row. For example, if this is set to @c 1.0 for an item
            ///     in the top row of a two-row canvas, then it will consume both rows. However,
            ///     if an item in a two-row canvas is set to @c 1.0, then the bottom half of the
            ///     object will go off of the canvas. Also, this should only be used if the canvas
            ///     is aligning its columns' content; otherwise, this object will be drawn on
            ///     top of rows' content beneath it. Likewise, the canvas's row alignment should
            ///     be turned off, as that would try to adjust the object's height and
            ///     negate this setting.
            /// @param canvasHeightProportion The percent of the canvas height that this object
            ///     should consume.
            /// @returns A self reference.
            GraphItemInfo& CanvasHeightProportion(
                const std::optional<double> canvasHeightProportion) noexcept
                {
                m_canvasHeightProportion = canvasHeightProportion;
                return *this;
                }
            /// @brief Sets the text of the item. Depending on context, this may be a label
            ///     when the item is selected, or text shown on the item.
            /// @param text The string to display.
            /// @returns A self reference.
            GraphItemInfo& Text(const wxString& text)
                {
                m_text = text;
                return *this;
                }
            /// @brief Sets the anchoring.
            /// @param anchoring The anchoring to use.
            /// @returns A self reference.
            GraphItemInfo& Anchoring(const Wisteria::Anchoring anchoring) noexcept
                {
                m_anchoring = anchoring;
                return *this;
                }
            /// @brief Sets the pen.
            /// @param pen The outlining pen.
            /// @returns A self reference.
            GraphItemInfo& Pen(const wxPen& pen)
                {
                m_pen = pen;
                return *this;
                }
            /// @brief Sets the brush.
            /// @param brush The brush to fill the object with.
            /// @returns A self reference.
            GraphItemInfo& Brush(const wxBrush& brush)
                {
                m_brush = brush;
                return *this;
                }
            /// @brief Sets the brush used for when the item is selected by the mouse.
            /// @param selectionBrush The brush to fill the object with when selected.
            /// @returns A self reference.
            GraphItemInfo& SelectionBrush(const wxBrush& selectionBrush)
                {
                m_selectionBrush = selectionBrush;
                return *this;
                }
            /// @brief Sets the base color, painted underneath the brush.
            /// @details This is useful if the brush is a hatch pattern.
            /// @param color The base color.
            /// @returns A self reference.
            GraphItemInfo& BaseColor(const std::optional<wxColour>& color)
                {
                m_baseColor = color;
                return *this;
                }
            /// @brief Sets the scaling.
            /// @param scaling The object's scaling.
            /// @returns A self reference.
            GraphItemInfo& Scaling(const double scaling) noexcept
                {
                m_scaling = scaling;
                return *this;
                }
            /// @brief Sets the DPI scaling.
            /// @param scaling The object's DPI scaling.
            /// @returns A self reference.
            GraphItemInfo& DPIScaling(const double scaling) noexcept
                {
                m_dpiScaleFactor = scaling;
                return *this;
                }
            /// @brief Sets the anchor point.
            /// @param pt The object's point on the parent.
            /// @returns A self reference.
            /// @sa Anchoring().
            /// @note This will not apply to objects with their own set of multiple
            ///     points (e.g., Polygon, Axis).
            GraphItemInfo& AnchorPoint(const wxPoint pt)
                {
                m_point = pt;
                return *this;
                }
            /// @brief Sets the relative alignment within the object's parent.
            /// @param alignment The object's relative alignment.
            /// @returns A self reference.
            GraphItemInfo& ChildAlignment(const RelativeAlignment alignment) noexcept
                {
                m_relativeAlignment = alignment;
                return *this;
                }
            /** @brief Sets the orientation of the text (if a label).
                @param orientation The orientation of the text.
                @returns A self reference.
                @note Label objects also have a Label::Tilt() function to tilt the text,
                    using its initial orientation as its basis.*/
            GraphItemInfo& Orient(const Orientation orientation) noexcept
                {
                m_orientation = orientation;
                return *this;
                }
            /** @brief Sets the padding, starting at 12 o'clock and going clockwise.
                @param top The top padding.
                @param right The right padding.
                @param bottom The bottom padding.
                @param left The left padding.
                @returns A self reference.*/
            GraphItemInfo& Padding(const wxCoord top, const wxCoord right,
                                   const wxCoord bottom, const wxCoord left) noexcept
                {
                m_topPadding = top;
                m_rightPadding = right;
                m_bottomPadding = bottom;
                m_leftPadding = left;
                return *this;
                }
            /** @brief Set how the label should adjust (if at all) its content
                    to fit inside its parent.
                @details This controls how to draw the label across an element
                    (and possibly fit inside it).\n
                    An example of this could be a label drawn on the center of a bar on a bar chart.
                    Essentially, this is used when the parent it treating this label like a decal.
                @param labelFit The fitting method to use.
                @returns A self reference.*/
            GraphItemInfo& LabelFitting(const LabelFit labelFit) noexcept
                {
                m_labelFit = labelFit;
                return *this;
                }
            /// @brief Sets the text alignment
            ///     (applies to Label objects or labels managed by the object, such as Axis).
            /// @param alignment How the text is alignment.
            /// @returns A self reference.
            GraphItemInfo& LabelAlignment(const TextAlignment& alignment) noexcept
                {
                m_textAlignment = alignment;
                return *this;
                }
            /// @brief Sets the text's vertical alignment
            ///     (applies to Label objects using a minimum user-defined size).
            /// @param alignment How the text is alignment.
            /// @returns A self reference.
            GraphItemInfo& LabelPageVerticalAlignment(
                const PageVerticalAlignment& alignment) noexcept
                {
                m_pageVerticalAlignment = alignment;
                return *this;
                }
            /// @brief Sets the text's horizontal alignment
            ///     (applies to Label objects using a minimum user-defined size).
            /// @param alignment How the text is alignment.
            /// @returns A self reference.
            GraphItemInfo& LabelPageHorizontalAlignment(
                const PageHorizontalAlignment& alignment) noexcept
                {
                m_pageHorizontalAlignment = alignment;
                return *this;
                }
            /// @brief Sets how to style the label background (applies only to Label).
            /// @param style How to style the label.
            /// @returns A self reference.
            GraphItemInfo& LabelStyling(const LabelStyle& style) noexcept
                {
                m_labelStyle = style;
                return *this;
                }
            /// @brief Sets the font.
            /// @param font The font.
            /// @returns A self reference.
            GraphItemInfo& Font(const wxFont& font)
                {
                m_font = font;
                return *this;
                }
            /// @brief Sets the font color.
            /// @param textColor The font color.
            /// @returns A self reference.
            GraphItemInfo& FontColor(const wxColour textColor)
                {
                m_textColor = textColor;
                return *this;
                }
            /// @brief Sets the font background color.
            /// @param textColor The font background color.
            /// @returns A self reference.
            GraphItemInfo& FontBackgroundColor(const wxColour textColor)
                {
                m_textBgColor = textColor;
                return *this;
                }
            /// @brief Sets whether the object is valid.
            /// @param isOk @c true to mark the object is valid.
            /// @returns A self reference.
            GraphItemInfo& Ok(const bool isOk) noexcept
                {
                m_isOk = isOk;
                return *this;
                }
            /// @brief Sets the area that the drawing of this object is restricted to.
            /// @param clippingRect The area to clip drawing to.
            /// @returns A self reference.
            GraphItemInfo& ClippingRect(const wxRect& clippingRect)
                {
                m_clippingRect = clippingRect;
                return *this;
                }
            /// @brief Sets the flags for which outlines around the object are shown.
            /// @param top @c true to show the top outline.
            /// @param right @c true to show the right outline.
            /// @param bottom @c true to show the bottom outline.
            /// @param left @c true to show the left outline.
            /// @note This is only relevant for object which are meant to draw an outline
            ///     (e.g., Labels and Graphs). This only returns the object's flag for this
            ///     option, which may be irrelevant for some objects.\n
            ///     Also, if the object is using a box corner style that is set to
            ///     @c BoxCorners::Rounded, then these flags will be ignored and the entire
            ///     outline is drawn. (This is the case for Labels.)
            ///     Finally, note that this is turned off for all objects by default
            ///     *except* for Labels.
            /// @returns A self reference.
            GraphItemInfo& Outline(const bool top, const bool right,
                                   const bool bottom, const bool left)
                {
                m_outline.set(0, top);
                m_outline.set(1, right);
                m_outline.set(2, bottom);
                m_outline.set(3, left);
                return *this;
                }
            // Accessors
            //----------
            /// @returns The scaling.
            [[nodiscard]] double GetScaling() const noexcept
                { return m_scaling; }
            /// @returns The scaling when the item was first embedded onto a canvas.
            /// @note This is only relevant for object embedded into a canvas's grid
            ///     and should only be used by canvases internally.
            [[nodiscard]] double GetOriginalCanvasScaling() const noexcept
                { return m_originalCanvasScaling; }
            /// @returns The DPI scaling.
            [[nodiscard]] std::optional<double> GetDPIScaleFactor() const noexcept
                { return m_dpiScaleFactor; }
            /// @returns The brush.
            [[nodiscard]] const wxBrush& GetBrush() const noexcept
                { return m_brush; }
            /// @returns The base color.
            [[nodiscard]] const std::optional<wxColour>& GetBaseColor() const noexcept
                { return m_baseColor; }
            /// @returns The pen.
            [[nodiscard]] const wxPen& GetPen() const noexcept
                { return m_pen; }
            /// @returns The text.
            [[nodiscard]] const wxString& GetText() const noexcept
                { return m_text; }
            /// @returns @c true if drawing a top border (with the object's pen).
            /// @note This is only relevant for object which are meant to draw an outline
            ///     (e.g., Labels and Graphs). This only returns the object's flag for this
            ///     option, which may be irrelevant for some objects.
            [[nodiscard]] bool IsShowingTopOutline() const noexcept
                { return m_outline[0]; }
            /// @returns @c true if drawing a right border (with the object's pen).
            /// @note This is only relevant for object which are meant to draw an outline
            ///     (e.g., Labels and Graphs). This only returns the object's flag for this
            ///     option, which may be irrelevant for some objects.
            [[nodiscard]] bool IsShowingRightOutline() const noexcept
                { return m_outline[1]; }
            /// @returns @c true if drawing a bottom border (with the object's pen).
            /// @note This is only relevant for object which are meant to draw an outline
            ///     (e.g., Labels and Graphs). This only returns the object's flag for this
            ///     option, which may be irrelevant for some objects.
            [[nodiscard]] bool IsShowingBottomOutline() const noexcept
                { return m_outline[2]; }
            /// @returns @c true if drawing a left border (with the object's pen).
            /// @note This is only relevant for object which are meant to draw an outline
            ///     (e.g., Labels and Graphs). This only returns the object's flag for this
            ///     option, which may be irrelevant for some objects.
            [[nodiscard]] bool IsShowingLeftOutline() const noexcept
                { return m_outline[3]; }
        private:
            bool m_show{ true };
            bool m_isSelectable{ true };
            bool m_freeFloating{ false };
            bool m_showLabelWhenSelected{ true };
            // ID
            long m_id{ wxID_ANY };
            std::set<long> m_selectedIds; // possible subitems
            // parent canvas info
            double m_canvasWidthProportion{ 1.0 };
            std::optional<double> m_canvasHeightProportion{ std::nullopt };
            RelativeAlignment m_relativeAlignment{ RelativeAlignment::Centered };
            wxCoord m_rightCanvasMargin{ 0 };
            wxCoord m_leftCanvasMargin{ 0 };
            wxCoord m_topCanvasMargin{ 0 };
            wxCoord m_bottomCanvasMargin{ 0 };
            bool m_fitCanvasRowToContent{ false };
            bool m_fitContentWidthToCanvas{ false };
            // labels and drawing
            wxPen m_pen{ *wxBLACK_PEN };
            wxBrush m_brush{ *wxWHITE_BRUSH };
            wxBrush m_selectionBrush{ wxNullBrush };
            std::bitset<4> m_outline{ 0 };
            /// @brief A color to show under the brush if it is hatch pattern.
            std::optional<wxColour> m_baseColor{ std::nullopt };
            Wisteria::Anchoring m_anchoring{ Anchoring::Center };
            LabelFit m_labelFit{ LabelFit::DisplayAsIsAutoFrame };
            Wisteria::Orientation m_orientation{ Orientation::Horizontal };
            wxCoord m_rightPadding{ 0 };
            wxCoord m_leftPadding{ 0 };
            wxCoord m_topPadding{ 0 };
            wxCoord m_bottomPadding{ 0 };
            TextAlignment m_textAlignment{ TextAlignment::FlushLeft };
            PageVerticalAlignment m_pageVerticalAlignment{ PageVerticalAlignment::TopAligned };
            PageHorizontalAlignment m_pageHorizontalAlignment{ PageHorizontalAlignment::LeftAligned };
            wxColour m_textColor{ *wxBLACK };
            wxColour m_textBgColor{ wxNullColour };
            wxString m_text;
            LabelStyle m_labelStyle{ LabelStyle::NoLabelStyle };
            wxFont m_font{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
            HeaderInfo m_headerInfo;
            std::optional<wxCoord> m_minimumUserWidthDIPs{ std::nullopt };
            std::optional<wxCoord> m_minimumUserHeightDIPs{ std::nullopt };
            std::optional<wxRect> m_clippingRect;

            bool m_isOk{ true };
            /*!< @todo Expand support for this for Label.*/
            ShadowType m_shadowType{ ShadowType::NoShadow };
            std::vector<Wisteria::Icons::LegendIcon> m_legendIcons;
            // center point
            wxPoint m_point{ 0, 0 };
            // scaling
            double m_scaling{ 1 };
            double m_originalCanvasScaling{ 1 };
            std::optional<double> m_dpiScaleFactor{ std::nullopt };
            };

        /// @brief Abstract class for elements that can be drawn on a canvas.
        class GraphItemBase
            {
            friend class Wisteria::Canvas;
            friend class Wisteria::CanvasItemScalingChanger;
            friend class Graphs::Graph2D;
            friend class Graphs::PieChart;
        public:
            /// @private
            GraphItemBase() noexcept
                {}
            /** @brief Constructor.
                @param scaling The current scaling to measure and render with.
                @param label The label to attach to the element
                    (can be used for things like selection labels).*/
            GraphItemBase(const double scaling, const wxString& label)
                {
                m_itemInfo.m_text = label;
                m_itemInfo.m_scaling = scaling;
                wxASSERT_LEVEL_2_MSG(m_itemInfo.m_scaling > 0,
                                     L"Scaling in canvas object is <= 0?!");
                }
            /** @brief Constructor.
                @param itemInfo Extended information to construct this item with.*/
            explicit GraphItemBase(const GraphItemInfo& itemInfo) : m_itemInfo(itemInfo)
                {}
            /// @private
            virtual ~GraphItemBase() {}

            /** @brief Sets the scaling of the element.
                @details This will affect the thickness of the object's outline.
                    Also, for objects with a center point (@c Image, @c Label, @c Point2D),
                    this will affect the size of the object.
                @param scaling The scaling factor.
                @note Objects with more than a single point (e.g., @c Axis) will
                    maintain its size and position; scaling will not affect it.*/
            virtual void SetScaling(const double scaling)
                {
                wxASSERT_LEVEL_2_MSG(scaling > 0,
                                     L"Scaling in canvas object is less than or equal to zero?!");
                if (scaling <= 0)
                    { return; }
                m_itemInfo.m_scaling = scaling;
                InvalidateCachedBoundingBox();
                }
            /** @returns The scaling of the element.
                @sa SetScaling().*/
            [[nodiscard]] double GetScaling() const noexcept
                { return m_itemInfo.m_scaling; }
            /** @brief Sets the DPI scaling of the element.
                @details This will affect the thickness of the object's outline.
                    Also, for objects with a center point (@c Image, @c Label, @c Point2D),
                    this will affect the size of the object.
                @param scaling The scaling factor.*/
            virtual void SetDPIScaleFactor(const double scaling)
                {
                wxASSERT_LEVEL_2_MSG(scaling > 0,
                                     L"DPI Scaling in canvas object is less than or equal to zero?!");
                if (scaling <= 0)
                    { return; }
                m_itemInfo.m_dpiScaleFactor = scaling;
                InvalidateCachedBoundingBox();
                }
            /** @returns The DPI scaling of the element.
                @sa SetScaling().*/
            [[nodiscard]] double GetDPIScaleFactor() const noexcept;
            /// @returns @c true if the object is not reset to specific coordinates on the canvas
            ///     and has to have its position adjusted as the canvas gets rescaled.
            [[nodiscard]] bool IsFreeFloating() const noexcept
                { return m_itemInfo.m_freeFloating; }
            /** @brief Sets whether the object should be shown.
                @param show Whether to show the object or not.
                @note When this is set to @c false, GetBoundingBox() will generally return
                    an invalid @c wxRect.*/
            void Show(const bool show = true) noexcept
                { m_itemInfo.m_show = show; }
            /// @returns Whether this object is being displayed or not.
            [[nodiscard]] bool IsShown() const noexcept
                { return m_itemInfo.m_show; }
            /// @returns The size of the shadow.
            /// @warning This will need to be scaled with being drawn or measured.
            [[nodiscard]] static constexpr double GetShadowOffset() noexcept
                { return 2; }
            /// @returns The color to draw the shadow of the object.
            [[nodiscard]] static wxColour GetShadowColour()
                { return wxColour(84, 84, 84, 175); }

            /** @brief Sets the point where the box will be anchored.
                @note Call SetAnchoring() to control what this point means in relation
                    to how it is anchored. The default is for this point to be the center point.
                @param pt The point where the box will be anchored.*/
            void SetAnchorPoint(const wxPoint pt)
                {
                m_itemInfo.m_point = pt;
                InvalidateCachedBoundingBox();
                }
            /// @returns The coordinates of where the label will be anchored.
            [[nodiscard]] const wxPoint& GetAnchorPoint() const noexcept
                { return m_itemInfo.m_point; }

            /** @name Selection Functions
                @brief Functions related to how the object behaves when selected.*/
            /// @{

            /// @returns Whether the element is selected.
            [[nodiscard]] bool IsSelected() const noexcept
                { return m_selected; }
            /** @brief Sets whether the element is selected.
                @param selected Whether the element is selected.
                @internal Don't make this @c noexcept as derived versions may need
                 to call @c noexcept functions.*/
            virtual void SetSelected(const bool selected)
                { m_selected = selected; }

            /// @returns Whether the element can be selected.
            [[nodiscard]] bool IsSelectable() const noexcept
                { return m_itemInfo.m_isSelectable; }
            /** @brief Sets whether the element can be selected.
                @param selectable Whether the element can be selected.
                @note It is recommended to check for this in SelectObjectAtPoint()
                    for derived objects if they override that function.*/
            virtual void SetSelectable(const bool selectable) noexcept
                { m_itemInfo.m_isSelectable = selectable; }

            /// @returns Whether a label should be drawn on top of the element when selected.
            [[nodiscard]] bool IsShowingLabelWhenSelected() const noexcept
                { return m_itemInfo.m_showLabelWhenSelected; }
            /** @brief Set whether to show the element's label as a text window on top of
                    the element when selected.
                @param show Whether to show the label.*/
            void ShowLabelWhenSelected(const bool show)
                {
                m_itemInfo.m_showLabelWhenSelected = show;
                InvalidateCachedBoundingBox();
                }

            /** @brief Gets/sets the painting brush used for when the object is selected.
                @returns The painting brush used to fill in the object.
                @sa For polygon objects, Polygon::SetBackgroundFill().*/
            [[nodiscard]] wxBrush& GetSelectionBrush() noexcept
                { return m_itemInfo.m_selectionBrush; }
            /// @}

            /** @brief Sets whether the object should be moved as the canvas scaling is changed.
                @details In other words, the object is not connected to coordinates on the canvas,
                    but rather sits arbitrarily on the canvas and has to have its coordinates
                    adjusted as the canvas gets rescaled.\n
                    This is meant for movable objects on a canvas that a client can manually move.
                @param freeFloat Whether the object should be free floating.*/
            virtual void SetFreeFloating(const bool freeFloat)
                {
                m_itemInfo.m_freeFloating = freeFloat;
                InvalidateCachedBoundingBox();
                }
            /** @returns The element, rendered to a bitmap. The image will be the size of the
                    bounding box. The area around the polygon will be set to transparent pixels.
                @param dc Measurement DC. Not used, just need for API requirements.
                @note This is used for dragging when an object is free floating.*/
            [[nodiscard]] virtual wxBitmap ToBitmap(wxDC& dc) const;
            /** @brief Moves the element by the specified x and y values.
                @param xToMove The amount to move horizontally.
                @param yToMove The amount to move vertically.*/
            virtual void Offset(const int xToMove, const int yToMove) = 0;
            /// @returns The rectangle on the canvas where the element would fit in.
            /// @param dc The DC to measure content with.
            [[nodiscard]] virtual wxRect GetBoundingBox(wxDC& dc) const = 0;
            /** @brief Override this to set the rectangular area of the object.
                @param rect The rectangle to bound the object to.
                    This is relative to the parent canvas.
                @param dc The DC to measure content with.
                @param parentScaling The scaling of the parent drawing this element.
                    Usually is not used, but may be used for objects to have a consistent scaling size.
                @note Derived variations should call InvalidateCachedBoundingBox() and
                    SetCachedBoundingBox().*/
            virtual void SetBoundingBox(const wxRect& rect, wxDC& dc, const double parentScaling) = 0;

            /** @brief Gets/sets the item's base attributes (e.g., anchoring, font info).
                @details This is a convenient way to chain multiple attribute updates.
                @code
                 label->GetGraphItemInfo().Scaling(GetScaling()).
                                           Pen(wxNullPen).Text(_(L"Number of obs."));
                @endcode
                @returns The base attributes.*/
            [[nodiscard]] virtual GraphItemInfo& GetGraphItemInfo()
                {
                InvalidateCachedBoundingBox();
                return m_itemInfo;
                }

            /** @brief Controls the anchoring of this item on its parent.
                @details When an item is drawn, its anchoring indicates what its point
                    is referencing.\n
                    For example, if an item is anchored to its center, then the item's point
                    refers to its center and it will be drawn on its parent based on that.
                @details This can be useful for lining up multiple labels a certain way
                    (e.g., left aligned).
                @param placement The method for how the point controls the anchoring
                    of this object.
                @note This will have no effect on objects with more than one point
                    (e.g., @c Axes::Axis, @c Points2D).
                    This mostly related to objects such as Label and Image.*/
            void SetAnchoring(const Wisteria::Anchoring placement)
                {
                m_itemInfo.m_anchoring = placement;
                InvalidateCachedBoundingBox();
                }

            /// @returns What the object's starting point is referencing when it need to be
            ///     rendered on its parent.
            [[nodiscard]] const Anchoring& GetAnchoring() const noexcept
                { return m_itemInfo.m_anchoring; }

            /** @brief Sets which type of shadow is being drawn under the object
                @param shadow The type of shadow to display.
                @note For some objects, shadow will always be displayed as @c RightSideShadow
                    (unless set to @c NoShadow); otherwise, it would look odd.
                    Set to @c NoShadow to turn off shadows.*/
            void SetShadowType(const ShadowType shadow) noexcept
                { m_itemInfo.m_shadowType = shadow; }
            /** @returns Which type of shadow is being drawn under the object.*/
            [[nodiscard]] ShadowType GetShadowType() const noexcept
                { return m_itemInfo.m_shadowType; }

            /** @name Text Functions
                @brief Functions related to text display.
                @details This applies when the object is a Label.
                    This also applies to the label displayed when this item is selected.*/
            /// @{

            /** @brief Sets the label, which the caller can use (e.g., as a selection label).
                @param label The text for the label.*/
            virtual void SetText(const wxString& label)
                {
                m_itemInfo.m_text = label;
                InvalidateCachedBoundingBox();
                }
            /// @returns The label associated with this element.
            [[nodiscard]] const wxString& GetText() const noexcept
                { return m_itemInfo.m_text; }

            /** @brief Gets/sets the label font.
                @returns The font used for any labelling.
                @note If the top line is being treated as a header, then it will manage its own
                    font. Call GetHeaderInfo() to manage the header line's font.*/
            [[nodiscard]] virtual wxFont& GetFont() noexcept
                {
                InvalidateCachedBoundingBox();
                return m_itemInfo.m_font;
                }
            /** @brief Sets the font.
                @param font The font to use.
                @note Calling GetFont() can access the font directly,
                    which is a simpler way to edit it.*/
            virtual void SetFont(const wxFont& font)
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_font = font;
                }

            /** @returns The text color.*/
            [[nodiscard]] const wxColour& GetFontColor() const noexcept
                { return m_itemInfo.m_textColor; }
            /** @brief Sets the text color.
                @param color The color to use for the font.*/
            virtual void SetFontColor(const wxColour& color)
                { m_itemInfo.m_textColor = color; }

            /** @returns The text background color.*/
            [[nodiscard]] const wxColour& GetFontBackgroundColor() const noexcept
                { return m_itemInfo.m_textBgColor; }
            /** @brief Sets the background color.
                @param color The color to use for the background.*/
            virtual void SetFontBackgroundColor(const wxColour& color)
                { m_itemInfo.m_textBgColor = color; }

            /** @returns The alignment of the first line of text (if multiline).*/
            [[nodiscard]] HeaderInfo& GetHeaderInfo() noexcept
                { return m_itemInfo.m_headerInfo; }

            /** @returns The orientation of the text.*/
            [[nodiscard]] const Orientation& GetTextOrientation() const noexcept
                { return m_itemInfo.m_orientation; }
            /** @brief If a Label, sets the orientation of the text.
                @param orientation The orientation of the text.*/
            void SetTextOrientation(const Orientation orientation) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_orientation = orientation;
                }

            /** @brief If a Label, returns how the label fits within its parent.
                @returns How the label fits within its parent.*/
            [[nodiscard]] LabelFit GetLabelFit() const noexcept
                { return m_itemInfo.m_labelFit; }
            /** @brief If a Label, sets how the label fits within its parent.
                @param labelFit How the label should fit.*/
            void SetLabelFit(const LabelFit labelFit) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_labelFit = labelFit;
                }

            /** @returns If a Label, the alignment of the text
                    (if multiline or using a minimum user-defined size).*/
            [[nodiscard]] const TextAlignment& GetTextAlignment() const noexcept
                { return m_itemInfo.m_textAlignment; }
            /** @brief If a Label, sets the alignment of the text
                    (if multiline or using a minimum user-defined size).
                @param alignment How to align the text.*/
            void SetTextAlignment(const TextAlignment alignment) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_textAlignment = alignment;
                }

            /** @returns The vertical alignment of the item's content within its bounding box.*/
            [[nodiscard]] const PageVerticalAlignment& GetPageVerticalAlignment() const noexcept
                { return m_itemInfo.m_pageVerticalAlignment; }
            /** @brief Sets where an item's content is vertically positioned within its
                    own bounding box.
                @details This only applies to Table and Label objects.\n
                    If a @c Label, sets the vertical alignment of the text
                    (if using a minimum user-defined size).\n
                    If a @c Table, sets where the table is vertically placed within
                    its bounding box.
                @param alignment How to align the content.
                @note This can be used to center or right align a legend vertically
                    if being placed on a canvas (beneath its plot).*/
            void SetPageVerticalAlignment(const PageVerticalAlignment alignment) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_pageVerticalAlignment = alignment;
                }

            /** @returns The horizontal alignment of the item's content within its bounding box.*/
            [[nodiscard]] const PageHorizontalAlignment& GetPageHorizontalAlignment() const noexcept
                { return m_itemInfo.m_pageHorizontalAlignment; }
            /** @brief Sets where an item's content is horizontally positioned within its
                    own bounding box.
                @details This only applies to Table and Label objects.\n
                    If a @c Label, sets the horizontal alignment of the text
                    (if using a minimum user-defined size).\n
                    If a @c Table, sets where the table is horizontally placed within
                    its bounding box.
                @param alignment How to align the content.
                @note This can be used to center or right align a legend horizontally
                    if being placed on a canvas (beneath its plot).*/
            void SetPageHorizontalAlignment(const PageHorizontalAlignment alignment) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_pageHorizontalAlignment = alignment;
                }

            /// @returns The visual style of the label.
            [[nodiscard]] const LabelStyle& GetLabelStyle() const noexcept
                 { return m_itemInfo.m_labelStyle; }
            /** @brief If a @c Label, sets the visual style of the label.
                @param style The visual style to use.*/
            void SetLabelStyle(const LabelStyle style) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_labelStyle = style;
                }

            /** @returns If a @c Label, the collection of icons (optionally) being drawn.
                @note Call SetLeftPadding() to make space for these icons
                    (with a minimum of 16 pixels).*/
            [[nodiscard]] std::vector<Wisteria::Icons::LegendIcon>& GetLegendIcons() noexcept
                {
                InvalidateCachedBoundingBox();
                return m_itemInfo.m_legendIcons;
                }
            /** @returns If a @c Label being used as a legend, @c true if icons have been added to it.
                    This is useful if trying to determine if legend padding is needed for a Label.
                @note This takes into account blank icons and separators that don't require
                    padding to be drawn, so this is more accurate than calling
                    `GetLegendIcons().size()`.*/
            [[nodiscard]] bool HasLegendIcons() const noexcept
                {
                for (const auto& icon : GetLegendIcons())
                    {
                    if (icon.m_shape != Wisteria::Icons::IconShape::Blank &&
                        icon.m_shape != Wisteria::Icons::IconShape::HorizontalSeparator &&
                        icon.m_shape != Wisteria::Icons::IconShape::HorizontalArrowRightSeparator)
                        { return true; }
                    }
                return false;
                }

            /** @brief Gets the minimum width for the item's bounding box that the client
                    has requested.\n
                    This is currently only relevant to @c Label objects.
                @note By default this is optional until the client calls SetMinimumUserSizeDIPs().\n
                    This is the minimum size that the client has requested, which may or may not be
                    the same as the actual content's size (including text, padding, icons, etc.).
                @sa SetMinimumUserSizeDIPs(), GetMinimumUserHeightDIPs().
                @returns The minimum user width.*/
            [[nodiscard]] std::optional<wxCoord> GetMinimumUserWidthDIPs() const noexcept
                { return m_itemInfo.m_minimumUserWidthDIPs; }
            /** @brief Gets the minimum height for the item's bounding box that the client
                    has requested.\n
                    This is currently only relevant to Label objects.
                @note By default this is optional until the client calls SetMinimumUserSizeDIPs().\n
                    This is the minimum size that the client has requested, which may or may not
                    be the same as the actual content's size (including text, padding, icons, etc.).
                @sa SetMinimumUserSizeDIPs(), GetMinimumUserWidthDIPs().
                @returns The minimum user width.*/
            [[nodiscard]] std::optional<wxCoord> GetMinimumUserHeightDIPs() const noexcept
                { return m_itemInfo.m_minimumUserHeightDIPs; }
            /** @brief Sets the minimum size for the item's bounding box.
                    This is currently only relevant to Label objects.
                @details This should include space for the text and its padding.
                @param width The minimum width. Set to @c std::nullopt to ignore it.
                @param height The minimum height. Set to @c std::nullopt to ignore it.
                @note This should be used if you wish to make the label larger than its content.
                    For example, use this to make a series of labels the same width.*/
            void SetMinimumUserSizeDIPs(const std::optional<wxCoord> width,
                                    std::optional<wxCoord> height) noexcept
                {
                m_itemInfo.m_minimumUserWidthDIPs = width;
                m_itemInfo.m_minimumUserHeightDIPs = height;
                InvalidateCachedBoundingBox();
                }

            /** @brief Gets the area that the object's rendering is restricted to.
                @details By default, objects are drawn as-is and are not clipped.
                @returns The clipping area, which by default is @c std::nullopt.*/
            [[nodiscard]] std::optional<wxRect>& GetClippingRect() noexcept
                { return m_itemInfo.m_clippingRect; }
            /** @brief Sets the area that the object's rendering is restricted to.
                @details By default, objects are drawn as-is and are not clipped.
                @param clipRect The clipping rect, or @c std::nullopt to turn off clipping.*/
            void SetClippingRect(std::optional<wxRect> clipRect)
                { m_itemInfo.m_clippingRect = clipRect; }
            /// @}

            /** @name Padding Functions
                @brief Functions related to setting padding around the object.*/
            /// @{

            /** @brief Sets the padding, starting at 12 o'clock and going clockwise.
                @param top The top padding.
                @param right The right padding.
                @param bottom The bottom padding.
                @param left The left padding.
                @note This will only affect Label and Axis;
                    will be ignored by other object types.*/
            virtual void SetPadding(const wxCoord top, const wxCoord right,
                                    const wxCoord bottom, const wxCoord left) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_topPadding = top;
                m_itemInfo.m_rightPadding = right;
                m_itemInfo.m_bottomPadding = bottom;
                m_itemInfo.m_leftPadding = left;
                }
            /// @returns The right padding of the object.
            /// @warning This will need to be scaled when being drawn or measured.
            [[nodiscard]] constexpr wxCoord GetRightPadding() const noexcept
                { return m_itemInfo.m_rightPadding; }
            /** @brief Sets the right padding of the object.
                @param padding The padding size.
                @note This is a pixel value that the framework will scale to
                    the screen for you.*/
            virtual void SetRightPadding(const wxCoord padding) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_rightPadding = padding;
                }
            /// @returns The left padding of the object.
            /// @warning This will need to be scaled when being drawn or measured.
            [[nodiscard]] constexpr wxCoord GetLeftPadding() const noexcept
                { return m_itemInfo.m_leftPadding; }
            /** @brief Sets the left padding of the object.
                @param padding The padding size.
                @note This is a pixel value that the framework will scale to
                    the screen for you.*/
            virtual void SetLeftPadding(const wxCoord padding) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_leftPadding = padding;
                }
            /// @returns The bottom padding of the object.
            /// @warning This will need to be scaled when being drawn or measured.
            [[nodiscard]] constexpr wxCoord GetBottomPadding() const noexcept
                { return m_itemInfo.m_bottomPadding; }
            /** @brief Sets the bottom padding of the object.
                @param padding The padding size.
                @note This is a pixel value that the framework will scale to
                    the screen for you.*/
            virtual void SetBottomPadding(const wxCoord padding) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_bottomPadding = padding;
                }
            /// @returns The top padding of the object.
            /// @warning This will need to be scaled when being drawn or measured.
            [[nodiscard]] constexpr wxCoord GetTopPadding() const noexcept
                { return m_itemInfo.m_topPadding; }
            /** @brief Sets the top padding of the object.
                @param padding The padding size.
                @note This is a pixel value that the framework will scale
                    to the screen for you.*/
            virtual void SetTopPadding(const wxCoord padding) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_topPadding = padding;
                }
            /// @}

            /// @brief Sets the element's identifier value.
            /// @param id The identifier to assign.
            void SetId(const long id) noexcept
                { m_itemInfo.m_id = id; }
            /// @returns The element's identifier value.
            [[nodiscard]] long GetId() const noexcept
                { return m_itemInfo.m_id; }
            /// @returns The list of selected subitem IDs.
            ///  This is only relevant for objects with subitems.
            [[nodiscard]] std::set<long>& GetSelectedIds() noexcept
                { return m_itemInfo.m_selectedIds; }

            /** @brief Gets/sets the pen used for outlining.
                @returns The pen used for outlining.
                @note Set to @c wxNullPen to disable the pen.*/
            [[nodiscard]] wxPen& GetPen() noexcept
                {
                InvalidateCachedBoundingBox();
                return m_itemInfo.m_pen;
                }

            /** @brief Gets/sets the painting brush.
                @returns The painting brush used to fill in the object.
                @sa For polygon objects, Polygon::SetBackgroundFill().*/
            [[nodiscard]] wxBrush& GetBrush() noexcept
                { return m_itemInfo.m_brush; }

            /** @name Canvas Functions
                @brief Functions related to how this object is placed, resized,
                    and padded when inserted into the grid of a multi-item canvas
                    (i.e., a Label being used as a legend next to a plot).*/
            /// @{

            /** @brief Sets the margins for this object when being used as a separate item
                    inside a grid cell of a multi-item canvas, starting at 12 o'clock
                    and going clockwise.
                @param top The top margin.
                @param right The right margin.
                @param bottom The bottom margin.
                @param left The left margin.
                @note This will have no effect unless it is the only object in a
                    canvas's grid cell.*/
            void SetCanvasMargins(const wxCoord top, const wxCoord right,
                                  const wxCoord bottom, const wxCoord left) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_topCanvasMargin = top;
                m_itemInfo.m_rightCanvasMargin = right;
                m_itemInfo.m_bottomCanvasMargin = bottom;
                m_itemInfo.m_leftCanvasMargin = left;
                }
            /// @returns The right margin of the object.
            /// @note This is a DIP value.
            [[nodiscard]] constexpr wxCoord GetRightCanvasMargin() const noexcept
                { return m_itemInfo.m_rightCanvasMargin; }
            /** @brief Sets the right margin of the object.
                @param margin The margin size.
                @note This is a DIP value that the framework will scale for you.
                @sa SetCanvasMargins().*/
            void SetRightCanvasMargin(const wxCoord margin) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_rightCanvasMargin = margin;
                }
            /// @returns The left margin of the object.
            /// @note This is a DIP value.
            [[nodiscard]] constexpr wxCoord GetLeftCanvasMargin() const noexcept
                { return m_itemInfo.m_leftCanvasMargin; }
            /** @brief Sets the left margin of the object.
                @param margin The margin size.
                @note This is a DIP value that the framework will scale for you.
                @sa SetCanvasMargins().*/
            void SetLeftCanvasMargin(const wxCoord margin) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_leftCanvasMargin = margin;
                }
            /// @returns The bottom margin of the object.
            /// @note This is a DIP value.
            [[nodiscard]] constexpr wxCoord GetBottomCanvasMargin() const noexcept
                { return m_itemInfo.m_bottomCanvasMargin; }
            /** @brief Sets the bottom margin of the object.
                @param margin The margin size.
                @note This is a DIP value that the framework will scale for you.
                @sa SetCanvasMargins().*/
            void SetBottomCanvasMargin(const wxCoord margin) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_bottomCanvasMargin = margin;
                }
            /// @returns The top margin of the object.
            /// @note This is a DIP value.
            [[nodiscard]] constexpr wxCoord GetTopCanvasMargin() const noexcept
                { return m_itemInfo.m_topCanvasMargin; }
            /** @brief Sets the top margin of the object.
                @param margin The margin size.
                @note This is a DIP value that the framework will scale for you.
                @sa SetCanvasMargins().*/
            void SetTopCanvasMargin(const wxCoord margin) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_topCanvasMargin = margin;
                }
            /// @returns @c true if this object's parent row should be as tall as
            ///     this object's height (at the default scaling) and no more.
            [[nodiscard]] bool IsFittingCanvasRowHeightToContent() const noexcept
                { return m_itemInfo.m_fitCanvasRowToContent; }
            /// @brief Tells the canvas that this object's parent row should be as tall as
            ///     this object's height (at the default scaling) and no more.
            /// @details By default, this is false and canvases will stretch all of its rows
            ///     (and the items in them) equally height-wise.
            /// @param fit @c true for the object to force the row to fit its content height-wise
            ///     (and no more).
            /// @note If two items in a given row have this set to @c true, then the row will
            ///     be the maximum height of the two items. This will result in the smaller
            ///     item being stretched taller.\n
            ///     Also, the client will need to call Canvas::CalcRowDimensions() after setting all
            ///     objects into its grid for this to take effect.
            void FitCanvasRowHeightToContent(const bool fit) noexcept
                { m_itemInfo.m_fitCanvasRowToContent = fit; }
            /// @returns @c true if this object's width within its row should
            ///     be its calculated width (at the default scaling) and no more.
            [[nodiscard]] bool IsFixedWidthOnCanvas() const noexcept
                { return m_itemInfo.m_fitContentWidthToCanvas; }
            /// @brief Tells the canvas to allocate just the necessary width for this item's width
            ///     (at default scaling) within its row, and nothing more.
            /// @details This is usually used for legends off to the side of a graph.
            /// @details By default, this is false and canvases will stretch all items in a given
            ///     row equally width-wise.
            /// @param fit @c true for the object to consume exactly enough space for its content
            ///     (and no more).
            /// @note Client will need to call Canvas::CalcRowDimensions() after setting all objects
            ///     into its grid for this to take effect.
            void SetFixedWidthOnCanvas(const bool fit) noexcept
                { m_itemInfo.m_fitContentWidthToCanvas = fit; }
            /// @returns The percent of the canvas width that this object should consume.
            [[nodiscard]] double GetCanvasWidthProportion() const noexcept
                { return m_itemInfo.m_canvasWidthProportion; }
            /** @brief Sets the percent of the canvas that this object should consume.
                @param widthProportion The percent of the canvas recommended for this object.*/
            void SetCanvasWidthProportion(const double widthProportion) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_canvasWidthProportion = widthProportion;
                }
            /// @returns The percent of the canvas height that this object should consume.
            [[nodiscard]] std::optional<double> GetCanvasHeightProportion() const noexcept
                { return m_itemInfo.m_canvasHeightProportion; }
            /** @brief Sets the percent of the canvas height that this object should consume.
                @param heightProportion The percent of the canvas recommended for this object.*/
            void SetCanvasHeightProportion(const std::optional<double> heightProportion) noexcept
                {
                InvalidateCachedBoundingBox();
                m_itemInfo.m_canvasHeightProportion = heightProportion;
                }
            /// @}

            /// @returns @c true if the object is valid.
            [[nodiscard]] bool IsOk() const noexcept
                { return m_itemInfo.m_isOk; }
            /** @brief Sets the object to valid or invalid.
                @param isOk Whether the object is valid.
                @note By default, all objects are valid until you call this with @c false.*/
            void SetOk(const bool isOk) noexcept
                { m_itemInfo.m_isOk = isOk; }

            /** @brief Sets the alignment of the object, relative to something else
                    (e.g., its parent).
                @details This is handled by the caller and can have different meanings based
                    on its context; may not be applicable to most objects (or even labels).\n
                    For example, this is used for canvas titles, but not axis labels.

                    Refer to any given object's documentation for how this feature is used.
                @param align How to align the text, relative to whatever the caller is
                    placing it against.*/
            void SetRelativeAlignment(const RelativeAlignment align)
                {
                m_itemInfo.m_relativeAlignment = align;
                InvalidateCachedBoundingBox();
                }
            /// @returns The relative alignment. @sa SetRelativeAlignment().
            [[nodiscard]] const RelativeAlignment& GetRelativeAlignment() const noexcept
                { return m_itemInfo.m_relativeAlignment; }

            // Just hiding these from Doxygen. If these are included inside of groupings,
            // then the "private" tag will break the group in the generated help.
            /// @private
            [[nodiscard]] const wxPen& GetPen() const noexcept
                { return m_itemInfo.m_pen; }
            /// @private
            [[nodiscard]] const wxBrush& GetBrush() const noexcept
                { return m_itemInfo.m_brush; }
            /// @private
            [[nodiscard]] const wxBrush& GetSelectionBrush() const noexcept
                { return m_itemInfo.m_selectionBrush; }
            /// @private
            [[nodiscard]] const GraphItemInfo& GetGraphItemInfo() const noexcept
                { return m_itemInfo; }
            /// @private
            [[nodiscard]] virtual const wxFont& GetFont() const noexcept
                { return m_itemInfo.m_font; }
            /// @private
            [[nodiscard]] const HeaderInfo& GetHeaderInfo() const noexcept
                { return m_itemInfo.m_headerInfo; }
            /// @private
            [[nodiscard]] const std::vector<Wisteria::Icons::LegendIcon>& GetLegendIcons() const noexcept
                { return m_itemInfo.m_legendIcons; }
            /// @private
            [[nodiscard]] const std::optional<wxRect>& GetClippingRect() const noexcept
                { return m_itemInfo.m_clippingRect; }
        protected:
            /** @brief Draws the element.
                @param dc The canvas to draw the element on.
                @returns The bounding box that the element was drawn with.
                    If the element is not being displayed, then an invalid @c wxRect
                    will usually be returned.*/
            virtual wxRect Draw(wxDC& dc) const = 0;
            /** @brief Draws the element's label (if it has one) in the middle of the
                    element if it is selected.
                @param dc The canvas to draw the element on.
                @param scaling The scaling to draw the text with.
                    This may be different from the scaling used by the element itself,
                    depending on what the scaling is of the caller.
                @param boundingBox An optional bounding box to attempt to constrain
                    the selection label to.*/
            virtual void DrawSelectionLabel(wxDC& dc, const double scaling,
                                            const wxRect boundingBox = wxRect()) const;
            /** @brief Recompute coordinates and sizes within this object.
                @param dc The DC used for measuring.
                @details This is usually done after a scaling update.
                @note Derived classes need to override this to perform subitem sizing logic.*/
            virtual void RecalcSizes([[maybe_unused]] wxDC& dc) {}
            /** @brief Reselect subitems if the object's subitems need to be re-created.
                @details This is called by the framework and does not need to be called
                    in client code. Just define it in derived classes, if applicable.
                @note Derived classes should override this if they contain subitems.*/
            virtual void UpdateSelectedItems() {}
            /// @brief Clears all selected items.
            /// @note Derived classes need to override this to unselect all subitems.
            virtual void ClearSelections()
                { SetSelected(false); }
            /** @returns @c true if the given point is inside of this element.
                @param pt The point to check.
                @param dc The DC used for measuring. Not all objects use this parameter.*/
            [[nodiscard]] virtual bool HitTest(const wxPoint pt, wxDC& dc) const = 0;
            /// @brief Apply screen DPI and parent canvas scaling to a value.
            /// @param value The value (e.g., pen width) to scale.
            /// @returns The scaled value.
            /// @warning This should be used to rescale pixel values used for line
            ///     widths and point sizes. It should NOT be used with font point sizes
            ///     because DPI scaling is handled by the OS for those.
            ///     Instead, font sizes should only be scaled to the canvas's scaling.
            [[nodiscard]] double ScaleToScreenAndCanvas(const double value) const noexcept
                { return value * GetScaling() * GetDPIScaleFactor(); }
            /// @brief Apply screen DPI and parent canvas scaling to a value.
            /// @param sz The size to be scaled.
            /// @returns The scaled size.
            [[nodiscard]] wxSize ScaleToScreenAndCanvas(const wxSize sz) const noexcept
                { return sz * GetScaling() * GetDPIScaleFactor(); }
            /// @brief Takes a DIP value that has been scaled from the screen DPI
            ///     and parent canvas scaling and converts it back to its base DIP.
            /// @param value The value (e.g., pen width) to scale.
            /// @returns The downscaled value.
            [[nodiscard]] double DownscaleFromScreenAndCanvas(const double value) const noexcept
                { return safe_divide(value, (GetScaling() * GetDPIScaleFactor())); }
            /** @brief Resets the cached bounding box to empty.
                @note Derived classes should call this in their setter functions
                    that may affect the bounding box as well.\n
                    This also resets the cached content bounding box
                    (only some objects like Label use this).*/
            void InvalidateCachedBoundingBox()
                { m_cachedBoundingBox = m_cachedContentBoundingBox = wxRect(); }
            /** @brief Saves the bounding box information, which can be later retrieved from
                 GetCachedBoundingBox() and thus avoid expensive recalculations in GetBoundingBox().
                @details Derived classes are responsible
                 for calling this in their implementation of GetBoundingBox() and are also
                 responsible for calling InvalidateCachedBoundingBox() in any setter function which
                 may affect bounding box calculations.
                @param cached The bounding box to use.
                @note The cached bounding box is mutable, so this function can be called within
                    const functions (e.g. GetBoundingBox()).*/
            void SetCachedBoundingBox(const wxRect cached) const noexcept
                { m_cachedBoundingBox = cached; }
            /** @returns The bounding box calculated from the last call to GetBoundingBox()
                    (which derived classes should implement ).
                @note Before using this, verify that it is not empty
                 (InvalidateCachedBoundingBox() will set it to empty).\n
                 Also, derived classes are responsible for calling InvalidateCachedBoundingBox() in any
                 setting/moving function that may affect the bounding box calculations of the object.*/
            [[nodiscard]] wxRect GetCachedBoundingBox() const noexcept
                { return m_cachedBoundingBox; }
            /// @brief Caches the content bounding box, which may be different
            ///     from the overall bounding box. This is only used by some objects, such as Label.
            /// @param cached The content box to cache.
            void SetCachedContentBoundingBox(const wxRect cached) const noexcept
                { m_cachedContentBoundingBox = cached; }
            /// @returns The cached content bounding box.
            [[nodiscard]] wxRect GetCachedContentBoundingBox() const noexcept
                { return m_cachedContentBoundingBox; }
            /// @returns @c true if element is currently being dragged.
            [[nodiscard]] bool IsInDragState() const noexcept
                { return m_inDragState; }
            /** @brief Sets whether the element is in a drag state.
                @param isBeingDragged Whether the element is being dragged.*/
            void SetInDragState(const bool isBeingDragged) noexcept
                { m_inDragState = isBeingDragged; }
            /** @brief Override this for selecting subitems at a given point.
                    This implementation will select the entire object if @c pt is inside of the object.
                @param pt The point to hit test.
                @param dc The DC used for measuring. Not all objects use this parameter.
                @returns @c true if something was selected at the given point.
                @note This will toggle the selection of an object. If it was selected before,
                    then it will become unselected.*/
            virtual bool SelectObjectAtPoint(const wxPoint& pt, wxDC& dc)
                {
                if (!IsSelectable())
                    { return false; }
                if (HitTest(pt, dc))
                    {
                    SetSelected(!IsSelected());
                    return true;
                    }
                return false;
                }
            /// @brief Returns the rectangle (relative to the canvas) of the object's main content.
            /// @details This is object specific and is used by the canvas
            ///     when aligning objects across a row or down a column.
            ///     For example, this can be used to align the axes of multiple plots.
            /// @returns The content area.
            [[nodiscard]] virtual wxRect GetContentRect() const noexcept
                { return wxRect(); }
            /// @returns The object's content area top point (relative to the parent canvas).
            [[nodiscard]] std::optional<wxCoord> GetContentTop() const noexcept
                { return m_contentTop; }
            /** @brief Sets the object's content area top point (relative to the parent canvas).
                @details This is object specific and is used by the canvas
                    when aligning objects across a row. For example, this can be used to
                    align the axes of multiple plots.
                @param pt The top point to constrain the content into.*/
            void SetContentTop(const std::optional<wxCoord>& pt) noexcept
                { m_contentTop = pt; }
            /// @returns The object's content area bottom point (relative to the parent canvas).
            [[nodiscard]] std::optional<wxCoord> GetContentBottom() const noexcept
                { return m_contentBottom; }
            /** @brief Sets the object's content area bottom point (relative to the parent canvas).
                @details This is object specific and is used by the canvas
                    when aligning objects across a row. For example, this can be used to
                    align the axes of multiple plots.
                @param pt The top point to constrain the content into.*/
            void SetContentBottom(const std::optional<wxCoord>& pt) noexcept
                { m_contentBottom = pt; }
            /// @returns The object's content area left point (relative to the parent canvas).
            [[nodiscard]] std::optional<wxCoord> GetContentLeft() const noexcept
                { return m_contentLeft; }
            /** @brief Sets the object's content area left point (relative to the parent canvas).
                @details This is object specific and is used by the canvas
                    when aligning objects down a column. For example, this can be used to
                    align the axes of multiple plots.
                @param pt The top point to constrain the content into.*/
            void SetContentLeft(const std::optional<wxCoord>& pt) noexcept
                { m_contentLeft = pt; }
            /// @returns The object's content area right point (relative to the parent canvas).
            [[nodiscard]] std::optional<wxCoord> GetContentRight() const noexcept
                { return m_contentRight; }
            /** @brief Sets the object's content area right point (relative to the parent canvas).
                @details This is object specific and is used by the canvas
                    when aligning objects down a column. For example, this can be used to
                    align the axes of multiple plots.
                @param pt The top point to constrain the content into.*/
            void SetContentRight(const std::optional<wxCoord>& pt) noexcept
                { m_contentRight = pt; }
        private:
            /** @brief Sets the original scaling of the element when it was first
                    embedded onto a canvas.
                @details This is only used by a canvas for when its dimensions change and it
                    needs to recalculate how much space this item needs.
                @param scaling The object's initial scaling factor.*/
             void SetOriginalCanvasScaling(const double scaling)
                {
                wxASSERT_LEVEL_2_MSG(scaling > 0,
                                     L"Scaling in canvas object is less than or equal to zero?!");
                if (scaling <= 0)
                    { return; }
                m_itemInfo.m_originalCanvasScaling = scaling;
                }
            /** @returns The original canvas scaling of the element.*/
            [[nodiscard]] double GetOriginalCanvasScaling() const noexcept
                { return m_itemInfo.m_originalCanvasScaling; }

            GraphItemInfo m_itemInfo;

            // These are used internally for common alignment with other
            // objects on a canvas
            std::optional<wxCoord> m_contentTop{ std::nullopt };
            std::optional<wxCoord> m_contentBottom{ std::nullopt };
            std::optional<wxCoord> m_contentLeft{ std::nullopt };
            std::optional<wxCoord> m_contentRight{ std::nullopt };

            // state info
            bool m_selected{ false };
            bool m_inDragState{ false };
            mutable wxRect m_cachedBoundingBox;
            mutable wxRect m_cachedContentBoundingBox;
            };
        }
    }

/** @}*/

#endif //__WISTERIA_CANVAS_ITEMS_H__
