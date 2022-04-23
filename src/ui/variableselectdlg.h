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
#include <wx/wupdlock.h>
#include "../data/dataset.h"

namespace Wisteria::UI
    {
    /** @brief Dialog for selecting variables for an analysis.*/
    class VariableSelectDlg final : public wxDialog
        {
    public:
        /// @brief Definition for a variable list for a user to select.
        /// @details Examples of this could be a list of categoricals, a single grouping variable,
        ///  an X variable, etc.
        class VariableListInfo
            {
            friend class VariableSelectDlg;
        public:
            /// @brief Sets the list's label.
            /// @param name The label to display above the variable list.
            /// @returns A self reference.
            VariableListInfo& Label(const wxString& label)
                {
                m_label = label;
                return *this;
                }
            /// @brief Sets whether the list can hold multiple variables or just one.
            /// @details The default is for the list to allow multiple variables.
            /// @param singleSelection @c true to only allow one variable to be
            ///  selected for this list.
            /// @returns A self reference.
            VariableListInfo& SingleSelection(const bool singleSelection)
                {
                m_singleSelection = singleSelection;
                return *this;
                }
            /// @brief Sets whether a variable must be selected for this list.
            /// @details The default is for the list to be required.
            /// @param required @c true to force the user to select a variable for this list.
            /// @returns A self reference.
            VariableListInfo& Required(const bool required)
                {
                m_required = required;
                return *this;
                }
        private:
            wxString m_label;
            bool m_singleSelection{ false };
            bool m_required{ true };
            };

        /** @brief Constructor.
            @param parent The dialog's parent.
            @param columnInfo The list of columns (and their respective data types) to choose from.
             This will usually be the result from a call to Dataset::ReadColumnInfo().
            @param varInfo Definitions for the variable lists that the user can specify.
            @param id The dialog's ID.
            @param caption The caption for the dialog.
            @param pos The dialog's position on the screen.
            @param size The default size of the dialog.
            @param style The dialog's style.*/
        VariableSelectDlg(wxWindow* parent, const Data::Dataset::ColumnPreviewInfo& columnInfo,
                          const std::vector<VariableListInfo>& varInfo,
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

        /// @brief Gets the variables that a user has moved into a given list.
        /// @detais The list is accessed by index, in the order that the variable definitions
        ///  were passed to the constructor. For example, if the VariableListInfo passed to the
        ///  constructor included a continuous and grouping set of variable lists (in that order),
        ///  then `1` will return the variables in the grouping list.
        /// @param listIndex The index of the client-defined variable list.
        /// @returns A list of the variable names that the user has selected for a given list.
        std::vector<wxString> VariableSelectDlg::GetSelectedVariables(const size_t listIndex) const;
    private:
        struct VariableList
            {
            wxString m_label;
            int m_addId{ wxID_ANY };
            int m_removeId{ wxID_ANY };
            bool m_singleSelection{ false };
            wxListView* m_list{ nullptr };
            bool m_required{ false };
            };

        bool Validate();
        void CreateControls(const std::vector<VariableListInfo>& varInfo);
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

        Data::Dataset::ColumnPreviewInfo m_columnInfo;
        wxListView* m_mainVarlist{ nullptr };
        std::vector<VariableList> m_varLists;
        };
    }

/** @}*/

#endif //__VARIABLE_SELECT_DLG_H__
