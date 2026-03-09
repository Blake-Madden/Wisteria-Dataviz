/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef DATASET_GRID_TABLE_H
#define DATASET_GRID_TABLE_H

#include "../../data/dataset.h"
#include <cmath>
#include <map>
#include <memory>
#include <wx/grid.h>
#include <wx/numformatter.h>

namespace Wisteria::UI
    {
    /// @brief The type of a column in a dataset grid.
    enum class DatasetGridColumnType
        {
        Id,
        Categorical,
        Date,
        Continuous
        };

    /// @brief Data provider that adapts a Dataset for display in a wxGrid.
    /// @details When constructed with ColumnPreviewInfo, columns are displayed
    ///     in the same order as they appeared in the original data file.
    ///     Otherwise, column order is: ID (if valid) | Categorical | Date | Continuous.
    class DatasetGridTable final : public wxGridTableBase
        {
      public:
        /// @private
        DatasetGridTable() = delete;

        /// @brief Constructor.
        /// @param dataset The dataset to display.
        explicit DatasetGridTable(std::shared_ptr<Data::Dataset> dataset)
            : m_dataset(std::move(dataset))
            {
            if (m_dataset != nullptr)
                {
                m_hasId = m_dataset->HasValidIdData();
                m_catCount = m_dataset->GetCategoricalColumns().size();
                m_dateCount = m_dataset->GetDateColumns().size();
                m_contCount = m_dataset->GetContinuousColumns().size();
                }
            }

        /// @brief Constructor that preserves original column order.
        /// @param dataset The dataset to display.
        /// @param columnInfo The column preview info from the original data file.
        DatasetGridTable(std::shared_ptr<Data::Dataset> dataset,
                         const Data::Dataset::ColumnPreviewInfo& columnInfo)
            : DatasetGridTable(std::move(dataset))
            {
            BuildColumnOrder(columnInfo);
            }

        /// @brief Sets the maximum number of rows to display.
        /// @param maxRows The maximum row count (0 means unlimited).
        void SetMaxRows(int maxRows) noexcept { m_maxRows = maxRows; }

        /// @returns The maximum number of rows to display (0 means unlimited).
        [[nodiscard]]
        int GetMaxRows() const noexcept
            {
            return m_maxRows;
            }

        /// @private
        [[nodiscard]]
        int GetNumberRows() final
            {
            if (m_dataset == nullptr)
                {
                return 0;
                }
            const auto rows = static_cast<int>(m_dataset->GetRowCount());
            return (m_maxRows > 0 && rows > m_maxRows) ? m_maxRows : rows;
            }

        /// @private
        [[nodiscard]]
        int GetNumberCols() final
            {
            if (!m_columnOrder.empty())
                {
                return static_cast<int>(m_columnOrder.size());
                }
            return static_cast<int>((m_hasId ? 1 : 0) + m_catCount + m_dateCount + m_contCount);
            }

        /// @private
        [[nodiscard]]
        wxString GetValue(int row, int col) final
            {
            if (m_dataset == nullptr || row < 0 ||
                static_cast<size_t>(row) >= m_dataset->GetRowCount() || col < 0 ||
                col >= GetNumberCols())
                {
                return {};
                }

            const auto rowIdx = static_cast<size_t>(row);

            if (!m_columnOrder.empty())
                {
                const auto& mapping = m_columnOrder[static_cast<size_t>(col)];
                return GetValueByMapping(rowIdx, mapping);
                }

            size_t colIndex = static_cast<size_t>(col);

            // ID column
            if (m_hasId)
                {
                if (colIndex == 0)
                    {
                    return m_dataset->GetIdColumn().GetValue(rowIdx);
                    }
                --colIndex;
                }

            // categorical columns
            if (colIndex < m_catCount)
                {
                return m_dataset->GetCategoricalColumns().at(colIndex).GetValueAsLabel(rowIdx);
                }
            colIndex -= m_catCount;

            // date columns
            if (colIndex < m_dateCount)
                {
                const auto& dt = m_dataset->GetDateColumns().at(colIndex).GetValue(rowIdx);
                return dt.IsValid() ? dt.FormatDate() : wxString{};
                }
            colIndex -= m_dateCount;

            // continuous columns
            if (colIndex < m_contCount)
                {
                return FormatContinuousValue(colIndex, rowIdx);
                }

            return {};
            }

        /// @private
        void SetValue([[maybe_unused]] int row, [[maybe_unused]] int col,
                      [[maybe_unused]] const wxString& s) final
            {
            }

        /// @private
        [[nodiscard]]
        wxString GetColLabelValue(int col) final
            {
            if (m_dataset == nullptr || col < 0 || col >= GetNumberCols())
                {
                return {};
                }

            if (!m_columnOrder.empty())
                {
                const auto& mapping = m_columnOrder[static_cast<size_t>(col)];
                return GetNameByMapping(mapping);
                }

            size_t colIndex = static_cast<size_t>(col);

            if (m_hasId)
                {
                if (colIndex == 0)
                    {
                    return m_dataset->GetIdColumn().GetName();
                    }
                --colIndex;
                }

            if (colIndex < m_catCount)
                {
                return m_dataset->GetCategoricalColumns().at(colIndex).GetName();
                }
            colIndex -= m_catCount;

            if (colIndex < m_dateCount)
                {
                return m_dataset->GetDateColumns().at(colIndex).GetName();
                }
            colIndex -= m_dateCount;

            if (colIndex < m_contCount)
                {
                return m_dataset->GetContinuousColumns().at(colIndex).GetName();
                }

            return {};
            }

        /// @brief Returns the column type for a given grid column index.
        /// @param col The grid column index.
        /// @returns The column type.
        [[nodiscard]]
        DatasetGridColumnType GetColumnType(int col) const
            {
            if (!m_columnOrder.empty())
                {
                if (col >= 0 && static_cast<size_t>(col) < m_columnOrder.size())
                    {
                    return m_columnOrder[static_cast<size_t>(col)].m_type;
                    }
                return DatasetGridColumnType::Continuous;
                }

            size_t colIndex = static_cast<size_t>(col);

            if (m_hasId)
                {
                if (colIndex == 0)
                    {
                    return DatasetGridColumnType::Id;
                    }
                --colIndex;
                }

            if (colIndex < m_catCount)
                {
                return DatasetGridColumnType::Categorical;
                }
            colIndex -= m_catCount;

            if (colIndex < m_dateCount)
                {
                return DatasetGridColumnType::Date;
                }

            return DatasetGridColumnType::Continuous;
            }

        /// @brief Sets a currency symbol for a continuous column.
        /// @details When set, the symbol is prepended to the formatted value
        ///     in the grid display. The underlying numeric data is unchanged.
        /// @param contColIndex The zero-based index into the continuous columns.
        /// @param symbol The currency symbol (e.g., L"$", L"€").
        void SetCurrencySymbol(size_t contColIndex, const wxString& symbol)
            {
            if (!symbol.empty())
                {
                m_currencySymbols[contColIndex] = symbol;
                }
            }

      private:
        /// @brief Maps a grid column to its dataset column type and index.
        struct ColumnMapping
            {
            DatasetGridColumnType m_type{ DatasetGridColumnType::Continuous };
            size_t m_index{ 0 };
            };

        /// @brief Builds the column order mapping from ColumnPreviewInfo.
        void BuildColumnOrder(const Data::Dataset::ColumnPreviewInfo& columnInfo)
            {
            if (m_dataset == nullptr || columnInfo.empty())
                {
                return;
                }

            m_columnOrder.reserve(columnInfo.size());

            for (const auto& col : columnInfo)
                {
                // check ID column
                if (m_hasId && m_dataset->GetIdColumn().GetName().CmpNoCase(col.m_name) == 0)
                    {
                    m_columnOrder.push_back({ DatasetGridColumnType::Id, 0 /* unused default */ });
                    continue;
                    }
                // check categorical columns
                bool found{ false };
                for (size_t i = 0; i < m_catCount; ++i)
                    {
                    if (m_dataset->GetCategoricalColumns().at(i).GetName().CmpNoCase(col.m_name) ==
                        0)
                        {
                        m_columnOrder.push_back({ DatasetGridColumnType::Categorical, i });
                        found = true;
                        break;
                        }
                    }
                if (found)
                    {
                    continue;
                    }
                // check date columns
                for (size_t i = 0; i < m_dateCount; ++i)
                    {
                    if (m_dataset->GetDateColumns().at(i).GetName().CmpNoCase(col.m_name) == 0)
                        {
                        m_columnOrder.push_back({ DatasetGridColumnType::Date, i });
                        found = true;
                        break;
                        }
                    }
                if (found)
                    {
                    continue;
                    }
                // check continuous columns
                for (size_t i = 0; i < m_contCount; ++i)
                    {
                    if (m_dataset->GetContinuousColumns().at(i).GetName().CmpNoCase(col.m_name) ==
                        0)
                        {
                        m_columnOrder.push_back({ DatasetGridColumnType::Continuous, i });
                        found = true;
                        break;
                        }
                    }
                if (!found)
                    {
                    wxLogWarning(L"'%s': column not found in dataset for grid display.",
                                 col.m_name);
                    }
                }
            }

        /// @brief Returns the formatted value for a continuous column.
        [[nodiscard]]
        wxString FormatContinuousValue(size_t contIndex, size_t row) const
            {
            const auto val = m_dataset->GetContinuousColumns().at(contIndex).GetValue(row);
            if (!std::isfinite(val))
                {
                return {};
                }
            auto formatted =
                wxNumberFormatter::ToString(val, 2,
                                            wxNumberFormatter::Style::Style_WithThousandsSep |
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes);
            const auto symIt = m_currencySymbols.find(contIndex);
            if (symIt != m_currencySymbols.cend())
                {
                formatted.Prepend(symIt->second);
                }
            return formatted;
            }

        /// @brief Returns the value for a column identified by its mapping.
        [[nodiscard]]
        wxString GetValueByMapping(size_t row, const ColumnMapping& mapping) const
            {
            switch (mapping.m_type)
                {
            case DatasetGridColumnType::Id:
                return m_dataset->GetIdColumn().GetValue(row);
            case DatasetGridColumnType::Categorical:
                return m_dataset->GetCategoricalColumns().at(mapping.m_index).GetValueAsLabel(row);
            case DatasetGridColumnType::Date:
                {
                const auto& dt = m_dataset->GetDateColumns().at(mapping.m_index).GetValue(row);
                return dt.IsValid() ? dt.FormatDate() : wxString{};
                }
            case DatasetGridColumnType::Continuous:
                return FormatContinuousValue(mapping.m_index, row);
                }
            return {};
            }

        /// @brief Returns the column name for a column identified by its mapping.
        [[nodiscard]]
        wxString GetNameByMapping(const ColumnMapping& mapping) const
            {
            switch (mapping.m_type)
                {
            case DatasetGridColumnType::Id:
                return m_dataset->GetIdColumn().GetName();
            case DatasetGridColumnType::Categorical:
                return m_dataset->GetCategoricalColumns().at(mapping.m_index).GetName();
            case DatasetGridColumnType::Date:
                return m_dataset->GetDateColumns().at(mapping.m_index).GetName();
            case DatasetGridColumnType::Continuous:
                return m_dataset->GetContinuousColumns().at(mapping.m_index).GetName();
                }
            return {};
            }

        std::shared_ptr<Data::Dataset> m_dataset;
        int m_maxRows{ 0 };
        bool m_hasId{ false };
        size_t m_catCount{ 0 };
        size_t m_dateCount{ 0 };
        size_t m_contCount{ 0 };
        std::map<size_t, wxString> m_currencySymbols;
        std::vector<ColumnMapping> m_columnOrder;
        };

    /// @brief Column header renderer that draws an icon before the label text.
    class DatasetColumnHeaderRenderer final : public wxGridColumnHeaderRendererDefault
        {
      public:
        /// @private
        DatasetColumnHeaderRenderer() = default;

        /// @brief Constructor.
        /// @param bmp The icon to draw in the header.
        explicit DatasetColumnHeaderRenderer(const wxBitmap& bmp) : m_bitmap(bmp) {}

        /// @private
        void DrawLabel(const wxGrid& grid, wxDC& dc, const wxString& value, const wxRect& rect,
                       int horizAlign, int vertAlign, int textOrientation) const override
            {
            if (m_bitmap.IsOk())
                {
                const int padding = 4;
                const int bmpW = m_bitmap.GetWidth();
                const int bmpH = m_bitmap.GetHeight();
                const int bmpY = rect.y + (rect.height - bmpH) / 2;

                dc.DrawBitmap(m_bitmap, rect.x + padding, bmpY, true);

                wxRect textRect(rect);
                textRect.x += bmpW + padding * 2;
                textRect.width -= bmpW + padding * 2;

                wxGridColumnHeaderRendererDefault::DrawLabel(grid, dc, value, textRect, horizAlign,
                                                             vertAlign, textOrientation);
                }
            else
                {
                wxGridColumnHeaderRendererDefault::DrawLabel(grid, dc, value, rect, horizAlign,
                                                             vertAlign, textOrientation);
                }
            }

      private:
        wxBitmap m_bitmap;
        };

    /// @brief Attribute provider that returns per-column header renderers with icons.
    class DatasetGridAttrProvider final : public wxGridCellAttrProvider
        {
      public:
        /// @brief Sets the header renderer for a column.
        /// @param col The column index.
        /// @param renderer The renderer to use.
        void SetColumnHeaderRenderer(int col, const DatasetColumnHeaderRenderer& renderer)
            {
            m_columnRenderers[col] = renderer;
            }

        /// @private
        [[nodiscard]]
        const wxGridColumnHeaderRenderer& GetColumnHeaderRenderer(int col) override
            {
            const auto it = m_columnRenderers.find(col);
            if (it != m_columnRenderers.end())
                {
                return it->second;
                }
            return wxGridCellAttrProvider::GetColumnHeaderRenderer(col);
            }

      private:
        std::map<int, DatasetColumnHeaderRenderer> m_columnRenderers;
        };
    } // namespace Wisteria::UI

/** @} */

#endif // DATASET_GRID_TABLE_H
