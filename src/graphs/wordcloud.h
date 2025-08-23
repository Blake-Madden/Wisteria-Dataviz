/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WORDCLOUD_H
#define WISTERIA_WORDCLOUD_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A visual display of word frequencies.
        @image html wordcloud.png width=90%

        @par %Data:
         This plot accepts a Data::Dataset, where a categorical column contains the words.
         An optional weight variable can also be used, which contains the frequency
         counts for the adjacent words in the word column.

         | Word   | Frequency |
         | :--    | --:       |
         | Rachel | 192       |
         | Ross   | 186       |
         | Monica | 181       |

        @par Missing Data:
         - Missing data in the word column will be ignored.
         - If summing a continuous column, then missing data will be ignored (listwise deletion).*/
    class WordCloud final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WordCloud);
        WordCloud() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param colors The color scheme to apply to the words.*/
        explicit WordCloud(Wisteria::Canvas* canvas,
                           const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);
        /** @brief Sets the data for the word cloud.
            @param data The data.
            @param wordColumnName The column containing the words.
            @param weightColumnName The column containing the words' frequency counts.\n
                If not provided, then the words will be tabulated by the word cloud.
            @param minFreq The minimum frequency that a word must appear to be included
                in the cloud.\n The default is `1`.
            @param maxFreq The maximum frequency that a word can appear and still be included
                in the cloud.\n This is useful for filtering high-frequency words.\n
                By default, all words above
           @c minFreq are included.
            @param maxWords The maximum number of words to show
                (going from the highest-to-lowest frequently occurring words).\n
                This is performed after the words not meeting the min and min frequency criteria
                has been removed (if applicable).
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& wordColumnName,
                     const std::optional<const wxString>& weightColumnName = std::nullopt,
                     size_t minFreq = 1, std::optional<size_t> maxFreq = std::nullopt,
                     std::optional<size_t> maxWords = std::nullopt);

      private:
        [[deprecated("Word clouds do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) override
            {
            return nullptr;
            }

        // randomly positions labels with a rect
        void TryPlaceLabelsInPolygon(std::vector<std::unique_ptr<GraphItems::Label>>& labels,
                                     wxDC& dc, const std::vector<wxPoint>& polygon);

        static void AdjustRectToDrawArea(wxRect& rect, const wxRect& drawArea)
            {
            if (rect.GetRight() > drawArea.GetRight())
                {
                rect.SetLeft(rect.GetLeft() - (rect.GetRight() - drawArea.GetRight()));
                }
            if (rect.GetBottom() > drawArea.GetBottom())
                {
                rect.SetTop(rect.GetTop() - (rect.GetBottom() - drawArea.GetBottom()));
                }
            }

        struct WordInfo
            {
            std::wstring m_word;
            double m_frequency;
            };

        void RecalcSizes(wxDC& dc) final;
        std::vector<WordInfo> m_words;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WORDCLOUD_H
