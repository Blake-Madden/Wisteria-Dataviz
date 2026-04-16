/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_NODE_LOADER_H
#define WISTERIA_REPORT_NODE_LOADER_H

#include "../graphs/table.h"
#include "../wxSimpleJSON/src/wxSimpleJSON.h"
#include "reportenumconvert.h"
#include <memory>
#include <optional>
#include <set>

/// @brief Forward declaration.
namespace Wisteria
    {
    class ReportBuilder;
    }

namespace Wisteria
    {
    /** @brief Applies procedural table features (sorting, grouping, formatting, etc.)
            from cached JSON property templates.
        @details Each Apply* method corresponds to a single table JSON property.
            The main entry point is ApplyTableFeatures(), which re-applies
            all cached templates in the correct order.*/
    class ReportNodeLoader
        {
      public:
        /** @brief Constructs a loader bound to a report builder.
            @details The report builder is used for helper functions such as
                ConvertColor(), ConvertNumber(), ExpandAndCache(), and LoadPen().
            @param reportBuilder The report builder to use for helper functions.*/
        explicit ReportNodeLoader(const ReportBuilder& reportBuilder) noexcept
            : m_reportBuilder(reportBuilder)
            {
            }

        /// @private
        ReportNodeLoader(const ReportNodeLoader&) = delete;
        /// @private
        ReportNodeLoader& operator=(const ReportNodeLoader&) = delete;

        /** @brief Re-applies all cached procedural features to a table.
            @details Parses each property template stored on the table and
                calls the corresponding Apply* method in the correct order.\n
                The application order is: sorting, group headers, row/column grouping,
                alternate row coloring, row additions, row/column suppression,
                column/row formatting, column highlighting, aggregates, row totals,
                cell updates, annotations, and footnotes.
            @param table The table to apply the features to.*/
        void ApplyTableFeatures(std::shared_ptr<Graphs::Table>& table);

        /** @brief Applies sorting to a table.
            @param table The table to sort.
            @param sortNode The JSON node containing the sort configuration.*/
        void ApplyTableSort(std::shared_ptr<Graphs::Table>& table,
                            const wxSimpleJSON::Ptr_t& sortNode) const;

        /** @brief Applies a group header to a table.
            @param table The table to apply the group header to.
            @param node The JSON node containing the group header configuration.*/
        void ApplyTableGroupHeader(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies row grouping to a table.
            @param table The table to apply row grouping to.
            @param node The JSON node containing the row grouping configuration.*/
        void ApplyTableRowGrouping(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies column grouping to a table.
            @param table The table to apply column grouping to.
            @param node The JSON node containing the column grouping configuration.*/
        void ApplyTableColumnGrouping(std::shared_ptr<Graphs::Table>& table,
                                      const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies alternate row coloring to a table.
            @param table The table to apply alternate row coloring to.
            @param node The JSON node containing the alternate row color configuration.*/
        void ApplyTableAlternateRowColor(std::shared_ptr<Graphs::Table>& table,
                                         const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies row additions (e.g., blank or separator rows) to a table.
            @param table The table to add rows to.
            @param node The JSON node containing the row addition configuration.*/
        void ApplyTableRowAdditions(std::shared_ptr<Graphs::Table>& table,
                                    const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies row suppression (hiding) to a table.
            @param table The table to suppress rows in.
            @param node The JSON node containing the row suppression configuration.*/
        void ApplyTableRowSuppression(std::shared_ptr<Graphs::Table>& table,
                                      const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies column suppression (hiding) to a table.
            @param table The table to suppress columns in.
            @param node The JSON node containing the column suppression configuration.*/
        void ApplyTableColumnSuppression(std::shared_ptr<Graphs::Table>& table,
                                         const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies column formatting to a table.
            @param table The table to format.
            @param formattingNode The JSON node for number/date formatting.*/
        void ApplyTableColumnFormatting(std::shared_ptr<Graphs::Table>& table,
                                        const wxSimpleJSON::Ptr_t& formattingNode) const;

        /** @brief Applies column color to a table.
            @param table The table to format.
            @param colorNode The JSON node for column colors.*/
        void ApplyTableColumnColor(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& colorNode) const;

        /** @brief Applies column boldness to a table.
            @param table The table to format.
            @param boldNode The JSON node for column bolding.*/
        void ApplyTableColumnBold(std::shared_ptr<Graphs::Table>& table,
                                  const wxSimpleJSON::Ptr_t& boldNode) const;

        /** @brief Applies column borders to a table.
            @param bordersNode The JSON node for column borders.*/
        void ApplyTableColumnBorders(std::shared_ptr<Graphs::Table>& table,
                                     const wxSimpleJSON::Ptr_t& bordersNode) const;

        /** @brief Applies row formatting to a table.
            @param table The table to format.
            @param formattingNode The JSON node for number/date formatting.*/
        void ApplyTableRowFormatting(std::shared_ptr<Graphs::Table>& table,
                                     const wxSimpleJSON::Ptr_t& formattingNode) const;

        /** @brief Applies row color to a table.
            @param table The table to format.
            @param colorNode The JSON node for row colors.*/
        void ApplyTableRowColor(std::shared_ptr<Graphs::Table>& table,
                                const wxSimpleJSON::Ptr_t& colorNode) const;

        /** @brief Applies row boldness to a table.
            @param boldNode The JSON node for row bolding.*/
        void ApplyTableRowBold(std::shared_ptr<Graphs::Table>& table,
                               const wxSimpleJSON::Ptr_t& boldNode) const;

        /** @brief Applies row borders to a table.
            @param bordersNode The JSON node for row borders.*/
        void ApplyTableRowBorders(std::shared_ptr<Graphs::Table>& table,
                                  const wxSimpleJSON::Ptr_t& bordersNode) const;

        /** @brief Applies row formatting to a table.
            @param table The table to format.
            @param contentAlignNode The JSON node for content alignment.*/
        void ApplyTableRowContentAlignment(std::shared_ptr<Graphs::Table>& table,
                                           const wxSimpleJSON::Ptr_t& contentAlignNode) const;

        /** @brief Applies column highlighting to a table.
            @param table The table to highlight columns in.
            @param node The JSON node containing the column highlight configuration.*/
        void ApplyTableColumnHighlight(std::shared_ptr<Graphs::Table>& table,
                                       const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies aggregate columns (e.g., sum, average) to a table.
            @param table The table to add aggregates to.
            @param node The JSON node containing the aggregate configuration.*/
        void ApplyTableAggregates(std::shared_ptr<Graphs::Table>& table,
                                  const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies row totals to a table.
            @param table The table to add row totals to.
            @param node The JSON node containing the row totals configuration.*/
        void ApplyTableRowTotals(std::shared_ptr<Graphs::Table>& table,
                                 const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies cell-level updates to a table.
            @param table The table to update cells in.
            @param node The JSON node containing the cell update configuration.*/
        void ApplyTableCellUpdates(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies annotations to a table.
            @param table The table to annotate.
            @param node The JSON node containing the annotation configuration.*/
        void ApplyTableAnnotations(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node) const;

        /** @brief Applies footnotes to a table.
            @param table The table to add footnotes to.
            @param node The JSON node containing the footnote configuration.*/
        void ApplyTableFootnotes(std::shared_ptr<Graphs::Table>& table,
                                 const wxSimpleJSON::Ptr_t& node) const;

      private:
        /// @brief Parses a row or column position from a JSON node.
        /// @param positionNode The position node to parse.
        /// @param table The table (used for named positions like "last-column").
        /// @note Column and row counts should be the table's original column and row
        ///     counts, prior to any aggregation columns being added.
        /// @returns The row or column position.
        [[nodiscard]]
        static std::optional<size_t> LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
                                                       std::shared_ptr<Graphs::Table> table);

        /// @brief Loads the positions from a row or column stops array.
        [[nodiscard]]
        static auto LoadTableStops(std::shared_ptr<Graphs::Table>& table, const auto& stopsNode)
            {
            std::set<size_t> rowOrColumnStops;
            const auto stops = stopsNode->AsNodes();
            if (stops.size())
                {
                for (const auto& stop : stops)
                    {
                    const std::optional<size_t> stopPosition =
                        LoadTablePosition(stop->GetProperty(L"position"), table);
                    if (stopPosition.has_value())
                        {
                        rowOrColumnStops.insert(stopPosition.value());
                        }
                    }
                }
            return rowOrColumnStops;
            }

        /// @brief Reads a single position and range of positions (start and end).
        [[nodiscard]]
        static auto ReadPositions(std::shared_ptr<Graphs::Table>& table,
                                  const wxSimpleJSON::Ptr_t& theNode)
            {
            const std::optional<size_t> position =
                LoadTablePosition(theNode->GetProperty(L"position"), table);
            const std::optional<size_t> startPosition =
                LoadTablePosition(theNode->GetProperty(L"start"), table);
            const std::optional<size_t> endPosition =
                LoadTablePosition(theNode->GetProperty(L"end"), table);
            return std::tuple(position, startPosition, endPosition);
            }

        const ReportBuilder& m_reportBuilder;
        };
    } // namespace Wisteria

/// @}

#endif // WISTERIA_REPORT_NODE_LOADER_H
