/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef LIX_GAUGE_H
#define LIX_GAUGE_H

#include "../base/colorbrewer.h"
#include "../data/jitter.h"
#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A plot showing Lix (Läsbarhetsindex) readability scores and what they represent.
        @image html LixGauge.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the Lix score(s)
         for document (or samples). The ID column's labels will be associated with each point,
         so it is recommended to fill this column with the documents' (or samples') names.

         A categorical column can also optionally be used as a grouping variable.

         | ID            | Score | Group     |
         | :--           | --:   | --:       |
         | ImportingData | 52    | Examples  |
         | ExportingData | 50    | Examples  |
         | Welcome       | 62    | Overviews |

        @par Missing Data:
         - Scores that are missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

        @par Citation:
            Björnsson, C.H. “Readability of Newspapers in 11 Languages.” *Reading Research
            Quarterly*, vol. 18, no. 4, 1983, pp. 480-97.
    */
    class LixGauge final : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(LixGauge);

        LixGauge() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.*/
        explicit LixGauge(Canvas* canvas,
                          const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
                          const std::shared_ptr<Icons::Schemes::IconScheme>& shapes = nullptr);

        /** @brief Sets the data.
            @param data The data to use.
            @param scoreColumnName The column containing the documents' scores
                (a continuous column).
            @param groupColumnName The (optional) categorical column to use for grouping.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& scoreColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @returns Whether the score is being showcased.
        /// @sa ShowcaseScore().
        [[nodiscard]]
        bool IsShowcasingScore() const noexcept
            {
            return m_showcaseScore;
            }

        /// @brief Makes most areas of the graph translucent, except for where the score is.
        ///     This helps draw attention to the areas of the scales that have scores falling
        ///     into them.
        /// @param showcase @c true to showcase where the score is.
        /// @note If there are multiple scores, then every area that has a score in it
        ///     will be showcased.
        void ShowcaseScore(const bool showcase) noexcept { m_showcaseScore = showcase; }

      private:
        void RecalcSizes(wxDC& dc) final;

        void AdjustAxes();

        void UpdateCustomAxes();

        const Data::Column<double>* m_scoresColumn{ nullptr };
        Data::Jitter m_jitter{ Wisteria::AxisType::LeftYAxis };

        bool m_showcaseScore{ false };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // LIX_GAUGE_H
