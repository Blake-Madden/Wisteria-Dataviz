/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_ENUMS_H__
#define __WISTERIA_ENUMS_H__

/// @brief Enumerations.
namespace Wisteria
    {
    /// @brief How values can be compared.
    enum class Comparison
        {
        /// @brief Items are equal.
        Equals,
        /// @brief Items are not equal.
        NotEquals,
        /// @brief First item is less than the other.
        LessThan,
        /// @brief First item is less than or equal to the other.
        LessThanOrEqualTo,
        /// @brief First item is greater than the other.
        GreaterThan,
        /// @brief First item is greater than or equal to the other.
        GreaterThanOrEqualTo,
        };

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
        /// @brief Positive influence (e.g., IVs with coefficients > 0).
        InfluencePositive = (1 << 0),
        /// @brief Negative infuence (e.g., IVs with coefficients < 0).
        InfluenceNegative = (1 << 1),
        /// @brief No influence (e.g., IVs with coefficients = 0).
        InfluenceNeutral = (1 << 2),
        /// @brief All levels of influence.
        InfluenceAll = (InfluencePositive | InfluenceNegative | InfluenceNeutral)
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
        NoDisplay,               /*!< No shadow should be drawn.*/
        NoShadow = NoDisplay,    /*!< No shadow should be drawn.*/
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

    /** @brief When calling `SetBoundingBox()` for a `Label`, this will control how the user-provided
            bounding box is used.
        @internal This enum is a bitmask, do not make it strongly typed.*/
    enum LabelBoundingBoxContentAdjustment
        {
        /// @brief The bounding box is used verbatim. If the measured content of the label
        ///     is smaller than the box, then the text will be aligned with the box
        ///     according to the label's page alignment.
        ContentAdjustNone = 0,
        /// @brief The height of the bounding box is treated as a suggestion and will be
        ///     adjusted to fit the final measured size of the text.
        ContentAdjustHeight = 1,
        /// @brief The width of the bounding box is treated as a suggestion and will be
        ///     adjusted to fit the final measured size of the text.
        ContentAdjustWidth = 2,
        /// @brief readjusts both width and height of the label to its content.
        ContentAdjustAll = (ContentAdjustHeight | ContentAdjustWidth)
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

    /// @brief How a number should be displayed.
    enum class NumberDisplay
        {
        /// @brief Display the number as-is.
        Value,
        /// @brief Show as a percent.
        Percentage
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
        /// @brief For backward compatibility, use @c StippleImage instead.
        Stipple,
        /// @brief Fill with a repeating image.
        StippleImage = Stipple,
        /// @brief Fill with a repeating shape (Icons::IconShape).
        StippleShape,
        /// @brief A subimage of a larger image shared by all boxes.
        CommonImage,
        /// @brief An image scaled down to fit the box.
        Image,
        /// @brief A watercolor-like effect, where the box is warped and looks
        ///     like it was filled in with watercolor paint (or a marker).\n
        ///     Note that if an outline pen is in use, it will be drawn over the
        ///     fill color, giving the look that it showing through the "watercolor".
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

    /// @brief Effects that could be applied to an image.
    enum class ImageEffect
        {
        /// @brief Do no alter the image.
        NoEffect,
        /// @brief Shades of gray (i.e., Black & White).
        Grayscale,
        /// @brief A horizontal blur across the image.
        BlurHorizontal,
        /// @brief A vertical blur across the image.
        BlurVertical,
        /// @brief A sepia (i.e., faded photograph) effect.
        Sepia,
        /// @brief A frosted glass window effect to an image.
        ///     In other words, the image as it may appear when viewed through frosted glass.
        FrostedGlass,
        /// @brief An oil painting effect.
        OilPainting
        };

    /// @brief How the corners of how various boxes are drawn.
    enum class BoxCorners
        {
        Straight, /*!< Straight lines meet at the corner.*/
        Rounded   /*!< Corners are rounded.\n
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

    /// @brief How to display the name of a column of items in a graph.
    enum class GraphColumnHeader
        {
        /// @brief Display the column's name as an axis header.
        AsHeader,
        /// @brief Display the column's name as an axis footer.
        AsFooter,
        /// @brief Do not display the column's name.
        NoDisplay
        };

    /// @brief How to draw a flow (e.g., Sankey diagram).
    enum class FlowShape
        {
        /// @brief Draw a flow with a spline-like shape.
        Curvy,
        /// @brief Draw straight lines between the points in the flow's shape.
        Jagged
        };

    /// @brief How to display a table cell's content.
    enum class TableCellFormat
        {
        /// @brief Displays a number generically.
        General,
        /// @brief Displays a value such as @c 0.25 as @c 25%.
        Percent,
        /// @brief Displays a value such as @c 0.25 as @c 25%,
        ///     but with an up or down arrow next to it if positive
        ///     or negative (respectively).
        PercentChange,
        /// @brief Displays a number generically,
        ///     but with an up or down arrow next to it if positive
        ///     or negative (respectively).
        GeneralChange,
        /// @brief Displays numbers in accounting format.
        /// @details For example, a negative value would appear as `$    (5,000.00)`.
        Accounting
        };

    /// @brief How to aggregate a row or column in a table.
    enum class AggregateType
        {
        /// @brief Sums a series of values.
        Total,
        /// @brief Calculates the change from one value to another (as a percentage).
        ChangePercent,
        /// @brief Calculates the ratio between two values
        ///     (ratios will be rounded to integers if the cell's precision is zero).
        Ratio,
        /// @brief Calculates the change (i.e., difference) from one value to another.
        Change
        };
    }

/** @}*/

#endif //__WISTERIA_ENUMS_H__
