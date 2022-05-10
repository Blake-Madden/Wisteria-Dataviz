/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_ROADMAP_GRAPH_H__
#define __WISTERIA_ROADMAP_GRAPH_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    class LRRoadmap final : public Graph2D
        {
    public:
        

        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit LRRoadmap(Canvas* canvas);
        /** @brief Sets the data.
            @details Along with the X and Y points, separate lines will be created based on
             the grouping column in the data. The group ID assigned to each line will also
             select which color, marker shape, and line style to use.
            @param data The data to use for the plot.
            @param predictorColumnName The column containing the independent variables'
             (i.e., predictors) names.
            @param coefficentColumnName The column containing the predictors' correlation coefficients.
            @param pValueColumnName The (optional) column containing the predictors' p-values.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& predictorColumnName,
                     const wxString& coefficentColumnName,
                     const std::optional<wxString>& pValueColumnName);

        /** @brief Specifies how to arrange the location markers' names.
            @param lPlacement How to arrange the labels.*/
        void GetLabelPlacement(const LabelPlacement lPlacement) noexcept
            { m_labelPlacement = lPlacement; }
        /// @returns How the labels are arranged in the plotting area.
        [[nodiscard]] LabelPlacement GetLabelPlacement() const noexcept
            { return m_labelPlacement; }
    public:
        void RecalcSizes(wxDC& dc) final;

        /// @brief A "stop on the road" (i.e., an IV from the multiple regression formula),
        ///  which causes a curve in the road based on its coefficient.
        class RoadStopInfo
            {
            friend class LRRoadmap;
        public:
            /// @brief Constructor.
            /// @param name The name of the IV.
            explicit RoadStopInfo(const wxString& name) : m_name(name) {}
            /// @brief Sets the name of the factor/IV.
            /// @param name The name of the IV.
            /// @returns A self reference.
            RoadStopInfo& Name(const wxString& name)
                {
                m_name = name;
                return *this;
                }
            /// @brief Sets the factor's coefficent in the equation,
            ///  which controls the direction and length of a curve in the road.
            /// @param t The IV's coefficient.
            /// @returns A self reference.
            RoadStopInfo& Coefficient(double coefficient)
                {
                m_coefficent = coefficient;
                return *this;
                }
        private:
            double m_coefficent{ 0 };
            wxString m_name;
            };

        std::vector<RoadStopInfo> m_roadStops;
        // min and max of IVs' coefficients (i.e., estimated effects)
        std::pair<double, double> m_coefficientsRange;

        LabelPlacement m_labelPlacement{ LabelPlacement::FlushBoth };
        };
    }

/** @}*/

#endif //__WISTERIA_ROADMAP_GRAPH_H__
