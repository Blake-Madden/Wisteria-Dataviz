///////////////////////////////////////////////////////////////////////////////
// Name:        pivot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pivot.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    void PivotedWiderRow::Combine(const PivotedWiderRow& that)
        {
        assert(m_id.CmpNoCase(that.m_id) == 0 && L"Combining pivot rows with mismatching IDs!");
        if (m_id.CmpNoCase(that.m_id) != 0)
            {
            return;
            }

        for (const auto& pivotsToAdd : that.m_pivotedColumns)
            {
            if (auto [iter, inserted] = m_pivotedColumns.insert(pivotsToAdd); !inserted)
                {
                iter->second += pivotsToAdd.second;
                }
            }
        }

    //---------------------------------------------------
    void
    PivotedWiderRow::Expand(const std::set<wxString, wxStringLessNoCase>& pivotedColumnNames,
                            const double fillValue /*= std::numeric_limits<double>::quiet_NaN()*/)
        {
        for (const auto& pivotedColumnName : pivotedColumnNames)
            {
            m_pivotedColumns.insert(std::make_pair(pivotedColumnName, fillValue));
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Pivot::PivotWider(
        const std::shared_ptr<const Dataset>& dataset, const std::vector<wxString>& IdColumns,
        const wxString& namesFromColumn, const std::vector<wxString>& valuesFromColumns,
        const wxString& namesSep /*= L"_"*/, const wxString& namesPrefix /*= wxEmptyString*/,
        const double fillValue /*= std::numeric_limits<double>::quiet_NaN()*/)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid dataset being pivoted.").ToUTF8());
            }
        if (IdColumns.empty())
            {
            throw std::runtime_error(
                _(L"At least one identifier column must be specified.").ToUTF8());
            }

        std::set<PivotedWiderRow> pivotedRows;

        struct IdColumnsInfo
            {
            const Column<wxString>* m_IdColumn{ nullptr };
            std::vector<CategoricalColumnConstIterator> m_catColumns;
            };

        IdColumnsInfo idColumnsIters;
        CategoricalColumnConstIterator namesFromColumnsIter;
        std::vector<ContinuousColumnConstIterator> valuesFromColumnsIters;

        // load the ID columns
        for (const auto& idCol : IdColumns)
            {
            if (dataset->GetIdColumn().GetName().CmpNoCase(idCol) == 0)
                {
                idColumnsIters.m_IdColumn = &dataset->GetIdColumn();
                }
            else
                {
                auto currentColumn = dataset->GetCategoricalColumn(idCol);
                if (currentColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': ID column not found when pivoting."), idCol)
                            .ToUTF8());
                    }
                idColumnsIters.m_catColumns.push_back(currentColumn);
                }
            }
            // load the "names from" column
            {
            auto currentColumn = dataset->GetCategoricalColumn(namesFromColumn);
            if (currentColumn == dataset->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': 'names from' column not found when pivoting."),
                                     namesFromColumn)
                        .ToUTF8());
                }
            namesFromColumnsIter = currentColumn;
            }
        // load the "values from" columns
        for (const auto& valuesFrom : valuesFromColumns)
            {
            auto currentColumn = dataset->GetContinuousColumn(valuesFrom);
            if (currentColumn == dataset->GetContinuousColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': 'values from' column not found when pivoting."),
                                     valuesFrom)
                        .ToUTF8());
                }
            valuesFromColumnsIters.push_back(currentColumn);
            }

        wxString currentKey;
        std::vector<std::pair<wxString, CategoricalOrIdDataType>> idColumns;
        std::map<wxString, double, wxStringLessNoCase> pivotedColumns;
        std::set<wxString, wxStringLessNoCase> pivotedColumnNames;
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            currentKey.clear();
            idColumns.clear();
            pivotedColumns.clear();
            // build the ID by combining the ID rows into one string,
            // and build the ID columns
            if (idColumnsIters.m_IdColumn != nullptr)
                {
                currentKey = idColumnsIters.m_IdColumn->GetValue(i);
                idColumns.emplace_back(idColumnsIters.m_IdColumn->GetName(),
                                       idColumnsIters.m_IdColumn->GetValue(i));
                }
            for (const auto& catCol : idColumnsIters.m_catColumns)
                {
                if (!currentKey.empty())
                    {
                    currentKey += L'\x1F'; // insert separator
                    }
                currentKey += catCol->GetValueAsLabel(i);
                idColumns.emplace_back(catCol->GetName(),
                                       // ID, not the string, to be optimal
                                       catCol->GetValue(i));
                }

            // build the pivots
            if (!valuesFromColumnsIters.empty())
                {
                const bool includeValueNameInPivotName = (valuesFromColumnsIters.size() > 1);
                for (const auto& valuesFrom : valuesFromColumnsIters)
                    {
                    const wxString colName =
                        namesPrefix + (includeValueNameInPivotName ?
                                           valuesFrom->GetName() + namesSep +
                                               namesFromColumnsIter->GetValueAsLabel(i) :
                                           namesFromColumnsIter->GetValueAsLabel(i));
                    pivotedColumns.insert(std::make_pair(colName, valuesFrom->GetValue(i)));
                    pivotedColumnNames.insert(colName);
                    }
                }
            // if no value columns to read from, just use frequency counts of each time
            // a label from the ID column(s) appears
            else
                {
                const wxString colName = namesPrefix + namesFromColumnsIter->GetValueAsLabel(i);
                pivotedColumns.insert(std::make_pair(colName, 1));
                pivotedColumnNames.insert(colName);
                }

            const PivotedWiderRow pivotRow{ currentKey, idColumns, pivotedColumns };
            const auto [iter, inserted] = pivotedRows.insert(pivotRow);
            // observation has already been loaded, so add new 'names from' labels
            // as we pivot columns to it
            if (!inserted)
                {
                auto nodeHandle = pivotedRows.extract(iter);
                nodeHandle.value().Combine(pivotRow);
                pivotedRows.insert(std::move(nodeHandle));
                }
            }

        // in case there was a label not present in a 'names from' column
        // for an observation, then add a pivot column for that (filled with MD)
        for (auto pivotedRowsIter = pivotedRows.begin(); pivotedRowsIter != pivotedRows.end();
             /* in loop */)
            {
            if (pivotedRowsIter->m_pivotedColumns.size() < pivotedColumnNames.size())
                {
                auto nextRow = std::next(pivotedRowsIter);

                auto nodeHandle = pivotedRows.extract(pivotedRowsIter);
                nodeHandle.value().Expand(pivotedColumnNames, fillValue);
                pivotedRows.insert(std::move(nodeHandle));

                pivotedRowsIter = nextRow;
                }
            else
                {
                ++pivotedRowsIter;
                }
            }

        // copy pivoted data to a new dataset
        auto pivotedData = std::make_shared<Dataset>();
        // copy ID column info from original dataset into pivot
        if (idColumnsIters.m_IdColumn != nullptr)
            {
            pivotedData->GetIdColumn().SetName(idColumnsIters.m_IdColumn->GetName());
            }
        for (const auto& catCol : idColumnsIters.m_catColumns)
            {
            pivotedData->AddCategoricalColumn(catCol->GetName(), catCol->GetStringTable());
            }
        // add the pivoted columns
        for (const auto& pivotedColumnName : pivotedColumnNames)
            {
            pivotedData->AddContinuousColumn(pivotedColumnName);
            }

        // write out the data
        for (const auto& pivotedRow : pivotedRows)
            {
            RowInfo row_info;
            size_t currentIdColumnIndex{ 0 };
            if (idColumnsIters.m_IdColumn != nullptr)
                {
                const auto* strVal = std::get_if<wxString>(&pivotedRow.m_idColumns[0].second);
                assert(strVal && L"String conversion failure with ID column while pivoting!");
                if (strVal != nullptr)
                    {
                    row_info.Id(*strVal);
                    }
                ++currentIdColumnIndex;
                }
            // fill in reset of IDs
            std::vector<GroupIdType> groupIdsForCurrentRow;
            for (/* initialized earlier, may be 1 now */;
                 currentIdColumnIndex < pivotedRow.m_idColumns.size(); ++currentIdColumnIndex)
                {
                const auto* groupId =
                    std::get_if<GroupIdType>(&pivotedRow.m_idColumns[currentIdColumnIndex].second);
                assert(groupId && L"Group ID conversion failure with ID column while pivoting!");
                if (groupId != nullptr)
                    {
                    groupIdsForCurrentRow.push_back(*groupId);
                    }
                }
            if (!groupIdsForCurrentRow.empty())
                {
                row_info.Categoricals(groupIdsForCurrentRow);
                }
            // fill in pivots
            std::vector<double> valuesForCurrentRow;
            valuesForCurrentRow.reserve(pivotedRow.m_pivotedColumns.size());
            for (const auto& pivotedColumn : pivotedRow.m_pivotedColumns)
                {
                valuesForCurrentRow.push_back(pivotedColumn.second);
                }
            if (!valuesForCurrentRow.empty())
                {
                row_info.Continuous(valuesForCurrentRow);
                }

            // add everything now
            pivotedData->AddRow(row_info);
            }

        return pivotedData;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Pivot::PivotLonger(
        const std::shared_ptr<const Dataset>& dataset, const std::vector<wxString>& columnsToKeep,
        const std::vector<wxString>& fromColumns, const std::vector<wxString>& namesTo,
        const wxString& valuesTo, [[maybe_unused]] const wxString& namesPattern /*= wxEmptyString*/)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(_(L"Invalid dataset being pivoted.").ToUTF8());
            }
        if (columnsToKeep.empty())
            {
            throw std::runtime_error(
                _(L"At least one column must be kept (e.g., an ID column).").ToUTF8());
            }
        if (fromColumns.empty())
            {
            throw std::runtime_error(_(L"'From' column is required to pivot dataset.").ToUTF8());
            }
        if (namesTo.empty())
            {
            throw std::runtime_error(
                _(L"'Names to' column is required to pivot dataset.").ToUTF8());
            }
        if (namesTo.size() > 1 && namesPattern.empty())
            {
            throw std::runtime_error(
                _(L"Multiple 'names to' columns were specified, but no names pattern "
                  "was provided to split the column names.")
                    .ToUTF8());
            }
        if (valuesTo.empty())
            {
            throw std::runtime_error(_(L"'Value to' column required to pivot dataset.").ToUTF8());
            }

        // build string tables from the "from" column names
        std::vector<ColumnWithStringTable::StringTableType> namesFromStringTables(namesTo.size());
        // prefill string tables with missing data for all possible labels (from the "from" columns)
        for (auto& nST : namesFromStringTables)
            {
            for (size_t i = 0; i < fromColumns.size(); ++i)
                {
                nST.insert(std::make_pair(i, wxEmptyString));
                }
            }
        const wxRegEx namesSplit(!namesPattern.empty() ? namesPattern : wxString(L"(.*)"));
        for (size_t i = 0; i < fromColumns.size(); ++i)
            {
            if (namesSplit.Matches(fromColumns[i]) && namesSplit.GetMatchCount() > 0)
                {
                for (size_t j = 0;
                     j < std::min(namesSplit.GetMatchCount() - 1, namesFromStringTables.size());
                     ++j)
                    {
                    namesFromStringTables[j][i] = namesSplit.GetMatch(fromColumns[i], j + 1);
                    }
                }
            }

        auto pivotedData = std::make_shared<Dataset>();
        pivotedData->GetContinuousColumns().reserve(dataset->GetCategoricalColumns().size());
        pivotedData->GetCategoricalColumns().reserve(dataset->GetCategoricalColumns().size());
        pivotedData->GetDateColumns().reserve(dataset->GetDateColumns().size());

        // map of columns between original dataset and pivoted one (of what's being kept)
        std::vector<std::pair<ColumnConstIterator, ColumnIterator>> columnsToKeepMap;
        // the pivot columns, where the data and label(s) comes from
        std::vector<ColumnConstIterator> fromNamesList;
        // where names go to as labels
        std::vector<CategoricalColumnIterator> toNamesList;

        // find and add the columns being kept, then map the columns between the datasets
        for (const auto& columnToKeep : columnsToKeep)
            {
            if (dataset->GetIdColumn().GetName().CmpNoCase(columnToKeep) == 0)
                {
                pivotedData->GetIdColumn().SetName(columnToKeep);
                columnsToKeepMap.emplace_back(&dataset->GetIdColumn(), &pivotedData->GetIdColumn());
                }
            else if (const auto foundCatVar = dataset->GetCategoricalColumn(columnToKeep);
                     foundCatVar != dataset->GetCategoricalColumns().cend())
                {
                pivotedData->AddCategoricalColumn(columnToKeep, foundCatVar->GetStringTable());
                columnsToKeepMap.emplace_back(foundCatVar, nullptr);
                }
            else if (const auto foundContinuousVar = dataset->GetContinuousColumn(columnToKeep);
                     foundContinuousVar != dataset->GetContinuousColumns().cend())
                {
                pivotedData->AddContinuousColumn(columnToKeep);
                columnsToKeepMap.emplace_back(foundContinuousVar, nullptr);
                }
            else if (const auto foundDateVar = dataset->GetDateColumn(columnToKeep);
                     foundDateVar != dataset->GetDateColumns().cend())
                {
                pivotedData->AddDateColumn(columnToKeep);
                columnsToKeepMap.emplace_back(foundDateVar, nullptr);
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: column not found."), columnToKeep).ToUTF8());
                }
            }
        // find and catalog "from" columns
        for (const auto& fromColumn : fromColumns)
            {

            if (const auto foundVar = dataset->GetContinuousColumn(fromColumn);
                foundVar != dataset->GetContinuousColumns().cend())
                {
                fromNamesList.emplace_back(foundVar);
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: continuous column not found."), fromColumn).ToUTF8());
                }
            }
        // add target column(s) for the pivoted column name (which will become group labels)
        for (const auto& nameTo : namesTo)
            {
            if (nameTo.empty())
                {
                throw std::runtime_error(_(L"'Names to' column name cannot be empty.").ToUTF8());
                }
            pivotedData->AddCategoricalColumn(nameTo);
            }
        // add target column for continuous values
        if (valuesTo.empty())
            {
            throw std::runtime_error(_(L"'Values to' column name cannot be empty.").ToUTF8());
            }
        pivotedData->AddContinuousColumn(valuesTo);
        auto valueToTarget = pivotedData->GetContinuousColumn(valuesTo);

        pivotedData->Resize(fromColumns.size() * dataset->GetRowCount());

        // map the columns to keep
        for (size_t i = 0; i < columnsToKeep.size(); ++i)
            {
            auto foundCol = pivotedData->FindColumn(columnsToKeep[i]);
            if (!foundCol.has_value())
                {
                throw std::runtime_error(_(L"Internal error building target column map.").ToUTF8());
                }
            columnsToKeepMap[i].second = foundCol.value();
            }
        // map the target name column(s)
        for (size_t i = 0; i < namesTo.size(); ++i)
            {
            auto foundCol = pivotedData->GetCategoricalColumn(namesTo[i]);
            if (foundCol == pivotedData->GetCategoricalColumns().end())
                {
                throw std::runtime_error(_(L"Internal error building target column map.").ToUTF8());
                }
            foundCol->SetStringTable(namesFromStringTables[i]);
            toNamesList.push_back(foundCol);
            }

        // go through each observation
        size_t pivotDataRow{ 0 };
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            // ...and pivot its "from" columns
            for (const auto& fromName : fromNamesList)
                {
                // fill in the kept columns (usually IDs columns)
                for (const auto& keepCol : columnsToKeepMap)
                    {
                        // ID columns
                        {
                        const auto* srcCol = std::get_if<const Column<wxString>*>(&keepCol.first);
                        const auto* targetCol = std::get_if<Column<wxString>*>(&keepCol.second);
                        if (srcCol != nullptr && targetCol != nullptr)
                            {
                            (*targetCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                        // categorical
                        {
                        const auto* srcCol =
                            std::get_if<CategoricalColumnConstIterator>(&keepCol.first);
                        const auto* targetCol =
                            std::get_if<CategoricalColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetCol != nullptr)
                            {
                            (*targetCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                        // continuous
                        {
                        const auto* srcCol =
                            std::get_if<ContinuousColumnConstIterator>(&keepCol.first);
                        const auto* targetCol =
                            std::get_if<ContinuousColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetCol != nullptr)
                            {
                            (*targetCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                        // date
                        {
                        const auto* srcCol = std::get_if<DateColumnConstIterator>(&keepCol.first);
                        const auto* targetCol = std::get_if<DateColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetCol != nullptr)
                            {
                            (*targetCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                    throw std::runtime_error(_(L"Internal error mapping from columns.").ToUTF8());
                    }
                // fill in the name column(s)
                for (auto& toName : toNamesList)
                    {
                    toName->SetValue(pivotDataRow, pivotDataRow % fromColumns.size());
                    }
                    // fill in the value column
                    // if continuous
                    {
                    const auto* srcCol = std::get_if<ContinuousColumnConstIterator>(&fromName);
                    if (srcCol != nullptr)
                        {
                        valueToTarget->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                        }
                    }
                ++pivotDataRow;
                }
            }

        return pivotedData;
        }
    } // namespace Wisteria::Data
