/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef CANDLESTICK_PLOT_H
#define CANDLESTICK_PLOT_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Time-based plot which shows a commodity or stock's daily price over a
         given time period.

         Each day will show the commodity's opening and closing price
         (the candle or left/right hinges), as well as the high and low price (the line).

         %Data can either be displayed with candlesticks or OHLC hinges.

         @image html CandlestickPlot.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where a date column is the days,
         and four continuous columns are supplied for the opening, high, low, and closing values.

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
            silverFuturesData->ImportCSV(L"/home/daphne/data/Silver Futures.csv",
                ImportInfo().
                ContinuousColumns({ L"Open", L"High", L"Low", L"Close/Last" }).
                DateColumns({ { L"Date" } }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

          auto candlestickPlot = std::make_shared<CandlestickPlot>(canvas);
          // Plot's left axis will start at zero by default so that the scale
          // isn't misleading; you can, however, turn that off like this
          // to better see the daily activity.
          // This should be done before calling SetData() so that it bases
          // axis range on the data.
          candlestickPlot->GetLeftYAxis().StartAtZero(false);

          // Uncomment this to fit the entire year onto the canvas
          // so that there isn't a scrollbar.
          // candlestickPlot->SetPointsPerDefaultCanvasSize(365);

          candlestickPlot->SetData(silverFuturesData,
            L"Date", L"Open", L"High", L"Low", L"Close/Last");

          candlestickPlot->GetTitle().SetText(_(L"Silver COMEX 2021 Trend"));

          candlestickPlot->SetCanvasMargins(5, 5, 5, 5);
          canvas->SetFixedObject(0, 0, candlestickPlot);

         @endcode
    */
    class CandlestickPlot : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(CandlestickPlot);
        CandlestickPlot() = default;

      public:
        /// @brief How to display the gains and losses
        enum class PlotType
            {
            Candlestick, /*!< Display gains and losses as candles.*/
            Ohlc         /*!< Display gains and losses as protruding lines.*/
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.*/
        explicit CandlestickPlot(Canvas* canvas) : Graph2D(canvas)
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
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& dateColumnName, const wxString& openColumnName,
                     const wxString& highColumnName, const wxString& lowColumnName,
                     const wxString& closeColumnName);

        /// @name Display Functions
        /// @brief Functions relating to the display of the plot.
        /// @{

        /** @brief Sets whether this is an OHLC plot or candlestick plot.
            @param type The plot type to set this to.*/
        void SetPlotType(const PlotType& type) { m_chartType = type; }

        /// @brief Gets/sets the brush used to paint days that saw a loss.
        /// @returns The brush used to paint days that saw a loss.
        [[nodiscard]]
        wxBrush& GetLossBrush() noexcept
            {
            return m_lossBrush;
            }

        /// @brief Gets/sets the brush used to paint days that saw a gain.
        /// @returns The brush used to paint days that saw a gain.
        [[nodiscard]]
        wxBrush& GetGainBrush() noexcept
            {
            return m_gainBrush;
            }

        /// @}

        /// @private
        [[deprecated("Candlestick plot does not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            wxFAIL_MSG(L"Candlestick plot does not support legends.");
            return nullptr;
            }

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

        wxBrush m_lossBrush{ *wxRED };
        wxBrush m_gainBrush{ *wxGREEN };

        std::vector<Ohlc> m_ohlcs;
        PlotType m_chartType{ PlotType::Candlestick };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // CANDLESTICK_PLOT_H
