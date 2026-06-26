///////////////////////////////////////////////////////////////////////////////
// Name:        join_left.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "join_left.h"
#include <unordered_map>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    DatasetLeftJoin::LeftJoinContext
    DatasetLeftJoin::PrepareLeftJoin(const std::shared_ptr<const Dataset>& leftDataset,
                                     const std::shared_ptr<const Dataset>& rightDataset,
                                     const std::vector<std::pair<wxString, wxString>>& byColumns,
                                     const wxString& suffix)
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

        LeftJoinContext ctx;
        ctx.mergedData = std::make_shared<Dataset>(*leftDataset);

        // intermediate name-to-name mappings (only needed during setup)
        std::vector<std::pair<wxString, wxString>> outCatColNamesMap;
        std::vector<std::pair<wxString, wxString>> outContinuousColNamesMap;
        std::vector<std::pair<wxString, wxString>> outDateColNamesMap;

        // verify that 'by' columns are in both datasets
        for (const auto& byColumn : byColumns)
            {
            // general column checks
            if (byColumn.first.empty() || byColumn.second.empty())
                {
                throw std::runtime_error(_(L"Empty 'by' column when left joining.").ToUTF8());
                }
            if (!ctx.mergedData->ContainsColumn(byColumn.first))
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
        if (rightDataset->HasValidIdData() && !ctx.mergedData->HasValidIdData() &&
            !rightColumnsToJoinBy.contains(rightDataset->GetIdColumn().GetName()))
            {
            ctx.mergedData->GetIdColumn().SetName(rightDataset->GetIdColumn().GetName());
            ctx.outIdColumnsMap =
                std::make_pair(&rightDataset->GetIdColumn(), &ctx.mergedData->GetIdColumn());
            }
        // If both datasets have ID columns and you are not joining by them,
        // then the one from the right will not be copied over since a dataset
        // only has one ID column. This is an odd situation, so just log a warning about it.
        if (rightDataset->HasValidIdData() && ctx.mergedData->HasValidIdData() &&
            !rightColumnsToJoinBy.contains(rightDataset->GetIdColumn().GetName()))
            {
            wxLogWarning(
                L"'%s': ID column from right dataset will not be copied while left joining.",
                rightDataset->GetIdColumn().GetName());
            }

        const auto makeUniqueColumnName = [&ctx, &suffix](const wxString& colName)
        {
            wxString name = colName;
            int n{ 1 };
            while (ctx.mergedData->ContainsColumn(name))
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
            ctx.mergedData->AddCategoricalColumn(mergeColName, catCol.GetStringTable());
            auto newCol = ctx.mergedData->GetCategoricalColumn(mergeColName);
            if (newCol != ctx.mergedData->GetCategoricalColumns().cend())
                {
                newCol->FillWithMissingData();
                }
            outCatColNamesMap.emplace_back(catCol.GetName(), mergeColName);
            }
        // add continuous
        for (const auto& continuousCol : rightDataset->GetContinuousColumns())
            {
            const wxString mergeColName = makeUniqueColumnName(continuousCol.GetName());
            ctx.mergedData->AddContinuousColumn(mergeColName);
            auto newCol = ctx.mergedData->GetContinuousColumn(mergeColName);
            if (newCol != ctx.mergedData->GetContinuousColumns().cend())
                {
                newCol->FillWithMissingData();
                }
            outContinuousColNamesMap.emplace_back(continuousCol.GetName(), mergeColName);
            }
        // add datetime
        for (const auto& dateCol : rightDataset->GetDateColumns())
            {
            const wxString mergeColName = makeUniqueColumnName(dateCol.GetName());
            ctx.mergedData->AddDateColumn(mergeColName);
            auto newCol = ctx.mergedData->GetDateColumn(mergeColName);
            if (newCol != ctx.mergedData->GetDateColumns().cend())
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
            if (ctx.mergedData->GetIdColumn().GetName().CmpNoCase(byColumn.first) == 0)
                {
                if (rightDataset->GetIdColumn().GetName().CmpNoCase(byColumn.second) == 0)
                    {
                    ctx.byIdColumnsMap = std::make_pair(&rightDataset->GetIdColumn(),
                                                        &ctx.mergedData->GetIdColumn());
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
            else if (const auto mergeCatCol = ctx.mergedData->GetCategoricalColumn(byColumn.first);
                     mergeCatCol != ctx.mergedData->GetCategoricalColumns().cend())
                {
                if (const auto rightCatCol = rightDataset->GetCategoricalColumn(byColumn.second);
                    rightCatCol != rightDataset->GetCategoricalColumns().cend())
                    {
                    ctx.byCatColsMap.emplace_back(rightCatCol, mergeCatCol);
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
            wxASSERT_MSG(rCol != rightDataset->GetCategoricalColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = ctx.mergedData->GetCategoricalColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != ctx.mergedData->GetCategoricalColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == ctx.mergedData->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            ctx.outCatColsMap.emplace_back(rCol, mCol);
            }
        for (const auto& [srcCol, outCol] : outContinuousColNamesMap)
            {
            const auto rCol = rightDataset->GetContinuousColumn(srcCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(rCol != rightDataset->GetContinuousColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = ctx.mergedData->GetContinuousColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != ctx.mergedData->GetContinuousColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == ctx.mergedData->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            ctx.outContinuousColsMap.emplace_back(rCol, mCol);
            }
        for (const auto& [srcCol, outCol] : outDateColNamesMap)
            {
            const auto rCol = rightDataset->GetDateColumn(srcCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(rCol != rightDataset->GetDateColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (rCol == rightDataset->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding right dataset column when left joining."),
                        srcCol)
                        .ToUTF8());
                }
            const auto mCol = ctx.mergedData->GetDateColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != ctx.mergedData->GetDateColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == ctx.mergedData->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': internal error finding merge dataset column when left joining."),
                        outCol)
                        .ToUTF8());
                }
            ctx.outDateColsMap.emplace_back(rCol, mCol);
            }

        ctx.rightDataset = rightDataset;
        return ctx;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset>
    DatasetLeftJoin::LeftJoinUniqueLast(const std::shared_ptr<const Dataset>& leftDataset,
                                        const std::shared_ptr<const Dataset>& rightDataset,
                                        const std::vector<std::pair<wxString, wxString>>& byColumns,
                                        const wxString& suffix /*= L".x"*/)
        {
        auto ctx = PrepareLeftJoin(leftDataset, rightDataset, byColumns, suffix);
        auto& [mergedData, byIdColumnsMap, byCatColsMap, outIdColumnsMap, outCatColsMap,
               outContinuousColsMap, outDateColsMap, rightDs] = ctx;

        // merge the data using hash-based lookup
        //--------------------------
        // build hash index of left/merged dataset rows by join key
        std::unordered_map<std::wstring, std::vector<size_t>> leftKeyIndex;
        leftKeyIndex.reserve(mergedData->GetRowCount());
        for (size_t mergeRow = 0; mergeRow < mergedData->GetRowCount(); ++mergeRow)
            {
            std::wstring leftKey;
            if (byIdColumnsMap.second != nullptr)
                {
                leftKey = byIdColumnsMap.second->GetValue(mergeRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!leftKey.empty())
                    {
                    leftKey += L'\x1F';
                    }
                leftKey += outCol->GetValueAsLabel(mergeRow).Lower().ToStdWstring();
                }
            leftKeyIndex[leftKey].push_back(mergeRow);
            }

        for (size_t rightDataRow = 0; rightDataRow < rightDs->GetRowCount(); ++rightDataRow)
            {
            // build key from right dataset
            std::wstring rightKey;
            if (byIdColumnsMap.first != nullptr)
                {
                rightKey = byIdColumnsMap.first->GetValue(rightDataRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!rightKey.empty())
                    {
                    rightKey += L'\x1F';
                    }
                rightKey += srcCol->GetValueAsLabel(rightDataRow).Lower().ToStdWstring();
                }
            if (const auto matchIter = leftKeyIndex.find(rightKey);
                matchIter != leftKeyIndex.cend())
                {
                // build key info string for warning messages
                wxString currentKeyInfo;
                if (byIdColumnsMap.first != nullptr)
                    {
                    currentKeyInfo.append(byIdColumnsMap.first->GetName())
                        .append(L": ")
                        .append(byIdColumnsMap.first->GetValue(rightDataRow));
                    }
                for (const auto& [srcCol, outCol] : byCatColsMap)
                    {
                    currentKeyInfo.append(L", ")
                        .append(srcCol->GetName())
                        .append(L": ")
                        .append(srcCol->GetValueAsLabel(rightDataRow));
                    }
                if (currentKeyInfo.starts_with(L", "))
                    {
                    currentKeyInfo.erase(0, 2);
                    }
                bool warningLogged{ false };
                for (const auto mergeRow : matchIter->second)
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

    //---------------------------------------------------
    std::shared_ptr<Dataset> DatasetLeftJoin::LeftJoinUniqueFirst(
        const std::shared_ptr<const Dataset>& leftDataset,
        const std::shared_ptr<const Dataset>& rightDataset,
        const std::vector<std::pair<wxString, wxString>>& byColumns,
        const wxString& suffix /*= L".x"*/)
        {
        auto ctx = PrepareLeftJoin(leftDataset, rightDataset, byColumns, suffix);
        auto& [mergedData, byIdColumnsMap, byCatColsMap, outIdColumnsMap, outCatColsMap,
               outContinuousColsMap, outDateColsMap, rightDs] = ctx;

        // merge the data using hash-based lookup
        //--------------------------
        // build hash index of left/merged dataset rows by join key
        std::unordered_map<std::wstring, std::vector<size_t>> leftKeyIndex;
        leftKeyIndex.reserve(mergedData->GetRowCount());
        for (size_t mergeRow = 0; mergeRow < mergedData->GetRowCount(); ++mergeRow)
            {
            std::wstring leftKey;
            if (byIdColumnsMap.second != nullptr)
                {
                leftKey = byIdColumnsMap.second->GetValue(mergeRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!leftKey.empty())
                    {
                    leftKey += L'\x1F';
                    }
                leftKey += outCol->GetValueAsLabel(mergeRow).Lower().ToStdWstring();
                }
            leftKeyIndex[leftKey].push_back(mergeRow);
            }

        for (size_t rightDataRow = 0; rightDataRow < rightDs->GetRowCount(); ++rightDataRow)
            {
            // build key from right dataset
            std::wstring rightKey;
            if (byIdColumnsMap.first != nullptr)
                {
                rightKey = byIdColumnsMap.first->GetValue(rightDataRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!rightKey.empty())
                    {
                    rightKey += L'\x1F';
                    }
                rightKey += srcCol->GetValueAsLabel(rightDataRow).Lower().ToStdWstring();
                }
            if (const auto matchIter = leftKeyIndex.find(rightKey);
                matchIter != leftKeyIndex.cend())
                {
                // build key info string for warning messages
                wxString currentKeyInfo;
                if (byIdColumnsMap.first != nullptr)
                    {
                    currentKeyInfo.append(byIdColumnsMap.first->GetName())
                        .append(L": ")
                        .append(byIdColumnsMap.first->GetValue(rightDataRow));
                    }
                for (const auto& [srcCol, outCol] : byCatColsMap)
                    {
                    currentKeyInfo.append(L", ")
                        .append(srcCol->GetName())
                        .append(L": ")
                        .append(srcCol->GetValueAsLabel(rightDataRow));
                    }
                if (currentKeyInfo.starts_with(L", "))
                    {
                    currentKeyInfo.erase(0, 2);
                    }
                for (const auto mergeRow : matchIter->second)
                    {
                    // check if this merge row already has data from a prior match
                    bool alreadyFilled{ false };
                    if (outIdColumnsMap.first != nullptr && outIdColumnsMap.second != nullptr &&
                        !outIdColumnsMap.second->GetValue(mergeRow).empty())
                        {
                        alreadyFilled = true;
                        }
                    if (!alreadyFilled)
                        {
                        for (const auto& [srcCol, outCol] : outContinuousColsMap)
                            {
                            if (!outCol->IsMissingData(mergeRow))
                                {
                                alreadyFilled = true;
                                break;
                                }
                            }
                        }
                    if (!alreadyFilled)
                        {
                        for (const auto& [srcCol, outCol] : outCatColsMap)
                            {
                            auto mdCode = outCol->FindMissingDataCode();
                            if (mdCode.has_value() && outCol->GetValue(mergeRow) != mdCode.value())
                                {
                                alreadyFilled = true;
                                break;
                                }
                            }
                        }
                    if (!alreadyFilled)
                        {
                        for (const auto& [srcCol, outCol] : outDateColsMap)
                            {
                            if (!outCol->IsMissingData(mergeRow))
                                {
                                alreadyFilled = true;
                                break;
                                }
                            }
                        }

                    if (alreadyFilled)
                        {
                        wxLogWarning(L"'%s': duplicate matching row from right dataset when "
                                     "performing left join. "
                                     "First occurrence of matching row will be used.",
                                     currentKeyInfo);
                        continue;
                        }

                    // first match — copy data
                    if (outIdColumnsMap.first != nullptr && outIdColumnsMap.second != nullptr)
                        {
                        outIdColumnsMap.second->SetValue(
                            mergeRow, outIdColumnsMap.first->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outCatColsMap)
                        {
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outContinuousColsMap)
                        {
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    for (const auto& [srcCol, outCol] : outDateColsMap)
                        {
                        outCol->SetValue(mergeRow, srcCol->GetValue(rightDataRow));
                        }
                    }
                }
            }

        return mergedData;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset>
    DatasetLeftJoin::LeftJoin(const std::shared_ptr<const Dataset>& leftDataset,
                              const std::shared_ptr<const Dataset>& rightDataset,
                              const std::vector<std::pair<wxString, wxString>>& byColumns,
                              const wxString& suffix /*= L".x"*/)
        {
        auto ctx = PrepareLeftJoin(leftDataset, rightDataset, byColumns, suffix);
        auto& [mergedData, byIdColumnsMap, byCatColsMap, outIdColumnsMap, outCatColsMap,
               outContinuousColsMap, outDateColsMap, rightDs] = ctx;

        // first pass: collect matching right rows for each left row using hash-based lookup
        //--------------------------
        const size_t originalRowCount = mergedData->GetRowCount();
        std::vector<std::vector<size_t>> matchesPerLeftRow(originalRowCount);

        // build hash index of right dataset rows by join key
        std::unordered_map<std::wstring, std::vector<size_t>> rightKeyIndex;
        rightKeyIndex.reserve(rightDs->GetRowCount());
        for (size_t rightDataRow = 0; rightDataRow < rightDs->GetRowCount(); ++rightDataRow)
            {
            std::wstring rightKey;
            if (byIdColumnsMap.first != nullptr)
                {
                rightKey = byIdColumnsMap.first->GetValue(rightDataRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!rightKey.empty())
                    {
                    rightKey += L'\x1F';
                    }
                rightKey += srcCol->GetValueAsLabel(rightDataRow).Lower().ToStdWstring();
                }
            rightKeyIndex[rightKey].push_back(rightDataRow);
            }

        // for each left row, find matching right rows via hash lookup
        for (size_t leftRow = 0; leftRow < originalRowCount; ++leftRow)
            {
            std::wstring leftKey;
            if (byIdColumnsMap.second != nullptr)
                {
                leftKey = byIdColumnsMap.second->GetValue(leftRow).Lower().ToStdWstring();
                }
            for (const auto& [srcCol, outCol] : byCatColsMap)
                {
                if (!leftKey.empty())
                    {
                    leftKey += L'\x1F';
                    }
                leftKey += outCol->GetValueAsLabel(leftRow).Lower().ToStdWstring();
                }
            if (const auto matchIter = rightKeyIndex.find(leftKey);
                matchIter != rightKeyIndex.cend())
                {
                matchesPerLeftRow[leftRow] = matchIter->second;
                }
            }

        // calculate the output row positions for each left row
        //--------------------------
        std::vector<size_t> outputStartPos(originalRowCount);
        size_t totalRows{ 0 };
        for (size_t leftRow = 0; leftRow < originalRowCount; ++leftRow)
            {
            outputStartPos[leftRow] = totalRows;
            totalRows += std::max<size_t>(1, matchesPerLeftRow[leftRow].size());
            }

        if (totalRows > originalRowCount)
            {
            mergedData->Resize(totalRows);
            }

        // helper to copy all column data within the merged dataset from one row to another
        const auto copyMergedRow = [&mergedData](size_t fromRow, size_t toRow)
        {
            mergedData->GetIdColumn().SetValue(toRow, mergedData->GetIdColumn().GetValue(fromRow));
            for (auto& col : mergedData->GetCategoricalColumns())
                {
                col.SetValue(toRow, col.GetValue(fromRow));
                }
            for (auto& col : mergedData->GetContinuousColumns())
                {
                col.SetValue(toRow, col.GetValue(fromRow));
                }
            for (auto& col : mergedData->GetDateColumns())
                {
                col.SetValue(toRow, col.GetValue(fromRow));
                }
        };

        // helper to copy right dataset columns into a merged row
        const auto fillRightData = [&](size_t rightDataRow, size_t outRow)
        {
            if (outIdColumnsMap.first != nullptr && outIdColumnsMap.second != nullptr)
                {
                outIdColumnsMap.second->SetValue(outRow,
                                                 outIdColumnsMap.first->GetValue(rightDataRow));
                }
            for (const auto& [srcCol, outCol] : outCatColsMap)
                {
                outCol->SetValue(outRow, srcCol->GetValue(rightDataRow));
                }
            for (const auto& [srcCol, outCol] : outContinuousColsMap)
                {
                outCol->SetValue(outRow, srcCol->GetValue(rightDataRow));
                }
            for (const auto& [srcCol, outCol] : outDateColsMap)
                {
                outCol->SetValue(outRow, srcCol->GetValue(rightDataRow));
                }
        };

        // second pass: fill data, working backwards to avoid overwriting
        // unprocessed left rows
        //--------------------------
        for (size_t i = originalRowCount; i > 0; --i)
            {
            const size_t leftRow = i - 1;
            const size_t outStart = outputStartPos[leftRow];
            const auto& matches = matchesPerLeftRow[leftRow];
            const size_t numOutRows = std::max<size_t>(1, matches.size());

            for (size_t m = 0; m < numOutRows; ++m)
                {
                const size_t outRow = outStart + m;
                // copy left data to the output row if it differs from the source
                if (outRow != leftRow)
                    {
                    copyMergedRow(leftRow, outRow);
                    }
                // fill right data if there is a match
                if (m < matches.size())
                    {
                    fillRightData(matches[m], outRow);
                    }
                }
            }

        return mergedData;
        }

    } // namespace Wisteria::Data
