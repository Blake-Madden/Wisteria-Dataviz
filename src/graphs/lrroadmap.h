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
    /** @brief Linear Regression Roadmap, which shows predictors' influence on a dependent variable
            from a multiple linear regression.
        @details This graphic displays a road leading towards a final goal (i.e., the dependent).
            Along this road are "road stops" of variable size that cause the road to curve. These
            road stops represent one of the independent variables from a linear regression. Both the
            size of the road stop and the curve in the road represent the strength of the predictor's
            influence. Additionally, stops on the right side of the road are positive influencers,
            left stops are negative.
        @image html LRRoadmapFirstYear.svg width=90%
        @par %Data:
            This plot accepts a Data::Dataset where one categorical column is the preditor names,
            a continuous column is their coefficients, and an optional continuous column are their
            p-values. (The p-values are only used for filtering which predictors to include.)

         | Factor                              | Coefficient | p-value |
         | :--                                 | --:         | :--     |
         | Being female                        | 0.19        | 0.009   |
         | Being an athlete                    | 0.29        | 0.001   |
         | "Being older, closer to 26 than 18" | -0.17       | 0.002   |

         ...

        @par Missing Data:
            Missing coefficients or p-values will result in list-wise deletion. Missing predictor
            names will be displayed as empty strings.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(2, 1);

         auto roadmapData = std::make_shared<Data::Dataset>();
         try
            {
            roadmapData->ImportCSV(appDir + L"/datasets/First-Year Osprey.csv",
                ImportInfo().
                ContinuousColumns({ L"coefficient" }).
                CategoricalColumns({ { L"factor", CategoricalImportMethod::ReadAsStrings } }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto roadmap = std::make_shared<LRRoadmap>(canvas);
         roadmap->SetData(roadmapData, L"factor", L"coefficient",
            std::nullopt, std::nullopt, std::nullopt, _("GPA"));
         roadmap->SetCanvasMargins(5, 5, 5, 5);
         // add the default caption explaining how to read the graph
         roadmap->AddDefaultCaption();
         roadmap->GetTitle().SetText(_("First-Year Osprey Roadmap\n"
            "How do background characteristics and decisions affect First - Year Students' GPA?"));
         // add a title with a blue banner background and white font
         roadmap->GetTitle().GetHeaderInfo().Enable(true).
            FontColor(*wxWHITE).GetFont().MakeBold();
         roadmap->GetTitle().SetPadding(5, 5, 5, 5);
         roadmap->GetTitle().SetFontColor(*wxWHITE);
         roadmap->GetTitle().SetFontBackgroundColor(
            ColorBrewer::GetColor(Colors::Color::NavyBlue));

         canvas->SetFixedObject(0, 0, roadmap);

         // add the legend at the bottom (beneath the explanatory caption)
         auto legend = roadmap->CreateLegend(LegendCanvasPlacementHint::AboveOrBeneathGraph, true);
         canvas->SetRowProportion(0, 1-canvas->CalcMinHeightProportion(legend));
         canvas->SetRowProportion(1, canvas->CalcMinHeightProportion(legend));
         canvas->SetFixedObject(1, 0, legend);
         
        @endcode
        @par Citation:
            https://www.airweb.org/article/2019/04/17/visualizing-regression-results-for-non-statistics-audiences
    */
    class LRRoadmap final : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit LRRoadmap(Canvas* canvas);
        /** @brief Sets the data.
            @param data The data to use for the plot.
            @param predictorColumnName The column containing the independent variables'
                (i.e., predictors) names.
            @param coefficentColumnName The column containing the predictors' correlation coefficients.
            @param pValueColumnName The (optional) column containing the predictors' p-values.
            @param preditorsToIncludes Which types of IVs (e.g., negative influence) to include.\n
                This is a bitmask that can include multiple flags. The default is to include all IVs.
            @param pLevel If a p-value column is supplied, only predictors with p-values lower than
                this will be included. (Predictors with missing p-values will be excluded.)\n
                The recommendations are usually @c .05 or @c .01 (most strict).
            @param dvName The name of the dependent variable from the original analysis.
                This will be used on the legend and default caption.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
                when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& predictorColumnName,
                     const wxString& coefficentColumnName,
                     const std::optional<wxString>& pValueColumnName = std::nullopt,
                     const std::optional<double> pLevel = std::nullopt,
                     const std::optional<Predictors> preditorsToIncludes = std::nullopt,
                     const std::optional<wxString> dvName = std::nullopt);

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
        void AddDefaultCaption();
    private:
        void RecalcSizes(wxDC& dc) final;

        /// @brief A "stop on the road" (i.e., an IV from the multiple regression formula),
        ///     which causes a curve in the road based on its coefficient.
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
            ///     which controls the direction and length of a curve in the road.
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
        wxString m_dvLabel{ _("DV") };

        wxPen m_roadPen{ *wxBLACK, 10 };

        LabelPlacement m_labelPlacement{ LabelPlacement::FlushBoth };
        };
    }

/** @}*/

#endif //__WISTERIA_ROADMAP_GRAPH_H__
