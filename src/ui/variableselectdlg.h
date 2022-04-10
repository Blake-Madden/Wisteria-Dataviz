/** @addtogroup UI
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __VARIABLE_SELECT_DLG_H__
#define __VARIABLE_SELECT_DLG_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/valgen.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include <wx/artprov.h>
#include "../dataset.h"

namespace Wisteria::UI
    {
    /** @brief Dialog for selecting variables for an analysis.*/
    class VariableSelectDlg final : public wxDialog
        {
    public:
        /// @internal This is used as a bitmask, so can't be strongly typed.
        /// @brief The type of variables that can be selected.
        /// @details These are bitmask values that can be ORed together.
        enum VariableSelections
            {
            NoVariables = 0,              /*!< Nothing to select.*/
            XVariable = 0x0010,           /*!< Select an X variable.*/
            YVariable = 0x0020,           /*!< Select a Y variable.*/
            GroupingVariables = 0x0040,   /*!< Select a grouping variable.*/
            CategoricalVariables = 0x0080 /*!< Select categorical variable(s).*/
            };

        /// @internal This is used as a bitmask, so can't be strongly typed.
        /// @brief Which variable styles are single selection.
        /// @details These are bitmask values that can be ORed together.
        /// @note X and Y areas are always single selection by design, regardless
        ///  of any values specified here.
        enum SingleSelectionTypes
            {
            NoSingleSelection,  /*!< Nothing is single selection.*/
            Grouping            /*!< Grouping variable area is single selection.*/
            };

        /** @brief Constructor.
            @param parent The dialog's parent.
            @param columnInfo The list of columns (and their respective data types) to choose from.
             This will usually be the result from a call to Dataset::ReadColumnInfo().
            @param varTypes Which type of variables the user can select.
            @param SingleSelectionTypes Which variable groups should be single selection.
            @param id The dialog's ID.
            @param caption The caption for the dialog.
            @param pos The dialog's position on the screen.
            @param size The default size of the dialog.
            @param style The dialog's style.*/
        VariableSelectDlg(wxWindow* parent, const Data::Dataset::ColumnPreviewInfo& columnInfo,
                          VariableSelections varTypes,
                          SingleSelectionTypes singleSelTypes = SingleSelectionTypes::NoSingleSelection,
                          wxWindowID id = wxID_ANY,
                          const wxString& caption = _("Select Variables"),
                          const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER);
        /// @private
        VariableSelectDlg() = delete;
        /// @private
        VariableSelectDlg(const VariableSelectDlg&) = delete;
        /// @private
        VariableSelectDlg& operator=(const VariableSelectDlg&) = delete;

        /** @brief Sets the display label for the X variable.
            @param label The label to show.*/
        void SetXVariableLabel(const wxString& label)
            {
            auto labelCtrl = FindWindowById(ID_X_VAR_LABEL);
            if (labelCtrl != nullptr)
                { labelCtrl->SetLabel(label+L":"); }
            }
        /** @brief Sets the display label for the Y variable.
            @param label The label to show.*/
        void SetYVariableLabel(const wxString& label)
            {
            auto labelCtrl = FindWindowById(ID_Y_VAR_LABEL);
            if (labelCtrl != nullptr)
                { labelCtrl->SetLabel(label+L":"); }
            }
        /** @brief Sets the display label for the categorical variables.
            @param label The label to show.*/
        void SetCategoricalVariablesLabel(const wxString& label)
            {
            auto labelCtrl = FindWindowById(ID_CAT_VARS_LABEL);
            if (labelCtrl != nullptr)
                { labelCtrl->SetLabel(label+L":"); }
            }
        /** @brief Sets the display label for the grouping variable.
            @param label The label to show.*/
        void SetGroupingVariableLabel(const wxString& label)
            {
            auto labelCtrl = FindWindowById(ID_GROUP_VAR_LABEL);
            if (labelCtrl != nullptr)
                { labelCtrl->SetLabel(label+L":"); }
            }

        /// @returns The X variable that the user selected.
        ///  Will be an empty string is nothing was selected.
        [[nodiscard]] wxString GetXVariable() const
            { return m_xVarList->GetItemCount() ? m_xVarList->GetItemText(0, 0) : wxString(L""); };
        /// @returns The Y variable that the user selected.
        ///  Will be an empty string is nothing was selected.
        [[nodiscard]] wxString GetYVariable() const
            { return m_yVarList->GetItemCount() ? m_yVarList->GetItemText(0, 0) : wxString(L""); };
        /// @returns The categorical variables that the user selected.
        [[nodiscard]] std::vector<wxString> GetCategoricalVariables() const;
        /// @returns The grouping variable that the user selected.
        ///  Will be an empty string is nothing was selected.
        [[nodiscard]] wxString GetGroupingVariable() const
            {
            return m_groupVarList->GetItemCount() ?
                m_groupVarList->GetItemText(0, 0) : wxString(L"");
            };
    private:
        void CreateControls(VariableSelections varTypes, SingleSelectionTypes singleSelTypes);
        /// @brief Moves the selected variables in one list to another.
        /// @param list The list to move items from.
        /// @param otherList The sub list to move the variables into.
        static void MoveSelectedVariables(wxListView* list, wxListView* othotherListerlist);
        /// @returns the list of variables selected a list.
        /// @param list The list to get the selected items from.
        /// @returns A vector of the selected strings.
        [[nodiscard]] static std::vector<wxString> GetSelectedVariables(wxListView* list);
        /// @brief Removes the selected items from a list.
        /// @param list The list box to remove items from.
        static void RemoveSelectedVariables(wxListView* list);
        /// @brief Enables/disables buttons as needed.
        void UpdateButtonStates();

        static constexpr int ID_X_VAR_LABEL = wxID_HIGHEST + 1;
        static constexpr int ID_X_VAR_ADD = wxID_HIGHEST + 2;
        static constexpr int ID_X_VAR_REMOVE = wxID_HIGHEST + 3;

        static constexpr int ID_Y_VAR_LABEL = wxID_HIGHEST + 4;
        static constexpr int ID_Y_VAR_ADD = wxID_HIGHEST + 5;
        static constexpr int ID_Y_VAR_REMOVE = wxID_HIGHEST + 6;

        static constexpr int ID_CAT_VARS_LABEL = wxID_HIGHEST + 7;
        static constexpr int ID_CAT_VARS_ADD = wxID_HIGHEST + 8;
        static constexpr int ID_CAT_VARS_REMOVE = wxID_HIGHEST + 9;

        static constexpr int ID_GROUP_VAR_LABEL = wxID_HIGHEST + 10;
        static constexpr int ID_GROUP_VAR_ADD = wxID_HIGHEST + 11;
        static constexpr int ID_GROUP_VAR_REMOVE = wxID_HIGHEST + 12;
        Data::Dataset::ColumnPreviewInfo m_columnInfo;

        wxListView* m_varList{ nullptr };

        wxListView* m_xVarList{ nullptr };
        wxListView* m_yVarList{ nullptr };
        wxListView* m_groupVarList{ nullptr };
        wxListView* m_categoricalVarList{ nullptr };
        };
    }

/** @}*/

#endif //__VARIABLE_SELECT_DLG_H__
