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
    /** @brief Roadmap graphic, which shows items' positive and negative influence on a subject.
        @details This is an abstract base class.*/
    class Roadmap : public Graph2D
        {
    public:
        /// @brief How the labels next to the road stops are displayed.
        enum class MarkerLabelDisplay
            {
            /// @brief Just the influencer's name.
            Name,
            /// @brief The name and value (value can be negative,
            ///     in the case of a linear regression coefficient).
            NameAndValue,
            /// @brief The name and absolutue value of the value.
            NameAndAbsoluteValue
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit Roadmap(Canvas* canvas);

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

        /** @brief Sets how to display the labels next to the road stops.
            @param mlDisplay The display method to use.*/
        void SetMarkerLabelDisplay(const MarkerLabelDisplay mlDisplay) noexcept
            { m_markerLableDisplay = mlDisplay; }

        /// @brief Adds a caption explaining how to interpret the graph.
        virtual void AddDefaultCaption() = 0;
    protected:
        /// @brief A "stop on the road" (i.e., an IV from the multiple regression formula,
        ///     strength from a SWOT analysis, etc.), which causes a curve in the road
        ///     based on its influence.
        class RoadStopInfo
            {
        public:
            /// @brief Constructor.
            /// @param name The name of the influencer.
            explicit RoadStopInfo(const wxString& name) : m_name(name)
                {}
            /// @brief Sets the name of the influencer.
            /// @param name The name to display.
            /// @returns A self reference.
            RoadStopInfo& Name(const wxString& name)
                {
                m_name = name;
                return *this;
                }
            /// @brief Sets the item's value (e.g., coefficent in the equation),
            ///     which controls the direction and length of a curve in the road.
            /// @details Negative values will place the item on the left side of the graph,
            ///     positive will appear on the right side.
            /// @param value The item's value.
            /// @returns A self reference.
            RoadStopInfo& Value(double value) noexcept
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

        /// @returns The positive label used for the legend.
        [[nodiscard]] virtual wxString GetPositiveLegendLabel() const = 0;
        /// @returns The negative label used for the legend.
        [[nodiscard]] virtual wxString GetNegativeLegendLabel() const = 0;
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
        /// @brief The range of the values, adjusted to be in terms of magnitude.
        /// @details For example, { -7, 1, 3 } should have a magnitude range of 1-7.
        /// @returns The magnitude range.
        [[nodiscard]] std::pair<double, double>& GetMagnitudeRange() noexcept
            { return m_valuesRange; }
    private:
        void RecalcSizes(wxDC& dc) final;

        std::vector<RoadStopInfo> m_roadStops;
        // min and max of values (e.g., IVs' coefficients)
        std::pair<double, double> m_valuesRange;
        wxString m_goalLabel{ _("Goal") };

        wxPen m_roadPen{ *wxBLACK, 10 };

        LabelPlacement m_labelPlacement{ LabelPlacement::FlushBoth };
        MarkerLabelDisplay m_markerLableDisplay{ MarkerLabelDisplay::NameAndValue };
        };
    }

/** @}*/

#endif //__WISTERIA_ROADMAP_H__