/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CATEGORICAL_BARCHART_H
#define WISTERIA_CATEGORICAL_BARCHART_H

#include "barchart.h"

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief %Bar chart that aggregates the frequency (or summed values)
         of a categorical column's labels.

         Bars can either be plotted as a regular bar or split into (stacked) groups.

         | Regular         | Grouped
         | :-------------- | :--------------------------------
         | @image html CategorizedBarChart.svg width=90% | @image html GroupedCategorizedBarChart.svg width=90%

        @par %Data:
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
            std::make_shared<Brushes::Schemes::BrushScheme>(Colors::Schemes::Decade1980s()) );

          plot->SetData(mpgData, L"manufacturer");
          // to group by model within the manufacturers
          // plot->SetData(mpgData, L"manufacturer", std::nullopt, L"model");

          canvas->SetFixedObject(0, 0, plot);
         @endcode
    */
    // clang-format on

    class CategoricalBarChart final : public Wisteria::Graphs::BarChart
        {
        wxDECLARE_DYNAMIC_CLASS(CategoricalBarChart);
        CategoricalBarChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param brushes The brush scheme, which will contain the color and brush patterns
                to render the bars with.
            @param colors The color scheme to apply to the bars underneath the bars'
                brush patterns.\n
                This is useful if using a hatched brush, as this color will be solid
                and show underneath it. Leave as null just to use the brush scheme.*/
        explicit CategoricalBarChart(
            Wisteria::Canvas* canvas,
            const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr)
            : Wisteria::Graphs::BarChart(canvas)
            {
            SetBrushScheme(brushes != nullptr ? brushes :
                                                std::make_unique<Brushes::Schemes::BrushScheme>(
                                                    *Settings::GetDefaultColorScheme()));
            SetColorScheme(colors);
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
            @param categoricalColumnName The categorical or ID column from the dataset with the
                labels to group the data into. The labels in this column will become the bars.\n
                Note that using the ID column here will be less optimal than a categorical column,
                so this should be avoided for larger datasets.
            @param weightColumnName The column with values to sum for each category.
                If not used (@c std::nullopt), then the frequency of the observations
                in each group will be used.
            @param groupColumnName An additional group column to split the bars into
                (this is optional).
            @param blDisplay Which type of labels to display at the end of the bars.\n
                Note that numeric labels (value or percentages) will be shown as integers
                (i.e., no precision) for simplicity.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& categoricalColumnName,
                     const std::optional<wxString>& weightColumnName = std::nullopt,
                     const std::optional<wxString>& groupColumnName = std::nullopt,
                     const BinLabelDisplay blDisplay = BinLabelDisplay::BinValue);

      private:
        struct CatBarBlock
            {
            // group ID in the main categorical column, used for the bar axis position ordering
            Data::GroupIdType m_bin{ 0 };
            // the name of the parent bar
            wxString m_binName;
            // 0-based index into the color scheme
            // (based on the alphabetically order of the group label from
            // the secondary group column)
            size_t m_schemeIndex{ 0 };
            // the name of the group for a subblock in a bar (from the secondary group column)
            wxString m_groupName;

            // sorts by group ID from the primary categorical column, or by the
            // subgroup label (if grouping is in use) alphabetically
            [[nodiscard]]
            bool operator<(const CatBarBlock& that) const noexcept
                {
                if (m_bin != that.m_bin)
                    {
                    return m_bin < that.m_bin;
                    }
                // if in the same bar, then compare by label alphabetically
                return m_groupName.CmpNoCase(that.m_groupName) < 0;
                }
            };

        void Calculate();

        /// @brief Simpler way to get the bar slots since this isn't like a histogram that
        ///     can have gaps in between the bars.
        [[nodiscard]]
        size_t GetBarSlotCount() const noexcept override final
            {
            return GetBars().size();
            }

        const Wisteria::Data::Column<wxString>* m_idColumn{ nullptr };
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_categoricalColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_weightColumn;

        bool m_useIDColumnForBars{ false };
        bool m_useWeightColumn{ false };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_CATEGORICAL_BARCHART_H
