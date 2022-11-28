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

        /// @brief Get the brush scheme used for the bars.
        /// @returns The brush scheme used for the bars.
        [[nodiscard]] const std::shared_ptr<Brushes::Schemes::BrushScheme>& GetBrushScheme() const noexcept
            { return m_brushScheme; }
        /** @brief Sets the color scheme.
            @param colors The color scheme to use.
            @warning For most derived graphs, this will have no effect until `SetData()` is called.*/
        void SetBrushScheme(std::shared_ptr<Brushes::Schemes::BrushScheme> colors)
            { m_brushScheme = colors; }
        /// @brief Get the color scheme used for the bars.
        /// @returns The color scheme used for the bars.
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }
        /** @brief Sets the color scheme.
            @param colors The color scheme to use.
            @warning For most derived graphs, this will have no effect until `SetData()` is called.*/
        void SetColorScheme(std::shared_ptr<Colors::Schemes::ColorScheme> colors)
            { m_colorScheme = colors; }

        /// @brief Get the shape scheme used for the points.
        /// @returns The shape scheme used for the points.
        [[nodiscard]] const std::shared_ptr<Icons::Schemes::IconScheme>&
            GetShapeScheme() const noexcept
            { return m_shapeScheme; }
        /** @brief Sets the shape/icon scheme.
            @param shapes The shape scheme to use.
            @warning For most derived graphs, this will have no effect until `SetData()` is called.*/
        void SetShapeScheme(std::shared_ptr<Icons::Schemes::IconScheme> shapes)
            { m_shapeScheme = shapes; }
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
            @warning Ensure that `m_groupColumn` has been set to a valid colum before calling this
                and call `UseGrouping(true)` This should normally be done in a call to
                `SetData()` in derived classes..*/
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
            if (m_useGrouping)
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
        /// @brief Turns off the grouping flag and clears the ordered IDs.
        void ResetGrouping() noexcept
            {
            m_groupIds.clear();
            m_useGrouping = false;
            }

        /// @private
        void UseGrouping(const bool useGrouping) noexcept
            { m_useGrouping = useGrouping; }
        /// @private
        [[nodiscard]] bool IsUsingGrouping() const noexcept
            { return m_useGrouping; }

        /// @private
        [[nodiscard]] Wisteria::Data::CategoricalColumnConstIterator
            GetGroupColum()
            {
            wxASSERT_MSG(IsUsingGrouping(),
                L"Grouping must be enabled to access grouping column!");
            if (!IsUsingGrouping())
                {
                throw std::runtime_error(
                    _(L"Grouping must be enabled to access grouping column.")
                    .ToUTF8());
                }
            return m_groupColumn;
            }
        /// @private
        /// @warning Call `UseGrouping(true)` to enable grouping, as this will not
        ///     assume that the provided iterator is valid.
        void SetGroupColumn(Wisteria::Data::CategoricalColumnConstIterator iter)
            { m_groupColumn = iter; }

        /// @private
        /// @note Only access this when needing to validate
        Wisteria::Data::CategoricalColumnConstIterator m_groupColumn;
    private:
        bool m_useGrouping{ false };

        // cat ID and string order
        std::map<Data::GroupIdType, size_t> m_groupIds;
        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme{ nullptr };
        std::shared_ptr<Brushes::Schemes::BrushScheme> m_brushScheme{ nullptr };
        std::shared_ptr<Icons::Schemes::IconScheme> m_shapeScheme{ nullptr };
        };
    }

/** @}*/

#endif //__WISTERIA_GROUPGRAPH2D_H__
