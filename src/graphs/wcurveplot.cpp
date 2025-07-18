///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wcurveplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WCurvePlot, Wisteria::Graphs::LinePlot)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WCurvePlot::WCurvePlot(
        Canvas * canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Icons::Schemes::IconScheme>& shapes /*= nullptr*/,
        const std::shared_ptr<LineStyleScheme>& linePenStyles /*= nullptr*/)
        : LinePlot(canvas, colors,
                   shapes != nullptr ? shapes :
                                       std::make_unique<Icons::Schemes::IconScheme>(
                                           Icons::Schemes::IconScheme{ Icons::IconShape::Blank }),
                   linePenStyles != nullptr ?
                       linePenStyles :
                       std::make_unique<LineStyleScheme>(
                           LineStyleScheme{ { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Arrows },
                                            { wxPenStyle::wxPENSTYLE_LONG_DASH, LineStyle::Arrows },
                                            { wxPenStyle::wxPENSTYLE_DOT, LineStyle::Arrows } }))
        {
        GetBottomXAxis().SetCapStyle(AxisCapStyle::Arrow);
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        GetLeftYAxis().SetCapStyle(AxisCapStyle::Arrow);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void WCurvePlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& yColumnName, const wxString& xColumnName,
                             const std::optional<wxString>& groupColumnName)
        {
        if (data == nullptr)
            {
            return;
            }
        if (!groupColumnName.has_value())
            {
            throw std::runtime_error(_(L"Group column required for W-curve plot.").ToUTF8());
            }
        LinePlot::SetData(data, yColumnName, xColumnName, groupColumnName);

        // force the X axes to use neat integers
        const auto [axisMin, axisMax] = GetBottomXAxis().GetRange();
        GetBottomXAxis().SetRange(previous_interval(axisMin, 1), next_interval(axisMax, 1), 0, 1,
                                  1);

        GetTopXAxis().CopySettings(GetBottomXAxis());
        GetTopXAxis().SetFontBackgroundColor(Colors::ColorBrewer::GetColor(Colors::Color::Black));
        GetTopXAxis().SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::White));
        GetTopXAxis().SetTextAlignment(TextAlignment::FlushLeft);
        GetTopXAxis().SetParallelLabelAlignment(RelativeAlignment::FlushLeft);
        GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetTopXAxis().GetAxisLinePen() = wxNullPen;

        GetBottomXAxis().GetTitle().SetText(
            /* TRANSLATORS: OK to translate as "Time".
               This is uppercased only because that's how it appears in the original article.*/
            _(L"TIME"));
        GetLeftYAxis().GetTitle().SetText(_(L"Level of control, satisfaction, and effectiveness"));

        ResetTimeLabels();
        }

    //----------------------------------------------------------------
    void WCurvePlot::ResetTimeLabels()
        {
        GetTopXAxis().ClearCustomLabels();
        if (GetDataset()->GetRowCount() == 0)
            {
            return;
            }

        auto [topRangeStart, topRangeEnd] = GetTopXAxis().GetRange();
        // if last datum collected is at the edge of the range, then add an extra label
        if (const auto maxXValue = GetXMinMax().second; maxXValue == topRangeEnd)
            {
            ++topRangeEnd;
            }
        for (size_t i = topRangeStart; i < topRangeEnd; i += GetTopXAxis().GetInterval())
            {
            GetTopXAxis().SetCustomLabel(i, GraphItems::Label(FormatTimeLabel(i)));
            }
        }

    //----------------------------------------------------------------
    wxString WCurvePlot::FormatTimeLabel(const uint8_t step) const
        {
        switch (step)
            {
        case 1:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"First %s"), m_timeLabel);
            break;
        case 2:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Second %s"), m_timeLabel);
            break;
        case 3:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Third %s"), m_timeLabel);
            break;
        case 4:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Fourth %s"), m_timeLabel);
            break;
        case 5:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Fifth %s"), m_timeLabel);
            break;
        case 6:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Sixth %s"), m_timeLabel);
            break;
        case 7:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Seventh %s"), m_timeLabel);
            break;
        case 8:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Eighth %s"), m_timeLabel);
            break;
        case 9:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Ninth %s"), m_timeLabel);
            break;
        case 10:
            return wxString::Format( // TRANSLATORS: %s is time interval (e.g., semester)
                _(L"Tenth %s"), m_timeLabel);
            break;
        default:
            return wxString{};
            }
        }
    } // namespace Wisteria::Graphs
