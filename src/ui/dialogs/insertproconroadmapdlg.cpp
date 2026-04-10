///////////////////////////////////////////////////////////////////////////////
// Name:        insertproconroadmapdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertproconroadmapdlg.h"
#include "../../graphs/proconroadmap.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>
#include <wx/valtext.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertProConRoadmapDlg::InsertProConRoadmapDlg(Canvas* canvas,
                                                   const ReportBuilder* reportBuilder,
                                                   wxWindow* parent, const wxString& caption,
                                                   const wxWindowID id, const wxPoint& pos,
                                                   const wxSize& size, const long style,
                                                   EditMode editMode)
        : InsertGraphDlg(
              canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
              static_cast<GraphDlgOptions>(GraphDlgIncludeMost & ~GraphDlgIncludeColorScheme))
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertProConRoadmapDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Pro & Con Options"), ID_OPTIONS_SECTION, true);

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

        auto* posLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Positive (pros):"));
        posLabel->SetFont(posLabel->GetFont().Bold());
        varGrid->Add(posLabel, wxSizerFlags{}.CenterVertical());
        m_positiveVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_positiveVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_positiveVarLabel, wxSizerFlags{}.CenterVertical());

        auto* posValLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Positive values:"));
        posValLabel->SetFont(posValLabel->GetFont().Bold());
        varGrid->Add(posValLabel, wxSizerFlags{}.CenterVertical());
        m_positiveValueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_positiveValueVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_positiveValueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* negLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Negative (cons):"));
        negLabel->SetFont(negLabel->GetFont().Bold());
        varGrid->Add(negLabel, wxSizerFlags{}.CenterVertical());
        m_negativeVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_negativeVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_negativeVarLabel, wxSizerFlags{}.CenterVertical());

        auto* negValLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Negative values:"));
        negValLabel->SetFont(negValLabel->GetFont().Bold());
        varGrid->Add(negValLabel, wxSizerFlags{}.CenterVertical());
        m_negativeValueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_negativeValueVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_negativeValueVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // minimum count
        auto* minCountSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        minCountSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Minimum count:")),
                           wxSizerFlags{}.CenterVertical());
        m_minCountSpin = new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                        wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 0);
        minCountSizer->Add(m_minCountSpin);
        optionsSizer->Add(minCountSizer, wxSizerFlags{}.Border());

        // legend labels
        auto* labelSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        labelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Positive label:")),
                        wxSizerFlags{}.CenterVertical());
        labelSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, 0,
                                       wxTextValidator(wxFILTER_NONE, &m_positiveLabel)));
        labelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Negative label:")),
                        wxSizerFlags{}.CenterVertical());
        labelSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, 0,
                                       wxTextValidator(wxFILTER_NONE, &m_negativeLabel)));
        optionsSizer->Add(labelSizer, wxSizerFlags{}.Border());

        // default caption
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Add explanatory caption"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_addDefaultCaption)),
                          wxSizerFlags{}.Border());

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 4));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertProConRoadmapDlg::OnDatasetChanged()
        {
        m_positiveVariable.clear();
        m_positiveValueVariable.clear();
        m_negativeVariable.clear();
        m_negativeValueVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertProConRoadmapDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

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
                  .Label(_(L"Positive (pros)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_positiveVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_positiveVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Positive values"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_positiveValueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_positiveValueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Negative (cons)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_negativeVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_negativeVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Negative values"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_negativeValueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_negativeValueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto posVars = dlg.GetSelectedVariables(0);
        m_positiveVariable = posVars.empty() ? wxString{} : posVars.front();

        const auto posValVars = dlg.GetSelectedVariables(1);
        m_positiveValueVariable = posValVars.empty() ? wxString{} : posValVars.front();

        const auto negVars = dlg.GetSelectedVariables(2);
        m_negativeVariable = negVars.empty() ? wxString{} : negVars.front();

        const auto negValVars = dlg.GetSelectedVariables(3);
        m_negativeValueVariable = negValVars.empty() ? wxString{} : negValVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertProConRoadmapDlg::UpdateVariableLabels()
        {
        m_positiveVarLabel->SetLabel(m_positiveVariable);
        m_positiveValueVarLabel->SetLabel(m_positiveValueVariable);
        m_negativeVarLabel->SetLabel(m_negativeVariable);
        m_negativeValueVarLabel->SetLabel(m_negativeValueVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertProConRoadmapDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertProConRoadmapDlg::GetSelectedDataset() const
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
    std::optional<size_t> InsertProConRoadmapDlg::GetMinimumCount() const
        {
        const int val = m_minCountSpin->GetValue();
        return (val > 0) ? std::optional<size_t>(static_cast<size_t>(val)) : std::nullopt;
        }

    //-------------------------------------------
    bool InsertProConRoadmapDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_positiveVariable.empty() || m_negativeVariable.empty())
            {
            wxMessageBox(_(L"Please select the positive and negative variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertProConRoadmapDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* roadmap = dynamic_cast<const Graphs::ProConRoadmap*>(&graph);
        if (roadmap == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = roadmap->GetPropertyTemplate(L"dataset");
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

        // load the actual column names from the graph
        m_positiveVariable = roadmap->GetPositiveColumnName();
        m_positiveValueVariable = roadmap->GetPositiveValueColumnName();
        m_negativeVariable = roadmap->GetNegativeColumnName();
        m_negativeValueVariable = roadmap->GetNegativeValueColumnName();
        UpdateVariableLabels();

        // minimum count
        const auto minCount = roadmap->GetMinimumCount();
        if (minCount.has_value())
            {
            m_minCountSpin->SetValue(static_cast<int>(minCount.value()));
            }

        // legend labels
        m_positiveLabel = roadmap->GetPositiveLabel();
        m_negativeLabel = roadmap->GetNegativeLabel();

        m_addDefaultCaption = roadmap->HasDefaultCaption();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
