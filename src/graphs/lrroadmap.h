/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_LRROADMAP_H
#define WISTERIA_LRROADMAP_H

#include "roadmap.h"

namespace Wisteria::Graphs
    {
    /** @brief Linear Regression %Roadmap, which shows predictors' influence on a dependent variable
            from a multiple linear regression.
        @details This graphic displays a road leading towards a final goal (i.e., the dependent).
            Along this road are "road stops" of variable size that cause the road to curve. These
            road stops represent the independent variables from a linear regression. Both the
            size of a road stop and the curve in the road next to it represent the strength of the
            predictor's influence. Additionally, stops on the right side of the road are positive
            influencers, left stops are negative.
        @par Roadmap displaying all factors and their level of influence:

        @image html LRRoadmapFirstYear.svg width=90%
        @par Roadmap showcasing only negative factors:

        @image html LRRoadmapFirstYearNegative.svg width=90%
        @par %Data:
            This graph accepts a Data::Dataset where one categorical column is the predictor names,
            a continuous column is the coefficients, and an optional continuous column is the
            p-values. (The p-values are used for filtering which predictors to include.)

         | Factor                            | Coefficient | p-value |
         | :--                               | --:         | --:     |
         | Being female                      | 0.19        | 0.009   |
         | Being an athlete                  | 0.29        | 0.001   |
         | Being older, closer to 26 than 18 | -0.17       | 0.002   |
         ...

        @par Missing Data:
            Missing coefficients or p-values will result in list-wise deletion. Missing predictor
            names will be displayed as empty strings.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the graph
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(2, 1);

         auto roadmapData = std::make_shared<Data::Dataset>();
         try
            {
            roadmapData->ImportCSV(L"/home/mwalker/data/First-Year Osprey.csv",
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
            std::nullopt, std::nullopt, std::nullopt, _(L"GPA"));
         roadmap->SetCanvasMargins(5, 5, 5, 5);
         // add the default caption explaining how to read the graph
         roadmap->AddDefaultCaption();
         roadmap->GetTitle().SetText(_(L"First-Year Osprey Roadmap\n"
            "How do background characteristics and decisions affect First - Year Students' GPA?"));
         // add a title with a blue banner background and white font
         roadmap->GetTitle().GetHeaderInfo().Enable(true).
            FontColor(Colors::ColorBrewer::GetColor(Colors::Color::White)).GetFont().MakeBold();
         roadmap->GetTitle().SetPadding(5, 5, 5, 5);
         roadmap->GetTitle().SetFontColor(Colors::ColorBrewer::GetColor(Colors::Color::White));
         roadmap->GetTitle().SetFontBackgroundColor(
            ColorBrewer::GetColor(Colors::Color::NavyBlue));

         canvas->SetFixedObject(0, 0, roadmap);

         // add the legend at the bottom (beneath the explanatory caption)
         auto legend = roadmap->CreateLegend(
            LegendOptions().
                IncludeHeader(true).
                PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
         canvas->GetRowInfo(0).HeightProportion(1-canvas->CalcMinHeightProportion(legend));
         canvas->GetRowInfo(1).HeightProportion(canvas->CalcMinHeightProportion(legend));
         canvas->SetFixedObject(1, 0, legend);

        @endcode
        @par Citation:
            Kulp, A., &amp; Grandstaff, M. (2019, April 17). <i>Visualizing regression results for
            non-statistics audiences.</i> Retrieved May 14, 2022, from
            https://www.airweb.org/article/2019/04/17/visualizing-regression-results-for-non-statistics-audiences
    */
    class LRRoadmap final : public Roadmap
        {
        wxDECLARE_DYNAMIC_CLASS(LRRoadmap);
        LRRoadmap() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit LRRoadmap(Canvas* canvas) : Roadmap(canvas) {}

        /** @brief Sets the data.
            @param data The data to use for the graph.
            @param predictorColumnName The column containing the independent variables'
                (i.e., predictors) names.
            @param coefficientColumnName The column containing the predictors'
                regression coefficients.
            @param pValueColumnName The (optional) column containing the predictors' p-values.
            @param pLevel If a p-value column is supplied, only predictors with p-values lower than
                this will be included. (Predictors with missing p-values will be excluded.)\n
                The recommendations are usually @c 0.05 or @c 0.01 (most strict).
            @param predictorsToIncludes Which types of IVs (e.g., negative influencers) to
                include.\n
                This is a bitmask that can include multiple flags.
                The default is to include all IVs.
            @param dvName The name of the dependent variable from the original analysis.\n
                This will be used on the legend and default caption.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to @c
                wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& predictorColumnName, const wxString& coefficientColumnName,
                     const std::optional<wxString>& pValueColumnName = std::nullopt,
                     std::optional<double> pLevel = std::nullopt,
                     std::optional<Influence> predictorsToIncludes = std::nullopt,
                     const std::optional<wxString>& dvName = std::nullopt);

        /// @brief Adds a caption explaining how to interpret the graph.
        void AddDefaultCaption() override final;

      private:
        /// @returns The positive label used for the legend.
        [[nodiscard]]
        wxString GetPositiveLegendLabel() const override final
            {
            return wxString::Format(_(L"Positively associated with %s"), GetGoalLabel());
            }

        /// @returns The negative label used for the legend.
        [[nodiscard]]
        wxString GetNegativeLegendLabel() const override final
            {
            return wxString::Format(_(L"Negatively associated with %s"), GetGoalLabel());
            }
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_LRROADMAP_H
