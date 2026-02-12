/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SCATTER_PLOT_H
#define WISTERIA_SCATTER_PLOT_H

#include "../math/statistics.h"
#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Scatter plot, which displays the relationship between two continuous variables
            with optional linear regression lines and statistics.

        @par %Data:
            This plot accepts a Data::Dataset, where one continuous column is the Y values
            (dependent variable) and another continuous column is the X values
            (independent variable).
            A grouping column can optionally be used to create separate point series
            with individual regression lines for different groups in the data.

        @par Missing Data:
            - Missing data in the group column will be shown as an empty legend label.
            - If either the X or Y value is missing data (NaN), that point will be excluded
              from both the plot and the regression calculation (i.e., pair-wise deletion).

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         auto scatterData = std::make_shared<Data::Dataset>();
         scatterData->ImportCSV(L"data.csv",
            ImportInfo().
            ContinuousColumns({ L"Height", L"Weight" }).
            CategoricalColumns({
                { L"Gender", CategoricalImportMethod::ReadAsStrings }
            }));

         auto scatterPlot = std::make_shared<ScatterPlot>(canvas);
         scatterPlot->SetData(scatterData, L"Weight", L"Height", L"Gender");

         canvas->SetFixedObject(0, 0, scatterPlot);
         canvas->SetFixedObject(0, 1,
            scatterPlot->CreateLegend(
                LegendOptions().
                    IncludeHeader(true).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        @endcode
    */
    class ScatterPlot : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(ScatterPlot);

        ScatterPlot() = default;

      public:
        /// @brief A data series on a scatter plot.
        class Series
            {
            friend class ScatterPlot;

          public:
            /// @name Group Information
            /// @brief Functions relating to the group associated with this series.
            /// @{

            /// @returns The label for the series (from the grouping column).
            [[nodiscard]]
            const wxString& GetText() const noexcept
                {
                return m_label;
                }

            /// @returns The group ID.
            [[nodiscard]]
            Data::GroupIdType GetGroupId() const noexcept
                {
                return m_groupId;
                }

            /// @returns The group column name.
            [[nodiscard]]
            std::optional<wxString> GetGroupColumnName() const
                {
                return m_groupColumnName;
                }

            /// @brief Sets the grouping information connected to this series.
            /// @param groupColumnName The grouping column name.
            /// @param groupId The group ID for this series.
            /// @param groupName The display name of the group.
            void SetGroupInfo(const std::optional<wxString>& groupColumnName,
                              const Data::GroupIdType groupId, const wxString& groupName)
                {
                m_groupColumnName = groupColumnName;
                m_groupId = groupId;
                m_label = groupName;
                }

            /// @}

            /// @name Point Appearance
            /// @brief Functions relating to the visual display of scatter points.
            /// @{

            /// @returns The marker shape for points in this series.
            [[nodiscard]]
            Icons::IconShape GetShape() const noexcept
                {
                return m_shape;
                }

            /// @brief Sets the marker shape for points.
            /// @param shape The shape.
            void SetShape(const Icons::IconShape shape) noexcept { m_shape = shape; }

            /// @returns The color for points in this series.
            [[nodiscard]]
            const wxColour& GetColor() const noexcept
                {
                return m_color;
                }

            /// @brief Sets the color for points.
            /// @param color The color.
            void SetColor(const wxColour& color) noexcept { m_color = color; }

            /// @}

            /// @name Regression Line Appearance
            /// @brief Functions relating to the visual display of the regression line.
            /// @{

            /// @returns The pen used for the regression line.
            [[nodiscard]]
            wxPen& GetRegressionPen() noexcept
                {
                return m_regressionPen;
                }

            /// @private
            [[nodiscard]]
            const wxPen& GetRegressionPen() const noexcept
                {
                return m_regressionPen;
                }

            /// @returns How the regression line is drawn.
            [[nodiscard]]
            LineStyle GetRegressionLineStyle() const noexcept
                {
                return m_regressionLineStyle;
                }

            /// @brief Sets how the regression line is drawn.
            /// @param lineStyle The line style.
            void SetRegressionLineStyle(const LineStyle lineStyle) noexcept
                {
                m_regressionLineStyle = lineStyle;
                }

            /// @}

            /// @name Regression Statistics
            /// @brief Functions relating to the regression analysis results.
            /// @{

            /// @returns The regression statistics for this series.
            [[nodiscard]]
            statistics::linear_regression_results GetRegressionResults() const noexcept
                {
                return m_regressionResults;
                }

            /// @}

          private:
            std::optional<wxString> m_groupColumnName;
            Data::GroupIdType m_groupId{ 0 };
            wxString m_label;

            Icons::IconShape m_shape{ Icons::IconShape::Circle };
            wxColour m_color{ *wxBLACK };

            wxPen m_regressionPen{ wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Red), 2 } };
            LineStyle m_regressionLineStyle{ LineStyle::Lines };

            statistics::linear_regression_results m_regressionResults;
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the scatter plot on.
            @param colors The color scheme to apply to the points and regression lines.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the point markers.
                Leave as @c nullptr to use the standard shapes.
            @param regressionLineStyles The line styles to use for the regression lines.
                The default is to use solid, straight lines.*/
        explicit ScatterPlot(
            Canvas* canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr,
            const std::shared_ptr<LineStyleScheme>& regressionLineStyles = nullptr);

        /** @brief Sets the data.
            @details Along with the x and y points, separate series can be created based
                on the (optional) grouping column in the data.
                Each series will have its own regression line and statistics.
            @param data The data to use for the scatter plot.
            @param yColumnName The y column data (a continuous column, dependent variable).
            @param xColumnName The x column data (a continuous column, independent variable).
            @param groupColumnName The (optional) categorical column to use for grouping.
                This will split the data into separate series with individual regression lines.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data, const wxString& yColumnName,
                     const wxString& xColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @name Series Functions
        /// @brief Functions relating to accessing and customizing the data series.
        /// @{

        /** @brief Gets the series at the specified index.
            @param index The index of the series to get.
            @returns The series.
            @note This should be called after SetData().
            @sa GetSeriesCount().*/
        [[nodiscard]]
        Series& GetSeries(const size_t index) noexcept
            {
            wxASSERT_MSG(index < m_series.size(), L"Invalid index in GetSeries()!");
            return m_series.at(index);
            }

        /// @brief Gets all series so that you can iterate through them.
        /// @returns Direct access to the series.
        /// @note This should be called after SetData().
        [[nodiscard]]
        std::vector<Series>& GetSeriesList() noexcept
            {
            return m_series;
            }

        /// @private
        [[nodiscard]]
        const std::vector<Series>& GetSeriesList() const noexcept
            {
            return m_series;
            }

        /** @brief Gets the number of series on the plot.
            @returns The number of series.
            @note This should be called after SetData().*/
        [[nodiscard]]
        size_t GetSeriesCount() const noexcept
            {
            return m_series.size();
            }

        /// @}

        /// @name Regression Line Functions
        /// @brief Functions relating to the regression line display.
        /// @{

        /// @brief Sets whether to show regression lines.
        /// @param show @c true to show regression lines.
        void ShowRegressionLines(const bool show) noexcept { m_showRegressionLines = show; }

        /// @returns @c true if regression lines are being shown.
        [[nodiscard]]
        bool IsShowingRegressionLines() const noexcept
            {
            return m_showRegressionLines;
            }

        /// @}

        /// @name Legend Functions
        /// @brief Functions relating to the legend.
        /// @{

        /** @brief Builds and returns a legend, containing the groups and their
                regression statistics.
            @details This can be then be managed by the parent canvas and placed
                next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

        /// @}

        /// @private
        [[nodiscard]]
        const std::shared_ptr<LineStyleScheme>& GetRegressionLineStyleScheme() const noexcept
            {
            return m_regressionLineStyles;
            }

      protected:
        /// @brief Recalculates the layout and plots the data.
        /// @param dc The DC to draw to.
        void RecalcSizes(wxDC& dc) final;

      private:
        void CalculateRegression(Series& series);

        std::vector<Series> m_series;
        std::shared_ptr<LineStyleScheme> m_regressionLineStyles;
        wxString m_xColumnName;
        wxString m_yColumnName;
        bool m_showRegressionLines{ true };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_SCATTER_PLOT_H
