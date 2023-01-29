///////////////////////////////////////////////////////////////////////////////
// Name:        heatmap.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wordcloud.h"
#include "../util/frequencymap.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Colors::Schemes;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WordCloud::WordCloud(Wisteria::Canvas* canvas,
                         std::shared_ptr<ColorScheme> colors /*= nullptr*/) :
        Graph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
            Settings::GetDefaultColorScheme());

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void WordCloud::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& wordColumnName,
        const std::optional<const wxString> valueColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        GetSelectedIds().clear();
        m_words.clear();

        if (GetDataset() == nullptr)
            { return; }

        const auto wordColumn = GetDataset()->GetCategoricalColumn(wordColumnName);
        if (wordColumn == GetDataset()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': word column not found for word cloud."),
                wordColumnName).ToUTF8());
            }
        
        const auto freqColumn = (valueColumnName ?
            GetDataset()->GetContinuousColumn(valueColumnName.value()) :
            GetDataset()->GetContinuousColumns().cend());
        if (valueColumnName && freqColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for word cloud."),
                valueColumnName.value()).ToUTF8());
            }

        const auto useValueColumn = (freqColumn != GetDataset()->GetContinuousColumns().cend());
        aggregate_frequency_set<Data::GroupIdType> groups;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useValueColumn &&
                std::isnan(freqColumn->GetValue(i)))
                { continue; }
            groups.insert(wordColumn->GetValue(i),
                useValueColumn ? freqColumn->GetValue(i) : 1);
            }
        for (const auto& [labelID, freqAndCount] : groups.get_data())
            {
            m_words.push_back(
                WordInfo{ wordColumn->GetLabelFromID(labelID).c_str(),
                          freqAndCount.second });
            }
        std::sort(m_words.begin(), m_words.end(),
            // least frequent to most frequent
            [](const auto& lhv, const auto& rhv) noexcept
            { return lhv.m_frequency < rhv.m_frequency; });
        // convert raw frequencies to percentages
        const auto grandTotal = std::accumulate(m_words.cbegin(), m_words.cend(), 0.0,
            [](const auto& val, const auto word) noexcept
            { return word.m_frequency + val; });
        std::for_each(m_words.begin(), m_words.end(),
            [&grandTotal](auto& val) noexcept
            { val.m_frequency = safe_divide(val.m_frequency, grandTotal); });
        }

    //----------------------------------------------------------------
    void WordCloud::RecalcSizes(wxDC& dc)
        {
        // if no data then bail
        if (m_words.size() == 0)
            { return; }

        Graph2D::RecalcSizes(dc);

        const auto drawAreaWidth = GetPlotAreaBoundingBox().GetWidth() * math_constants::three_fourths;
        const auto drawAreaHeight = GetPlotAreaBoundingBox().GetHeight();

        // create the word labels and stack them on top of each other
        std::vector<std::shared_ptr<GraphItems::Label>> labels;
        wxPoint origin = GetPlotAreaBoundingBox().GetTopLeft();
        size_t wordCount{ 0 };
        for (const auto& word : m_words)
            {
            wxRect suggestedRect(
                wxPoint(0, origin.y),
                wxSize(drawAreaWidth, drawAreaHeight * word.m_frequency));
            auto currentLabel =
                std::make_shared<GraphItems::Label>(GraphItemInfo(word.m_word).
                    Pen(wxNullPen).DPIScaling(GetDPIScaleFactor()).
                    Anchoring(Anchoring::TopLeftCorner).AnchorPoint(origin));
            currentLabel->SetBoundingBoxToContentAdjustment(LabelBoundingBoxContentAdjustment::ContentAdjustAll);
            currentLabel->SetBoundingBox(suggestedRect, dc, GetScaling());
            const auto bBox = currentLabel->GetBoundingBox(dc);
            origin.y += bBox.GetHeight();
            labels.push_back(currentLabel);
            ++wordCount;
            }

        RandomLayout(labels, dc);
        }

    //----------------------------------------------------------------
    void WordCloud::RandomLayout(std::vector<std::shared_ptr<GraphItems::Label>>& labels, wxDC& dc)
        {
        // sort remaining labels by width
        std::sort(labels.begin(), labels.end(),
            [&dc](const auto& lhv, const auto& rhv) noexcept
            { return lhv->GetBoundingBox(dc).GetHeight() < rhv->GetBoundingBox(dc).GetHeight(); });

        std::vector<wxRect> drawnRects;

        auto drawArea = GetPlotAreaBoundingBox();

        std::uniform_int_distribution<> xPosDistro(drawArea.GetLeft(), drawArea.GetWidth());
        std::uniform_int_distribution<> yPosDistro(drawArea.GetLeft(), drawArea.GetHeight());

        // random positioning lambda
        const auto tryPlaceLabel = [&,this]()
            {
            auto& label{ labels.back() };
            auto bBox = label->GetBoundingBox(dc);

            // make it fit with the drawing area
            bBox.SetTopLeft(wxPoint{ xPosDistro(m_mt), yPosDistro(m_mt) });
            AdjustRectToDrawArea(bBox);

            auto foundPos = std::find_if(drawnRects.cbegin(), drawnRects.cend(),
                [&bBox](const auto& rect) noexcept
                { return rect.Intersects(bBox); });
            // overlapping another label...
            if (foundPos != drawnRects.cend())
                {
                // ...push over to the right of it
                bBox.SetTopLeft(foundPos->GetTopRight());
                if (bBox.GetRight() > GetPlotAreaBoundingBox().GetRight())
                    {
                    // ...try it under the other label if it went outside of the draw area
                    bBox.SetTopLeft(foundPos->GetBottomLeft());
                    if (bBox.GetBottom() > GetPlotAreaBoundingBox().GetBottom())
                        {
                        // ...try it to the left of the other label
                        bBox.SetTopLeft(foundPos->GetTopLeft());
                        bBox.SetLeft(bBox.GetLeft() - bBox.GetWidth());
                        // ...try it above the other label
                        if (bBox.GetLeft() < GetPlotAreaBoundingBox().GetLeft())
                            {
                            bBox.SetTopLeft(foundPos->GetTopLeft());
                            bBox.SetTop(bBox.GetTop() - bBox.GetHeight());
                            // ...too high (outside of draw area), so give up
                            if (bBox.GetTop() < GetPlotAreaBoundingBox().GetTop())
                                { return; }
                            }
                        }
                    }
                // it fit next to the other label, so recheck for overlapping
                // and give up if another overlap happens
                foundPos = std::find_if(drawnRects.cbegin(), drawnRects.cend(),
                    [&bBox](const auto& rect) noexcept
                    { return rect.Intersects(bBox); });
                if (foundPos != drawnRects.cend())
                    { return; }
                }

            // place it, add it to be rendered, and remove from the list
            drawnRects.push_back(bBox);
            label->SetAnchorPoint(bBox.GetTopLeft());
            AddObject(label);
            labels.pop_back();
            };

        // the number of times that we will loop through the labels and try to place
        // them within the current drawing area
        constexpr auto maxPlacementAttempts{ 5 };

        // try to place everything in a band half the height of the plot area
        // so that the labels are more dense
        int64_t placementAttempts{ static_cast<int64_t>(labels.size()) * maxPlacementAttempts };
        while (labels.size() && placementAttempts > 0)
            {
            tryPlaceLabel();
            --placementAttempts;
            }

        // If there are any remaining labels at this point, then they won't be drawn.
        // This is preferrable to looping an enormous (or infinite) number of times
        // if the real estate is really tight. This is acceptable, as these would
        // be the less frequently occurring labels.
        }
    }
