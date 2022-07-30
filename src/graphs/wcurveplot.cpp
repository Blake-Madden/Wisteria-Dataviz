///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wcurveplot.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Colors::Schemes;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WCurvePlot::WCurvePlot(Canvas* canvas,
            std::shared_ptr<ColorScheme> colors /*= nullptr*/,
            std::shared_ptr<IconScheme> shapes /*= nullptr*/,
            std::shared_ptr<LineStyleScheme> linePenStyles /*= nullptr*/) :
            LinePlot(canvas, colors,
                shapes != nullptr ? shapes : std::make_shared<IconScheme>(IconScheme{ IconShape::BlankIcon }),
                linePenStyles != nullptr ? linePenStyles :
                    std::make_shared<LineStyleScheme>(LineStyleScheme{
                        { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Arrows },
                        { wxPenStyle::wxPENSTYLE_LONG_DASH, LineStyle::Arrows },
                        { wxPenStyle::wxPENSTYLE_DOT, LineStyle::Arrows }
                        }))
        {
        GetBottomXAxis().SetCapStyle(AxisCapStyle::Arrow);
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        GetLeftYAxis().SetCapStyle(AxisCapStyle::Arrow);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);

        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void WCurvePlot::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& yColumnName,
        const wxString& xColumnName,
        std::optional<const wxString> groupColumnName)
        {
         if (data == nullptr)
            { return; }
        if (!groupColumnName.has_value())
            {
            throw std::runtime_error(_(L"Group column required for W-curve plot.").ToUTF8());
            return;
            }
        LinePlot::SetData(data, yColumnName, xColumnName, groupColumnName);

        // force the X axes to use neat integers
        const auto [axisMin, axisMax] = GetBottomXAxis().GetRange();
        GetBottomXAxis().SetRange(previous_interval(axisMin, 1), next_interval(axisMax, 1), 0, 1, 1);

        GetTopXAxis().CopySettings(GetBottomXAxis());
        GetTopXAxis().SetFontBackgroundColor(ColorBrewer::GetColor(Color::Black));
        GetTopXAxis().SetFontColor(ColorBrewer::GetColor(Color::White));
        GetTopXAxis().SetTextAlignment(TextAlignment::FlushLeft);
        GetTopXAxis().SetParallelLabelAlignment(RelativeAlignment::FlushLeft);
        GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetTopXAxis().GetAxisLinePen() = wxNullPen;

        GetBottomXAxis().GetTitle().SetText(_(L"TIME"));
        GetLeftYAxis().GetTitle().SetText(_(L"Level of control, satisfaction, and effectiveness"));

        ResetTimeLabels();
        }

    //----------------------------------------------------------------
    void WCurvePlot::ResetTimeLabels()
        {
        GetTopXAxis().ClearCustomLabels();
        if (GetData()->GetRowCount() == 0)
            { return; }

        auto [topRangeStart, topRangeEnd] = GetTopXAxis().GetRange();
        const auto maxXValue = GetXMinMax().second;
        // if last datum collected is at the edge of the range, then add an extra label
        if (maxXValue == topRangeEnd)
            { ++topRangeEnd; }
        for (size_t i = topRangeStart; i < topRangeEnd; i += GetTopXAxis().GetInterval())
            { GetTopXAxis().SetCustomLabel(i, Label(FormatTimeLabel(i))); }
        }

    //----------------------------------------------------------------
    wxString WCurvePlot::FormatTimeLabel(const uint8_t step) const
        {
        switch (step)
            {
        case 1:
            return wxString::Format(_(L"First %s"), m_timeLabel);
            break;
        case 2:
            return wxString::Format(_(L"Second %s"), m_timeLabel);
            break;
        case 3:
            return wxString::Format(_(L"Third %s"), m_timeLabel);
            break;
        case 4:
            return wxString::Format(_(L"Fourth %s"), m_timeLabel);
            break;
        case 5:
            return wxString::Format(_(L"Fifth %s"), m_timeLabel);
            break;
        case 6:
            return wxString::Format(_(L"Sixth %s"), m_timeLabel);
            break;
        case 7:
            return wxString::Format(_(L"Seventh %s"), m_timeLabel);
            break;
        case 8:
            return wxString::Format(_(L"Eighth %s"), m_timeLabel);
            break;
        case 9:
            return wxString::Format(_(L"Ninth %s"), m_timeLabel);
            break;
        case 10:
            return wxString::Format(_(L"Tenth %s"), m_timeLabel);
            break;
        default:
            return L"";
            }
        }
    }
