/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_BUBBLE_PLOT_H
#define WISTERIA_BUBBLE_PLOT_H

#include "scatterplot.h"

namespace Wisteria::Graphs
    {
    /** @brief Bubble plot, which extends scatter plots by adding a third continuous variable
            that controls the size of each point (scaled by area, not diameter).

        @image html bubbleplot.svg width=90%

        @par %Data:
            This plot accepts a Data::Dataset, where one continuous column is the Y values
            (dependent variable), another continuous column is the X values
            (independent variable), and a third continuous column controls the bubble sizes.
            A grouping column can optionally be used to create separate point series
            with individual regression lines for different groups in the data.

        @par Missing Data:
            - Missing data in the group column will be shown as an empty legend label.
            - If any of the X, Y, or size values is missing data (NaN), that point will be
              excluded from both the plot and the regression calculation (i.e., pairwise deletion).

        @par Bubble Sizing:
            Bubble sizes are scaled by area (not diameter) for perceptually accurate comparisons.
            The size values are normalized to a 0-1 range, mapping the smallest value to the
            minimum bubble radius and the largest value to the maximum bubble radius.
            This ensures all bubbles remain visible regardless of the data range, making it
            easier to compare relative differences.

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         auto bubbleData = std::make_shared<Data::Dataset>();
         bubbleData->ImportCSV(L"data.csv",
            ImportInfo().
            ContinuousColumns({ L"Height", L"Weight", L"Age" }).
            CategoricalColumns({
                { L"Gender", CategoricalImportMethod::ReadAsStrings }
            }));

         auto bubblePlot = std::make_shared<BubblePlot>(canvas);
         bubblePlot->SetData(bubbleData, L"Weight", L"Height", L"Age", L"Gender");

         canvas->SetFixedObject(0, 0, bubblePlot);
         canvas->SetFixedObject(0, 1,
            bubblePlot->CreateLegend(
                LegendOptions{}.
                    IncludeHeader(true).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        @endcode
    */
    class BubblePlot : public ScatterPlot
        {
        wxDECLARE_DYNAMIC_CLASS(BubblePlot);

        BubblePlot() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the bubble plot on.
            @param colors The color scheme to apply to the points and regression lines.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the point markers.
                Leave as @c nullptr to use the standard shapes.
            @param regressionLineStyles The line styles to use for the regression lines.
                The default is to use solid, straight lines.*/
        explicit BubblePlot(
            Canvas* canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr,
            const std::shared_ptr<LineStyleScheme>& regressionLineStyles = nullptr);

        /** @brief Sets the data.
            @details Along with the x, y, and size points, separate series can be created based
                on the (optional) grouping column in the data.
                Each series will have its own regression line and statistics.
            @param data The data to use for the bubble plot.
            @param yColumnName The y column data (a continuous column, dependent variable).
            @param xColumnName The x column data (a continuous column, independent variable).
            @param sizeColumnName The size column data (a continuous column).
                Larger values result in larger bubble areas (not diameters).
            @param groupColumnName The (optional) categorical column to use for grouping.
                This will split the data into separate series with individual regression lines.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data, const wxString& yColumnName,
                     const wxString& xColumnName, const wxString& sizeColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @private
        [[nodiscard]]
        const wxString& GetSizeColumnName() const noexcept
            {
            return m_sizeColumnName;
            }

        /// @name Bubble Size Functions
        /// @brief Functions relating to bubble sizing.
        /// @{

        /// @returns The minimum bubble radius (in DIPs).
        [[nodiscard]]
        size_t GetMinBubbleRadius() const noexcept
            {
            return m_minBubbleRadius;
            }

        /// @brief Sets the minimum bubble radius.
        /// @param radius The minimum radius (in DIPs).
        void SetMinBubbleRadius(const size_t radius) noexcept { m_minBubbleRadius = radius; }

        /// @returns The maximum bubble radius (in DIPs).
        [[nodiscard]]
        size_t GetMaxBubbleRadius() const noexcept
            {
            return m_maxBubbleRadius;
            }

        /// @brief Sets the maximum bubble radius.
        /// @param radius The maximum radius (in DIPs).
        void SetMaxBubbleRadius(const size_t radius) noexcept { m_maxBubbleRadius = radius; }

        /// @}

        /// @name Legend Functions
        /// @brief Functions relating to the legend.
        /// @{

        /** @brief Builds and returns a legend, containing the groups, regression statistics,
                and a size scale showing reference bubbles.
            @details This can be then be managed by the parent canvas and placed
                next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

        /// @}

      protected:
        /// @brief Recalculates the layout and plots the data.
        /// @param dc The DC to draw to.
        void RecalcSizes(wxDC& dc) final;

      private:
        wxString m_sizeColumnName;
        size_t m_minBubbleRadius{ 4 };
        size_t m_maxBubbleRadius{ 30 };
        double m_sizeMin{ 0 };
        double m_sizeMax{ 0 };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_BUBBLE_PLOT_H
