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
            const double fillValue /*= std::numeric_limits<double>::quiet_NaN()*/)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                _(L"Invalid dataset being pivoted.").ToUTF8());
            }
        else if (IdColumns.size() == 0)
            {
            throw std::runtime_error(
                _(L"ID column(s) required to pivot dataset.").ToUTF8());
            }
        else if (valuesFromColumns.size() == 0)
            {
            throw std::runtime_error(
                _(L"'Values from' column(s) required to pivot dataset.").ToUTF8());
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
            const bool includeValueNameInPivotName = (valuesFromColumnsIters.size() > 1);
            for (const auto& valuesFrom : valuesFromColumnsIters)
                {
                const wxString colName = includeValueNameInPivotName ?
                    valuesFrom->GetName() + namesSep + namesFromColumnsIter->GetValueAsLabel(i):
                    namesFromColumnsIter->GetValueAsLabel(i);
                pivotedColumns.insert(std::make_pair(colName, valuesFrom->GetValue(i)));
                pivotedColumnNames.insert(colName);
                }

            const PivotedWiderRow pr{ currentKey, idColumns, pivotedColumns };
            const auto [iter, inserted] = pivotedRows.insert(pr);
            // observation has already been load, so add new 'names from' labels
            // as pivot columns to it
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
    }
