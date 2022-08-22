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
         - If summing a continuous column, then missing data will be ignored (listwise deletion).
          
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
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& categoricalColumnName,
                     const std::optional<const wxString> valueColumnName = std::nullopt,
                     const std::optional<const wxString> groupColumnName = std::nullopt,
                     const BinLabelDisplay blDisplay = BinLabelDisplay::BinValue);

        /// @returns The number of groups found during the last call to SetData().
        ///  This is only relevant if using a grouping variable.
        [[nodiscard]] size_t GetGroupCount() const noexcept
            { return m_groupIds.size(); }

        /** @brief Builds and returns a legend.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendOptions& options) final;

        /// @private
        [[deprecated("Use version that takes a LegendOptions parameter.")]]
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint,
            const bool includeHeader)
            {
            return CreateLegend(
                LegendOptions().IncludeHeader(includeHeader).PlacementHint(hint));
            }
    private:
        struct CatBarBlock
            {
            // group ID in the main categorical column
            Data::GroupIdType m_bin{ 0 };
            // 0-based index into the color scheme
            // (based on the alphabetically order of the group label from
            //  the secondary group column)
            size_t m_schemeIndex{ 0 };
            // the name of the group for a subblock in a bar (from the secondary group column)
            wxString m_groupName;
            // sorts by group ID from the primary categorical column, or by the
            // subgroup label (if grouping is in use) alphabetically
            [[nodiscard]] bool operator<(const CatBarBlock& that) const noexcept
                {
                if (m_bin != that.m_bin)
                    { return m_bin < that.m_bin; }
                // if in the same bar, then compare by label alphabetically
                return m_groupName.CmpNoCase(that.m_groupName) < 0;
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
        // cat ID and string order
        std::map<Data::GroupIdType, size_t> m_groupIds;
        };
    }

/** @}*/

#endif //__WISTERIA_CATEGORICAL_BARCHART_H__
