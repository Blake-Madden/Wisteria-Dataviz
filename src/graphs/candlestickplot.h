/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __CANDLESTICK_PLOT_H__
#define __CANDLESTICK_PLOT_H__

#include "graph2d.h"
#include "../base/colorbrewer.h"

namespace Wisteria::Graphs
    {
    /** @brief Time-based plot which shows a commodity or stock's daily price over a given time period.

         Each day will show the commodity's opening and closing price (the candle or left/right hinges),
         as well as the high and low price (the line).

         %Data can either be displayed with candlesticks or OHLC hinges.

         @image html CandlestickPlot.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where a date column is the days, and four continuous columns
         are supplied for the opening, high, low, and closing values.

         | Date       | Close  | High   | Low    | Open  | 
         | :--        | --:    | --:    | --:    | --:   |
         | 12/31/2021 | 23.352 | 23.095 | 23.39  | 23.07 |
         | 12/30/2021 | 23.06  | 22.87  | 23.155 | 22.63 |
         | 12/29/2021 | 22.858 | 23.05  | 23.185 | 22.6  |

         ...

         @par Missing Data:
          - Any missing data in an observation will result in listwise deletion.

         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog,
          // "canvas" is a scrolled window derived object
          // that will hold the plot
          auto canvas = new Wisteria::Canvas{ this };
          canvas->SetFixedObjectsGridSize(1, 1);

          auto silverFuturesData = std::make_shared<Data::Dataset>();
          try
            {
            silverFuturesData->ImportCSV(L"datasets/Silver Futures.csv",
                ImportInfo().
                ContinuousColumns({ L"Open", L"High", L"Low", L"Close/Last" }).
                DateColumns({ { L"Date" } }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

          auto candlestickChart = std::make_shared<CandlestickPlot>(canvas);
          // Chart's left axis will start at zero by default so that the scale
          // isn't misleading; you can, however, turn that off like this
          // to better see the daily activity.
          // This should be done before calling SetData() so that it bases
          // axis range on the data.
          candlestickChart->GetLeftYAxis().StartAtZero(false);

          // Uncomment this to fit the entire year onto the canvas
          // so that there isn't a scrollbar.
          // candlestickChart->SetPointsPerDefaultCanvasSize(365);

          candlestickChart->SetData(silverFuturesData,
            L"Date", L"Open", L"High", L"Low", L"Close/Last");

          candlestickChart->GetTitle().SetText(_(L"Silver COMEX 2021 Trend"));

          candlestickChart->SetCanvasMargins(5, 5, 5, 5);
          canvas->SetFixedObject(0, 0, candlestickChart);
          
         @endcode
    */
    class CandlestickPlot : public Graph2D
        {
    public:
        /// @brief How to display the gains and losses
        enum class ChartType
            {
            Candlestick, /*!< Display gains and losses as candles.*/
            Ohlc         /*!< Display gains and losses as protruding lines.*/
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the line plot on.*/
        explicit CandlestickPlot(Canvas* canvas) :
            Graph2D(canvas)
            {
            GetBottomXAxis().GetGridlinePen() = wxNullPen;
            GetLeftYAxis().StartAtZero(true);
            }

        /** @brief Sets the data.
            @param data The data to use for the plot.
            @param dateColumnName The column containing the date.
            @param openColumnName The column containing the opening price.
            @param lowColumnName The column containing the lowest price during the day.
            @param highColumnName The column containing the highest price during the day.
            @param closeColumnName The column containing the closing price.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& dateColumnName,
                     const wxString& openColumnName, const wxString& highColumnName,
                     const wxString& lowColumnName, const wxString& closeColumnName);

        /// @name Display Functions
        /// @brief Functions relating to the display of the plot.
        /// @{

        /** @brief Sets whether this is an OHLC chart or candlestick chart.
            @param type The chart type to set this to.*/
        void SetChartType(const ChartType& type)
            { m_chartType = type; }
        /// @brief Gets/sets the brush used to paint days that saw a loss.
        /// @returns The brush used to paint days that saw a loss.
        [[nodiscard]] wxBrush& GetLossBrush() noexcept
            { return m_lossBrush; }
        /// @brief Gets/sets the brush used to paint days that saw a gain.
        /// @returns The brush used to paint days that saw a gain.
        [[nodiscard]] wxBrush& GetGainBrush() noexcept
            { return m_gainBrush; }
        /// @}

        /// @name Layout Functions
        /// @brief Functions relating to layout of the plot.
        /// @{

        /// @brief Gets the maximum number of days displayed before the parent canvas
        ///  is forced to be made wider. (Which will make this plot easier to read).
        /// @returns The most days that can be plotted before the parent canvas will be widened.
        [[nodiscard]] size_t GetPointsPerDefaultCanvasSize() const noexcept
            { return m_pointsPerDefaultCanvasSize; }
        /** @brief Sets the maximum number of days displayed before the parent canvas is
             forced to be made wider.
            @details Adjusting this is useful for when you have a large number of days and the
             display looks too condensed. Increasing this value will widen the plot, allowing for
             more space to spread the points out. The default is 100 days.
            @param pointsPerDefaultCanvasSize The number points to display before requiring the canvas
             to be made wider.*/
        void SetPointsPerDefaultCanvasSize(const size_t pointsPerDefaultCanvasSize)
            {
            m_pointsPerDefaultCanvasSize = pointsPerDefaultCanvasSize;
            UpdateCanvasForPoints();
            }
        /// @}
    private:
        struct Ohlc
            {
            wxDateTime m_date;
            double m_open{ 0 };
            double m_high{ 0 };
            double m_low{ 0 };
            double m_close{ 0 };
            };

        void Calculate(const std::shared_ptr<const Data::Dataset>& data,
            const wxString& openColumnName, const wxString& highColumnName,
            const wxString& lowColumnName, const wxString& closeColumnName);
        /// @brief Recalculates the size of embedded objects on the plot.
        void RecalcSizes(wxDC& dc) final;

        void UpdateCanvasForPoints()
            {
            if (m_ohlcs.size() > GetPointsPerDefaultCanvasSize())
                {
                GetCanvas()->SetCanvasMinWidthDIPs(GetCanvas()->GetDefaultCanvasWidthDIPs() *
                    std::ceil(safe_divide<double>(m_ohlcs.size(),
                                                  GetPointsPerDefaultCanvasSize())));
                }
            }

        wxBrush m_lossBrush{ *wxRED };
        wxBrush m_gainBrush{ *wxGREEN };

        std::vector<Ohlc> m_ohlcs;
        size_t m_pointsPerDefaultCanvasSize{ 100 };
        ChartType m_chartType{ ChartType::Candlestick };
        };
    }

/** @}*/

#endif //__CANDLESTICK_PLOT_H__
