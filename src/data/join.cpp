///////////////////////////////////////////////////////////////////////////////
// Name:        join.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "join.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    std::shared_ptr<Dataset>
    DatasetJoin::LeftJoinUnique(const std::shared_ptr<const Dataset>& leftDataset,
                                const std::shared_ptr<const Dataset>& rightDataset,
                                const std::vector<std::pair<wxString, wxString>>& byColumns,
                                const wxString& suffix /*= L".x"*/)
        {
        if (leftDataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid left dataset when left joining.").ToUTF8());
            }
        if (rightDataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid right dataset when left joining.").ToUTF8());
            }
        if (byColumns.empty())
            {
            throw std::runtime_error(
                _(L"No comparison columns were provided when left joining.").ToUTF8());
            }
        if (suffix.empty())
            {
            throw std::runtime_error(_(L"Suffix should not be empty when left joining.").ToUTF8());
            }

        auto mergedData = std::make_shared<Dataset>(*leftDataset);

        // mapped 'by' columns between datasets (src -> output)
        std::pair<const Column<wxString>*, const Column<wxString>*> byIdColumnsMap{ std::make_pair(
            nullptr, nullptr) };
        std::vector<std::pair<CategoricalColumnConstIterator, CategoricalColumnConstIterator>>
            byCatColsMap;

        // mapped input (right dataset) and output columns
        std::pair<const Column<wxString>*, Column<wxString>*> outIdColumnsMap{ std::make_pair(
            nullptr, nullptr) };
        std::vector<std::pair<wxString, wxString>> outCatColNamesMap;
        std::vector<std::pair<CategoricalColumnConstIterator, CategoricalColumnIterator>>
            outCatColsMap;
        std::vector<std::pair<wxString, wxString>> outContinuousColNamesMap;
        std::vector<std::pair<ContinuousColumnConstIterator, ContinuousColumnIterator>>
            outContinuousColsMap;
        std::vector<std::pair<wxString, wxString>> outDateColNamesMap;
        std::vector<std::pair<DateColumnConstIterator, DateColumnIterator>> outDateColsMap;

        // verify that 'by' columns are in both datasets
        for (const auto& byColumn : byColumns)
            {
            // general column checks
            if (byColumn.first.empty() || byColumn.second.empty())
                {
                throw std::runtime_error(_(L"Empty 'by' column when left joining.").ToUTF8());
                }
            if (!mergedData->ContainsColumn(byColumn.first))
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': column not found in left dataset when left joining."),
                        byColumn.first)
                        .ToUTF8());
                }
            if (!rightDataset->ContainsColumn(byColumn.second))
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': column not found in right dataset when left joining."),
                        byColumn.second)
                        .ToUTF8());
                }
            }

        // prepare the fused dataset with non-join columns from the right dataset
        //--------------------------
        std::set<wxString, wxStringLessNoCase> rightColumnsToJoinBy;
        std::ranges::for_each(byColumns,
                              [&](const auto& val) { rightColumnsToJoinBy.insert(val.second); });

        // if right has an active ID column, left does not, and we are not joining by
        // it then add that (this would be an unlikely case)
        if (rightDataset->HasValidIdData() && !mergedData->HasValidIdData() &&
            !rightColumnsToJoinBy.contains(rightDataset->GetIdColumn().GetName()))
            {
            mergedData->GetIdColumn().SetName(rightDataset->GetIdColumn().GetName());
            outIdColumnsMap =
                std::make_pair(&rightDataset->GetIdColumn(), &mergedData->GetIdColumn());
            }
        // If both datasets have ID columns and you are not joining by them,
        // then the one from the right will not be copied over since a dataset
        // only has one ID column. This is an odd situation, so just log a warning about it.
        if (rightDataset->HasValidIdData() && mergedData->HasValidIdData() &&
            !rightColumnsToJoinBy.contains(rightDataset->GetIdColumn().GetName()))
            {
            wxLogWarning(
                L"'%s': ID column from right dataset will not be copied while left joining.",
                rightDataset->GetIdColumn().GetName());
            }

        const auto makeUniqueColumnName = [&](const wxString& colName)
        {
            wxString name = colName;
            int n{ 1 };
            while (mergedData->ContainsColumn(name))
                {
                name = colName + suffix + (n == 1 ? wxString{} : wxString(std::to_wstring(n)));
                ++n;
                }
            return name;
        };

        // add categoricals
        for (const auto& catCol : rightDataset->GetCategoricalColumns())
            {
            // if cat column is a join key, then we won't be adding that
            if (rightColumnsToJoinBy.contains(catCol.GetName()))
                {
                continue;
                }
            const wxString mergeColName = makeUniqueColumnName(catCol.GetName());
            mergedData->AddCategoricalColumn(mergeColName, catCol.GetStringTable());
            auto newCol = mergedData->GetCategoricalColumn(mergeColName);
            if (newCol != mergedData->GetCategoricalColumns().cend())
                {
                newCol->FillWithMissingData();
                }
            outCatColNamesMap.emplace_back(catCol.GetName(), mergeColName);
            }
        // add continuous
        for (const auto& continuousCol : rightDataset->GetContinuousColumns())
            {
            const wxString mergeColName = makeUniqueColumnName(continuousCol.GetName());
            mergedData->AddContinuousColumn(mergeColName);
            auto newCol = mergedData->GetContinuousColumn(mergeColName);
            if (newCol != mergedData->GetContinuousColumns().cend())
                {
                newCol->FillWithMissingData();
                }
            outContinuousColNamesMap.emplace_back(continuousCol.GetName(), mergeColName);
            }
        // add datetime
        for (const auto& dateCol : rightDataset->GetDateColumns())
            {
            const wxString mergeColName = makeUniqueColumnName(dateCol.GetName());
            mergedData->AddDateColumn(mergeColName);
            auto newCol = mergedData->GetDateColumn(mergeColName);
            if (newCol != mergedData->GetDateColumns().cend())
                {
                newCol->FillWithMissingData();
                }
            outDateColNamesMap.emplace_back(dateCol.GetName(), mergeColName);
            }

        // map the 'by' columns
        //--------------------------
        for (const auto& byColumn : byColumns)
            {
            // map ID columns
            if (mergedData->GetIdColumn().GetName().CmpNoCase(byColumn.first) == 0)
                {
                if (rightDataset->GetIdColumn().GetName().CmpNoCase(byColumn.second) == 0)
                    {
                    byIdColumnsMap =
                        std::make_pair(&rightDataset->GetIdColumn(), &mergedData->GetIdColumn());
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"Left joining by ID columns, but '%s' is not the ID column "
                              "in the right dataset."),
                            byColumn.second)
                            .ToUTF8());
                    }
                }
            // map categorical columns
            else if (const auto mergeCatCol = mergedData->GetCategoricalColumn(byColumn.first);
                     mergeCatCol != mergedData->GetCategoricalColumns().cend())
                {
                if (const auto rightCatCol = rightDataset->GetCategoricalColumn(byColumn.second);
                    rightCatCol != rightDataset->GetCategoricalColumns().cend())
                    {
                    byCatColsMap.emplace_back(rightCatCol, mergeCatCol);
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': categorical column not found in right dataset when left "
                              L"joining. "
                              "'By' columns must be either ID or categorical columns."),
                            byColumn.second)
                            .ToUTF8());
                    }
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': categorical column not found in left dataset when left joining. "
                          "'By' columns must be either ID or categorical columns."),
                        byColumn.first)
                        .ToUTF8());
                }
            }

        // map the right (source) columns with the out columns
        //--------------------------
        for (const auto& [srcCol, outCol] : outCatColNamesMap)
            {
            const auto rCol = rightDataset->GetCategoricalColumn(srcCol);
            // cppcheck-suppress assertWithSideEffect
            assert(rCol != rightDataset->GetCategoricalColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetCategoricalColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            assert(mCol != mergedData->GetCategoricalColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            outCatColsMap.emplace_back(rCol, mCol);
            }
        for (const auto& [srcCol, outCol] : outContinuousColNamesMap)
            {
            const auto rCol = rightDataset->GetContinuousColumn(srcCol);
            // cppcheck-suppress assertWithSideEffect
            assert(rCol != rightDataset->GetContinuousColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetContinuousColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            assert(mCol != mergedData->GetContinuousColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            outContinuousColsMap.emplace_back(rCol, mCol);
            }
        for (const auto& [srcCol, outCol] : outDateColNamesMap)
            {
            const auto rCol = rightDataset->GetDateColumn(srcCol);
            // cppcheck-suppress assertWithSideEffect
            assert(rCol != rightDataset->GetDateColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetDateColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            assert(mCol != mergedData->GetDateColumns().cend() &&
                   L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            outDateColsMap.emplace_back(rCol, mCol);
            }

        // merge the data
        //--------------------------
        wxString currentKeyInfo;
        bool warningLogged{ false };
        for (size_t rightDataRow = 0; rightDataRow < rightDataset->GetRowCount(); ++rightDataRow)
            {
            for (size_t mergeRow = 0; mergeRow < mergedData->GetRowCount(); ++mergeRow)
                {
                currentKeyInfo.clear();
                warningLogged = false;
                // matching on ID columns
                if (byIdColumnsMap.first != nullptr && byIdColumnsMap.second != nullptr &&
                    byIdColumnsMap.first->GetValue(rightDataRow)
                            .CmpNoCase(byIdColumnsMap.second->GetValue(mergeRow)) != 0)
                    {
                    continue;
                    }
                if (byIdColumnsMap.first != nullptr && byIdColumnsMap.second != nullptr)
                    {
                    currentKeyInfo.append(byIdColumnsMap.first->GetName())
                        .append(L": ")
                        .append(byIdColumnsMap.first->GetValue(rightDataRow));
                    }
                bool allKeysMatch{ true };
                // compare all categorical keys and bail if any that don't match
                for (const auto& [srcCol, outCol] : byCatColsMap)
                    {
                    const auto srcStr = srcCol->GetValueAsLabel(rightDataRow);
                    if (srcStr.CmpNoCase(outCol->GetValueAsLabel(mergeRow)) != 0)
                        {
                        allKeysMatch = false;
                        break;
                        }
                    currentKeyInfo.append(L", ")
                        .append(srcCol->GetName())
                        .append(L": ")
                        .append(srcStr);
                    }
                if (currentKeyInfo.starts_with(L", "))
                    {
                    currentKeyInfo.erase(0, 2);
                    }
                // we have a match, so copy over data
                if (allKeysMatch)
                    {
                    if (outIdColumnsMap.first != nullptr && outIdColumnsMap.second != nullptr)
                        {
                        if (!outIdColumnsMap.second->GetValue(mergeRow).empty())
                            {
                            wxLogWarning(L"'%s': duplicate matching row from right dataset when "
                                         L"performing left join. "
                                         "Last occurrence of matching row will be used.",
                                         currentKeyInfo);
                            warningLogged = true;
                            }
                        outIdColumnsMap.second->SetValue(
                            mergeRow, outIdColumnsMap.first->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outCatColsMap)
                        {
                        // Matching out row should have empty cells for the out columns from
                        // the right dataset. If there is something there, then that means we
                        // already had a match for this set of keys and there are more than one
                        // matching row from the right file. In this case, we will overwrite the
                        // data from the current match.
                        auto mdCode = outCol->FindMissingDataCode();
                        if (mdCode.has_value() && outCol->GetValue(mergeRow) != mdCode.value() &&
                            // if we already logged a duplicate for the current row,
                            // then don't log again
                            !warningLogged)
                            {
                            wxLogWarning(L"'%s': duplicate matching row from right dataset when "
                                         L"performing left join. "
                                         "Last occurrence of matching row will be used.",
                                         currentKeyInfo);
                            warningLogged = true;
                            }
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outContinuousColsMap)
                        {
                        if (!outCol->IsMissingData(mergeRow) && !warningLogged)
                            {
                            wxLogWarning(L"'%s': duplicate matching row from right dataset when "
                                         "performing left join. "
                                         "Last occurrence of matching row will be used.",
                                         currentKeyInfo);
                            warningLogged = true;
                            }
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outDateColsMap)
                        {
                        if (!outCol->IsMissingData(mergeRow) && !warningLogged)
                            {
                            wxLogWarning(L"'%s': duplicate matching row from right dataset when "
                                         "performing left join. "
                                         "Last occurrence of matching row will be used.",
                                         currentKeyInfo);
                            warningLogged = true;
                            }
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    }
                }
            }

        return mergedData;
        }
    } // namespace Wisteria::Data
