/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_CATEGORICAL_BARCHART_H__
#define __WISTERIA_CATEGORICAL_BARCHART_H__

#include "barchart.h"

namespace Wisteria::Graphs
    {
    /** @brief %Bar chart that aggregates the frequency (or summed values)
         of a categorical column's labels.
    
         Bars can either be plotted as a regular bar or split into (stacked) groups.
        
         | Regular         | Grouped
         | :-------------- | :--------------------------------
         | @image html CatagorizedBarChart.svg width=90% | @image html GroupedCatagorizedBarChart.svg width=90%

        @par Data:
         This plot accepts a Data::Dataset, where a categorical column is split into levels
         and aggregated. The aggregation can either be the frequency of observations or
         summed values from a corresponding continuous column.

         A grouping column can optionally be used to create separate blocks within the bars.

        @par Missing Data:
         - Missing data in the categorical column will be shown as an empty axis label.
         - Missing data in the group column will be shown as an empty legend label.
         - If summing a continuous column, then missing data will be ignored.
           (Listwise deletion of the observation.)
          
        @note If you want to create a bar chart that sums the counts of unique, discrete values
         from a continuous variable, then histograms offer this ability. Refer to the
         @c BinUniqueValues binning method in the Histogram documentation to learn more.
         
         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog, "canvas"
          // is a scrolled window derived object that will hold the box plot
          auto canvas = new Wisteria::Canvas(this);
          canvas->SetFixedObjectsGridSize(1, 1);
          auto mpgData = std::make_shared<Data::Dataset>();
          try
            {
            mpgData->ImportCSV(L"datasets/mpg.csv",
                ImportInfo().
                CategoricalColumns({
                    { L"manufacturer", CategoricalImportMethod::ReadAsStrings },
                    { L"model", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

          auto plot = std::make_shared<CategoricalBarChart>(subframe->m_canvas,
            std::make_shared<Colors::Schemes::Decade1980s>());

          plot->SetData(mpgData, L"manufacturer");
          // to group by model within the manufacturers
          // plot->SetData(mpgData, L"manufacturer", std::nullopt, L"model");

          canvas->SetFixedObject(0, 0, plot);
         @endcode
        */
    class CategoricalBarChart final : public Wisteria::Graphs::BarChart
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param colors The color scheme to apply to the points.
             Leave as null to use the default theme.*/
        explicit CategoricalBarChart(Wisteria::Canvas* canvas,
                           std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr) :
            Wisteria::Graphs::BarChart(canvas),
            m_data(nullptr),
            m_colorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme())
            {
            // categorical axis labels (especially longer ones) usually look
            // better with horizontal bars
            SetBarOrientation(Wisteria::Orientation::Horizontal);
            GetBarAxis().GetGridlinePen() = wxNullPen;
            GetBarAxis().GetGridlinePen() = wxNullPen;
            GetBarAxis().ShowOuterLabels(false);
            GetScalingAxis().GetGridlinePen() = wxNullPen;
            GetRightYAxis().Show(false);
            GetTopXAxis().Show(false);
            SetSortable(true);
            }

        /** @brief Sets the data.
            @param data The data to use for the histogram.
            @param categoricalColumnName The column from the dataset with the labels to aggregate.
            @param valueColumnName The column with values to sum for each category.
             If not used (@c std::nullopt), then the frequency of the observations will be used.
            @param groupColumnName The group column to split the bars into
             (this is optional).
            @param blDisplay Which type of labels to display for the bars.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& categoricalColumnName,
                     const std::optional<const wxString> valueColumnName = std::nullopt,
                     const std::optional<const wxString> groupColumnName = std::nullopt,
                     const BinLabelDisplay blDisplay = BinLabelDisplay::BinValue);

        /// @returns The number of groups found during the last call to SetData().
        ///  This is only relevant if using a grouping variable.
        [[nodiscard]] size_t GetGroupCount() const noexcept
            { return m_groupIds.size(); }

        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
             This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @param includeHeader `true` to show the grouping column name as the header.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint,
            const bool includeHeader);
    private:
        struct CatBarBlock
            {
            Data::GroupIdType m_bin{ 0 };
            Data::GroupIdType m_block{ 0 };
            [[nodiscard]] bool operator<(const CatBarBlock& that) const noexcept
                {
                if (m_bin != that.m_bin)
                    { return m_bin < that.m_bin; }
                return m_block < that.m_block;
                }
            };
        void Calculate();
        /// @returns The type of labels being shown on the bars.
        [[nodiscard]] BinLabelDisplay GetBinLabelDisplay() const noexcept
            { return m_binLabelDisplay; }
        /// @brief Get the color scheme used for the points.
        /// @returns The color scheme used for the points.
        [[nodiscard]] std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() noexcept
            { return m_colorScheme; }
        /// @private
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }

        std::shared_ptr<const Data::Dataset> m_data{ nullptr };
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_categoricalColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;

        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        uint8_t m_barOopacity{ wxALPHA_OPAQUE };
        BoxEffect m_barEffect{ BoxEffect::Solid };
        BinLabelDisplay m_binLabelDisplay{ BinLabelDisplay::BinValue };
        bool m_useGrouping{ false };
        bool m_useValueColumn{ false };
        std::set<Data::GroupIdType> m_groupIds;
        };
    }

/** @}*/

#endif //__WISTERIA_CATEGORICAL_BARCHART_H__
