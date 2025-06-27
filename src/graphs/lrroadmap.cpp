///////////////////////////////////////////////////////////////////////////////
// Name:        lrroadmap.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lrroadmap.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LRRoadmap, Wisteria::Graphs::Roadmap)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void LRRoadmap::SetData(const std::shared_ptr<const Data::Dataset>& data,
                            const wxString& predictorColumnName,
                            const wxString& coefficientColumnName,
                            const std::optional<wxString>& pValueColumnName /*= std::nullopt*/,
                            const std::optional<double> pLevel /*= std::nullopt*/,
                            const std::optional<Influence> predictorsToIncludes /*= std::nullopt*/,
                            const std::optional<wxString>& dvName /*= std::nullopt*/)
        {
        if (data == nullptr)
            {
            return;
            }

        if (dvName)
            {
            SetGoalLabel(dvName.value());
            }

        auto predictorColumn = data->GetCategoricalColumn(predictorColumnName);
        if (predictorColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                // TRANSLATORS: "IV" means independent variable.
                wxString::Format(_(L"'%s': IV name column not found for roadmap."),
                                 predictorColumnName)
                    .ToUTF8());
            }
        auto coefficientColumn = data->GetContinuousColumn(coefficientColumnName);
        if (coefficientColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': coefficient column not found for roadmap."),
                                 coefficientColumnName)
                    .ToUTF8());
            }
        auto pValueColumn =
            (pValueColumnName.has_value() ? data->GetContinuousColumn(pValueColumnName.value()) :
                                            data->GetContinuousColumns().cend());
        if (pValueColumnName && pValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': p-value column not found for roadmap."),
                                 pValueColumnName.value())
                    .ToUTF8());
            }

        const auto maxVal =
            std::ranges::max_element(coefficientColumn->GetValues(),
                                     [](auto lh, auto rh) { return std::abs(lh) < std::abs(rh); });
        // set the magnitude to the strongest coefficient (either negative or positive)
        SetMagnitude(std::abs(*maxVal));
        // if no valid coefficients, then quit
        if (std::isnan(GetMagnitude()))
            {
            return;
            }

        const auto includePredictor = [&](const double value, const std::optional<double> pValue)
        {
            if (std::isnan(value))
                {
                return false;
                }
            if (pLevel.has_value() && !std::isnan(pLevel.value()) && pValue.has_value() &&
                (std::isnan(pValue.value()) || pValue >= pLevel.value()))
                {
                return false;
                }
            // default to all included
            if (!predictorsToIncludes.has_value())
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluenceAll) == Influence::InfluenceAll)
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluenceNegative) ==
                    Influence::InfluenceNegative &&
                value < 0)
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluenceNeutral) ==
                    Influence::InfluenceNeutral &&
                value == 0)
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluencePositive) ==
                    Influence::InfluencePositive &&
                value > 0)
                {
                return true;
                }
            return false;
        };

        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (includePredictor(coefficientColumn->GetValue(i),
                                 (pValueColumn != data->GetContinuousColumns().cend() ?
                                      std::optional<double>(pValueColumn->GetValue(i)) :
                                      std::nullopt)))
                {
                GetRoadStops().push_back(
                    RoadStopInfo(predictorColumn->GetLabelFromID(predictorColumn->GetValue(i)))
                        .Value(coefficientColumn->GetValue(i)));
                }
            }
        }

    //----------------------------------------------------------------
    void LRRoadmap::AddDefaultCaption()
        {
        GetCaption().SetText(wxString::Format(_(L"The larger the map marker and deeper the curve, "
                                                "the stronger the item's association with %s"),
                                              GetGoalLabel()));
        }
    } // namespace Wisteria::Graphs
