/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_WORDCLOUD_H__
#define __WISTERIA_WORDCLOUD_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A visual display of word frequencies.

        @par %Data:
         This plot accepts a Data::Dataset, where a categorical column of words are
         aggregated. The aggregation can either be the frequency of observations or
         summed values from a corresponding continuous column.

        @par Missing Data:
         - Missing data in the word column will be ignored.
         - If summing a continuous column, then missing data will be ignored (listwise deletion).
    */
    class WordCloud final : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param colors The color scheme to apply to the words.*/
        explicit WordCloud(Wisteria::Canvas* canvas,
                           std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr);
        /** @brief Sets the data for the word cloud.
            @param data The data.
            @param wordColumnName The column containing the words.
            @param valueColumnName The column containing the words' frequency counts.\n
                If not provided, then the words will be tabulated by the word cloud.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
            const wxString& wordColumnName,
            const std::optional<const wxString> valueColumnName = std::nullopt);
    private:
        [[deprecated("Word Clouds do not support legends.")]]
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> CreateLegend(
            [[maybe_unused]] const LegendOptions& options) override
            { return nullptr; }

        void AdjustRectToDrawArea(wxRect& rect) const
            {
            if (rect.GetRight() > GetPlotAreaBoundingBox().GetRight())
                {
                rect.SetLeft(rect.GetLeft() - (rect.GetRight() - GetPlotAreaBoundingBox().GetRight()));
                }
            if (rect.GetBottom() > GetPlotAreaBoundingBox().GetBottom())
                {
                rect.SetTop(rect.GetTop() - (rect.GetBottom() - GetPlotAreaBoundingBox().GetBottom()));
                }
            }
        // layout algorithms from https://www2.cs.arizona.edu/~kobourov/wordle2.pdf.
        void RandomLayout(std::vector<std::shared_ptr<GraphItems::Label>>& labels, wxDC& dc);
        struct WordInfo
            {
            std::wstring m_word;
            double m_frequency;
            };
        void RecalcSizes(wxDC& dc) final;
        std::vector<WordInfo> m_words;
        };
    }

/** @}*/

#endif //__WISTERIA_HEATMAP_H__
