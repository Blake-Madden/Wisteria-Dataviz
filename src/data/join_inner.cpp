///////////////////////////////////////////////////////////////////////////////
// Name:        join_inner.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "join_inner.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    std::shared_ptr<Dataset>
    DatasetInnerJoin::InnerJoin(const std::shared_ptr<const Dataset>& leftDataset,
                                const std::shared_ptr<const Dataset>& rightDataset,
                                const std::vector<std::pair<wxString, wxString>>& byColumns,
                                const wxString& suffix /*= L".x"*/)
        {
        if (leftDataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid left dataset when inner joining.").ToUTF8());
            }
        if (rightDataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid right dataset when inner joining.").ToUTF8());
            }
        if (byColumns.empty())
            {
            throw std::runtime_error(
                _(L"No comparison columns were provided when inner joining.").ToUTF8());
            }
        if (suffix.empty())
            {
            throw std::runtime_error(_(L"Suffix should not be empty when inner joining.").ToUTF8());
            }

        // start with a copy of the left dataset to get its structure
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
                throw std::runtime_error(_(L"Empty 'by' column when inner joining.").ToUTF8());
                }
            if (!mergedData->ContainsColumn(byColumn.first))
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': column not found in left dataset when inner joining."),
                        byColumn.first)
                        .ToUTF8());
                }
            if (!rightDataset->ContainsColumn(byColumn.second))
                {
                throw std::runtime_error(
                    wxString::Format(
                        _(L"'%s': column not found in right dataset when inner joining."),
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
        // if both datasets have ID columns and you are not joining by them,
        // then the one from the right will not be copied over since a dataset
        // only has one ID column
        if (rightDataset->HasValidIdData() && mergedData->HasValidIdData() &&
            !rightColumnsToJoinBy.contains(rightDataset->GetIdColumn().GetName()))
            {
            wxLogWarning(L"'%s': ID column from right dataset will not be copied "
                         "while inner joining.",
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
                        wxString::Format(_(L"Inner joining by ID columns, but '%s' is not "
                                           "the ID column in the right dataset."),
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
                        wxString::Format(_(L"'%s': categorical column not found in right "
                                           "dataset when inner joining. "
                                           "'By' columns must be either ID or categorical "
                                           "columns."),
                                         byColumn.second)
                            .ToUTF8());
                    }
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': categorical column not found in left "
                                       "dataset when inner joining. "
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
                    wxString::Format(_(L"'%s': internal error finding right dataset "
                                       "column when inner joining."),
                                     srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetCategoricalColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != mergedData->GetCategoricalColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': internal error finding merge dataset "
                                       "column when inner joining."),
                                     outCol)
                        .ToUTF8());
                }
            outCatColsMap.emplace_back(rCol, mCol);
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
                    wxString::Format(_(L"'%s': internal error finding right dataset "
                                       "column when inner joining."),
                                     srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetContinuousColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != mergedData->GetContinuousColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': internal error finding merge dataset "
                                       "column when inner joining."),
                                     outCol)
                        .ToUTF8());
                }
            outContinuousColsMap.emplace_back(rCol, mCol);
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
                    wxString::Format(_(L"'%s': internal error finding right dataset "
                                       "column when inner joining."),
                                     srcCol)
                        .ToUTF8());
                }
            const auto mCol = mergedData->GetDateColumn(outCol);
            // cppcheck-suppress assertWithSideEffect
            wxASSERT_MSG(mCol != mergedData->GetDateColumns().cend(),
                         L"Error getting mapped right dataset column!");
            if (mCol == mergedData->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': internal error finding merge dataset "
                                       "column when inner joining."),
                                     outCol)
                        .ToUTF8());
                }
            outDateColsMap.emplace_back(rCol, mCol);
            }

        // first pass: collect all matching (leftRow, rightRow) pairs
        //--------------------------
        const size_t originalRowCount = mergedData->GetRowCount();
        std::vector<std::pair<size_t, size_t>> matchPairs;

        for (size_t leftRow = 0; leftRow < originalRowCount; ++leftRow)
            {
            for (size_t rightDataRow = 0; rightDataRow < rightDataset->GetRowCount();
                 ++rightDataRow)
                {
                // matching on ID columns
                if (byIdColumnsMap.first != nullptr && byIdColumnsMap.second != nullptr &&
                    byIdColumnsMap.first->GetValue(rightDataRow)
                            .CmpNoCase(byIdColumnsMap.second->GetValue(leftRow)) != 0)
                    {
                    continue;
                    }
                bool allKeysMatch{ true };
                for (const auto& [srcCol, outCol] : byCatColsMap)
                    {
                    if (srcCol->GetValueAsLabel(rightDataRow)
                            .CmpNoCase(outCol->GetValueAsLabel(leftRow)) != 0)
                        {
                        allKeysMatch = false;
                        break;
                        }
                    }
                if (allKeysMatch)
                    {
                    matchPairs.emplace_back(leftRow, rightDataRow);
                    }
                }
            }

        // second pass: resize output and fill matched rows
        //--------------------------
        mergedData->Resize(matchPairs.size());

        for (size_t outRow = 0; outRow < matchPairs.size(); ++outRow)
            {
            const auto [leftRow, rightDataRow] = matchPairs[outRow];

            // copy left data from the original left dataset
            mergedData->GetIdColumn().SetValue(outRow,
                                               leftDataset->GetIdColumn().GetValue(leftRow));
            for (size_t col = 0; col < leftDataset->GetCategoricalColumns().size(); ++col)
                {
                mergedData->GetCategoricalColumns().at(col).SetValue(
                    outRow, leftDataset->GetCategoricalColumns().at(col).GetValue(leftRow));
                }
            for (size_t col = 0; col < leftDataset->GetContinuousColumns().size(); ++col)
                {
                mergedData->GetContinuousColumns().at(col).SetValue(
                    outRow, leftDataset->GetContinuousColumns().at(col).GetValue(leftRow));
                }
            for (size_t col = 0; col < leftDataset->GetDateColumns().size(); ++col)
                {
                mergedData->GetDateColumns().at(col).SetValue(
                    outRow, leftDataset->GetDateColumns().at(col).GetValue(leftRow));
                }

            // copy right data
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
            }

        return mergedData;
        }

    } // namespace Wisteria::Data
