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
    /// @details Column order: ID (if valid) | Categorical | Date | Continuous.
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

        /// @private
        [[nodiscard]]
        int GetNumberRows() final
            {
            return (m_dataset != nullptr) ? static_cast<int>(m_dataset->GetRowCount()) : 0;
            }

        /// @private
        [[nodiscard]]
        int GetNumberCols() final
            {
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

            size_t colIndex = static_cast<size_t>(col);

            // ID column
            if (m_hasId)
                {
                if (colIndex == 0)
                    {
                    return m_dataset->GetIdColumn().GetValue(static_cast<size_t>(row));
                    }
                --colIndex;
                }

            // categorical columns
            if (colIndex < m_catCount)
                {
                return m_dataset->GetCategoricalColumns().at(colIndex).GetValueAsLabel(
                    static_cast<size_t>(row));
                }
            colIndex -= m_catCount;

            // date columns
            if (colIndex < m_dateCount)
                {
                const auto& dt =
                    m_dataset->GetDateColumns().at(colIndex).GetValue(static_cast<size_t>(row));
                return dt.IsValid() ? dt.FormatDate() : wxString{};
                }
            colIndex -= m_dateCount;

            // continuous columns
            if (colIndex < m_contCount)
                {
                const auto val = m_dataset->GetContinuousColumns().at(colIndex).GetValue(
                    static_cast<size_t>(row));
                return !std::isfinite(val) ?
                           wxString{} :
                           wxNumberFormatter::ToString(
                               val, -1, wxNumberFormatter::Style::Style_NoTrailingZeroes);
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

      private:
        std::shared_ptr<Data::Dataset> m_dataset;
        bool m_hasId{ false };
        size_t m_catCount{ 0 };
        size_t m_dateCount{ 0 };
        size_t m_contCount{ 0 };
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
