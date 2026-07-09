///////////////////////////////////////////////////////////////////////////////
// Name:        reportbuilder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "../data/pivot.h"
#include "../data/subset.h"
#include "reportbuilder.h"
#include <utility>

namespace Wisteria
    {
    //---------------------------------------------------
    void ReportBuilder::LoadConstants(const wxSimpleJSON::Ptr_t& constantsNode)
        {
        if (constantsNode->IsOk())
            {
            const auto values = constantsNode->AsNodes();
            for (const auto& value : values)
                {
                if (value->IsOk())
                    {
                    const wxString vName = value->GetProperty(_DT(L"name"))->AsString();
                    m_values.insert_or_assign(
                        vName, value->GetProperty(L"value")->IsValueString() ?
                                   ValuesType(value->GetProperty(L"value")->AsString()) :
                                   ValuesType(value->GetProperty(L"value")->AsDouble()));

                    DatasetFormulaInfo constInfo;
                    constInfo.m_name = vName;
                    if (value->GetProperty(L"value")->IsValueString())
                        {
                        constInfo.m_value = value->GetProperty(L"value")->AsString();
                        }
                    else if (value->GetProperty(L"value")->IsValueNumber())
                        {
                        constInfo.m_value =
                            std::to_wstring(value->GetProperty(L"value")->AsDouble());
                        }
                    m_constants.insert(std::move(constInfo));
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::RecalcFormula(const wxString& formulaName, const wxString& formulaValue,
                                      const wxString& datasetName)
        {
        const auto dsIt = m_datasets.find(datasetName);
        if (dsIt == m_datasets.cend() || dsIt->second == nullptr)
            {
            return;
            }
        m_values.insert_or_assign(formulaName, CalcFormula(formulaValue, dsIt->second));
        }

    //---------------------------------------------------
    std::optional<std::vector<wxString>>
    ReportBuilder::ExpandColumnSelections(wxString var,
                                          const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (var.starts_with(L"{{") && var.ends_with(L"}}"))
            {
            var = var.substr(2, var.length() - 4);
            }
        else
            {
            return std::nullopt;
            }
        const wxRegEx re(FunctionStartRegEx() + L"(everything|everythingexcept|matches)" +
                         OpeningParenthesisRegEx() + ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(var))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount < 2)
                {
                return std::nullopt;
                }

            const wxString funcName = re.GetMatch(var, 1).MakeLower();
            std::vector<wxString> columns;

            if (funcName.CmpNoCase(L"everything") == 0)
                {
                if (!dataset->GetIdColumn().GetName().empty())
                    {
                    columns.push_back(dataset->GetIdColumn().GetName());
                    }
                const auto catCols{ dataset->GetCategoricalColumnNames() };
                const auto contCols{ dataset->GetContinuousColumnNames() };
                const auto dateCols{ dataset->GetDateColumnNames() };
                std::ranges::copy(catCols, std::back_inserter(columns));
                std::ranges::copy(contCols, std::back_inserter(columns));
                std::ranges::copy(dateCols, std::back_inserter(columns));

                return columns;
                }
            if (paramPartsCount >= 3)
                {
                const wxString columnPattern =
                    ConvertColumnOrGroupParameter(re.GetMatch(var, 2), dataset);

                if (funcName.CmpNoCase(L"matches") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that contain the string
                        if (columnRE.Matches(dataset->GetIdColumn().GetName()))
                            {
                            columns.push_back(dataset->GetIdColumn().GetName());
                            }
                        for (const auto& col : dataset->GetCategoricalColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetContinuousColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetDateColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        return columns;
                        }
                    return std::nullopt;
                    }
                if (funcName.CmpNoCase(L"everythingexcept") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that DON'T contain the string
                        if (!dataset->GetIdColumn().GetName().empty() &&
                            !columnRE.Matches(dataset->GetIdColumn().GetName()))
                            {
                            columns.push_back(dataset->GetIdColumn().GetName());
                            }
                        for (const auto& col : dataset->GetCategoricalColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetContinuousColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetDateColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        return columns;
                        }
                    return std::nullopt;
                    }

                return std::nullopt;
                }

            return std::nullopt;
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    void ReportBuilder::CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
                                     const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (formulasNode->IsOk())
            {
            const auto formulas = formulasNode->AsNodes();
            for (const auto& formula : formulas)
                {
                if (formula->IsOk())
                    {
                    const wxString vName = formula->GetProperty(_DT(L"name"))->AsString();
                    if (formula->GetProperty(L"value")->IsValueString())
                        {
                        m_values.insert_or_assign(
                            vName,
                            CalcFormula(formula->GetProperty(L"value")->AsString(), dataset));
                        }
                    else if (formula->GetProperty(L"value")->IsValueNumber())
                        {
                        m_values.insert_or_assign(vName,
                                                  formula->GetProperty(L"value")->AsDouble());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType
    ReportBuilder::CalcFormula(const wxString& formula,
                               const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        const wxRegEx re(
            FunctionStartRegEx() +
            LR"((min|max|median|n|total|grandtotal|groupcount|grouppercentdecimal|grouppercent|continuouscolumn|now|pagenumber|reportname|add|distinct))" +
            OpeningParenthesisRegEx());
        if (re.Matches(formula))
            {
            const wxString funcName = re.GetMatch(formula, 1).MakeLower();
            if (funcName.CmpNoCase(L"min") == 0 || funcName.CmpNoCase(L"max") == 0)
                {
                return CalcMinMax(formula, dataset);
                }
            if (funcName.CmpNoCase(L"median") == 0)
                {
                const auto calcValue = CalcMedian(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"distinct") == 0)
                {
                const auto calcValue = CalcDistinct(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"n") == 0)
                {
                const auto calcValue = CalcValidN(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"total") == 0)
                {
                const auto calcValue = CalcTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grandtotal") == 0)
                {
                const auto calcValue = CalcGrandTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"groupcount") == 0)
                {
                const auto calcValue = CalcGroupCount(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grouppercentdecimal") == 0)
                {
                const auto calcValue = CalcGroupPercentDecimal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grouppercent") == 0)
                {
                const auto calcValue = CalcGroupPercent(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"continuouscolumn") == 0)
                {
                return ExpandColumnSelection(formula, dataset);
                }
            if (funcName.CmpNoCase(L"now") == 0)
                {
                return CalcNow(formula);
                }
            if (funcName.CmpNoCase(L"add") == 0)
                {
                return CalcAdd(formula);
                }
            if (funcName.CmpNoCase(L"pagenumber") == 0)
                {
                return FormatPageNumber(formula);
                }
            if (funcName.CmpNoCase(L"reportname") == 0)
                {
                return m_name;
                }
            }
        // note that formula may just be a constant (e.g., color string),
        // so return that if it can't be calculated into something else
        return formula;
        }

    //---------------------------------------------------
    wxString
    ReportBuilder::ExpandColumnSelection(const wxString& formula,
                                         const std::shared_ptr<const Data::Dataset>& dataset)
        {
        const wxRegEx re(FunctionStartRegEx() + L"(continuouscolumn)" + OpeningParenthesisRegEx() +
                         NumberOrStringRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            if (dataset->GetContinuousColumns().empty())
                {
                throw std::runtime_error(
                    wxString(_(L"ContinuousColumn() failed. "
                               "There are no continuous columns in the dataset."))
                        .ToUTF8());
                }
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                unsigned long columnIndex{ 0 };
                wxString columnIndexStr = re.GetMatch(formula, 2);
                if (columnIndexStr.starts_with(L"`") && columnIndexStr.ends_with(L"`"))
                    {
                    columnIndexStr = columnIndexStr.substr(1, columnIndexStr.length() - 2);
                    if (columnIndexStr.CmpNoCase(L"last") == 0)
                        {
                        columnIndex = dataset->GetContinuousColumns().size() - 1;
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(
                                _(L"'%s': unknown constant for continuous column index."),
                                columnIndexStr)
                                .ToUTF8());
                        }
                    }
                else if (columnIndexStr.ToULong(&columnIndex))
                    {
                    if (columnIndex >= dataset->GetContinuousColumns().size())
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%lu: invalid continuous column index."),
                                             columnIndex)
                                .ToUTF8());
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': unable to convert value for continuous column index."),
                            columnIndexStr)
                            .ToUTF8());
                    }
                return dataset->GetContinuousColumn(columnIndex).GetName();
                }
            }

        // can't get the name of the column, just return the original text
        return formula;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGroupCount(const wxString& formula,
                                  const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(groupcount)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 4)
                {
                const wxString groupName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 3), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }

                return dataset->GetCategoricalColumnValidN(groupName, groupName, groupID);
                }
            // dataset or something missing
            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupPercentDecimal(
        const wxString& formula, const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() + L"(grouppercentdecimal)" +
                         OpeningParenthesisRegEx() + ColumnNameOrFormulaRegEx() +
                         ParamSeparatorRegEx() + ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercentdecimal)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"groupcount");
                if (const auto groupTotal = CalcGroupCount(countFormula, dataset))
                    {
                    return safe_divide<double>(groupTotal.value(), dataset->GetRowCount());
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGroupPercent(const wxString& formula,
                                    const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() + L"(grouppercent)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            if (const wxRegEx reFunctionRename(L"(?i)(grouppercent)");
                reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"grouppercentdecimal");
                if (const auto percDec = CalcGroupPercentDecimal(countFormula, dataset))
                    {
                    return wxRound(percDec.value() * 100);
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcMedian(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() + L"(median)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() + L"(median)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (reSimple.Matches(formula))
            {
            const auto paramPartsCount = reSimple.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reSimple.GetMatch(formula, 2), dataset);
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousMedian(columnName);
                    }
                }
            // dataset or column name missing
            else
                {
                return std::nullopt;
                }
            }
        else if (reExtended.Matches(formula))
            {
            const auto paramPartsCount = reExtended.GetMatchCount();
            if (paramPartsCount >= 5)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 2), dataset);
                const wxString groupName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 3), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 4), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousMedian(columnName, groupName, groupID);
                    }
                }
            // dataset or something missing
            else
                {
                return std::nullopt;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcValidN(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() + L"(n)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() + L"(n)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (reSimple.Matches(formula))
            {
            const auto paramPartsCount = reSimple.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reSimple.GetMatch(formula, 2), dataset);
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName);
                    }
                }
            // dataset or column name missing
            else
                {
                return std::nullopt;
                }
            }
        else if (reExtended.Matches(formula))
            {
            const auto paramPartsCount = reExtended.GetMatchCount();
            if (paramPartsCount >= 5)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 2), dataset);
                const wxString groupName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 3), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 4), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName, groupName, groupID);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName, groupName, groupID);
                    }
                }
            // dataset or something missing
            else
                {
                return std::nullopt;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcDistinct(const wxString& formula,
                                const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(distinct)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // only continuous can be totaled
                if (const auto catColumn = dataset->GetCategoricalColumn(columnName);
                    catColumn != dataset->GetCategoricalColumns().cend())
                    {
                    return catColumn->GetStringTable().size();
                    }

                throw std::runtime_error(
                    wxString::Format(_(L"%s: column must be categorical for distinct."), columnName)
                        .ToUTF8());
                }
            // dataset or column name missing

            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ConvertColumnOrGroupParameter(
        wxString columnStr, const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (columnStr.starts_with(L"`") && columnStr.ends_with(L"`"))
            {
            columnStr = columnStr.substr(1, columnStr.length() - 2);
            }
        else if (columnStr.starts_with(L"{{") && columnStr.ends_with(L"}}"))
            {
            columnStr = columnStr.substr(2, columnStr.length() - 4);
            columnStr = ExpandConstants(columnStr);
            const auto calcStr = CalcFormula(columnStr, dataset);
            if (const auto* const strVal{ std::get_if<wxString>(&calcStr) }; strVal != nullptr)
                {
                return *strVal;
                }

            return {};
            }

        return columnStr;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGrandTotal(const wxString& formula,
                                  const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(grandtotal)" + OpeningParenthesisRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                // only continuous can be totaled
                double total{ 0 };
                for (size_t i = 0; i < dataset->GetContinuousColumns().size(); ++i)
                    {
                    total += dataset->GetContinuousTotal(i);
                    }
                return total;
                }
            // dataset or column name missing

            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcTotal(const wxString& formula,
                             const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(total)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // only continuous can be totaled
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousTotal(columnName);
                    }

                throw std::runtime_error(
                    wxString::Format(_(L"%s: column must be continuous when totaling."), columnName)
                        .ToUTF8());
                }
            // dataset or column name missing

            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType
    ReportBuilder::CalcMinMax(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(min|max)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);

                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    const auto [minVal, maxVal] = dataset->GetCategoricalMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    const auto [minVal, maxVal] = dataset->GetContinuousMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                wxLogWarning(L"'%s' column not found in call to MIN or MAX. "
                             "A continuous or categorical column was expected.",
                             columnName);
                }
            // dataset or column name missing
            else
                {
                return formula;
                }
            }
        return formula;
        }

    //---------------------------------------------------
    wxString ReportBuilder::FormatPageNumber(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() + L"(pagenumber)" + OpeningParenthesisRegEx() +
                         StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        return re.Matches(formula) ?
                   wxNumberFormatter::ToString(m_pageNumber, 0,
                                               wxNumberFormatter::Style::Style_WithThousandsSep) :
                   wxString{};
        }

    //---------------------------------------------------
    wxString ReportBuilder::CalcAdd(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() + L"(add)" + OpeningParenthesisRegEx() +
                         NumberOrStringRegEx() + ParamSeparatorRegEx() + NumberOrStringRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            if (re.GetMatchCount() >= 3)
                {
                wxString firstValue = re.GetMatch(formula, 2);
                if (firstValue.starts_with(L"`") && firstValue.ends_with(L"`"))
                    {
                    firstValue = ExpandConstants(firstValue.substr(1, firstValue.length() - 2));
                    }
                wxString secondValue = re.GetMatch(formula, 3);
                if (secondValue.starts_with(L"`") && secondValue.ends_with(L"`"))
                    {
                    secondValue = ExpandConstants(secondValue.substr(1, secondValue.length() - 2));
                    }
                double secondDouble{ 0 };
                if (secondValue.ToCDouble(&secondDouble))
                    {
                    const auto firstNumber = firstValue.find_first_of(L"0123456789");
                    if (firstNumber == wxString::npos)
                        {
                        return {};
                        }
                    const auto endOfNumber =
                        firstValue.find_first_not_of(L"0123456789.", firstNumber);
                    const wxString prefix = firstValue.substr(0, firstNumber);
                    double firstDouble{ 0 };
                    if (firstValue
                            .substr(firstNumber, endOfNumber == wxString::npos ?
                                                     wxString::npos :
                                                     endOfNumber - firstNumber)
                            .ToCDouble(&firstDouble))
                        {
                        return prefix + wxNumberFormatter::ToString(
                                            (firstDouble + secondDouble), 2,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes);
                        }
                    }
                }
            }
        return {};
        }

    //---------------------------------------------------
    wxString ReportBuilder::CalcNow(const wxString& formula)
        {
        const wxRegEx re(FunctionStartRegEx() + L"(now)" + OpeningParenthesisRegEx() +
                         StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                wxString paramValue = re.GetMatch(formula, 2).MakeLower();
                if (paramValue.starts_with(L"`") && paramValue.ends_with(L"`"))
                    {
                    paramValue = paramValue.substr(1, paramValue.length() - 2);
                    }

                if (paramValue.empty())
                    {
                    return wxDateTime::Now().FormatDate();
                    }
                if (paramValue == L"fancy")
                    {
                    return wxDateTime::Now().Format(L"%B %d, %Y");
                    }
                // which part of the date is requested?
                if (paramValue == L"year")
                    {
                    return std::to_wstring(wxDateTime::Now().GetYear());
                    }
                if (paramValue == L"month")
                    {
                    return std::to_wstring(wxDateTime::Now().GetMonth());
                    }
                if (paramValue == L"monthname")
                    {
                    return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth());
                    }
                if (paramValue == L"monthshortname")
                    {
                    return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth(),
                                                    wxDateTime::Name_Abbr);
                    }
                if (paramValue == L"day")
                    {
                    return std::to_wstring(wxDateTime::Now().GetDay());
                    }
                if (paramValue == L"dayname")
                    {
                    return wxDateTime::GetWeekDayName(wxDateTime::Now().GetWeekDay());
                    }

                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown parameter passed to Now()."), paramValue)
                        .ToUTF8());
                }
            // no param, just return full date
            return wxDateTime::Now().Format();
            }
        return wxDateTime::Now().Format();
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::ExpandNumericConstant(wxString str) const
        {
        if (str.starts_with(L"{{") && str.ends_with(L"}}"))
            {
            str = str.substr(2, str.length() - 4);
            }
        const auto foundVal = m_values.find(str);
        if (foundVal != m_values.cend())
            {
            if (const auto* const dVal{ std::get_if<double>(&foundVal->second) };
                dVal != nullptr && !std::isnan(*dVal))
                {
                return *dVal;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ExpandConstants(wxString str) const
        {
        const wxRegEx re(L"{{([^}]+)}}");
        size_t start{ 0 }, len{ 0 };
        std::wstring_view processText(str.wc_str());
        std::map<wxString, wxString> replacements;
        while (re.Matches(processText.data()))
            {
            // catalog all the placeholders and their replacements
            [[maybe_unused]]
            const auto fullMatchResult = re.GetMatch(&start, &len, 0);
            const auto foundVal = m_values.find(re.GetMatch(processText.data(), 1));
            if (foundVal != m_values.cend())
                {
                if (const auto* const strVal{ std::get_if<wxString>(&foundVal->second) };
                    strVal != nullptr)
                    {
                    replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                  *strVal);
                    }
                else if (const auto* const dVal{ std::get_if<double>(&foundVal->second) };
                         dVal != nullptr)
                    {
                    if (std::isnan(*dVal))
                        {
                        replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                      wxString{});
                        }
                    else
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)),
                            wxNumberFormatter::ToString(
                                *dVal, 2,
                                wxNumberFormatter::Style::Style_WithThousandsSep |
                                    wxNumberFormatter::Style::Style_NoTrailingZeroes));
                        }
                    }
                }
            // or a function like "Now()" that doesn't require a dataset
            else
                {
                const auto calcStr = CalcFormula(re.GetMatch(processText.data(), 1), nullptr);
                if (const auto* const strVal{ std::get_if<wxString>(&calcStr) }; strVal != nullptr)
                    {
                    replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                  *strVal);
                    }
                }
            processText = processText.substr(start + len);
            }

        // now, replace the placeholders with the user-defined values mapped to them
        for (const auto& rep : replacements)
            {
            str.Replace(rep.first, rep.second);
            }

        return str;
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(const wxSimpleJSON::Ptr_t& colorNode) const
        {
        if (!colorNode->IsOk())
            {
            return wxNullColour;
            }

        return (colorNode->IsValueNull() ?
                    // using null in JSON for a color implies that we want
                    // a legit color that is transparent
                    wxTransparentColour :
                    ConvertColor(colorNode->AsString()));
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(wxString colorStr,
                                         GraphItems::GraphItemBase* item /*= nullptr*/,
                                         const wxString& property /*= wxString{}*/) const
        {
        long opacity{ wxALPHA_OPAQUE };
        if (const auto colonPos = colorStr.find(L':'); colonPos != wxString::npos)
            {
            colorStr.substr(colonPos + 1).ToLong(&opacity);
            colorStr = colorStr.substr(0, colonPos);
            }
        // in case the color is a user-defined constant in the file
        const wxString rawColorStr = colorStr;
        colorStr = ExpandConstants(colorStr);
        if (!rawColorStr.empty() && item != nullptr && !property.empty())
            {
            item->SetPropertyTemplate(property, rawColorStr);
            }

        wxColour retColor;
        // see if it is one of our defined colors
        auto foundPos = m_colorMap.find(std::wstring_view(colorStr.MakeLower().wc_str()));
        if (foundPos != m_colorMap.cend())
            {
            retColor = Colors::ColorBrewer::GetColor(foundPos->second);
            }
        else if (colorStr == L"transparent")
            {
            retColor = wxTransparentColour;
            }
        // may be a hex string or RGB string
        else
            {
            retColor = wxColour{ colorStr };
            }

        if (std::cmp_not_equal(opacity, wxALPHA_OPAQUE))
            {
            retColor = Colors::ColorContrast::ChangeOpacity(retColor, opacity);
            }
        return retColor;
        }
    } // namespace Wisteria
