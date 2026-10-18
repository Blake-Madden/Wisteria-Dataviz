/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SPC_CHART_H
#define WISTERIA_SPC_CHART_H

#include "lineplot.h"
#include <functional>

namespace Wisteria::Graphs
    {
    /// @brief The statistic used to reduce a sample to a single plotted point.
    enum class SampleStatistic
        {
        /// @brief The mean of the sample's observations.
        Mean,
        /// @brief The median of the sample's observations.
        Median
        };

    /** @brief Base class for statistical process control (SPC) charts.
        @details Observations are plotted in the order that they appear in the data,
            at equally spaced positions along the x-axis (1, 2, 3, ...).
            The position of a point is its place in the sequence, not a measured value.
            An optional label column supplies the x-axis tick text (e.g., dates or batch IDs),
            without changing the spacing.

            A missing value (NaN) keeps its place in the sequence and is drawn as a blank,
            disconnected point.

            Observations can also be collected into samples, where each sample is
            plotted as one point (its mean or median). A sample is either a fixed number of
            consecutive rows, or a run of adjacent rows sharing the same sample ID.
            The same ID in non-adjacent rows starts a new sample.
        @par %Data:
            This plot accepts a Data::Dataset with one continuous column (the measurement)
            and, optionally, a column (categorical, date, or continuous) of labels for the
            x-axis. When using sample IDs, the dataset also has a categorical or
            continuous column holding each row's sample.

         | DIAMETER | SAMPLE | BATCH   |
         | --:      | --:    | :--     |
         | 74.030   | 1      | Mon     |
         | 74.002   | 1      | Mon     |
         | 74.019   | 2      | Tue     |
         | 74.017   | 2      | Tue     |
        @par Citation:
            Montgomery, D. C. "Introduction to Statistical Quality Control."
            John Wiley & Sons.
    */
    class SpcChart : public LinePlot
        {
        wxDECLARE_DYNAMIC_CLASS(SpcChart);
        SpcChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.
            @param linePenStyles The line styles to use for the lines.
                Leave as @c nullptr to use solid, straight lines.*/
        explicit SpcChart(Canvas* canvas,
                          const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
                          const std::shared_ptr<Icons::Schemes::IconScheme>& shapes = nullptr,
                          const std::shared_ptr<LineStyleScheme>& linePenStyles = nullptr);

#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
        /** @brief Sets the data, plotting each row as one observation.
            @param data The data to use for the chart.
            @param valueColumnName The measurement column (a continuous column).
            @param labelColumnName The (optional) column to use for the x-axis labels.
                This can be a categorical, date, or continuous column.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        virtual void SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& valueColumnName,
                             const std::optional<wxString>& labelColumnName);
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

        /** @brief Sets the data, plotting one point for each sample of adjacent rows
                that share the same sample ID.
            @param data The data to use for the chart.
            @param valueColumnName The measurement column (a continuous column).
            @param sampleColumnName The column identifying each row's sample
                (a categorical or continuous column).\n
                A new sample starts whenever the ID changes from the previous row.
            @param labelColumnName The (optional) column to use for the x-axis labels.
                The label from the first row of each sample is used.
            @param statistic Whether to plot each sample's mean or median.
            @note Missing values are left out of a sample's statistic.
                A sample with no valid values is drawn as a gap.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& valueColumnName, const wxString& sampleColumnName,
                           const std::optional<wxString>& labelColumnName = std::nullopt,
                           SampleStatistic statistic = SampleStatistic::Mean);

        /** @brief Sets the data, plotting one point for each run of @p sampleSize
                consecutive rows.
            @param data The data to use for the chart.
            @param valueColumnName The measurement column (a continuous column).
            @param sampleSize The number of rows in each sample.\n
                The last sample can be smaller.
            @param labelColumnName The (optional) column to use for the x-axis labels.
                The label from the first row of each sample is used.
            @param statistic Whether to plot each sample's mean or median.
            @throws std::runtime_error If any columns can't be found by name,
                or if @p sampleSize is zero, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& valueColumnName, size_t sampleSize,
                           const std::optional<wxString>& labelColumnName = std::nullopt,
                           SampleStatistic statistic = SampleStatistic::Mean);

        /// @returns The column used for the x-axis labels, if any.
        [[nodiscard]]
        const std::optional<wxString>& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns The column identifying each row's sample, if samples are being defined
        ///     by an ID column.
        [[nodiscard]]
        const std::optional<wxString>& GetSampleColumnName() const noexcept
            {
            return m_sampleColumnName;
            }

        /// @returns The number of rows in each sample, if samples are being defined
        ///     by a fixed size.
        [[nodiscard]]
        const std::optional<size_t>& GetSampleSize() const noexcept
            {
            return m_sampleSize;
            }

        /// @returns The statistic plotted for each sample.
        [[nodiscard]]
        SampleStatistic GetSampleStatistic() const noexcept
            {
            return m_sampleStatistic;
            }

        /// @returns @c true if the individual observations in each sample are drawn
        ///     as faint points behind the sample statistic.
        [[nodiscard]]
        bool IsShowingIndividualPoints() const noexcept
            {
            return m_showIndividualPoints;
            }

        /// @brief Shows or hides the individual observations of each sample.
        /// @param show @c true to draw each observation as a faint point.
        /// @note This only applies when the data is collected into samples.
        ///     The y-axis is widened to include the observations when shown.
        void ShowIndividualPoints(bool show);

        /// @returns The opacity (0-255) of the individual observation points.
        [[nodiscard]]
        uint8_t GetIndividualPointOpacity() const noexcept
            {
            return m_individualPointOpacity;
            }

        /// @brief Sets the opacity of the individual observation points.
        /// @param opacity The opacity (0-255).
        void SetIndividualPointOpacity(const uint8_t opacity) noexcept
            {
            m_individualPointOpacity = opacity;
            }

      protected:
        void RecalcSizes(wxDC& dc) override;

        /// @returns The values being plotted, one per point (i.e., per observation or sample).
        [[nodiscard]]
        const std::vector<double>& GetPlottedValues() const noexcept
            {
            return m_plottedValues;
            }

        /// @brief Called after new data has been set and the plotted values are available.
        virtual void OnDataSet() {}

      private:
        [[nodiscard]]
        static const Data::Column<double>&
        FindValueColumn(const std::shared_ptr<const Data::Dataset>& data,
                        const wxString& valueColumnName);

        /// @returns A reader of the x-axis text for a row (empty text if there is no label column).
        [[nodiscard]]
        static std::function<wxString(size_t)>
        MakeLabelReader(const std::shared_ptr<const Data::Dataset>& data,
                        const std::optional<wxString>& labelColumnName);

        /// @returns The first row of each sample, where a new sample starts whenever the ID
        ///     differs from the previous row's ID.
        [[nodiscard]]
        static std::vector<size_t>
        SampleStartsById(const std::shared_ptr<const Data::Dataset>& data,
                         const wxString& sampleColumnName);

        [[nodiscard]]
        static std::vector<size_t> SampleStartsBySize(size_t rowCount, size_t sampleSize);

        void UpdateYAxisRange();

        void SetSamplesAt(const std::shared_ptr<const Data::Dataset>& data,
                          const wxString& valueColumnName, const std::vector<size_t>& starts,
                          const std::optional<wxString>& labelColumnName,
                          SampleStatistic statistic);

        void SetSequence(const std::vector<double>& values, const std::vector<wxString>& labels,
                         const wxString& valueColumnName, const wxString& sequenceName,
                         bool hasLabels);

        std::shared_ptr<Data::Dataset> m_sequenceData;
        std::vector<double> m_plottedValues;
        // position along the x-axis and value of each valid observation within a sample
        std::vector<std::pair<size_t, double>> m_observations;
        bool m_showIndividualPoints{ false };
        uint8_t m_individualPointOpacity{ 90 };
        std::optional<wxString> m_labelColumnName;
        std::optional<wxString> m_sampleColumnName;
        std::optional<size_t> m_sampleSize;
        SampleStatistic m_sampleStatistic{ SampleStatistic::Mean };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_SPC_CHART_H
