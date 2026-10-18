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
    void SpcChart::RecalcSizes(wxDC & dc)
        {
        LinePlot::RecalcSizes(dc);

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
        }
    } // namespace Wisteria::Graphs
