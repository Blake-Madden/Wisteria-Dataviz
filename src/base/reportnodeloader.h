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
                calls the corresponding Apply* method in the correct order.
            @param table The table to apply the features to.*/
        void ApplyTableFeatures(std::shared_ptr<Graphs::Table>& table);

        /// @name Individual feature methods
        /// @brief Each method applies one JSON-driven feature to a table.
        /// @{
        static void ApplyTableSort(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& sortNode);
        static void ApplyTableGroupHeader(std::shared_ptr<Graphs::Table>& table,
                                          const wxSimpleJSON::Ptr_t& node);
        static void ApplyTableRowGrouping(std::shared_ptr<Graphs::Table>& table,
                                          const wxSimpleJSON::Ptr_t& node);
        static void ApplyTableColumnGrouping(std::shared_ptr<Graphs::Table>& table,
                                             const wxSimpleJSON::Ptr_t& node);
        void ApplyTableAlternateRowColor(std::shared_ptr<Graphs::Table>& table,
                                         const wxSimpleJSON::Ptr_t& node);
        void ApplyTableRowAdditions(std::shared_ptr<Graphs::Table>& table,
                                    const wxSimpleJSON::Ptr_t& node);
        void ApplyTableRowSuppression(std::shared_ptr<Graphs::Table>& table,
                                      const wxSimpleJSON::Ptr_t& node);
        void ApplyTableRowFormatting(std::shared_ptr<Graphs::Table>& table,
                                     const wxSimpleJSON::Ptr_t& formattingNode,
                                     const wxSimpleJSON::Ptr_t& colorNode,
                                     const wxSimpleJSON::Ptr_t& boldNode,
                                     const wxSimpleJSON::Ptr_t& bordersNode,
                                     const wxSimpleJSON::Ptr_t& contentAlignNode);
        void ApplyTableColumnSuppression(std::shared_ptr<Graphs::Table>& table,
                                         const wxSimpleJSON::Ptr_t& node);
        void ApplyTableColumnFormatting(std::shared_ptr<Graphs::Table>& table,
                                        const wxSimpleJSON::Ptr_t& formattingNode,
                                        const wxSimpleJSON::Ptr_t& colorNode,
                                        const wxSimpleJSON::Ptr_t& boldNode,
                                        const wxSimpleJSON::Ptr_t& bordersNode,
                                        const wxSimpleJSON::Ptr_t& contentAlignNode);
        static void ApplyTableColumnHighlight(std::shared_ptr<Graphs::Table>& table,
                                              const wxSimpleJSON::Ptr_t& node);
        void ApplyTableAggregates(std::shared_ptr<Graphs::Table>& table,
                                  const wxSimpleJSON::Ptr_t& node);
        void ApplyTableRowTotals(std::shared_ptr<Graphs::Table>& table,
                                 const wxSimpleJSON::Ptr_t& node);
        void ApplyTableCellUpdates(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node);
        void ApplyTableAnnotations(std::shared_ptr<Graphs::Table>& table,
                                   const wxSimpleJSON::Ptr_t& node);
        void ApplyTableFootnotes(std::shared_ptr<Graphs::Table>& table,
                                 const wxSimpleJSON::Ptr_t& node);

        /// @}

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

        /// @brief Parses a row or column position from a JSON node.
        /// @param positionNode The position node to parse.
        /// @param table The table (used for named positions like "last-column").
        /// @note Column and row counts should be the table's original column and row
        ///     counts, prior to any aggregation columns being added.
        /// @returns The row or column position.
        [[nodiscard]]
        static std::optional<size_t> LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
                                                       std::shared_ptr<Graphs::Table> table);

      private:
        const ReportBuilder& m_reportBuilder;
        };
    } // namespace Wisteria

/// @}

#endif // WISTERIA_REPORT_NODE_LOADER_H
