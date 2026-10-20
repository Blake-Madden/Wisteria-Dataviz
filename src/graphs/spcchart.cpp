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
                           const std::optional<wxString>& labelColumnName)
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
            OnDataSet();
            EvaluateRules();
            return;
            }

        const auto& valueColumn = FindValueColumn(data, valueColumnName);
        const auto readLabel = MakeLabelReader(data, labelColumnName);

        std::vector<double> values;
        std::vector<wxString> labels;
        values.reserve(data->GetRowCount());
        labels.reserve(data->GetRowCount());
        for (size_t row = 0; row < data->GetRowCount(); ++row)
            {
            values.push_back(valueColumn.GetValue(row));
            labels.push_back(readLabel(row));
            }

        m_observations.clear();
        SetSequence(values, labels, valueColumnName, _(L"Observation"),
                    labelColumnName.has_value());

        m_labelColumnName = labelColumnName;
        m_sampleColumnName.reset();
        m_sampleSize.reset();
        m_sampleStatistic = SampleStatistic::Mean;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                                 const wxString& valueColumnName, const wxString& sampleColumnName,
                                 const std::optional<wxString>& labelColumnName,
                                 const SampleStatistic statistic)
        {
        if (data == nullptr)
            {
            SetData(data, valueColumnName, labelColumnName);
            return;
            }
        SetSamplesAt(data, valueColumnName, SampleStartsById(data, sampleColumnName),
                     labelColumnName, statistic);

        m_labelColumnName = labelColumnName;
        m_sampleColumnName = sampleColumnName;
        m_sampleSize.reset();
        m_sampleStatistic = statistic;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSampleData(const std::shared_ptr<const Data::Dataset>& data,
                                 const wxString& valueColumnName, const size_t sampleSize,
                                 const std::optional<wxString>& labelColumnName,
                                 const SampleStatistic statistic)
        {
        if (data == nullptr)
            {
            SetData(data, valueColumnName, labelColumnName);
            return;
            }
        SetSamplesAt(data, valueColumnName, SampleStartsBySize(data->GetRowCount(), sampleSize),
                     labelColumnName, statistic);

        m_labelColumnName = labelColumnName;
        m_sampleColumnName.reset();
        m_sampleSize = sampleSize;
        m_sampleStatistic = statistic;
        }

    //----------------------------------------------------------------
    void SpcChart::SetSamplesAt(const std::shared_ptr<const Data::Dataset>& data,
                                const wxString& valueColumnName, const std::vector<size_t>& starts,
                                const std::optional<wxString>& labelColumnName,
                                const SampleStatistic statistic)
        {
        const auto& valueColumn = FindValueColumn(data, valueColumnName);
        const auto readLabel = MakeLabelReader(data, labelColumnName);

        std::vector<double> values;
        std::vector<wxString> labels;
        values.reserve(starts.size());
        labels.reserve(starts.size());
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
                if (std::isfinite(valueColumn.GetValue(row)))
                    {
                    observations.push_back(valueColumn.GetValue(row));
                    allObservations.emplace_back(sample + 1, valueColumn.GetValue(row));
                    }
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
        for (double scaled{ interval };
             precision < 8 && std::abs(scaled - std::round(scaled)) > 1e-9; scaled *= 10)
            {
            ++precision;
            }
        // end a hair early so that floating-point error can't add an extra tick
        GetLeftYAxis().SetRange(yStart, yEnd - (interval * 1e-6), precision, interval, 1);
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
        return wxPen(Colors::ColorBrewer::GetColor(Colors::Color::SlateGray), 2,
                     wxPenStyle::wxPENSTYLE_SHORT_DASH);
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

        const auto& values = GetPlottedValues();
        const auto center = GetCenterLine();
        const auto addViolations = [this](std::vector<statistics::spc_rule_violation> violations)
        { std::ranges::move(violations, std::back_inserter(m_ruleViolations)); };

        if (center.has_value() && IsRuleEnabled(statistics::spc_rule::shift))
            {
            addViolations(
                statistics::spc_rules::find_shifts(values, center.value(), m_shiftLength));
            }
        if (IsRuleEnabled(statistics::spc_rule::trend))
            {
            addViolations(statistics::spc_rules::find_trends(values, m_trendLength));
            }
        if (IsRuleEnabled(statistics::spc_rule::alternating))
            {
            addViolations(statistics::spc_rules::find_alternating(values, m_alternatingLength));
            }
        if (center.has_value() && IsRuleEnabled(statistics::spc_rule::runs))
            {
            m_runsSummary = statistics::spc_rules::count_runs(values, center.value());
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
    wxString SpcChart::GetRunsNote() const
        {
        if (!IsRuleEnabled(statistics::spc_rule::runs) || !GetCenterLine().has_value())
            {
            return {};
            }
        if (!m_runsSummary.has_value())
            {
            // TRANSLATORS: %s is a heading such as "Runs about the median"
            return wxString::Format(_(L"%s: too few points to test (at least %zu are needed)."),
                                    GetRunsLabel(), statistics::spc_rules::MIN_RUNS_OBSERVATIONS);
            }

        wxString note{ wxString::Format(
            // TRANSLATORS: the first %s is a heading such as "Runs about the median"
            _(L"%s: %zu (expected %s), longest run: %zu\n"
              "P-value for clustering: %s, for mixtures: %s"),
            GetRunsLabel(), m_runsSummary->runs,
            wxNumberFormatter::ToString(m_runsSummary->expected_runs, 1,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes),
            m_runsSummary->longest_run, FormatPValue(m_runsSummary->clustering_p_value),
            FormatPValue(m_runsSummary->mixtures_p_value)) };
        if (m_runsSummary->has_too_few_runs())
            {
            note += L"\n" + _(L"Too few runs, which suggests a shift in the process.");
            }
        else if (m_runsSummary->has_too_many_runs())
            {
            note += L"\n" + _(L"Too many runs, which suggests the process is oscillating.");
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
        const auto centerLineEntry = GetCenterLineLegendEntry();
        if (!centerLineEntry.has_value() && !hasCircles && !hasBaseline && runsNote.empty() &&
            !hasLegendReferences)
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

        // one entry for each rule that found a pattern, in a fixed order
        for (const auto& [rule, description] :
             { std::pair{
                   statistics::spc_rule::shift,
                   wxString::Format(_(L"Shift (%zu or more points on one side)"), m_shiftLength) },
               std::pair{ statistics::spc_rule::trend,
                          wxString::Format(_(L"Trend (%zu or more points rising or falling)"),
                                           m_trendLength) },
               std::pair{ statistics::spc_rule::alternating,
                          wxString::Format(_(L"Alternating (%zu or more points going up and down)"),
                                           m_alternatingLength) } })
            {
            const wxColour ringColor{ GetRuleRingColor(rule) };
            if (ringColor.IsOk() &&
                std::ranges::any_of(m_ruleViolations, [rule](const auto& violation)
                                    { return violation.rule == rule; }))
                {
                addEntry(description,
                         Icons::LegendIcon{ Icons::IconShape::Circle, wxPen{ ringColor, 2 },
                                            *wxTRANSPARENT_BRUSH });
                }
            }

        // the center line and the end of the baseline
        if (centerLineEntry.has_value() || hasBaseline)
            {
            startSection();
            if (centerLineEntry.has_value())
                {
                addEntry(centerLineEntry->first,
                         Icons::LegendIcon{ Icons::IconShape::HorizontalLine,
                                            centerLineEntry->second, *wxTRANSPARENT_BRUSH });
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
                }
            }

        // the runs note has no icon
        if (!runsNote.empty())
            {
            startSection();
            legendText += runsNote;
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
        AddBaselineDivider();
        AddRuleViolationMarkers(dc);
        }

    //----------------------------------------------------------------
    void SpcChart::AddBaselineDivider()
        {
        if (!HasBaseline())
            {
            return;
            }

        // drawn through the last baseline point
        wxPoint pt;
        if (!GetPhysicalCoordinates(static_cast<double>(m_baselineLength),
                                    GetLeftYAxis().GetRange().first, pt))
            {
            return;
            }
        auto divider = std::make_unique<GraphItems::Lines>(BaselineDividerPen(), GetScaling());
        divider->SetDPIScaleFactor(GetDPIScaleFactor());
        divider->AddLine(wxPoint{ pt.x, GetPlotAreaBoundingBox().GetTop() },
                         wxPoint{ pt.x, GetPlotAreaBoundingBox().GetBottom() });
        AddObject(std::move(divider));
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

        // each point is placed at its position in the sequence (1, 2, 3, ...)
        m_sequenceData = std::make_shared<Data::Dataset>();
        m_sequenceData->AddContinuousColumn(sequenceName);
        m_sequenceData->AddContinuousColumn(valueColumnName);
        m_sequenceData->Reserve(values.size());
        for (size_t index = 0; index < values.size(); ++index)
            {
            m_sequenceData->AddRow(
                Data::RowInfo{}
                    .Id(labels[index].empty() ? wxString::Format(L"%zu", index + 1) : labels[index])
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
                                                wxString::Format(L"%zu", position)));
                }
            }

        OnDataSet();
        EvaluateRules();
        }
    } // namespace Wisteria::Graphs
