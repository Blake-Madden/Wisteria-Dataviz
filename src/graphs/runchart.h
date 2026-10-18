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
        @par %Data:
            This chart accepts a Data::Dataset with one continuous column (the measurement)
            and, optionally, a column of labels for the x-axis (e.g., dates or batch IDs).

         | DIAMETER | BATCH   |
         | --:      | :--     |
         | 74.030   | Mon     |
         | 74.002   | Tue     |
         | 74.019   | Wed     |

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
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.
            @param linePenStyles The line styles to use for the lines.
                Leave as @c nullptr to use solid, straight lines.*/
        explicit RunChart(Canvas* canvas,
                          const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
                          const std::shared_ptr<Icons::Schemes::IconScheme>& shapes = nullptr,
                          const std::shared_ptr<LineStyleScheme>& linePenStyles = nullptr);

        /// @returns The median of the plotted points (the observations, or the sample
        ///     statistics when using samples), or @c std::nullopt if there is no valid data.
        [[nodiscard]]
        const std::optional<double>& GetMedian() const noexcept
            {
            return m_median;
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
        bool IsMedianLine(const GraphItems::ReferenceLine& refLine) const;

      private:
        [[nodiscard]]
        static wxPen MedianLinePen();

        [[nodiscard]]
        static wxString MedianLabelPrefix();

        void OnDataSet() override;
        void RemoveMedianLine();
        void UpdateMedianLine();

        std::optional<double> m_median;
        bool m_showMedianLine{ true };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_RUN_CHART_H
