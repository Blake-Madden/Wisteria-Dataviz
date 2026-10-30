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

#include "../math/spcrules.h"
#include "lineplot.h"
#include <functional>
#include <map>
#include <set>

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

    /// @brief The set of rules used to find non-random patterns.
    enum class SpcRuleSet
        {
        /// @brief The rules chosen with SpcChart::EnableRule(), using fixed lengths
        ///     (Perla et al., 2011, and Nelson, 1984).
        Perla,
        /// @brief The Anhøj rules, which test for an unusually long run and unusually few
        ///     crossings of the center line, with limits based on the number of points
        ///     (Anhøj and Olesen, 2014).
        Anhoej
        };

    /// @brief The center line and results of the rules for one stage of a chart.
    /// @note A chart that is not divided into stages has a single entry, which covers
    ///     all of the points and has no label.
    struct StageSummary
        {
        /// @brief The stage's label, or empty text if the chart is not divided into stages.
        wxString label;
        /// @brief The zero-based plotted point where the stage starts.
        size_t firstPoint{ 0 };
        /// @brief The number of plotted points in the stage.
        size_t pointCount{ 0 };
        /// @brief The stage's center line, or @c std::nullopt if it does not have one.
        std::optional<double> center;
        /// @brief The runs about the center line, or @c std::nullopt if the runs rule
        ///     is disabled or there are too few points.
        std::optional<statistics::runs_summary> runs;
        /// @brief The Anhøj results, or @c std::nullopt if not using the SpcRuleSet::Anhoej
        ///     rule set or there are no useful points.
        std::optional<statistics::anhoej_summary> anhoej;
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

            A chart can also be divided into stages (see the @c stageColumnName parameter of
            SetData() and SetSampleData()), where a column labels each row's stage
            (e.g., "Baseline" and "New protocol"). A new stage starts wherever the label
            differs from the previous sample's label, so a label that comes back later
            starts a new stage.\n
            Each stage has its own center line, and the rules restart at each stage break,
            so a run that spans two stages is not a pattern. A dashed divider is drawn at each
            break, and each stage's name is shown in a bracket under the x-axis.\n
            A stage cannot be combined with a baseline, since a baseline freezes one center
            line for the whole chart.
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
            @param stageColumnName The (optional) column labeling each row's stage
                (a categorical, date, or continuous column).\n
                Leave as @c std::nullopt to not divide the chart into stages.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                A stage cannot be combined with a baseline (see SetBaselineLength()).\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        virtual void SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& valueColumnName,
                             const std::optional<wxString>& labelColumnName,
                             const std::optional<wxString>& stageColumnName = std::nullopt);
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
            @param stageColumnName The (optional) column labeling each row's stage
                (a categorical, date, or continuous column).\n
                Leave as @c std::nullopt to not divide the chart into stages.
            @note Missing values are left out of a sample's statistic.
                A sample with no valid values is drawn as a gap.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                Every row of a sample must have the same stage, and a stage cannot be
                combined with a baseline (see SetBaselineLength()), otherwise an exception
                is thrown.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& valueColumnName, const wxString& sampleColumnName,
                           const std::optional<wxString>& labelColumnName = std::nullopt,
                           SampleStatistic statistic = SampleStatistic::Mean,
                           const std::optional<wxString>& stageColumnName = std::nullopt);

        /** @brief Sets the data, plotting one point for each run of @p sampleSize
                consecutive rows.
            @param data The data to use for the chart.
            @param valueColumnName The measurement column (a continuous column).
            @param sampleSize The number of rows in each sample.\n
                The last sample can be smaller.
            @param labelColumnName The (optional) column to use for the x-axis labels.
                The label from the first row of each sample is used.
            @param statistic Whether to plot each sample's mean or median.
            @param stageColumnName The (optional) column labeling each row's stage
                (a categorical, date, or continuous column).\n
                Leave as @c std::nullopt to not divide the chart into stages.
            @throws std::runtime_error If any columns can't be found by name,
                or if @p sampleSize is zero, throws an exception.\n
                Every row of a sample must have the same stage, and a stage cannot be
                combined with a baseline (see SetBaselineLength()), otherwise an exception
                is thrown.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& valueColumnName, size_t sampleSize,
                           const std::optional<wxString>& labelColumnName = std::nullopt,
                           SampleStatistic statistic = SampleStatistic::Mean,
                           const std::optional<wxString>& stageColumnName = std::nullopt);

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

        /// @returns The column labeling each row's stage, if the chart is divided
        ///     into stages.
        [[nodiscard]]
        const std::optional<wxString>& GetStageColumnName() const noexcept
            {
            return m_stageColumnName;
            }

        /// @returns @c true if the chart is divided into stages.
        [[nodiscard]]
        bool HasStages() const noexcept
            {
            return m_stageColumnName.has_value();
            }

        /// @returns The label of each stage, in order.\n
        ///     Empty if the chart is not divided into stages.
        [[nodiscard]]
        const std::vector<wxString>& GetStageLabels() const noexcept
            {
            return m_stageLabels;
            }

        /// @returns The zero-based plotted point where each stage starts, in order.\n
        ///     Empty if the chart is not divided into stages.
        [[nodiscard]]
        const std::vector<size_t>& GetStageStarts() const noexcept
            {
            return m_stageStarts;
            }

        /// @returns The center line and the results of the rules for each stage.\n
        ///     A chart that is not divided into stages has a single entry,
        ///     which covers all points and has no label.
        [[nodiscard]]
        const std::vector<StageSummary>& GetStageSummaries() const noexcept
            {
            return m_stageSummaries;
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

        /// @returns The number of points at the start of the chart used to compute
        ///     the center line, or @c 0 if all points are used.
        [[nodiscard]]
        size_t GetBaselineLength() const noexcept
            {
            return m_baselineLength;
            }

        /// @brief Sets the number of points at the start of the chart used to compute
        ///     the center line (i.e., freezes the center line to a baseline period).
        /// @details The rules still apply to all points, which are compared against the
        ///     baseline's center line. A dashed line is drawn through the last baseline point.\n
        ///     The legend notes when the baseline has fewer than 10 valid points,
        ///     which is the minimum recommended by Perla, R. J., Provost, L. P., and Murray, S. K.
        ///     "The run chart: a simple analytical tool for learning from variation in healthcare
        ///     processes." BMJ Quality & Safety, 20(1), 46-51, 2011.
        /// @param length The number of plotted points (i.e., samples when using samples,
        ///     not rows), or @c 0 to use all points.
        /// @note A baseline cannot be combined with stages (see SetData() and
        ///     SetSampleData()), because each stage sets its own center line.
        ///     Pass @c 0 to clear the baseline before setting staged data.
        /// @throws std::runtime_error If @p length is not zero and the chart is divided
        ///     into stages, throws an exception.
        void SetBaselineLength(size_t length);

        /// @name Rule Functions
        /// @brief Functions for detecting non-random patterns in the plotted points.
        /// @details Rules are applied to the plotted points (i.e., the sample statistics
        ///     when using samples). Points that are part of a pattern are circled,
        ///     with a different color for each rule.\n
        ///     The default shift (6) and trend (5) lengths are from Perla et al. (2011),
        ///     and the default alternating length (14) is from Nelson (1984).
        ///     See statistics::spc_rules for the full citations.
        /// @{

        /// @returns The set of rules used to find patterns.
        [[nodiscard]]
        SpcRuleSet GetRuleSet() const noexcept
            {
            return m_ruleSet;
            }

        /// @brief Sets the set of rules used to find patterns.
        /// @details With SpcRuleSet::Anhoej, the points of any run longer than its limit
        ///     are circled with the shift color, and the legend shows the longest run and
        ///     the number of crossings against their limits.
        ///     The rules chosen with EnableRule() are kept, but not applied.\n
        ///     The default is SpcRuleSet::Anhoej.
        /// @param ruleSet The set of rules.
        /// @note The Anhøj rules only apply to charts that have a center line.
        void SetRuleSet(SpcRuleSet ruleSet);

        /// @brief Enables or disables a rule.
        /// @param rule The rule.
        /// @param enable @c true to enable the rule.
        /// @note Rules that use the center line (shifts and runs) only apply to charts
        ///     that have a center line.\n
        ///     Rules are only applied when using the SpcRuleSet::Perla rule set.
        void EnableRule(statistics::spc_rule rule, bool enable);

        /// @param rule The rule.
        /// @returns @c true if @p rule is enabled.
        [[nodiscard]]
        bool IsRuleEnabled(const statistics::spc_rule rule) const
            {
            return m_enabledRules.contains(rule);
            }

        /// @returns The fewest points on one side of the center line that make a shift.
        [[nodiscard]]
        size_t GetShiftLength() const noexcept
            {
            return m_shiftLength;
            }

        /// @brief Sets the fewest points on one side of the center line that make a shift.
        /// @param length The number of points.
        /// @throws std::runtime_error If @p length is less than 2, throws an exception.
        void SetShiftLength(size_t length);

        /// @returns The fewest points all increasing or decreasing that make a trend.
        [[nodiscard]]
        size_t GetTrendLength() const noexcept
            {
            return m_trendLength;
            }

        /// @brief Sets the fewest points all increasing or decreasing that make a trend.
        /// @param length The number of points.
        /// @throws std::runtime_error If @p length is less than 3, throws an exception.
        void SetTrendLength(size_t length);

        /// @returns The fewest points alternating up and down that make an alternating run.
        [[nodiscard]]
        size_t GetAlternatingLength() const noexcept
            {
            return m_alternatingLength;
            }

        /// @brief Sets the fewest points alternating up and down that make an alternating run.
        /// @param length The number of points.
        /// @throws std::runtime_error If @p length is less than 3, throws an exception.
        void SetAlternatingLength(size_t length);

        /// @param rule The rule.
        /// @returns The color used to circle points that are part of a pattern found by @p rule.
        [[nodiscard]]
        wxColour GetRuleColor(const statistics::spc_rule rule) const
            {
            const auto ruleColor = m_ruleColors.find(rule);
            return (ruleColor != m_ruleColors.cend()) ? ruleColor->second : wxNullColour;
            }

        /// @brief Sets the color used to circle points that are part of a pattern found by a rule.
        /// @param rule The rule.
        /// @param color The color.
        /// @note The runs rule does not circle points, so its color is not used.
        void SetRuleColor(const statistics::spc_rule rule, const wxColour& color)
            {
            m_ruleColors[rule] = color;
            }

        /// @param rule The rule.
        /// @returns The default color used to circle points that are part of a pattern
        ///     found by @p rule.
        [[nodiscard]]
        static wxColour GetDefaultRuleColor(statistics::spc_rule rule);

        /// @returns The patterns found by the enabled rules.\n
        ///     The point indices are zero-based positions in the sequence.
        [[nodiscard]]
        const std::vector<statistics::spc_rule_violation>& GetRuleViolations() const noexcept
            {
            return m_ruleViolations;
            }

        /// @returns The number of runs about the center line and its critical values,
        ///     or @c std::nullopt if the runs rule is disabled or there are too few points.
        /// @note When the chart is divided into stages, this is the first stage's results.
        ///     Use GetStageSummaries() for all of them.
        [[nodiscard]]
        const std::optional<statistics::runs_summary>& GetRunsSummary() const noexcept
            {
            return m_runsSummary;
            }

        /// @returns The longest run and the number of crossings with their limits,
        ///     or @c std::nullopt if not using the SpcRuleSet::Anhoej rule set
        ///     or there are no useful points.
        /// @note When the chart is divided into stages, this is the first stage's results.
        ///     Use GetStageSummaries() for all of them.
        [[nodiscard]]
        const std::optional<statistics::anhoej_summary>& GetAnhoejSummary() const noexcept
            {
            return m_anhoejSummary;
            }

        /// @returns The results of the runs test (the number of runs, the expected number,
        ///     the longest run, and the p-values for clustering and mixtures),
        ///     or empty text if the runs rule is disabled.\n
        ///     With the SpcRuleSet::Anhoej rule set, this is the longest run and the number of
        ///     crossings with their limits instead.\n
        ///     When the chart is divided into stages, each stage's results are
        ///     listed under its label.
        /// @note This note is shown in the legend.
        [[nodiscard]]
        wxString GetRunsNote() const;

        /// @}

        /** @brief Builds a legend showing the center line, explaining the circled points,
                showing the results of the runs test, and listing any reference lines and areas
                meant for the legend.
            @param options The options for how to build the legend.
            @returns The legend for the chart, or @c nullptr if there is nothing to show.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

      protected:
        void RecalcSizes(wxDC& dc) override;

        /// @brief Draws the center line beneath the data.
        /// @details When a baseline is set, the line is drawn with its pen across the
        ///     baseline and dashed past it, where the center line is frozen.\n
        ///     When the chart is divided into stages, each stage is drawn from its own
        ///     center line.
        void AddUnderlayObjects() override;

        /// @returns The center line of the first stage, or @c std::nullopt if the chart
        ///     does not have one.
        [[nodiscard]]
        std::optional<double> GetCenterLine() const noexcept;

        /// @returns The heading of the runs note.
        [[nodiscard]]
        virtual wxString GetRunsLabel() const
            {
            return _(L"Runs about the center line");
            }

        /// @returns The legend text and the pen for drawing the center line, with one entry
        ///     for each stage (or a single entry if the chart is not divided into stages).
        ///     Empty if the center line is not drawn.
        [[nodiscard]]
        virtual std::vector<std::pair<wxString, wxPen>> GetCenterLineLegendEntries() const
            {
            return {};
            }

        /// @brief Sets the center line that is drawn for each stage.
        /// @param centers One center line per stage, in the order of GetStageStarts(),
        ///     with @c std::nullopt for a stage that does not have one.\n
        ///     A chart that is not divided into stages has a single center line.
        /// @note This is called from OnDataSet(), once the plotted values are available.
        void SetStageCenterLines(std::vector<std::optional<double>> centers);

        /// @returns The values being plotted, one per point (i.e., per observation or sample).
        [[nodiscard]]
        const std::vector<double>& GetPlottedValues() const noexcept
            {
            return m_plottedValues;
            }

        /// @returns The values used to compute the center line, which are the first
        ///     GetBaselineLength() plotted values, or all of them if there is no baseline.
        [[nodiscard]]
        std::vector<double> GetBaselineValues() const;

        /// @returns @c true if a baseline is set and it ends before the last point.
        [[nodiscard]]
        bool HasBaseline() const noexcept
            {
            return m_baselineLength > 0 && m_baselineLength < m_plottedValues.size();
            }

        /// @brief Called after new data (or a new baseline) has been set and
        ///     the plotted values are available.
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

        [[nodiscard]]
        static size_t ValidateRuleLength(size_t length, size_t smallest);

        [[nodiscard]]
        static wxString FormatPValue(double pValue);

        /// @returns The stage label of every row.
        /// @throws std::runtime_error If the column can't be found or has a blank label.
        [[nodiscard]]
        static std::vector<wxString>
        ReadStageLabels(const std::shared_ptr<const Data::Dataset>& data,
                        const wxString& stageColumnName);

        /// @brief Stores the stage of each plotted point.
        /// @param stagePerPoint The stage label of each plotted point, in order.
        /// @param stageColumnName The stage column, or @c std::nullopt to clear the stages.
        /// @throws std::runtime_error If @p stageColumnName is set and a baseline is in use.
        void ApplyStages(const std::vector<wxString>& stagePerPoint,
                         const std::optional<wxString>& stageColumnName);

        /// @brief Clears the stages and their results.
        void ClearStages();

        /// @returns The first and past-the-last plotted point of each stage.
        [[nodiscard]]
        std::vector<std::pair<size_t, size_t>> GetStageRanges() const;

        /// @brief Rebuilds the brackets that label each stage along the bottom x-axis.
        /// @details These brackets are generated from the data, so they are not saved.
        void UpdateStageBrackets();

        /// @returns The results of the rules for @p stage, as shown in the legend.
        [[nodiscard]]
        wxString RunsNoteForStage(const StageSummary& stage) const;

        void UpdateYAxisRange();
        void EvaluateRules();
        void AddIndividualPoints(wxDC& dc);
        void AddRuleViolationMarkers(wxDC& dc);
        void AddDividers();

        [[nodiscard]]
        static wxPen BaselineDividerPen();

        /// @returns The color of the rings drawn for @p rule,
        ///     or an invalid color if the rule is not drawn.
        [[nodiscard]]
        wxColour GetRuleRingColor(statistics::spc_rule rule) const;

        /// @returns @c true if any pattern was found by a rule that has a color.
        [[nodiscard]]
        bool HasRuleRings() const;

        constexpr static uint8_t RING_OPACITY{ 160 };
        // fewest valid baseline points recommended for a stable center line (Perla et al., 2011)
        constexpr static size_t MIN_BASELINE_POINTS{ 10 };

        void SetSamplesAt(const std::shared_ptr<const Data::Dataset>& data,
                          const wxString& valueColumnName, const std::vector<size_t>& starts,
                          const std::optional<wxString>& labelColumnName, SampleStatistic statistic,
                          const std::optional<wxString>& stageColumnName);

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
        size_t m_baselineLength{ 0 };

        // stage division and the results of the rules for each stage
        std::optional<wxString> m_stageColumnName;
        std::vector<size_t> m_stageStarts;
        std::vector<wxString> m_stageLabels;
        std::vector<std::optional<double>> m_stageCenterLines;
        std::vector<StageSummary> m_stageSummaries;

        SpcRuleSet m_ruleSet{ SpcRuleSet::Anhoej };
        std::set<statistics::spc_rule> m_enabledRules;
        size_t m_shiftLength{ 6 };
        size_t m_trendLength{ 5 };
        size_t m_alternatingLength{ 14 };
        std::map<statistics::spc_rule, wxColour> m_ruleColors{
            { statistics::spc_rule::shift, GetDefaultRuleColor(statistics::spc_rule::shift) },
            { statistics::spc_rule::trend, GetDefaultRuleColor(statistics::spc_rule::trend) },
            { statistics::spc_rule::alternating,
              GetDefaultRuleColor(statistics::spc_rule::alternating) }
        };
        std::vector<statistics::spc_rule_violation> m_ruleViolations;
        std::optional<statistics::runs_summary> m_runsSummary;
        std::optional<statistics::anhoej_summary> m_anhoejSummary;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_SPC_CHART_H
