/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __CRAWFORD_GRAPH_H__ 
#define __CRAWFORD_GRAPH_H__

#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A plot showing Crawford readability scores.
        @details This is a Spanish readability test.
        @image html CrawfordGraph.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the Crawford
         score(s) and another holds the syllables per 100 words for respective
         document (or samples). The ID column's labels will be associated with each point,
         so it is recommended to fill this column with the documents' (or samples') names.

         A categorical column can also optionally be used as a grouping variable.

         | ID            | Score | SylPer100W | Group       |
         | :--           | --:   | --:        | --:         |
         | ImportingData | 3.2   | 201        | Examples    |
         | ExportingData | 4     | 220        | Examples    |
         | Welcome       | 2.1   | 170        | Overviews   |

        @par Missing Data:
         - Scores or syllable counts that are missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

        @par Citations:
            Crawford, Alan. “A Spanish Language Fry-Type Readability Procedure: Elementary Level.”
            *Bilingual Education Paper Series*, vol. 7, no. 8, 1984, pp. 1-17.*/
    class CrawfordGraph final : public GroupGraph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
               Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points.
               Leave as null to use the standard shapes.*/
        explicit CrawfordGraph(Wisteria::Canvas* canvas,
            std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes = nullptr);

        /** @brief Sets the data.
            @param data The data to use.
            @param scoreColumnName The column containing the documents' scores
                (a continuous column).
            @param syllablesPer100WordsColumnName The column containing the documents'
                syllables per 100 words (a continuous column).
            @param groupColumnName The (optional) categorical column to use for grouping.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Wisteria::Data::Dataset> data,
                     const wxString& scoreColumnName,
                     const wxString& syllablesPer100WordsColumnName,
                     std::optional<const wxString> groupColumnName = std::nullopt);
    private:
        void RecalcSizes(wxDC& dc) final;

        const Wisteria::Data::Column<double>* m_scoresColumn{ nullptr };
        const Wisteria::Data::Column<double>* m_syllablesPer100WordsColumn{ nullptr };
        };
    }

/** @}*/

#endif //__CRAWFORD_GRAPH_H__
