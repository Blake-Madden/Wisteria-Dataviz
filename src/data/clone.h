/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_CLONE_H__
#define __WISTERIA_CLONE_H__

#include "dataset.h"

namespace Wisteria::Data
    {
    /// @brief Interface for cloning a dataset.
    /// @note This is a base class for subsetting operations and is not recommended
    ///     for client code. If you are only needing to copy a full dataset,
    ///     then use a deep copy construction.
    class DatasetClone
        {
    public:
        /** @brief Sets the source dataset to clone from.
            @param fromDataset The dataset to clone.
            @warning The source dataset should not be altered after calling this.
                Pointers to its internal structure (e.g., columns) are constructed
                here and it is assumed that a cloning will take place prior to any
                changes to the dataset being cloned.*/
        void SetSourceData(const std::shared_ptr<const Dataset>& fromDataset);
        /// @brief Creates a clone of the original dataset.
        /// @returns A cloned copy of the original dataset, or null if SetSourceData()
        ///     hasn't been called.
        [[nodiscard]] std::shared_ptr<Dataset> Clone();
    protected:
        /// @returns @c true if there are more rows that can be copied or skipped.
        [[nodiscard]] bool HasMoreRows() const noexcept
            { return m_currentSrcRow < m_fromDataset->GetRowCount(); }
        /// @brief Skip the next row in the source file, not copying it into the destination.
        void SkipNextRow() noexcept
            { ++m_currentSrcRow; }
        /// @brief Copies the next row from the source dataset into the destination.
        void CopyNextRow();
    private:
        /// @brief Builds a map of pointers to the corresponding columns between the datasets.
        void MapColumns();

        std::shared_ptr<const Dataset> m_fromDataset;
        std::shared_ptr<Dataset> m_toDataset;

        size_t m_currentSrcRow{ 0 };

        // column mappings between datasets
        std::map<const Column<wxString>*, Column<wxString>*> m_idColumnsMap;
        std::map<const Column<wxDateTime>*, Column<wxDateTime>*> m_dateColumnsMap;
        std::map<const ColumnWithStringTable*, ColumnWithStringTable*> m_catColumnsMap;
        std::map<const Column<double>*, Column<double>*> m_continuousColumnsMap;
        };
    }

/** @}*/

#endif //__WISTERIA_CLONE_H__
