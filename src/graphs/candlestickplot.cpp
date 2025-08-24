///////////////////////////////////////////////////////////////////////////////
// Name:        candlestickplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "candlestickplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::CandlestickPlot, Wisteria::Graphs::Graph2D)

    using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void CandlestickPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                  const wxString& dateColumnName, const wxString& openColumnName,
                                  const wxString& highColumnName, const wxString& lowColumnName,
                                  const wxString& closeColumnName)
        {
        if (data == nullptr)
            {
            return;
            }

        GetSelectedIds().clear();

        auto dateColumn = data->GetDateColumn(dateColumnName);
        if (dateColumn == data->GetDateColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': date column not found for Candlestick plot."),
                                 dateColumnName)
                    .ToUTF8());
            }
        auto openColumn = data->GetContinuousColumn(openColumnName);
        if (openColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': opening column not found for Candlestick plot."),
                                 openColumnName)
                    .ToUTF8());
            }
        auto highColumn = data->GetContinuousColumn(highColumnName);
        if (highColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': high column not found for Candlestick plot."),
                                 highColumnName)
                    .ToUTF8());
            }
        auto lowColumn = data->GetContinuousColumn(lowColumnName);
        if (lowColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': low column not found for Candlestick plot."),
                                 lowColumnName)
                    .ToUTF8());
            }
        auto closeColumn = data->GetContinuousColumn(closeColumnName);
        if (closeColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': closing column not found for Candlestick plot."),
                                 closeColumnName)
                    .ToUTF8());
            }

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (!dateColumn->GetValue(i).IsValid() || std::isnan(openColumn->GetValue(i)) ||
                std::isnan(highColumn->GetValue(i)) || std::isnan(lowColumn->GetValue(i)) ||
                std::isnan(closeColumn->GetValue(i)))
                {
                continue;
                }
            m_ohlcs.push_back({ dateColumn->GetValue(i), openColumn->GetValue(i),
                                highColumn->GetValue(i), lowColumn->GetValue(i),
                                closeColumn->GetValue(i) });
            }
        Calculate(data, openColumnName, lowColumnName, highColumnName, closeColumnName);
        }

    //----------------------------------------------------------------
    void CandlestickPlot::Calculate(const std::shared_ptr<const Data::Dataset>& data,
                                    const wxString& openColumnName, const wxString& highColumnName,
                                    const wxString& lowColumnName, const wxString& closeColumnName)
        {
        if (m_ohlcs.empty() || data == nullptr)
            {
            return;
            }

        const wxDateTime firstDay =
            std::ranges::min_element(
                std::as_const(m_ohlcs),
                [](const auto& ohlc1, const auto& ohlc2)
                {
                    return (!ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? false :
                           (!ohlc1.m_date.IsValid() && ohlc2.m_date.IsValid())  ? false :
                           (ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid())  ? true :
                                                                                  ohlc1.m_date <
                                                                                     ohlc2.m_date;
                })
                ->m_date;
        const wxDateTime lastDay =
            std::ranges::max_element(
                std::as_const(m_ohlcs),
                [](const auto& ohlc1, const auto& ohlc2)
                {
                    return (!ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? false :
                           (!ohlc1.m_date.IsValid() && ohlc2.m_date.IsValid())  ? true :
                           (ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid())  ? false :
                                                                                  ohlc1.m_date <
                                                                                     ohlc2.m_date;
                })
                ->m_date;

        if (firstDay.IsValid() && lastDay.IsValid())
            {
            GetBottomXAxis().SetRange(firstDay, lastDay, DateInterval::Monthly,
                                      FiscalYear::USBusiness);
            }

        if (GetBottomXAxis().GetRangeDates().first.IsValid() &&
            GetBottomXAxis().GetRangeDates().second.IsValid())
            {
            m_debugDrawInfoLabel = wxString::Format(
                _DT(L"Date range: %s-%s"), GetBottomXAxis().GetRangeDates().first.FormatDate(),
                GetBottomXAxis().GetRangeDates().second.FormatDate());
            }

        const auto openMinMaxValues = data->GetContinuousMinMax(openColumnName, std::nullopt, 0);
        const auto lowMinMaxValues = data->GetContinuousMinMax(lowColumnName, std::nullopt, 0);
        const auto highMinMaxValues = data->GetContinuousMinMax(highColumnName, std::nullopt, 0);
        const auto closeMinMaxValues = data->GetContinuousMinMax(closeColumnName, std::nullopt, 0);

        GetLeftYAxis().SetRange(std::min({ openMinMaxValues.first, lowMinMaxValues.first,
                                           highMinMaxValues.first, closeMinMaxValues.first }),
                                std::max({ openMinMaxValues.second, lowMinMaxValues.second,
                                           highMinMaxValues.second, closeMinMaxValues.second }),
                                2);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [adjustedYStartCurrent, adjustedYEndCurrent] =
            adjust_intervals(yStartCurrent, yEndCurrent);

        GetLeftYAxis().SetRange(
            adjustedYStartCurrent, adjustedYEndCurrent,
            ((get_mantissa(adjustedYStartCurrent) == 0 && get_mantissa(adjustedYEndCurrent) == 0) ?
                 0 :
                 1),
            false);
        }

    //----------------------------------------------------------------
    void CandlestickPlot::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        const auto candleWidth = std::floor(safe_divide<double>(
            GetPlotAreaBoundingBox().GetWidth(), GetBottomXAxis().GetAxisPointsCount()));

        for (const auto& ohlc : m_ohlcs)
            {
            if (!ohlc.m_date.IsValid() || std::isnan(ohlc.m_open) || std::isnan(ohlc.m_high) ||
                std::isnan(ohlc.m_low) || std::isnan(ohlc.m_close))
                {
                wxFAIL_MSG(L"Invalid observation in CandlestickPlot::RecalcSizes()! "
                           "This item should have been filtered earlier. "
                           "Obs. will be ignored.");
                continue;
                }

            const wxString ohlcInfo = wxString::Format(
                _(L"Date: %s\n"
                  "Opening: %s\n"
                  "High : %s\n"
                  "Low : %s\n"
                  "Closing : %s"),
                ohlc.m_date.FormatDate(),
                wxNumberFormatter::ToString(ohlc.m_open, Settings::GetDefaultNumberFormat()),
                wxNumberFormatter::ToString(ohlc.m_high, Settings::GetDefaultNumberFormat()),
                wxNumberFormatter::ToString(ohlc.m_low, Settings::GetDefaultNumberFormat()),
                wxNumberFormatter::ToString(ohlc.m_close, Settings::GetDefaultNumberFormat()));

            wxPoint lowPt, hiPt;
            const auto datePos = GetBottomXAxis().FindDatePosition(ohlc.m_date);
            if (!datePos.has_value() ||
                !GetPhysicalCoordinates(datePos.value(), ohlc.m_low, lowPt) ||
                !GetPhysicalCoordinates(datePos.value(), ohlc.m_high, hiPt))
                {
                continue;
                }
            const auto wickX = lowPt.x;
            auto wick = std::make_unique<GraphItems::Lines>(
                wxPen((ohlc.m_open <= ohlc.m_close ?
                           ColorContrast::Shade(m_gainBrush.GetColour(), .2) :
                           ColorContrast::Shade(m_lossBrush.GetColour(), .2))),
                GetScaling());
            wick->AddLine(lowPt, hiPt);
            if (!datePos.has_value() ||
                !GetPhysicalCoordinates(datePos.value(), ohlc.m_open, lowPt) ||
                !GetPhysicalCoordinates(datePos.value(), ohlc.m_close, hiPt))
                {
                continue;
                }

            // make candles as wide as possible with the given area, with
            // 1 DIP padding on each side
            const auto candleSideWidth =
                std::max<wxCoord>(std::floor((candleWidth / 2) - ScaleToScreenAndCanvas(1)), 1);
            std::array<wxPoint, 4> points{ hiPt - wxPoint(candleSideWidth, 0),
                                           hiPt + wxPoint(candleSideWidth, 0),
                                           lowPt + wxPoint(candleSideWidth, 0),
                                           lowPt - wxPoint(candleSideWidth, 0) };

            wick->SetText(ohlcInfo);

            if (m_chartType == PlotType::Candlestick)
                {
                AddObject(std::move(wick));

                auto candle = std::make_unique<GraphItems::Polygon>(
                    GraphItemInfo(ohlcInfo).Brush(
                        (ohlc.m_open <= ohlc.m_close ? m_gainBrush : m_lossBrush)),
                    points);

                // if candle is really thin, then remove the outline so that
                // we can at least see the color
                if ((points[1].x - points[0].x) <= ScaleToScreenAndCanvas(1))
                    {
                    candle->GetPen() = wxNullPen;
                    }
                AddObject(std::move(candle));
                }
            else
                {
                points[0].x = wickX;
                points[2].x = wickX;
                wick->AddLine(points[0], points[1]);
                wick->AddLine(points[2], points[3]);
                AddObject(std::move(wick));
                }
            }
        }
    } // namespace Wisteria::Graphs
