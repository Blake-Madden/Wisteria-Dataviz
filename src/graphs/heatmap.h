/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_HEATMAP_H__
#define __WISTERIA_HEATMAP_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A discrete heat map, which is a grid-based plot which compares each value from a vector and
         maps them along a color scale (e.g., grayscale). Each cell shows an observation's
         value and its color represents that value's scale in comparison to the other
         observations.

         This is useful for reviewing densities of high and low values in a data series.
         The data can be visualized as a single series, or it can be grouped (refer to SetData()).

         %Data can either be plotted as a single series or split into groups.

         | Single Series   | Grouped |
         | :-------------- | :-------------------------------- |
         | @image html Heatmap.svg width=90% | @image html HeatmapGrouped.svg width=90% |

        @par %Data:
         This plot accepts a Data::Dataset where a continuous column is the data that is color mapped.
         Also, this data series can optionally be grouped by a categorical column from the dataset.
         Finally, an ID/label can optionally be assigned to each cell (corresponding to the data points)
         that is displayed when selected by the client.

         | NAME   | TEST_SCORE | WEEK   |
         | :--    | --:        | :--    |
         | Anna   | 87         | week 1 |
         | Anna   | 89         | week 2 |
         | Anna   | 91         | week 3 |
         | Anna   | 95         | week 4 |
         | Joe    | 37         | week 1 |
         | Joe    |            | week 2 |
         | Joe    |            | week 3 |
         | Joe    | 59         | week 4 |
         | Julie  | 91         | week 1 |
         | Julie  | 98         | week 2 |
         | Julie  | 95         | week 3 |
         | Julie  | 96         | week 4 |
         | Tina   | 95         | week 1 |
         | Tina   | 77         | week 2 |
         | Tina   | 91         | week 3 |
         | Tina   | 94         | week 4 |

         ...

         With the above dataset, the column `TEST_SCORE` will be the continuous variable,
         and `WEEK` can optionally be used as the label/ID when a cell is selected.
         If you wish to separate heat maps for each group, then `NAME` should be imported
         as the grouping variable.

         Note that the data is mapped exactly in the order that it appears in the data
         (i.e., nothing is sorted). Because this, the grouping column should be presorted
         and each group's values should be in the order that want them to be appear in the plot.

         @par Missing Data:
         - Missing data in the ID column will result in an empty selection label for the cell.
         - Missing data in the group column will be shown as an empty row label (for the group).
         - If the value is missing data, then that will be shown as a transparent cell with a red 'X' in the middle.\n
           With the above dataset, a row for the student `Joe` will only have two valid cells
           (for weeks 1 and 4), and two cells in the middle that are crossed out (for weeks 2 and 3).

         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog,
          // "canvas" is a scrolled window derived object
          // that will hold the plot
          auto canvas = new Wisteria::Canvas{ this };
          canvas->SetFixedObjectsGridSize(1, 2);
          auto testScoresData = std::make_shared<Data::Dataset>();
          try
            {
            testScoresData->ImportCSV(L"/home/daphne/data/Student Scores.csv",
                ImportInfo().
                ContinuousColumns({ L"test_score" }).
                IdColumn(L"Week").
                CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

          auto plot = std::make_shared<HeatMap>(canvas);
          // add a title to the plot
          plot->GetTitle().GetGraphItemInfo().Text(_(L"Test Scores")).
                ChildAlignment(RelativeAlignment::FlushLeft).
                Pen(wxNullPen).Padding(4, 0, 0, 4).
                Font(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)).
                     MakeBold().MakeLarger());

          // use group and put all of the students' heatmaps into one row
          plot->SetData(testScoresData, L"test_score", L"Name", 1);
          // say "Students" at the top instead of "Groups"
          plot->SetGroupHeaderPrefix(_(L"Students"));

          canvas->SetFixedObject(0, 0, plot);

          // customize the header of the legend and add it to the canvas
          auto legend{ plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, true) };
          legend->SetLine(0, _(L"Range of Scores"));
          canvas->SetFixedObject(0, 1, legend);
         @endcode
    */
    class HeatMap final : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param colors The color scheme to apply to the points.\n
                Leave as null to use black & white.
            @note For the color scheme, the first colors map to the lower values,
                last colors map to the higher values.\n
                The default color scale is white (low values) to black (high values),
                which creates a grayscale spectrum.*/
        explicit HeatMap(Wisteria::Canvas* canvas,
                         std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr);
        /** @brief Set the data across the heatmap.
            @param data The data.
            @param continuousColumnName The data column from the dataset to use for
                the heatmapping.
            @param groupColumnName The group column to split the data into
                (this is optional).
            @param groupColumnCount If grouping, the number of columns to split
                the sub-heatmaps into. Must be between 1-5 (and will be clamped otherwise),
                as more than 5 columns would make the boxes too small.
                Also, this parameter is ignored if @c grouping is `false`.
            @warning If grouping the data, the data must be sorted ahead of time
                (given that ordering is important in a heatmap anyway).
                It is assumed that it is sorted by the value that the caller
                wants to display left to right, but then also by group. These
                groups will be drawn top-to-bottom.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
            const wxString& continuousColumnName,
            std::optional<const wxString> groupColumnName = std::nullopt,
            std::optional<size_t> groupColumnCount = std::nullopt);

        /** @name Grouping Functions
            @brief Functions related to how grouped cells are displayed.
            @details These functions are only relevant if a grouping column was supplied.*/
        /// @{

        /// @returns The group headers' prefix shown above each column.
        /// @note This is only relevant if grouping is being used.
        [[nodiscard]] const wxString& GetGroupHeaderPrefix() const noexcept
            { return m_groupHeaderPrefix; }
        /** @brief Sets the prefix of the label shown above each column.
            @param prefix The group label.
            @note This is only relevant if grouping is being used.*/
        void SetGroupHeaderPrefix(const wxString& prefix)
            { m_groupHeaderPrefix = prefix; }
        /// @brief Whether to show group headers over each column.
        /// @param show @c true to show the column headers.
        /// @note This is only relevant if grouping is being used.
        void ShowGroupHeaders(const bool show) noexcept
            { m_showGroupHeaders = show; }
        /// @brief Whether group headers are shown over each column.
        /// @note This is only relevant if grouping is being used.
        /// @returns @c true if group column headers are being shown.
        [[nodiscard]] bool IsShowingGroupHeaders() const noexcept
            { return m_showGroupHeaders; }
        /// @}

        /// @brief Builds and returns a legend using the current colors spectrum.
        /// @details This can be then be managed by the parent canvas and
        ///     placed next to the plot.
        /// @param hint A hint about where the legend will be placed after construction.
        ///     This is used for defining the legend's padding, outlining,
        ///     canvas proportions, etc.
        /// @param includeHeader @c true to show the continuous column name as the header.
        /// @returns The legend for the plot.
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint,
            const bool includeHeader);
    private:
        void RecalcSizes(wxDC& dc) final;

        struct HeatCell
            {
            wxString m_valueLabel;
            wxString m_selectionLabel;
            wxColour m_color;
            Data::GroupIdType m_groupId{ 0 };
            };
        std::vector<std::vector<HeatCell>> m_matrix;
        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorSpectrum;
        std::vector<wxColour> m_reversedColorSpectrum; // used for the legend
        [[nodiscard]] const std::shared_ptr<const Data::Dataset>& GetData() const noexcept
            { return m_data; }
        std::shared_ptr<const Data::Dataset> m_data{ nullptr };
        std::pair<double, double> m_range{ 0,0 };
        wxString m_groupHeaderPrefix{ _(L"Groups") };
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;
        bool m_useGrouping{ false };
        bool m_showGroupHeaders{ true };
        size_t m_groupColumnCount{ 1 };
        };
    }

/** @}*/

#endif //__WISTERIA_HEATMAP_H__
