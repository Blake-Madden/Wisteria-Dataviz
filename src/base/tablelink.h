/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_TABLELINK_H
#define WISTERIA_TABLELINK_H

#include "../graphs/table.h"

namespace Wisteria
    {
    /// @brief Links multiple tables, ensuring that they have the same dimensions.
    class TableLink
        {
      public:
        /// @brief Constructor.
        /// @param Id The unique ID for this table linker.
        explicit TableLink(const size_t Id) : m_id(Id) {}

        /// @returns The table linker's ID.
        [[nodiscard]]
        size_t GetId() const noexcept
            {
            return m_id;
            }

        /// @private
        [[nodiscard]]
        bool operator<(const TableLink& that) const noexcept
            {
            return m_id < that.m_id;
            }

        /// @brief Adds a table to the list of connected tables.
        /// @param table The table to add.
        void AddTable(std::shared_ptr<Graphs::Table> table)
            {
            m_tables.push_back(std::move(table));
            }

        /// @brief Syncs the dimensions of the tables, so that they all have a minimum
        ///     number of rows and columns (based on the largest number of rows and columns).
        void SyncTableSizes() const
            {
            if (m_tables.empty())
                {
                return;
                }
            // validate all tables
            if (std::ranges::any_of(m_tables,
                                    [](const auto& table) noexcept { return table == nullptr; }))
                {
                return;
                }

            size_t maxRows{ 0 }, maxCols{ 0 };
            for (const auto& table : m_tables)
                {
                const auto [row, col] = table->GetTableSize();
                maxRows = std::max(maxRows, row);
                maxCols = std::max(maxCols, col);
                }
            for (const auto& table : m_tables)
                {
                table->SetTableSize(maxRows, maxCols);
                }
            }

      private:
        size_t m_id{ 0 };
        std::vector<std::shared_ptr<Graphs::Table>> m_tables;
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_TABLELINK_H
