#include "pivot.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    void PivotedWiderRow::Combine(const PivotedWiderRow& that)
        {
        wxASSERT_MSG(m_Id.CmpNoCase(that.m_Id) == 0,
                     L"Combining pivot rows with mismatching IDs!");
        if (m_Id.CmpNoCase(that.m_Id) != 0)
            { return; }

        for (const auto& pivotsToAdd : that.m_pivotedColumns)
            {
            if (auto [iter, inserted] = m_pivotedColumns.insert(pivotsToAdd);
                !inserted)
                { iter->second += pivotsToAdd.second; }
            }
        }

    //---------------------------------------------------
    void PivotedWiderRow::Expand(const std::set<wxString, StringCmpNoCase>& pivotedColumnNames,
        const double fillValue /*= std::numeric_limits<double>::quiet_NaN()*/)
        {
        for (const auto& pivotedColumnName : pivotedColumnNames)
            {
            m_pivotedColumns.insert(
                std::make_pair(pivotedColumnName, fillValue));
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Pivot::PivotWider(
            const std::shared_ptr<const Dataset>& dataset,
            const std::vector<wxString>& IdColumns,
            const wxString& namesFromColumn,
            const std::vector<wxString>& valuesFromColumns,
            const wxString& namesSep /*= L"_"*/,
            const wxString& namesPrefix /*= wxEmptyString*/,
            const double fillValue /*= std::numeric_limits<double>::quiet_NaN()*/)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                _(L"Invalid dataset being pivoted.").ToUTF8());
            }
        else if (IdColumns.empty())
            {
            throw std::runtime_error(
                _(L"ID column(s) required to pivot dataset.").ToUTF8());
            }

        std::set<PivotedWiderRow> pivotedRows;
        struct IdColumnsInfo
            {
            const Column<wxString>* m_IdColumn{ nullptr };
            std::vector<CategoricalColumnConstIterator> m_catColumns;
            };
        IdColumnsInfo IdColumnsIters;
        CategoricalColumnConstIterator namesFromColumnsIter;
        std::vector<ContinuousColumnConstIterator> valuesFromColumnsIters;

        // load the ID columns
        for (const auto& IdCol : IdColumns)
            {
            if (dataset->GetIdColumn().GetName().CmpNoCase(IdCol) == 0)
                {
                IdColumnsIters.m_IdColumn = &dataset->GetIdColumn();
                }
            else
                {
                auto currentColumn = dataset->GetCategoricalColumn(IdCol);
                if (currentColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"'%s': ID column not found when pivoting."), IdCol).ToUTF8());
                    }
                IdColumnsIters.m_catColumns.push_back(currentColumn);
                }
            }
        // load the "names from" column
            {
            auto currentColumn = dataset->GetCategoricalColumn(namesFromColumn);
            if (currentColumn == dataset->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': 'names from' column not found when pivoting."), namesFromColumn).ToUTF8());
                }
            namesFromColumnsIter = currentColumn;
            }
        // load the "values from" columns
        for (const auto& valuesFrom : valuesFromColumns)
            {
            auto currentColumn = dataset->GetContinuousColumn(valuesFrom);
            if (currentColumn == dataset->GetContinuousColumns().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': 'values from' column not found when pivoting."), valuesFrom).ToUTF8());
                }
            valuesFromColumnsIters.push_back(currentColumn);
            }

        wxString currentKey;
        std::vector<std::pair<wxString, CategoricalOrIdDataType>> idColumns;
        std::map<wxString, double, StringCmpNoCase> pivotedColumns;
        std::set<wxString, StringCmpNoCase> pivotedColumnNames;
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            currentKey.clear();
            idColumns.clear();
            pivotedColumns.clear();
            // build the ID by combining the ID rows into one string,
            // and build the ID columns
            if (IdColumnsIters.m_IdColumn != nullptr)
                {
                currentKey = IdColumnsIters.m_IdColumn->GetValue(i);
                idColumns.emplace_back(
                    std::make_pair(IdColumnsIters.m_IdColumn->GetName(),
                                   IdColumnsIters.m_IdColumn->GetValue(i)));
                }
            for (const auto& catCol : IdColumnsIters.m_catColumns)
                {
                currentKey += catCol->GetValueAsLabel(i);
                idColumns.emplace_back(
                    std::make_pair(catCol->GetName(),
                                   // ID, not the string, to be optimal
                                   catCol->GetValue(i)));
                }

            // build the pivots
            if (valuesFromColumnsIters.size() > 0)
                {
                const bool includeValueNameInPivotName = (valuesFromColumnsIters.size() > 1);
                for (const auto& valuesFrom : valuesFromColumnsIters)
                    {
                    const wxString colName = namesPrefix +
                        (includeValueNameInPivotName ?
                            valuesFrom->GetName() + namesSep + namesFromColumnsIter->GetValueAsLabel(i):
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

            const PivotedWiderRow pr{ currentKey, idColumns, pivotedColumns };
            const auto [iter, inserted] = pivotedRows.insert(pr);
            // observation has already been loaded, so add new 'names from' labels
            // as we pivot columns to it
            if (!inserted)
                {
                auto nh = pivotedRows.extract(iter);
                nh.value().Combine(pr);
                pivotedRows.insert(std::move(nh));
                }
            }

        // in case there was a label not present in a 'names from' column
        // for an observation, then add a pivot column for that (filled with MD)
        for (auto pivotedRowsIter = pivotedRows.begin();
            pivotedRowsIter != pivotedRows.end();
            /* in loop */)
            {
            if (pivotedRowsIter->m_pivotedColumns.size() < pivotedColumnNames.size())
                {
                auto nh = pivotedRows.extract(pivotedRowsIter);
                nh.value().Expand(pivotedColumnNames, fillValue);
                pivotedRows.insert(std::move(nh));
                // go back to the start after updating this row,
                // assuming that we don't know where this was inserted
                pivotedRowsIter = pivotedRows.begin();
                }
            else
                { ++pivotedRowsIter; }
            }

        // copy pivoted data to a new dataset
        auto pivotedData = std::make_shared<Dataset>();
        // copy ID column info from original dataset into pivot
        if (IdColumnsIters.m_IdColumn != nullptr)
            {
            pivotedData->GetIdColumn().SetName(IdColumnsIters.m_IdColumn->GetName());
            }
        for (const auto& catCol : IdColumnsIters.m_catColumns)
            {
            pivotedData->AddCategoricalColumn(catCol->GetName()).
                SetStringTable(catCol->GetStringTable());
            }
        // add the pivoted columns
        for (const auto& pivotedColumnName : pivotedColumnNames)
            { pivotedData->AddContinuousColumn(pivotedColumnName); }

        // write out the data
        for (const auto& pivotedRow : pivotedRows)
            {
            RowInfo ri;
            size_t currentIdColumnIndex{ 0 };
            if (IdColumnsIters.m_IdColumn != nullptr)
                {
                auto strVal = std::get_if<wxString>(&pivotedRow.m_idColumns[0].second);
                wxASSERT_MSG(strVal, L"String conversion failure with ID column while pivoting!");
                if (strVal != nullptr)
                    { ri.Id(*strVal); }
                ++currentIdColumnIndex;
                }
            // fill in reset of IDs
            std::vector<GroupIdType> groupIdsForCurrentRow;
            for (/* initialized earlier, may be 1 now */;
                currentIdColumnIndex < pivotedRow.m_idColumns.size();
                ++currentIdColumnIndex)
                {
                auto groupId = std::get_if<GroupIdType>(
                    &pivotedRow.m_idColumns[currentIdColumnIndex].second);
                wxASSERT_MSG(groupId, L"Group ID conversion failure with ID column while pivoting!");
                if (groupId != nullptr)
                    { groupIdsForCurrentRow.push_back(*groupId); }
                }
            if (groupIdsForCurrentRow.size())
                { ri.Categoricals(groupIdsForCurrentRow); }
            // fill in pivots
            std::vector<double> valuesForCurrentRow;
            for (const auto& pivotedColumn : pivotedRow.m_pivotedColumns)
                { valuesForCurrentRow.push_back(pivotedColumn.second); }
            if (valuesForCurrentRow.size())
                { ri.Continuous(valuesForCurrentRow); }

            // add everything now
            pivotedData->AddRow(ri);
            }

        return pivotedData;
        }

    //---------------------------------------------------
    std::shared_ptr<Dataset> Pivot::PivotLonger(
        const std::shared_ptr<const Dataset>& dataset,
        const std::vector<wxString>& columnsToKeep,
        const std::vector<wxString>& fromColumns,
        const std::vector<wxString>& namesTo,
        const wxString& valuesTo,
        [[maybe_unused]] const wxString& namesPattern /*= wxEmptyString*/)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                _(L"Invalid dataset being pivoted.").ToUTF8());
            }
        else if (columnsToKeep.empty())
            {
            throw std::runtime_error(
                _(L"ID column(s) required to pivot dataset.").ToUTF8());
            }
        else if (fromColumns.empty())
            {
            throw std::runtime_error(
                _(L"'From' column(s) required to pivot dataset.").ToUTF8());
            }
        else if (namesTo.empty())
            {
            throw std::runtime_error(
                _(L"'Names to' column(s) required to pivot dataset.").ToUTF8());
            }
        else if (namesTo.size() > 1 && namesPattern.empty())
            {
            throw std::runtime_error(
                _(L"Multiple 'names to' columns were specified, but no names pattern "
                   "was provided to split the column names.").ToUTF8());
            }
        else if (valuesTo.empty())
            {
            throw std::runtime_error(
                _(L"'Value to' column required to pivot dataset.").ToUTF8());
            }

        // build string tables from the "from" column names
        std::vector<ColumnWithStringTable::StringTableType> namesFromStringTables(namesTo.size());
        // prefill string tables with missing data for all possible labels (from the "from" columns)
        for (auto& nST : namesFromStringTables)
            {
            for (size_t i = 0; i < fromColumns.size(); ++i)
                { nST.insert(std::make_pair(i, wxEmptyString)); }
            }
        wxRegEx namesSplit(namesPattern.length() ? namesPattern : wxString(L"(.*)"));
        for (size_t i = 0; i < fromColumns.size(); ++i)
            {
            if (namesSplit.Matches(fromColumns[i]) && namesSplit.GetMatchCount() > 0)
                {
                for (size_t j = 0;
                     j < std::min(namesSplit.GetMatchCount()-1, namesFromStringTables.size());
                     ++j)
                    {
                    namesFromStringTables[j][i] = namesSplit.GetMatch(fromColumns[i], j+1);
                    }
                }
            }

        auto pivottedData = std::make_shared<Dataset>();
        pivottedData->GetContinuousColumns().reserve(dataset->GetCategoricalColumns().size());
        pivottedData->GetCategoricalColumns().reserve(dataset->GetCategoricalColumns().size());
        pivottedData->GetDateColumns().reserve(dataset->GetDateColumns().size());

        // map of columns between original dataset and pivotted one (of what's being kept)
        std::vector<std::pair<ColumnConstIterator, ColumnIterator>> columnsToKeepMap;
        // the pivot columns, where the data and label(s) comes from
        std::vector<ColumnConstIterator> fromNamesList;
        // where names go to as labels
        std::vector<CategoricalColumnIterator> toNamesList;
        // the one column that the source columns' data are going into
        ColumnIterator valueTo{ nullptr };

        // find and add the columns being kept, then map the columns between the datasets
        for (const auto& columnToKeep : columnsToKeep)
            {
            if (dataset->GetIdColumn().GetName().CmpNoCase(columnToKeep) == 0)
                {
                pivottedData->GetIdColumn().SetName(columnToKeep);
                columnsToKeepMap.push_back(std::make_pair(&dataset->GetIdColumn(),
                                                          &pivottedData->GetIdColumn()));
                }
            else if (const auto foundCatVar = dataset->GetCategoricalColumn(columnToKeep);
                foundCatVar != dataset->GetCategoricalColumns().cend())
                {
                pivottedData->AddCategoricalColumn(columnToKeep).
                    SetStringTable(foundCatVar->GetStringTable());
                columnsToKeepMap.push_back(std::make_pair(foundCatVar, nullptr));
                }
            else if (const auto foundContinousVar = dataset->GetContinuousColumn(columnToKeep);
                foundContinousVar != dataset->GetContinuousColumns().cend())
                {
                pivottedData->AddContinuousColumn(columnToKeep);
                columnsToKeepMap.push_back(std::make_pair(foundContinousVar, nullptr));
                }
            else if (const auto foundDateVar = dataset->GetDateColumn(columnToKeep);
                foundDateVar != dataset->GetDateColumns().cend())
                {
                pivottedData->AddDateColumn(columnToKeep);
                columnsToKeepMap.push_back(std::make_pair(foundDateVar, nullptr));
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format( _(L"%s: column not found."), columnToKeep).ToUTF8());
                }
            }
        // find and catalog "from" columns
        for (const auto& fromColumn : fromColumns)
            {

            if (const auto foundVar = dataset->GetContinuousColumn(fromColumn);
                foundVar != dataset->GetContinuousColumns().cend())
                { fromNamesList.push_back(foundVar); }
            else
                {
                throw std::runtime_error(
                    wxString::Format( _(L"%s: continous column not found."), fromColumn).ToUTF8());
                }
            }
        // add target column(s) for the pivotted column name (which will become group labels)
        for (const auto& nameTo : namesTo)
            {
            if (nameTo.empty())
                {
                throw std::runtime_error(
                    _(L"'Names to' column name cannot be empty.").ToUTF8());
                }
            pivottedData->AddCategoricalColumn(nameTo);
            }
        // add target column for continuous values
        if (valuesTo.empty())
                {
                throw std::runtime_error(
                    _(L"'Values to' column name cannot be empty.").ToUTF8());
                }
        pivottedData->AddContinuousColumn(valuesTo);
        auto valueToTarget = pivottedData->GetContinuousColumnWritable(valuesTo);

        pivottedData->Resize(fromColumns.size() * dataset->GetRowCount());

        // map the columns to keep
        for (size_t i = 0; i < columnsToKeep.size(); ++i)
            {
            auto foundCol = pivottedData->FindColumn(columnsToKeep[i]);
            if (!foundCol.has_value())
                {
                throw std::runtime_error(
                    _(L"Internal error building target column map.").ToUTF8());
                }
            columnsToKeepMap[i].second = foundCol.value();
            }
        // map the target name column(s)
        for (size_t i = 0; i < namesTo.size(); ++i)
            {
            auto foundCol = pivottedData->GetCategoricalColumnWritable(namesTo[i]);
            if (foundCol == pivottedData->GetCategoricalColumns().end())
                {
                throw std::runtime_error(
                    _(L"Internal error building target column map.").ToUTF8());
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
                        auto srcCol = std::get_if<const Column<wxString>*>(&keepCol.first);
                        auto targetlCol = std::get_if<Column<wxString>*>(&keepCol.second);
                        if (srcCol != nullptr && targetlCol != nullptr)
                            {
                            (*targetlCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                    // categorical
                        {
                        auto srcCol = std::get_if<CategoricalColumnConstIterator>(&keepCol.first);
                        auto targetlCol = std::get_if<CategoricalColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetlCol != nullptr)
                            {
                            (*targetlCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                    // continuous
                        {
                        auto srcCol = std::get_if<ContinuousColumnConstIterator>(&keepCol.first);
                        auto targetlCol = std::get_if<ContinuousColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetlCol != nullptr)
                            {
                            (*targetlCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                    // date
                        {
                        auto srcCol = std::get_if<DateColumnConstIterator>(&keepCol.first);
                        auto targetlCol = std::get_if<DateColumnIterator>(&keepCol.second);
                        if (srcCol != nullptr && targetlCol != nullptr)
                            {
                            (*targetlCol)->SetValue(pivotDataRow, (*srcCol)->GetValue(i));
                            continue;
                            }
                        }
                    throw std::runtime_error(
                        _(L"Internal error mapping from columns.").ToUTF8());
                    }
                // fill in the name column(s)
                for (auto& toName : toNamesList)
                    { toName->SetValue(pivotDataRow, pivotDataRow % fromColumns.size()); }
                // fill in the value column
                // if continuous
                    {
                    auto srcCol = std::get_if<ContinuousColumnConstIterator>(&fromName);
                    if (srcCol != nullptr)
                        { valueToTarget->SetValue(pivotDataRow, (*srcCol)->GetValue(i)); }
                    }
                ++pivotDataRow;
                }
            }

        return pivottedData;
        }
    }
