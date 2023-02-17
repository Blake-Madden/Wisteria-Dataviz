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
            // most frequent to least frequent
            [](const auto& lhv, const auto& rhv) noexcept
            { return lhv.m_frequency > rhv.m_frequency; });

        // remove words that don't meet the minimum frequency
        auto wordsToRemoveStart = std::find_if(m_words.cbegin(), m_words.cend(),
            [this](const auto& item) noexcept
            { return item.m_frequency < m_minFrequency; });
        if (wordsToRemoveStart != m_words.cend())
            { m_words.erase(wordsToRemoveStart, m_words.end()); }

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
        double labelsArea{ 0.0 };
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
            labelsArea += bBox.GetWidth() * bBox.GetHeight();
            maxWidth = std::max({ maxWidth, bBox.GetWidth() });
            maxHeight = std::max({ maxHeight, bBox.GetHeight() });
            origin.y += bBox.GetHeight();
            labels.push_back(currentLabel);
            ++wordCount;
            }

        // a cloud polygon
        std::vector<wxPoint> polygon
            {
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

        const auto polyArea = Polygon::GetPolygonArea(polygon) * math_constants::half;
        const auto polygonBoundingBox = Polygon::GetPolygonBoundingBox(polygon);

        const double getWidthRescale = safe_divide<double>(Polygon::GetPolygonWidth(polygon), maxWidth);
        const double getHeightRescale = safe_divide<double>(polygonBoundingBox.GetHeight(), maxHeight);
        const double rescaleValue = std::min({ getWidthRescale, getHeightRescale,
                                               std::sqrt(safe_divide<double>(polyArea, labelsArea)) });

        for (auto& label : labels)
            {
            label->SetScaling(label->GetScaling() * rescaleValue);
            label->SetMinimumUserSizeDIPs(std::nullopt, std::nullopt);
            }

        // the less words, the more aggressively we should try to fit them together
        m_placementAttempts = (labels.size() < 100) ? 25 :
            (labels.size() < 1'000) ? 10 :
            5;

        // sort remaining labels by width, largest-to-smallest
        std::sort(labels.begin(), labels.end(),
            [&dc](const auto& lhv, const auto& rhv) noexcept
            { return lhv->GetBoundingBox(dc).GetWidth() > rhv->GetBoundingBox(dc).GetWidth(); });

        TryPlaceLabelsInPolygon(labels, dc, polygon);
        }

    //----------------------------------------------------------------
    void WordCloud::TryPlaceLabelsInPolygon(std::vector<std::shared_ptr<GraphItems::Label>>& labels,
                                            wxDC& dc, const std::vector<wxPoint>& polygon)
        {
        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            AddObject(std::make_shared<GraphItems::Polygon>(
                GraphItemInfo().Pen(*wxBLUE), polygon));
            }

        auto polyBoundingBox{ Polygon::GetPolygonBoundingBox(polygon) };

        std::vector<wxRect> drawnRects;

        // random positioning lambda for the last label in the list
        const auto tryPlaceLabel = [&dc, &polygon, &drawnRects, this]
                                    (auto& label, const auto& polyBoundingBox,
                                     const wxPoint pt)
            {
            auto bBox = label->GetBoundingBox(dc); // will already be cached

            if (bBox.GetWidth() > polyBoundingBox.GetWidth() ||
                bBox.GetHeight() > polyBoundingBox.GetWidth())
                { return false; }

            // make it fit with the drawing area
            bBox.SetTopLeft(pt);
            AdjustRectToDrawArea(bBox, polyBoundingBox);

            auto foundPos = std::find_if(std::execution::par,
                drawnRects.cbegin(), drawnRects.cend(),
                [&bBox](const auto& rect) noexcept
                { return rect.Intersects(bBox); });
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

            if (Polygon::IsRectInsidePolygon(bBox, polygon))
                {
                // place it and add it to be rendered
                drawnRects.push_back(bBox);
                label->SetAnchorPoint(bBox.GetTopLeft());
                AddObject(label);

                return true;
                }
            else
                { return false; }
            };

        std::uniform_int_distribution<> xPosDistro(polyBoundingBox.GetLeft(),
                                                   polyBoundingBox.GetLeft() + polyBoundingBox.GetWidth());
        std::uniform_int_distribution<> yPosDistro(polyBoundingBox.GetTop(),
                                                   polyBoundingBox.GetTop() + polyBoundingBox.GetHeight());

        for (auto labelPos = labels.begin(); labelPos < labels.end(); /*in loop*/)
            {
            bool sucessfullyPlaced{ false };
            for (size_t i = 0; i < m_placementAttempts; ++i)
                {
                if (tryPlaceLabel(*labelPos, polyBoundingBox, wxPoint{ xPosDistro(m_mt), yPosDistro(m_mt) }))
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
                if (bBox.GetWidth() <= polyBoundingBox.GetWidth() &&
                    bBox.GetHeight() <= polyBoundingBox.GetHeight())
                    {
                    const wxPoint centerPoint{ polyBoundingBox.GetLeft() + (polyBoundingBox.GetWidth() / 2),
                                               polyBoundingBox.GetTop() + (polyBoundingBox.GetHeight() / 2) };

                    bBox.SetTopLeft(
                        { centerPoint.x - (bBox.GetWidth() / 2),
                          centerPoint.y - (bBox.GetHeight() / 2) });
                    (*labelPos)->SetAnchorPoint(
                        { centerPoint.x - (bBox.GetWidth() / 2),
                          centerPoint.y - (bBox.GetHeight() / 2) });
                    drawnRects.push_back(bBox);
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
