/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_COMMON_AXIS_DIALOG_H
#define INSERT_COMMON_AXIS_DIALOG_H

#include "../../base/axis.h"
#include "axisoptionspanel.h"
#include "insertitemdlg.h"
#include <wx/checklst.h>
#include <wx/valgen.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting or editing a common (shared) axis on a canvas.
        @details A common axis connects two or more graphs in the same row or column,
            unifying their scales and displaying one set of labels instead of
            duplicating them per graph.

            Extends InsertItemDlg with two additional sidebar pages:
            - "Common Axis" for selecting the axis type, connected graphs,
              and the common perpendicular axis option.
            - "Axis" (reusing AxisOptionsPanel) for per-axis display
              settings such as pen, gridlines, tickmarks, labels, and brackets.

        Can also be used to edit an existing common axis by calling
        LoadFromAxis() before showing the dialog.*/
    class InsertCommonAxisDlg final : public InsertItemDlg
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
        InsertCommonAxisDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                            const wxString& caption = _(L"Insert Common Axis"),
                            wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                            EditMode editMode = EditMode::Insert);

        /// @private
        InsertCommonAxisDlg(const InsertCommonAxisDlg&) = delete;
        /// @private
        InsertCommonAxisDlg& operator=(const InsertCommonAxisDlg&) = delete;

        /// @returns The selected axis type.
        [[nodiscard]]
        AxisType GetAxisType() const noexcept;

        /// @returns The IDs of the selected child graphs.
        [[nodiscard]]
        std::vector<long> GetChildGraphIds() const;

        /// @returns @c true if the common perpendicular axis option is checked.
        [[nodiscard]]
        bool GetCommonPerpendicularAxis() const noexcept
            {
            return m_commonPerpAxis;
            }

        /// @returns The edited axis options from the AxisOptionsPanel.
        [[nodiscard]]
        std::map<AxisType, GraphItems::Axis> GetAxes();

        /// @brief Populates controls from an existing common axis for editing.
        /// @param axis The axis to read settings from.
        void LoadFromAxis(const GraphItems::Axis& axis);

      private:
        void CreateControls() final;
        bool Validate() final;

        void PopulateGraphChecklist();
        void OnAxisTypeChanged();

        constexpr static wxWindowID ID_COMMON_AXIS_SECTION{ wxID_HIGHEST + 5 };
        constexpr static wxWindowID ID_AXIS_OPTIONS_SECTION{ wxID_HIGHEST + 6 };

        // Common Axis page controls
        wxChoice* m_axisTypeChoice{ nullptr };
        wxCheckListBox* m_graphChecklist{ nullptr };
        wxCheckBox* m_commonPerpAxisCheck{ nullptr };

        // Axis Options page
        AxisOptionsPanel* m_axisOptionsPanel{ nullptr };

        // DDX
        int m_axisTypeIndex{ 0 };
        bool m_commonPerpAxis{ false };

        // maps checklist index to graph ID
        std::vector<long> m_graphIdMap;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_COMMON_AXIS_DIALOG_H
