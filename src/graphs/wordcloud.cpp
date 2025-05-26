///////////////////////////////////////////////////////////////////////////////
// Name:        wordcloud.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wordcloud.h"
#include "../util/frequencymap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WordCloud, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WordCloud::WordCloud(Wisteria::Canvas * canvas,
                         const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void WordCloud::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& wordColumnName,
        const std::optional<const wxString>& weightColumnName /*= std::nullopt*/,
        const size_t minFreq /*= 1*/, const std::optional<size_t> maxFreq /*= std::nullopt*/,
        const std::optional<size_t> maxWords /*= std::nullopt*/)
        {
        SetDataset(data);
        GetSelectedIds().clear();
        m_words.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto wordColumn = GetDataset()->GetCategoricalColumn(wordColumnName);
        if (wordColumn == GetDataset()->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': word column not found for word cloud."), wordColumnName)
                    .ToUTF8());
            }

        const auto freqColumn =
            (weightColumnName ? GetDataset()->GetContinuousColumn(weightColumnName.value()) :
                                GetDataset()->GetContinuousColumns().cend());
        if (weightColumnName && freqColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': continuous weight column not found for word cloud."),
                                 weightColumnName.value())
                    .ToUTF8());
            }

        const auto useValueColumn = (freqColumn != GetDataset()->GetContinuousColumns().cend());
        aggregate_frequency_set<Data::GroupIdType> groups;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // entire observation is ignored if value being aggregated is NaN
            if (useValueColumn && std::isnan(freqColumn->GetValue(i)))
                {
                continue;
                }
            groups.insert(wordColumn->GetValue(i), useValueColumn ? freqColumn->GetValue(i) : 1);
            }
        for (const auto& [labelID, freqAndCount] : groups.get_data())
            {
            m_words.push_back(WordInfo{ wordColumn->GetLabelFromID(labelID).ToStdWstring(),
                                        freqAndCount.second });
            }
        std::sort(m_words.begin(), m_words.end(),
                  // most frequent to least frequent
                  [](const auto& lhv, const auto& rhv) noexcept
                  { return lhv.m_frequency > rhv.m_frequency; });

        // remove words that don't meet the minimum frequency
        auto wordsToRemoveStart =
            std::find_if(m_words.cbegin(), m_words.cend(), [minFreq](const auto& item) noexcept
                         { return item.m_frequency < minFreq; });
        if (wordsToRemoveStart != m_words.cend())
            {
            m_words.erase(wordsToRemoveStart, m_words.end());
            }

        std::sort(m_words.begin(), m_words.end(),
                  // least frequent to most frequent because we go backwards when
                  // moving these into drawable labels
                  [](const auto& lhv, const auto& rhv) noexcept
                  { return lhv.m_frequency < rhv.m_frequency; });

        if (maxFreq)
            {
            // remove words that exceed the maximum frequency
            auto maxWordsToRemoveStart =
                std::find_if(m_words.cbegin(), m_words.cend(), [maxFreq](const auto& item)
                             { return item.m_frequency > maxFreq.value(); });
            if (maxWordsToRemoveStart != m_words.cend())
                {
                m_words.erase(maxWordsToRemoveStart, m_words.end());
                }
            }

        if (maxWords && maxWords.value() < m_words.size())
            {
            m_words.erase(m_words.begin(), m_words.end() - maxWords.value());
            }

        // convert raw frequencies to percentages
        const auto grandTotal = std::accumulate(m_words.cbegin(), m_words.cend(), 0.0,
                                                [](const auto& val, const auto word) noexcept
                                                { return word.m_frequency + val; });
        std::for_each(m_words.begin(), m_words.end(), [&grandTotal](auto& val) noexcept
                      { val.m_frequency = safe_divide(val.m_frequency, grandTotal); });
        }

    //----------------------------------------------------------------
    void WordCloud::RecalcSizes(wxDC & dc)
        {
        // if no data then bail
        if (m_words.empty())
            {
            return;
            }

        Graph2D::RecalcSizes(dc);

        // create the word labels and stack them on top of each other
        std::vector<std::unique_ptr<GraphItems::Label>> labels;
        wxPoint origin = GetPlotAreaBoundingBox().GetTopLeft();
        size_t wordCount{ 0 };
        double labelsArea{ 0.0 };
        int maxWidth{ 0 }, maxHeight{ 0 };
        for (const auto& word : m_words)
            {
            const wxRect suggestedRect(
                wxPoint(0, origin.y),
                wxSize(GetPlotAreaBoundingBox().GetWidth(),
                       GetPlotAreaBoundingBox().GetHeight() * word.m_frequency));
            auto currentLabel = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo(word.m_word)
                    .Pen(wxNullPen)
                    .DPIScaling(GetDPIScaleFactor())
                    .Anchoring(Anchoring::TopLeftCorner)
                    .AnchorPoint(origin)
                    .FontColor(GetColorScheme()->GetRecycledColor(wordCount)));
            currentLabel->SetBoundingBoxToContentAdjustment(
                LabelBoundingBoxContentAdjustment::ContentAdjustAll);
            currentLabel->SetBoundingBox(suggestedRect, dc, GetScaling());
            const auto bBox = currentLabel->GetBoundingBox(dc);
            labelsArea += bBox.GetWidth() * bBox.GetHeight();
            maxWidth = std::max({ maxWidth, bBox.GetWidth() });
            maxHeight = std::max({ maxHeight, bBox.GetHeight() });
            origin.y += bBox.GetHeight();
            labels.push_back(std::move(currentLabel));
            ++wordCount;
            }

        // a cloud polygon
        const std::vector<wxPoint> polygon{
            // top
            wxPoint{ GetPlotAreaBoundingBox().GetLeftTop() +
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * math_constants::quarter, 0) },
            wxPoint{ GetPlotAreaBoundingBox().GetRightTop() -
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * math_constants::quarter, 0) },
            // right
            wxPoint{ GetPlotAreaBoundingBox().GetRightTop() -
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * (math_constants::eighth / 2.0),
                             -(GetPlotAreaBoundingBox().GetHeight() * math_constants::eighth)) },
            wxPoint{ GetPlotAreaBoundingBox().GetRightTop() +
                     wxPoint(0, GetPlotAreaBoundingBox().GetHeight() * math_constants::half) },
            wxPoint{ GetPlotAreaBoundingBox().GetRightBottom() -
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * (math_constants::eighth / 2.0),
                             (GetPlotAreaBoundingBox().GetHeight() * math_constants::eighth)) },
            // bottom
            wxPoint{ GetPlotAreaBoundingBox().GetRightBottom() -
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * math_constants::quarter, 0) },
            wxPoint{ GetPlotAreaBoundingBox().GetLeftBottom() +
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * math_constants::quarter, 0) },
            wxPoint{ GetPlotAreaBoundingBox().GetLeftBottom() +
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * (math_constants::eighth / 2.0),
                             -(GetPlotAreaBoundingBox().GetHeight() * math_constants::eighth)) },
            // left
            wxPoint{ GetPlotAreaBoundingBox().GetLeftTop() +
                     wxPoint(0, GetPlotAreaBoundingBox().GetHeight() * math_constants::half) },
            wxPoint{ GetPlotAreaBoundingBox().GetLeftTop() +
                     wxPoint(GetPlotAreaBoundingBox().GetWidth() * (math_constants::eighth / 2.0),
                             (GetPlotAreaBoundingBox().GetHeight() * math_constants::eighth)) },
        };

        const auto polyArea = GraphItems::Polygon::GetPolygonArea(polygon) * math_constants::half;
        const auto polygonBoundingBox = GraphItems::Polygon::GetPolygonBoundingBox(polygon);

        const auto getWidthRescale =
            safe_divide<double>(GraphItems::Polygon::GetPolygonWidth(polygon), maxWidth);
        const auto getHeightRescale =
            safe_divide<double>(polygonBoundingBox.GetHeight(), maxHeight);
        const double rescaleValue =
            std::min({ getWidthRescale, getHeightRescale,
                       std::sqrt(safe_divide<double>(polyArea, labelsArea)) });

        for (auto& label : labels)
            {
            label->SetScaling(label->GetScaling() * rescaleValue);
            label->SetMinimumUserSizeDIPs(std::nullopt, std::nullopt);
            }

        // sort remaining labels by width, largest-to-smallest
        std::sort(
            labels.begin(), labels.end(), [&dc](const auto& lhv, const auto& rhv) noexcept
            { return lhv->GetBoundingBox(dc).GetWidth() > rhv->GetBoundingBox(dc).GetWidth(); });

        TryPlaceLabelsInPolygon(labels, dc, polygon);
        }

    //----------------------------------------------------------------
    void WordCloud::TryPlaceLabelsInPolygon(std::vector<std::unique_ptr<GraphItems::Label>> &
                                                labels,
                                            wxDC & dc, const std::vector<wxPoint>& polygon)
        {
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            AddObject(std::make_unique<GraphItems::Polygon>(
                GraphItems::GraphItemInfo().Pen(*wxBLUE), polygon));
            }

        std::vector<wxRect> drawnRects;

        // Random positioning lambda for the last label in the list.
        // Returns true if the label was std::move'd into the graph.
        const auto tryPlaceLabel = [&dc, &polygon, &drawnRects, this](
                                       auto& label, const auto& polyBoundingBox, const wxPoint pt)
        {
            auto bBox = label->GetBoundingBox(dc); // will already be cached

            if (bBox.GetWidth() > polyBoundingBox.GetWidth() ||
                bBox.GetHeight() > polyBoundingBox.GetWidth())
                {
                return false;
                }

            // make it fit with the drawing area
            bBox.SetTopLeft(pt);
            AdjustRectToDrawArea(bBox, polyBoundingBox);

            auto foundPos =
                std::find_if(drawnRects.cbegin(), drawnRects.cend(),
                             [&bBox](const auto& rect) noexcept { return rect.Intersects(bBox); });
            // overlapping another label...
            if (foundPos != drawnRects.cend())
                {
                // ...push over to the right of it
                bBox.SetTopLeft(foundPos->GetTopRight());
                if (bBox.GetRight() > polyBoundingBox.GetRight())
                    {
                    // ...try it under the other label if it went outside of the draw area
                    bBox.SetTopLeft(foundPos->GetBottomLeft());
                    if (bBox.GetBottom() > polyBoundingBox.GetBottom())
                        {
                        // ...try it to the left of the other label
                        bBox.SetTopLeft(foundPos->GetTopLeft());
                        bBox.SetLeft(bBox.GetLeft() - bBox.GetWidth());
                        // ...try it above the other label
                        if (bBox.GetLeft() < polyBoundingBox.GetLeft())
                            {
                            bBox.SetTopLeft(foundPos->GetTopLeft());
                            bBox.SetTop(bBox.GetTop() - bBox.GetHeight());
                            // ...too high (outside of draw area), so give up
                            if (bBox.GetTop() < polyBoundingBox.GetTop())
                                {
                                return false;
                                }
                            }
                        }
                    }
                // it fit next to the other label, so recheck for overlapping
                // and give up if another overlap happens
                foundPos = std::find_if(drawnRects.cbegin(), drawnRects.cend(),
                                        [&bBox](const auto& rect) noexcept
                                        { return rect.Intersects(bBox); });
                if (foundPos != drawnRects.cend())
                    {
                    return false;
                    }
                }

            if (GraphItems::Polygon::IsRectInsidePolygon(bBox, polygon))
                {
                // place it and add it to be rendered
                drawnRects.push_back(bBox);
                label->SetAnchorPoint(bBox.GetTopLeft());
                AddObject(std::move(label));

                return true;
                }
            return false;
        };

        auto polyBoundingBox{ GraphItems::Polygon::GetPolygonBoundingBox(polygon) };

        std::uniform_int_distribution<> xPosDistro(
            polyBoundingBox.GetLeft(), polyBoundingBox.GetLeft() + polyBoundingBox.GetWidth());
        std::uniform_int_distribution<> yPosDistro(
            polyBoundingBox.GetTop(), polyBoundingBox.GetTop() + polyBoundingBox.GetHeight());

        wxPoint lastForcedPt{ polyBoundingBox.GetTopLeft() };
        for (auto labelPos = labels.begin(); labelPos < labels.end(); /*in loop*/)
            {
            bool successfullyPlaced{ false };
            // the less words that have been drawn so far, the more aggressively we should try to
            // fit the current word as it will be a wider then the remaining words
            const size_t placementAttempts = (drawnRects.size() <= 5)     ? 100 :
                                             (drawnRects.size() <= 10)    ? 50 :
                                             (drawnRects.size() <= 100)   ? 25 :
                                             (drawnRects.size() <= 1'000) ? 10 :
                                                                            5;
            for (size_t i = 0; i < placementAttempts; ++i)
                {
                if (tryPlaceLabel(*labelPos, polyBoundingBox,
                                  wxPoint{ xPosDistro(m_mt), yPosDistro(m_mt) }))
                    {
                    labelPos = labels.erase(labelPos);
                    successfullyPlaced = true;
                    break;
                    }
                }
            // If the first (and implicitly widest) label didn't get placed in the rect,
            // then force it to be drawn in the center. This will help with ensuring that
            // more frequently occurring words are shown.
            if (!successfullyPlaced && drawnRects.empty())
                {
                auto bBox = (*labelPos)->GetBoundingBox(dc);
                // if it can fit, then center it
                if (bBox.GetWidth() <= polyBoundingBox.GetWidth() &&
                    bBox.GetHeight() <= polyBoundingBox.GetHeight())
                    {
                    const wxPoint centerPoint{
                        polyBoundingBox.GetLeft() + (polyBoundingBox.GetWidth() / 2),
                        polyBoundingBox.GetTop() + (polyBoundingBox.GetHeight() / 2)
                    };

                    bBox.SetTopLeft({ centerPoint.x - (bBox.GetWidth() / 2),
                                      centerPoint.y - (bBox.GetHeight() / 2) });
                    (*labelPos)->SetAnchorPoint({ centerPoint.x - (bBox.GetWidth() / 2),
                                                  centerPoint.y - (bBox.GetHeight() / 2) });
                    drawnRects.push_back(bBox);
                    AddObject(std::move(*labelPos));
                    labelPos = labels.erase(labelPos);
                    }
                else
                    {
                    ++labelPos;
                    }
                }
            // not the first, but one of the top ten widest labels couldn't be placed,
            // so try to force it in the first empty spot, going left-to-right (going downward)
            else if (!successfullyPlaced && drawnRects.size() <= 10)
                {
                auto bBox = (*labelPos)->GetBoundingBox(dc);
                while (true)
                    {
                    if (lastForcedPt.x + bBox.GetWidth() > polyBoundingBox.GetWidth())
                        {
                        lastForcedPt.x = polyBoundingBox.GetX();
                        ++lastForcedPt.y;
                        if (lastForcedPt.y + bBox.GetHeight() > polyBoundingBox.GetBottom())
                            {
                            break;
                            }
                        }
                    if (tryPlaceLabel(*labelPos, polyBoundingBox, lastForcedPt))
                        {
                        labelPos = labels.erase(labelPos);
                        successfullyPlaced = true;
                        lastForcedPt = bBox.GetTopLeft();
                        break;
                        }
                    ++lastForcedPt.x;

                    // we are out of space vertically, just give up finally
                    if (lastForcedPt.y + bBox.GetHeight() > polyBoundingBox.GetBottom())
                        {
                        break;
                        }
                    }
                // wasn't erased, so skip over it
                if (!successfullyPlaced)
                    {
                    ++labelPos;
                    }
                }
            // wasn't erased, so skip over it
            else if (!successfullyPlaced)
                {
                ++labelPos;
                }
            }
        }
    } // namespace Wisteria::Graphs
