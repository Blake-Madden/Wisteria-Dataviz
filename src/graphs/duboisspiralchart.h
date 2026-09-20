/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_DUBOIS_SPIRAL_CHART_H
#define WISTERIA_DUBOIS_SPIRAL_CHART_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /// @brief A Du Bois spiral chart (zigzag path ending in a spiral).
    /// @details Each row of data is one segment of a single continuous path,
    ///     drawn in dataset order with lengths proportional to values.
    ///     The first row draws horizontally across the top, middle rows zigzag
    ///     diagonally downward, and the last row winds inward as a spiral.
    ///     This suits data where the final value dwarfs the others, as the spiral
    ///     can absorb a very long length in a compact area.
    /// @par Citation:
    ///     The layout follows W. E. B. Du Bois's <i>City and Rural Population. 1890</i>,
    ///     one of the hand-drawn plates that he and his students at Atlanta University
    ///     prepared for the 1900 Paris Exposition.\n
    ///     \n
    ///     Battle-Baptiste, W., &amp; Rusert, B. (Eds.). (2018). <i>W. E. B. Du Bois's data
    ///     portraits: Visualizing Black America.</i> Princeton Architectural Press.
    class DuBoisSpiralChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(DuBoisSpiralChart);
        DuBoisSpiralChart() = default;

      public:
        /// @brief The default zigzag angle (in degrees below horizontal).
        constexpr static double DEFAULT_ZIGZAG_ANGLE{ 40.0 };
        /// @brief The default outer radius of the spiral (as a proportion of the plot size).
        constexpr static double DEFAULT_OUTER_RADIUS_PROPORTION{ 0.36 };
        /// @brief The default line thickness (as a proportion of the plot size).
        constexpr static double DEFAULT_LINE_THICKNESS_PROPORTION{ 0.025 };

        /// @brief Information about a single segment of the spiral path.
        class SpiralNodeInfo
            {
          public:
            /// @brief Constructor.
            /// @param label The label for this segment.
            /// @param value The value (determines segment length).
            /// @param color The color to draw this segment with.
            explicit SpiralNodeInfo(wxString label, const double value, const wxColour& color)
                : m_label(std::move(label)), m_value(value), m_color(color)
                {
                }

            /// @returns The label text.
            [[nodiscard]]
            const wxString& GetLabel() const noexcept
                {
                return m_label;
                }

            /// @brief Set the label text.
            /// @param label The new label.
            void SetLabel(const wxString& label) { m_label = label; }

            /// @returns The value.
            [[nodiscard]]
            double GetValue() const noexcept
                {
                return m_value;
                }

            /// @brief Set the value.
            /// @param value The new value.
            void SetValue(const double value) noexcept { m_value = value; }

            /// @returns The color.
            [[nodiscard]]
            const wxColour& GetColor() const noexcept
                {
                return m_color;
                }

            /// @brief Set the color.
            /// @param color The new color.
            void SetColor(const wxColour& color) { m_color = color; }

          private:
            wxString m_label;
            double m_value{ 0 };
            wxColour m_color;
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param brushes The brush scheme to take the segment colors from.\n
                Segments are drawn as solid lines, so only each brush's color is used.
                Brush patterns (such as hatching) are ignored.
            @param colors The color scheme to take the segment colors from.\n
                If provided, it is used instead of the brush scheme's colors.
                Leave as @c nullptr just to use the brush scheme.*/
        explicit DuBoisSpiralChart(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the chart from a dataset.
            @param data The data to use, which should contain a categorical column
                (labels) and a continuous column (values).
            @param valueColumnName The continuous column containing segment values.
            @param labelColumnName The categorical column containing segment labels.
            @note Segments are drawn in dataset order: the first row draws horizontally,
                middle rows zigzag diagonally, and the last row forms the spiral.\n
                Rows with a missing, non-finite, zero, or negative value are skipped.\n
                Each plotted segment is colored from the color/brush scheme according to its
                position among the plotted rows.\n
                Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                In that case, the chart's previous data is left in place.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& valueColumnName, const wxString& labelColumnName);

        /// @returns The name of the continuous column that the segment values came from.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the categorical column that the segment labels came from.
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns The segments in drawing order.
        [[nodiscard]]
        const std::vector<SpiralNodeInfo>& GetNodes() const noexcept
            {
            return m_nodes;
            }

        /// @returns How the segment values are formatted when displayed as labels.
        [[nodiscard]]
        NumberDisplay GetValueFormat() const noexcept
            {
            return m_valueFormat;
            }

        /// @brief Sets how the segment values are formatted when displayed as labels.
        /// @param format The format to display values with (default is @c NumberDisplay::Value).
        void SetValueFormat(const NumberDisplay format) noexcept { m_valueFormat = format; }

        /// @returns @c true if the chart is showing labels.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @brief Whether to show labels alongside the segments.
        /// @param show @c true to show labels.
        void ShowLabels(const bool show) noexcept { m_showLabels = show; }

        /// @returns The zigzag angle in degrees below horizontal (default is 40).
        [[nodiscard]]
        double GetZigZagAngle() const noexcept
            {
            return m_zigZagAngleDeg;
            }

        /// @brief Sets the zigzag angle for the middle diagonal segments.
        /// @param angleDeg The angle in degrees below horizontal, clamped to 10-80.
        ///     A non-finite value is ignored.
        void SetZigZagAngle(const double angleDeg) noexcept
            {
            if (std::isfinite(angleDeg))
                {
                m_zigZagAngleDeg = std::clamp(angleDeg, 10.0, 80.0);
                }
            }

        /// @returns The outer radius of the spiral as a proportion of the smaller
        ///     plot dimension (default is 0.36).
        [[nodiscard]]
        double GetOuterRadiusProportion() const noexcept
            {
            return m_outerRadiusProportion;
            }

        /// @brief Sets the outer radius of the spiral.
        /// @param proportion Proportion of the smaller plot dimension, clamped to 0.15-0.48.
        ///     A non-finite value is ignored.
        void SetOuterRadiusProportion(const double proportion) noexcept
            {
            if (std::isfinite(proportion))
                {
                m_outerRadiusProportion = std::clamp(proportion, 0.15, 0.48);
                }
            }

        /// @returns The line thickness as a proportion of the smaller plot dimension
        ///     (default is 0.025).
        [[nodiscard]]
        double GetLineThicknessProportion() const noexcept
            {
            return m_lineThicknessProportion;
            }

        /// @brief Sets the line thickness.
        /// @param proportion Proportion of the smaller plot dimension, clamped to 0.005-0.08.
        ///     A non-finite value is ignored.
        void SetLineThicknessProportion(const double proportion) noexcept
            {
            if (std::isfinite(proportion))
                {
                m_lineThicknessProportion = std::clamp(proportion, 0.005, 0.08);
                }
            }

      private:
        /// @deprecated
        [[deprecated("Du Bois spiral charts do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

        void SetAutoAccessibilityAttributes() final;
        void RecalcSizes(wxDC& dc) final;

        /** @brief Base class for the segments that make up the path.
            @details Handles the color, line width, hit testing, and graphics context
                setup shared by the spiral and straight segments. Each derived class
                only strokes its own shape.*/
        class PathSegment : public GraphItems::GraphItemBase
            {
          protected:
            /** @brief Constructor.
                @param thickness The width of the line.
                @param color The color to draw the segment with.*/
            PathSegment(double thickness, const wxColour& color);

            /// @returns The width of the line.
            [[nodiscard]]
            double GetThickness() const noexcept
                {
                return m_thickness;
                }

            /// @brief Strokes this segment's shape.
            /// @param gc The graphics context to draw with.
            virtual void StrokeShape(wxGraphicsContext& gc) const = 0;

            /// @returns @c true if a point is on this segment's stroked shape.
            /// @param pt The point to test.
            [[nodiscard]]
            virtual bool IsOnShape(wxPoint pt) const = 0;

            /// @returns How far (in pixels) from the center of the stroke a point can be
            ///     and still count as touching it.
            [[nodiscard]]
            double GetHitReach() const noexcept
                {
                constexpr double MIN_HIT_REACH{ 3.0 };
                return std::max(m_thickness * 0.5, MIN_HIT_REACH);
                }

          private:
            wxRect Draw(wxDC& dc) const final;

            [[nodiscard]]
            bool HitTest(wxPoint pt, wxDC& dc) const final;

            void SetBoundingBox(const wxRect& rect, wxDC& dc, double scaling) final;

            double m_thickness{ 0 };
            };

        /** @brief The final spiral segment, drawn as a thick Archimedean spiral.
            @details Starts at @c m_startRadius and loses @c m_radiusPerDegree
                of radius for every degree traveled, so it winds inward at a constant
                rate. Sampling as a polyline (rather than arcs) keeps sweeps longer
                than a full revolution intact.*/
        class SpiralSegment final : public PathSegment
            {
          public:
            /** @brief Constructor.
                @param center The center of the spiral.
                @param startAngle The angle (in degrees) that the spiral starts at.
                @param sweepAngle How far (in degrees) the spiral runs (may exceed 360).
                @param startRadius The radius that the spiral starts at.
                @param radiusPerDegree How much radius the spiral loses per degree traveled.
                @param thickness The width of the line.
                @param color The color to draw the spiral with.*/
            SpiralSegment(const wxPoint& center, double startAngle, double sweepAngle,
                          double startRadius, double radiusPerDegree, double thickness,
                          const wxColour& color);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            void StrokeShape(wxGraphicsContext& gc) const final;

            [[nodiscard]]
            bool IsOnShape(wxPoint pt) const final;

            void Offset(int xOffset, int yOffset) final;

            wxPoint m_center;
            double m_startAngle{ 0 };
            double m_sweepAngle{ 0 };
            double m_startRadius{ 0 };
            double m_radiusPerDegree{ 0 };
            };

        /** @brief A straight segment drawn with the same unscaled stroke as the
                spiral, so all parts of the path share one line width.
            @details Stroked directly with a graphics pen (rather than
                @c GraphItems::Lines, which rescales its pen), matching @c SpiralSegment.*/
        class StraightSegment final : public PathSegment
            {
          public:
            /** @brief Constructor.
                @param start The starting point.
                @param end The ending point.
                @param thickness The width of the line.
                @param color The color to draw the segment with.*/
            StraightSegment(const wxPoint& start, const wxPoint& end, double thickness,
                            const wxColour& color);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            void StrokeShape(wxGraphicsContext& gc) const final;

            [[nodiscard]]
            bool IsOnShape(wxPoint pt) const final;

            void Offset(int xOffset, int yOffset) final;

            wxPoint m_start;
            wxPoint m_end;
            };

        /// @returns A segment value, formatted according to @c GetValueFormat().
        /// @param value The value to format.
        /// @param thousandsSeparator @c false to leave out thousands separators.
        [[nodiscard]]
        wxString FormatValue(double value, bool thousandsSeparator = true) const;

        /// @returns A segment's value and label as text (e.g., "1,000: Label").
        /// @param node The segment to format.
        /// @param thousandsSeparator @c false to leave out thousands separators.
        [[nodiscard]]
        wxString FormatNodeText(const SpiralNodeInfo& node, bool thousandsSeparator = true) const;

        /// @brief A point on an Archimedean spiral at the given sweep.
        /// @param centerX The x-coordinate of the spiral's center.
        /// @param centerY The y-coordinate of the spiral's center.
        /// @param radius The radius at this sweep.
        /// @param angleDeg The angle (in degrees) at this sweep.
        /// @returns The position on the spiral.
        [[nodiscard]]
        static wxPoint2DDouble SpiralPointAt(double centerX, double centerY, double radius,
                                             double angleDeg);

        std::vector<SpiralNodeInfo> m_nodes;

        // layout configuration
        double m_zigZagAngleDeg{ DEFAULT_ZIGZAG_ANGLE };
        double m_outerRadiusProportion{ DEFAULT_OUTER_RADIUS_PROPORTION };
        double m_lineThicknessProportion{ DEFAULT_LINE_THICKNESS_PROPORTION };
        double m_spiralGapRatio{ 0.6 };
        bool m_showLabels{ true };
        NumberDisplay m_valueFormat{ NumberDisplay::Value };

        // column names (for dataset mode)
        wxString m_valueColumnName;
        wxString m_labelColumnName;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_DUBOIS_SPIRAL_CHART_H
