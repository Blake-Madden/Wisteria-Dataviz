/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_RACETRACK_CHART_H
#define WISTERIA_RACETRACK_CHART_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /// @brief A race track chart (circular chart with a hollow center).
    /// @details Each row of data is a concentric track lane, starting from the outer
    ///     edge and going inward. All track lanes start at the same angular position
    ///     and extend clockwise proportionally to their value. The longest track lane
    ///     goes almost all the way around (360 degrees minus gap).
    ///     Labels are placed at the outer edge at the start position.
    /// @par Citation:
    ///     The spiraling layout follows W. E. B. Du Bois's <i>City and Rural Population. 1890</i>,
    ///     one of the hand-drawn plates that he and his students at Atlanta University prepared
    ///     for the 1900 Paris Exposition.\n
    ///     \n
    ///     Battle-Baptiste, W., &amp; Rusert, B. (Eds.). (2018). <i>W. E. B. Du Bois's data
    ///     portraits: Visualizing Black America.</i> Princeton Architectural Press.
    class RaceTrackChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(RaceTrackChart);
        RaceTrackChart() = default;

      public:
        /// @brief How many concentric tracks the track lanes are spread across.
        enum class TrackCount
            {
            Auto, /*!< Use two tracks if the smallest track lane would be too narrow
                       to read on a single one, otherwise use one.*/
            One,  /*!< Always use a single track, so that no track lane runs past one
                       revolution. Small values may end up too narrow to read.*/
            Two   /*!< Always use two tracks, giving every track lane twice the sweep
                       to work with. Track lanes long enough to run past one revolution
                       spiral inward to finish on the inner track.*/
            };

        /// @brief Information about a single track lane in the race track chart.
        class TrackLaneInfo
            {
            friend class RaceTrackChart;

          public:
            /// @brief Constructor.
            /// @param label The label for this track lane.
            /// @param value The value (determines angular extent).
            /// @param color The color to draw this track lane with.
            explicit TrackLaneInfo(wxString label, const double value, const wxColour& color)
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
            @param brushes The brush scheme, which will contain the color and brush patterns
                to render the track lanes with.
            @param colors The color scheme to apply to the track lanes underneath the
                track lanes' brush patterns.\n
                This is useful if using a hatched brush, as this color will be solid
                and show underneath it. Leave as @c nullptr just to use the brush scheme.*/
        explicit RaceTrackChart(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the chart from a dataset.
            @param data The data to use, which should contain a categorical column
                (labels) and a continuous column (values).
            @param valueColumnName The continuous column containing track lane values.
            @param labelColumnName The categorical column containing track lane labels.
            @note Each track lane is colored from the color/brush scheme according to its
                position in the dataset.\n
                Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& valueColumnName, const wxString& labelColumnName);

        /// @returns The name of the continuous column that the track lane values came from.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the categorical column that the track lane labels came from.
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns How the track lanes' values are formatted when displayed as labels.
        [[nodiscard]]
        NumberDisplay GetValueFormat() const noexcept
            {
            return m_valueFormat;
            }

        /// @brief Sets how the track lanes' values are formatted when displayed as labels.
        /// @param format The format to display values with (default is
        ///     @c NumberDisplay::Value).
        void SetValueFormat(const NumberDisplay format) noexcept { m_valueFormat = format; }

        /// @returns How many concentric tracks the track lanes are spread across.
        [[nodiscard]]
        TrackCount GetTrackCount() const noexcept
            {
            return m_trackCount;
            }

        /// @brief Sets how many concentric tracks to spread the track lanes across.
        /// @param trackCount The number of tracks, or @c TrackCount::Auto to decide
        ///     based on how narrow the smallest track lane would be.
        /// @note This is a visual preference, not a data limit. Track lanes are always
        ///     scaled proportionally. This only controls how much sweep they have to
        ///     work with.
        void SetTrackCount(const TrackCount trackCount) noexcept { m_trackCount = trackCount; }

        /// @returns The proportion of the plot area used for the track (vs. the donut hole).
        [[nodiscard]]
        double GetTrackProportion() const noexcept
            {
            return m_trackProportion;
            }

        /// @brief Sets the proportion of the plot area used for the track.
        /// @param proportion Value between 0.3 and 0.9 (default is 0.65).
        void SetTrackProportion(const double proportion) noexcept
            {
            m_trackProportion = std::clamp(proportion, 0.3, 0.9);
            }

        /// @returns @c true if the chart is showing labels.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @brief Whether to show labels at the start position of the track lanes.
        /// @param show @c true to show labels.
        void ShowLabels(const bool show) noexcept { m_showLabels = show; }

        /// @returns The angle (in degrees) where track lanes start
        ///     (default is 270.0 [12 o'clock]).
        [[nodiscard]]
        double GetStartAngle() const noexcept
            {
            return m_startAngle;
            }

        /// @brief Sets the angle where track lanes start.
        /// @param angleDeg The start angle in degrees (0 = 3 o'clock, 90 = 6 o'clock, etc.).
        void SetStartAngle(const double angleDeg) noexcept { m_startAngle = angleDeg; }

        /// @deprecated
        [[deprecated("Race track charts do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

      private:
        /** @brief Adds a track lane to the chart.
          @param lane The track lane information to add.
          @note Call the parent canvas's `CalcAllSizes()` after adding all track lanes
              to re-plot the data.*/
        void AddTrackLane(const TrackLaneInfo& lane) { m_trackLanes.push_back(lane); }

        void SetAutoAccessibilityAttributes() final;
        void RecalcSizes(wxDC& dc) final;

        /** @brief A single track lane, drawn as a curved band of constant thickness.
            @details The track lane leaves @c m_startRadius and loses @c m_radiusPerDegree
                of radius for every degree it travels, so it winds inward at a constant
                rate. A @c m_radiusPerDegree of zero draws a plain circular track instead.*/
        class TrackLaneSegment final : public GraphItems::GraphItemBase
            {
          public:
            /** @brief Constructor.
                @param center The center of the track.
                @param startAngle The angle (in degrees) that the track lane starts at.
                @param sweepAngle How far (in degrees) the track lane runs (may exceed 360).
                @param startRadius The radius that the track lane starts at.
                @param radiusPerDegree How much radius the track lane loses per degree
                    traveled. Pass zero for a track lane that holds a constant radius.
                @param thickness The width of the track.
                @param color The color to draw the track lane with.*/
            TrackLaneSegment(const wxPoint& center, double startAngle, double sweepAngle,
                             double startRadius, double radiusPerDegree, double thickness,
                             const wxColour& color);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            wxRect Draw(wxDC& dc) const final;

            [[nodiscard]]
            bool HitTest(wxPoint pt, wxDC& dc) const final;

            void Offset(int xOffset, int yOffset) final;

            void SetBoundingBox(const wxRect& rect, wxDC& dc, double scaling) final;

            wxPoint m_center;
            double m_startAngle{ 0 };
            double m_sweepAngle{ 0 };
            double m_startRadius{ 0 };
            double m_radiusPerDegree{ 0 };
            double m_thickness{ 0 };
            };

        /// @returns The number of laps to lay the track lanes out across, resolving
        ///     @c TrackCount::Auto against the data.
        [[nodiscard]]
        int ComputeLapCount() const;

        /// @brief Computes the angular extent for each track lane.
        /// @param laps The number of laps (1 or 2) to scale angles for.
        [[nodiscard]]
        std::vector<double> ComputeLaneAngles(int laps) const;

        /// @returns A track lane's value, formatted according to @c GetValueFormat().
        /// @param value The value to format.
        [[nodiscard]]
        wxString FormatValue(double value) const;

        std::vector<TrackLaneInfo> m_trackLanes;

        // the angular gap left at the start angle so a track lane's sweep doesn't wrap
        // all the way around and touch its own starting point
        constexpr static double m_startSeamGapAngle{ 2.0 };
        // the angular gap left where a spiraling track lane crosses back through the
        // start angle, transitioning from one lap into the next
        constexpr static double m_lapSeamGapAngle{ 3.0 };
        // the minimum angle the smallest track lane must occupy on a single track for
        // TrackCount::Auto to consider that track count readable
        constexpr static double m_minReadableAngle{ 30.0 };

        // layout configuration
        TrackCount m_trackCount{ TrackCount::Auto };
        double m_trackProportion{ 0.65 };
        double m_startAngle{ 270.0 }; // 12 o'clock
        bool m_showLabels{ true };
        NumberDisplay m_valueFormat{ NumberDisplay::Value };

        // column names (for dataset mode)
        wxString m_valueColumnName;
        wxString m_labelColumnName;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_RACETRACK_CHART_H
