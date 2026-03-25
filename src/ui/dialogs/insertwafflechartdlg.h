/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_WAFFLE_CHART_DIALOG_H
#define INSERT_WAFFLE_CHART_DIALOG_H

#include "../../base/shapes.h"
#include "../../graphs/waffle_chart.h"
#include "insertgraphdlg.h"
#include "insertshapedlg.h"
#include <optional>
#include <vector>
#include <wx/editlbox.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a waffle chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A shapes list with add, edit, and remove buttons.
              Each shape entry is edited via InsertShapeDlg.
            - Grid rounding options (cell count and shape index).
            - An optional row count.
            - Legend placement.*/
    class InsertWaffleChartDlg final : public InsertGraphDlg
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder (may be @c nullptr).
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.*/
        InsertWaffleChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                             const wxString& caption = _(L"Insert Waffle Chart"),
                             wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER,
                             EditMode editMode = EditMode::Insert);

        /// @private
        InsertWaffleChartDlg(const InsertWaffleChartDlg&) = delete;
        /// @private
        InsertWaffleChartDlg& operator=(const InsertWaffleChartDlg&) = delete;

        /// @returns The shapes for the waffle chart.
        [[nodiscard]]
        const std::vector<GraphItems::ShapeInfo>& GetShapes() const noexcept
            {
            return m_shapes;
            }

        /// @returns The grid rounding settings, or @c std::nullopt if disabled.
        [[nodiscard]]
        std::optional<Graphs::WaffleChart::GridRounding> GetGridRounding() const noexcept;

        /// @returns The row count, or @c std::nullopt if not specified.
        [[nodiscard]]
        std::optional<size_t> GetRowCount() const noexcept;

        /// @brief Populates all dialog controls from an existing waffle chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnAddShape();
        void OnEditShape();
        void OnRemoveShape();
        void RefreshShapeList();
        static void BuildShapeFromDlg(const InsertShapeDlg& dlg, GraphItems::ShapeInfo& shapeInfo);
        wxString FormatShapeDescription(const GraphItems::ShapeInfo& shapeInfo) const;

        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };

        wxEditableListBox* m_shapeListBox{ nullptr };

        // grid rounding
        wxSpinCtrl* m_gridRoundCellsSpin{ nullptr };
        wxSpinCtrl* m_gridRoundIndexSpin{ nullptr };

        // row count
        wxSpinCtrl* m_rowCountSpin{ nullptr };

        // DDX data members
        bool m_useGridRounding{ false };
        bool m_useRowCount{ false };

        std::vector<GraphItems::ShapeInfo> m_shapes;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_WAFFLE_CHART_DIALOG_H
