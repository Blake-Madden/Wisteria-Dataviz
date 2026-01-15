///////////////////////////////////////////////////////////////////////////////
// Name:        lrroadmap.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
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

        const auto predictorColumn = data->GetCategoricalColumn(predictorColumnName);
        if (predictorColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                // TRANSLATORS: "IV" means independent variable.
                wxString::Format(_(L"'%s': IV name column not found for roadmap."),
                                 predictorColumnName)
                    .ToUTF8());
            }
        const auto coefficientColumn = data->GetContinuousColumn(coefficientColumnName);
        if (coefficientColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': coefficient column not found for roadmap."),
                                 coefficientColumnName)
                    .ToUTF8());
            }
        const auto pValueColumn =
            (pValueColumnName.has_value() ? data->GetContinuousColumn(pValueColumnName.value()) :
                                            data->GetContinuousColumns().cend());
        if (pValueColumnName && pValueColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': p-value column not found for roadmap."),
                                 pValueColumnName.value())
                    .ToUTF8());
            }

        const auto& vals = coefficientColumn->GetValues();
        std::vector<double> finite;
        finite.reserve(vals.size());
        for (const double val : vals)
            {
            if (std::isfinite(val))
                {
                finite.push_back(std::abs(val));
                }
            }

        if (finite.empty())
            {
            return;
            }

        // set the magnitude to the strongest coefficient (either negative or positive)
        SetMagnitude(*std::ranges::max_element(finite));
        // if no valid coefficients, then quit
        if (!std::isfinite(GetMagnitude()))
            {
            return;
            }

        const auto includePredictor = [&](const double value, const std::optional<double> pValue)
        {
            if (!std::isfinite(value))
                {
                return false;
                }
            // if there is a provided p-level cutoff and the p-level is either invalid
            // or higher than the cutoff, then this predictor is not significant
            if (pLevel.has_value() && std::isfinite(pLevel.value()) && pValue.has_value() &&
                (!std::isfinite(pValue.value()) || pValue.value() >= pLevel.value()))
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
                compare_doubles_less(value, 0))
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluenceNeutral) ==
                    Influence::InfluenceNeutral &&
                compare_doubles(value, 0))
                {
                return true;
                }
            if ((predictorsToIncludes.value() & Influence::InfluencePositive) ==
                    Influence::InfluencePositive &&
                compare_doubles_greater(value, 0))
                {
                return true;
                }
            return false;
        };

        GetRoadStops().clear();
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
