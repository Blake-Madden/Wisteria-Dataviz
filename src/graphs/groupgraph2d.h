/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_GROUPGRAPH2D_H
#define WISTERIA_GROUPGRAPH2D_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A graph that may have grouping used in it.
        @details SetGroupColumn() and GetGroupColumn() are provided to connect to a grouping
            column when setting the data, which can used by calling UseGrouping().\n
            This class will handle mapping the group codes in alphabetical order
            to the brush and color schemes (also provided in this class). It will also
            handle building a legend in alphabetical order (this can still be
            overridden in derived classes).*/
    class GroupGraph2D : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(GroupGraph2D);
        GroupGraph2D() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas that the plot is being drawn on.*/
        explicit GroupGraph2D(Canvas* canvas) : Graph2D(canvas) {}

        /// @private
        GroupGraph2D(const GroupGraph2D&) = delete;
        /// @private
        GroupGraph2D& operator=(const GroupGraph2D&) = delete;

        /** @brief Builds and returns a legend.
            @details This can be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

        /// @returns The number of subgroups found during the last call to SetData().\n
        ///     This is only relevant if using the secondary grouping variable.
        [[nodiscard]]
        size_t GetGroupCount() const noexcept
            {
            return m_groupIds.size();
            }

        /// @returns @c true if a grouping column is in use.
        [[nodiscard]]
        bool IsUsingGrouping() const noexcept
            {
            return m_groupColumn.has_value();
            }

      protected:
        /** @private
            @brief Builds a list of group IDs, sorted by their respective strings'
                alphabetical order.
            @details The map's key is the group ID, and the value is its index in the map.\n
                This value is useful for mapping group IDs to an index in the various schemes
                (e.g., color scheme).\n
                The ordering of this map can also be used to build a legend, where the
                group IDs are sorted in their respective labels' alphabetical order.
            @warning Ensure that SetGroupColumn() has been passed a valid column before
                calling this. This should normally be done in a call to `SetData()`
                in derived classes.*/
        void BuildGroupIdMap();

        /** @private
            @param identifier The group ID.
            @returns The ordered position of a group ID, or @c 0 if grouping is not in use.
            @throws std::runtime_error If the ID can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @note Call BuildGroupIdMap() prior to this to load the ordered codes and their
                respective positions.*/
        [[nodiscard]]
        size_t GetSchemeIndexFromGroupId(const Wisteria::Data::GroupIdType identifier) const
            {
            if (IsUsingGrouping())
                {
                const auto pos = m_groupIds.find(identifier);
                wxASSERT_MSG((pos != m_groupIds.cend()), L"Error finding scheme index for group!");
                if (pos != m_groupIds.cend())
                    {
                    return pos->second;
                    }
                throw std::runtime_error(
                    wxString::Format(_(L"%zu: code not found in categorical data."), identifier)
                        .ToUTF8());
                }
            return 0;
            }

        /// @private
        /// @brief Invalidates the group column pointer and clears the ordered IDs.
        void ResetGrouping() noexcept
            {
            m_groupIds.clear();
            m_groupColumn.reset();
            }

        /// @brief Sets the shape to use in the legend (if a shape scheme isn't in use).
        /// @param shape The shape to use.
        void SetDefaultLegendShape(const Wisteria::Icons::IconShape& shape) noexcept
            {
            m_defaultLegendShape = shape;
            }

        /** @brief Returns the grouping column.
            @returns An iterator to the grouping column. If not found, throws.
            @note Call SetGroupColumn() first to set the grouping column.
            @throws std::runtime_error If the column is not found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        auto GetGroupColumn() const
            {
            wxASSERT_MSG(GetDataset(),
                         L"You must call SetDataset() before calling SetGroupColumn()!");
            if (GetDataset() == nullptr)
                {
                throw std::runtime_error(
                    wxString(_(L"Dataset not set before calling SetGroupColumn().")).ToUTF8());
                }
            const auto groupColIter =
                m_groupColumn ? GetDataset()->GetCategoricalColumn(m_groupColumn.value()) :
                                GetDataset()->GetCategoricalColumns().cend();
            if (m_groupColumn && groupColIter == GetDataset()->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': group column not found for graph."),
                                     m_groupColumn.value())
                        .ToUTF8());
                }
            return groupColIter;
            }

        /// @brief Sets the grouping column (or keep it as `std::nullopt` if not in use).
        /// @param groupColumnName The group column's name.
        /// @warning Call SetDataset() first before calling this.
        void SetGroupColumn(const std::optional<wxString>& groupColumnName = std::nullopt)
            {
            m_groupColumn = groupColumnName;
            }

      private:
        [[nodiscard]]
        const std::map<Data::GroupIdType, size_t>& GetGroupIds() const noexcept
            {
            return m_groupIds;
            }

        Wisteria::Icons::IconShape m_defaultLegendShape{ Wisteria::Icons::IconShape::Square };

        // cat ID and string order
        std::map<Data::GroupIdType, size_t> m_groupIds;
        std::optional<wxString> m_groupColumn{ std::nullopt };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_GROUPGRAPH2D_H
