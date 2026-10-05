/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_DUELING_PIE_CHART_H
#define WISTERIA_DUELING_PIE_CHART_H

#include "piechart.h"

namespace Wisteria::Graphs
    {
    /** @brief A dueling pie chart, which compares how two groups divide up the same categories.
        @details Two pie fans meet at a shared center point. The first group is drawn as a fan
            along the top, reading left to right. The second group is drawn as the same fan,
            rotated 180 degrees along the bottom. Each fan spans 90 degrees, and each of its
            slices is sized by the category's share of that group's own total.

            The categories share the same colors in both fans. The legend is always shown, split
            in two and placed in the empty wedges to the left and right of the center point.

        @par Citation:
            The layout follows one of the hand-drawn plates that W. E. B. Du Bois and his
            students at Atlanta University prepared for the 1900 Paris Exposition.\n
            \n
            Battle-Baptiste, W., &amp; Rusert, B. (Eds.). (2018). <i>W. E. B. Du Bois's data
            portraits: Visualizing Black America.</i> Princeton Architectural Press.

        @par %Data:
            This plot accepts a Data::Dataset in long format. A continuous column contains the
            values, a categorical column contains the categories (the slices), and a second
            categorical column contains the group that each value belongs to.

            Below is an example where the continuous column is `Hours`, the category column is
            `Activity`, and the group column is `Day`.

            | Activity | Day     | Hours |
            | :--      | :--     | --:   |
            | Work     | Weekday | 8     |
            | Sleep    | Weekday | 7     |
            | Leisure  | Weekday | 4     |
            | Work     | Weekend | 1     |
            | Sleep    | Weekend | 9     |
            | Leisure  | Weekend | 8     |
            ...

            The group column must contain exactly two distinct values. The first group found in
            the dataset is drawn along the top, and the second is drawn along the bottom.
            Categories are drawn in the order that they first appear in the dataset.

        @par Missing Data:
        - Rows with a missing or non-positive value are ignored.
        - If a group has no value for a category, then that category has no slice in that group's
          fan. (The category keeps its color and legend entry.)

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 1);

         auto timeData = std::make_shared<Data::Dataset>();
         try
            {
            timeData->ImportCSV(L"/home/rdoyle/data/Daily Hours.csv",
                ImportInfo().
                ContinuousColumns({ L"Hours" }).
                CategoricalColumns({
                    { L"Activity", CategoricalImportMethod::ReadAsStrings },
                    { L"Day", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
         auto plot = std::make_shared<DuelingPieChart>(canvas);
         plot->SetData(timeData, L"Hours", L"Activity", L"Day");

         canvas->SetFixedObject(0, 0, plot);
        @endcode*/
    class DuelingPieChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(DuelingPieChart);
        DuelingPieChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param brushes The brush scheme, which will contain the color and brush patterns
                to render the categories with.
            @param colors The color scheme to apply to the slices underneath the slices'
                brush patterns.\n
                This is useful if using a hatched brush, as this color will be solid
                and show underneath it. Leave as @c nullptr just to use the brush scheme.*/
        explicit DuelingPieChart(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the chart from a dataset.
            @param data The data to use, which should contain a continuous column (values),
                and two categorical columns (categories and groups).
            @param valueColumnName The continuous column containing the values.
            @param categoryColumnName The categorical column containing the categories
                (i.e., the slices).
            @param groupColumnName The categorical column containing the two groups
                (i.e., the fans).
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, or if the group
                column does not contain exactly two distinct values, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& valueColumnName, const wxString& categoryColumnName,
                     const wxString& groupColumnName);

        /// @returns The name of the continuous column that the values came from.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the categorical column that the categories came from.
        [[nodiscard]]
        const wxString& GetCategoryColumnName() const noexcept
            {
            return m_categoryColumnName;
            }

        /// @returns The name of the categorical column that the groups came from.
        [[nodiscard]]
        const wxString& GetGroupColumnName() const noexcept
            {
            return m_groupColumnName;
            }

        /// @returns The label of the group drawn along the top.
        [[nodiscard]]
        const wxString& GetTopGroupLabel() const noexcept
            {
            return m_topGroupLabel;
            }

        /// @returns The label of the group drawn along the bottom.
        [[nodiscard]]
        const wxString& GetBottomGroupLabel() const noexcept
            {
            return m_bottomGroupLabel;
            }

        /// @returns What the labels inside the slices are displaying.
        [[nodiscard]]
        BinLabelDisplay GetMidPointLabelDisplay() const noexcept
            {
            return m_midPointLabelDisplay;
            }

        /// @brief Sets what the labels inside the slices are displaying.
        /// @param display What to display. Only @c BinLabelDisplay::BinPercentage (the default),
        ///     @c BinLabelDisplay::BinValue, and @c BinLabelDisplay::BinValueAndPercentage are
        ///     accepted. Any other value is ignored.
        /// @note The category names are always shown in the legend, never inside the slices.
        void SetMidPointLabelDisplay(const BinLabelDisplay display) noexcept
            {
            if (display == BinLabelDisplay::BinPercentage || display == BinLabelDisplay::BinValue ||
                display == BinLabelDisplay::BinValueAndPercentage)
                {
                m_midPointLabelDisplay = display;
                }
            }

        /// @private
        [[nodiscard]]
        const PieChart::PieInfo& GetTopPie() const noexcept
            {
            return m_topPie;
            }

        /// @private
        [[nodiscard]]
        const PieChart::PieInfo& GetBottomPie() const noexcept
            {
            return m_bottomPie;
            }

      private:
        /// @deprecated
        [[deprecated("Dueling pie charts always include their own legend.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

        void SetAutoAccessibilityAttributes() final;
        void RecalcSizes(wxDC& dc) final;

        // the angle each fan spans
        constexpr static double m_fanSweepAngle{ 90.0 };

        // the slices of each fan, in the same category order
        PieChart::PieInfo m_topPie;
        PieChart::PieInfo m_bottomPie;
        wxString m_topGroupLabel;
        wxString m_bottomGroupLabel;

        BinLabelDisplay m_midPointLabelDisplay{ BinLabelDisplay::BinPercentage };

        // column names
        wxString m_valueColumnName;
        wxString m_categoryColumnName;
        wxString m_groupColumnName;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_DUELING_PIE_CHART_H
