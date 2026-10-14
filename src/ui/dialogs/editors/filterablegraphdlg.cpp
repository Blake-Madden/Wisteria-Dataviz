///////////////////////////////////////////////////////////////////////////////
// Name:        filterablegraphdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "filterablegraphdlg.h"
#include "../../../graphs/filterablegraph.h"
#include <algorithm>
#include <utility>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    FilterableGraphDlg::FilterableGraphDlg(
        const ReportBuilder* reportBuilder, const wxArrayString& graphTypes, wxWindow* parent,
        const bool editMode /*= false*/, wxWindowID id /*= wxID_ANY*/,
        const wxString& caption /*= _(L"Filtered Graph")*/,
        const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
        long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN*/)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_reportBuilder(reportBuilder),
          m_editMode(editMode)
        {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_BLOCK_EVENTS);

        if (m_reportBuilder != nullptr)
            {
            for (const auto& [name, dataset] : m_reportBuilder->GetDatasets())
                {
                m_datasetNames.push_back(name);
                }
            }

        CreateControls(graphTypes);
        FillColumns();
        FillDefaultValues();
        TransferDataToWindow();

        Bind(wxEVT_BUTTON, &FilterableGraphDlg::OnOK, this, wxID_OK);

        Centre();
        }

    //-------------------------------------------
    void FilterableGraphDlg::CreateControls(const wxArrayString& graphTypes)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        // dataset
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Dataset:")),
                       wxSizerFlags{}.CenterVertical());
        wxArrayString datasetNames;
        for (const auto& name : m_datasetNames)
            {
            datasetNames.Add(name);
            }
        auto* datasetChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition,
                                           FromDIP(wxSize{ 250, -1 }), datasetNames);
        datasetChoice->SetValidator(wxGenericValidator{ &m_datasetIndex });
        gridSizer->Add(datasetChoice, wxSizerFlags{}.Expand());

        // column to filter on
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Filter by:")),
                       wxSizerFlags{}.CenterVertical());
        m_columnChoice = new wxChoice(this, wxID_ANY);
        m_columnChoice->SetValidator(wxGenericValidator{ &m_columnIndex });
        gridSizer->Add(m_columnChoice, wxSizerFlags{}.Expand());

        // value shown by default
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Show by default:")),
                       wxSizerFlags{}.CenterVertical());
        m_defaultChoice = new wxChoice(this, wxID_ANY);
        m_defaultChoice->SetValidator(wxGenericValidator{ &m_defaultIndex });
        gridSizer->Add(m_defaultChoice, wxSizerFlags{}.Expand());

        // graph type
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Graph:")),
                       wxSizerFlags{}.CenterVertical());
        auto* graphChoice =
            new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, graphTypes);
        graphChoice->SetValidator(wxGenericValidator{ &m_graphType });
        gridSizer->Add(graphChoice, wxSizerFlags{}.Expand());

        mainSizer->Add(gridSizer, wxSizerFlags{}.Expand().Border());

        auto* noteLabel = new wxStaticText(
            this, wxID_STATIC,
            _(L"The default value is what you see in the program and in most exports. "
              "(Filtering only works in HTML dashboards.) "
              "Filtered graphs that use the same dataset and column are linked together."));
        noteLabel->Wrap(FromDIP(350));
        mainSizer->Add(noteLabel, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        // only the default value can be changed once the graph exists
        if (m_editMode)
            {
            datasetChoice->Disable();
            m_columnChoice->Disable();
            graphChoice->Disable();
            }

        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        m_columnChoice->Bind(wxEVT_CHOICE,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnColumnChanged(); });

        SetSizerAndFit(mainSizer);
        }

    //-------------------------------------------
    wxString FilterableGraphDlg::GetItem(const std::vector<wxString>& items, const int index)
        {
        return (index >= 0 && std::cmp_less(index, items.size())) ? items[index] : wxString{};
        }

    //-------------------------------------------
    void FilterableGraphDlg::FillColumns()
        {
        m_columnNames.clear();
        if (m_reportBuilder != nullptr)
            {
            const auto& datasets = m_reportBuilder->GetDatasets();
            if (const auto foundPos = datasets.find(GetDatasetName());
                foundPos != datasets.cend() && foundPos->second != nullptr)
                {
                for (const auto& column : foundPos->second->GetCategoricalColumns())
                    {
                    m_columnNames.push_back(column.GetName());
                    }
                }
            }

        wxArrayString columnNames;
        for (const auto& name : m_columnNames)
            {
            columnNames.Add(name);
            }
        m_columnChoice->Set(columnNames);
        m_columnIndex = 0;
        }

    //-------------------------------------------
    void FilterableGraphDlg::FillDefaultValues()
        {
        m_defaultValues.clear();
        m_defaultValues.push_back(
            m_allLabel.empty() ? Graphs::FilterableGraph::GetDefaultAllLabel() : m_allLabel);
        if (m_reportBuilder != nullptr)
            {
            const auto& datasets = m_reportBuilder->GetDatasets();
            if (const auto foundPos = datasets.find(GetDatasetName());
                foundPos != datasets.cend() && foundPos->second != nullptr)
                {
                for (const auto& value :
                     Graphs::FilterableGraph::GetFilterValues(*foundPos->second, GetFilterColumn()))
                    {
                    m_defaultValues.push_back(value);
                    }
                }
            }

        wxArrayString defaultValues;
        for (const auto& value : m_defaultValues)
            {
            defaultValues.Add(value);
            }
        m_defaultChoice->Set(defaultValues);
        m_defaultIndex = 0;
        }

    //-------------------------------------------
    void FilterableGraphDlg::OnDatasetChanged()
        {
        TransferDataFromWindow();
        FillColumns();
        FillDefaultValues();
        TransferDataToWindow();
        }

    //-------------------------------------------
    void FilterableGraphDlg::OnColumnChanged()
        {
        TransferDataFromWindow();
        FillDefaultValues();
        TransferDataToWindow();
        }

    //-------------------------------------------
    void FilterableGraphDlg::SetFilter(const wxString& datasetName, const wxString& filterColumn,
                                       const wxString& defaultValue, const int graphType,
                                       const wxString& allLabel /*= wxString{}*/)
        {
        m_allLabel = allLabel;
        const auto findIndex = [](const std::vector<wxString>& items, const wxString& item)
        {
            const auto foundPos = std::ranges::find_if(items, [&item](const auto& entry)
                                                       { return entry.CmpNoCase(item) == 0; });
            return (foundPos != items.cend()) ?
                       static_cast<int>(std::distance(items.cbegin(), foundPos)) :
                       0;
        };

        m_datasetIndex = findIndex(m_datasetNames, datasetName);
        m_graphType = graphType;
        FillColumns();
        m_columnIndex = findIndex(m_columnNames, filterColumn);
        FillDefaultValues();
        m_defaultIndex = findIndex(m_defaultValues, defaultValue);
        TransferDataToWindow();
        }

    //-------------------------------------------
    wxString FilterableGraphDlg::GetDatasetName() const
        {
        return GetItem(m_datasetNames, m_datasetIndex);
        }

    //-------------------------------------------
    wxString FilterableGraphDlg::GetFilterColumn() const
        {
        return GetItem(m_columnNames, m_columnIndex);
        }

    //-------------------------------------------
    wxString FilterableGraphDlg::GetDefaultValue() const
        {
        return GetItem(m_defaultValues, m_defaultIndex);
        }

    //-------------------------------------------
    void FilterableGraphDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
        {
        TransferDataFromWindow();

        if (GetDatasetName().empty())
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return;
            }
        if (GetFilterColumn().empty())
            {
            wxMessageBox(_(L"Please select a categorical column to filter by."),
                         _(L"No Filter Column"), wxOK | wxICON_WARNING, this);
            return;
            }
        // the first value is the choice for all values
        if (m_defaultValues.size() < 2)
            {
            wxMessageBox(_(L"The selected column does not contain any values to filter by."),
                         _(L"No Filter Values"), wxOK | wxICON_WARNING, this);
            return;
            }
        // each value builds its own graph and adds a copy of the page to dashboard exports
        if (!m_editMode && m_defaultValues.size() > MAX_RECOMMENDED_FILTER_VALUES &&
            wxMessageBox(
                wxString::Format(_(L"'%s' has %zu values to filter by. A graph will be built for "
                                   "each value, which may be slow and make exported dashboards "
                                   "very large.\n\nDo you wish to continue?"),
                                 GetFilterColumn(), m_defaultValues.size()),
                _(L"Many Filter Values"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
            {
            return;
            }

        if (IsModal())
            {
            EndModal(wxID_OK);
            }
        else
            {
            Show(false);
            }
        }
    } // namespace Wisteria::UI
