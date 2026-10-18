///////////////////////////////////////////////////////////////////////////////
// Name:        runchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "runchart.h"
#include "../math/statistics.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::RunChart, Wisteria::Graphs::SpcChart)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    RunChart::RunChart(Canvas * canvas,
                       const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
                       const std::shared_ptr<Icons::Schemes::IconScheme>& shapes /*= nullptr*/,
                       const std::shared_ptr<LineStyleScheme>& linePenStyles /*= nullptr*/)
        : SpcChart(canvas, colors, shapes, linePenStyles)
        {
        }

    //----------------------------------------------------------------
    wxPen RunChart::MedianLinePen()
        {
        return wxPen(Colors::ColorBrewer::GetColor(Colors::Color::TractorRed), 2,
                     wxPenStyle::wxPENSTYLE_LONG_DASH);
        }

    //----------------------------------------------------------------
    wxString RunChart::MedianLabelPrefix() { return _(L"Median: "); }

    //----------------------------------------------------------------
    bool RunChart::IsMedianLine(const GraphItems::ReferenceLine& refLine) const
        {
        const wxPen medianPen{ MedianLinePen() };
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
                      [this](const auto& refLine) { return IsMedianLine(refLine); });
        }

    //----------------------------------------------------------------
    void RunChart::OnDataSet()
        {
        m_median.reset();
        const auto& values = GetPlottedValues();
        if (std::ranges::any_of(values, [](const auto val) noexcept { return std::isfinite(val); }))
            {
            m_median = statistics::median(values);
            }
        UpdateMedianLine();
        }

    //----------------------------------------------------------------
    void RunChart::UpdateMedianLine()
        {
        RemoveMedianLine();

        if (!m_showMedianLine || !m_median.has_value())
            {
            return;
            }

        const wxString medianLabel{ MedianLabelPrefix() +
                                    wxNumberFormatter::ToString(
                                        m_median.value(), 4,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes) };
        AddReferenceLine(GraphItems::ReferenceLine(AxisType::LeftYAxis, m_median.value(),
                                                   medianLabel, MedianLinePen(),
                                                   ReferenceLabelPlacement::OppositeAxis));
        }
    } // namespace Wisteria::Graphs
