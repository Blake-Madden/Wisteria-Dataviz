///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "proconroadmap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ProConRoadmap, Wisteria::Graphs::Roadmap)

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void ProConRoadmap::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& positiveColumnName,
        const std::optional<wxString>& positiveValueColumnName,
        const wxString& negativeColumnName,
        const std::optional<wxString>& negativeValueColumnName,
        const std::optional<size_t>& minimumCountForItem)
        {
        if (data == nullptr)
            { return; }

        // get positive columns
        auto positiveColumn = data->GetCategoricalColumn(positiveColumnName);
        if (positiveColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': positive influencers name column not found for roadmap."),
                positiveColumnName).ToUTF8());
            }
        auto positiveValueColumn = (positiveValueColumnName.has_value() ?
            data->GetContinuousColumn(positiveValueColumnName.value()) :
            data->GetContinuousColumns().cend());
        if (positiveValueColumnName &&
            positiveValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': positive value column not found for roadmap."),
                positiveValueColumnName.value()).ToUTF8());
            }

        // get negative columns
        auto negativeColumn = data->GetCategoricalColumn(negativeColumnName);
        if (negativeColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': negative influencers name column not found for roadmap."),
                negativeColumnName).ToUTF8());
            }
        auto negativeValueColumn = (negativeValueColumnName.has_value() ?
            data->GetContinuousColumn(negativeValueColumnName.value()) :
            data->GetContinuousColumns().cend());
        if (negativeValueColumnName &&
            negativeValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': negative value column not found for roadmap."),
                negativeValueColumnName.value()).ToUTF8());
            }

        // calculate how many positive and negative items there are
        aggregate_frequency_set<wxString> influencers;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            // only include item if it has a valid label and a
            // valid aggregate count (if aggregate value is in use)
            if (!(positiveValueColumnName.has_value() &&
                  std::isnan(positiveValueColumn->GetValue(i))) &&
                positiveColumn->GetLabelFromID(positiveColumn->GetValue(i)).length())
                {
                influencers.insert(
                    positiveColumn->GetLabelFromID(positiveColumn->GetValue(i)),
                    (positiveValueColumnName.has_value() ?
                        // force positive influencers to be positive
                        // (in case data had negative values)
                        std::abs(positiveValueColumn->GetValue(i)) : 1));
                }
            if (!(negativeValueColumnName.has_value() &&
                  std::isnan(negativeValueColumn->GetValue(i))) &&
                negativeColumn->GetLabelFromID(negativeColumn->GetValue(i)).length())
                {
                influencers.insert(
                    negativeColumn->GetLabelFromID(negativeColumn->GetValue(i)),
                    // force negative influencers to be negative
                    // (in case data had negative values)
                    -(negativeValueColumnName.has_value() ?
                      std::abs(negativeValueColumn->GetValue(i)) : 1));
                }
            }

        if (minimumCountForItem.has_value())
            {
            for (auto iter = influencers.get_data().begin();
                 iter != influencers.get_data().end();
                 /* in loop*/)
                {
                if (std::abs(iter->second.second) < minimumCountForItem.value())
                    { iter = influencers.get_data().erase(iter); }
                else
                    { ++iter; }
                }
            }

        // if nothing left after filtering then quit
        if (influencers.get_data().size() == 0)
            { return; }

        // Get the range of values, which will be the magnitude (not raw values).
        // In other words, we set the values to positive and then get the min and max
        std::vector<double> values;
        values.reserve(influencers.get_data().size());
        for (const auto& influencer : influencers.get_data())
            { values.push_back(std::abs(influencer.second.second)); }

        auto maxVal = std::max_element(values.cbegin(), values.cend());
        // set the magnitude to the highest category count
        SetMagnitude(std::abs(*maxVal));

        // add the influencers as road stops
        for (const auto& influencer : influencers.get_data())
            {
            GetRoadStops().push_back(
                RoadStopInfo(influencer.first).
                Value(influencer.second.second));
            }
        }

    //----------------------------------------------------------------
    void ProConRoadmap::AddDefaultCaption()
        {
        GetCaption().SetText(_(L"The larger the map marker and deeper the curve, "
              "the more responses for the positive or negative sentiment"));
        }
    }
