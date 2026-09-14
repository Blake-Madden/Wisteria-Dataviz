/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_BULLET_CHART_H
#define WISTERIA_BULLET_CHART_H

#include "barchart.h"
#include <limits>

namespace Wisteria::Graphs
    {
    /// @brief How the actual/target values are cosmetically displayed in a bullet chart's
    ///     leader-line callouts.
    enum class BulletChartValueFormat
        {
        Value,     /*!< Show the raw value (e.g., "Actual: 82").*/
        Percentage /*!< Append a "%" suffix (e.g., "Actual: 82%").\n
                        This is purely cosmetic; the underlying value is not transformed.*/
        };

    /// @brief How the qualitative performance bands (set via BulletChart::SetRanges())
    ///     are colored.
    enum class BulletChartRangeColorScheme
        {
        TwoTone,   /*!< Every row's bands blend between BulletChart::SetRangeStartColor()
                        and BulletChart::SetRangeEndColor(), from worst to best.*/
        GoalStatus /*!< Each row's bands are shaded (worst) through tinted (best)
                        variants of a single color: BulletChart::SetGoalSuccessColor() if
                        that row's actual value met its target, or
                        BulletChart::SetGoalFailureColor() otherwise.
                        A row reads as one solid hue (all green, or all red) rather than
                        every row sharing the same gradient. A row with no target to
                        compare against falls back to the TwoTone gradient.\n
                        This is the default.*/
        };

    // clang-format off
    /** @brief A chart for comparing a list of KPIs (each an actual value against a target)
            within the context of qualitative performance ranges.
        @details Each row is a KPI: a thin bar shows the actual value, a bold tick shows
            the target, and background bands (set via SetRanges()) show qualitative context
            (e.g., poor/satisfactory/good). All rows share one scaling axis, so the chart
            reads as a compact, scannable list, a denser alternative to a row of KPI cards.
        @image html bullet_chart.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one column is a label (categorical or ID)
         for each KPI, and two continuous columns hold the actual and target values.

         | Label                   | Actual | Target |
         | :--                     | --:    | --:    |
         | Employee Satisfaction   | 82     | 85     |
         | Community Satisfaction  | 74     | 80     |
         | Customer Satisfaction   | 91     | 90     |

        @par Missing Data:
         - A row with a missing actual value will not have its bar drawn (the target
           tick and callout for it are unaffected).
         - A row with a missing target value will not have its tick drawn.
         - A row with both values missing is skipped entirely.

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         auto kpiData = std::make_shared<Data::Dataset>();
         kpiData->AddContinuousColumn(L"Actual");
         kpiData->AddContinuousColumn(L"Target");
         kpiData->AddRow(Data::RowInfo().Id(L"Employee Satisfaction").Continuous({ 82, 85 }));
         kpiData->AddRow(Data::RowInfo().Id(L"Community Satisfaction").Continuous({ 74, 80 }));
         kpiData->AddRow(Data::RowInfo().Id(L"Customer Satisfaction").Continuous({ 91, 90 }));
         kpiData->GetIdColumn().SetName(L"Label");

         auto plot = std::make_shared<BulletChart>(canvas);
         plot->SetData(kpiData, L"Label", L"Actual", L"Target");
         plot->SetRanges({
             { 50,  _(L"Poor") },
             { 80,  _(L"Satisfactory") },
             { 100, _(L"Good") } });
         plot->SetValueDisplayFormat(BulletChartValueFormat::Percentage);

         canvas->SetFixedObject(0, 0, plot);
        @endcode*/
    // clang-format on

    class BulletChart final : public Wisteria::Graphs::BarChart
        {
        wxDECLARE_DYNAMIC_CLASS(BulletChart);
        BulletChart() = default;

      public:
        /// @brief One qualitative performance band (e.g., "Poor", "Satisfactory", "Good").
        struct Range
            {
            /// @brief The cumulative upper bound of this band.
            /// @details The band runs from the previous range's end (or the scaling axis
            ///     start) up to this value.
            double m_end{ 0 };
            /// @brief The label shown for this band (in the shared axis bracket).
            wxString m_label;
            };

        /** @brief Constructor.
            @param canvas The parent canvas to render on.*/
        explicit BulletChart(Wisteria::Canvas* canvas);

        /** @brief Sets the data.
            @param data The data to use.
            @param labelColumnName The column containing the KPIs' labels
                (a categorical or ID column). One row is expected per KPI.
            @param actualColumnName The column containing the KPIs' actual values
                (a continuous column).
            @param targetColumnName The column containing the KPIs' target values
                (a continuous column).
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& labelColumnName, const wxString& actualColumnName,
                     const wxString& targetColumnName);

        /** @brief Sets the qualitative performance bands, shared across every KPI row.
            @param ranges The bands, in ascending order of their cumulative end value
                (e.g., `{ {50, "Poor"}, {80, "Satisfactory"}, {100, "Good"} }`).
            @note A bullet-chart list only reads as a scannable dashboard if every KPI sits
                on one shared qualitative scale. That's also what makes the shared axis
                (and its brackets) meaningful. If the ranges are never set, each row still
                gets a single neutral-colored bar, just with no qualitative bands or
                brackets drawn.
            @sa SetRangeColorScheme() for how the bands are colored.*/
        void SetRanges(std::vector<Range> ranges);

        /// @returns The qualitative performance bands.
        /// @sa SetRanges().
        [[nodiscard]]
        const std::vector<Range>& GetRanges() const noexcept
            {
            return m_ranges;
            }

        /// @returns How the qualitative performance bands are colored.
        [[nodiscard]]
        BulletChartRangeColorScheme GetRangeColorScheme() const noexcept
            {
            return m_rangeColorScheme;
            }

        /** @brief Sets how the qualitative performance bands are colored.
            @param scheme The color scheme to use.
            @details Regardless of scheme, the first band is filled with the scheme's
                "worst" color and the last with its "best" color. Any bands in between
                are filled with a blend between the two, proportional to their position.
                Defaults to BulletChartRangeColorScheme::GoalStatus.
            @sa SetRangeStartColor(), SetRangeEndColor(), SetGoalSuccessColor(),
                SetGoalFailureColor().*/
        void SetRangeColorScheme(const BulletChartRangeColorScheme scheme) noexcept
            {
            m_rangeColorScheme = scheme;
            }

        /// @returns The band color used for the worst band under
        ///     BulletChartRangeColorScheme::TwoTone.
        [[nodiscard]]
        const wxColour& GetRangeStartColor() const noexcept
            {
            return m_rangeStartColor;
            }

        /// @brief Sets the band color used for the worst band under
        ///     BulletChartRangeColorScheme::TwoTone.
        /// @param color The color to use.
        void SetRangeStartColor(const wxColour& color) noexcept { m_rangeStartColor = color; }

        /// @returns The band color used for the best band under
        ///     BulletChartRangeColorScheme::TwoTone.
        [[nodiscard]]
        const wxColour& GetRangeEndColor() const noexcept
            {
            return m_rangeEndColor;
            }

        /// @brief Sets the band color used for the best band under
        ///     BulletChartRangeColorScheme::TwoTone.
        /// @param color The color to use.
        void SetRangeEndColor(const wxColour& color) noexcept { m_rangeEndColor = color; }

        /// @returns The base color used for a row's bands under
        ///     BulletChartRangeColorScheme::GoalStatus, when that row's actual value
        ///     met its target.
        [[nodiscard]]
        const wxColour& GetGoalSuccessColor() const noexcept
            {
            return m_goalSuccessColor;
            }

        /// @brief Sets the base color used for a row's bands under
        ///     BulletChartRangeColorScheme::GoalStatus, when that row's actual value
        ///     met its target.
        /// @param color The color to use.
        void SetGoalSuccessColor(const wxColour& color) noexcept { m_goalSuccessColor = color; }

        /// @returns The base color used for a row's bands under
        ///     BulletChartRangeColorScheme::GoalStatus, when that row's actual value
        ///     did not meet its target.
        [[nodiscard]]
        const wxColour& GetGoalFailureColor() const noexcept
            {
            return m_goalFailureColor;
            }

        /// @brief Sets the base color used for a row's bands under
        ///     BulletChartRangeColorScheme::GoalStatus, when that row's actual value
        ///     did not meet its target.
        /// @param color The color to use.
        void SetGoalFailureColor(const wxColour& color) noexcept { m_goalFailureColor = color; }

        /// @returns The proportion (0 < x <= 1) of a row's slot that the range (background)
        ///     bar consumes.
        [[nodiscard]]
        double GetRangeBarWidthProportion() const noexcept
            {
            return m_rangeBarWidthProportion;
            }

        /// @brief Sets the proportion of a row's slot that the range (background) bar
        ///     consumes.
        /// @param proportion A value between @c 0 (exclusive) and @c 1 (inclusive).
        void SetRangeBarWidthProportion(const double proportion) noexcept
            {
            if (proportion > 0 && proportion <= 1)
                {
                m_rangeBarWidthProportion = proportion;
                }
            }

        /// @returns The proportion (0 < x <= 1) of a row's slot that the actual-value bar
        ///     consumes.
        [[nodiscard]]
        double GetActualBarWidthProportion() const noexcept
            {
            return m_actualBarWidthProportion;
            }

        /// @brief Sets the proportion of a row's slot that the actual-value bar consumes.
        /// @param proportion A value between @c 0 (exclusive) and @c 1 (inclusive).
        ///     Should be smaller than GetRangeBarWidthProportion() so that the actual-value
        ///     bar reads as being layered on top of the range bar.
        void SetActualBarWidthProportion(const double proportion) noexcept
            {
            if (proportion > 0 && proportion <= 1)
                {
                m_actualBarWidthProportion = proportion;
                }
            }

        /// @returns The brush used for the actual-value bars.
        [[nodiscard]]
        wxBrush& GetActualBarBrush() noexcept
            {
            return m_actualBarBrush;
            }

        /// @returns The pen used to outline the actual-value bars.
        [[nodiscard]]
        wxPen& GetActualBarPen() noexcept
            {
            return m_actualBarPen;
            }

        /** @brief Sets the color of the actual-value bars.
            @param color The color to fill the bars with.\n
                The outline pen is derived from this (a shaded or tinted variant),
                so the bar reads as one cohesive color rather than needing the
                brush and pen set separately.
            @sa GetActualBarBrush(), GetActualBarPen() for finer-grained control.*/
        void SetActualBarColor(const wxColour& color);

        /// @returns The pen used for the target tick marks.
        [[nodiscard]]
        wxPen& GetTargetTickPen() noexcept
            {
            return m_targetTickPen;
            }

        /** @brief Sets the color of the target tick marks.
            @param color The color to draw the tick marks with.
            @sa GetTargetTickPen() for finer-grained control (e.g., width, style).*/
        void SetTargetTickColor(const wxColour& color);

        /// @returns The color used for the "Actual" word in its value callout
        ///     (the text before the colon).
        [[nodiscard]]
        const wxColour& GetActualCalloutLabelColor() const noexcept
            {
            return m_actualCalloutLabelColor;
            }

        /// @brief Sets the color used for the "Actual" word in its value callout
        ///     (the text before the colon).
        /// @param color The color to draw that word with.
        void SetActualCalloutLabelColor(const wxColour& color) noexcept
            {
            m_actualCalloutLabelColor = color;
            }

        /// @returns The color used for the "Target" word in its value callout
        ///     (the text before the colon).
        [[nodiscard]]
        const wxColour& GetTargetCalloutLabelColor() const noexcept
            {
            return m_targetCalloutLabelColor;
            }

        /// @brief Sets the color used for the "Target" word in its value callout
        ///     (the text before the colon).
        /// @param color The color to draw that word with.
        void SetTargetCalloutLabelColor(const wxColour& color) noexcept
            {
            m_targetCalloutLabelColor = color;
            }

        /// @returns @c true if the "Actual"/"Target" leader-line callouts are shown.
        [[nodiscard]]
        bool IsShowingValueCallouts() const noexcept
            {
            return m_showValueCallouts;
            }

        /** @brief Shows (or hides) the "Actual"/"Target" leader-line callouts.
            @param show @c true to show the callouts.
            @details A single callout pair is drawn above the visually topmost row
                (every row shares the same visual encoding, so labelling one is enough).
                Defaults to @c true.*/
        void ShowValueCallouts(const bool show) noexcept { m_showValueCallouts = show; }

        /// @returns How the actual/target values are displayed in the callouts.
        [[nodiscard]]
        BulletChartValueFormat GetValueDisplayFormat() const noexcept
            {
            return m_valueDisplayFormat;
            }

        /// @brief Sets how the actual/target values are displayed in the callouts.
        /// @param format The display format to use.
        void SetValueDisplayFormat(const BulletChartValueFormat format) noexcept
            {
            m_valueDisplayFormat = format;
            }

        /// @returns @c true if the qualitative bands are labelled with an axis bracket strip.
        [[nodiscard]]
        bool IsShowingRangeLabels() const noexcept
            {
            return m_showRangeLabels;
            }

        /** @brief Shows (or hides) the qualitative bands' axis bracket labels.
            @param show @c true to show the labels. Defaults to @c true.
            @sa SetRanges().*/
        void ShowRangeLabels(const bool show)
            {
            m_showRangeLabels = show;
            RebuildRangeBrackets();
            }

        /// @returns The name of the label column.
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns The name of the actual-value column.
        [[nodiscard]]
        const wxString& GetActualColumnName() const noexcept
            {
            return m_actualColumnName;
            }

        /// @returns The name of the target-value column.
        [[nodiscard]]
        const wxString& GetTargetColumnName() const noexcept
            {
            return m_targetColumnName;
            }

      private:
        /// @brief One colored segment of a row's range band.
        class RangeSegment final : public GraphItems::GraphItemBase
            {
          public:
            RangeSegment(GraphItems::GraphItemInfo itemInfo, const wxRect& rect,
                         const bool roundLeftCorners, const bool roundRightCorners)
                : GraphItemBase(std::move(itemInfo)), m_rect(rect),
                  m_roundLeftCorners(roundLeftCorners), m_roundRightCorners(roundRightCorners)
                {
                }

          private:
            [[nodiscard]]
            wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
                {
                return m_rect;
                }

            [[nodiscard]]
            bool HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const final
                {
                return m_rect.Contains(pt);
                }

            void Offset(const int xToMove, const int yToMove) final
                {
                m_rect.Offset(xToMove, yToMove);
                }

            wxRect Draw(wxDC& dc) const final;

            /** @deprecated Do not call this function. It is only included because it's
                    contractually required by the base class.*/
            [[deprecated("Not implemented")]]
            void SetBoundingBox([[maybe_unused]] const wxRect& rect, [[maybe_unused]] wxDC& dc,
                                [[maybe_unused]] const double parentScaling) final
                {
                wxFAIL_MSG(L"SetBoundingBox() not supported for RangeSegment objects.");
                }

            wxRect m_rect;
            bool m_roundLeftCorners{ false };
            bool m_roundRightCorners{ false };
            };

        /// @brief One KPI row, resolved from the dataset.
        struct RowInfo
            {
            wxString m_label;
            double m_axisPosition{ 0 };
            double m_actual{ std::numeric_limits<double>::quiet_NaN() };
            double m_target{ std::numeric_limits<double>::quiet_NaN() };
            };

        void RecalcSizes(wxDC& dc) final;

        void SetAutoAccessibilityAttributes() final;

        /// @brief Applies the axis styling used by this chart (opposite scaling axis
        ///     hidden, bar axis showing only custom KPI labels, etc.).
        void ApplyAxisAppearance();

        /// @brief Rebuilds every row's background Bar and the shared axis brackets from
        ///     the current @c m_rows/@c m_ranges.
        void RebuildRows();

        /// @brief Rebuilds the shared axis brackets from the current @c m_ranges,
        ///     honoring @c IsShowingRangeLabels().
        void RebuildRangeBrackets();

        /// @brief Adds the single "Actual"/"Target" leader-line callout pair for the
        ///     visually topmost row.
        /// @details Both labels are physically measured and, if they would overlap
        ///     (e.g., when the actual and target values are close together), nudged
        ///     apart horizontally so their text stays readable.
        /// @param dc The rendering DC, used to measure the labels' physical sizes.
        /// @param row The row to add the callouts for.
        void AddValueCallouts(wxDC& dc, const RowInfo& row);

        /// @returns The row that renders visually topmost.
        [[nodiscard]]
        const RowInfo& GetTopmostRow() const noexcept;

        /// @returns A blend between @p start and @p end.
        /// @param start The color at @p proportion @c 0.
        /// @param end The color at @p proportion @c 1.
        /// @param proportion Where between the two colors to sample, from @c 0 to @c 1.
        [[nodiscard]]
        static wxColour BlendColors(const wxColour& start, const wxColour& end, double proportion);

        std::vector<RowInfo> m_rows;
        std::vector<Range> m_ranges;

        wxString m_labelColumnName;
        wxString m_actualColumnName;
        wxString m_targetColumnName;

        wxBrush m_actualBarBrush{ *wxBLACK };
        wxPen m_actualBarPen{ *wxBLACK, 1 };
        wxPen m_targetTickPen{ *wxBLACK, 2 };
        wxColour m_actualCalloutLabelColor{ *wxBLACK };
        wxColour m_targetCalloutLabelColor{ *wxBLACK };

        BulletChartRangeColorScheme m_rangeColorScheme{ BulletChartRangeColorScheme::GoalStatus };
        wxColour m_rangeStartColor{ 217, 217, 217 };
        wxColour m_rangeEndColor{ 89, 89, 89 };
        wxColour m_goalSuccessColor{ Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen) };
        wxColour m_goalFailureColor{ Colors::ColorBrewer::GetColor(Colors::Color::FireEngineRed) };

        bool m_showValueCallouts{ true };
        BulletChartValueFormat m_valueDisplayFormat{ BulletChartValueFormat::Value };
        bool m_showRangeLabels{ true };

        double m_rangeBarWidthProportion{ 0.7 };
        double m_actualBarWidthProportion{ 0.3 };

        constexpr static double m_targetTickOvershootFactor{ 1.2 };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_BULLET_CHART_H
