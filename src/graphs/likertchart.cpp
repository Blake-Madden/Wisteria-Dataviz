///////////////////////////////////////////////////////////////////////////////
// Name:        likertchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "likertchart.h"
#include <ranges>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LikertChart, Wisteria::Graphs::BarChart)

    namespace Wisteria::Graphs
    {
    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::MakeFormatCategorized(
        const LikertSurveyQuestionFormat format) noexcept
        {
        // NOLINTBEGIN(misc-redundant-expression)
        return (format == LikertSurveyQuestionFormat::TwoPoint ||
                format == LikertSurveyQuestionFormat::TwoPointCategorized) ?
                   LikertSurveyQuestionFormat::TwoPointCategorized :
               (format == LikertSurveyQuestionFormat::ThreePoint ||
                format == LikertSurveyQuestionFormat::ThreePointCategorized) ?
                   LikertSurveyQuestionFormat::ThreePointCategorized :
               (format == LikertSurveyQuestionFormat::FourPoint ||
                format == LikertSurveyQuestionFormat::FourPointCategorized) ?
                   LikertSurveyQuestionFormat::FourPointCategorized :
               (format == LikertSurveyQuestionFormat::FivePoint ||
                format == LikertSurveyQuestionFormat::FivePointCategorized) ?
                   LikertSurveyQuestionFormat::FivePointCategorized :
               (format == LikertSurveyQuestionFormat::SixPoint ||
                format == LikertSurveyQuestionFormat::SixPointCategorized) ?
                   LikertSurveyQuestionFormat::SixPointCategorized :
               (format == LikertSurveyQuestionFormat::SevenPoint ||
                format == LikertSurveyQuestionFormat::SevenPointCategorized) ?
                   LikertSurveyQuestionFormat::SevenPointCategorized :
                   LikertSurveyQuestionFormat::SevenPointCategorized;
        // NOLINTEND(misc-redundant-expression)
        }

    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::MakeFormatUncategorized(
        const LikertSurveyQuestionFormat format) noexcept
        {
        // NOLINTBEGIN(misc-redundant-expression)
        return (format == LikertSurveyQuestionFormat::TwoPoint ||
                format == LikertSurveyQuestionFormat::TwoPointCategorized) ?
                   LikertSurveyQuestionFormat::TwoPoint :
               (format == LikertSurveyQuestionFormat::ThreePoint ||
                format == LikertSurveyQuestionFormat::ThreePointCategorized) ?
                   LikertSurveyQuestionFormat::ThreePoint :
               (format == LikertSurveyQuestionFormat::FourPoint ||
                format == LikertSurveyQuestionFormat::FourPointCategorized) ?
                   LikertSurveyQuestionFormat::FourPoint :
               (format == LikertSurveyQuestionFormat::FivePoint ||
                format == LikertSurveyQuestionFormat::FivePointCategorized) ?
                   LikertSurveyQuestionFormat::FivePoint :
               (format == LikertSurveyQuestionFormat::SixPoint ||
                format == LikertSurveyQuestionFormat::SixPointCategorized) ?
                   LikertSurveyQuestionFormat::SixPoint :
               (format == LikertSurveyQuestionFormat::SevenPoint ||
                format == LikertSurveyQuestionFormat::SevenPointCategorized) ?
                   LikertSurveyQuestionFormat::SevenPoint :
                   LikertSurveyQuestionFormat::SevenPoint;
        // NOLINTEND(misc-redundant-expression)
        }

    bool LikertChart::IsCategorized(const LikertSurveyQuestionFormat format) noexcept
        {
        return (format == LikertSurveyQuestionFormat::TwoPointCategorized ||
                format == LikertSurveyQuestionFormat::ThreePointCategorized ||
                format == LikertSurveyQuestionFormat::FourPointCategorized ||
                format == LikertSurveyQuestionFormat::FivePointCategorized ||
                format == LikertSurveyQuestionFormat::SixPointCategorized ||
                format == LikertSurveyQuestionFormat::SevenPointCategorized);
        }

    //-----------------------------------
    void LikertChart::SetLabels(const std::shared_ptr<Data::Dataset>& data,
                                const std::vector<wxString>& questionColumns,
                                const Data::ColumnWithStringTable::StringTableType& codes)
        {
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            categoricalColumn->GetStringTable() = codes;
            }
        }

    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::Simplify(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const LikertSurveyQuestionFormat currentFormat)
        {
        // 7 -> 3
        if (currentFormat == LikertSurveyQuestionFormat::SevenPoint ||
            currentFormat == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            Collapse7PointsTo3(data, questionColumns,
                               CreateLabels(LikertSurveyQuestionFormat::ThreePoint));
            return (IsCategorized(currentFormat) ?
                        LikertSurveyQuestionFormat::ThreePointCategorized :
                        LikertSurveyQuestionFormat::ThreePoint);
            }
        // 6 -> 2
        if (currentFormat == LikertSurveyQuestionFormat::SixPoint ||
            currentFormat == LikertSurveyQuestionFormat::SixPointCategorized)
            {
            Collapse6PointsTo2(data, questionColumns,
                               CreateLabels(LikertSurveyQuestionFormat::TwoPoint));
            return (IsCategorized(currentFormat) ? LikertSurveyQuestionFormat::TwoPointCategorized :
                                                   LikertSurveyQuestionFormat::TwoPoint);
            }
        // 5 -> 3
        if (currentFormat == LikertSurveyQuestionFormat::FivePoint ||
            currentFormat == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            Collapse5PointsTo3(data, questionColumns,
                               CreateLabels(LikertSurveyQuestionFormat::ThreePoint));
            return (IsCategorized(currentFormat) ?
                        LikertSurveyQuestionFormat::ThreePointCategorized :
                        LikertSurveyQuestionFormat::ThreePoint);
            }
        // 4 -> 2
        if (currentFormat == LikertSurveyQuestionFormat::FourPoint ||
            currentFormat == LikertSurveyQuestionFormat::FourPointCategorized)
            {
            Collapse4PointsTo2(data, questionColumns,
                               CreateLabels(LikertSurveyQuestionFormat::TwoPoint));
            return (IsCategorized(currentFormat) ? LikertSurveyQuestionFormat::TwoPointCategorized :
                                                   LikertSurveyQuestionFormat::TwoPoint);
            }
        // 3 and 2 are already as simple as they are going to get,
        // just ensure their labels are correct
        if (currentFormat == LikertSurveyQuestionFormat::ThreePoint ||
            currentFormat == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            SetLabels(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::ThreePoint));
            }
        else if (currentFormat == LikertSurveyQuestionFormat::TwoPoint ||
                 currentFormat == LikertSurveyQuestionFormat::TwoPointCategorized)
            {
            SetLabels(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::TwoPoint));
            }
        return currentFormat;
        }

    //-----------------------------------
    void LikertChart::Collapse4PointsTo2(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        assert(condensedCodes.size() == 3 && L"String table should have 3 values!");
        assert(std::ranges::min_element(condensedCodes)->first == 0 &&
               L"String table should start at zero!");
        assert(std::ranges::max_element(condensedCodes)->first == 2 &&
               L"String table should end at 2!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            assert(*std::max_element(categoricalColumn->GetValues().cbegin(),
                                     categoricalColumn->GetValues().cend()) <= 4 &&
                   L"Categorical codes shouldn't be higher than 4!");
            // collapse both degrees of "negative" into one
            categoricalColumn->Recode(2, 1);
            // collapse both degrees of "positive" into one
            categoricalColumn->Recode(3, 2);
            categoricalColumn->Recode(4, 2);
            // use the simpler string table
            categoricalColumn->GetStringTable() = condensedCodes;
            }
        }

    //-----------------------------------
    void LikertChart::Collapse6PointsTo2(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        assert(condensedCodes.size() == 3 && L"String table should have 3 values!");
        assert(std::min_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 0 &&
               L"String table should start at zero!");
        assert(std::max_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 2 &&
               L"String table should end at 2!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            assert(*std::max_element(categoricalColumn->GetValues().cbegin(),
                                     categoricalColumn->GetValues().cend()) <= 6 &&
                   L"Categorical codes shouldn't be higher than 6!");
            // collapse all degrees of "negative" into one
            categoricalColumn->Recode(2, 1);
            categoricalColumn->Recode(3, 1);
            // collapse all degrees of "positive" into one
            categoricalColumn->Recode(4, 2);
            categoricalColumn->Recode(5, 2);
            categoricalColumn->Recode(6, 2);
            // use the simpler string table
            categoricalColumn->GetStringTable() = condensedCodes;
            }
        }

    //-----------------------------------
    void LikertChart::Collapse5PointsTo3(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        assert(condensedCodes.size() == 4 && L"String table should have 4 values!");
        assert(std::min_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 0 &&
               L"String table should start at zero!");
        assert(std::max_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 3 &&
               L"String table should end at 3!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            assert(*std::max_element(categoricalColumn->GetValues().cbegin(),
                                     categoricalColumn->GetValues().cend()) <= 5 &&
                   L"Categorical codes shouldn't be higher than 5!");
            // collapse both degrees of "negative" into one
            categoricalColumn->Recode(2, 1);
            // old neutral code
            categoricalColumn->Recode(3, 2);
            // collapse both degrees of "positive" into one
            categoricalColumn->Recode(4, 3);
            categoricalColumn->Recode(5, 3);
            // use the simpler string table
            categoricalColumn->GetStringTable() = condensedCodes;
            }
        }

    //-----------------------------------
    void LikertChart::Collapse7PointsTo3(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        assert(condensedCodes.size() == 4 && L"String table should have 4 values!");
        assert(std::min_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 0 &&
               L"String table should start at zero!");
        assert(std::max_element(condensedCodes.cbegin(), condensedCodes.cend())->first == 3 &&
               L"String table should end at 3!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            assert(*std::max_element(categoricalColumn->GetValues().cbegin(),
                                     categoricalColumn->GetValues().cend()) <= 7 &&
                   L"Categorical codes shouldn't be higher than 7!");
            // collapse all three degrees of "negative" into one
            categoricalColumn->Recode(2, 1);
            categoricalColumn->Recode(3, 1);
            // old neutral code
            categoricalColumn->Recode(4, 2);
            // collapse all three degrees of "positive" into one
            categoricalColumn->Recode(5, 3);
            categoricalColumn->Recode(6, 3);
            categoricalColumn->Recode(7, 3);
            // use the simpler string table
            categoricalColumn->GetStringTable() = condensedCodes;
            }
        }

    //-----------------------------------
    Data::ColumnWithStringTable::StringTableType LikertChart::CreateLabels(
        const LikertChart::LikertSurveyQuestionFormat& type)
        {
        switch (type)
            {
        // 0-7
        case LikertSurveyQuestionFormat::SevenPointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::SevenPoint:
            return { { 0, _(L"No response").wc_str() }, { 1, _(L"Strongly disagree").wc_str() },
                     { 2, _(L"Disagree").wc_str() },    { 3, _(L"Somewhat disagree").wc_str() },
                     { 4, _(L"Neutral").wc_str() },     { 5, _(L"Somewhat agree").wc_str() },
                     { 6, _(L"Agree").wc_str() },       { 7, _(L"Strongly agree").wc_str() } };
        // 0-6
        case LikertSurveyQuestionFormat::SixPointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::SixPoint:
            return { { 0, _(L"No response").wc_str() },    { 1, _(L"Strongly disagree").wc_str() },
                     { 2, _(L"Disagree").wc_str() },       { 3, _(L"Somewhat disagree").wc_str() },
                     { 4, _(L"Somewhat agree").wc_str() }, { 5, _(L"Agree").wc_str() },
                     { 6, _(L"Strongly agree").wc_str() } };
        // 0-5
        case LikertSurveyQuestionFormat::FivePointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::FivePoint:
            return { { 0, _(L"No response").wc_str() },       { 1, _(L"Disagree").wc_str() },
                     { 2, _(L"Somewhat disagree").wc_str() }, { 3, _(L"Neutral").wc_str() },
                     { 4, _(L"Somewhat agree").wc_str() },    { 5, _(L"Agree").wc_str() } };
        // 0-4
        case LikertSurveyQuestionFormat::FourPointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::FourPoint:
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Disagree").wc_str() },
                     { 2, _(L"Somewhat disagree").wc_str() },
                     { 3, _(L"Somewhat agree").wc_str() },
                     { 4, _(L"Agree").wc_str() } };
        // 0-3
        case LikertSurveyQuestionFormat::ThreePointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::ThreePoint:
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Disagree").wc_str() },
                     { 2, _(L"Neutral").wc_str() },
                     { 3, _(L"Agree").wc_str() } };
        // 0-2
        case LikertSurveyQuestionFormat::TwoPointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::TwoPoint:
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Disagree").wc_str() },
                     { 2, _(L"Agree").wc_str() } };
            };

        return Data::ColumnWithStringTable::StringTableType{};
        }

    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::DeduceScale(
        const std::shared_ptr<Data::Dataset>& data, const std::vector<wxString>& questionColumns,
        const std::optional<wxString>& groupColumnName /*= std::nullopt*/)
        {
        Data::GroupIdType maxVal{ 0 };
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     catColumnName)
                        .ToUTF8());
                }
            if (!categoricalColumn->GetValues().empty())
                {
                maxVal =
                    std::max(*std::ranges::max_element(categoricalColumn->GetValues()), maxVal);
                if (maxVal > 7)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Invalid Likert response: %" PRIu64
                                           "\nColumn: %s\nValues should not exceed 7."),
                                         maxVal, categoricalColumn->GetName())
                            .ToUTF8());
                    }
                }
            }

        const auto groupColumn =
            (groupColumnName ? data->GetCategoricalColumn(groupColumnName.value()) :
                               data->GetCategoricalColumns().cend());
        std::set<Data::GroupIdType> uniqueGroups;
        if (groupColumn != data->GetCategoricalColumns().cend())
            {
            for (const auto& val : groupColumn->GetValues())
                {
                uniqueGroups.insert(val);
                }
            }

        // NOLINTBEGIN(misc-redundant-expression)
        const LikertSurveyQuestionFormat format =
            (maxVal == 7) ? LikertSurveyQuestionFormat::SevenPoint :
            (maxVal == 6) ? LikertSurveyQuestionFormat::SixPoint :
            (maxVal == 5) ? LikertSurveyQuestionFormat::FivePoint :
            (maxVal == 4) ? LikertSurveyQuestionFormat::FourPoint :
            (maxVal == 3) ? LikertSurveyQuestionFormat::ThreePoint :
            (maxVal == 2) ? LikertSurveyQuestionFormat::TwoPoint :
                            LikertSurveyQuestionFormat::TwoPoint;
        // NOLINTEND(misc-redundant-expression)

        return (uniqueGroups.size() > 1) ? MakeFormatCategorized(format) : format;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> LikertChart::CreateLegend(const LegendOptions& options)
        {
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo()
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .LabelAlignment(TextAlignment::FlushLeft)
                .DPIScaling(GetDPIScaleFactor()));

        // fill in the legend
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            // don't add neutral to legend if there aren't any neutral responses
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNeutralColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor())
                };
                legend->SetText(GetPositiveLabel(1) + L"\n" + GetNeutralLabel() + L"\n" +
                                GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor())
                };
                legend->SetText(GetPositiveLabel(1) + L"\n" + GetNegativeLabel(1));
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNeutralColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                };
                legend->SetText(GetPositiveLabel(2) + L"\n" + GetPositiveLabel(1) + L"\n" +
                                GetNeutralLabel() + L"\n" + GetNegativeLabel(2) + L"\n" +
                                GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                };
                legend->SetText(GetPositiveLabel(2) + L"\n" + GetPositiveLabel(1) + L"\n" +
                                GetNegativeLabel(2) + L"\n" + GetNegativeLabel(1));
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor(), .40)),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNeutralColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor(), .40))
                };
                legend->SetText(GetPositiveLabel(3) + L"\n" + GetPositiveLabel(2) + L"\n" +
                                GetPositiveLabel(1) + L"\n" + GetNeutralLabel() + L"\n" +
                                GetNegativeLabel(3) + L"\n" + GetNegativeLabel(2) + L"\n" +
                                GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() = {
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor(), .40)),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetPositiveColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetPositiveColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      GetNegativeColor()),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor())),
                    Icons::LegendIcon(Icons::IconShape::Square,
                                      Colors::ColorBrewer::GetColor(Colors::Color::Black),
                                      Colors::ColorContrast::ShadeOrTint(GetNegativeColor(), .40))
                };
                legend->SetText(GetPositiveLabel(3) + L"\n" + GetPositiveLabel(2) + L"\n" +
                                GetPositiveLabel(1) + L"\n" + GetNegativeLabel(3) + L"\n" +
                                GetNegativeLabel(2) + L"\n" + GetNegativeLabel(1));
                }
            }

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    LikertChart::LikertChart(Canvas * canvas, const LikertSurveyQuestionFormat type,
                             const std::optional<wxColour>& negativeColor /*= std::nullopt*/,
                             const std::optional<wxColour>& positiveColor /*= std::nullopt*/,
                             const std::optional<wxColour>& neutralColor /*= std::nullopt*/,
                             const std::optional<wxColour>& noResponseColor /*= std::nullopt*/)
        : BarChart(canvas), m_surveyType(type)
        {
        SetNegativeColor(negativeColor.value_or(wxNullColour));
        SetPositiveColor(positiveColor.value_or(wxNullColour));
        SetNeutralColor(neutralColor.value_or(wxNullColour));
        SetNoResponseColor(noResponseColor.value_or(wxNullColour));

        SetBarOrientation(Orientation::Horizontal);
        SetBinLabelDisplay(BinLabelDisplay::NoDisplay);
        GetScalingAxis().GetGridlinePen() = wxNullPen;
        GetBarAxis().GetGridlinePen() = wxNullPen;

        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetLeftYAxis().GetAxisLinePen().IsOk())
            {
            GetBarAxis().GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            }
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetScalingAxis().GetAxisLinePen().IsOk())
            {
            GetScalingAxis().GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            }
        MirrorXAxis(true);
        MirrorYAxis(true);

        IncludeSpacesBetweenBars(true);
        }

    //----------------------------------------------------------------
    void LikertChart::SetData(const std::shared_ptr<Data::Dataset>& data,
                              const std::vector<wxString>& questionColumns,
                              const std::optional<wxString>& groupColumnName /*= std::nullopt*/)
        {
        if (data == nullptr)
            {
            return;
            }

        GetSelectedIds().clear();
        ClearBarGroups();
        m_positive1Label.clear();
        m_positive2Label.clear();
        m_positive3Label.clear();
        m_negative1Label.clear();
        m_negative2Label.clear();
        m_negative3Label.clear();
        // if no string tables in the data, then this will be used for the section header
        m_neutralLabel = _(L"Neutral");

        const auto groupColumn =
            (groupColumnName ? data->GetCategoricalColumn(groupColumnName.value()) :
                               data->GetCategoricalColumns().cend());
        if (groupColumnName && groupColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': group column not found for Likert chart."),
                                 groupColumnName.value())
                    .ToUTF8());
            }
        // if a grouping column is used, then make the format categorized
        if (groupColumnName)
            {
            m_surveyType = MakeFormatCategorized(m_surveyType);
            }
        else
            {
            m_surveyType = MakeFormatUncategorized(m_surveyType);
            }

        // go in reverse order so that the first to last questions go from top-to-bottom
        for (const auto& questionColumn : std::ranges::reverse_view(questionColumns))
            {
            const auto categoricalColumn = data->GetCategoricalColumn(questionColumn);
            if (categoricalColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found for Likert chart."),
                                     questionColumn)
                        .ToUTF8());
                }
            if (IsCategorized() && groupColumnName.has_value())
                {
                AddSurveyQuestion(questionColumn, *groupColumn, *categoricalColumn);
                }
            else
                {
                AddSurveyQuestion(questionColumn, *categoricalColumn);
                }

            // set the level labels from the data's string table,
            // these will be used for the legend
            if (!categoricalColumn->GetStringTable().empty())
                {
                if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(2), 1);
                    }
                else if (GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint ||
                         GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetNeutralLabel(categoricalColumn->GetLabelFromID(2));
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(3), 1);
                    }
                else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
                         GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(2), 2);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(3), 1);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(4), 2);
                    }
                else if (GetSurveyType() == LikertSurveyQuestionFormat::FivePoint ||
                         GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(2), 2);
                    SetNeutralLabel(categoricalColumn->GetLabelFromID(3));
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(4), 1);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(5), 2);
                    }
                else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
                         GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(2), 2);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(3), 3);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(4), 1);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(5), 2);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(6), 3);
                    }
                else if (GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint ||
                         GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
                    {
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(1), 1);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(2), 2);
                    SetNegativeLabel(categoricalColumn->GetLabelFromID(3), 3);
                    SetNeutralLabel(categoricalColumn->GetLabelFromID(4));
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(5), 1);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(6), 2);
                    SetPositiveLabel(categoricalColumn->GetLabelFromID(7), 3);
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const wxString& question,
                                        const Data::ColumnWithStringTable& groups,
                                        const Data::ColumnWithStringTable& responses)
        {
        if (responses.GetRowCount() == 0)
            {
            return;
            }
        assert(IsCategorized() &&
               L"Categorized data being passed into non-categorized Likert chart!");

        m_maxRespondents = std::max(m_maxRespondents, responses.GetRowCount());

        // the group IDs and their frequencies in the data
        std::map<Data::GroupIdType, frequency_set<double>> fMap;
        for (size_t i = 0; i < groups.GetRowCount(); ++i)
            {
            auto foundPos = fMap.find(groups.GetValue(i));
            if (foundPos == fMap.end())
                {
                auto [iterator, found] =
                    fMap.try_emplace(groups.GetValue(i), frequency_set<double>());
                iterator->second.insert(responses.GetValue(i));
                }
            else
                {
                foundPos->second.insert(responses.GetValue(i));
                }
            }

        const auto findCount = [&fMap](const Data::GroupIdType group, const double code) -> size_t
        {
            const auto groupPos = fMap.find(group);
            if (groupPos != fMap.cend())
                {
                const auto found = groupPos->second.get_data().find(code);
                return (found != groupPos->second.get_data().cend()) ? found->second : 0;
                }

            return 0;
        };

        const auto findNACount = [&fMap](const Data::GroupIdType group) -> size_t
        {
            const auto groupPos = fMap.find(group);
            if (groupPos != fMap.cend())
                {
                size_t naCount{ 0 };
                auto found = groupPos->second.get_data().find(0);
                if (found != groupPos->second.get_data().cend())
                    {
                    naCount += found->second;
                    }
                return naCount;
                }

            return 0;
        };

        [[maybe_unused]]
        size_t groupResponses{ 0 }; // used for assertions
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized)
            {
            LikertCategorizedThreePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertThreePointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    0 /* no neutrals*/, findCount(group.first, 2), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            LikertCategorizedThreePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertThreePointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    findCount(group.first, 2), findCount(group.first, 3), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized)
            {
            LikertCategorizedFivePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertFivePointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    findCount(group.first, 2), 0 /* no neutrals*/, findCount(group.first, 3),
                    findCount(group.first, 4), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            LikertCategorizedFivePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertFivePointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    findCount(group.first, 2), findCount(group.first, 3), findCount(group.first, 4),
                    findCount(group.first, 5), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized)
            {
            LikertCategorizedSevenPointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertSevenPointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    findCount(group.first, 2), findCount(group.first, 3), 0 /* no neutrals*/,
                    findCount(group.first, 4), findCount(group.first, 5), findCount(group.first, 6),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            LikertCategorizedSevenPointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertSevenPointSurveyQuestion surveyQuestion(
                    groups.GetLabelFromID(group.first), findCount(group.first, 1),
                    findCount(group.first, 2), findCount(group.first, 3), findCount(group.first, 4),
                    findCount(group.first, 5), findCount(group.first, 6), findCount(group.first, 7),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            assert(groupResponses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const wxString& question,
                                        const Data::ColumnWithStringTable& responses)
        {
        if (responses.GetRowCount() == 0)
            {
            return;
            }
        assert(!IsCategorized() &&
               L"Non-categorized data being loaded into categorized Likert chart!");

        m_maxRespondents = std::max(m_maxRespondents, responses.GetRowCount());

        frequency_set<double> fSet;
        for (const auto& value : responses.GetValues())
            {
            fSet.insert(value);
            }

        const auto findCount = [&fSet](const double code) -> size_t
        {
            const auto found = fSet.get_data().find(code);
            return (found != fSet.get_data().cend()) ? found->second : 0;
        };

        const auto findNACount = [&fSet]() -> size_t
        {
            size_t naCount{ 0 };
            auto found = fSet.get_data().find(0);
            if (found != fSet.get_data().cend())
                {
                naCount += found->second;
                }
            return naCount;
        };

        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint)
            {
            const LikertThreePointSurveyQuestion surveyQuestion(
                question, findCount(1), 0 /* no neutrals*/, findCount(2), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint)
            {
            const LikertThreePointSurveyQuestion surveyQuestion(question,
                                                                // 1-3, negative to positive
                                                                findCount(1), findCount(2),
                                                                findCount(3), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint)
            {
            const LikertFivePointSurveyQuestion surveyQuestion(question, findCount(1), findCount(2),
                                                               0 /* no neutrals*/, findCount(3),
                                                               findCount(4), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FivePoint)
            {
            const LikertFivePointSurveyQuestion surveyQuestion(question, findCount(1), findCount(2),
                                                               findCount(3), findCount(4),
                                                               findCount(5), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint)
            {
            const LikertSevenPointSurveyQuestion surveyQuestion(
                question, findCount(1), findCount(2), findCount(3), 0 /* no neutrals*/,
                findCount(4), findCount(5), findCount(6), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint)
            {
            const LikertSevenPointSurveyQuestion surveyQuestion(
                question, findCount(1), findCount(2), findCount(3), findCount(4), findCount(5),
                findCount(6), findCount(7), findNACount());
            assert(surveyQuestion.m_responses == responses.GetRowCount() &&
                   L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertThreePointSurveyQuestion& question)
        {
        m_threePointQuestions.push_back(question);
        m_negativeBlockSize =
            std::max(m_negativeBlockSize, next_interval(question.m_negativeRate, 2));
        m_positiveBlockSize =
            std::max(m_positiveBlockSize, next_interval(question.m_positiveRate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize, next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedThreePointSurveyQuestion& question)
        {
        m_threePointCategorizedQuestions.push_back(question);
        for (const auto& category : question.m_threePointCategories)
            {
            m_negativeBlockSize =
                std::max(m_negativeBlockSize, next_interval(category.m_negativeRate, 2));
            m_positiveBlockSize =
                std::max(m_positiveBlockSize, next_interval(category.m_positiveRate, 2));
            m_neutralBlockSize =
                std::max(m_neutralBlockSize, next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertFivePointSurveyQuestion& question)
        {
        m_fivePointQuestions.push_back(question);
        m_negativeBlockSize =
            std::max(m_negativeBlockSize,
                     next_interval(question.m_negative1Rate + question.m_negative2Rate, 2));
        m_positiveBlockSize =
            std::max(m_positiveBlockSize,
                     next_interval(question.m_positive1Rate + question.m_positive2Rate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize, next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedFivePointSurveyQuestion& question)
        {
        m_fivePointCategorizedQuestions.push_back(question);
        for (const auto& category : question.m_fivePointCategories)
            {
            m_negativeBlockSize =
                std::max(m_negativeBlockSize,
                         next_interval(category.m_negative1Rate + category.m_negative2Rate, 2));
            m_positiveBlockSize =
                std::max(m_positiveBlockSize,
                         next_interval(category.m_positive1Rate + category.m_positive2Rate, 2));
            m_neutralBlockSize =
                std::max(m_neutralBlockSize, next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertSevenPointSurveyQuestion& question)
        {
        m_sevenPointQuestions.push_back(question);
        m_negativeBlockSize = std::max(
            m_negativeBlockSize,
            next_interval(
                question.m_negative1Rate + question.m_negative2Rate + question.m_negative3Rate, 2));
        m_positiveBlockSize = std::max(
            m_positiveBlockSize,
            next_interval(
                question.m_positive1Rate + question.m_positive2Rate + question.m_positive3Rate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize, next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedSevenPointSurveyQuestion& question)
        {
        m_sevenPointCategorizedQuestions.push_back(question);
        for (const auto& category : question.m_sevenPointCategories)
            {
            m_negativeBlockSize =
                std::max(m_negativeBlockSize,
                         next_interval(category.m_negative1Rate + category.m_negative2Rate +
                                           category.m_negative3Rate,
                                       2));
            m_positiveBlockSize =
                std::max(m_positiveBlockSize,
                         next_interval(category.m_positive1Rate + category.m_positive2Rate +
                                           category.m_positive3Rate,
                                       2));
            m_neutralBlockSize =
                std::max(m_neutralBlockSize, next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertThreePointSurveyQuestion& question)
        {
        Bar currentBar(
            GetBarSlotCount() + 1,
            { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushRight)))
                           .Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Brush(wxTransparentColor)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())),
              // space in front of negative block
              BarBlock(BarBlockInfo(m_negativeBlockSize - question.m_negativeRate).Show(false)),
              // negative block
              BarBlock(
                  BarBlockInfo(question.m_negativeRate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetNegativeColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negativeRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negativeRate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .
                          // if block and background are light, then don't auto-frame;
                          // this will make it so that an outlining won't obscure smaller blocks
                          LabelFitting((Colors::ColorContrast::IsLight(GetNegativeColor()) &&
                                        Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                           LabelFit::DisplayAsIs :
                                           LabelFit::DisplayAsIsAutoFrame)
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // positive block
              BarBlock(
                  BarBlockInfo(question.m_positiveRate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetPositiveColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positiveRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positiveRate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .LabelFitting(
                                  (Colors::ColorContrast::IsLight(GetPositiveColor()) &&
                                   Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                      LabelFit::DisplayAsIs :
                                      LabelFit::DisplayAsIsAutoFrame)
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // block after positive
              BarBlock(BarBlockInfo(m_positiveBlockSize - question.m_positiveRate).Show(false)),
              // neutral block
              BarBlock(
                  BarBlockInfo(question.m_neutralRate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetNeutralColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_neutralRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_neutralRate,
                                          0, wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNeutralColor()))
                              .LabelFitting(
                                  (Colors::ColorContrast::IsLight(GetNeutralColor()) &&
                                   Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                      LabelFit::DisplayAsIs :
                                      LabelFit::DisplayAsIsAutoFrame)
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize - question.m_neutralRate)
                           .Show(false)
                           .Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(
                  BarBlockInfo(question.m_naRate)
                      .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                          Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8))
                      .Brush(GetNoResponseColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_naRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_naRate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNoResponseColor()))
                              .LabelFitting(
                                  (Colors::ColorContrast::IsLight(GetNoResponseColor()) &&
                                   Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                      LabelFit::DisplayAsIs :
                                      LabelFit::DisplayAsIsAutoFrame)
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(L"NA_BLOCK")) },
            // empty info for the bar itself
            wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        // Scale bars down to 90% so that we have spaces between bars.
        // This is how you can include spaces between bars when custom bar widths are being used.
        if (IsSettingBarSizesToRespondentSize())
            {
            currentBar.SetCustomWidth(
                (safe_divide<double>(question.m_responses, m_maxRespondents)) * .9f);
            }
        SetBarBlockFullWidth(currentBar, GetQuestionBlockLabel());
        AddBar(currentBar);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(
        const LikertCategorizedThreePointSurveyQuestion& question)
        {
        for (const auto& group : question.m_threePointCategories)
            {
            // set the width of the categories column to fit the content
            if (group.m_question.length() > 10)
                {
                m_categoryBlockSize = m_questionBlockSize / 2;
                }
            else
                {
                m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize / 4);
                }
            }

        Bar questionBar(
            safe_divide<double>(question.m_threePointCategories.size(), 2) + GetBarSlotCount() +
                0.5,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .SelectionLabel(GraphItems::Label(
                               GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushLeft)))
                           .Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .Brush(wxTransparentColor)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())) },
            wxEmptyString, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_threePointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponse{ 0 };
        for (const auto& category : question.m_threePointCategories)
            {
            maxCategoryResponse = std::max(maxCategoryResponse, category.m_responses);
            }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (const auto& threePointCategory :
             std::ranges::reverse_view(question.m_threePointCategories))
            {
            Bar currentBar(
                GetBarSlotCount() + 1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(
                      BarBlockInfo(IsShowingResponseCounts() ?
                                       m_categoryBlockSize - m_responseCountBlockSize :
                                       m_categoryBlockSize)
                          .Brush(wxTransparentColor)
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(threePointCategory.m_question)
                                  .Font(GetBarAxis().GetFont())
                                  .LabelFitting(LabelFit::SplitTextToFit)
                                  .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(
                      BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                          .Brush(wxTransparentColor)
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  wxString::Format(L"(%s)",
                                                   wxNumberFormatter::ToString(
                                                       threePointCategory.m_responses, 0,
                                                       Settings::GetDefaultNumberFormat())))
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))
                          .Show(IsShowingResponseCounts())),
                  // space in front of negative block
                  BarBlock(BarBlockInfo(m_negativeBlockSize - (threePointCategory.m_negativeRate))
                               .Show(false)),
                  // negative block
                  BarBlock(
                      BarBlockInfo(threePointCategory.m_negativeRate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNegativeColor())
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   threePointCategory.m_negativeRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              threePointCategory.m_negativeRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .LabelFitting(
                                      (Colors::ColorContrast::IsLight(GetNegativeColor()) &&
                                       Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                          LabelFit::DisplayAsIs :
                                          LabelFit::DisplayAsIsAutoFrame)
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // positive block
                  BarBlock(
                      BarBlockInfo(threePointCategory.m_positiveRate)
                          .Brush(GetPositiveColor())
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   threePointCategory.m_positiveRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              threePointCategory.m_positiveRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .LabelFitting(
                                      (Colors::ColorContrast::IsLight(GetPositiveColor()) &&
                                       Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                          LabelFit::DisplayAsIs :
                                          LabelFit::DisplayAsIsAutoFrame)
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // block after positive
                  BarBlock(BarBlockInfo(m_positiveBlockSize - (threePointCategory.m_positiveRate))
                               .Show(false)),
                  // neutral block
                  BarBlock(
                      BarBlockInfo(threePointCategory.m_neutralRate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNeutralColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && threePointCategory.m_neutralRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              threePointCategory.m_neutralRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNeutralColor()))
                                  .LabelFitting(
                                      (Colors::ColorContrast::IsLight(GetNeutralColor()) &&
                                       Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                          LabelFit::DisplayAsIs :
                                          LabelFit::DisplayAsIsAutoFrame)
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize - threePointCategory.m_neutralRate)
                               .Show(false)
                               .Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(
                      BarBlockInfo(threePointCategory.m_naRate)
                          .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                              Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                              .8))
                          .Brush(GetNoResponseColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && threePointCategory.m_naRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              threePointCategory.m_naRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNoResponseColor()))
                                  .LabelFitting(
                                      (Colors::ColorContrast::IsLight(GetNoResponseColor()) &&
                                       Colors::ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                          LabelFit::DisplayAsIs :
                                          LabelFit::DisplayAsIsAutoFrame)
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(L"NA_BLOCK")) },
                // empty info for the bar itself
                wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                {
                currentBar.SetCustomWidth(
                    safe_divide<double>(threePointCategory.m_responses, maxCategoryResponse));
                }
            SetBarBlockFullWidth(currentBar, GetCategoryBlockLabel());
            AddBar(currentBar);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertFivePointSurveyQuestion& question)
        {
        Bar currentBar(
            GetBarSlotCount() + 1,
            { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .SelectionLabel(GraphItems::Label(
                               GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushLeft)))
                           .Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .Brush(wxTransparentColor)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())),
              // space in front of negative block
              BarBlock(BarBlockInfo(m_negativeBlockSize -
                                    (question.m_negative1Rate + question.m_negative2Rate))
                           .Show(false)),
              // strong negative block
              BarBlock(
                  BarBlockInfo(question.m_negative1Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negative1Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negative1Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // negative block
              BarBlock(
                  BarBlockInfo(question.m_negative2Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetNegativeColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negative2Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negative2Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // positive block
              BarBlock(
                  BarBlockInfo(question.m_positive1Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor()))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positive1Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positive1Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // strong positive block
              BarBlock(
                  BarBlockInfo(question.m_positive2Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetPositiveColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positive2Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positive2Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // block after positive
              BarBlock(BarBlockInfo(m_positiveBlockSize -
                                    (question.m_positive1Rate + question.m_positive2Rate))
                           .Show(false)),
              // neutral block
              BarBlock(
                  BarBlockInfo(question.m_neutralRate)
                      .Brush(GetNeutralColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_neutralRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_neutralRate,
                                          0, wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNeutralColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize - question.m_neutralRate)
                           .Show(false)
                           .Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(
                  BarBlockInfo(question.m_naRate)
                      .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                          Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8))
                      .Brush(GetNoResponseColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_naRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_naRate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNoResponseColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(L"NA_BLOCK")) },
            // empty info for the bar itself
            wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        if (IsSettingBarSizesToRespondentSize())
            {
            currentBar.SetCustomWidth(
                (safe_divide<double>(question.m_responses, m_maxRespondents)) * .9f);
            }
        SetBarBlockFullWidth(currentBar, GetQuestionBlockLabel());
        AddBar(currentBar);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertCategorizedFivePointSurveyQuestion& question)
        {
        for (const auto& group : question.m_fivePointCategories)
            {
            // set the width of the categories column to fit the content
            if (group.m_question.length() > 10)
                {
                m_categoryBlockSize = m_questionBlockSize / 2;
                }
            else
                {
                m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize / 4);
                }
            }

        Bar questionBar(
            safe_divide<double>(question.m_fivePointCategories.size(), 2) + GetBarSlotCount() + 0.5,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .SelectionLabel(GraphItems::Label(
                               GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushLeft)))
                           .Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .Brush(wxTransparentColor)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())) },
            wxEmptyString, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_fivePointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponse{ 0 };
        for (const auto& category : question.m_fivePointCategories)
            {
            maxCategoryResponse = std::max(maxCategoryResponse, category.m_responses);
            }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (const auto& fivePointCategory :
             std::ranges::reverse_view(question.m_fivePointCategories))
            {
            Bar currentBar(
                GetBarSlotCount() + 1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                            m_categoryBlockSize - m_responseCountBlockSize :
                                            m_categoryBlockSize)
                               .Brush(wxTransparentColor)
                               .SelectionLabel(GraphItems::Label(
                                   GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                               .OutlinePen(wxColour{ 0, 0, 0, 0 })
                               .Decal(GraphItems::Label(
                                   GraphItems::GraphItemInfo(fivePointCategory.m_question)
                                       .Font(GetBarAxis().GetFont())
                                       .LabelFitting(LabelFit::SplitTextToFit)
                                       .ChildAlignment(RelativeAlignment::FlushLeft)))
                               .Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(
                      BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                          .Brush(wxTransparentColor)
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  wxString::Format(L"(%s)",
                                                   wxNumberFormatter::ToString(
                                                       fivePointCategory.m_responses, 0,
                                                       Settings::GetDefaultNumberFormat())))
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))
                          .Show(IsShowingResponseCounts())),
                  // space in front of negative block
                  BarBlock(BarBlockInfo(m_negativeBlockSize - (fivePointCategory.m_negative1Rate +
                                                               fivePointCategory.m_negative2Rate))
                               .Show(false)),
                  // strong negative block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_negative1Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   fivePointCategory.m_negative1Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_negative1Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // negative block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_negative2Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNegativeColor())
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   fivePointCategory.m_negative2Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_negative2Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // positive block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_positive1Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor()))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   fivePointCategory.m_positive1Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_positive1Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // strong positive block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_positive2Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetPositiveColor())
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   fivePointCategory.m_positive2Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_positive2Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // block after positive
                  BarBlock(BarBlockInfo(m_positiveBlockSize - (fivePointCategory.m_positive1Rate +
                                                               fivePointCategory.m_positive2Rate))
                               .Show(false)),
                  // neutral block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_neutralRate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNeutralColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && fivePointCategory.m_neutralRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_neutralRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNeutralColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize - fivePointCategory.m_neutralRate)
                               .Show(false)
                               .Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(
                      BarBlockInfo(fivePointCategory.m_naRate)
                          .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                              Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                              .8))
                          .Brush(GetNoResponseColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && fivePointCategory.m_naRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              fivePointCategory.m_naRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNoResponseColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(L"NA_BLOCK")) },
                // empty info for the bar itself
                wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                {
                currentBar.SetCustomWidth(
                    safe_divide<double>(fivePointCategory.m_responses, maxCategoryResponse));
                }
            SetBarBlockFullWidth(currentBar, GetCategoryBlockLabel());
            AddBar(currentBar);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertSevenPointSurveyQuestion& question)
        {
        Bar currentBar(
            GetBarSlotCount() + 1,
            { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .SelectionLabel(GraphItems::Label(
                               GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushLeft)))
                           .Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .Brush(wxTransparentColor)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())),
              // space in front of negative block
              BarBlock(BarBlockInfo(m_negativeBlockSize -
                                    (question.m_negative1Rate + question.m_negative2Rate +
                                     question.m_negative3Rate))
                           .Show(false)),
              // strong negative block
              BarBlock(
                  BarBlockInfo(question.m_negative1Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor(), .40))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negative1Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negative1Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // negative block
              BarBlock(
                  BarBlockInfo(question.m_negative2Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negative2Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negative2Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // somewhat negative block
              BarBlock(
                  BarBlockInfo(question.m_negative3Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetNegativeColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_negative3Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_negative3Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNegativeColor()))
                              .ChildAlignment(RelativeAlignment::FlushRight)))),
              // somewhat positive block
              BarBlock(
                  BarBlockInfo(question.m_positive1Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetPositiveColor())
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positive1Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positive1Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // positive block
              BarBlock(
                  BarBlockInfo(question.m_positive2Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor()))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positive2Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positive2Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // strong positive block
              BarBlock(
                  BarBlockInfo(question.m_positive3Rate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor(), .40))
                      .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                          Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_positive3Rate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_positive3Rate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetPositiveColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))),
              // block after positive
              BarBlock(BarBlockInfo(m_positiveBlockSize -
                                    (question.m_positive1Rate + question.m_positive2Rate +
                                     question.m_positive3Rate))
                           .Show(false)),
              // neutral block
              BarBlock(
                  BarBlockInfo(question.m_neutralRate)
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Brush(GetNeutralColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_neutralRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_neutralRate,
                                          0, wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNeutralColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize - question.m_neutralRate)
                           .Show(false)
                           .Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(
                  BarBlockInfo(question.m_naRate)
                      .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                          Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8))
                      .Brush(GetNoResponseColor())
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              (IsShowingPercentages() && question.m_naRate > 0) ?
                                  wxString::Format(
                                      // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                      _("%s%%"),
                                      wxNumberFormatter::ToString(
                                          question.m_naRate, 0,
                                          wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                  wxString{})
                              .Font(GetBarAxis().GetFont())
                              .FontColor(
                                  Colors::ColorContrast::BlackOrWhiteContrast(GetNoResponseColor()))
                              .ChildAlignment(RelativeAlignment::FlushLeft)))
                      .Tag(L"NA_BLOCK")) },
            // empty info for the bar itself
            wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        if (IsSettingBarSizesToRespondentSize())
            {
            currentBar.SetCustomWidth(
                (safe_divide<double>(question.m_responses, m_maxRespondents)) * .9f);
            }
        SetBarBlockFullWidth(currentBar, GetQuestionBlockLabel());
        AddBar(currentBar);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(
        const LikertCategorizedSevenPointSurveyQuestion& question)
        {
        for (const auto& group : question.m_sevenPointCategories)
            {
            // set the width of the categories column to fit the content
            if (group.m_question.length() > 10)
                {
                m_categoryBlockSize = m_questionBlockSize / 2;
                }
            else
                {
                m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize / 4);
                }
            }

        Bar questionBar(
            safe_divide<double>(question.m_sevenPointCategories.size(), 2) + GetBarSlotCount() +
                0.5,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                                        m_questionBlockSize - m_responseCountBlockSize :
                                        m_questionBlockSize)
                           .Brush(wxTransparentColor)
                           .SelectionLabel(GraphItems::Label(
                               GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                           .OutlinePen(wxColour{ 0, 0, 0, 0 })
                           .Decal(GraphItems::Label(
                               GraphItems::GraphItemInfo(question.m_question)
                                   .Font(GetBarAxis().GetFont())
                                   .LabelFitting(LabelFit::SplitTextToFit)
                                   .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                   .ChildAlignment(RelativeAlignment::FlushLeft)))
                           .Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(
                  BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                      .Brush(wxTransparentColor)
                      .SelectionLabel(GraphItems::Label(
                          GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                      .OutlinePen(wxColour{ 0, 0, 0, 0 })
                      .Decal(GraphItems::Label(
                          GraphItems::GraphItemInfo(
                              wxString::Format(L"(%s)", wxNumberFormatter::ToString(
                                                            question.m_responses, 0,
                                                            Settings::GetDefaultNumberFormat())))
                              .Font(GetBarAxis().GetFont())
                              .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                              .ChildAlignment(RelativeAlignment::FlushRight)))
                      .Show(IsShowingResponseCounts())) },
            wxEmptyString, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_sevenPointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponse{ 0 };
        for (const auto& category : question.m_sevenPointCategories)
            {
            maxCategoryResponse = std::max(maxCategoryResponse, category.m_responses);
            }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (const auto& sevenPointCategory :
             std::ranges::reverse_view(question.m_sevenPointCategories))
            {
            Bar currentBar(
                GetBarSlotCount() + 1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(
                      BarBlockInfo(IsShowingResponseCounts() ?
                                       m_categoryBlockSize - m_responseCountBlockSize :
                                       m_categoryBlockSize)
                          .Brush(wxTransparentColor)
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(sevenPointCategory.m_question)
                                  .Font(GetBarAxis().GetFont())
                                  .LabelFitting(LabelFit::SplitTextToFit)
                                  .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(
                      BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0)
                          .Brush(wxTransparentColor)
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  wxString::Format(L"(%s)",
                                                   wxNumberFormatter::ToString(
                                                       sevenPointCategory.m_responses, 0,
                                                       Settings::GetDefaultNumberFormat())))
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))
                          .Show(IsShowingResponseCounts())),
                  // space in front of negative block
                  BarBlock(BarBlockInfo(m_negativeBlockSize - (sevenPointCategory.m_negative1Rate +
                                                               sevenPointCategory.m_negative2Rate +
                                                               sevenPointCategory.m_negative3Rate))
                               .Show(false)),
                  // strong negative block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_negative1Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor(), .40))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_negative1Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_negative1Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // negative block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_negative2Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetNegativeColor()))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_negative2Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_negative2Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // somewhat negative block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_negative3Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNegativeColor())
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_negative3Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_negative3Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNegativeColor()))
                                  .ChildAlignment(RelativeAlignment::FlushRight)))),
                  // somewhat positive block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_positive1Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetPositiveColor())
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_positive1Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_positive1Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // positive block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_positive2Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor()))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_positive2Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_positive2Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // strong positive block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_positive3Rate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(Colors::ColorContrast::ShadeOrTint(GetPositiveColor(), 0.40))
                          .SelectionLabel(GraphItems::Label(GraphItems::GraphItemInfo().Pen(
                              Colors::ColorBrewer::GetColor(Colors::Color::Black))))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() &&
                                   sevenPointCategory.m_positive3Rate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_positive3Rate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetPositiveColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))),
                  // block after positive
                  BarBlock(BarBlockInfo(m_positiveBlockSize - (sevenPointCategory.m_positive1Rate +
                                                               sevenPointCategory.m_positive2Rate +
                                                               sevenPointCategory.m_positive3Rate))
                               .Show(false)),
                  // neutral block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_neutralRate)
                          .OutlinePen(wxColour{ 0, 0, 0, 0 })
                          .Brush(GetNeutralColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && sevenPointCategory.m_neutralRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_neutralRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNeutralColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize - sevenPointCategory.m_neutralRate)
                               .Show(false)
                               .Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(
                      BarBlockInfo(sevenPointCategory.m_naRate)
                          .OutlinePen(Colors::ColorContrast::ShadeOrTint(
                              Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                              0.8))
                          .Brush(GetNoResponseColor())
                          .SelectionLabel(GraphItems::Label(
                              GraphItems::GraphItemInfo().Pen(wxColour{ 0, 0, 0, 0 })))
                          .Decal(GraphItems::Label(
                              GraphItems::GraphItemInfo(
                                  (IsShowingPercentages() && sevenPointCategory.m_naRate > 0) ?
                                      wxString::Format(
                                          // TRANSLATORS: formatted number ("%s") and percent ("%%")
                                          _("%s%%"),
                                          wxNumberFormatter::ToString(
                                              sevenPointCategory.m_naRate, 0,
                                              wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                                      wxString{})
                                  .Font(GetBarAxis().GetFont())
                                  .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                      GetNoResponseColor()))
                                  .ChildAlignment(RelativeAlignment::FlushLeft)))
                          .Tag(L"NA_BLOCK")) },
                // empty info for the bar itself
                wxString{}, GraphItems::Label{}, GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                {
                currentBar.SetCustomWidth(
                    safe_divide<double>(sevenPointCategory.m_responses, maxCategoryResponse));
                }
            SetBarBlockFullWidth(currentBar, GetCategoryBlockLabel());
            AddBar(currentBar);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddQuestionBrackets()
        {
        GetLeftYAxis().ClearBrackets();
        for (const auto& bracket : m_questionBrackets)
            {
            const auto firstBar = std::ranges::find_if(
                std::as_const(GetBars()),
                [&](const auto& bar)
                {
                    return (bar.GetBlocks().size() &&
                            bar.GetBlocks().at(0).GetDecal().GetText().CmpNoCase(
                                bracket.m_question1) == 0);
                });
            const auto secondBar = std::ranges::find_if(
                std::as_const(GetBars()),
                [&](const auto& bar)
                {
                    return (bar.GetBlocks().size() &&
                            bar.GetBlocks().at(0).GetDecal().GetText().CmpNoCase(
                                bracket.m_question2) == 0);
                });

            if (firstBar != GetBars().cend() && secondBar != GetBars().cend())
                {
                GetLeftYAxis().AddBracket(GraphItems::Axis::AxisBracket(
                    firstBar->GetAxisPosition(), secondBar->GetAxisPosition(),
                    safe_divide(firstBar->GetAxisPosition() + secondBar->GetAxisPosition(), 2.0),
                    bracket.m_title));
                }
            }
        }

    //----------------------------------------------------------------
    void LikertChart::RecalcSizes(wxDC & dc)
        {
        m_responseBarCount = 0;

        ClearBars();
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetLeftYAxis().GetAxisLinePen().IsOk())
            {
            GetBarAxis().GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            }
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetScalingAxis().GetAxisLinePen().IsOk())
            {
            GetScalingAxis().GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            }
        // load the questions and responses
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint)
            {
            for (const auto& question : m_threePointQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            for (const auto& question : m_threePointCategorizedQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePoint)
            {
            for (const auto& question : m_fivePointQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            for (const auto& question : m_fivePointCategorizedQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint)
            {
            for (const auto& question : m_sevenPointQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            for (const auto& question : m_sevenPointCategorizedQuestions)
                {
                AddSurveyQuestionBar(question);
                }
            }

        GetScalingAxis().SetRange(0,
                                  m_questionBlockSize + m_categoryBlockSize + m_negativeBlockSize +
                                      m_positiveBlockSize +
                                      (m_neutralMaxSize > 0 ? m_neutralBlockSize : 0) +
                                      (m_naMaxSize > 0 ? m_naBlockSize : 0),
                                  0, 10, 1);
        GetBarAxis().SetRange(0.5, GetBarSlotCount() + .5, 1, 1, 1);

        // add dividers between the positive, negative, and neutral sections
        GetCustomAxes().clear();

        if (m_neutralMaxSize > 0)
            {
            GraphItems::Axis neutralDividerLine(AxisType::RightYAxis);
            neutralDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize +
                                                  m_negativeBlockSize + m_positiveBlockSize);
            neutralDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
            neutralDividerLine.GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            neutralDividerLine.GetAxisLinePen() = wxNullPen;
            if (IsShowingSectionHeaders())
                {
                neutralDividerLine.GetHeader().SetText(GetNeutralLabel() + L" " + GetRightArrow());
                neutralDividerLine.GetHeader().GetFont().MakeBold();
                neutralDividerLine.GetHeader().SetFontColor(
                    Colors::ColorContrast::Shade(GetNeutralColor()));
                neutralDividerLine.GetHeader().SetRelativeAlignment(RelativeAlignment::FlushLeft);
                neutralDividerLine.GetHeader().SetLeftPadding(5);
                neutralDividerLine.GetHeader().GetPen() = wxNullPen;
                }
            AddCustomAxis(neutralDividerLine);
            }
        else
            {
            // if no neutral values, then remove those blocks from the bars
            // so that "0%" labels don't appear
            for (auto& bar : GetBars())
                {
                for (;;)
                    {
                    const auto foundPos = std::ranges::find_if(
                        std::as_const(bar.GetBlocks()), [](const auto& block) noexcept
                        { return block.GetTag() == GetNeutralBlockLabel(); });
                    if (foundPos != bar.GetBlocks().cend())
                        {
                        bar.GetBlocks().erase(foundPos);
                        }
                    else
                        {
                        break;
                        }
                    }
                }
            }

        if (m_naMaxSize > 0)
            {
            GraphItems::Axis naDividerLine(AxisType::RightYAxis);
            naDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize +
                                             m_negativeBlockSize + m_positiveBlockSize +
                                             m_neutralBlockSize);
            naDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
            naDividerLine.GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            naDividerLine.GetAxisLinePen() = wxNullPen;
            if (IsShowingSectionHeaders())
                {
                naDividerLine.GetFooter().SetText(GetNoResponseHeader() + L" " + GetRightArrow());
                naDividerLine.GetFooter().GetFont().MakeBold();
                naDividerLine.GetFooter().SetRelativeAlignment(RelativeAlignment::FlushLeft);
                naDividerLine.GetFooter().SetLeftPadding(5);
                naDividerLine.GetFooter().GetPen() = wxNullPen;
                }
            AddCustomAxis(naDividerLine);
            }
        else
            {
            // if no NA values, then remove those blocks from the bars
            // so that "0%" labels don't appear
            for (auto& bar : GetBars())
                {
                for (;;)
                    {
                    auto foundPos = std::ranges::find_if(
                        std::as_const(bar.GetBlocks()), [](const auto& block) noexcept
                        { return block.GetTag() == wxString(L"NA_BLOCK"); });
                    if (foundPos != bar.GetBlocks().cend())
                        {
                        bar.GetBlocks().erase(foundPos);
                        }
                    else
                        {
                        break;
                        }
                    }
                }
            }

        GraphItems::Axis agreeDividerLine(AxisType::RightYAxis);
        agreeDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize +
                                            m_negativeBlockSize);
        agreeDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
        agreeDividerLine.GetAxisLinePen().SetColour(
            Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
        agreeDividerLine.GetAxisLinePen() = wxNullPen;
        if (IsShowingSectionHeaders())
            {
            agreeDividerLine.GetHeader().SetText(GetPositiveHeader() + L" " + GetRightArrow());
            agreeDividerLine.GetHeader().SetRelativeAlignment(RelativeAlignment::FlushLeft);
            agreeDividerLine.GetHeader().GetPen() = wxNullPen;
            agreeDividerLine.GetHeader().GetFont().MakeBold();
            agreeDividerLine.GetHeader().SetFontColor(
                Colors::ColorContrast::Shade(GetPositiveColor()));
            agreeDividerLine.GetHeader().SetLeftPadding(5);
            }
        AddCustomAxis(agreeDividerLine);

        GraphItems::Axis disagreeDividerLine(AxisType::LeftYAxis);
        disagreeDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize +
                                               m_negativeBlockSize);
        disagreeDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
        disagreeDividerLine.GetAxisLinePen().SetColour(
            Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
        disagreeDividerLine.GetAxisLinePen() = wxNullPen;
        if (IsShowingSectionHeaders())
            {
            disagreeDividerLine.GetHeader().SetText(GetLeftArrow() + L" " + GetNegativeHeader());
            disagreeDividerLine.GetHeader().SetRelativeAlignment(RelativeAlignment::FlushRight);
            disagreeDividerLine.GetHeader().GetPen() = wxNullPen;
            disagreeDividerLine.GetHeader().GetFont().MakeBold();
            disagreeDividerLine.GetHeader().SetFontColor(
                Colors::ColorContrast::Shade(GetNegativeColor()));
            disagreeDividerLine.GetHeader().SetRightPadding(5);
            }
        AddCustomAxis(disagreeDividerLine);

        GraphItems::Axis questionDividerBar(AxisType::LeftYAxis);
        questionDividerBar.SetCustomXPosition(m_questionBlockSize);
        questionDividerBar.SetCustomYPosition(GetBarAxis().GetRange().second);
        questionDividerBar.GetAxisLinePen().SetColour(
            Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
        AddCustomAxis(questionDividerBar);

        if (m_categoryBlockSize > 0)
            {
            GraphItems::Axis categoryDividerBar(AxisType::LeftYAxis);
            categoryDividerBar.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize);
            categoryDividerBar.SetCustomYPosition(GetBarAxis().GetRange().second);
            categoryDividerBar.GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            AddCustomAxis(categoryDividerBar);
            }

        // if showing categorized data, then only draw full horizontal divider lines
        // between main questions
        std::vector<size_t> fullDividerLines;
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            size_t accumulated{ 0 };
            for (const auto& question : m_threePointCategorizedQuestions)
                {
                accumulated += question.m_threePointCategories.size();
                fullDividerLines.push_back(accumulated - 1);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            size_t accumulated{ 0 };
            for (const auto& question : m_fivePointCategorizedQuestions)
                {
                accumulated += question.m_fivePointCategories.size();
                fullDividerLines.push_back(accumulated - 1);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            size_t accumulated{ 0 };
            for (const auto& question : m_sevenPointCategorizedQuestions)
                {
                accumulated += question.m_sevenPointCategories.size();
                fullDividerLines.push_back(accumulated - 1);
                }
            }

        // lines between the bars, to make the plot look like a grid
        for (size_t i = 0; i < GetBarSlotCount(); ++i)
            {
            GraphItems::Axis dividerHorizontalBar(AxisType::BottomXAxis);
            dividerHorizontalBar.SetCustomXPosition(GetScalingAxis().GetRange().second);
            dividerHorizontalBar.SetCustomYPosition(i + 1.5);
            if (!fullDividerLines.empty() &&
                std::ranges::find(std::as_const(fullDividerLines), i) == fullDividerLines.cend())
                {
                dividerHorizontalBar.SetOffsetFromParentAxis(m_questionBlockSize);
                }
            dividerHorizontalBar.GetAxisLinePen().SetColour(
                Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
            AddCustomAxis(dividerHorizontalBar);
            }

        BarChart::RecalcSizes(dc);

        // overlay dashed dividing lines between sections
        auto sectionDividerLines = std::make_unique<GraphItems::Lines>(
            wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black), 1, wxPENSTYLE_LONG_DASH),
            GetScaling());
        wxCoord bottomPosAndNegX{ 0 }, bottomNeutralX{ 0 }, bottomMAX{ 0 };
        if (GetBottomXAxis().GetPhysicalCoordinate(
                m_questionBlockSize + m_categoryBlockSize + m_negativeBlockSize, bottomPosAndNegX))
            {
            sectionDividerLines->AddLine(
                wxPoint(bottomPosAndNegX, GetLeftYAxis().GetBottomPoint().y),
                wxPoint(bottomPosAndNegX, GetLeftYAxis().GetTopPoint().y));
            }
        if (GetBottomXAxis().GetPhysicalCoordinate(m_questionBlockSize + m_categoryBlockSize +
                                                       m_negativeBlockSize + m_positiveBlockSize,
                                                   bottomNeutralX))
            {
            sectionDividerLines->AddLine(wxPoint(bottomNeutralX, GetLeftYAxis().GetBottomPoint().y),
                                         wxPoint(bottomNeutralX, GetLeftYAxis().GetTopPoint().y));
            }
        if (GetBottomXAxis().GetPhysicalCoordinate(m_questionBlockSize + m_categoryBlockSize +
                                                       m_negativeBlockSize + m_positiveBlockSize +
                                                       m_neutralBlockSize,
                                                   bottomMAX))
            {
            sectionDividerLines->AddLine(wxPoint(bottomMAX, GetLeftYAxis().GetBottomPoint().y),
                                         wxPoint(bottomMAX, GetLeftYAxis().GetTopPoint().y));
            }
        AddObject(std::move(sectionDividerLines));

        AddQuestionBrackets();
        // make a little smaller as these could be rather lengthy
        // and consume a lot of real estate
        for (auto& bracket : GetLeftYAxis().GetBrackets())
            {
            bracket.GetLabel().GetFont().MakeSmaller();
            }
        }
    } // namespace Wisteria::Graphs
