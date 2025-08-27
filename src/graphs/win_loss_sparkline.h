/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WIN_LOSS_H
#define WISTERIA_WIN_LOSS_H

#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A series of lines showing the outcome of a team's season.

        @details Multiple series can be included, showing either different teams or multiple
         seasons for the same team.

        @par %Data:
         This plot accepts a Data::Dataset where a categorical column specifies the row labels
         (e.g., teams' names, or season labels) and continuous (boolean) columns specifies the
         outcomes of the games and whether they were home games.

         | SEASON   | WON   | SHUTOUT | HOMEGAME |
         | :--      | --:   | :--     |          |
         | 2022     | 1     | 1       | 1        |
         | 2022     | 0     | 0       | 1        |
         | 2022     | 1     | 0       | 1        |
         | 2022     | 1     | 0       | 0        |
         | 2022     | 0     | 0       | 0        |
         | 2022     | 0     | 0       | 1        |
         | 2022     | 1     | 0       | 1        |
         | 2022     | 1     | 0       | 1        |
         | 2022     | 0     | 0       | 0        |
         | 2022     | 1     | 0       | 0        |
         | 2022     | 1     | 0       | 0        |
         | 2022     | 1     | 0       | 1        |
         | 2023     | 1     | 0       | 1        |
         | 2023     | 0     | 0       | 1        |
         | 2023     | 1     | 1       | 0        |
         | 2023     | 1     | 0       | 0        |
         | 2023     | 0     | 0       | 1        |
         | 2023     | 1     | 0       | 1        |
         | 2023     | 0     | 0       | 1        |
         | 2023     | 1     | 0       | 0        |
         | 2023     | 0     | 0       | 0        |

         ...

         Note that for the continuous (boolean) columns, they should either contain @c 0
         (indicating @c false), @c 1 (indicating @c true), or be blank.
         A blank cell could indicate a game that was canceled, for example.
         Any other non-zero value will be considered @c true.

         @warning The data is mapped exactly in the order that it appears in the data
         (i.e., nothing is sorted). Because this, the grouping column should be presorted
         and each group's values should be in the order that want them to appear in the plot.

         @par Missing Data:
         - Missing data in the group column will be shown as an empty row label (for the group).
         - For any missing data in boolean columns, a blank space will be shown along series
           where the game would have been. (If it appears to be a canceled game midseason, then
           it will be X'ed out.)

         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog,
          // "canvas" is a scrolled window derived object
          // that will hold the plot
          auto canvas = new Wisteria::Canvas{ this };
          canvas->SetFixedObjectsGridSize(2, 1);
          auto volleyballData = std::make_shared<Data::Dataset>();
          try
            {
            volleyballData->ImportCSV(L"/home/daphne/data/volley-ball.csv",
                ImportInfo()
                    .ContinuousColumns({ L"WON", L"SHUTOUT", L"HOMEGAME" })
                    .CategoricalColumns({ { L"SEASON", CategoricalImportMethod::ReadAsStrings } }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

          auto plot = std::make_shared<WinLossSparkline>(canvas);

          plot->SetData(volleyballData, L"WON", L"SHUTOUT", L"HOMEGAME", L"SEASON");

          canvas->SetFixedObject(0, 0, plot);

          // add a descriptive legend
          auto legend{ plot->CreateLegend(
            LegendOptions().
                PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph)) };
          canvas->SetFixedObject(1, 0, legend);
         @endcode

         @par Citations:
            Tufte, Edward Rolfe. *Beautiful Evidence*. Graphics Press, 2019. 
    */
    class WinLossSparkline final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WinLossSparkline);
        WinLossSparkline() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.*/
        explicit WinLossSparkline(Canvas* canvas);

        /** @brief Sets the data.
            @param data The data.
            @param wonColumnName A boolean column indicating whether the
                game was won or lost.
            @param shutoutColumnName A boolean column indicating whether one team was
                held scoreless.
            @param homeGameColumnName A boolean column indicating whether the game was played
                at the home stadium.
            @param groupColumnName The group column to split the data by row.\n
                This will normally be a team's seasons, or multiple teams.
            @warning Regarding the group column, the data must be sorted ahead of time
                (given that ordering is important in a sparkline anyway).
            @note Boolean columns should be continuous columns that contain
                @c 0, @c 1, or be empty. (Non-zero values will be read as @c true.)
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& wonColumnName, const wxString& shutoutColumnName,
                     const wxString& homeGameColumnName, const wxString& groupColumnName);

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
        wxColour m_winColor;
        wxColour m_lossColor;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WIN_LOSS_H
