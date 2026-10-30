///////////////////////////////////////////////////////////////////////////////
// Name:        spcchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "spcchart.h"
#include "../math/statistics.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::SpcChart, Wisteria::Graphs::LinePlot)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    const Data::Column<double>& SpcChart::FindValueColumn(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumnName)
        {
        const auto valueColumn = data->GetContinuousColumn(valueColumnName);
        if (valueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': value column not found for SPC chart."), valueColumnName)
                    .ToUTF8());
            }
        return *valueColumn;
        }

    //----------------------------------------------------------------
    // returns the text shown on the x-axis for a row (empty if there is no label column)
    std::function<wxString(size_t)> SpcChart::MakeLabelReader(
        const std::shared_ptr<const Data::Dataset>& data,
        const std::optional<wxString>& labelColumnName)
        {
        if (!labelColumnName.has_value())
            {
            return [](const size_t) { return wxString{}; };
            }

        const auto labelContinuous = data->GetContinuousColumn(labelColumnName.value());
        const auto labelCategorical = data->GetCategoricalColumn(labelColumnName.value());
        const auto labelDate = data->GetDateColumn(labelColumnName.value());
        const auto* continuousColumn =
            (labelContinuous != data->GetContinuousColumns().cend()) ? &*labelContinuous : nullptr;
        const auto* categoricalColumn = (labelCategorical != data->GetCategoricalColumns().cend()) ?
                                            &*labelCategorical :
                                            nullptr;
        const auto* dateColumn =
            (labelDate != data->GetDateColumns().cend()) ? &*labelDate : nullptr;
        if (continuousColumn == nullptr && categoricalColumn == nullptr && dateColumn == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': label column not found for SPC chart."),
                                 labelColumnName.value())
                    .ToUTF8());
            }

        return [continuousColumn, categoricalColumn, dateColumn](const size_t row) -> wxString
        {
            if (categoricalColumn != nullptr)
                {
                return categoricalColumn->GetLabelFromID(categoricalColumn->GetValue(row));
                }
            if (dateColumn != nullptr)
                {
                return dateColumn->GetValue(row).IsValid() ?
                           dateColumn->GetValue(row).FormatDate() :
                           wxString{};
                }
            return std::isfinite(continuousColumn->GetValue(row)) ?
                       wxNumberFormatter::ToString(
                           continuousColumn->GetValue(row), 6,
                           wxNumberFormatter::Style::Style_NoTrailingZeroes) :
                       wxString{};
        };
        }

    //----------------------------------------------------------------
    // a new sample starts whenever the ID differs from the previous row's ID
    std::vector<size_t> SpcChart::SampleStartsById(const std::shared_ptr<const Data::Dataset>& data,
                                                   const wxString& sampleColumnName)
        {
        const auto sampleContinuous = data->GetContinuousColumn(sampleColumnName);
        const auto sampleCategorical = data->GetCategoricalColumn(sampleColumnName);
        const bool isContinuous{ sampleContinuous != data->GetContinuousColumns().cend() };
        const bool isCategorical{ sampleCategorical != data->GetCategoricalColumns().cend() };
        if (!isContinuous && !isCategorical)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': sample column not found for SPC chart."),
                                 sampleColumnName)
                    .ToUTF8());
            }

        // a numeric sample column is an ID, so it can only hold whole numbers
        if (!isCategorical &&
            std::ranges::any_of(sampleContinuous->GetValues(), [](const double val) noexcept
                                { return std::isfinite(val) && val != std::trunc(val); }))
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': sample column must contain whole numbers."),
                                 sampleColumnName)
                    .ToUTF8());
            }

        const auto idAt = [&](const size_t row)
        {
            return isCategorical ? static_cast<double>(sampleCategorical->GetValue(row)) :
                                   sampleContinuous->GetValue(row);
        };

        std::vector<size_t> starts;
        for (size_t row = 0; row < data->GetRowCount(); ++row)
            {
            if (row == 0)
                {
                starts.push_back(row);
                continue;
                }
            const double current{ idAt(row) };
            const double previous{ idAt(row - 1) };
            const bool bothMissing{ std::isnan(current) && std::isnan(previous) };
            if (current != previous && !bothMissing)
                {
                starts.push_back(row);
                }
            }
        return starts;
        }

    //----------------------------------------------------------------
    std::vector<size_t> SpcChart::SampleStartsBySize(const size_t rowCount, const size_t sampleSize)
        {
        if (sampleSize == 0)
            {
            throw std::runtime_error(_(L"Sample size must be greater than zero.").ToUTF8());
            }

        std::vector<size_t> starts;
        size_t countInSample{ 0 };
        for (size_t row = 0; row < rowCount; ++row)
            {
            if (countInSample == 0)
                {
                starts.push_back(row);
                }
            ++countInSample;
            if (countInSample == sampleSize)
                {
                countInSample = 0;
                }
            }
        return starts;
        }

    //----------------------------------------------------------------
    std::vector<wxString> SpcChart::ReadStageLabels(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& stageColumnName)
        {
        if (data->GetContinuousColumn(stageColumnName) == data->GetContinuousColumns().cend() &&
            data->GetCategoricalColumn(stageColumnName) == data->GetCategoricalColumns().cend() &&
            data->GetDateColumn(stageColumnName) == data->GetDateColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': stage column not found for SPC chart."), stageColumnName)
                    .ToUTF8());
            }

        const auto readStage = MakeLabelReader(data, stageColumnName);
        std::vector<wxString> labels;
        labels.reserve(data->GetRowCount());
        for (size_t row = 0; row < data->GetRowCount(); ++row)
            {
            const wxString label{ readStage(row) };
            // a row has to belong to a stage, otherwise its points would be unlabeled
            if (label.empty())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': row %zu does not have a stage. Every row needs a stage label."),
                        stageColumnName, row + 1)
                        .ToUTF8());
                }
            labels.push_back(label);
            }
        return labels;
        }

    //----------------------------------------------------------------
    // a new stage starts wherever the label differs from the previous point's label
    void SpcChart::ApplyStages(const std::vector<wxString>& stagePerPoint,
                               const std::optional<wxString>& stageColumnName)
        {
        if (!stageColumnName.has_value())
            {
            ClearStages();
            return;
            }
        if (m_baselineLength > 0)
            {
            throw std::runtime_error(
                _(L"Stages cannot be combined with a baseline. Set the baseline length to zero "
                  L"first.")
                    .ToUTF8());
            }

        m_stageColumnName = stageColumnName;
        m_stageStarts.clear();
        m_stageLabels.clear();
        for (size_t point = 0; point < stagePerPoint.size(); ++point)
            {
            if (point == 0 || stagePerPoint[point] != stagePerPoint[point - 1])
                {
                m_stageStarts.push_back(point);
                m_stageLabels.push_back(stagePerPoint[point]);
                }
            }
        }

    //----------------------------------------------------------------
    void SpcChart::ClearStages()
        {
        m_stageColumnName.reset();
        m_stageStarts.clear();
        m_stageLabels.clear();
        m_stageCenterLines.clear();
        m_stageSummaries.clear();
        }

    //----------------------------------------------------------------
    void SpcChart::UpdateStageBrackets()
        {
        auto& brackets{ GetBottomXAxis().GetBrackets() };
        // only remove the brackets that this chart added, so user-defined ones are kept
        const auto isStageBracket = [](const GraphItems::Axis::AxisBracket& bracket)
        { return !bracket.GetLabel().GetPropertyTemplate(L"stage-bracket").empty(); };
        std::erase_if(brackets, isStageBracket);

        if (m_stageStarts.empty() || m_plottedValues.empty())
            {
            return;
            }

        // each stage is labeled with its name, spanning the points that belong to it
        const auto ranges{ GetStageRanges() };
        for (size_t stage = 0; stage < ranges.size() && stage < m_stageLabels.size(); ++stage)
            {
            const wxString& label{ m_stageLabels[stage] };
            if (label.empty())
                {
                continue;
                }
            // the points are placed at one-based positions
            const double start{ static_cast<double>(ranges[stage].first + 1) };
            const double end{ static_cast<double>(ranges[stage].second) };
            // the default bracket color, which also lets an HTML dashboard invert it in dark mode
            GraphItems::Axis::AxisBracket bracket(
                start, end, (start + end) / 2, label,
                wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black), 1),
                BracketLineStyle::Lines);
            // marks the bracket as generated from the data, so that it isn't saved to a report
            bracket.GetLabel().SetPropertyTemplate(L"stage-bracket", L"true");
            GetBottomXAxis().AddBracket(std::move(bracket));
            }

        // the labels are rebuilt from the data, so they shouldn't be persisted anywhere
        GetBottomXAxis().SetBracketsAreDynamic(true);
        }

    //----------------------------------------------------------------
    std::vector<std::pair<size_t, size_t>> SpcChart::GetStageRanges() const
        {
        if (m_stageStarts.empty())
            {
            return { std::pair<size_t, size_t>{ 0, m_plottedValues.size() } };
            }

        std::vector<std::pair<size_t, size_t>> ranges;
        ranges.reserve(m_stageStarts.size());
        for (size_t stage = 0; stage < m_stageStarts.size(); ++stage)
            {
            ranges.emplace_back(m_stageStarts[stage], (stage + 1 < m_stageStarts.size()) ?
                                                          m_stageStarts[stage + 1] :
                                                          m_plottedValues.size());
            }
        return ranges;
        }

    //----------------------------------------------------------------
    void SpcChart::SetStageCenterLines(std::vector<std::optional<double>> centers)
        {
        m_stageCenterLines = std::move(centers);
        }

    //----------------------------------------------------------------
    std::optional<double> SpcChart::GetCenterLine() const noexcept
        {
        return m_stageCenterLines.empty() ? std::nullopt : m_stageCenterLines.front();
        }

    //----------------------------------------------------------------
    SpcChart::SpcChart(Canvas * canvas,
                       const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
                       const std::shared_ptr<Icons::Schemes::IconScheme>& shapes /*= nullptr*/,
                       const std::shared_ptr<LineStyleScheme>& linePenStyles /*= nullptr*/)
        : LinePlot(canvas, colors, shapes, linePenStyles)
        {
        GetLeftYAxis().StartAtZero(false);
        // the right axis only carries labels (e.g., the median), and the top axis is unused
        GetRightYAxis().GetAxisLinePen() = wxNullPen;
        GetTopXAxis().GetAxisLinePen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void SpcChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                           const wxString& valueColumnName,
                           const std::optional<wxString>& labelColumnName,
                           const std::optional<wxString>& stageColumnName)
        {
        if (data == nullptr)
            {
            m_observations.clear();
            m_plottedValues.clear();
            m_sequenceData.reset();
            LinePlot::SetData(nullptr, wxString{}, wxString{}, std::nullopt);
            GetLines().clear();
            m_labelColumnName.reset();
            m_sampleColumnName.reset();
            m_sampleSize.reset();
            m_sampleStatistic = SampleStatistic::Mean;
            ClearStages();
            UpdateStageBrackets();
            OnDataSet();
            EvaluateRules();
            return;
            }

        const auto& valueColumn = FindValueColumn(data, valueColumnName);
        const auto readLabel = MakeLabelReader(data, labelColumnName);
        const std::vector<wxString> stageLabels{
            stageColumnName.has_value() ? ReadStageLabels(data, stageColumnName.value()) :
                                          std::vector<wxString>{}
        };

        std::vector<double> values;
        std::vector<wxString> labels;
        values.reserve(data->GetRowCount());
        labels.reserve(data->GetRowCount());
        for (size_t row = 0; row < data->GetRowCount(); ++row)
            {
            values.push_back(valueColumn.GetValue(row));
            labels.push_back(readLabel(row));
            }

        ApplyStages(stageLabels, stageColumnName);
        m_observations.clear();
        SetSequence(values, labels, valueColumnName, _(L"Observation"),
                    labelColumnName.has_value());

        m_labelColumnName = labelColumnName;
        m_sampleColumnName.reset();
        m_sampleSize.reset();
        m_sampleStatistic = SampleStatistic::Mean;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSampleData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumnName,
        const wxString& sampleColumnName, const std::optional<wxString>& labelColumnName,
        const SampleStatistic statistic, const std::optional<wxString>& stageColumnName)
        {
        if (data == nullptr)
            {
            SetData(data, valueColumnName, labelColumnName, stageColumnName);
            return;
            }
        SetSamplesAt(data, valueColumnName, SampleStartsById(data, sampleColumnName),
                     labelColumnName, statistic, stageColumnName);

        m_labelColumnName = labelColumnName;
        m_sampleColumnName = sampleColumnName;
        m_sampleSize.reset();
        m_sampleStatistic = statistic;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSampleData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumnName,
        const size_t sampleSize, const std::optional<wxString>& labelColumnName,
        const SampleStatistic statistic, const std::optional<wxString>& stageColumnName)
        {
        if (data == nullptr)
            {
            SetData(data, valueColumnName, labelColumnName, stageColumnName);
            return;
            }
        SetSamplesAt(data, valueColumnName, SampleStartsBySize(data->GetRowCount(), sampleSize),
                     labelColumnName, statistic, stageColumnName);

        m_labelColumnName = labelColumnName;
        m_sampleColumnName.reset();
        m_sampleSize = sampleSize;
        m_sampleStatistic = statistic;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSamplesAt(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumnName,
        const std::vector<size_t>& starts, const std::optional<wxString>& labelColumnName,
        const SampleStatistic statistic, const std::optional<wxString>& stageColumnName)
        {
        const auto& valueColumn = FindValueColumn(data, valueColumnName);
        const auto readLabel = MakeLabelReader(data, labelColumnName);
        const std::vector<wxString> rowStages{ stageColumnName.has_value() ?
                                                   ReadStageLabels(data, stageColumnName.value()) :
                                                   std::vector<wxString>{} };

        std::vector<double> values;
        std::vector<wxString> labels;
        // the stage of each sample, which must not differ from the stages of its rows
        std::vector<wxString> stagePerSample;
        values.reserve(starts.size());
        labels.reserve(starts.size());
        stagePerSample.reserve(starts.size());
        std::vector<std::pair<size_t, double>> allObservations;
        for (size_t sample = 0; sample < starts.size(); ++sample)
            {
            const size_t first{ starts[sample] };
            const size_t last{ (sample + 1 < starts.size()) ? starts[sample + 1] :
                                                              data->GetRowCount() };

            std::vector<double> observations;
            observations.reserve(last - first);
            for (size_t row = first; row < last; ++row)
                {
                // a sample belongs to one stage, so that it has a single median to plot
                if (stageColumnName.has_value() && rowStages[row] != rowStages[first])
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            // TRANSLATORS: the first %s is a column name, %zu is the
                            // (one-based) number of a sample, and the other two are stage labels
                            _(L"'%s': sample %zu spans two stages ('%s' and '%s')."),
                            stageColumnName.value(), sample + 1, rowStages[first], rowStages[row])
                            .ToUTF8());
                    }
                if (std::isfinite(valueColumn.GetValue(row)))
                    {
                    observations.push_back(valueColumn.GetValue(row));
                    allObservations.emplace_back(sample + 1, valueColumn.GetValue(row));
                    }
                }
            if (stageColumnName.has_value())
                {
                stagePerSample.push_back(rowStages[first]);
                }

            // a sample without any valid values is a gap
            double plotted{ std::numeric_limits<double>::quiet_NaN() };
            if (!observations.empty())
                {
                plotted = (statistic == SampleStatistic::Mean) ? statistics::mean(observations) :
                                                                 statistics::median(observations);
                }
            values.push_back(plotted);
            labels.push_back(readLabel(first));
            }

        ApplyStages(stagePerSample, stageColumnName);
        m_observations = std::move(allObservations);
        SetSequence(values, labels, valueColumnName, _(L"Sample"), labelColumnName.has_value());
        }

    //----------------------------------------------------------------
    void SpcChart::ShowIndividualPoints(const bool show)
        {
        m_showIndividualPoints = show;
        UpdateYAxisRange();
        }

    //----------------------------------------------------------------
    void SpcChart::UpdateYAxisRange()
        {
        double minValue{ std::numeric_limits<double>::max() };
        double maxValue{ std::numeric_limits<double>::lowest() };
        for (const auto value : m_plottedValues)
            {
            if (std::isfinite(value))
                {
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
                }
            }
        if (m_showIndividualPoints)
            {
            for (const auto& observation : m_observations)
                {
                minValue = std::min(minValue, observation.second);
                maxValue = std::max(maxValue, observation.second);
                }
            }
        if (minValue > maxValue)
            {
            return;
            }

        // a single value (or identical values) is given a range around it
        if (minValue == maxValue)
            {
            const double padding{ (minValue == 0.0) ? 1.0 : std::abs(minValue) * 0.1 };
            minValue -= padding;
            maxValue += padding;
            }

        const auto [yStart, yEnd] = adjust_intervals(minValue, maxValue);
        const double interval{ neat_interval(minValue, maxValue) };

        // use the same interval that the range was rounded to, rather than the axis's own
        // deduction (which uses an interval of 0.2 for any range under 1, no matter its scale)
        uint8_t precision{ 0 };
        double scaled{ interval };
        while (precision < 8 && std::abs(scaled - std::round(scaled)) > 1e-9)
            {
            ++precision;
            scaled *= 10;
            }
        // end a hair early so that floating-point error can't add an extra tick
        GetLeftYAxis().SetRange(yStart, yEnd - (interval * 1e-6), precision, interval, 1);
        }

    //----------------------------------------------------------------
    void SpcChart::SetRuleSet(const SpcRuleSet ruleSet)
        {
        m_ruleSet = ruleSet;
        EvaluateRules();
        }

    //----------------------------------------------------------------
    void SpcChart::EnableRule(const statistics::spc_rule rule, const bool enable)
        {
        if (enable)
            {
            m_enabledRules.insert(rule);
            }
        else
            {
            m_enabledRules.erase(rule);
            }
        EvaluateRules();
        }

    //----------------------------------------------------------------
    size_t SpcChart::ValidateRuleLength(const size_t length, const size_t smallest)
        {
        if (length < smallest)
            {
            throw std::runtime_error(
                wxString::Format(_(L"SPC rule length must be at least %zu."), smallest).ToUTF8());
            }
        return length;
        }

    //----------------------------------------------------------------
    void SpcChart::SetShiftLength(const size_t length)
        {
        m_shiftLength = ValidateRuleLength(length, statistics::spc_rules::MIN_SHIFT_LENGTH);
        EvaluateRules();
        }

    //----------------------------------------------------------------
    void SpcChart::SetTrendLength(const size_t length)
        {
        m_trendLength = ValidateRuleLength(length, statistics::spc_rules::MIN_TREND_LENGTH);
        EvaluateRules();
        }

    //----------------------------------------------------------------
    void SpcChart::SetAlternatingLength(const size_t length)
        {
        m_alternatingLength =
            ValidateRuleLength(length, statistics::spc_rules::MIN_ALTERNATING_LENGTH);
        EvaluateRules();
        }

    //----------------------------------------------------------------
    void SpcChart::SetBaselineLength(const size_t length)
        {
        if (length != 0 && HasStages())
            {
            throw std::runtime_error(
                _(L"A baseline cannot be combined with stages. Remove the chart's stages first.")
                    .ToUTF8());
            }
        m_baselineLength = length;
        OnDataSet();
        EvaluateRules();
        }

    //----------------------------------------------------------------
    std::vector<double> SpcChart::GetBaselineValues() const
        {
        if (!HasBaseline())
            {
            return m_plottedValues;
            }
        return { m_plottedValues.cbegin(),
                 std::next(m_plottedValues.cbegin(), static_cast<ptrdiff_t>(m_baselineLength)) };
        }

    //----------------------------------------------------------------
    wxPen SpcChart::BaselineDividerPen()
        {
        return { Colors::ColorBrewer::GetColor(Colors::Color::SlateGray), 2,
                 wxPenStyle::wxPENSTYLE_SHORT_DASH };
        }

    //----------------------------------------------------------------
    wxColour SpcChart::GetDefaultRuleColor(const statistics::spc_rule rule)
        {
        switch (rule)
            {
        case statistics::spc_rule::shift:
            return Colors::ColorBrewer::GetColor(Colors::Color::Orange);
        case statistics::spc_rule::trend:
            return Colors::ColorBrewer::GetColor(Colors::Color::Purple);
        case statistics::spc_rule::alternating:
            return Colors::ColorBrewer::GetColor(Colors::Color::Teal);
        default:
            return wxNullColour;
            }
        }

    //----------------------------------------------------------------
    wxColour SpcChart::GetRuleRingColor(const statistics::spc_rule rule) const
        {
        const wxColour ruleColor{ GetRuleColor(rule) };
        return ruleColor.IsOk() ? Colors::ColorContrast::ChangeOpacity(ruleColor, RING_OPACITY) :
                                  wxNullColour;
        }

    //----------------------------------------------------------------
    bool SpcChart::HasRuleRings() const
        {
        return std::ranges::any_of(m_ruleViolations, [this](const auto& violation)
                                   { return GetRuleColor(violation.rule).IsOk(); });
        }

    //----------------------------------------------------------------
    void SpcChart::EvaluateRules()
        {
        m_ruleViolations.clear();
        m_runsSummary.reset();
        m_anhoejSummary.reset();
        m_stageSummaries.clear();

        const auto& values = GetPlottedValues();
        const auto ranges = GetStageRanges();
        m_stageSummaries.reserve(ranges.size());
        for (size_t stage = 0; stage < ranges.size(); ++stage)
            {
            const size_t first{ ranges[stage].first };
            const size_t last{ ranges[stage].second };
            // the rules are checked within a stage, so a streak cannot span a stage break
            const std::vector<double> stageValues(
                std::next(values.cbegin(), static_cast<ptrdiff_t>(first)),
                std::next(values.cbegin(), static_cast<ptrdiff_t>(last)));
            const std::optional<double> center{ (stage < m_stageCenterLines.size()) ?
                                                    m_stageCenterLines[stage] :
                                                    std::nullopt };
            StageSummary summary;
            summary.label = (stage < m_stageLabels.size()) ? m_stageLabels[stage] : wxString{};
            summary.firstPoint = first;
            summary.pointCount = last - first;
            summary.center = center;

            // the points of a violation are shifted from the stage to the whole sequence
            const auto addViolations =
                [this, first](std::vector<statistics::spc_rule_violation> violations)
            {
                for (auto& violation : violations)
                    {
                    for (auto& point : violation.points)
                        {
                        point += first;
                        }
                    }
                std::ranges::move(violations, std::back_inserter(m_ruleViolations));
            };

            if (m_ruleSet == SpcRuleSet::Anhoej)
                {
                if (center.has_value())
                    {
                    summary.anhoej =
                        statistics::spc_rules::anhoej_analysis(stageValues, center.value());
                    // the points of any run longer than the limit are shown as a shift
                    if (summary.anhoej.has_value())
                        {
                        addViolations(statistics::spc_rules::find_shifts(
                            stageValues, center.value(), summary.anhoej->longest_run_limit + 1));
                        }
                    }
                }
            else
                {
                if (center.has_value() && IsRuleEnabled(statistics::spc_rule::shift))
                    {
                    addViolations(statistics::spc_rules::find_shifts(stageValues, center.value(),
                                                                     m_shiftLength));
                    }
                if (IsRuleEnabled(statistics::spc_rule::trend))
                    {
                    addViolations(statistics::spc_rules::find_trends(stageValues, m_trendLength));
                    }
                if (IsRuleEnabled(statistics::spc_rule::alternating))
                    {
                    addViolations(
                        statistics::spc_rules::find_alternating(stageValues, m_alternatingLength));
                    }
                if (center.has_value() && IsRuleEnabled(statistics::spc_rule::runs))
                    {
                    summary.runs = statistics::spc_rules::count_runs(stageValues, center.value());
                    }
                }
            m_stageSummaries.push_back(std::move(summary));
            }

        if (!m_stageSummaries.empty())
            {
            m_runsSummary = m_stageSummaries.front().runs;
            m_anhoejSummary = m_stageSummaries.front().anhoej;
            }
        }

    //----------------------------------------------------------------
    wxString SpcChart::FormatPValue(const double pValue)
        {
        constexpr double SMALLEST_SHOWN{ 0.001 };
        return (pValue < SMALLEST_SHOWN) ?
                   L"< " + wxNumberFormatter::ToString(SMALLEST_SHOWN, 3,
                                                       wxNumberFormatter::Style::Style_None) :
                   wxNumberFormatter::ToString(pValue, 3, wxNumberFormatter::Style::Style_None);
        }

    //----------------------------------------------------------------
    wxString SpcChart::RunsNoteForStage(const StageSummary& stage) const
        {
        if (m_ruleSet == SpcRuleSet::Anhoej)
            {
            if (!stage.anhoej.has_value())
                {
                return {};
                }
            wxString note{ wxString::Format(
                // TRANSLATORS: %s is a heading such as "Runs about the median"
                _(L"%s (Anhøj rules)\nLongest run: %zu (signal if more than %zu)\n"
                  "Crossings: %zu (signal if fewer than %zu)"),
                GetRunsLabel(), stage.anhoej->longest_run, stage.anhoej->longest_run_limit,
                stage.anhoej->crossings, stage.anhoej->crossings_limit) };
            if (stage.anhoej->has_long_run())
                {
                note += L"\n" + _(L"Unusually long run, which suggests a shift in the process.");
                }
            if (stage.anhoej->has_too_few_crossings())
                {
                note += L"\n" + _(L"Too few crossings, which suggests a shift in the process.");
                }
            return note;
            }

        if (!IsRuleEnabled(statistics::spc_rule::runs) || !stage.center.has_value())
            {
            return {};
            }
        if (!stage.runs.has_value())
            {
            // TRANSLATORS: %s is a heading such as "Runs about the median"
            return wxString::Format(_(L"%s: too few points to test (at least %zu are needed)."),
                                    GetRunsLabel(), statistics::spc_rules::MIN_RUNS_OBSERVATIONS);
            }

        wxString note{ wxString::Format(
            // TRANSLATORS: the first %s is a heading such as "Runs about the median"
            _(L"%s: %zu (expected %s), longest run: %zu\n"
              "P-value for clustering: %s, for mixtures: %s"),
            GetRunsLabel(), stage.runs->runs,
            wxNumberFormatter::ToString(stage.runs->expected_runs, 1,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes),
            stage.runs->longest_run, FormatPValue(stage.runs->clustering_p_value),
            FormatPValue(stage.runs->mixtures_p_value)) };
        if (stage.runs->has_too_few_runs())
            {
            note += L"\n" + _(L"Too few runs, which suggests a shift in the process.");
            }
        else if (stage.runs->has_too_many_runs())
            {
            note += L"\n" + _(L"Too many runs, which suggests the process is oscillating.");
            }
        return note;
        }

    //----------------------------------------------------------------
    wxString SpcChart::GetRunsNote() const
        {
        if (!HasStages())
            {
            return m_stageSummaries.empty() ? wxString{} :
                                              RunsNoteForStage(m_stageSummaries.front());
            }

        wxString note;
        for (const auto& stage : m_stageSummaries)
            {
            const wxString stageNote{ RunsNoteForStage(stage) };
            if (stageNote.empty())
                {
                continue;
                }
            if (!note.empty())
                {
                note += L"\n";
                }
            // the stage's label prefixes its results
            note += wxString::Format(L"%s: %s", stage.label, stageNote);
            }
        return note;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> SpcChart::CreateLegend(const LegendOptions& options)
        {
        SetLegendInfo(options);
        const wxString runsNote{ GetRunsNote() };
        const bool hasCircles{ HasRuleRings() };
        // the median line is labeled on the opposite axis, so only count lines meant for the legend
        const bool hasLegendReferences{
            !GetReferenceAreas().empty() ||
            std::ranges::any_of(
                GetReferenceLines(), [](const auto& refLine)
                { return refLine.GetLabelPlacement() == ReferenceLabelPlacement::Legend; })
        };
        const bool hasBaseline{ HasBaseline() };
        const auto centerLineEntries = GetCenterLineLegendEntries();
        // the dividers between stages are only drawn at the breaks
        const bool hasStageBreaks{ m_stageStarts.size() > 1 };
        if (centerLineEntries.empty() && !hasCircles && !hasBaseline && !hasStageBreaks &&
            runsNote.empty() && !hasLegendReferences)
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));
        wxString legendText;
        const wxString separatorText{ L"\n \n" };
        // a separator goes between sections, but not before the first one
        const auto startSection = [&legendText, &legend, &separatorText]()
        {
            if (!legendText.empty())
                {
                legendText += separatorText;
                legend->GetLegendIcons().emplace_back(
                    Icons::IconShape::HorizontalSeparator,
                    wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black), 2 },
                    *wxTRANSPARENT_BRUSH);
                }
        };
        const auto addEntry = [&legendText, &legend, &separatorText](const wxString& text,
                                                                     const Icons::LegendIcon& icon)
        {
            if (!legendText.empty() && !legendText.ends_with(separatorText))
                {
                legendText += L"\n";
                }
            legendText += text;
            legend->GetLegendIcons().push_back(icon);
        };

        // the Anhøj rules find runs longer than a limit based on the number of points
        const bool isAnhoej{ m_ruleSet == SpcRuleSet::Anhoej &&
                             std::ranges::any_of(m_stageSummaries, [](const StageSummary& stage)
                                                 { return stage.anhoej.has_value(); }) };
        const wxString shiftDescription{
            isAnhoej ?
                (HasStages() ? _(L"Long run (longer than the limit for its stage)") :
                               wxString::Format(_(L"Long run (%zu or more points on one side)"),
                                                m_anhoejSummary->longest_run_limit + 1)) :
                wxString::Format(_(L"Shift (%zu or more points on one side)"), m_shiftLength)
        };
        // one entry for each rule that found a pattern, in a fixed order
        for (const auto& [rule, description] :
             { std::pair{ statistics::spc_rule::shift, shiftDescription },
               std::pair{ statistics::spc_rule::trend,
                          wxString::Format(_(L"Trend (%zu or more points rising or falling)"),
                                           m_trendLength) },
               std::pair{ statistics::spc_rule::alternating,
                          wxString::Format(_(L"Alternating (%zu or more points going up and down)"),
                                           m_alternatingLength) } })
            {
            const wxColour ringColor{ GetRuleRingColor(rule) };
            if (ringColor.IsOk() &&
                std::ranges::find(m_ruleViolations, rule, &statistics::spc_rule_violation::rule) !=
                    m_ruleViolations.end())
                {
                addEntry(description,
                         Icons::LegendIcon{ Icons::IconShape::Circle, wxPen{ ringColor, 2 },
                                            *wxTRANSPARENT_BRUSH });
                }
            }

        // the center line, the end of the baseline, and the stage breaks
        if (!centerLineEntries.empty() || hasBaseline || hasStageBreaks)
            {
            startSection();
            for (const auto& [text, pen] : centerLineEntries)
                {
                addEntry(text, Icons::LegendIcon{ Icons::IconShape::HorizontalLine, pen,
                                                  *wxTRANSPARENT_BRUSH });
                }
            if (hasBaseline)
                {
                const bool usesSamples{ m_sampleColumnName.has_value() ||
                                        m_sampleSize.has_value() };
                addEntry(wxString::Format(usesSamples ?
                                              _(L"End of baseline (first %zu samples)") :
                                              _(L"End of baseline (first %zu observations)"),
                                          m_baselineLength),
                         Icons::LegendIcon{ Icons::IconShape::VerticalLine, BaselineDividerPen(),
                                            *wxTRANSPARENT_BRUSH });

                const auto baselineValues = GetBaselineValues();
                const auto validBaselineCount = static_cast<size_t>(std::ranges::count_if(
                    baselineValues, [](const double val) noexcept { return std::isfinite(val); }));
                if (validBaselineCount < MIN_BASELINE_POINTS)
                    {
                    addEntry(wxString::Format(
                                 _(L"Baseline has %zu points (at least %zu are recommended)."),
                                 validBaselineCount, MIN_BASELINE_POINTS),
                             Icons::LegendIcon{ Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                *wxTRANSPARENT_BRUSH });
                    }
                }
            // the dashed dividers between the stages
            if (hasStageBreaks)
                {
                addEntry(wxString::Format(_(L"Stage breaks (%zu stages)"), m_stageStarts.size()),
                         Icons::LegendIcon{ Icons::IconShape::VerticalLine, BaselineDividerPen(),
                                            *wxTRANSPARENT_BRUSH });
                }
            }

        // the runs results have no icons, other than the dividers between the stages
        if (!runsNote.empty())
            {
            startSection();
            if (!HasStages())
                {
                legendText += runsNote;
                }
            else
                {
                // each line of text gets a blank icon, so that the dividers line up with their rows
                const auto addStageNote = [&legendText, &legend](const wxString& note)
                {
                    legendText += note;
                    const size_t lineCount{ static_cast<size_t>(note.Freq(L'\n')) + 1 };
                    for (size_t line = 0; line < lineCount; ++line)
                        {
                        legend->GetLegendIcons().emplace_back(
                            Icons::IconShape::Blank, *wxTRANSPARENT_PEN, *wxTRANSPARENT_BRUSH);
                        }
                };
                bool isFirstStage{ true };
                for (const auto& stage : m_stageSummaries)
                    {
                    const wxString stageNote{ RunsNoteForStage(stage) };
                    if (stageNote.empty())
                        {
                        continue;
                        }
                    // each stage's results are divided from the next stage's
                    if (!isFirstStage)
                        {
                        startSection();
                        }
                    isFirstStage = false;
                    // the stage's label prefixes its results
                    addStageNote(wxString::Format(L"%s: %s", stage.label, stageNote));
                    }
                }
            }
        legend->SetText(legendText);

        if (hasLegendReferences)
            {
            AddReferenceLinesAndAreasToLegend(*legend);
            // the reference lines start with a separator, which is not needed
            // if they are the only thing in the legend
            if (legendText.empty() && legend->GetText().starts_with(separatorText) &&
                !legend->GetLegendIcons().empty())
                {
                legend->SetText(legend->GetText().substr(separatorText.length()));
                legend->GetLegendIcons().erase(legend->GetLegendIcons().begin());
                }
            }

        // the header is added last, since it does not have an icon
        if (options.IsIncludingHeader())
            {
            legend->SetText(wxString::Format(
                L"%s\n%s", options.GetTitle().empty() ? _(L"Rules") : options.GetTitle(),
                legend->GetText()));
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }

        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    void SpcChart::RecalcSizes(wxDC & dc)
        {
        LinePlot::RecalcSizes(dc);

        AddIndividualPoints(dc);
        AddDividers();
        AddRuleViolationMarkers(dc);
        }

    //----------------------------------------------------------------
    void SpcChart::AddUnderlayObjects()
        {
        const auto centerLineEntries = GetCenterLineLegendEntries();
        if (centerLineEntries.empty() || m_stageCenterLines.empty())
            {
            return;
            }
        const wxPen& centerPen{ centerLineEntries.front().second };

        const wxCoord leftX{ GetPlotAreaBoundingBox().GetLeft() };
        const wxCoord rightX{ GetPlotAreaBoundingBox().GetRight() };
        const auto addSegment =
            [this](const wxPen& pen, const wxCoord startX, const wxCoord endX, const wxCoord y)
        {
            auto segment = std::make_unique<GraphItems::Lines>(pen, GetScaling());
            segment->SetDPIScaleFactor(GetDPIScaleFactor());
            segment->AddLine(wxPoint{ startX, y }, wxPoint{ endX, y });
            AddObject(std::move(segment));
        };

        // the center line is frozen past the last baseline point
        if (HasBaseline())
            {
            const auto center = GetCenterLine();
            wxCoord centerY{ 0 };
            if (!center.has_value() ||
                !GetLeftYAxis().GetPhysicalCoordinate(center.value(), centerY))
                {
                return;
                }
            wxCoord baselineEndX{ rightX };
            wxPoint pt;
            if (GetPhysicalCoordinates(static_cast<double>(m_baselineLength), center.value(), pt))
                {
                baselineEndX = std::clamp(pt.x, leftX, rightX);
                }
            addSegment(centerPen, leftX, baselineEndX, centerY);
            if (baselineEndX < rightX)
                {
                wxPen extendedPen{ centerPen };
                extendedPen.SetStyle(wxPenStyle::wxPENSTYLE_LONG_DASH);
                addSegment(extendedPen, baselineEndX, rightX, centerY);
                }
            return;
            }

        // each stage is drawn from its own center line, from one stage break to the next
        const auto ranges = GetStageRanges();
        for (size_t stage = 0; stage < ranges.size(); ++stage)
            {
            if (stage >= m_stageCenterLines.size() || !m_stageCenterLines[stage].has_value())
                {
                continue;
                }
            const double center{ m_stageCenterLines[stage].value() };
            wxCoord centerY{ 0 };
            if (!GetLeftYAxis().GetPhysicalCoordinate(center, centerY))
                {
                continue;
                }

            // stages span the plot, with each break halfway between two points
            wxPoint pt;
            wxCoord startX{ leftX };
            if (stage != 0 &&
                GetPhysicalCoordinates(
                    static_cast<double>(ranges[stage].first) + math_constants::half, center, pt))
                {
                startX = std::clamp(pt.x, leftX, rightX);
                }
            wxCoord endX{ rightX };
            if (stage + 1 < ranges.size() &&
                GetPhysicalCoordinates(
                    static_cast<double>(ranges[stage].second) + math_constants::half, center, pt))
                {
                endX = std::clamp(pt.x, leftX, rightX);
                }
            addSegment(centerPen, startX, std::max(startX, endX), centerY);
            }
        }

    //----------------------------------------------------------------
    void SpcChart::AddDividers()
        {
        const auto addDivider = [this](const wxCoord x)
        {
            auto divider = std::make_unique<GraphItems::Lines>(BaselineDividerPen(), GetScaling());
            divider->SetDPIScaleFactor(GetDPIScaleFactor());
            divider->AddLine(wxPoint{ x, GetPlotAreaBoundingBox().GetTop() },
                             wxPoint{ x, GetPlotAreaBoundingBox().GetBottom() });
            AddObject(std::move(divider));
        };

        if (HasBaseline())
            {
            // drawn through the last baseline point
            wxPoint pt;
            if (GetPhysicalCoordinates(static_cast<double>(m_baselineLength),
                                       GetLeftYAxis().GetRange().first, pt))
                {
                addDivider(pt.x);
                }
            return;
            }

        // one divider halfway between the last point of a stage and the first of the next
        for (size_t stage = 1; stage < m_stageStarts.size(); ++stage)
            {
            wxPoint pt;
            if (GetPhysicalCoordinates(static_cast<double>(m_stageStarts[stage]) +
                                           math_constants::half,
                                       GetLeftYAxis().GetRange().first, pt))
                {
                addDivider(pt.x);
                }
            }
        }

    //----------------------------------------------------------------
    void SpcChart::AddRuleViolationMarkers(wxDC & dc)
        {
        if (!HasRuleRings())
            {
            return;
            }

        auto markers = std::make_unique<GraphItems::Points2D>(wxNullPen);
        markers->SetScaling(GetScaling());
        markers->SetDPIScaleFactor(GetDPIScaleFactor());

        // a point that is part of patterns from more than one rule gets a larger ring
        // for each additional rule
        std::map<size_t, size_t> ringsAtPoint;
        constexpr size_t RING_SPACING{ 3 };
        wxPoint pt;
        for (const auto rule : { statistics::spc_rule::shift, statistics::spc_rule::trend,
                                 statistics::spc_rule::alternating })
            {
            const wxColour ringColor{ GetRuleRingColor(rule) };
            if (!ringColor.IsOk())
                {
                continue;
                }
            std::set<size_t> rulePoints;
            for (const auto& violation : m_ruleViolations)
                {
                if (violation.rule == rule)
                    {
                    rulePoints.insert(violation.points.cbegin(), violation.points.cend());
                    }
                }
            for (const auto index : rulePoints)
                {
                const size_t ringIndex{ ringsAtPoint[index]++ };
                if (index < m_plottedValues.size() &&
                    GetPhysicalCoordinates(static_cast<double>(index + 1), m_plottedValues[index],
                                           pt))
                    {
                    markers->AddPoint(GraphItems::Point2D(GraphItems::GraphItemInfo{}
                                                              .AnchorPoint(pt)
                                                              .Pen(wxPen{ ringColor, 2 })
                                                              .Brush(*wxTRANSPARENT_BRUSH),
                                                          (Settings::GetPointRadius() * 2) +
                                                              (ringIndex * RING_SPACING),
                                                          Icons::IconShape::Circle),
                                      dc);
                    }
                }
            }
        AddObject(std::move(markers));
        }

    //----------------------------------------------------------------
    void SpcChart::AddIndividualPoints(wxDC & dc)
        {
        if (!m_showIndividualPoints || m_observations.empty() || GetLines().empty())
            {
            return;
            }

        const wxColour pointColor{ Colors::ColorContrast::ChangeOpacity(
            GetLines().front().GetPen().GetColour(), m_individualPointOpacity) };
        auto points = std::make_unique<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(m_observations.size());

        wxPoint pt;
        for (const auto& [position, value] : m_observations)
            {
            if (GetPhysicalCoordinates(static_cast<double>(position), value, pt))
                {
                points->AddPoint(
                    GraphItems::Point2D(GraphItems::GraphItemInfo{}
                                            .AnchorPoint(pt)
                                            .Pen(pointColor)
                                            .Brush(pointColor),
                                        std::max<size_t>(1, Settings::GetPointRadius() / 2),
                                        Icons::IconShape::Circle),
                    dc);
                }
            }
        AddObject(std::move(points));
        }

    //----------------------------------------------------------------
    void SpcChart::SetSequence(const std::vector<double>& values,
                               const std::vector<wxString>& labels, const wxString& valueColumnName,
                               const wxString& sequenceName, const bool hasLabels)
        {
        m_plottedValues = values;
        // a chart that is not divided into stages still has one center line for all points
        m_stageCenterLines.assign(m_stageStarts.empty() ? 1 : m_stageStarts.size(), std::nullopt);
        m_stageSummaries.clear();

        // each point is placed at its position in the sequence (1, 2, 3, ...)
        m_sequenceData = std::make_shared<Data::Dataset>();
        m_sequenceData->AddContinuousColumn(sequenceName);
        m_sequenceData->AddContinuousColumn(valueColumnName);
        m_sequenceData->Reserve(values.size());
        for (size_t index = 0; index < values.size(); ++index)
            {
            m_sequenceData->AddRow(
                Data::RowInfo{}
                    .Id(labels[index].empty() ? wxString{ std::to_wstring(index + 1) } :
                                                labels[index])
                    .Continuous({ static_cast<double>(index + 1), values[index] }));
            }

        LinePlot::SetData(m_sequenceData, valueColumnName, sequenceName, std::nullopt);
        UpdateYAxisRange();

        const auto pointCount = m_sequenceData->GetRowCount();
        if (pointCount > 1)
            {
            GetBottomXAxis().SetRange(1, static_cast<double>(pointCount), 0, 1, 1);

            // label every step-th point, where the step grows
            // through 1, 2, 5, 10, 20, 50, ... as the number of points grows
            constexpr size_t MAX_LABELS{ 12 };
            size_t step{ 1 };
            for (size_t magnitude{ 1 }; pointCount > MAX_LABELS * step; magnitude *= 10)
                {
                for (const size_t multiplier : { size_t{ 1 }, size_t{ 2 }, size_t{ 5 } })
                    {
                    step = multiplier * magnitude;
                    if (pointCount <= MAX_LABELS * step)
                        {
                        break;
                        }
                    }
                }

            std::vector<size_t> labeledPositions;
            // start with the first point, unless the first multiple of the step is right next to it
            if (step > 2)
                {
                labeledPositions.push_back(1);
                }
            for (size_t position = step; position <= pointCount; position += step)
                {
                labeledPositions.push_back(position);
                }
            // also label the last point, unless it would crowd the previous label
            if (labeledPositions.back() != pointCount &&
                (pointCount - labeledPositions.back()) * 2 > step)
                {
                labeledPositions.push_back(pointCount);
                }

            GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
            for (const auto position : labeledPositions)
                {
                const bool hasText{ hasLabels && position <= labels.size() &&
                                    !labels[position - 1].empty() };
                GetBottomXAxis().SetCustomLabel(
                    static_cast<double>(position),
                    GraphItems::Label(hasText ? labels[position - 1] :
                                                wxString{ std::to_wstring(position) }));
                }
            }

        UpdateStageBrackets();
        OnDataSet();
        EvaluateRules();
        }
    } // namespace Wisteria::Graphs
