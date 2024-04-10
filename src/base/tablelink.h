/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_TABLELINK_H__
#define __WISTERIA_TABLELINK_H__

#include "../graphs/table.h"

namespace Wisteria
    {
    /// @brief Links multiple tables, ensuring that they have the same dimensions.
    class TableLink
        {
      public:
        /// @brief Constructor.
        /// @param Id The unique ID for this table linker.
        explicit TableLink(const size_t Id) : m_Id(Id) {}

        /// @returns The table linker's ID.
        [[nodiscard]]
        size_t GetId() const noexcept
            {
            return m_Id;
            }

        /// @private
        [[nodiscard]]
        bool
        operator<(const TableLink& that) const noexcept
            {
            return m_Id < that.m_Id;
            }

        /// @brief Adds a table to the list of connected tables.
        /// @param table The table to add.
        void AddTable(std::shared_ptr<Graphs::Table> table)
            {
            m_tables.push_back(std::move(table));
            }

        /// @brief Syncs the dimensions of the tables, so that they all have a minimum
        ///     number of rows and columns (based on the largest number of rows and columns).
        void SyncTableSizes()
            {
            if (m_tables.size() == 0)
                {
                return;
                }
            const auto maxRowCount =
                std::max_element(m_tables.cbegin(), m_tables.cend(),
                                 [](const auto& lhv, const auto& rhv) noexcept
                                 { return lhv->GetTableSize().first < rhv->GetTableSize().first; })
                    ->get()
                    ->GetTableSize()
                    .first;
            const auto maxColumnCount =
                std::max_element(m_tables.cbegin(), m_tables.cend(),
                                 [](const auto& lhv, const auto& rhv) noexcept {
                                     return lhv->GetTableSize().second < rhv->GetTableSize().second;
                                 })
                    ->get()
                    ->GetTableSize()
                    .second;
            for (const auto& table : m_tables)
                {
                table->SetTableSize(maxRowCount, maxColumnCount);
                }
            }

      private:
        size_t m_Id{ 0 };
        std::vector<std::shared_ptr<Graphs::Table>> m_tables;
        };
    } // namespace Wisteria

/** @}*/

#endif //__WISTERIA_TABLELINK_H__
