/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_JOIN_INNER_H
#define WISTERIA_JOIN_INNER_H

#include "dataset.h"

namespace Wisteria::Data
    {
    /// @brief Interface for inner joining datasets.
    class DatasetInnerJoin
        {
      public:
        /// @private
        DatasetInnerJoin() = default;
        /// @private
        DatasetInnerJoin(const DatasetInnerJoin&) = delete;
        /// @private
        DatasetInnerJoin& operator=(const DatasetInnerJoin&) = delete;
        /// @private
        virtual ~DatasetInnerJoin() = default;
        /** @brief Inner joins two datasets.\n
                Returns only the rows where the keys match in both datasets.
                If a key in the left dataset has one-to-many matching keys in the right
                dataset, the left row will be duplicated for each matching right row.
                Rows from either dataset that have no matching key in the other dataset
                are excluded from the output.
            @param leftDataset The left dataset to join.
            @param rightDataset The right dataset to join. All columns except for
                the 'by' (i.e., matching ID) columns will be added to the output.\n
                Only data that matches against the left dataset will be imported.
            @param byColumns Pairs of columns to join by between the two datasets.
                Columns can be the ID columns from the datasets, as well as
                categorical columns.
            @param suffix In the case of a (non-joining) column from the right dataset
                already having a column with the same name in the left file, @c suffix
                will be added to the column when being copied to make it unique.
            @returns The joined dataset, containing only rows with matching keys.
            @note For example, if a key in the left dataset matches three rows in the
                right dataset, the output will contain three rows for that key, each with
                the left row's data duplicated and combined with the respective right row's data.\n
                If a left row has no matching right rows, it is excluded from the output.
                Likewise, right rows with no matching left rows are excluded.\n
                Categorical labels must match textually between datasets.
            @throws std::runtime_error If invalid columns or dataset are provided,
                throws an exception.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        InnerJoin(const std::shared_ptr<const Dataset>& leftDataset,
                  const std::shared_ptr<const Dataset>& rightDataset,
                  const std::vector<std::pair<wxString, wxString>>& byColumns,
                  const wxString& suffix = L".x");
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_JOIN_INNER_H
