/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef LIX_GAUGE_GERMAN_H
#define LIX_GAUGE_GERMAN_H

#include "../data/jitter.h"
#include "../math/mathematics.h"
#include "../util/donttranslate.h"
#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A plot showing Lix (Läsbarhetsindex) readability scores and what they represent.
        @details This is an adaptation of the original gauge, designed for German materials.
        @image html LixGaugeGerman.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the Lix score(s)
         for document (or samples). The ID column's labels will be associated with each point,
         so it is recommended to fill this column with the documents' (or samples') names.

         A categorical column can also optionally be used as a grouping variable.

         | ID            | Score | Group       |
         | :--           | --:   | --:         |
         | ImportingData | 52    | Beispielen  |
         | ExportingData | 50    | Beispielen  |
         | Welcome       | 62    | Überblicken |

        @par Missing Data:
         - Scores that are missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

        @par Citations:
            Björnsson, C.H. “Readability of Newspapers in 11 Languages.” *Reading Research
            Quarterly*, vol. 18, no. 4, 1983, pp. 480-97.

            Schulz, Renate A. “Literature and Readability: Bridging the Gap in Foreign Language
            Reading.” *The Modern Language Journal*, vol. 65, no. 1, Spring 1981, pp. 43-53.*/
    class LixGaugeGerman final : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(LixGaugeGerman);
        LixGaugeGerman() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
                Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as null to use the standard shapes.*/
        explicit LixGaugeGerman(
            Wisteria::Canvas* canvas,
            std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes = nullptr);

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
        void SetData(std::shared_ptr<const Wisteria::Data::Dataset> data,
                     const wxString& scoreColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @returns @c true if English labels are being used for the brackets.\n
        ///     Otherwise, the German labels from the original article are shown.
        [[nodiscard]]
        bool IsUsingEnglishLabels() const noexcept
            {
            return m_useEnglishLabels;
            }

        /** @brief Sets whether to use English labels for the brackets.
            @param useEnglish @c true to use the translated (English) labels.
                @c false to use the German labels from the article.*/
        void UseEnglishLabels(const bool useEnglish) noexcept { m_useEnglishLabels = useEnglish; }

      private:
        void RecalcSizes(wxDC& dc) override final;
        void AdjustAxes();

        const Wisteria::Data::Column<double>* m_scoresColumn{ nullptr };
        Wisteria::Data::Jitter m_jitter{ Wisteria::AxisType::LeftYAxis };
        bool m_useEnglishLabels{ false }; // use translated labels from Schulz article
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // LIX_GAUGE_GERMAN_H
