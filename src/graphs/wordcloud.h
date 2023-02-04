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

        /// @returns How the words are organized visually.
        [[nodiscard]]
        WordCloudLayout GetLayout() const noexcept
            { return m_layout; }
    private:
        [[deprecated("Word clouds do not support legends.")]]
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> CreateLegend(
            [[maybe_unused]] const LegendOptions& options) override
            { return nullptr; }

        wxRect GetMaxDrawingRect() const
            {
            return (GetLayout() == WordCloudLayout::Spiral) ?
                wxRect{ wxSize(GetPlotAreaBoundingBox().GetWidth() * math_constants::half,
                               GetPlotAreaBoundingBox().GetHeight() * math_constants::half) } :
                GetPlotAreaBoundingBox();
            }

        // randomly positions labels with a rect
        void TryPlaceLabelsInRect(std::vector<std::shared_ptr<GraphItems::Label>>& labels,
                                  wxDC& dc, const wxRect& drawArea);

        void AdjustRectToDrawArea(wxRect& rect, const wxRect& drawArea) const
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
        /// @brief Fill in a center rect, then fill rects around that,
        ///     going clockwise (starting at 3 o'clock).
        /// @note layout algorithm from https://www2.cs.arizona.edu/~kobourov/wordle2.pdf.
        void RandomLayoutSpiral(std::vector<std::shared_ptr<GraphItems::Label>>& labels, wxDC& dc);
        struct WordInfo
            {
            std::wstring m_word;
            double m_frequency;
            };
        void RecalcSizes(wxDC& dc) final;
        std::vector<WordInfo> m_words;
        WordCloudLayout m_layout{ WordCloudLayout::Spiral };
        // the number of times that we will try to place a label within a given ploygon
        size_t m_placementAttempts{ 10 };
        };
    }

/** @}*/

#endif //__WISTERIA_HEATMAP_H__
