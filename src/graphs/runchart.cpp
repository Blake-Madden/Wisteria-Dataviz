///////////////////////////////////////////////////////////////////////////////
// Name:        runchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "runchart.h"
#include "../math/statistics.h"
#include <iterator>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::RunChart, Wisteria::Graphs::SpcChart)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    RunChart::RunChart(Canvas * canvas,
                       const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
                       const std::shared_ptr<Icons::Schemes::IconScheme>& shapes /*= nullptr*/,
                       const std::shared_ptr<LineStyleScheme>& linePenStyles /*= nullptr*/)
        : SpcChart(canvas,
                   // black lets an HTML dashboard invert the line and its points in dark mode
                   colors != nullptr ?
                       colors :
                       std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::ColorScheme{
                           Colors::ColorBrewer::GetColor(Colors::Color::Black) }),
                   shapes, linePenStyles)
        {
        }

    //----------------------------------------------------------------
    wxPen RunChart::MedianLinePen()
        {
        return { Colors::ColorBrewer::GetColor(Colors::Color::TractorRed), 2,
                 wxPenStyle::wxPENSTYLE_SOLID };
        }

    //----------------------------------------------------------------
    wxPen RunChart::MedianLabelPen()
        {
        return { MedianLinePen().GetColour(), 2, wxPenStyle::wxPENSTYLE_TRANSPARENT };
        }

    //----------------------------------------------------------------
    wxString RunChart::MedianLabelPrefix() { return _(L"Median: "); }

    //----------------------------------------------------------------
    wxString RunChart::MedianLabel(const std::optional<double>& median)
        {
        if (!median.has_value())
            {
            return {};
            }
        return MedianLabelPrefix() +
               wxNumberFormatter::ToString(median.value(), 4,
                                           wxNumberFormatter::Style::Style_NoTrailingZeroes);
        }

    //----------------------------------------------------------------
    wxString RunChart::StageMedianLabel(const size_t stage) const
        {
        if (stage >= m_stageMedians.size() || !m_stageMedians[stage].has_value() ||
            stage >= GetStageLabels().size())
            {
            return {};
            }
        // TRANSLATORS: the first %s is a stage's label and the second is its median
        return wxString::Format(
            _(L"%s: median %s"), GetStageLabels()[stage],
            wxNumberFormatter::ToString(m_stageMedians[stage].value(), 4,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes));
        }

    //----------------------------------------------------------------
    std::vector<std::pair<wxString, wxPen>> RunChart::GetCenterLineLegendEntries() const
        {
        if (!m_showMedianLine)
            {
            return {};
            }

        std::vector<std::pair<wxString, wxPen>> entries;
        if (!HasStages())
            {
            if (m_median.has_value())
                {
                entries.emplace_back(MedianLabel(m_median), MedianLinePen());
                }
            return entries;
            }

        // one entry for each stage, named after the stage
        entries.reserve(m_stageMedians.size());
        for (size_t stage = 0; stage < m_stageMedians.size(); ++stage)
            {
            const wxString label{ StageMedianLabel(stage) };
            if (!label.empty())
                {
                entries.emplace_back(label, MedianLinePen());
                }
            }
        return entries;
        }

    //----------------------------------------------------------------
    bool RunChart::IsMedianLine(const GraphItems::ReferenceLine& refLine)
        {
        const wxPen medianPen{ MedianLabelPen() };
        return refLine.GetAxisType() == AxisType::LeftYAxis &&
               refLine.GetLabel().starts_with(MedianLabelPrefix()) &&
               refLine.GetLabelPlacement() == ReferenceLabelPlacement::OppositeAxis &&
               refLine.GetPen().IsOk() && refLine.GetPen().GetColour() == medianPen.GetColour() &&
               refLine.GetPen().GetWidth() == medianPen.GetWidth() &&
               refLine.GetPen().GetStyle() == medianPen.GetStyle();
        }

    //----------------------------------------------------------------
    void RunChart::RemoveMedianLine()
        {
        std::erase_if(GetReferenceLines(),
                      [](const auto& refLine) { return IsMedianLine(refLine); });
        }

    //----------------------------------------------------------------
    void RunChart::OnDataSet()
        {
        m_median.reset();
        m_stageMedians.clear();

        std::vector<std::optional<double>> centers;
        if (!HasStages())
            {
            const auto values = GetBaselineValues();
            if (std::ranges::any_of(values,
                                    [](const auto val) noexcept { return std::isfinite(val); }))
                {
                m_median = statistics::median(values);
                }
            centers.push_back(m_median);
            }
        else
            {
            // each stage has its own median
            const auto& values = GetPlottedValues();
            const auto& starts = GetStageStarts();
            m_stageMedians.reserve(starts.size());
            centers.reserve(starts.size());
            for (size_t stage = 0; stage < starts.size(); ++stage)
                {
                const size_t first{ starts[stage] };
                const size_t last{ (stage + 1 < starts.size()) ? starts[stage + 1] :
                                                                 values.size() };
                const std::vector<double> stageValues(
                    std::next(values.cbegin(), static_cast<ptrdiff_t>(first)),
                    std::next(values.cbegin(), static_cast<ptrdiff_t>(last)));

                std::optional<double> median;
                if (std::ranges::any_of(stageValues,
                                        [](const auto val) noexcept { return std::isfinite(val); }))
                    {
                    median = statistics::median(stageValues);
                    }
                m_stageMedians.push_back(median);
                centers.push_back(median);
                }
            // a single stage is the median of the whole chart
            if (m_stageMedians.size() == 1)
                {
                m_median = m_stageMedians.front();
                }
            }

        SetStageCenterLines(std::move(centers));
        UpdateMedianLine();
        }

    //----------------------------------------------------------------
    void RunChart::UpdateMedianLine()
        {
        RemoveMedianLine();

        // a chart with stages names its medians in the legend, not on the opposite axis
        if (!m_showMedianLine || HasStages() || !m_median.has_value())
            {
            return;
            }

        // this only places the median's label, because AddUnderlayObjects() draws the line
        AddReferenceLine(GraphItems::ReferenceLine(AxisType::LeftYAxis, m_median.value(),
                                                   MedianLabel(m_median), MedianLabelPen(),
                                                   ReferenceLabelPlacement::OppositeAxis));
        }
    } // namespace Wisteria::Graphs
