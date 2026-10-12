/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef FILTERABLE_GRAPH_DIALOG_H
#define FILTERABLE_GRAPH_DIALOG_H

#include "../../reporting/reportbuilder.h"
#include "../dialogwithhelp.h"
#include <vector>
#include <wx/choice.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for choosing how a graph is filtered: the dataset, the categorical
            column to filter on, the value shown by default, and the type of graph.
        @details When editing, only the default value can be changed.*/
    class FilterableGraphDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param reportBuilder The report builder containing the project's datasets.
            @param graphTypes The display names of the graph types to choose from.
            @param parent The parent window.
            @param editMode @c true if editing an existing graph's filter.
            @param id The window ID.
            @param caption The title of the dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        FilterableGraphDlg(const ReportBuilder* reportBuilder, const wxArrayString& graphTypes,
                           wxWindow* parent, bool editMode = false, wxWindowID id = wxID_ANY,
                           const wxString& caption = _(L"Filtered Graph"),
                           const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);

        /// @private
        FilterableGraphDlg(const FilterableGraphDlg&) = delete;
        /// @private
        FilterableGraphDlg& operator=(const FilterableGraphDlg&) = delete;

        /** @brief Selects the filter settings of an existing graph.
            @param datasetName The graph's dataset.
            @param filterColumn The column being filtered on.
            @param defaultValue The value shown by default.
            @param includeAll Whether a choice for all values is included.
            @param graphType The index of the graph's type.
            @param allLabel The label of the choice for all values (empty for the default).*/
        void SetFilter(const wxString& datasetName, const wxString& filterColumn,
                       const wxString& defaultValue, bool includeAll, int graphType,
                       const wxString& allLabel = wxString{});

        /// @returns The name of the selected dataset.
        [[nodiscard]]
        wxString GetDatasetName() const;

        /// @returns The selected column to filter on.
        [[nodiscard]]
        wxString GetFilterColumn() const;

        /// @returns The selected value to show by default.
        [[nodiscard]]
        wxString GetDefaultValue() const;

        /// @returns Whether to include a choice for all values.
        [[nodiscard]]
        bool IsIncludingAll() const noexcept
            {
            return m_includeAll;
            }

        /// @returns The index of the selected graph type.
        [[nodiscard]]
        int GetGraphType() const noexcept
            {
            return m_graphType;
            }

      private:
        /// @brief Number of filter values above which the user is warned about
        ///     the cost of building (and exporting) a graph for each one.
        constexpr static size_t MAX_RECOMMENDED_FILTER_VALUES{ 25 };

        void CreateControls(const wxArrayString& graphTypes);
        void FillColumns();
        void FillDefaultValues();
        void OnDatasetChanged();
        void OnColumnChanged();
        void OnIncludeAllChanged();
        void OnOK([[maybe_unused]] wxCommandEvent& event);

        [[nodiscard]]
        static wxString GetItem(const std::vector<wxString>& items, int index);

        const ReportBuilder* m_reportBuilder{ nullptr };
        bool m_editMode{ false };
        wxString m_allLabel;

        std::vector<wxString> m_datasetNames;
        std::vector<wxString> m_columnNames;
        std::vector<wxString> m_defaultValues;

        // DDX data members
        int m_datasetIndex{ 0 };
        int m_columnIndex{ 0 };
        int m_defaultIndex{ 0 };
        int m_graphType{ 0 };
        bool m_includeAll{ true };

        wxChoice* m_columnChoice{ nullptr };
        wxChoice* m_defaultChoice{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // FILTERABLE_GRAPH_DIALOG_H
