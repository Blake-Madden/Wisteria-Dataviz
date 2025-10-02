/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_JOIN_H
#define WISTERIA_JOIN_H

#include "dataset.h"

namespace Wisteria::Data
    {
    /// @brief Interface for joining datasets.
    class DatasetJoin
        {
      public:
        /// @private
        DatasetJoin() = default;
        /// @private
        DatasetJoin(const DatasetJoin&) = delete;
        /// @private
        DatasetJoin& operator=(const DatasetJoin&) = delete;
        /// @private
        virtual ~DatasetJoin() = default;
        /** @brief Left joins one dataset with another.\n
                In the case of duplicate keys from the right dataset, only the last
                instance's data will be included in the results. This is useful preventing
                duplicate keys in the right dataset from causing duplicate rows in the results.
            @param leftDataset The left dataset which will be joined with. All rows and columns
                from this dataset will be preserved in the output.
            @param rightDataset The dataset being joined with the left one. All columns except for
                the 'by' (i.e., matching ID) columns will be added to the output.\n
                Only data that matches against the left dataset will be imported; in other words,
                ID rows in this dataset that don't appear in the left dataset will be ignored.
                Also, when duplicate ID columns appear in this dataset, only the last occurrence
                will be imported.
            @param byColumns Pairs of columns to join by between the two datasets. Columns can be
                the ID columns from the datasets, as well as categorical columns.
            @param suffix In the case of a (non-joining) column from the right dataset already
                having a column with the same name in the left file, @c suffix will be added to
                the column when being copied to make it unique.
            @returns The joined dataset.
            @note This method differs from how left joins usually work in that if there are
                multiple rows in the right dataset with the same key, only the data from the
                last instance will be used.\n
                Normally, left joins will create empty rows in
                the results for these duplicate rows. This will result in the final dataset
                having more rows than the original left dataset, adding what could be considered
                duplicate rows.\n
                This function instead will not add additional rows compared to the left dataset,
                but instead merge data from the last row from any duplicate keys from the
                right dataset.\n
                Also, if duplicate keys are encountered in the right dataset, a warning will
                be issued via @c wxLogWarning().
            @throws std::runtime_error If invalid columns or dataset are provided,
                throws an exception.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        LeftJoinUnique(const std::shared_ptr<const Dataset>& leftDataset,
                       const std::shared_ptr<const Dataset>& rightDataset,
                       const std::vector<std::pair<wxString, wxString>>& byColumns,
                       const wxString& suffix = L".x");
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_JOIN_H
