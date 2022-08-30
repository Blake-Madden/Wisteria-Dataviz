#include "clone.h"

namespace Wisteria::Data
    {
    //---------------------------------------------------
    std::shared_ptr<Dataset> DatasetClone::Clone()
        {
        // source hasn't been set yet, so return null
        if (m_fromDataset == nullptr)
            { return nullptr; }

        while (HasMoreRows())
            { CopyNextRow(); }

        return m_toDataset;
        }

    //---------------------------------------------------
    void DatasetClone::SetSourceData(const std::shared_ptr<const Dataset>& fromDataset)
        {
        m_fromDataset = fromDataset;
        m_toDataset = std::make_shared<Dataset>();

        m_idColumnsMap.clear();
        m_dateColumnsMap.clear();
        m_catColumnsMap.clear();
        m_continuousColumnsMap.clear();

        m_currentSrcRow = 0;

        // add columns to new dataset (in the same order as the source)
        //---------------

        // ID columns
        m_toDataset->GetIdColumn().SetName(m_fromDataset->GetIdColumn().GetName());
        m_toDataset->GetIdColumn().Reserve(m_fromDataset->GetIdColumn().GetRowCount());

        // date columns
        m_toDataset->GetDateColumns().reserve(
            m_fromDataset->GetDateColumns().size());
        for (const auto& srcColumn : m_fromDataset->GetDateColumns())
            {
            auto& newColumn = m_toDataset->AddDateColumn(srcColumn.GetName());
            newColumn.Reserve(srcColumn.GetRowCount());
            }
        // continous columns
        m_toDataset->GetContinuousColumns().reserve(
            m_fromDataset->GetContinuousColumns().size());
        for (const auto& srcColumn : m_fromDataset->GetContinuousColumns())
            {
            auto& newColumn = m_toDataset->AddContinuousColumn(srcColumn.GetName());
            newColumn.Reserve(srcColumn.GetRowCount());
            }
        // categorical columns
        m_toDataset->GetCategoricalColumns().reserve(
            m_fromDataset->GetCategoricalColumns().size());
        for (const auto& srcColumn : m_fromDataset->GetCategoricalColumns())
            {
            auto& newColumn = m_toDataset->AddCategoricalColumn(srcColumn.GetName());
            newColumn.SetStringTable(srcColumn.GetStringTable());
            newColumn.Reserve(srcColumn.GetRowCount());
            }

        // map the variables between the source and destination datasets
        MapColumns();
        }

    //---------------------------------------------------
    void DatasetClone::MapColumns()
        {
        // ID
        m_idColumnsMap.insert(std::make_pair(&m_fromDataset->GetIdColumn(),
                                             &m_toDataset->GetIdColumn()));

        // continuous
        m_continuousColumnsMap.clear();
        // will only happen if the source was changed after SetSourceData(),
        // which the client should not be doing
        wxASSERT_MSG(m_fromDataset->GetContinuousColumns().size() ==
                     m_toDataset->GetContinuousColumns().size(),
                     L"Continuous column counts are different between datasets!");
        for (size_t i = 0; i < m_fromDataset->GetContinuousColumns().size(); ++i)
            {
            wxASSERT_MSG(m_fromDataset->GetContinuousColumn(i).GetName() ==
                         m_toDataset->GetContinuousColumn(i).GetName(),
                         L"Continuous columns aren't mapped correctly!");
            m_continuousColumnsMap.insert(
                std::make_pair(&m_fromDataset->GetContinuousColumn(i),
                               &m_toDataset->GetContinuousColumn(i)));
            }

        // categoricals
        m_catColumnsMap.clear();
        wxASSERT_MSG(m_fromDataset->GetCategoricalColumns().size() ==
                     m_toDataset->GetCategoricalColumns().size(),
                     L"Categorical column counts are different between datasets!");
        for (size_t i = 0; i < m_fromDataset->GetCategoricalColumns().size(); ++i)
            {
            wxASSERT_MSG(m_fromDataset->GetCategoricalColumn(i).GetName() ==
                         m_toDataset->GetCategoricalColumn(i).GetName(),
                         L"Categorical columns aren't mapped correctly!");
            m_catColumnsMap.insert(
                std::make_pair(&m_fromDataset->GetCategoricalColumn(i),
                               &m_toDataset->GetCategoricalColumn(i)));
            }

        // dates
        m_dateColumnsMap.clear();
        wxASSERT_MSG(m_fromDataset->GetDateColumns().size() ==
                     m_toDataset->GetDateColumns().size(),
                     L"Date column counts are different between datasets!");
        for (size_t i = 0; i < m_fromDataset->GetDateColumns().size(); ++i)
            {
            wxASSERT_MSG(m_fromDataset->GetDateColumn(i).GetName() ==
                         m_toDataset->GetDateColumn(i).GetName(),
                         L"Date columns aren't mapped correctly!");
            m_dateColumnsMap.insert(
                std::make_pair(&m_fromDataset->GetDateColumn(i),
                               &m_toDataset->GetDateColumn(i)));
            }
        }

    //---------------------------------------------------
    void DatasetClone::CopyNextRow()
        {
        if (!HasMoreRows())
            { return; }

        // copy values from source to destination columns
        for (const auto& col : m_idColumnsMap)
            {
            col.second->AddValue(col.first->GetValue(m_currentSrcRow));
            }
        for (const auto& col : m_dateColumnsMap)
            {
            col.second->AddValue(col.first->GetValue(m_currentSrcRow));
            }
        for (const auto& col : m_catColumnsMap)
            {
            col.second->AddValue(col.first->GetValue(m_currentSrcRow));
            }
        for (const auto& col : m_continuousColumnsMap)
            {
            col.second->AddValue(col.first->GetValue(m_currentSrcRow));
            }

        ++m_currentSrcRow;
        }
    }