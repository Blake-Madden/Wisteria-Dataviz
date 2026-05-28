///////////////////////////////////////////////////////////////////////////////
// Name:        axisoptionspanel.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef AXIS_OPTIONS_PANEL_H
#define AXIS_OPTIONS_PANEL_H

#include "../../base/axis.h"
#include "../../base/canvas.h"
#include "../../base/reportbuilder.h"
#include <map>
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/datectrl.h>
#include <wx/editlbox.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief A panel for editing axis-related options.
        @details This panel is intended to be used as a page in a sidebar or notebook
            within a graph insertion/editing dialog.*/
    class AxisOptionsPanel : public wxPanel
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param canvas The canvas (optional, used for label editing).
            @param reportBuilder The report builder (optional, used for bracket data).*/
        AxisOptionsPanel(wxWindow* parent, Canvas* canvas, const ReportBuilder* reportBuilder);

        /** @brief Selects which axis type is active in the axis selector.
            @param axisType The axis type to select.*/
        void SelectAxis(AxisType axisType);

        /** @brief Sets the axes to be edited.
            @param axes A map of axis types to axis objects.*/
        void SetAxes(const std::map<AxisType, GraphItems::Axis>& axes);

        /** @brief Returns the edited axes.
            @details Call this before closing the dialog to get the final axis state.
            @returns A map of axis types to axis objects.*/
        [[nodiscard]]
        std::map<AxisType, GraphItems::Axis> GetAxes();

        /** @brief Sets whether the X axis should be mirrored.
            @param mirror @c true to mirror.*/
        void SetMirrorX(bool mirror);
        /** @returns @c true if the X axis should be mirrored.*/
        [[nodiscard]]
        bool GetMirrorX() const;

        /** @brief Sets whether the Y axis should be mirrored.
            @param mirror @c true to mirror.*/
        void SetMirrorY(bool mirror);
        /** @returns @c true if the Y axis should be mirrored.*/
        [[nodiscard]]
        bool GetMirrorY() const;

        /** @brief Sets the suggested dataset and column names for dataset-driven brackets.
            @param datasetName The dataset name hint (preselects in the dataset combo).
            @param labelColumn The label column hint (e.g., the group variable).
            @param valueColumn The value column hint (e.g., the categorical variable).*/
        void SetBracketColumnHints(const wxString& datasetName, const wxString& labelColumn,
                                   const wxString& valueColumn)
            {
            m_bracketDatasetHint = datasetName;
            m_bracketLabelColumnHint = labelColumn;
            m_bracketValueColumnHint = valueColumn;
            }

        /** @brief Hints that a given axis carries date values (for range control display).
            @details Call this before SetAxes() when inserting a new graph so the Range
                group shows date pickers instead of numeric spinners. In edit mode the
                panel auto-detects date axes from the stored axis state.
            @param axisType The axis type.
            @param isDate @c true if the axis represents date values.*/
        void SetAxisIsDate(AxisType axisType, bool isDate);

      private:
        void OnAxisSelectionChanged();
        void ReadControlsFromAxis(const GraphItems::Axis& axis);
        void WriteControlsToAxis(GraphItems::Axis& axis);

        void OnRangeTypeChanged();
        void UpdateRangeControlVisibility();

        void OnAddBracket();
        void OnEditBracket();
        void OnRemoveBracket();
        void OnAddBracketsFromDataset();
        void RefreshBracketList();

        Canvas* m_canvas{ nullptr };
        const ReportBuilder* m_reportBuilder{ nullptr };

        wxString m_bracketDatasetHint;
        wxString m_bracketLabelColumnHint;
        wxString m_bracketValueColumnHint;

        // current editing state
        AxisType m_currentAxisType{ AxisType::BottomXAxis };
        std::map<AxisType, GraphItems::Axis> m_savedAxes;
        std::map<AxisType, bool> m_axisIsDate;

        // controls
        wxChoice* m_axisSelector{ nullptr };

        // per-axis control pointers (Range group)
        wxRadioButton* m_autoRangeRadio{ nullptr };
        wxRadioButton* m_userRangeRadio{ nullptr };
        wxPanel* m_numericRangePanel{ nullptr };
        wxPanel* m_dateRangePanel{ nullptr };
        wxSpinCtrlDouble* m_rangeStartSpin{ nullptr };
        wxSpinCtrlDouble* m_rangeEndSpin{ nullptr };
        wxSpinCtrlDouble* m_rangeIntervalSpin{ nullptr };
        wxSpinCtrl* m_rangeDisplayIntervalSpin{ nullptr };
        wxDatePickerCtrl* m_rangeStartDate{ nullptr };
        wxDatePickerCtrl* m_rangeEndDate{ nullptr };
        wxChoice* m_dateIntervalChoice{ nullptr };

        // per-axis control pointers (Group 2: Axis Line)
        wxColourPickerCtrl* m_axisLineColorPicker{ nullptr };
        wxSpinCtrl* m_axisLineWidthSpin{ nullptr };
        wxChoice* m_axisLineStyleChoice{ nullptr };
        wxChoice* m_axisCapStyleChoice{ nullptr };
        wxCheckBox* m_axisReverseCheck{ nullptr };

        // per-axis control pointers (Group 3: Gridlines)
        wxColourPickerCtrl* m_gridlineColorPicker{ nullptr };
        wxSpinCtrl* m_gridlineWidthSpin{ nullptr };
        wxChoice* m_gridlineStyleChoice{ nullptr };

        // per-axis control pointers (Group 4: Tickmarks)
        wxChoice* m_tickmarkDisplayChoice{ nullptr };

        // per-axis control pointers (Group 5: Labels)
        wxChoice* m_labelDisplayChoice{ nullptr };
        wxChoice* m_numberDisplayChoice{ nullptr };
        wxChoice* m_labelOrientationChoice{ nullptr };
        wxChoice* m_perpAlignmentChoice{ nullptr };
        wxSpinCtrl* m_precisionSpin{ nullptr };
        wxCheckBox* m_doubleSidedCheck{ nullptr };
        wxCheckBox* m_showOuterLabelsCheck{ nullptr };
        wxCheckBox* m_stackLabelsCheck{ nullptr };
        wxSpinCtrl* m_labelLineLengthSpin{ nullptr };

        // per-axis control pointers (Group 6: Brackets)
        wxEditableListBox* m_bracketListBox{ nullptr };

        // global mirror checkboxes
        wxCheckBox* m_mirrorXAxisCheck{ nullptr };
        wxCheckBox* m_mirrorYAxisCheck{ nullptr };
        };
    } // namespace Wisteria::UI

#endif // AXIS_OPTIONS_PANEL_H
