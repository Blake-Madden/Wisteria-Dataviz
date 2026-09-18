///////////////////////////////////////////////////////////////////////////////
// Name:        insertfunnelchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertfunnelchartdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertFunnelChartDlg::InsertFunnelChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Funnel Chart"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

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

        auto* stageLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Stage:"));
        stageLabel->SetFont(stageLabel->GetFont().Bold());
        varGrid->Add(stageLabel, wxSizerFlags{}.CenterVertical());
        m_stageVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_stageVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_stageVarLabel, wxSizerFlags{}.CenterVertical());

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* targetLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Target (optional):"));
        targetLabel->SetFont(targetLabel->GetFont().Bold());
        varGrid->Add(targetLabel, wxSizerFlags{}.CenterVertical());
        m_targetVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_targetVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_targetVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // style and display options
        auto* styleSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Funnel style:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString funnelStyleChoices;
        funnelStyleChoices.Add(_(L"Glassy"));
        funnelStyleChoices.Add(_(L"Standard"));
        styleSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     funnelStyleChoices, 0,
                                     wxGenericValidator{ &m_funnelStyleIndex }));
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Target ghost opacity:")),
                        wxSizerFlags{}.CenterVertical());
            {
            auto* opacitySpin = new wxSpinCtrl(optionsPage, wxID_ANY);
            opacitySpin->SetRange(0, 255);
            opacitySpin->SetValue(m_targetGhostOpacity);
            opacitySpin->SetValidator(wxGenericValidator{ &m_targetGhostOpacity });
            styleSizer->Add(opacitySpin);
            }
        optionsSizer->Add(styleSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show explanations"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showExplanations }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show conversion labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showConversionLabels }),
                          wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::OnDatasetChanged()
        {
        m_stageVariable.clear();
        m_valueVariable.clear();
        m_targetVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::OnSelectVariables()
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
                const auto foundPos = importOpts.find(m_datasetNames[sel]);
                if (foundPos != importOpts.cend())
                    {
                    columnInfo = foundPos->second.m_columnPreviewInfo;
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
                  .Label(_(L"Stage"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_stageVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_stageVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Value"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_valueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_valueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Target"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_targetVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_targetVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto stageVars = dlg.GetSelectedVariables(0);
        m_stageVariable = stageVars.empty() ? wxString{} : stageVars.front();

        const auto valueVars = dlg.GetSelectedVariables(1);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        const auto targetVars = dlg.GetSelectedVariables(2);
        m_targetVariable = targetVars.empty() ? wxString{} : targetVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::UpdateVariableLabels()
        {
        m_stageVarLabel->SetLabel(m_stageVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);
        m_targetVarLabel->SetLabel(m_targetVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertFunnelChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertFunnelChartDlg::GetSelectedDataset() const
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
        const auto foundPos = datasets.find(m_datasetNames[sel]);
        return (foundPos != datasets.cend()) ? foundPos->second : nullptr;
        }

    //-------------------------------------------
    bool InsertFunnelChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_stageVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the stage and value variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
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
    void InsertFunnelChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* funnelChart = dynamic_cast<const Graphs::FunnelChart*>(&graph);
        if (funnelChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = funnelChart->GetPropertyTemplate(L"dataset");
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

        // load column names from the graph
        m_stageVariable = funnelChart->GetStageColumnName();
        m_valueVariable = funnelChart->GetValueColumnName();
        m_targetVariable = funnelChart->GetTargetColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // style and display options
        m_funnelStyleIndex =
            (funnelChart->GetFunnelStyle() == Graphs::FunnelChart::FunnelStyle::Standard) ? 1 : 0;
        m_showExplanations = funnelChart->AreExplanationsShown();
        m_showConversionLabels = funnelChart->AreConversionLabelsShown();
        m_targetGhostOpacity = funnelChart->GetTargetGhostOpacity();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::FunnelChart>
    InsertFunnelChartDlg::BuildFunnelChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::FunnelChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetFunnelStyle(GetFunnelStyle());
        plot->ShowExplanations(IsShowingExplanations());
        plot->ShowConversionLabels(IsShowingConversionLabels());
        plot->SetTargetGhostOpacity(GetTargetGhostOpacity());

        plot->SetData(GetSelectedDataset(), GetStageVariable(), GetValueVariable(),
                      GetTargetVariable().empty() ? std::nullopt :
                                                    std::optional<wxString>{ GetTargetVariable() });

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::FunnelChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.stage", GetStageVariable(),
                oldChart != nullptr ? oldChart->GetStageColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.target", GetTargetVariable(),
                oldChart != nullptr ? oldChart->GetTargetColumnName().value_or(wxString{}) :
                                      wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.stage", GetStageVariable());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            plot->SetPropertyTemplate(L"variables.target", GetTargetVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
