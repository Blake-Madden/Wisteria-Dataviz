/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_ROADMAP_H
#define WISTERIA_ROADMAP_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief %Roadmap graphic, which shows items' positive and negative influence on a subject.
        @details This is an abstract base class.*/
    class Roadmap : public Graph2D
        {
        wxDECLARE_ABSTRACT_CLASS(Roadmap);
        Roadmap() = default;

      public:
        /// @brief Which type of markers to use for the road stops.
        enum class RoadStopTheme
            {
            /// @brief A Geolocation marker.
            LocationMarkers,
            /// @brief Warning and GO road signs.
            RoadSigns
            };

        /// @brief The style of the lane separator.
        enum class LaneSeparatorStyle
            {
            /// @brief Single line.
            SingleLine,
            /// @brief Double line.
            DoubleLine,
            /// @brief Do not draw a lane separator.
            NoDisplay
            };

        /// @brief How the labels next to the road stops are displayed.
        enum class MarkerLabelDisplay
            {
            /// @brief Just the influencer's name.
            Name,
            /// @brief The name and value (value can be negative,
            ///     in the case of a linear regression coefficient).
            NameAndValue,
            /// @brief The name and absolute value of the value.
            NameAndAbsoluteValue
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit Roadmap(Canvas* canvas);

        /** @brief Specifies how to arrange the location markers' names.
            @param lPlacement How to arrange the labels.*/
        void SetLabelPlacement(const LabelPlacement lPlacement) noexcept
            {
            m_labelPlacement = lPlacement;
            }

        /// @returns How the labels are arranged in the plotting area.
        [[nodiscard]]
        LabelPlacement GetLabelPlacement() const noexcept
            {
            return m_labelPlacement;
            }

        /// @brief Gets/sets the pen used for the road.
        /// @details The default is a black pavement, 10 DIPs wide.
        /// @details This is useful for changing the width or color of the road.
        /// @returns The pen used to draw the road.
        [[nodiscard]]
        wxPen& GetRoadPen() noexcept
            {
            return m_roadPen;
            }

        /// @brief Gets/sets the pen used to draw the lane separator on the road.
        /// @details This is useful for changing the color, pen style, or even removing the
        ///     line on the middle of the road.\n
        ///     The width of this pen will always be ignored, though, as the lane separator
        ///     will always be a tenth the width of the road.
        /// @note Set this to @c wxNullPen to not draw a line down the middle of the road.
        /// @returns The pen used to draw the lane separator.
        [[nodiscard]]
        wxPen& GetLaneSeparatorPen() noexcept
            {
            return m_laneSeparatorPen;
            }

        /** @brief Sets the icon theme for the road stops.
            @param theme Which theme to use.*/
        void SetRoadStopTheme(const RoadStopTheme theme) noexcept { m_iconTheme = theme; }

        /** @brief Builds and returns a legend.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

        /** @brief Sets how to display the labels next to the road stops.
            @param mlDisplay The display method to use.*/
        void SetMarkerLabelDisplay(const MarkerLabelDisplay mlDisplay) noexcept
            {
            m_markerLabelDisplay = mlDisplay;
            }

        /// @returns The lane separator style.
        [[nodiscard]]
        LaneSeparatorStyle GetLaneSeparatorStyle() const noexcept
            {
            return m_laneSeparatorStyle;
            }

        /// @brief Sets the lane separator style.
        /// @param lStyle The style to use.
        void SetLaneSeparatorStyle(const LaneSeparatorStyle lStyle) noexcept
            {
            m_laneSeparatorStyle = lStyle;
            }

        /// @brief Adds a caption explaining how to interpret the graph.
        virtual void AddDefaultCaption() = 0;

        /// @brief The maximum absolute value of the values (e.g., coefficients, counts, etc.).
        /// @details Essentially, this is the value of the most influential road stop
        ///     (either positive or negative).\n
        ///     For example, the values `{ -7, 1, 3 }` would have a magnitude @c 7.
        /// @returns The magnitude of the values.
        [[nodiscard]]
        double GetMagnitude() const noexcept
            {
            return m_magnitude;
            }

        /// @brief Sets the maximum absolute value of the values (e.g., coefficients, counts, etc.).
        /// @details This should be calculated in derived classes' @c SetData() function.\n
        ///     Client code would not normally need to call this. It can, however, be used
        ///     to set the same scale between two or more roadmaps being stacked into
        ///     one large road (refer to example code for @c ProConRoadmap).
        /// @param magnitude The maximum influence of the road stops.
        void SetMagnitude(const double magnitude) noexcept { m_magnitude = magnitude; }

        /// @private
        [[deprecated("Use version that takes a LegendOptions parameter.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendCanvasPlacementHint hint,
                                                        const bool includeHeader)
            {
            return CreateLegend(LegendOptions().IncludeHeader(includeHeader).PlacementHint(hint));
            }

      protected:
        /// @brief Description of icon used for a road stop.
        using RoadStopIcon = std::pair<Wisteria::Icons::IconShape, wxColour>;

        /// @brief A "stop on the road" (i.e., an IV from the multiple regression formula,
        ///     strength from a SWOT analysis, etc.), which causes a curve in the road
        ///     based on its influence.
        class RoadStopInfo
            {
          public:
            /// @brief Constructor.
            /// @param name The name of the influencer.
            explicit RoadStopInfo(const wxString& name) : m_name(name) {}

            /// @brief Sets the name of the influencer.
            /// @param name The name to display.
            /// @returns A self reference.
            RoadStopInfo& Name(const wxString& name)
                {
                m_name = name;
                return *this;
                }

            /// @brief Sets the item's value (e.g., coefficient in the equation),
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
            [[nodiscard]]
            double GetValue() const noexcept
                {
                return m_value;
                }

            /// @returns The name displayed on the road stop.
            [[nodiscard]]
            const wxString& GetName() const noexcept
                {
                return m_name;
                }

          private:
            double m_value{ 0 };
            wxString m_name;
            };

        /// @returns The positive label used for the legend.
        [[nodiscard]]
        virtual wxString GetPositiveLegendLabel() const = 0;
        /// @returns The negative label used for the legend.
        [[nodiscard]]
        virtual wxString GetNegativeLegendLabel() const = 0;

        /// @returns The name of the goal.
        [[nodiscard]]
        const wxString& GetGoalLabel() const noexcept
            {
            return m_goalLabel;
            }

        /// @brief Sets the name of the goal.
        /// @param label The goal name.
        void SetGoalLabel(const wxString& label) { m_goalLabel = label; }

        /// @returns The road stops.
        [[nodiscard]]
        std::vector<RoadStopInfo>& GetRoadStops() noexcept
            {
            return m_roadStops;
            }

      private:
        void RecalcSizes(wxDC& dc) final;

        std::vector<RoadStopInfo> m_roadStops;
        // (absolute) max of values (e.g., IVs' coefficients)
        double m_magnitude{ 0 };
        wxString m_goalLabel{ _(L"Goal") };

        wxPen m_roadPen{ *wxBLACK, 10 };
        wxPen m_laneSeparatorPen{ wxPenInfo(
            Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow), 1,
            wxPenStyle::wxPENSTYLE_LONG_DASH) };
        LaneSeparatorStyle m_laneSeparatorStyle{ LaneSeparatorStyle::SingleLine };
        RoadStopTheme m_iconTheme{ RoadStopTheme::LocationMarkers };

        /// @returns The icon used for negative sentiments, based on current theme.
        [[nodiscard]]
        RoadStopIcon GetNegativeIcon() const noexcept
            {
            return m_iconTheme == RoadStopTheme::LocationMarkers ?
                       std::make_pair(Wisteria::Icons::IconShape::LocationMarker,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Tomato)) :
                       std::make_pair(
                           Wisteria::Icons::IconShape::WarningRoadSign,
                           Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow));
            }

        /// @returns The icon used for positive sentiments, based on current theme.
        [[nodiscard]]
        RoadStopIcon GetPositiveIcon() const noexcept
            {
            return m_iconTheme == RoadStopTheme::LocationMarkers ?
                       std::make_pair(Wisteria::Icons::IconShape::LocationMarker,
                                      Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen)) :
                       std::make_pair(Wisteria::Icons::IconShape::GoRoadSign,
                                      Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen));
            }

        LabelPlacement m_labelPlacement{ LabelPlacement::Flush };
        MarkerLabelDisplay m_markerLabelDisplay{ MarkerLabelDisplay::NameAndValue };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_ROADMAP_H
