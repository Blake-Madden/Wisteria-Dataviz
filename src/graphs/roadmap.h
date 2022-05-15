/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_ROADMAP_H__
#define __WISTERIA_ROADMAP_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Base roadmap graphic, which shows items' positive and negative influence on a subject.*/
    class Roadmap : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit Roadmap(Canvas* canvas);
        /** @brief Sets the data.
            @param data The data to use for the plot.
            @param influencerColumnName The column containing the influencers' names.
            @param valueColumnName The column containing the influencers' values.
            @param filterCriteriaColumnName The (optional) column containing values to filter the
                influencers on.
            @param filterValue If a filter column is supplied, the value to filter with.\n
                This will be context dependent on how the derived graph uses it.
            @param influencersToIncludes Which types of influencers (e.g., negative) to include.\n
                This is a bitmask that can include multiple flags. The default is to include all influencers.
            @param goalName The name of the goal (e.g., a dependent variable from a regression formula).\n
                This will be used on the legend and default caption.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
                when formatting it for an error message.*/
        virtual void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& influencerColumnName,
                     const wxString& valueColumnName,
                     const std::optional<wxString>& filterCriteriaColumnName = std::nullopt,
                     const std::optional<double> filterValue = std::nullopt,
                     const std::optional<Influence> influencersToIncludes = std::nullopt,
                     const std::optional<wxString> goalName = std::nullopt) = 0;

        /** @brief Specifies how to arrange the location markers' names.
            @param lPlacement How to arrange the labels.*/
        void GetLabelPlacement(const LabelPlacement lPlacement) noexcept
            { m_labelPlacement = lPlacement; }
        /// @returns How the labels are arranged in the plotting area.
        [[nodiscard]] LabelPlacement GetLabelPlacement() const noexcept
            { return m_labelPlacement; }

        /// @brief Gets/sets the pen used for the road.
        /// @details This is useful for changing the width or color of the road.
        /// @returns The pen used to draw the road.
        [[nodiscard]] wxPen& GetRoadPen() noexcept
            { return m_roadPen; }

        /** @brief Builds and returns a legend.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
                This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @param includeHeader `true` to show the grouping column name as the header.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint,
            const bool includeHeader);

        /// @brief Adds a caption explaining how to interpret the graph.
        virtual void AddDefaultCaption() = 0;
    protected:
        /// @returns The positive label used for the legend.
        [[nodiscard]] virtual wxString GetPositiveLegendLabel() const = 0;
        /// @returns The negative label used for the legend.
        [[nodiscard]] virtual wxString GetNegativeLegendLabel() const = 0;
        /// @brief A "stop on the road" (i.e., an IV from the multiple regression formula,
        ///     strength from a SWOT analysis, etc.), which causes a curve in the road
        ///     based on its coefficient.
        class RoadStopInfo
            {
        public:
            /// @brief Constructor.
            /// @param name The name of the IV, strength, etc.
            explicit RoadStopInfo(const wxString& name) : m_name(name) {}
            /// @brief Sets the name of the IV, strength, etc.
            /// @param name The name to display.
            /// @returns A self reference.
            RoadStopInfo& Name(const wxString& name)
                {
                m_name = name;
                return *this;
                }
            /// @brief Sets the item's value (e.g., coefficent in the equation),
            ///     which controls the direction and length of a curve in the road.
            /// @param value The item's value.
            /// @returns A self reference.
            RoadStopInfo& Value(double value)
                {
                m_value = value;
                return *this;
                }
            /// @returns The value of the road stop.
            [[nodiscard]] double GetValue() const noexcept
                { return m_value; }
            /// @returns The name displayed on the road stop.
            [[nodiscard]] const wxString& GetName() const noexcept
                { return m_name; }
        private:
            double m_value{ 0 };
            wxString m_name;
            };
        /// @returns The name of the goal.
        [[nodiscard]] const wxString& GetGoalLabel() const noexcept
            { return m_goalLabel; }
        /// @brief  Sets the name of the goal.
        /// @param label The goal name.
        void SetGoalLabel(const wxString& label)
            { m_goalLabel = label; }
        /// @returns The road stops.
        [[nodiscard]] std::vector<RoadStopInfo>& GetRoadStops() noexcept
            { return m_roadStops; }
        /// @returns The range of the values loaded from SetData().
        [[nodiscard]] std::pair<double, double>& GetValuesRange() noexcept
            { return m_valuesRange; }
    private:
        void RecalcSizes(wxDC& dc) final;

        std::vector<RoadStopInfo> m_roadStops;
        // min and max of values (e.g., IVs' coefficients)
        std::pair<double, double> m_valuesRange;
        wxString m_goalLabel{ _("DV") };

        wxPen m_roadPen{ *wxBLACK, 10 };

        LabelPlacement m_labelPlacement{ LabelPlacement::FlushBoth };
        };
    }

/** @}*/

#endif //__WISTERIA_ROADMAP_H__
