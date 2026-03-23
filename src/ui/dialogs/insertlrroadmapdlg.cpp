///////////////////////////////////////////////////////////////////////////////
// Name:        insertlrroadmapdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlrroadmapdlg.h"
#include "../../graphs/lrroadmap.h"
#include "variableselectdlg.h"
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/valtext.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLRRoadmapDlg::InsertLRRoadmapDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                           wxWindow* parent, const wxString& caption,
                                           const wxWindowID id, const wxPoint& pos,
                                           const wxSize& size, const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertLRRoadmapDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"LR Roadmap Options"), ID_OPTIONS_SECTION, true);

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
        auto* varButton = new wxButton(optionsPage, ID_SELECT_VARS_BUTTON, _(L"Variables..."));
        optionsSizer->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* predLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Predictor:"));
        predLabel->SetFont(predLabel->GetFont().Bold());
        varGrid->Add(predLabel, wxSizerFlags{}.CenterVertical());
        m_predictorVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_predictorVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_predictorVarLabel, wxSizerFlags{}.CenterVertical());

        auto* coefLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Coefficient:"));
        coefLabel->SetFont(coefLabel->GetFont().Bold());
        varGrid->Add(coefLabel, wxSizerFlags{}.CenterVertical());
        m_coefficientVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_coefficientVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_coefficientVarLabel, wxSizerFlags{}.CenterVertical());

        auto* pvalLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"P-value:"));
        pvalLabel->SetFont(pvalLabel->GetFont().Bold());
        varGrid->Add(pvalLabel, wxSizerFlags{}.CenterVertical());
        m_pValueVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_pValueVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_pValueVarLabel, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(varGrid, wxSizerFlags{}.Border());

        // dependent variable name
        auto* dvSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        dvSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dependent variable:")),
                     wxSizerFlags{}.CenterVertical());
        dvSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                    wxDefaultSize, 0, wxTextValidator(wxFILTER_NONE, &m_dvName)));
        optionsSizer->Add(dvSizer, wxSizerFlags{}.Border());

        // predictors filter
        auto* filterSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        filterSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Show predictors:")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* filterChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_predictorsFilter));
            filterChoice->Append(_(L"All"));
            filterChoice->Append(_(L"Positive only"));
            filterChoice->Append(_(L"Negative only"));
            filterSizer->Add(filterChoice);
            }
        optionsSizer->Add(filterSizer, wxSizerFlags{}.Border());

        // p-value threshold
        auto* pLevelSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        pLevelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"P-value threshold:")),
                         wxSizerFlags{}.CenterVertical());
        m_pLevelSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                            wxDefaultSize, wxSP_ARROW_KEYS, 0.001, 1.0, 0.05, 0.01);
        pLevelSizer->Add(m_pLevelSpin);
        optionsSizer->Add(pLevelSizer, wxSizerFlags{}.Border());

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
    void InsertLRRoadmapDlg::OnDatasetChanged()
        {
        m_predictorVariable.clear();
        m_coefficientVariable.clear();
        m_pValueVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLRRoadmapDlg::OnSelectVariables()
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
                  .Label(_(L"Predictor"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_predictorVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_predictorVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Coefficient"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_coefficientVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_coefficientVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"p-value"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_pValueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_pValueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto predVars = dlg.GetSelectedVariables(0);
        m_predictorVariable = predVars.empty() ? wxString{} : predVars.front();

        const auto coefVars = dlg.GetSelectedVariables(1);
        m_coefficientVariable = coefVars.empty() ? wxString{} : coefVars.front();

        const auto pvalVars = dlg.GetSelectedVariables(2);
        m_pValueVariable = pvalVars.empty() ? wxString{} : pvalVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLRRoadmapDlg::UpdateVariableLabels()
        {
        m_predictorVarLabel->SetLabel(m_predictorVariable);
        m_coefficientVarLabel->SetLabel(m_coefficientVariable);
        m_pValueVarLabel->SetLabel(m_pValueVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertLRRoadmapDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertLRRoadmapDlg::GetSelectedDataset() const
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
    std::optional<double> InsertLRRoadmapDlg::GetPLevel() const
        {
        if (m_pValueVariable.empty())
            {
            return std::nullopt;
            }
        return m_pLevelSpin->GetValue();
        }

    //-------------------------------------------
    std::optional<Influence> InsertLRRoadmapDlg::GetPredictorsToInclude() const
        {
        switch (m_predictorsFilter)
            {
        case 1:
            return InfluencePositive;
        case 2:
            return InfluenceNegative;
        default:
            return std::nullopt;
            }
        }

    //-------------------------------------------
    bool InsertLRRoadmapDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_predictorVariable.empty() || m_coefficientVariable.empty())
            {
            wxMessageBox(_(L"Please select the predictor and coefficient variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertLRRoadmapDlg::LoadFromGraph(const Graphs::Graph2D& graph, Canvas* canvas)
        {
        const auto* roadmap = dynamic_cast<const Graphs::LRRoadmap*>(&graph);
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
        m_predictorVariable = roadmap->GetPredictorColumnName();
        m_coefficientVariable = roadmap->GetCoefficientColumnName();
        m_pValueVariable = roadmap->GetPValueColumnName();
        UpdateVariableLabels();

        // dependent variable name from the goal label
        m_dvName = roadmap->GetGoalLabel();

        // p-value threshold
        const auto pThreshold = roadmap->GetPValueThreshold();
        if (pThreshold.has_value())
            {
            m_pLevelSpin->SetValue(pThreshold.value());
            }

        // predictors filter
        const auto pred = roadmap->GetPredictorsToInclude();
        if (!pred.has_value() || pred.value() == InfluenceAll)
            {
            m_predictorsFilter = 0;
            }
        else if (pred.value() == InfluencePositive)
            {
            m_predictorsFilter = 1;
            }
        else if (pred.value() == InfluenceNegative)
            {
            m_predictorsFilter = 2;
            }

        m_addDefaultCaption = roadmap->HasDefaultCaption();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
