/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_FUNNELCHART_H
#define WISTERIA_FUNNELCHART_H

#include "barchart.h"
#include <optional>

namespace Wisteria::Graphs
    {
    /** @brief Funnel chart for visualizing sequential pipeline stages.

        Rows define the funnel top to bottom. Row 0 is the top. Last row is the bottom.
        No sorting. Row order is funnel order.

        Each stage is drawn as a centered trapezoid whose width is proportional
        to its value. The width of the next stage forms the bottom edge, so
        the sides slope inward and the drop-off between stages is immediately
        visible.

        An optional target column draws a ghost behind each stage,
        showing the gap between actual and target.

        | Regular | With Target |
        | :-------------- | :-------------- |
        | @image html funnel-chart.svg width=90% |  |

        @par %Data:
         This plot accepts a Data::Dataset. One column provides the stage
         labels (categorical or ID column, each row is one stage).
         One continuous column provides the actual value for each stage. An
         optional second continuous column provides the target value.

        @par Missing Data:
         - Missing stage label: stage still appears with an empty axis label,
           preserving row order.
         - Missing actual value (NaN): treated as 0, stage collapses to a line.
         - Missing target value (NaN) or no target column: no ghost is drawn
           for that stage.

        @par Example:
        @code
         auto canvas{ new Wisteria::Canvas{ this } };
         canvas->SetFixedObjectsGridSize(1, 1);

         auto data{ std::make_shared<Data::Dataset>() };
         data->AddCategoricalColumn(L"Stage");
         data->AddContinuousColumn(L"Actual");
         data->AddContinuousColumn(L"Target");
         // Stage column string table: Visits, Signups, Trials, Paid
         data->GetCategoricalColumn(L"Stage")->GetStringTable() =
             { { 0, L"Visits" }, { 1, L"Signups" }, { 2, L"Trials" }, { 3, L"Paid" } };
         data->Reserve(4);
         data->AddRow(Data::RowInfo{}.Categoricals({ 0 }).Continuous({ 10000, 12000 }));
         data->AddRow(Data::RowInfo{}.Categoricals({ 1 }).Continuous({ 3200, 4000 }));
         data->AddRow(Data::RowInfo{}.Categoricals({ 2 }).Continuous({ 1800, 2000 }));
         data->AddRow(Data::RowInfo{}.Categoricals({ 3 }).Continuous({ 480, 600 }));

         auto plot{ std::make_shared<FunnelChart>(canvas) };
         plot->SetData(data, L"Stage", L"Actual", L"Target");
         canvas->SetFixedObject(0, 0, plot);
        @endcode
    */
    class FunnelChart final : public BarChart
        {
        wxDECLARE_DYNAMIC_CLASS(FunnelChart);
        FunnelChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param brushes The brush scheme for stage fill patterns.
            @param colors The color scheme for stage fill colors.\n
                If @c nullptr only the brush scheme is used.
            @note Brushes are assigned in row order (stage 0 gets brush 0, etc.;
                wrapping if there are more stages than brushes).*/
        explicit FunnelChart(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the funnel.
            @details Rows define stages top-to-bottom.

            @param data The dataset containing the funnel data.
            @param stageColumnName The categorical or ID column with stage labels.
            @param valueColumnName The continuous column with the actual value
                for each stage.
            @param targetColumnName Optional continuous column with target values.
                When present, a ghosted trapezoid is drawn behind the actual stage.
            @throws std::runtime_error If any column cannot be found by name.
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting for display.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new
                dataset to re-plot the data.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& stageColumnName, const wxString& valueColumnName,
                     const std::optional<wxString>& targetColumnName = std::nullopt);

        /// @returns The name of the stage (label) column.
        [[nodiscard]]
        const wxString& GetStageColumnName() const noexcept
            {
            return m_stageColumnName;
            }

        /// @returns The name of the value (actual) column.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the target column, or @c std::nullopt if none.
        [[nodiscard]]
        const std::optional<wxString>& GetTargetColumnName() const noexcept
            {
            return m_targetColumnName;
            }

        /// @returns The ghost opacity used for the target overlay.
        [[nodiscard]]
        uint8_t GetTargetGhostOpacity() const noexcept
            {
            return m_targetGhostOpacity;
            }

        /// @brief Sets the ghost opacity for the target overlay.
        /// @param opacity Opacity 0 (transparent) to 255 (opaque).
        void SetTargetGhostOpacity(const uint8_t opacity) noexcept
            {
            m_targetGhostOpacity = opacity;
            }

        /// @returns Whether explanations are shown.
        [[nodiscard]]
        bool AreExplanationsShown() const noexcept
            {
            return m_showExplanations;
            }

        /// @brief Shows or hides "Actual" and "Target" explanations that point
        ///     at the top funnel segment's edges. Labels are placed to the left
        ///     of the funnel and scaled together to avoid overlap.
        /// @param show @c true to show the explanations.
        void ShowExplanations(const bool show = true) noexcept { m_showExplanations = show; }

        /// @returns Whether conversion rate labels are shown.
        [[nodiscard]]
        bool AreConversionLabelsShown() const noexcept
            {
            return m_showConversionLabels;
            }

        /// @brief Shows or hides the conversion labels between stages.
        /// @param show @c true to show the conversion labels.
        void ShowConversionLabels(const bool show = true) noexcept
            {
            m_showConversionLabels = show;
            }

        /// @brief The visual style of the funnel.
        enum class FunnelStyle
            {
            Standard, /*!< Flat solid fill. */
            Glassy    /*!< 3D glassy sheen with highlight. */
            };

        /// @returns The funnel style.
        [[nodiscard]]
        FunnelStyle GetFunnelStyle() const noexcept
            {
            return m_style;
            }

        /// @brief Sets the funnel style.
        /// @param style The style to use.
        void SetFunnelStyle(const FunnelStyle style) noexcept { m_style = style; }

        /// @returns The target values in row order (@c NaN if no target for that row).
        [[nodiscard]]
        const std::vector<double>& GetTargetValues() const noexcept
            {
            return m_targetValues;
            }

        /// @returns @c false always. Funnels are not sortable as funnel order is meaningful.
        [[nodiscard]]
        bool IsSortable() const noexcept final
            {
            return false;
            }

        /// @deprecated Funnel charts do not support sorting.
        /// @private
        [[deprecated("Row order defines the funnel")]]
        void SortBars(BarSortComparison, SortDirection) final {}

        /// @deprecated Funnel charts do not support sorting.
        /// @private
        [[deprecated("Row order defines the funnel")]]
        void SortBars(std::vector<wxString>, SortDirection) final {}

        /// @deprecated Funnel charts do not support legends.
        /// @private
        [[deprecated(
            "Funnel charts label stages along the funnel; a legend is not used")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

      protected:
        /// @private
        void RecalcSizes(wxDC& dc) final;

        /// @private
        [[nodiscard]]
        size_t GetBarSlotCount() const noexcept final
            {
            return GetBars().size();
            }

        /// @private
        void SetAutoAccessibilityAttributes() final;

      private:
        [[nodiscard]]
        auto GetStageColumn(const wxString& stageColumnName) const;

        [[nodiscard]]
        auto GetValueColumn(const wxString& valueColumnName) const;

        [[nodiscard]]
        auto GetTargetColumn(const std::optional<wxString>& targetColumnName) const;

        wxString m_stageColumnName;
        wxString m_valueColumnName;
        std::optional<wxString> m_targetColumnName{ std::nullopt };
        std::vector<double> m_targetValues;
        bool m_useIdColumnForStage{ false };
        uint8_t m_targetGhostOpacity{ Settings::GHOST_OPACITY };
        bool m_showExplanations{ false };
        bool m_showConversionLabels{ true };
        FunnelStyle m_style{ FunnelStyle::Glassy };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_FUNNELCHART_H
