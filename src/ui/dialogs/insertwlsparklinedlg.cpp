///////////////////////////////////////////////////////////////////////////////
// Name:        insertwlsparklinedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwlsparklinedlg.h"
#include "../../graphs/win_loss_sparkline.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWLSparklineDlg::InsertWLSparklineDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertWLSparklineDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Win/Loss Sparkline"), ID_OPTIONS_SECTION, true);

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

        auto* seasonLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Season:"));
        seasonLabel->SetFont(seasonLabel->GetFont().Bold());
        varGrid->Add(seasonLabel, wxSizerFlags{}.CenterVertical());
        m_seasonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_seasonVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_seasonVarLabel, wxSizerFlags{}.CenterVertical());

        auto* wonLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Won/Lost:"));
        wonLabel->SetFont(wonLabel->GetFont().Bold());
        varGrid->Add(wonLabel, wxSizerFlags{}.CenterVertical());
        m_wonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_wonVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_wonVarLabel, wxSizerFlags{}.CenterVertical());

        auto* shutoutLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Shutout:"));
        shutoutLabel->SetFont(shutoutLabel->GetFont().Bold());
        varGrid->Add(shutoutLabel, wxSizerFlags{}.CenterVertical());
        m_shutoutVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_shutoutVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_shutoutVarLabel, wxSizerFlags{}.CenterVertical());

        auto* homeLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Home game:"));
        homeLabel->SetFont(homeLabel->GetFont().Bold());
        varGrid->Add(homeLabel, wxSizerFlags{}.CenterVertical());
        m_homeGameVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_homeGameVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_homeGameVarLabel, wxSizerFlags{}.CenterVertical());

        auto* postLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Postseason:"));
        postLabel->SetFont(postLabel->GetFont().Bold());
        varGrid->Add(postLabel, wxSizerFlags{}.CenterVertical());
        m_postseasonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_postseasonVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_postseasonVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // highlight best records
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Highlight best records"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_highlightBestRecords)),
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
    void InsertWLSparklineDlg::OnDatasetChanged()
        {
        m_seasonVariable.clear();
        m_wonVariable.clear();
        m_shutoutVariable.clear();
        m_homeGameVariable.clear();
        m_postseasonVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::OnSelectVariables()
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
                  .Label(_(L"Season"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_seasonVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_seasonVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Won/Lost"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_wonVariable.empty() ? std::vector<wxString>{} :
                                                            std::vector<wxString>{ m_wonVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Shutout"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_shutoutVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_shutoutVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Home game"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_homeGameVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_homeGameVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Postseason"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_postseasonVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_postseasonVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto seasonVars = dlg.GetSelectedVariables(0);
        m_seasonVariable = seasonVars.empty() ? wxString{} : seasonVars.front();

        const auto wonVars = dlg.GetSelectedVariables(1);
        m_wonVariable = wonVars.empty() ? wxString{} : wonVars.front();

        const auto shutoutVars = dlg.GetSelectedVariables(2);
        m_shutoutVariable = shutoutVars.empty() ? wxString{} : shutoutVars.front();

        const auto homeVars = dlg.GetSelectedVariables(3);
        m_homeGameVariable = homeVars.empty() ? wxString{} : homeVars.front();

        const auto postVars = dlg.GetSelectedVariables(4);
        m_postseasonVariable = postVars.empty() ? wxString{} : postVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::UpdateVariableLabels()
        {
        m_seasonVarLabel->SetLabel(m_seasonVariable);
        m_wonVarLabel->SetLabel(m_wonVariable);
        m_shutoutVarLabel->SetLabel(m_shutoutVariable);
        m_homeGameVarLabel->SetLabel(m_homeGameVariable);
        m_postseasonVarLabel->SetLabel(m_postseasonVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertWLSparklineDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertWLSparklineDlg::GetSelectedDataset() const
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
    bool InsertWLSparklineDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_seasonVariable.empty() || m_wonVariable.empty() || m_shutoutVariable.empty() ||
            m_homeGameVariable.empty())
            {
            wxMessageBox(
                _(L"Please select the season, won/lost, shutout, and home game variables."),
                _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* sparkline = dynamic_cast<const Graphs::WinLossSparkline*>(&graph);
        if (sparkline == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = sparkline->GetPropertyTemplate(L"dataset");
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
        m_seasonVariable = sparkline->GetSeasonColumnName();
        m_wonVariable = sparkline->GetWonColumnName();
        m_shutoutVariable = sparkline->GetShutoutColumnName();
        m_homeGameVariable = sparkline->GetHomeGameColumnName();
        m_postseasonVariable = sparkline->GetPostseasonColumnName();
        UpdateVariableLabels();

        m_highlightBestRecords = sparkline->IsHighlightingBestRecords();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
