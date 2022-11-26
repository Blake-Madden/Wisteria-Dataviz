///////////////////////////////////////////////////////////////////////////////
// Name:        likertchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "likertchart.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Data;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::MakeFormatCategorized(
        const LikertSurveyQuestionFormat format) noexcept
        {
        return
            (format == LikertSurveyQuestionFormat::TwoPoint ||
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
        }

    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::MakeFormatUncategorized(
        const LikertSurveyQuestionFormat format) noexcept
        {
        return
            (format == LikertSurveyQuestionFormat::TwoPoint ||
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
        }

    bool LikertChart::IsCategorized(const LikertSurveyQuestionFormat format) noexcept
        {
        return
            (format == LikertSurveyQuestionFormat::TwoPointCategorized ||
             format == LikertSurveyQuestionFormat::ThreePointCategorized ||
             format == LikertSurveyQuestionFormat::FourPointCategorized ||
             format == LikertSurveyQuestionFormat::FivePointCategorized ||
             format == LikertSurveyQuestionFormat::SixPointCategorized ||
             format == LikertSurveyQuestionFormat::SevenPointCategorized);
        }

    //-----------------------------------
    void LikertChart::SetLabels(std::shared_ptr<Data::Dataset>& data,
                                const std::vector<wxString>& questionColumns,
                                const Data::ColumnWithStringTable::StringTableType& codes)
        {
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            categoricalColumn->GetStringTable() = codes;
            }
        }

    //-----------------------------------
    LikertChart::LikertSurveyQuestionFormat LikertChart::Simplify(
        std::shared_ptr<Data::Dataset>& data,
        const std::vector<wxString>& questionColumns,
        LikertSurveyQuestionFormat currentFormat)
        {
        // 7 -> 3
        if (currentFormat == LikertSurveyQuestionFormat::SevenPoint ||
            currentFormat == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            Collapse7PointsTo3(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::ThreePoint));
            return (IsCategorized(currentFormat) ?
                LikertSurveyQuestionFormat::ThreePointCategorized :
                LikertSurveyQuestionFormat::ThreePoint);
            }
        // 6 -> 2
        else if (currentFormat == LikertSurveyQuestionFormat::SixPoint ||
            currentFormat == LikertSurveyQuestionFormat::SixPointCategorized)
            {
            Collapse6PointsTo2(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::TwoPoint));
            return (IsCategorized(currentFormat) ?
                LikertSurveyQuestionFormat::TwoPointCategorized :
                LikertSurveyQuestionFormat::TwoPoint);
            }
        // 5 -> 3
        else if (currentFormat == LikertSurveyQuestionFormat::FivePoint ||
            currentFormat == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            Collapse5PointsTo3(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::ThreePoint));
            return (IsCategorized(currentFormat) ?
                LikertSurveyQuestionFormat::ThreePointCategorized :
                LikertSurveyQuestionFormat::ThreePoint);
            }
        // 4 -> 2
        else if (currentFormat == LikertSurveyQuestionFormat::FourPoint ||
            currentFormat == LikertSurveyQuestionFormat::FourPointCategorized)
            {
            Collapse4PointsTo2(data, questionColumns, CreateLabels(LikertSurveyQuestionFormat::TwoPoint));
            return (IsCategorized(currentFormat) ?
                LikertSurveyQuestionFormat::TwoPointCategorized :
                LikertSurveyQuestionFormat::TwoPoint);
            }
        // 3 and 2 are already as simple as they are going to get,
        // just ensure their labels are correct
        else if (currentFormat == LikertSurveyQuestionFormat::ThreePoint ||
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
    void LikertChart::Collapse4PointsTo2(std::shared_ptr<Data::Dataset>& data,
                                         const std::vector<wxString>& questionColumns,
                                         const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        wxASSERT_LEVEL_2_MSG(condensedCodes.size() == 3, L"String table should have 3 values!");
        wxASSERT_LEVEL_2_MSG(std::min_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 0,
                                              L"String table should start at zero!");
        wxASSERT_LEVEL_2_MSG(std::max_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 2,
                                              L"String table should end at 2!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            wxASSERT_LEVEL_2_MSG(*std::max_element(categoricalColumn->GetValues().cbegin(),
                categoricalColumn->GetValues().cend()) <= 4,
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
    void LikertChart::Collapse6PointsTo2(std::shared_ptr<Data::Dataset>& data,
                                         const std::vector<wxString>& questionColumns,
                                         const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        wxASSERT_LEVEL_2_MSG(condensedCodes.size() == 3, L"String table should have 3 values!");
        wxASSERT_LEVEL_2_MSG(std::min_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 0,
                                              L"String table should start at zero!");
        wxASSERT_LEVEL_2_MSG(std::max_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 2,
                                              L"String table should end at 2!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            wxASSERT_LEVEL_2_MSG(*std::max_element(categoricalColumn->GetValues().cbegin(),
                categoricalColumn->GetValues().cend()) <= 6,
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
    void LikertChart::Collapse5PointsTo3(std::shared_ptr<Data::Dataset>& data,
                                         const std::vector<wxString>& questionColumns,
                                         const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        wxASSERT_LEVEL_2_MSG(condensedCodes.size() == 4, L"String table should have 4 values!");
        wxASSERT_LEVEL_2_MSG(std::min_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 0,
                                              L"String table should start at zero!");
        wxASSERT_LEVEL_2_MSG(std::max_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 3,
                                              L"String table should end at 3!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            wxASSERT_LEVEL_2_MSG(*std::max_element(categoricalColumn->GetValues().cbegin(),
                categoricalColumn->GetValues().cend()) <= 5,
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
    void LikertChart::Collapse7PointsTo3(std::shared_ptr<Data::Dataset>& data,
                                         const std::vector<wxString>& questionColumns,
                                         const Data::ColumnWithStringTable::StringTableType& condensedCodes)
        {
        wxASSERT_LEVEL_2_MSG(condensedCodes.size() == 4, L"String table should have 4 values!");
        wxASSERT_LEVEL_2_MSG(std::min_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 0,
                                              L"String table should start at zero!");
        wxASSERT_LEVEL_2_MSG(std::max_element(condensedCodes.cbegin(),
                                              condensedCodes.cend())->first == 3,
                                              L"String table should end at 3!");
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().end())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            wxASSERT_LEVEL_2_MSG(*std::max_element(categoricalColumn->GetValues().cbegin(),
                categoricalColumn->GetValues().cend()) <= 7,
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
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Strongly disagree").wc_str() },
                     { 2, _(L"Disagree").wc_str() },
                     { 3, _(L"Somewhat disagree").wc_str() },
                     { 4, _(L"Neutral").wc_str() },
                     { 5, _(L"Somewhat agree").wc_str() },
                     { 6, _(L"Agree").wc_str() },
                     { 7, _(L"Strongly agree").wc_str() } };
        // 0-6
        case LikertSurveyQuestionFormat::SixPointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::SixPoint:
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Strongly disagree").wc_str() },
                     { 2, _(L"Disagree").wc_str() },
                     { 3, _(L"Somewhat disagree").wc_str() },
                     { 4, _(L"Somewhat agree").wc_str() },
                     { 5, _(L"Agree").wc_str() },
                     { 6, _(L"Strongly agree").wc_str() } };
        // 0-5
        case LikertSurveyQuestionFormat::FivePointCategorized:
            [[fallthrough]];
        case LikertSurveyQuestionFormat::FivePoint:
            return { { 0, _(L"No response").wc_str() },
                     { 1, _(L"Disagree").wc_str() },
                     { 2, _(L"Somewhat disagree").wc_str() },
                     { 3, _(L"Neutral").wc_str() },
                     { 4, _(L"Somewhat agree").wc_str() },
                     { 5, _(L"Agree").wc_str() } };
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
        const std::shared_ptr<Data::Dataset>& data,
        const std::vector<wxString>& questionColumns,
        std::optional<wxString> groupColumnName /*= std::nullopt*/)
        {
        Data::GroupIdType maxVal{ 0 };
        for (const auto& catColumnName : questionColumns)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(catColumnName);
            if (categoricalColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."),
                    catColumnName).ToUTF8());
                }
            if (categoricalColumn->GetValues().size())
                {
                maxVal = std::max(*std::max_element(
                    categoricalColumn->GetValues().cbegin(),
                    categoricalColumn->GetValues().cend()),
                    maxVal);
                if (maxVal > 7)
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"Invalid Likert response: %" PRIu64 "\nColumn: %s\nValues should not exceed 7."),
                        maxVal, categoricalColumn->GetName()).ToUTF8());
                    }
                }
            }

        const auto groupColumn = (groupColumnName ?
            data->GetCategoricalColumn(groupColumnName.value()) :
            data->GetCategoricalColumns().cend());
        std::set<Data::GroupIdType> uniqueGroups;
        if (groupColumn != data->GetCategoricalColumns().cend())
            {
            for (const auto& val : groupColumn->GetValues())
                { uniqueGroups.insert(val); }
            }

        const LikertSurveyQuestionFormat format =
            (maxVal == 7) ? LikertSurveyQuestionFormat::SevenPoint :
            (maxVal == 6) ? LikertSurveyQuestionFormat::SixPoint :
            (maxVal == 5) ? LikertSurveyQuestionFormat::FivePoint :
            (maxVal == 4) ? LikertSurveyQuestionFormat::FourPoint :
            (maxVal == 3) ? LikertSurveyQuestionFormat::ThreePoint :
            (maxVal == 2) ? LikertSurveyQuestionFormat::TwoPoint :
            LikertSurveyQuestionFormat::TwoPoint;

        return (uniqueGroups.size() > 1) ? MakeFormatCategorized(format) : format;
        }

    //-----------------------------------
    void LikertChart::UpdateCanvasForBars()
        {
        auto barCount = GetBars().size();
        // if using categorization, then ignore the parent question bars and
        // just look at the group label bars
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            { barCount -= m_threePointCategorizedQuestions.size(); }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            { barCount -= m_fivePointCategorizedQuestions.size(); }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            { barCount -= m_sevenPointCategorizedQuestions.size(); }
        if (barCount > GetBarsPerDefaultCanvasSize())
            {
            GetCanvas()->SetCanvasMinHeightDIPs(GetCanvas()->GetDefaultCanvasHeightDIPs() *
                std::ceil(safe_divide<double>(barCount, GetBarsPerDefaultCanvasSize())));
            }
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> LikertChart::CreateLegend(const LegendOptions& options)
        {
        auto legend = std::make_shared<GraphItems::Label>(GraphItemInfo().
            Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs()).
            LabelAlignment(TextAlignment::FlushLeft).
            DPIScaling(GetDPIScaleFactor()));

        // fill in the legend
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            // don't add neutral to legend if there aren't any neutral responses
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNeutralColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor())
                    };
                legend->SetText(
                    GetPositiveLabel(1) + L"\n" +
                    GetNeutralLabel() + L"\n" +
                    GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor())
                    };
                legend->SetText(
                    GetPositiveLabel(1) + L"\n" +
                    GetNegativeLabel(1));
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNeutralColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor()))
                    };
                legend->SetText(
                    GetPositiveLabel(2) + L"\n" +
                    GetPositiveLabel(1) + L"\n" +
                    GetNeutralLabel() + L"\n" +
                    GetNegativeLabel(2) + L"\n" +
                    GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor()))
                    };
                legend->SetText(
                    GetPositiveLabel(2) + L"\n" +
                    GetPositiveLabel(1) + L"\n" +
                    GetNegativeLabel(2) + L"\n" +
                    GetNegativeLabel(1));
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint ||
                 GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            if (m_neutralMaxSize > 0)
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor(), .40f)),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNeutralColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor(), .40f))
                    };
                legend->SetText(
                    GetPositiveLabel(3) + L"\n" +
                    GetPositiveLabel(2) + L"\n" +
                    GetPositiveLabel(1) + L"\n" +
                    GetNeutralLabel() + L"\n" +
                    GetNegativeLabel(3) + L"\n" +
                    GetNegativeLabel(2) + L"\n" +
                    GetNegativeLabel(1));
                }
            else
                {
                legend->GetLegendIcons() =
                    {
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor(), .40f)),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetPositiveColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, GetPositiveColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, GetNegativeColor()),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor())),
                    LegendIcon(IconShape::Square, *wxBLACK, ColorContrast::ShadeOrTint(GetNegativeColor(), .40f))
                    };
                legend->SetText(
                    GetPositiveLabel(3) + L"\n" +
                    GetPositiveLabel(2) + L"\n" +
                    GetPositiveLabel(1) + L"\n" +
                    GetNegativeLabel(3) + L"\n" +
                    GetNegativeLabel(2) + L"\n" +
                    GetNegativeLabel(1));
                }
            }

        AddReferenceLinesAndAreasToLegend(legend);
        AdjustLegendSettings(legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    LikertChart::LikertChart(Canvas* canvas, const LikertSurveyQuestionFormat type,
        std::optional<wxColour> negativeColor /*= std::nullopt*/,
        std::optional<wxColour> positiveColor /*= std::nullopt*/,
        std::optional<wxColour> neutralColor /*= std::nullopt*/,
        std::optional<wxColour> noResponseColor /*= std::nullopt*/) :
        BarChart(canvas), m_surveyType(type)
        {
        SetNegativeColor(negativeColor.value_or(wxNullColour));
        SetPositiveColor(positiveColor.value_or(wxNullColour));
        SetNeutralColor(neutralColor.value_or(wxNullColour));
        SetNoResponseColor(noResponseColor.value_or(wxNullColour));

        SetBarOrientation(Orientation::Horizontal);
        GetScalingAxis().GetGridlinePen() = wxNullPen;
        GetBarAxis().GetGridlinePen() = wxNullPen;

        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetLeftYAxis().GetAxisLinePen().IsOk())
            { GetBarAxis().GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey)); }
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetScalingAxis().GetAxisLinePen().IsOk())
            { GetScalingAxis().GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey)); }
        MirrorXAxis(true);
        MirrorYAxis(true);

        IncludeSpacesBetweenBars(true);

        // make the plot taller if it contains a certain number of questions (or groups)
        SetBarsPerDefaultCanvasSize(20);
        }

    //----------------------------------------------------------------
    void LikertChart::SetData(std::shared_ptr<const Data::Dataset> data,
        const std::vector<wxString>& questionColumns,
        const std::optional<wxString> groupColumnName /*= std::nullopt*/)
        {
        if (data == nullptr)
            { return; }

        GetSelectedIds().clear();
        m_positive1Label.clear();
        m_positive2Label.clear();
        m_positive3Label.clear();
        m_negative1Label.clear();
        m_negative2Label.clear();
        m_negative3Label.clear();
        // if no string tables in the data, then this will be used for the section header
        m_neutralLabel = _(L"Neutral");

        const auto groupColumn = (groupColumnName ?
            data->GetCategoricalColumn(groupColumnName.value()) :
            data->GetCategoricalColumns().cend());
        if (groupColumnName && groupColumn == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for Likert chart."),
                groupColumnName.value()).ToUTF8());
            }
        // if a grouping column is used, then make the format categorized
        if (groupColumnName)
            { m_surveyType = MakeFormatCategorized(m_surveyType); }
        else
            { m_surveyType = MakeFormatUncategorized(m_surveyType); }

        // go in reverse order so that the first to last questions go from top-to-bottom
        for (auto questionIter = questionColumns.crbegin();
             questionIter != questionColumns.crend();
             ++questionIter)
            {
            const auto categoricalColumn = data->GetCategoricalColumn(*questionIter);
            if (categoricalColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': categorical column not found for Likert chart."), *questionIter).ToUTF8());
                }
            if (IsCategorized() && groupColumnName.has_value())
                {
                AddSurveyQuestion(*questionIter, *groupColumn, *categoricalColumn);
                }
            else
                {
                AddSurveyQuestion(*questionIter, *categoricalColumn);
                }

            // set the level labels from the data's string table,
            // these will be used for the legend
            if (categoricalColumn->GetStringTable().size())
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
            { return; }
        wxASSERT_LEVEL_2_MSG(IsCategorized(),
            L"Categorized data being passed into non-categorized Likert chart!");

        m_maxResondants = std::max(m_maxResondants, responses.GetRowCount());

        // the group IDs and their frequencies in the data
        std::map<GroupIdType, frequency_set<double>> fMap;
        for (size_t i = 0; i < groups.GetRowCount(); ++i)
            {
            auto foundPos = fMap.find(groups.GetValue(i));
            if (foundPos == fMap.end())
                {
                auto [iterator, found] = fMap.try_emplace(groups.GetValue(i), frequency_set<double>());
                iterator->second.insert(responses.GetValue(i));
                }
            else
                { foundPos->second.insert(responses.GetValue(i)); }
            }

        const auto findCount = [&fMap](const GroupIdType group, const double code) -> size_t
            {
            const auto groupPos = fMap.find(group);
            if (groupPos != fMap.cend())
                {
                const auto found = groupPos->second.get_data().find(code);
                return (found != groupPos->second.get_data().cend()) ? found->second : 0;
                }
            else
                { return 0; }
            };

        const auto findNACount = [&fMap](const GroupIdType group) -> size_t
            {
            const auto groupPos = fMap.find(group);
            if (groupPos != fMap.cend())
                {
                size_t naCount{ 0 };
                auto found = groupPos->second.get_data().find(0);
                if (found != groupPos->second.get_data().cend())
                    { naCount += found->second; }
                return naCount;
                }
            else
                { return 0; }
            };

        size_t groupResponses{ 0 };
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized)
            {
            LikertCategorizedThreePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertThreePointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), 0/* no neutrals*/, findCount(group.first, 2),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            LikertCategorizedThreePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertThreePointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), findCount(group.first, 2), findCount(group.first, 3),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized)
            {
            LikertCategorizedFivePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertFivePointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), findCount(group.first, 2), 0/* no neutrals*/,
                    findCount(group.first, 3), findCount(group.first, 4),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            LikertCategorizedFivePointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertFivePointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), findCount(group.first, 2), findCount(group.first, 3),
                    findCount(group.first, 4), findCount(group.first, 5),
                    findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized)
            {
            LikertCategorizedSevenPointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertSevenPointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), findCount(group.first, 2), findCount(group.first, 3),
                    0/* no neutrals*/, findCount(group.first, 4), findCount(group.first, 5),
                    findCount(group.first, 6), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            LikertCategorizedSevenPointSurveyQuestion sQuestion(question);
            for (const auto& group : fMap)
                {
                const LikertSevenPointSurveyQuestion surveyQuestion(groups.GetLabelFromID(group.first),
                    findCount(group.first, 1), findCount(group.first, 2), findCount(group.first, 3),
                    findCount(group.first, 4), findCount(group.first, 5), findCount(group.first, 6),
                    findCount(group.first, 7), findNACount(group.first));
                groupResponses += surveyQuestion.m_responses;
                sQuestion.AddCategoricalResponse(surveyQuestion);
                }
            wxASSERT_LEVEL_2_MSG(groupResponses == responses.GetRowCount(),
                L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(sQuestion);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const wxString& question,
                                        const Data::ColumnWithStringTable& responses)
        {
        if (responses.GetRowCount() == 0)
            { return; }
        wxASSERT_LEVEL_2_MSG(!IsCategorized(),
            L"Non-categorized data being loaded into categorized Likert chart!");

        m_maxResondants = std::max(m_maxResondants, responses.GetRowCount());

        frequency_set<double> fSet;
        for (const auto& value : responses.GetValues())
            { fSet.insert(value); }

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
                { naCount += found->second; }
            return naCount;
            };

        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint)
            {
            const LikertThreePointSurveyQuestion surveyQuestion(question,
                findCount(1), 0/* no neutrals*/, findCount(2), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint)
            {
            const LikertThreePointSurveyQuestion surveyQuestion(question,
                // 1-3, negative to positive
                findCount(1), findCount(2), findCount(3), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint)
            {
            const LikertFivePointSurveyQuestion surveyQuestion(question,
                findCount(1), findCount(2),
                0/* no neutrals*/, findCount(3),
                findCount(4), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FivePoint)
            {
            const LikertFivePointSurveyQuestion surveyQuestion(question,
                findCount(1), findCount(2),
                findCount(3), findCount(4),
                findCount(5), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint)
            {
            const LikertSevenPointSurveyQuestion surveyQuestion(question,
                findCount(1), findCount(2), findCount(3),
                0/* no neutrals*/, findCount(4), findCount(5),
                findCount(6), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint)
            {
            const LikertSevenPointSurveyQuestion surveyQuestion(question,
                findCount(1), findCount(2), findCount(3),
                findCount(4), findCount(5), findCount(6),
                findCount(7), findNACount());
            wxASSERT_LEVEL_2_MSG(surveyQuestion.m_responses == responses.GetRowCount(),
                                 L"Classified responses don't equal the overall responses count!");
            AddSurveyQuestion(surveyQuestion);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertThreePointSurveyQuestion& question)
        {
        m_threePointQuestions.emplace_back(question);
        m_negativeBlockSize = std::max(m_negativeBlockSize,
            next_interval(question.m_negativeRate, 2));
        m_positiveBlockSize = std::max(m_positiveBlockSize,
            next_interval(question.m_positiveRate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize,
            next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedThreePointSurveyQuestion& question)
        {
        m_threePointCategorizedQuestions.emplace_back(question);
        for (const auto& category : question.m_threePointCategories)
            {
            m_negativeBlockSize = std::max(m_negativeBlockSize,
                next_interval(category.m_negativeRate, 2));
            m_positiveBlockSize = std::max(m_positiveBlockSize,
                next_interval(category.m_positiveRate, 2));
            m_neutralBlockSize = std::max(m_neutralBlockSize,
                next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertFivePointSurveyQuestion& question)
        {
        m_fivePointQuestions.emplace_back(question);
        m_negativeBlockSize = std::max(m_negativeBlockSize,
            next_interval(question.m_negative1Rate+question.m_negative2Rate, 2));
        m_positiveBlockSize = std::max(m_positiveBlockSize,
            next_interval(question.m_positive1Rate+question.m_positive2Rate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize,
            next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedFivePointSurveyQuestion& question)
        {
        m_fivePointCategorizedQuestions.emplace_back(question);
        for (const auto& category : question.m_fivePointCategories)
            {
            m_negativeBlockSize = std::max(m_negativeBlockSize,
                next_interval(category.m_negative1Rate + category.m_negative2Rate, 2));
            m_positiveBlockSize = std::max(m_positiveBlockSize,
                next_interval(category.m_positive1Rate + category.m_positive2Rate, 2));
            m_neutralBlockSize = std::max(m_neutralBlockSize,
                next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

     //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertSevenPointSurveyQuestion& question)
        {
        m_sevenPointQuestions.emplace_back(question);
        m_negativeBlockSize = std::max(m_negativeBlockSize,
            next_interval(question.m_negative1Rate+question.m_negative2Rate+question.m_negative3Rate, 2));
        m_positiveBlockSize = std::max(m_positiveBlockSize,
            next_interval(question.m_positive1Rate+question.m_positive2Rate+question.m_positive3Rate, 2));
        m_neutralBlockSize = std::max(m_neutralBlockSize,
            next_interval(question.m_neutralRate, 2));
        m_naBlockSize = std::max(m_naBlockSize, next_interval(question.m_naRate, 2));
        // to see if we even need a neutral or NA sections later
        m_neutralMaxSize = std::max(m_neutralMaxSize, question.m_neutralRate);
        m_naMaxSize = std::max(m_naMaxSize, question.m_naRate);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestion(const LikertCategorizedSevenPointSurveyQuestion& question)
        {
        m_sevenPointCategorizedQuestions.emplace_back(question);
        for (const auto& category : question.m_sevenPointCategories)
            {
            m_negativeBlockSize = std::max(m_negativeBlockSize,
                next_interval(category.m_negative1Rate+category.m_negative2Rate+category.m_negative3Rate, 2));
            m_positiveBlockSize = std::max(m_positiveBlockSize,
                next_interval(category.m_positive1Rate+category.m_positive2Rate+category.m_positive3Rate, 2));
            m_neutralBlockSize = std::max(m_neutralBlockSize,
                next_interval(category.m_neutralRate, 2));
            m_naBlockSize = std::max(m_naBlockSize, next_interval(category.m_naRate, 2));
            // to see if we even need a neutral or NA sections later
            m_neutralMaxSize = std::max(m_neutralMaxSize, category.m_neutralRate);
            m_naMaxSize = std::max(m_naMaxSize, category.m_naRate);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertThreePointSurveyQuestion& question)
        {
        Bar currentBar(GetBarSlotCount()+1,
            { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                    LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).
                    Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Brush(wxTransparentColor).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(question.m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                    Font(GetBarAxis().GetFont()).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts())),
              // space in front of negative block
              BarBlock(BarBlockInfo(m_negativeBlockSize-question.m_negativeRate).Show(false)),
              // negative block
              BarBlock(BarBlockInfo(question.m_negativeRate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetNegativeColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negativeRate > 0) ?
                      wxNumberFormatter::ToString(question.m_negativeRate, 0,
                          wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                      wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      // if block and background are light, then don't auto-frame;
                      // this will make it so that an outlining won't obscure smaller blocks
                      LabelFitting(
                        (ColorContrast::IsLight(GetNegativeColor()) &&
                         ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                            LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                      ChildAlignment(RelativeAlignment::FlushRight)))),
              // positive block
              BarBlock(BarBlockInfo(question.m_positiveRate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetPositiveColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positiveRate > 0) ?
                      wxNumberFormatter::ToString(question.m_positiveRate, 0,
                          wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                      wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      LabelFitting(
                        (ColorContrast::IsLight(GetPositiveColor()) &&
                         ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                            LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                      ChildAlignment(RelativeAlignment::FlushLeft)))),
              // block after positive
              BarBlock(BarBlockInfo(m_positiveBlockSize-question.m_positiveRate).Show(false)),
              // neutral block
              BarBlock(BarBlockInfo(question.m_neutralRate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetNeutralColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_neutralRate > 0) ?
                    wxNumberFormatter::ToString(question.m_neutralRate, 0,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                    wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                      LabelFitting(
                        (ColorContrast::IsLight(GetNeutralColor()) &&
                         ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                            LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                      ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize-question.m_neutralRate).
                       Show(false).Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(BarBlockInfo(question.m_naRate).
                OutlinePen(ColorContrast::ShadeOrTint(
                           ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8)).
                Brush(GetNoResponseColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_naRate > 0) ?
                      wxNumberFormatter::ToString(question.m_naRate, 0,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                      wxString()).
                Font(GetBarAxis().GetFont()).
                FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                LabelFitting(
                (ColorContrast::IsLight(GetNoResponseColor()) &&
                    ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                    LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
               // empty info for the bar itself
               L"", Label(), GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        // Scale bars down to 90% so that we have spaces between bars.
        // This is how you can include spaces between bars when custom bar widths are being used.
        if (IsSettingBarSizesToRespondentSize())
            { currentBar.SetCustomWidth((safe_divide<double>(question.m_responses, m_maxResondants))*.9f); }
        SetBarBlockFullWidth(currentBar, GetQuestionBlockLabel());
        AddBar(currentBar);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertCategorizedThreePointSurveyQuestion& question)
        {
        for (const auto& group : question.m_threePointCategories)
            {
            // set the width of the categories column to fit the content
            if (group.m_question.length() > 10)
                { m_categoryBlockSize = m_questionBlockSize/2; }
            else
                { m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize/4); }
            }

        Bar questionBar(safe_divide<double>(question.m_threePointCategories.size(),2) + GetBarSlotCount() + 0.5f,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(
                Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                    LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushLeft))).
                Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(
                Label(GraphItemInfo(wxString::Format(L"(%s)",
                    wxNumberFormatter::ToString(question.m_responses,0,
                        Settings::GetDefaultNumberFormat()))).
                    Font(GetBarAxis().GetFont()).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts())) },
            wxEmptyString, Label(), GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_threePointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponese{ 0 };
        for (const auto& category : question.m_threePointCategories)
            { maxCategoryResponese = std::max(maxCategoryResponese, category.m_responses); }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (auto category = question.m_threePointCategories.crbegin();
             category != question.m_threePointCategories.crend();
             ++category)
            {
            Bar currentBar(GetBarSlotCount()+1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                    m_categoryBlockSize-m_responseCountBlockSize : m_categoryBlockSize).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(
                    Label(GraphItemInfo(category->m_question).
                        Font(GetBarAxis().GetFont()).
                        LabelFitting(LabelFit::SplitTextToFit).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushLeft))).
                        Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(
                    Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(category->m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                        Font(GetBarAxis().GetFont()).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushRight))).
                        Show(IsShowingResponseCounts()) ),
                  // space in front of negative block
                  BarBlock(BarBlockInfo(m_negativeBlockSize-(category->m_negativeRate)).Show(false) ),
                  // negative block
                  BarBlock(BarBlockInfo(category->m_negativeRate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNegativeColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negativeRate > 0) ?
                            wxNumberFormatter::ToString(category->m_negativeRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          LabelFitting(
                            (ColorContrast::IsLight(GetNegativeColor()) &&
                             ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // positive block
                  BarBlock(BarBlockInfo(category->m_positiveRate).Brush(GetPositiveColor()).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positiveRate > 0) ?
                            wxNumberFormatter::ToString(category->m_positiveRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          LabelFitting(
                            (ColorContrast::IsLight(GetPositiveColor()) &&
                             ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // block after positive
                  BarBlock(BarBlockInfo(m_positiveBlockSize - (category->m_positiveRate)).Show(false)),
                  // neutral block
                  BarBlock(BarBlockInfo(category->m_neutralRate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNeutralColor()).
                    SelectionLabel(Label(GraphItemInfo().
                    Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_neutralRate > 0) ?
                            wxNumberFormatter::ToString(category->m_neutralRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                          LabelFitting(
                            (ColorContrast::IsLight(GetNeutralColor()) &&
                             ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                                LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                          ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize-category->m_neutralRate).
                      Show(false).Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(BarBlockInfo(category->m_naRate).
                    OutlinePen(ColorContrast::ShadeOrTint(
                        ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8)).
                    Brush(GetNoResponseColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_naRate > 0) ?
                            wxNumberFormatter::ToString(category->m_naRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                    Font(GetBarAxis().GetFont()).
                    FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                    LabelFitting(
                    (ColorContrast::IsLight(GetNoResponseColor()) &&
                        ColorContrast::IsLight(GetPlotOrCanvasColor())) ?
                        LabelFit::DisplayAsIs : LabelFit::DisplayAsIsAutoFrame).
                    ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
                   // empty info for the bar itself
                   L"", Label(), GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                {
                currentBar.SetCustomWidth(
                    safe_divide<double>(category->m_responses, maxCategoryResponese));
                }
            SetBarBlockFullWidth(currentBar, GetCategoryBlockLabel());
            AddBar(currentBar);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertFivePointSurveyQuestion& question)
        {
        Bar currentBar(GetBarSlotCount()+1,
            { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                        LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushLeft))).
                    Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                    wxNumberFormatter::ToString(question.m_responses,0,
                        Settings::GetDefaultNumberFormat()))).
                    Font(GetBarAxis().GetFont()).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts())),
              // space in front of negative block
              BarBlock(
                  BarBlockInfo(m_negativeBlockSize -
                               (question.m_negative1Rate + question.m_negative2Rate)).
                  Show(false)),
              // strong negative block
              BarBlock(BarBlockInfo(question.m_negative1Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetNegativeColor())).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negative1Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_negative1Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      ChildAlignment(RelativeAlignment::FlushRight))) ),
              // negative block
              BarBlock(BarBlockInfo(question.m_negative2Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetNegativeColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negative2Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_negative2Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      ChildAlignment(RelativeAlignment::FlushRight))) ),
              // positive block
              BarBlock(BarBlockInfo(question.m_positive1Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetPositiveColor())).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positive1Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_positive1Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))) ),
              // strong positive block
              BarBlock(BarBlockInfo(question.m_positive2Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetPositiveColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positive2Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_positive2Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))) ),
              // block after positive
              BarBlock(
                  BarBlockInfo(m_positiveBlockSize -
                               (question.m_positive1Rate + question.m_positive2Rate)).
                  Show(false)),
              // neutral block
              BarBlock(BarBlockInfo(question.m_neutralRate).
                Brush(GetNeutralColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_neutralRate > 0) ?
                        wxNumberFormatter::ToString(question.m_neutralRate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize-question.m_neutralRate).
                  Show(false).Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(BarBlockInfo(question.m_naRate).
                OutlinePen(ColorContrast::ShadeOrTint(
                    ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8)).
                Brush(GetNoResponseColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_naRate > 0) ?
                        wxNumberFormatter::ToString(question.m_naRate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                Font(GetBarAxis().GetFont()).
                FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
               // empty info for the bar itself
               L"", Label(), GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        if (IsSettingBarSizesToRespondentSize())
            { currentBar.SetCustomWidth((safe_divide<double>(question.m_responses, m_maxResondants))*.9f); }
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
                { m_categoryBlockSize = m_questionBlockSize/2; }
            else
                { m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize/4); }
            }

        Bar questionBar(safe_divide<double>(question.m_fivePointCategories.size(),2) + GetBarSlotCount() + 0.5f,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                    LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushLeft))).
                    Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(question.m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                    Font(GetBarAxis().GetFont()).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts())) },
            wxEmptyString, Label(), GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_fivePointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponese{ 0 };
        for (const auto& category : question.m_fivePointCategories)
            { maxCategoryResponese = std::max(maxCategoryResponese, category.m_responses); }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (auto category = question.m_fivePointCategories.crbegin();
             category != question.m_fivePointCategories.crend();
             ++category)
            {
            Bar currentBar(GetBarSlotCount()+1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                    m_categoryBlockSize-m_responseCountBlockSize : m_categoryBlockSize).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(Label(GraphItemInfo(category->m_question).
                        Font(GetBarAxis().GetFont()).
                        LabelFitting(LabelFit::SplitTextToFit).
                        ChildAlignment(RelativeAlignment::FlushLeft))).
                        Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(category->m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                        Font(GetBarAxis().GetFont()).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts()) ),
                  // space in front of negative block
                  BarBlock(
                      BarBlockInfo(m_negativeBlockSize -
                                   (category->m_negative1Rate + category->m_negative2Rate)).
                      Show(false)),
                  // strong negative block
                  BarBlock(BarBlockInfo(category->m_negative1Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetNegativeColor())).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negative1Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_negative1Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // negative block
                  BarBlock(BarBlockInfo(category->m_negative2Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNegativeColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negative2Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_negative2Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // positive block
                  BarBlock(BarBlockInfo(category->m_positive1Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetPositiveColor())).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positive1Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_positive1Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // strong positive block
                  BarBlock(BarBlockInfo(category->m_positive2Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetPositiveColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positive2Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_positive2Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // block after positive
                  BarBlock(
                      BarBlockInfo(m_positiveBlockSize -
                                   (category->m_positive1Rate+ category->m_positive2Rate)).
                      Show(false)),
                  // neutral block
                  BarBlock(BarBlockInfo(category->m_neutralRate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNeutralColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_neutralRate > 0) ?
                            wxNumberFormatter::ToString(category->m_neutralRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize - category->m_neutralRate).
                      Show(false).Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(BarBlockInfo(category->m_naRate).
                    OutlinePen(ColorContrast::ShadeOrTint(
                           ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8)).
                    Brush(GetNoResponseColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_naRate > 0) ?
                            wxNumberFormatter::ToString(category->m_naRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                    Font(GetBarAxis().GetFont()).
                    FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                    ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
                   // empty info for the bar itself
                   L"", Label(), GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                { currentBar.SetCustomWidth(safe_divide<double>(category->m_responses, maxCategoryResponese)); }
            SetBarBlockFullWidth(currentBar, GetCategoryBlockLabel());
            AddBar(currentBar);
            }
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertSevenPointSurveyQuestion& question)
        {
        Bar currentBar(GetBarSlotCount()+1,
              { // the question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                    LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushLeft))).
                    Tag(GetQuestionBlockLabel())),
              // response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(question.m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                        Font(GetBarAxis().GetFont()).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts()) ),
              // space in front of negative block
              BarBlock(
                  BarBlockInfo(m_negativeBlockSize -
                               (question.m_negative1Rate + question.m_negative2Rate +
                                question.m_negative3Rate)).
                  Show(false)),
              // strong negative block
              BarBlock(BarBlockInfo(question.m_negative1Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetNegativeColor(), .40f)).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negative1Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_negative1Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      ChildAlignment(RelativeAlignment::FlushRight))) ),
              // negative block
              BarBlock(BarBlockInfo(question.m_negative2Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetNegativeColor())).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negative2Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_negative2Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      ChildAlignment(RelativeAlignment::FlushRight))) ),
              // somewhat negative block
              BarBlock(BarBlockInfo(question.m_negative3Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetNegativeColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_negative3Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_negative3Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                      ChildAlignment(RelativeAlignment::FlushRight))) ),
              // somewhat positive block
              BarBlock(BarBlockInfo(question.m_positive1Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetPositiveColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positive1Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_positive1Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))) ),
              // positive block
              BarBlock(BarBlockInfo(question.m_positive2Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetPositiveColor())).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positive2Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_positive2Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))) ),
              // strong positive block
              BarBlock(BarBlockInfo(question.m_positive3Rate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(ColorContrast::ShadeOrTint(GetPositiveColor(), .40f)).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_positive3Rate > 0) ?
                        wxNumberFormatter::ToString(question.m_positive3Rate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))) ),
              // block after positive
              BarBlock(
                  BarBlockInfo(m_positiveBlockSize -
                               (question.m_positive1Rate + question.m_positive2Rate +
                                question.m_positive3Rate)).
                  Show(false)),
              // neutral block
              BarBlock(BarBlockInfo(question.m_neutralRate).
                OutlinePen(*wxTRANSPARENT_PEN).
                Brush(GetNeutralColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_neutralRate > 0) ?
                        wxNumberFormatter::ToString(question.m_neutralRate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                      Font(GetBarAxis().GetFont()).
                      FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                      ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
              // block after neutral
              BarBlock(BarBlockInfo(m_neutralBlockSize-question.m_neutralRate).
                  Show(false).Tag(GetNeutralBlockLabel())),
              // no response block
              BarBlock(BarBlockInfo(question.m_naRate).
                OutlinePen(ColorContrast::ShadeOrTint(
                      ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), .8)).
                Brush(GetNoResponseColor()).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                Decal(Label(GraphItemInfo((IsShowingPercentages() && question.m_naRate > 0) ?
                        wxNumberFormatter::ToString(question.m_naRate, 0,
                            wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                        wxString()).
                Font(GetBarAxis().GetFont()).
                FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
               // empty info for the bar itself
               L"", Label(), GetBarEffect(), GetBarOpacity());

        ++m_responseBarCount;
        if (IsSettingBarSizesToRespondentSize())
            { currentBar.SetCustomWidth((safe_divide<double>(question.m_responses, m_maxResondants))*.9f); }
        SetBarBlockFullWidth(currentBar, GetQuestionBlockLabel());
        AddBar(currentBar);
        }

    //----------------------------------------------------------------
    void LikertChart::AddSurveyQuestionBar(const LikertCategorizedSevenPointSurveyQuestion& question)
        {
        for (const auto& group : question.m_sevenPointCategories)
            {
            // set the width of the categories column to fit the content
            if (group.m_question.length() > 10)
                { m_categoryBlockSize = m_questionBlockSize/2; }
            else
                { m_categoryBlockSize = std::max(m_categoryBlockSize, m_questionBlockSize/4); }
            }

        Bar questionBar(
            safe_divide<double>(question.m_sevenPointCategories.size(),2) + GetBarSlotCount()+0.5f,
            { // main question
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                m_questionBlockSize-m_responseCountBlockSize : m_questionBlockSize).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(question.m_question).
                    Font(GetBarAxis().GetFont()).
                    LabelFitting(LabelFit::SplitTextToFit).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushLeft))).
                    Tag(GetQuestionBlockLabel())),
              // overall response count
              BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                Brush(wxTransparentColor).
                SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                OutlinePen(*wxTRANSPARENT_PEN).
                Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(question.m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                    Font(GetBarAxis().GetFont()).
                    FontColor(*wxBLACK).
                    ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts()) ) },
            wxEmptyString, Label(), GetBarEffect(), GetBarOpacity());
        // tall as all its categorical bars combined
        questionBar.SetCustomWidth(question.m_sevenPointCategories.size());
        AddBar(questionBar);

        size_t maxCategoryResponese{ 0 };
        for (const auto& category : question.m_sevenPointCategories)
            { maxCategoryResponese = std::max(maxCategoryResponese, category.m_responses); }

        // blocks are filled bottom-to-top, so go in reverse order so that the group sorting
        // appears top-to-bottom
        for (auto category = question.m_sevenPointCategories.crbegin();
             category != question.m_sevenPointCategories.crend();
             ++category)
            {
            Bar currentBar(GetBarSlotCount()+1,
                { // empty space for parent question
                  BarBlock(BarBlockInfo(m_questionBlockSize).Show(false)),
                  // the category
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ?
                    m_categoryBlockSize-m_responseCountBlockSize : m_categoryBlockSize).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(Label(GraphItemInfo(category->m_question).
                        Font(GetBarAxis().GetFont()).
                        LabelFitting(LabelFit::SplitTextToFit).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushLeft))).
                        Tag(GetCategoryBlockLabel())),
                  // response count (for current category)
                  BarBlock(BarBlockInfo(IsShowingResponseCounts() ? m_responseCountBlockSize : 0.0f).
                    Brush(wxTransparentColor).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Decal(Label(GraphItemInfo(wxString::Format(L"(%s)",
                        wxNumberFormatter::ToString(category->m_responses,0,
                            Settings::GetDefaultNumberFormat()))).
                        Font(GetBarAxis().GetFont()).
                        FontColor(*wxBLACK).
                        ChildAlignment(RelativeAlignment::FlushRight))).Show(IsShowingResponseCounts()) ),
                  // space in front of negative block
                  BarBlock(
                      BarBlockInfo(m_negativeBlockSize -
                                   (category->m_negative1Rate + category->m_negative2Rate +
                                    category->m_negative3Rate)).
                      Show(false)),
                  // strong negative block
                  BarBlock(BarBlockInfo(category->m_negative1Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetNegativeColor(), .40f)).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negative1Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_negative1Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // negative block
                  BarBlock(BarBlockInfo(category->m_negative2Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetNegativeColor())).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negative2Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_negative2Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // somewhat negative block
                  BarBlock(BarBlockInfo(category->m_negative3Rate).
                      OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNegativeColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_negative3Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_negative3Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNegativeColor())).
                          ChildAlignment(RelativeAlignment::FlushRight))) ),
                  // somewhat positive block
                  BarBlock(BarBlockInfo(category->m_positive1Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetPositiveColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positive1Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_positive1Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // positive block
                  BarBlock(BarBlockInfo(category->m_positive2Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetPositiveColor())).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positive2Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_positive2Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // strong positive block
                  BarBlock(BarBlockInfo(category->m_positive3Rate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(ColorContrast::ShadeOrTint(GetPositiveColor(), 0.40f)).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxBLACK_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_positive3Rate > 0) ?
                            wxNumberFormatter::ToString(category->m_positive3Rate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetPositiveColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))) ),
                  // block after positive
                  BarBlock(
                      BarBlockInfo(m_positiveBlockSize -
                                   (category->m_positive1Rate + category->m_positive2Rate +
                                    category->m_positive3Rate)).
                      Show(false)),
                  // neutral block
                  BarBlock(BarBlockInfo(category->m_neutralRate).
                    OutlinePen(*wxTRANSPARENT_PEN).
                    Brush(GetNeutralColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_neutralRate > 0) ?
                            wxNumberFormatter::ToString(category->m_neutralRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                          Font(GetBarAxis().GetFont()).
                          FontColor(ColorContrast::BlackOrWhiteContrast(GetNeutralColor())).
                          ChildAlignment(RelativeAlignment::FlushLeft))).Tag(GetNeutralBlockLabel())),
                  // block after neutral
                  BarBlock(BarBlockInfo(m_neutralBlockSize - category->m_neutralRate).
                      Show(false).Tag(GetNeutralBlockLabel())),
                  // no response block
                  BarBlock(BarBlockInfo(category->m_naRate).
                    OutlinePen(ColorContrast::ShadeOrTint(
                          ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()), 0.8)).
                    Brush(GetNoResponseColor()).
                    SelectionLabel(Label(GraphItemInfo().Pen(*wxTRANSPARENT_PEN))).
                    Decal(Label(GraphItemInfo((IsShowingPercentages() && category->m_naRate > 0) ?
                            wxNumberFormatter::ToString(category->m_naRate, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                            wxString()).
                    Font(GetBarAxis().GetFont()).
                    FontColor(ColorContrast::BlackOrWhiteContrast(GetNoResponseColor())).
                    ChildAlignment(RelativeAlignment::FlushLeft))).Tag(L"NA_BLOCK")) },
                   // empty info for the bar itself
                   L"", Label(), GetBarEffect(), GetBarOpacity());

            ++m_responseBarCount;
            if (IsSettingBarSizesToRespondentSize())
                { currentBar.SetCustomWidth(safe_divide<double>(category->m_responses, maxCategoryResponese)); }
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
            const auto firstBar = std::find_if(GetBars().cbegin(), GetBars().cend(),
                [&](const auto& bar)
                {
                return (bar.GetBlocks().size() &&
                    bar.GetBlocks().at(0).GetDecal().GetText().CmpNoCase(bracket.m_question1) == 0);
                });
            const auto secondBar = std::find_if(GetBars().cbegin(), GetBars().cend(),
                [&](const auto& bar)
                {
                return (bar.GetBlocks().size() &&
                    bar.GetBlocks().at(0).GetDecal().GetText().CmpNoCase(bracket.m_question2) == 0);
                });

            if (firstBar != GetBars().cend() &&
                secondBar != GetBars().cend())
                {
                GetLeftYAxis().AddBracket(Axis::AxisBracket(
                    firstBar->GetAxisPosition(), secondBar->GetAxisPosition(),
                    safe_divide(firstBar->GetAxisPosition() + secondBar->GetAxisPosition(), 2.0),
                    bracket.m_title));
                }
            }
        }

    //----------------------------------------------------------------
    void LikertChart::RecalcSizes(wxDC& dc)
        {
        ClearBars();
        m_responseBarCount = 0;
        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetLeftYAxis().GetAxisLinePen().IsOk())
            { GetBarAxis().GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey)); }
        GetScalingAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        if (GetScalingAxis().GetAxisLinePen().IsOk())
            { GetScalingAxis().GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey)); }
        // load the questions and responses
        if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint)
            {
            for (const auto& question : m_threePointQuestions)
                { AddSurveyQuestionBar(question); }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized)
            {
            for (const auto& question : m_threePointCategorizedQuestions)
                { AddSurveyQuestionBar(question); }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::FivePoint)
            {
            for (const auto& question : m_fivePointQuestions)
                { AddSurveyQuestionBar(question); }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            for (const auto& question : m_fivePointCategorizedQuestions)
                { AddSurveyQuestionBar(question); }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
            GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint)
            {
            for (const auto& question : m_sevenPointQuestions)
                { AddSurveyQuestionBar(question); }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            for (const auto& question : m_sevenPointCategorizedQuestions)
                { AddSurveyQuestionBar(question); }
            }

        GetScalingAxis().SetRange(0,
            m_questionBlockSize +
            m_categoryBlockSize +
            m_negativeBlockSize +
            m_positiveBlockSize +
            (m_neutralMaxSize > 0 ? m_neutralBlockSize : 0) +
            (m_naMaxSize > 0 ? m_naBlockSize : 0),
            0, 10, 1);
        GetBarAxis().SetRange(0.5, GetBarSlotCount()+.5, 1, 1, 1);

        // add dividers between the positive, negative, and neutral sections
        GetCustomAxes().clear();

        if (m_neutralMaxSize > 0)
            {
            Axis neutralDividerLine(AxisType::RightYAxis);
            neutralDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize+
                                                  m_negativeBlockSize + m_positiveBlockSize);
            neutralDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
            neutralDividerLine.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
            neutralDividerLine.GetAxisLinePen() = wxNullPen;
            if (IsShowingSectionHeaders())
                {
                neutralDividerLine.GetHeader().SetText(GetNeutralLabel() + L"\U0001F816");
                neutralDividerLine.GetHeader().GetFont().MakeBold();
                neutralDividerLine.GetHeader().SetFontColor(ColorContrast::Shade(GetNeutralColor()));
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
                    const auto foundPos = std::find_if(bar.GetBlocks().cbegin(),
                                                       bar.GetBlocks().cend(),
                        [this](const auto& block) noexcept
                        { return block.GetTag() == GetNeutralBlockLabel(); });
                    if (foundPos != bar.GetBlocks().cend())
                        { bar.GetBlocks().erase(foundPos); }
                    else
                        { break; }
                    }
                }
            }

        if (m_naMaxSize > 0)
            {
            Axis naDividerLine(AxisType::RightYAxis);
            naDividerLine.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize +
                                             m_negativeBlockSize + m_positiveBlockSize +
                                             m_neutralBlockSize);
            naDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
            naDividerLine.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
            naDividerLine.GetAxisLinePen() = wxNullPen;
            if (IsShowingSectionHeaders())
                {
                naDividerLine.GetFooter().SetText(GetNoResponseHeader() + L"\U0001F816");
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
                    auto foundPos = std::find_if(bar.GetBlocks().cbegin(), bar.GetBlocks().cend(),
                        [](const auto& block) noexcept
                        { return block.GetTag() == wxString(L"NA_BLOCK"); });
                    if (foundPos != bar.GetBlocks().cend())
                        { bar.GetBlocks().erase(foundPos); }
                    else
                        { break; }
                    }
                }
            }

        Axis agreeDividerLine(AxisType::RightYAxis);
        agreeDividerLine.SetCustomXPosition(
            m_questionBlockSize + m_categoryBlockSize + m_negativeBlockSize);
        agreeDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
        agreeDividerLine.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
        agreeDividerLine.GetAxisLinePen() = wxNullPen;
        if (IsShowingSectionHeaders())
            {
            agreeDividerLine.GetHeader().SetText(GetPositiveHeader() + L"\U0001F816");
            agreeDividerLine.GetHeader().SetRelativeAlignment(RelativeAlignment::FlushLeft);
            agreeDividerLine.GetHeader().GetPen() = wxNullPen;
            agreeDividerLine.GetHeader().GetFont().MakeBold();
            agreeDividerLine.GetHeader().SetFontColor(ColorContrast::Shade(GetPositiveColor()));
            agreeDividerLine.GetHeader().SetLeftPadding(5);
            }
        AddCustomAxis(agreeDividerLine);

        Axis disagreeDividerLine(AxisType::LeftYAxis);
        disagreeDividerLine.SetCustomXPosition(
            m_questionBlockSize + m_categoryBlockSize + m_negativeBlockSize);
        disagreeDividerLine.SetCustomYPosition(GetBarAxis().GetRange().second);
        disagreeDividerLine.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
        disagreeDividerLine.GetAxisLinePen() = wxNullPen;
        if (IsShowingSectionHeaders())
            {
            disagreeDividerLine.GetHeader().SetText(L"\U0001F814 " + GetNegativeHeader());
            disagreeDividerLine.GetHeader().SetRelativeAlignment(RelativeAlignment::FlushRight);
            disagreeDividerLine.GetHeader().GetPen() = wxNullPen;
            disagreeDividerLine.GetHeader().GetFont().MakeBold();
            disagreeDividerLine.GetHeader().SetFontColor(ColorContrast::Shade(GetNegativeColor()));
            disagreeDividerLine.GetHeader().SetRightPadding(5);
            }
        AddCustomAxis(disagreeDividerLine);

        Axis questionDividerBar(AxisType::LeftYAxis);
        questionDividerBar.SetCustomXPosition(m_questionBlockSize);
        questionDividerBar.SetCustomYPosition(GetBarAxis().GetRange().second);
        questionDividerBar.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
        AddCustomAxis(questionDividerBar);

        if (m_categoryBlockSize > 0)
            {
            Axis categoryDividerBar(AxisType::LeftYAxis);
            categoryDividerBar.SetCustomXPosition(m_questionBlockSize + m_categoryBlockSize);
            categoryDividerBar.SetCustomYPosition(GetBarAxis().GetRange().second);
            categoryDividerBar.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
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
                fullDividerLines.emplace_back(accumulated-1);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized)
            {
            size_t accumulated{ 0 };
            for (const auto& question : m_fivePointCategorizedQuestions)
                {
                accumulated += question.m_fivePointCategories.size();
                fullDividerLines.emplace_back(accumulated-1);
                }
            }
        else if (GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized ||
            GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized)
            {
            size_t accumulated{ 0 };
            for (const auto& question : m_sevenPointCategorizedQuestions)
                {
                accumulated += question.m_sevenPointCategories.size();
                fullDividerLines.emplace_back(accumulated-1);
                }
            }

        // lines between the bars, to make the plot look like a grid
        for (size_t i = 0; i < GetBarSlotCount(); ++i)
            {
            Axis dividerHorizontalBar(AxisType::BottomXAxis);
            dividerHorizontalBar.SetCustomXPosition(GetScalingAxis().GetRange().second);
            dividerHorizontalBar.SetCustomYPosition(i+1.5);
            if (fullDividerLines.size() &&
                std::find(fullDividerLines.cbegin(), fullDividerLines.cend(), i) == fullDividerLines.cend())
                { dividerHorizontalBar.SetOffsetFromParentAxis(m_questionBlockSize); }
            dividerHorizontalBar.GetAxisLinePen().SetColour(ColorBrewer::GetColor(Color::AshGrey));
            AddCustomAxis(dividerHorizontalBar);
            }

        BarChart::RecalcSizes(dc);

        // overlay dashed dividing lines between sections
        auto sectionDividerLines = std::make_shared<GraphItems::Lines>(
                                                    wxPen(*wxBLACK, 1, wxPENSTYLE_LONG_DASH), GetScaling());
        wxCoord bottomPosAndNegX{ 0 }, bottomNeutralX{ 0 }, bottomNAX{ 0 };
        if (GetBottomXAxis().GetPhysicalCoordinate(m_questionBlockSize + m_categoryBlockSize +
                                                   m_negativeBlockSize, bottomPosAndNegX))
            {
            sectionDividerLines->AddLine(wxPoint(bottomPosAndNegX, GetLeftYAxis().GetBottomPoint().y),
                wxPoint(bottomPosAndNegX, GetLeftYAxis().GetTopPoint().y));
            }
        if (GetBottomXAxis().GetPhysicalCoordinate(m_questionBlockSize + m_categoryBlockSize +
                                                   m_negativeBlockSize + m_positiveBlockSize, bottomNeutralX))
            {
            sectionDividerLines->AddLine(wxPoint(bottomNeutralX, GetLeftYAxis().GetBottomPoint().y),
                wxPoint(bottomNeutralX, GetLeftYAxis().GetTopPoint().y));
            }
        if (GetBottomXAxis().GetPhysicalCoordinate(m_questionBlockSize + m_categoryBlockSize +
                                                   m_negativeBlockSize + m_positiveBlockSize +
                                                   m_neutralBlockSize, bottomNAX))
            {
            sectionDividerLines->AddLine(wxPoint(bottomNAX, GetLeftYAxis().GetBottomPoint().y),
                wxPoint(bottomNAX, GetLeftYAxis().GetTopPoint().y));
            }
        AddObject(sectionDividerLines);

        AddQuestionBrackets();
        // make a little smaller as these could be rather lengthy
        // and consume a lot of real estate
        for (auto& bracket : GetLeftYAxis().GetBrackets())
            { bracket.GetLabel().GetFont().MakeSmaller(); }
        }
    }
