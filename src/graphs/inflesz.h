/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_INFLESZ_CHART_H
#define WISTERIA_INFLESZ_CHART_H

#include "scalechart.h"

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A Flesch Reading Ease like chart for Spanish.
        @details This includes the Inflesz scale, as well as the Szigriszt
            and Flesch Reading Ease scales for comparison.

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the values(s).
         The ID column's labels will be associated with each point,
         so it is recommended to fill this column with meaningful names.

         A categorical column can also optionally be used as a grouping variable.

         | ID     | Score | Group    |
         | :--    | --:   | --:      |
         | week 1 | 52    | Topic 1  |
         | week 2 | 50    | Topic 1  |
         | week 1 | 62    | Topic 2  |
         ...

        @par Missing Data:
         - Values that are missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

        @par Citations:
         Barrio-Cantalejo, I M et al. “Validación de la Escala INFLESZ para evaluar la legibilidad 
         de los textos dirigidos a pacientes”
         [Validation of the INFLESZ scale to evaluate readability of texts aimed at the patient].
         Anales del sistema sanitario de Navarra vol. 31,2 (2008): 135-52. doi:10.4321/s1137-66272008000300004*/
    // clang-format on
    class InfleszChart final : public Wisteria::Graphs::ScaleChart
        {
        wxDECLARE_DYNAMIC_CLASS(InfleszChart);
        InfleszChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
                Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as null to use the standard shapes.
            @param includeSzigriszt Whether to include the Szigriszt
                scale for comparison.
            @param includeFlesch Whether to include the FRE scale
                for comparison.*/
        explicit InfleszChart(
            Wisteria::Canvas* canvas,
            std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes = nullptr,
            const bool includeSzigriszt = true, const bool includeFlesch = true);
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_INFLESZ_CHART_H
