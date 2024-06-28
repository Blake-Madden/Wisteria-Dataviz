///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lrroadmap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LRRoadmap, Wisteria::Graphs::Roadmap);

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void LRRoadmap::SetData(std::shared_ptr<const Data::Dataset> data,
        const wxString& predictorColumnName,
        const wxString& coefficentColumnName,
        const std::optional<wxString>& pValueColumnName /*= std::nullopt*/,
        const std::optional<double> pLevel /*= std::nullopt*/,
        const std::optional<Influence> preditorsToIncludes /*= std::nullopt*/,
        const std::optional<wxString> dvName /*= std::nullopt*/)
        {
        if (data == nullptr)
            { return; }

        if (dvName)
            { SetGoalLabel(dvName.value()); }

        auto predictorColumn = data->GetCategoricalColumn(predictorColumnName);
        if (predictorColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': IV name column not found for roadmap."),
                predictorColumnName).ToUTF8());
            }
        auto coefficientColumn = data->GetContinuousColumn(coefficentColumnName);
        if (coefficientColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': coefficient column not found for roadmap."),
                coefficentColumnName).ToUTF8());
            }
        auto pValueColumn = (pValueColumnName.has_value() ?
            data->GetContinuousColumn(pValueColumnName.value()) :
            data->GetContinuousColumns().cend());
        if (pValueColumnName && pValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': p-value column not found for roadmap."),
                pValueColumnName.value()).ToUTF8());
            }

        auto maxVal = std::max_element(coefficientColumn->GetValues().cbegin(),
            coefficientColumn->GetValues().cend(),
            [](auto lh, auto rh)
            { return std::abs(lh) < std::abs(rh); });
        // set the magnitude to the strongest coefficient (either negative or positive)
        SetMagnitude(std::abs(*maxVal));
        // if no valid coefficients, then quit 
        if (std::isnan(GetMagnitude()))
            { return; }

        const auto includePredictor = [&](const double value, const std::optional<double> pValue)
            {
            if (std::isnan(value))
                { return false; }
            else if (pLevel.has_value() && !std::isnan(pLevel.value()) &&
                pValue.has_value() &&
                (std::isnan(pValue.value()) || pValue >= pLevel.value()) )
                { return false; }
            // default to all included
            else if (!preditorsToIncludes.has_value())
                { return true; }
            else if ((preditorsToIncludes.value() & Influence::InfluenceAll) == Influence::InfluenceAll)
                { return true; }
            else if ((preditorsToIncludes.value() & Influence::InfluenceNegative) == Influence::InfluenceNegative &&
                value < 0)
                { return true; }
            else if ((preditorsToIncludes.value() & Influence::InfluenceNeutral) == Influence::InfluenceNeutral &&
                value == 0)
                { return true; }
            else if ((preditorsToIncludes.value() & Influence::InfluencePositive) == Influence::InfluencePositive &&
                value > 0)
                { return true; }
            else
                { return false; }
            };

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (includePredictor(coefficientColumn->GetValue(i),
                                 (pValueColumn != data->GetContinuousColumns().cend() ?
                                  std::optional<double>(pValueColumn->GetValue(i)) :
                                  std::nullopt)) )
                {
                GetRoadStops().push_back(
                    RoadStopInfo(predictorColumn->GetLabelFromID(predictorColumn->GetValue(i))).
                    Value(coefficientColumn->GetValue(i)));
                }
            }
        }

    //----------------------------------------------------------------
    void LRRoadmap::AddDefaultCaption()
        {
        GetCaption().SetText(wxString::Format(
            _(L"The larger the map marker and deeper the curve, "
               "the stronger the item's association with %s"), GetGoalLabel()));
        }
    }
