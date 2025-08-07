/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef FLESCH_CHART_H
#define FLESCH_CHART_H

#include "../data/jitter.h"
#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Chart that demonstrates the meaning of a Flesch Reading Ease Score.
        @image html FleschChart.png

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the
         FRE score, another holds the average words-per-sentence value, and another the
         average syllables-per-word value for a given document or sample.
         \n
         The ID column's labels will be associated with each point,
         so it is recommended to fill this column with the documents' (or samples') names.

         A categorical column can also optionally be used as a grouping variable.

         | ID            | Score | SylPerW | WordsPerSent | Group       |
         | :--           | --:   | --:     | --:          | --:         |
         | ImportingData | 79    | 1.31    | 16           | Examples    |

        @par Missing Data:
         - Words-per-sentence, scores, or syllables-per-word values that are
           missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

        @par Citations:
            Flesch, Rudolf Franz. *The Art of Readable Writing*. Harper & Row, 1949.*/
    class FleschChart final : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(FleschChart);
        FleschChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
               Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
               Leave as @c nullptr to use the standard shapes.*/
        explicit FleschChart(
            Wisteria::Canvas* canvas,
            const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr);

        /** @brief Sets the data.
            @param data The data to use.
            @param wordsPerSentenceColumnName The column containing the
                number of words per sentence (shown on left "ruler" on chart).
            @param scoreColumnName The column containing the score to add
                (shown on middle "ruler" on chart).
            @param syllablesPerWordColumnName The column containing the
                number of syllables per word (shown on right "ruler" on chart).
            @param groupColumnName The (optional) categorical column to use for grouping.
            @param includeSyllableRulerDocumentGroups Sets whether to include brackets along
                the syllables-per-word ruler, showing the document names under each bracket.\n
                This will only be applied if there are 1-50 documents on the
                graph, and the document names must be in the dataset's ID column.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Wisteria::Data::Dataset>& data,
                     const wxString& wordsPerSentenceColumnName, const wxString& scoreColumnName,
                     const wxString& syllablesPerWordColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt,
                     bool includeSyllableRulerDocumentGroups = false);

        /** @brief Sets whether to draw a line connecting the points between the rulers.
            @param show Whether to draw the line.
            @note This is useful to turn off if numerous documents are being plotted
                and you are only needing to see the scores' clustering.*/
        void ShowConnectionLine(const bool show) noexcept { m_showConnectionLine = show; }

        /// @returns @c true if points on the rulers are being connected by a line.
        [[nodiscard]]
        bool IsShowingConnectionLine() const noexcept
            {
            return m_showConnectionLine;
            }

      private:
        void RecalcSizes(wxDC& dc) override final;

        const Wisteria::Data::Column<double>* m_wordsPerSentenceColumn{ nullptr };
        const Wisteria::Data::Column<double>* m_scoresColumn{ nullptr };
        const Wisteria::Data::Column<double>* m_syllablesPerWordColumn{ nullptr };

        Wisteria::Data::Jitter m_jitterWords{ Wisteria::AxisType::LeftYAxis };
        Wisteria::Data::Jitter m_jitterScores{ Wisteria::AxisType::LeftYAxis };
        Wisteria::Data::Jitter m_jitterSyllables{ Wisteria::AxisType::LeftYAxis };
        bool m_showConnectionLine{ true };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // FLESCH_CHART_H
