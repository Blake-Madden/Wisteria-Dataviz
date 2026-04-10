///////////////////////////////////////////////////////////////////////////////
// Name:        insertheatmapdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertheatmapdlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertHeatMapDlg::InsertHeatMapDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                       wxWindow* parent, const wxString& caption,
                                       const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                       const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Heat Map"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        datasetSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dataset:")),
                          wxSizerFlags{}.CenterVertical());
        m_datasetChoice = new wxChoice(optionsPage, ID_DATASET_CHOICE);
        datasetSizer->Add(m_datasetChoice);

        // populate dataset names from the report builder
        if (GetReportBuilder() != nullptr)
            {
            for (const auto& [name, dataset] : GetReportBuilder()->GetDatasets())
                {
                m_datasetNames.push_back(name);
                m_datasetChoice->Append(name);
                }
            }
        if (!m_datasetNames.empty())
            {
            m_datasetChoice->SetSelection(0);
            }

        optionsSizer->Add(datasetSizer, wxSizerFlags{}.Border());

        // variables button
        auto* varsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Variables"));
        auto* varButton =
            new wxButton(varsBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select..."));
        varsBox->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* contLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Continuous:"));
        contLabel->SetFont(contLabel->GetFont().Bold());
        varGrid->Add(contLabel, wxSizerFlags{}.CenterVertical());
        m_continuousVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_continuousVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_continuousVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Group:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // group column count
        auto* groupOptsSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        m_groupColumnCountLabel =
            new wxStaticText(optionsPage, wxID_ANY, _(L"Group column count:"));
        groupOptsSizer->Add(m_groupColumnCountLabel, wxSizerFlags{}.CenterVertical());
        m_groupColumnCountSpin = new wxSpinCtrl(optionsPage, wxID_ANY);
        m_groupColumnCountSpin->SetRange(1, 5);
        m_groupColumnCountSpin->SetValue(m_groupColumnCount);
        m_groupColumnCountSpin->SetValidator(wxGenericValidator(&m_groupColumnCount));
        groupOptsSizer->Add(m_groupColumnCountSpin, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(groupOptsSizer, wxSizerFlags{}.Border());

        // show group headers
        m_showGroupHeadersCheck =
            new wxCheckBox(optionsPage, wxID_ANY, _(L"Show group headers"), wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator(&m_showGroupHeaders));
        optionsSizer->Add(m_showGroupHeadersCheck, wxSizerFlags{}.Border(wxLEFT));

        // group header prefix
        auto* prefixSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        m_groupHeaderPrefixLabel =
            new wxStaticText(optionsPage, wxID_ANY, _(L"Group header prefix:"));
        prefixSizer->Add(m_groupHeaderPrefixLabel, wxSizerFlags{}.CenterVertical());
        m_groupHeaderPrefixText =
            new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize, 0,
                           wxGenericValidator(&m_groupHeaderPrefix));
        prefixSizer->Add(m_groupHeaderPrefixText, wxSizerFlags{}.CenterVertical().Expand());

        optionsSizer->Add(prefixSizer, wxSizerFlags{}.Border());

        // legend placement
        auto* legendGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                        wxSizerFlags{}.CenterVertical());
        legendGrid->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendGrid, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        m_showGroupHeadersCheck->Bind(wxEVT_CHECKBOX, [this]([[maybe_unused]] wxCommandEvent&)
                                      { UpdateGroupControlStates(); });

        UpdateGroupControlStates();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::OnDatasetChanged()
        {
        m_continuousVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        // prefer the stored column preview info (preserves original file order)
        Data::Dataset::ColumnPreviewInfo columnInfo;
        if (GetReportBuilder() != nullptr)
            {
            const auto& importOpts = GetReportBuilder()->GetDatasetImportOptions();
            const int sel = m_datasetChoice->GetSelection();
            if (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size()))
                {
                const auto it = importOpts.find(m_datasetNames[sel]);
                if (it != importOpts.cend())
                    {
                    columnInfo = it->second.m_columnPreviewInfo;
                    }
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        using VLI = VariableSelectDlg::VariableListInfo;
        VariableSelectDlg dlg(
            this, columnInfo,
            { VLI{}
                  .Label(_(L"Continuous"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_continuousVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_continuousVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Group"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto contVars = dlg.GetSelectedVariables(0);
        m_continuousVariable = contVars.empty() ? wxString{} : contVars.front();

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::UpdateVariableLabels()
        {
        m_continuousVarLabel->SetLabel(m_continuousVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        UpdateGroupControlStates();

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::UpdateGroupControlStates()
        {
        const bool hasGroup = !m_groupVariable.empty();
        const bool showHeaders = hasGroup && m_showGroupHeadersCheck->IsChecked();

        m_groupColumnCountLabel->Enable(hasGroup);
        m_groupColumnCountSpin->Enable(hasGroup);
        m_showGroupHeadersCheck->Enable(hasGroup);
        m_groupHeaderPrefixLabel->Enable(showHeaders);
        m_groupHeaderPrefixText->Enable(showHeaders);
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertHeatMapDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
        {
        Data::Dataset::ColumnPreviewInfo info;

        for (const auto& col : dataset.GetContinuousColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::Numeric, wxString{});
            }
        for (const auto& col : dataset.GetCategoricalColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::String, wxString{});
            }
        for (const auto& col : dataset.GetDateColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::Date, wxString{});
            }

        return info;
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> InsertHeatMapDlg::GetSelectedDataset() const
        {
        if (GetReportBuilder() == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }

        const auto& datasets = GetReportBuilder()->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    bool InsertHeatMapDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_continuousVariable.empty())
            {
            wxMessageBox(_(L"Please select a continuous variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertHeatMapDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* heatmap = dynamic_cast<const Graphs::HeatMap*>(&graph);
        if (heatmap == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = heatmap->GetPropertyTemplate(L"dataset");
        if (!dsName.empty() && m_datasetChoice != nullptr)
            {
            for (size_t i = 0; i < m_datasetNames.size(); ++i)
                {
                if (m_datasetNames[i] == dsName)
                    {
                    m_datasetChoice->SetSelection(static_cast<int>(i));
                    break;
                    }
                }
            }

        // load column names from property templates
        m_continuousVariable = heatmap->GetPropertyTemplate(L"variables.continuous");
        m_groupVariable = heatmap->GetPropertyTemplate(L"variables.group");
        UpdateVariableLabels();

        // group options
        if (heatmap->GetGroupColumnCount().has_value())
            {
            m_groupColumnCount = static_cast<int>(heatmap->GetGroupColumnCount().value());
            }
        m_showGroupHeaders = heatmap->IsShowingGroupHeaders();
        m_groupHeaderPrefix = heatmap->GetGroupHeaderPrefix();

        TransferDataToWindow();

        UpdateGroupControlStates();
        }
    } // namespace Wisteria::UI
