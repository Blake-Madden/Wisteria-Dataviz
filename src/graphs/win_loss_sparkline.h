/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WINLOSS_H
#define WISTERIA_WINLOSS_H

#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    class WinLossSparkline final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WinLossSparkline);
        WinLossSparkline() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param colors The color scheme to apply to the points.\n
                Leave as @c nullptr to use black & white.
            @note For the color scheme, the first colors map to the lower values,
                last colors map to the higher values.\n
                The default color scale is white (low values) to black (high values),
                which creates a grayscale spectrum.*/
        explicit WinLossSparkline(
            Wisteria::Canvas* canvas,
                         const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);
        /** @brief Sets the data.
            @param data The data.
            @param wonColumnName A boolean column indicating whether a game was won or lost.
            @param shutoutColumnName
            @param groupColumnName The group column to split the data by row.\n
                This will normally be a team's seasons, or multiple teams.
            @warning Regarding the group column, the data must be sorted ahead of time
                (given that ordering is important in a sparkline anyway).
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& wonColumnName,
                     const wxString& shutoutColumnName,
                     const wxString& homeGameColumnName,
                     const wxString& groupColumnName);

        /** @brief Builds and returns a legend.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

      private:
        void RecalcSizes(wxDC& dc) final;

        struct WinLossCell
            {
            bool m_valid{ false };
            bool m_won{ true };
            bool m_shutout{ false };
            bool m_homeGame{ true };
            };

        struct WinLossRow
            {
            wxString m_groupLabel;
            wxString m_overallRecordLabel;
            wxString m_homeRecordLabel;
            wxString m_roadRecordLabel;
            wxString m_pctLabel;
            };

        std::vector<std::pair<WinLossRow, std::vector<WinLossCell>>> m_matrix;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WINLOSS_H
