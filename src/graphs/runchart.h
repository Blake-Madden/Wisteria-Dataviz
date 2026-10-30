/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_RUN_CHART_H
#define WISTERIA_RUN_CHART_H

#include "spcchart.h"

namespace Wisteria::Graphs
    {
    /** @brief Run chart, which plots observations in sequence with a median line.
        @details A run chart is used to look for trends, shifts, and other non-random patterns
            in a process over time. It is often used as a first step before building a
            control chart.

            Observations are plotted in the order that they appear in the data, and the
            median of the plotted points is drawn as a reference line.
            Observations can also be collected into samples (see SetSampleData()),
            in which case each sample's mean or median is plotted.
            Missing values (NaN) are shown as gaps and are not included in the median.
            The median can be computed from a baseline period at the start of the chart
            instead of from all points (see SetBaselineLength()).
            The baseline is known as Phase I, where the median is set from data that
            represent how the process normally behaves. The points after it are Phase II,
            where new data are compared against that frozen median to see if the process
            has changed.
            Instead of a baseline, the chart can be divided into stages (see the
            @c stageColumnName parameter of SetSampleData()), where each stage
            (e.g., "Baseline" and "New protocol") gets its own median and its own rules,
            with a divider drawn at each break and the stage's name shown in a bracket
            under the x-axis.
            A baseline is the simple form of this: one median taken from the first @c n
            points and frozen for the rest of the chart. Stages generalize that to one
            median for each named group.
            Stages and a baseline cannot be used together.
            Rules for shifts, trends, alternating points, and the number of runs about the
            median can be enabled to circle non-random patterns (see EnableRule()).
        @par %Data:
            This chart accepts a Data::Dataset with one continuous column (the measurement)
            and, optionally, columns for the x-axis labels (e.g., dates or batch IDs)
            and for the stage of each row.

         | DIAMETER | BATCH   | STAGE         |
         | --:      | :--     | :--           |
         | 74.030   | Mon     | Baseline      |
         | 74.002   | Tue     | Baseline      |
         | 74.019   | Wed     | New protocol  |

        @code
         auto runChart = std::make_shared<RunChart>(canvas);
         runChart->SetData(dataset, L"DIAMETER", L"BATCH");
         canvas->SetFixedObject(0, 0, runChart);
        @endcode
        @par Citation:
            Perla, R. J., Provost, L. P., and Murray, S. K. "The run chart: a simple analytical
            tool for learning from variation in healthcare processes."
            BMJ Quality & Safety, 20(1), 46-51, 2011.
    */
    class RunChart final : public SpcChart
        {
        wxDECLARE_DYNAMIC_CLASS(RunChart);
        RunChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to draw them in black.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.
            @param linePenStyles The line styles to use for the lines.
                Leave as @c nullptr to use solid, straight lines.*/
        explicit RunChart(Canvas* canvas,
                          const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
                          const std::shared_ptr<Icons::Schemes::IconScheme>& shapes = nullptr,
                          const std::shared_ptr<LineStyleScheme>& linePenStyles = nullptr);

        /// @returns The median of the plotted points (the observations, or the sample
        ///     statistics when using samples) in the baseline period,
        ///     or @c std::nullopt if there is no valid data.\n
        ///     When the chart is divided into stages, each stage has its own median,
        ///     so this is @c std::nullopt (unless there is only one stage).
        ///     Use GetStageMedians() instead.
        [[nodiscard]]
        const std::optional<double>& GetMedian() const noexcept
            {
            return m_median;
            }

        /// @returns The median of each stage, or an empty list if the chart is not
        ///     divided into stages.\n
        ///     A stage without any valid points has no median.
        [[nodiscard]]
        const std::vector<std::optional<double>>& GetStageMedians() const noexcept
            {
            return m_stageMedians;
            }

        /// @returns @c true if the median line is shown.
        [[nodiscard]]
        bool IsShowingMedianLine() const noexcept
            {
            return m_showMedianLine;
            }

        /// @brief Shows or hides the median line.
        /// @param show @c true to show the median line.
        void ShowMedianLine(const bool show)
            {
            m_showMedianLine = show;
            UpdateMedianLine();
            }

        /// @param refLine The reference line to review.
        /// @returns @c true if @p refLine is the median line that this chart draws.
        /// @note The line is recognized by its style and label, not its position.
        ///     It is rebuilt from the data, so it should not be saved or loaded
        ///     as a user-defined reference line.
        [[nodiscard]]
        static bool IsMedianLine(const GraphItems::ReferenceLine& refLine);

      protected:
        /// @returns The heading of the runs note.
        [[nodiscard]]
        wxString GetRunsLabel() const override
            {
            return _(L"Runs about the median");
            }

        /// @returns The legend text and pen for each stage's median line, with one entry
        ///     for each stage, or an empty list if the median line is not shown.
        [[nodiscard]]
        std::vector<std::pair<wxString, wxPen>> GetCenterLineLegendEntries() const override;

      private:
        [[nodiscard]]
        static wxPen MedianLinePen();

        /// @returns The transparent pen of the reference line that places the median's label.
        [[nodiscard]]
        static wxPen MedianLabelPen();

        [[nodiscard]]
        static wxString MedianLabelPrefix();

        /// @returns The prefix followed by @p median, or empty text if there is no median.
        [[nodiscard]]
        static wxString MedianLabel(const std::optional<double>& median);

        /// @returns The median of @p stage, prefixed by the stage's label
        ///     for the legend, or empty text if the stage has no median.
        [[nodiscard]]
        wxString StageMedianLabel(size_t stage) const;

        void OnDataSet() override;
        void RemoveMedianLine();
        void UpdateMedianLine();

        std::optional<double> m_median;
        std::vector<std::optional<double>> m_stageMedians;
        bool m_showMedianLine{ true };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_RUN_CHART_H
