/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_GROUPGRAPH2D_H__
#define __WISTERIA_GROUPGRAPH2D_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A graph that may have grouping used in it.
        @details SetGroupColumn() & GetGroupColumn() are provided to connect to a grouping
            column when setting the data, which can used by calling UseGrouping().\n
            This class will handle mapping the group codes in alphabetical order
            to the brush and color schemes (also provided in this class). It will also
            handle building a legend in alphabetical order (this can still be
            overridden in derived classes).*/
    class GroupGraph2D : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The parent canvas that the plot is being drawn on.*/
        explicit GroupGraph2D(Canvas* canvas) : Graph2D(canvas)
            {}
        /// @private
        GroupGraph2D(const GroupGraph2D&) = delete;
        /// @private
        GroupGraph2D(GroupGraph2D&&) = delete;
        /// @private
        GroupGraph2D& operator=(const GroupGraph2D&) = delete;
        /// @private
        GroupGraph2D& operator=(GroupGraph2D&&) = delete;

        /** @brief Builds and returns a legend.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendOptions& options) override;

        /// @returns The number of subgroups found during the last call to SetData().\n
        ///     This is only relevant if using the secondary grouping variable.
        [[nodiscard]] size_t GetGroupCount() const noexcept
            { return m_groupIds.size(); }
    protected:
        /** @private
            @brief Builds a list of group IDs, sorted their respective strings' alphabetical order.
            @details The map's key is the group ID, and the value is its index in the map.\n
                This value is useful for mapping group IDs to an index in the various schemes
                (e.g., color scheme).\n
                The ordering of this map can also be used to build a legend, where the
                group IDs are sorted in their respective label's alphabetical order.
            @warning Ensure that SetGroupColumn() has been passed a valid column before calling this
                and call `UseGrouping(true)` This should normally be done in a call to
                `SetData()` in derived classes.*/
        void BuildGroupIdMap();

        /** @private
            @returns The ordered position of a group ID, or @c 0 if grouping is not in use.
            @throws std::runtime_error If the ID can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @note Call BuildGroupIdMap() prior to this to load the ordered codes and their
                respective positions.*/
        [[nodiscard]] size_t GetSchemeIndexFromGroupId(const Wisteria::Data::GroupIdType Id)
            {
            if (IsUsingGrouping())
                {
                const auto pos = m_groupIds.find(Id);
                wxASSERT_MSG((pos != m_groupIds.cend()),
                             L"Error finding scheme index for group!");
                if (pos != m_groupIds.cend())
                    { return pos->second; }
                else
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"%zu: code not found in categorical data."),
                        Id).ToUTF8());
                    }
                }
            else
                { return 0; }
            }

        /// @private
        /// @brief Invalidates the group column pointer and clears the ordered IDs.
        void ResetGrouping() noexcept
            {
            m_groupIds.clear();
            m_groupColumn = nullptr;
            }

        /// @returns @c true if the grouping column is set.
        [[nodiscard]] bool IsUsingGrouping() const noexcept
            { return (m_groupColumn != nullptr); }

        /// @brief Sets the shape to use in the legend (if a shape scheme isn't in use).
        /// @param shape The shape to use.
        void SetDefaultLegendShape(const Wisteria::Icons::IconShape& shape) noexcept
            { m_defaultLegendShape = shape; }

        /// @private
        [[nodiscard]] const Wisteria::Data::ColumnWithStringTable* GetGroupColumn() const
            { return m_groupColumn;  }
        /// @private
        void SetGroupColumn(const Wisteria::Data::ColumnWithStringTable* groupColumn)
            { m_groupColumn = groupColumn; }

        /// @private
        /// @brief Sets the grouping column (or keep it as null if not in use).
        /// @warning Call SetDataset() first before calling this.
        void SetGroupColumn(const std::optional<const wxString> groupColumnName = std::nullopt);
    private:
        [[nodiscard]] const std::map<Data::GroupIdType, size_t>& GetGroupIds() const noexcept
            { return m_groupIds; }

        Wisteria::Icons::IconShape m_defaultLegendShape{ Wisteria::Icons::IconShape::Square };

        // cat ID and string order
        std::map<Data::GroupIdType, size_t> m_groupIds;
        const Wisteria::Data::ColumnWithStringTable* m_groupColumn{ nullptr };
        };
    }

/** @}*/

#endif //__WISTERIA_GROUPGRAPH2D_H__
