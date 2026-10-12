/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_FILTERABLE_GRAPH_H
#define WISTERIA_FILTERABLE_GRAPH_H

#include "../base/label.h"
#include "../data/subset.h"
#include "graph2d.h"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Wisteria::Graphs
    {
    /** @brief A graph built once for each value of a categorical column,
            showing one of these variants at a time.
        @details Only the active variant is shown on a canvas. When exported to an
            HTML dashboard, every variant is included and can be selected from the
            dashboard's filter menu.
        @note Each variant is built from a subset of the data. A group missing from a
            subset may be assigned a different color than in the other variants.
            Use explicit color schemes in the factory to keep colors consistent.
        @par Example:
        @code
         auto filteredGraph = std::make_shared<FilterableGraph>(
             theData, L"Region", L"North",
             [canvas](const std::shared_ptr<const Data::Dataset>& subset)
                 {
                 auto plot = std::make_shared<CategoricalBarChart>(canvas);
                 plot->SetData(subset, L"Product");
                 return plot;
                 });
         canvas->SetFixedObject(0, 0, filteredGraph);
         canvas->SetFixedObject(0, 1,
             filteredGraph->CreateLegend(
                 LegendOptions{}.PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        @endcode*/
    class FilterableGraph final : public GraphItems::GraphItemBase
        {
        wxDECLARE_DYNAMIC_CLASS(FilterableGraph);
        FilterableGraph() = default;

      public:
        /// @brief Builds a graph from a dataset.
        using GraphFactory =
            std::function<std::shared_ptr<Graph2D>(const std::shared_ptr<const Data::Dataset>&)>;

        /// @brief Builds a legend for a graph (e.g., a specialized legend that isn't a Label).
        using LegendFactory = std::function<std::unique_ptr<GraphItems::GraphItemBase>(
            Graph2D&, const LegendOptions&)>;

        /// @brief A graph built from one value of the filter column.
        struct Variant
            {
            /// @brief The filter value (or the label for the unfiltered data).
            wxString m_label;
            /// @brief The graph built from the filtered data.
            std::shared_ptr<Graph2D> m_graph;
            };

        /** @brief Constructor.
            @param data The dataset to filter.
            @param filterColumn The categorical column to filter on.
            @param defaultValue The value whose graph is shown initially.
                If not found, the first variant is used.
            @param factory Builds a graph from a filtered dataset.
            @param includeAllOption @c true to also include a variant built from
                the unfiltered dataset.
            @throws std::runtime_error If the column isn't found or the factory
                doesn't return a graph.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        FilterableGraph(const std::shared_ptr<const Data::Dataset>& data,
                        const wxString& filterColumn, const wxString& defaultValue,
                        const GraphFactory& factory, bool includeAllOption = false);
        /// @private
        FilterableGraph(const FilterableGraph&) = delete;
        /// @private
        FilterableGraph& operator=(const FilterableGraph&) = delete;

        /// @returns The values of a categorical column that a graph would be built for,
        ///     sorted alphabetically (empty if the column isn't found).
        /// @param data The dataset to read.
        /// @param filterColumn The categorical column to filter on.
        [[nodiscard]]
        static std::vector<wxString> GetFilterValues(const Data::Dataset& data,
                                                     const wxString& filterColumn);

        /// @returns The label used for the unfiltered variant, unless SetAllLabel() changes it.
        [[nodiscard]]
        static wxString GetDefaultAllLabel()
            {
            return _(L"(All)");
            }

        /// @returns The name of the column being filtered on.
        [[nodiscard]]
        const wxString& GetFilterColumn() const noexcept
            {
            return m_filterColumn;
            }

        /// @returns @c true if a variant for the unfiltered data is included.
        /// @note If included, it is the first variant.
        [[nodiscard]]
        bool HasAllOption() const noexcept
            {
            return m_includeAll;
            }

        /// @brief Sets the label for the unfiltered variant.
        /// @param label The label to display.
        void SetAllLabel(const wxString& label);

        /// @returns The graph variants.
        [[nodiscard]]
        const std::vector<Variant>& GetVariants() const noexcept
            {
            return m_variants;
            }

        /// @returns The index of the variant with the given label, if found.
        /// @param label The filter value to search for.
        [[nodiscard]]
        std::optional<size_t> FindVariant(const wxString& label) const;

        /// @returns The index of the variant being shown.
        [[nodiscard]]
        size_t GetActiveIndex() const noexcept
            {
            return m_activeIndex;
            }

        /** @brief Sets which variant is shown.
            @param index The index of the variant.
            @note Call the canvas's @c CalcAllSizes() afterward to lay out the new graph.*/
        void SetActiveIndex(size_t index);

        /** @brief Sets which variant is shown.
            @param label The filter value of the variant.
            @returns @c true if the variant was found.*/
        bool SetActiveValue(const wxString& label);

        /// @returns The graph being shown.
        [[nodiscard]]
        const std::shared_ptr<Graph2D>& GetActiveGraph() const noexcept
            {
            return m_variants.at(m_activeIndex).m_graph;
            }

        /** @brief Builds a legend from the graph being shown.
            @details The legend is updated when a different variant is shown.
            @param options Options for how to build the legend.
            @returns The legend.*/
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options);

        /** @brief Builds a legend from the graph being shown, using a custom builder.
            @details The builder is used again whenever a different variant is shown.
                If the legend isn't a Label, it is replaced in its canvas cell then.
            @param options Options for how to build the legend.
            @param factory Builds the legend from a graph.
            @returns The legend.*/
        [[nodiscard]]
        std::shared_ptr<GraphItems::GraphItemBase> CreateLegend(const LegendOptions& options,
                                                                const LegendFactory& factory);

        /// @returns The legend from CreateLegend(), or null if it was never created
        ///     or no longer exists.
        [[nodiscard]]
        std::shared_ptr<GraphItems::GraphItemBase> GetLegend() const noexcept
            {
            return m_legend.lock();
            }

        /// @private
        [[nodiscard]]
        wxRect GetBoundingBox(wxDC& dc) const override;
        /// @private
        void SetBoundingBox(const wxRect& rect, wxDC& dc, double parentScaling) override;
        /// @private
        void Offset(int xToMove, int yToMove) override;
        /// @private
        void SetScaling(double scaling) override;
        /// @private
        void SetDPIScaleFactor(double scaling) override;
        /// @private
        void SetSelected(bool selected) override;
        /// @private
        void SetSelectable(bool selectable) noexcept override;
        /// @private
        void SetFreeFloating(bool freeFloat) override;
        /// @private
        [[nodiscard]]
        wxBitmap ToBitmap(wxDC& dc) const override;
        /// @private
        void SetAutoAccessibilityAttributes() override;
        /// @private
        [[nodiscard]]
        bool IsOk() const noexcept override;

      private:
        wxRect Draw(wxDC& dc) const override;
        void DrawSelectionLabel(wxDC& dc, double scaling, wxRect boundingBox) const override;
        void RecalcSizes(wxDC& dc) override;
        void UpdateSelectedItems() override;
        void ClearSelections() override;
        [[nodiscard]]
        bool HitTest(wxPoint pt, wxDC& dc) const override;
        bool SelectObjectAtPoint(const wxPoint& pt, wxDC& dc) override;
        [[nodiscard]]
        wxRect GetContentRect() const noexcept override;

        /// @brief Copies the canvas layout settings from a graph to this object.
        void AdoptLayoutFrom(const Graph2D& graph);
        /// @brief Copies the settings that the canvas set on this object to a graph.
        void SyncToGraph(Graph2D& graph) const;
        /// @brief Refreshes the legend from the graph being shown.
        void UpdateLegend();
        /// @returns A legend for the graph being shown, using the stored legend options.
        [[nodiscard]]
        std::unique_ptr<GraphItems::GraphItemBase> BuildLegend() const;

        [[nodiscard]]
        GraphItems::GraphItemBase& GetActiveItem() const
            {
            return *m_variants.at(m_activeIndex).m_graph;
            }

        wxString m_filterColumn;
        bool m_includeAll{ true };
        std::vector<Variant> m_variants;
        size_t m_activeIndex{ 0 };

        std::weak_ptr<GraphItems::GraphItemBase> m_legend;
        std::optional<LegendOptions> m_legendOptions;
        LegendFactory m_legendFactory;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_FILTERABLE_GRAPH_H
