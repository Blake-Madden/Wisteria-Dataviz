///////////////////////////////////////////////////////////////////////////////
// Name:        insertwilmarthbridgeplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwilmarthbridgeplotdlg.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWilmarthBridgePlotDlg::InsertWilmarthBridgePlotDlg(
        Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
        const wxString& caption, const wxWindowID id, const wxPoint& pos, const wxSize& size,
        const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Wilmarth Bridge Plot"), ID_OPTIONS_SECTION,
                                  true);

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

        const auto addVarRow = [varGrid, varsBox](const wxString& caption)
        {
            auto* rowLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, caption);
            rowLabel->SetFont(rowLabel->GetFont().Bold());
            varGrid->Add(rowLabel, wxSizerFlags{}.CenterVertical());
            auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
            valueLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
            varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
            return valueLabel;
        };

        m_labelVarLabel = addVarRow(_(L"Label:"));
        m_entryVarLabel = addVarRow(_(L"Entry:"));
        m_exitVarLabel = addVarRow(_(L"Exit:"));
        m_statusVarLabel = addVarRow(_(L"Status:"));
        m_intermediateEventVarLabel = addVarRow(_(L"Intermediate event:"));

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // display options
        auto* displaySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Fade effect:")),
                          wxSizerFlags{}.CenterVertical());
        wxArrayString fadeChoices;
        fadeChoices.Add(_(L"None"));
        fadeChoices.Add(_(L"Remaining lifetime"));
        fadeChoices.Add(_(L"Elapsed time"));
        displaySizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       fadeChoices, 0,
                                       wxGenericValidator{ &m_fadeEffectSelection }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Survival statistics:")),
                          wxSizerFlags{}.CenterVertical());
        wxArrayString survivalChoices;
        survivalChoices.Add(_(L"None"));
        survivalChoices.Add(_(L"At-risk count"));
        survivalChoices.Add(_(L"Survival percent"));
        survivalChoices.Add(_(L"Both"));
        displaySizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       survivalChoices, 0,
                                       wxGenericValidator{ &m_survivalDisplaySelection }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Terminal row label:")),
                          wxSizerFlags{}.CenterVertical());
        displaySizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0,
                                         wxGenericValidator{ &m_terminalRowLabel }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Intermediate event color:")),
                          wxSizerFlags{}.CenterVertical());
        m_intermediateEventColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::Emerald));
        displaySizer->Add(m_intermediateEventColorPicker, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(displaySizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show censored markers"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showCensoredMarkers }),
                          wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage(false, 4);
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_exitVariable.clear();
        m_entryVariable.clear();
        m_statusVariable.clear();
        m_intermediateEventVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::OnSelectVariables()
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
                  .Label(_(L"Label"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_labelVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_labelVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete,
                                   Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Entry"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_entryVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_entryVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::Date }),
              VLI{}
                  .Label(_(L"Exit"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_exitVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_exitVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::Date }),
              VLI{}
                  .Label(_(L"Status"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_statusVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_statusVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Intermediate Event"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_intermediateEventVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_intermediateEventVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto entryVars = dlg.GetSelectedVariables(1);
        m_entryVariable = entryVars.empty() ? wxString{} : entryVars.front();

        const auto exitVars = dlg.GetSelectedVariables(2);
        m_exitVariable = exitVars.empty() ? wxString{} : exitVars.front();

        const auto statusVars = dlg.GetSelectedVariables(3);
        m_statusVariable = statusVars.empty() ? wxString{} : statusVars.front();

        const auto intermediateEventVars = dlg.GetSelectedVariables(4);
        m_intermediateEventVariable =
            intermediateEventVars.empty() ? wxString{} : intermediateEventVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_exitVarLabel->SetLabel(m_exitVariable);
        m_entryVarLabel->SetLabel(m_entryVariable);
        m_statusVarLabel->SetLabel(m_statusVariable);
        m_intermediateEventVarLabel->SetLabel(m_intermediateEventVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertWilmarthBridgePlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertWilmarthBridgePlotDlg::GetSelectedDataset() const
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
    wxColour InsertWilmarthBridgePlotDlg::GetIntermediateEventColor() const
        {
        return (m_intermediateEventColorPicker != nullptr) ?
                   m_intermediateEventColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Emerald);
        }

    //-------------------------------------------
    Graphs::WilmarthBridgePlot::FadeEffect
    InsertWilmarthBridgePlotDlg::GetFadeEffect() const noexcept
        {
        switch (m_fadeEffectSelection)
            {
        case 1:
            return Graphs::WilmarthBridgePlot::FadeEffect::RemainingLifetime;
        case 2:
            return Graphs::WilmarthBridgePlot::FadeEffect::ElapsedTime;
        default:
            return Graphs::WilmarthBridgePlot::FadeEffect::None;
            }
        }

    //-------------------------------------------
    Graphs::WilmarthBridgePlot::SurvivalDisplay
    InsertWilmarthBridgePlotDlg::GetSurvivalDisplay() const noexcept
        {
        switch (m_survivalDisplaySelection)
            {
        case 1:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::AtRiskCount;
        case 2:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::SurvivalPercent;
        case 3:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::Both;
        default:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::None;
            }
        }

    //-------------------------------------------
    bool InsertWilmarthBridgePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_exitVariable.empty())
            {
            wxMessageBox(_(L"Please select the label and exit variables."),
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
    void InsertWilmarthBridgePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* bridgePlot = dynamic_cast<const Graphs::WilmarthBridgePlot*>(&graph);
        if (bridgePlot == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = bridgePlot->GetPropertyTemplate(L"dataset");
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
        m_labelVariable = bridgePlot->GetLabelColumnName();
        m_exitVariable = bridgePlot->GetExitColumnName();
        m_entryVariable = bridgePlot->GetEntryColumnName();
        m_statusVariable = bridgePlot->GetStatusColumnName();
        m_intermediateEventVariable = bridgePlot->GetIntermediateEventColumnName();
        UpdateVariableLabels();

        if (m_intermediateEventColorPicker != nullptr)
            {
            m_intermediateEventColorPicker->SetColour(bridgePlot->GetIntermediateEventColor());
            }

        // display options
        switch (bridgePlot->GetFadeEffect())
            {
        case Graphs::WilmarthBridgePlot::FadeEffect::RemainingLifetime:
            m_fadeEffectSelection = 1;
            break;
        case Graphs::WilmarthBridgePlot::FadeEffect::ElapsedTime:
            m_fadeEffectSelection = 2;
            break;
        case Graphs::WilmarthBridgePlot::FadeEffect::None:
            m_fadeEffectSelection = 0;
            break;
            }

        switch (bridgePlot->GetSurvivalDisplay())
            {
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::AtRiskCount:
            m_survivalDisplaySelection = 1;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::SurvivalPercent:
            m_survivalDisplaySelection = 2;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::Both:
            m_survivalDisplaySelection = 3;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::None:
            m_survivalDisplaySelection = 0;
            break;
            }

        m_showCensoredMarkers = bridgePlot->IsShowingCensoredMarkers();

        const auto terminalRowTmpl = bridgePlot->GetPropertyTemplate(L"terminal-row-label");
        m_terminalRowLabel =
            terminalRowTmpl.empty() ? bridgePlot->GetTerminalRowLabel() : terminalRowTmpl;

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
