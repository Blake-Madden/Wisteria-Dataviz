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
            // least frequent to most frequent because we go backwards when
            // moving these into drawable labels
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

        // create the word labels and stack them on top of each other
        std::vector<std::shared_ptr<GraphItems::Label>> labels;
        wxPoint origin = GetPlotAreaBoundingBox().GetTopLeft();
        size_t wordCount{ 0 };
        int maxWidth{ 0 }, maxHeight{ 0 };
        for (const auto& word : m_words)
            {
            wxRect suggestedRect(
                wxPoint(0, origin.y),
                wxSize(GetPlotAreaBoundingBox().GetWidth(), GetPlotAreaBoundingBox().GetHeight() * word.m_frequency));
            auto currentLabel =
                std::make_shared<GraphItems::Label>(GraphItemInfo(word.m_word).
                    Pen(wxNullPen).DPIScaling(GetDPIScaleFactor()).
                    Anchoring(Anchoring::TopLeftCorner).AnchorPoint(origin).
                    FontColor(GetColorScheme()->GetRecycledColor(wordCount)));
            currentLabel->SetBoundingBoxToContentAdjustment(LabelBoundingBoxContentAdjustment::ContentAdjustAll);
            currentLabel->SetBoundingBox(suggestedRect, dc, GetScaling());
            const auto bBox = currentLabel->GetBoundingBox(dc);
            origin.y += bBox.GetHeight();
            labels.push_back(currentLabel);
            ++wordCount;

            maxWidth = std::max({ maxWidth, bBox.GetWidth() });
            maxHeight = std::max({ maxHeight, bBox.GetHeight() });
            }

        const wxRect maxDrawRect = GetMaxDrawingRect();

        const double getWidthRescale = (maxWidth > maxDrawRect.GetWidth()) ?
            safe_divide<double>(maxDrawRect.GetWidth(), maxWidth) : 1.0;
        const double getHeightRescale = (maxHeight > maxDrawRect.GetHeight()) ?
            safe_divide<double>(maxDrawRect.GetHeight(), maxHeight) : 1.0;
        const double rescaleValue = std::min({ getWidthRescale, getHeightRescale , 1.0 });

        for (auto& label : labels)
            {
            label->SetScaling(label->GetScaling() * rescaleValue);
            label->SetMinimumUserSizeDIPs(std::nullopt, std::nullopt);
            }

        // the less words, the more aggressively we should try to fit them together
        m_placementAttempts = (labels.size() < 100) ? 25 :
            (labels.size() < 1'000) ? 10 :
            5;

        if (GetLayout() == WordCloudLayout::Spiral)
            { RandomLayoutSpiral(labels, dc); }
        }

    //----------------------------------------------------------------
    void WordCloud::RandomLayoutSpiral(std::vector<std::shared_ptr<GraphItems::Label>>& labels, wxDC& dc)
        {
        // sort remaining labels by width, largest-to-smallest
        std::sort(labels.begin(), labels.end(),
            [&dc](const auto& lhv, const auto& rhv) noexcept
            { return lhv->GetBoundingBox(dc).GetWidth() > rhv->GetBoundingBox(dc).GetWidth(); });

        // try to place everything in a band 1/2 the size of the plot area
        // so that the labels are more dense
        auto drawArea = GetPlotAreaBoundingBox();
        drawArea.Deflate(drawArea.GetWidth() * math_constants::fourth,
                         drawArea.GetHeight() * math_constants::fourth);
        const auto centerRect{ drawArea };
        TryPlaceLabelsInRect(labels, dc, drawArea);

        // right side
        if (labels.size())
            {
            drawArea = centerRect;
            drawArea.SetWidth(drawArea.GetWidth() / 2);
            drawArea.SetLeft(centerRect.GetRight());
            TryPlaceLabelsInRect(labels, dc, drawArea);
            }

        // bottom side
        if (labels.size())
            {
            drawArea = centerRect;
            drawArea.SetHeight(drawArea.GetHeight() / 2);
            drawArea.SetTop(centerRect.GetBottom());
            TryPlaceLabelsInRect(labels, dc, drawArea);
            }

        // left side
        if (labels.size())
            {
            drawArea = centerRect;
            drawArea.SetWidth(drawArea.GetWidth() / 2);
            drawArea.SetLeft(GetPlotAreaBoundingBox().GetLeft());
            TryPlaceLabelsInRect(labels, dc, drawArea);
            }

        // top side
        if (labels.size())
            {
            drawArea = centerRect;
            drawArea.SetHeight(drawArea.GetHeight() / 2);
            drawArea.SetTop(GetPlotAreaBoundingBox().GetTop());
            TryPlaceLabelsInRect(labels, dc, drawArea);
            }

        // If there are any remaining labels at this point, then they won't be drawn.
        // This is preferrable to looping an enormous (or infinite) number of times
        // if the real estate is really tight. This is acceptable, as these would
        // be the less frequently occurring labels.
        }

    //----------------------------------------------------------------
    void WordCloud::TryPlaceLabelsInRect(std::vector<std::shared_ptr<GraphItems::Label>>& labels,
                                         wxDC& dc, const wxRect& drawArea)
        {
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            wxPoint pt[4];
            GraphItems::Polygon::GetRectPoints(drawArea, pt);
            AddObject(std::make_shared<GraphItems::Polygon>(
                GraphItemInfo().Pen(*wxBLUE), pt, std::size(pt)));
            }

        // random positioning lambda for the last label in the list
        const auto tryPlaceLabel = [&dc, this]
                                    (auto& label, const auto& drawArea, std::vector<wxRect>& drawnRects,
                                     auto& xPosDistro, auto& yPosDistro)
            {
            auto bBox = label->GetBoundingBox(dc); // will already be cached

            if (bBox.GetWidth() > drawArea.GetWidth() ||
                bBox.GetHeight() > drawArea.GetWidth())
                { return false; }

            // make it fit with the drawing area
            bBox.SetTopLeft(wxPoint{ xPosDistro(m_mt), yPosDistro(m_mt) });
            AdjustRectToDrawArea(bBox, drawArea);

            auto foundPos = std::find_if(std::execution::par,
                drawnRects.cbegin(), drawnRects.cend(),
                [&bBox](const auto& rect) noexcept
                { return rect.Intersects(bBox); });
            // overlapping another label...
            if (foundPos != drawnRects.cend())
                {
                // ...push over to the right of it
                bBox.SetTopLeft(foundPos->GetTopRight());
                if (bBox.GetRight() > drawArea.GetRight())
                    {
                    // ...try it under the other label if it went outside of the draw area
                    bBox.SetTopLeft(foundPos->GetBottomLeft());
                    if (bBox.GetBottom() > drawArea.GetBottom())
                        {
                        // ...try it to the left of the other label
                        bBox.SetTopLeft(foundPos->GetTopLeft());
                        bBox.SetLeft(bBox.GetLeft() - bBox.GetWidth());
                        // ...try it above the other label
                        if (bBox.GetLeft() < drawArea.GetLeft())
                            {
                            bBox.SetTopLeft(foundPos->GetTopLeft());
                            bBox.SetTop(bBox.GetTop() - bBox.GetHeight());
                            // ...too high (outside of draw area), so give up
                            if (bBox.GetTop() < drawArea.GetTop())
                                { return false; }
                            }
                        }
                    }
                // it fit next to the other label, so recheck for overlapping
                // and give up if another overlap happens
                foundPos = std::find_if(std::execution::par,
                    drawnRects.cbegin(), drawnRects.cend(),
                    [&bBox](const auto& rect) noexcept
                    { return rect.Intersects(bBox); });
                if (foundPos != drawnRects.cend())
                    { return false; }
                }

            // place it and add it to be rendered
            drawnRects.push_back(bBox);
            label->SetAnchorPoint(bBox.GetTopLeft());
            AddObject(label);

            return true;
            };

        std::vector<wxRect> drawnRects;

        std::uniform_int_distribution<> xPosDistro(drawArea.GetLeft(), drawArea.GetLeft() + drawArea.GetWidth());
        std::uniform_int_distribution<> yPosDistro(drawArea.GetTop(), drawArea.GetTop() + drawArea.GetHeight());

        for (auto labelPos = labels.begin(); labelPos < labels.end(); /*in loop*/)
            {
            bool sucessfullyPlaced{ false };
            for (size_t i = 0; i < m_placementAttempts; ++i)
                {
                if (tryPlaceLabel(*labelPos, drawArea, drawnRects, xPosDistro, yPosDistro))
                    {
                    labelPos = labels.erase(labelPos);
                    sucessfullyPlaced = true;
                    break;
                    }
                }
            // If the first (and implicitly widest) label didn't get placed in the rect,
            // then force it to be drawn in the center. This will help with ensuring that
            // more frequently occurring words are shown.
            if (!sucessfullyPlaced && drawnRects.empty())
                {
                auto bBox = (*labelPos)->GetBoundingBox(dc);
                // if it can fit, then center it
                if (bBox.GetWidth() <= drawArea.GetWidth() &&
                    bBox.GetHeight() <= drawArea.GetHeight())
                    {
                    wxPoint centerPoint{ drawArea.GetLeft() + (drawArea.GetWidth() / 2),
                                         drawArea.GetTop() + (drawArea.GetHeight() / 2) };
                    drawnRects.push_back(bBox);
                    bBox.SetTopLeft(
                        { centerPoint.x - (bBox.GetWidth() / 2),
                          centerPoint.y - (bBox.GetHeight() / 2) });
                    (*labelPos)->SetAnchorPoint(
                        { centerPoint.x - (bBox.GetWidth() / 2),
                          centerPoint.y - (bBox.GetHeight() / 2) });
                    AddObject((*labelPos));
                    labelPos = labels.erase(labelPos);
                    }
                else
                    { ++labelPos; }
                }
            // wasn't erased, so skip over it
            else if (!sucessfullyPlaced)
                { ++labelPos; }
            }
        }
    }
