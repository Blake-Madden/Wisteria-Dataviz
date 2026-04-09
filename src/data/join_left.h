/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
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
    class DatasetLeftJoin
        {
      public:
        /// @private
        DatasetLeftJoin() = default;
        /// @private
        DatasetLeftJoin(const DatasetLeftJoin&) = delete;
        /// @private
        DatasetLeftJoin& operator=(const DatasetLeftJoin&) = delete;
        /// @private
        virtual ~DatasetLeftJoin() = default;
        /** @brief Left joins one dataset with another.\n
                If a key in the left dataset has one-to-many matching keys in the right
                dataset, only the data from the last matching right row will be used.
                This is useful for collapsing one-to-many relationships into a single row.
            @param leftDataset The left dataset which will be joined with. All rows and columns
                from this dataset will be preserved in the output.
            @param rightDataset The dataset being joined with the left one. All columns except for
                the 'by' (i.e., matching ID) columns will be added to the output.\n
                Only data that matches against the left dataset will be imported; in other words,
                rows in this dataset whose keys don't appear in the left dataset will be ignored.
                Also, when multiple rows share the same key, only data from the last row
                will be imported.
            @param byColumns Pairs of columns to join by between the two datasets. Columns can be
                the ID columns from the datasets, as well as categorical columns.
            @param suffix In the case of a (non-joining) column from the right dataset already
                having a column with the same name in the left file, @c suffix will be added to
                the column when being copied to make it unique.
            @returns The joined dataset.
            @note Unlike a standard left join (see LeftJoin()), this function will not add
                additional rows when a left key matches multiple right rows. Instead, each
                prior match is overwritten, so that only the last matching right row's data
                is retained.\n
                If one-to-many keys are encountered in the right dataset, a warning will
                be issued via @c wxLogWarning().\n
                Categorical labels must match textually between datasets.
            @throws std::runtime_error If invalid columns or dataset are provided,
                throws an exception.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        LeftJoinUniqueLast(const std::shared_ptr<const Dataset>& leftDataset,
                           const std::shared_ptr<const Dataset>& rightDataset,
                           const std::vector<std::pair<wxString, wxString>>& byColumns,
                           const wxString& suffix = L".x");
        /** @brief Left joins one dataset with another.\n
                If a key in the left dataset has one-to-many matching keys in the right
                dataset, only the data from the first matching right row will be used.
                This is useful for collapsing one-to-many relationships into a single row.
            @param leftDataset The left dataset which will be joined with. All rows and columns
                from this dataset will be preserved in the output.
            @param rightDataset The dataset being joined with the left one. All columns except for
                the 'by' (i.e., matching ID) columns will be added to the output.\n
                Only data that matches against the left dataset will be imported; in other words,
                rows in this dataset whose keys don't appear in the left dataset will be ignored.
                Also, when multiple rows share the same key, only data from the first row
                will be imported.
            @param byColumns Pairs of columns to join by between the two datasets. Columns can be
                the ID columns from the datasets, as well as categorical columns.
            @param suffix In the case of a (non-joining) column from the right dataset already
                having a column with the same name in the left file, @c suffix will be added to
                the column when being copied to make it unique.
            @returns The joined dataset.
            @note Unlike a standard left join (see LeftJoin()), this function will not add
                additional rows when a left key matches multiple right rows. Instead, only
                the first matching right row's data is retained, and subsequent matches
                are ignored.\n
                If one-to-many keys are encountered in the right dataset, a warning will
                be issued via @c wxLogWarning().\n
                Categorical labels must match textually between datasets.
            @throws std::runtime_error If invalid columns or dataset are provided,
                throws an exception.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        LeftJoinUniqueFirst(const std::shared_ptr<const Dataset>& leftDataset,
                            const std::shared_ptr<const Dataset>& rightDataset,
                            const std::vector<std::pair<wxString, wxString>>& byColumns,
                            const wxString& suffix = L".x");
        /** @brief Left joins one dataset with another.\n
                This performs a standard left join. If a key in the left dataset has
                one-to-many matching keys in the right dataset, the left row will be
                duplicated for each matching right row. This means the output may have
                more rows than the left dataset.
            @param leftDataset The left dataset which will be joined with. All rows and columns
                from this dataset will be preserved in the output.
            @param rightDataset The dataset being joined with the left one. All columns except for
                the 'by' (i.e., matching ID) columns will be added to the output.\n
                Only data that matches against the left dataset will be imported; in other words,
                rows in this dataset whose keys don't appear in the left dataset will be ignored.
            @param byColumns Pairs of columns to join by between the two datasets. Columns can be
                the ID columns from the datasets, as well as categorical columns.
            @param suffix In the case of a (non-joining) column from the right dataset already
                having a column with the same name in the left file, @c suffix will be added to
                the column when being copied to make it unique.
            @returns The joined dataset.
            @note For example, if a key in the left dataset matches three rows in the
                right dataset, the output will contain three rows for that key, each with
                the left data combined with the respective right row's data.\n
                If a left row has no matching right rows, it is still included in the output
                with missing data in the right columns.\n
                Categorical labels must match textually between datasets.
            @throws std::runtime_error If invalid columns or dataset are provided,
                throws an exception.*/
        [[nodiscard]]
        static std::shared_ptr<Dataset>
        LeftJoin(const std::shared_ptr<const Dataset>& leftDataset,
                 const std::shared_ptr<const Dataset>& rightDataset,
                 const std::vector<std::pair<wxString, wxString>>& byColumns,
                 const wxString& suffix = L".x");
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_JOIN_H
