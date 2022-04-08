#include "candlestickplot.h"

using namespace Wisteria::GraphItems;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void CandlestickPlot::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& dateColumnName,
        const wxString& openColumnName, const wxString& highColumnName,
        const wxString& lowColumnName, const wxString& closeColumnName)
        {
        if (data == nullptr)
            { return; }
    
        auto dateColumn = data->GetDateColumn(dateColumnName);
        if (dateColumn == data->GetDateColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                L"'%s': date column not found for Candlestick plot", dateColumnName));
            return;
            }
        auto openColumn = data->GetContinuousColumn(openColumnName);
        if (openColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                L"'%s': opening column not found for Candlestick plot", openColumnName));
            return;
            }
        auto highColumn = data->GetContinuousColumn(highColumnName);
        if (highColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                L"'%s': high column not found for Candlestick plot", highColumnName));
            return;
            }
        auto lowColumn = data->GetContinuousColumn(lowColumnName);
        if (lowColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                L"'%s': low column not found for Candlestick plot", lowColumnName));
            return;
            }
        auto closeColumn = data->GetContinuousColumn(closeColumnName);
        if (closeColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                L"'%s': closing column not found for Candlestick plot", closeColumnName));
            return;
            }

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            m_ohlcs.push_back({ dateColumn->GetValue(i), openColumn->GetValue(i),
                                highColumn->GetValue(i), lowColumn->GetValue(i),
                                closeColumn->GetValue(i) });
            }
        Calculate(data, openColumnName, lowColumnName, highColumnName, closeColumnName);
        UpdateCanvasForPoints();
        }

    //----------------------------------------------------------------
    void CandlestickPlot::Calculate(const std::shared_ptr<const Data::Dataset>& data,
        const wxString& openColumnName, const wxString& highColumnName,
        const wxString& lowColumnName, const wxString& closeColumnName)
        {
        if (!m_ohlcs.size() || data == nullptr)
            { return; }

        wxDateTime firstDay = std::min_element(m_ohlcs.cbegin(), m_ohlcs.cend(),
            [](const auto& ohlc1, const auto& ohlc2)
                {
                return (!ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? false :
                    (!ohlc1.m_date.IsValid() && ohlc2.m_date.IsValid()) ? false :
                    (ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? true :
                    ohlc1.m_date < ohlc2.m_date;
                })->m_date;
        wxDateTime lastDay = std::max_element(m_ohlcs.cbegin(), m_ohlcs.cend(),
            [](const auto& ohlc1, const auto& ohlc2)
                {
                return (!ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? false :
                    (!ohlc1.m_date.IsValid() && ohlc2.m_date.IsValid()) ? true :
                    (ohlc1.m_date.IsValid() && !ohlc2.m_date.IsValid()) ? false :
                    ohlc1.m_date < ohlc2.m_date;
                })->m_date;

        if (firstDay.IsValid() && lastDay.IsValid())
            {
            GetBottomXAxis().SetRange(firstDay, lastDay,
                DateInterval::Monthly, FiscalYear::USBusiness);
            }

        if (GetBottomXAxis().GetRangeDates().first.IsValid() &&
            GetBottomXAxis().GetRangeDates().second.IsValid())
            {
            m_debugDrawInfoLabel = wxString::Format(_DT(L"Date range: %s-%s"),
                GetBottomXAxis().GetRangeDates().first.FormatDate(),
                GetBottomXAxis().GetRangeDates().second.FormatDate());
            }

        const auto openMinMaxValues = data->GetContinuousMinMax(openColumnName, std::nullopt, 0);
        const auto lowMinMaxValues = data->GetContinuousMinMax(lowColumnName, std::nullopt, 0);
        const auto highMinMaxValues = data->GetContinuousMinMax(highColumnName, std::nullopt, 0);
        const auto closeMinMaxValues = data->GetContinuousMinMax(closeColumnName, std::nullopt, 0);

        GetLeftYAxis().SetRange(
            std::min({ openMinMaxValues.first, lowMinMaxValues.first,
                       highMinMaxValues.first, closeMinMaxValues.first}),
            std::max({ openMinMaxValues.second, lowMinMaxValues.second,
                       highMinMaxValues.second, closeMinMaxValues.second }),
            2);

        const auto [yStartCurrent, yEndCurrent] = GetLeftYAxis().GetRange();
        const auto [adjustedYStartCurrent, adjustedYEndCurrent] =
            adjust_intervals(yStartCurrent, yEndCurrent);

        GetLeftYAxis().SetRange(
            adjustedYStartCurrent,
            adjustedYEndCurrent,
            ((get_mantissa(adjustedYStartCurrent) == 0 &&
              get_mantissa(adjustedYEndCurrent) == 0) ? 0 : 1),
            false);
        }

    //----------------------------------------------------------------
    void CandlestickPlot::RecalcSizes()
        {
        Graph2D::RecalcSizes();

        const auto candleWidth = std::floor(safe_divide<double>(
            GetPlotAreaBoundingBox().GetWidth(), GetBottomXAxis().GetAxisPointsCount()));

        for (const auto& ohlc : m_ohlcs)
            {
            wxString ohlcInfo = wxString::Format(
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
            const auto datePos = GetBottomXAxis().GetPointFromDate(ohlc.m_date);
            if (!datePos.has_value() ||
                !GetPhyscialCoordinates(datePos.value(), ohlc.m_low, lowPt) ||
                !GetPhyscialCoordinates(datePos.value(), ohlc.m_high, hiPt))
                { continue; }
            const auto wickX = lowPt.x;
            auto wick = std::make_shared<GraphItems::Lines>(
                wxPen((m_chartType == ChartType::Ohlc) ?
                  (ohlc.m_open <= ohlc.m_close ? m_gainBrush.GetColour() :
                   m_lossBrush.GetColour()) :
                  *wxBLACK), GetScaling());
            wick->AddLine(lowPt, hiPt);
            if (!datePos.has_value() ||
                !GetPhyscialCoordinates(datePos.value(), ohlc.m_open, lowPt) ||
                !GetPhyscialCoordinates(datePos.value(), ohlc.m_close, hiPt))
                { continue; }

            // make candles as wide as possible with the given area, which
            // 1 DIP padding on each side
            const auto candleSideWidth = std::max<wxCoord>(
                std::floor((candleWidth / 2) - ScaleToScreenAndCanvas(1)), 1);
            std::array<wxPoint, 4> points{
                hiPt - wxPoint(candleSideWidth,0),
                hiPt + wxPoint(candleSideWidth,0),
                lowPt + wxPoint(candleSideWidth,0),
                lowPt - wxPoint(candleSideWidth,0) };

            wick->SetText(ohlcInfo);

            if (m_chartType == ChartType::Candlestick)
                {
                AddObject(wick);

                auto candle = std::make_shared<GraphItems::Polygon>(
                    GraphItemInfo(ohlcInfo).
                    Brush((ohlc.m_open <= ohlc.m_close ? m_gainBrush : m_lossBrush)),
                    &points[0], std::size(points));
                
                // if candle is really thin, then remove the outline so that
                // we can at least see the color
                if ((points[1].x - points[0].x) <= ScaleToScreenAndCanvas(1))
                    { candle->GetPen() = wxNullPen; }
                AddObject(candle);
                }
            else
                {
                points[0].x = wickX;
                points[2].x = wickX;
                wick->AddLine(points[0], points[1]);
                wick->AddLine(points[2], points[3]);
                AddObject(wick);
                }
            }
        }
    }
