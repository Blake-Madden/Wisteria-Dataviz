///////////////////////////////////////////////////////////////////////////////
// Name:        insertsankeydiagramdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertsankeydiagramdlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertSankeyDiagramDlg::InsertSankeyDiagramDlg(Canvas* canvas,
                                                   const ReportBuilder* reportBuilder,
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
    void InsertSankeyDiagramDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Sankey Diagram"), ID_OPTIONS_SECTION,
                                  true);

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

        auto* fromLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"From:"));
        fromLabel->SetFont(fromLabel->GetFont().Bold());
        varGrid->Add(fromLabel, wxSizerFlags{}.CenterVertical());
        m_fromVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_fromVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_fromVarLabel, wxSizerFlags{}.CenterVertical());

        auto* toLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"To:"));
        toLabel->SetFont(toLabel->GetFont().Bold());
        varGrid->Add(toLabel, wxSizerFlags{}.CenterVertical());
        m_toVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_toVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_toVarLabel, wxSizerFlags{}.CenterVertical());

        auto* fromWeightLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"From weight:"));
        fromWeightLabel->SetFont(fromWeightLabel->GetFont().Bold());
        varGrid->Add(fromWeightLabel, wxSizerFlags{}.CenterVertical());
        m_fromWeightVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_fromWeightVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_fromWeightVarLabel, wxSizerFlags{}.CenterVertical());

        auto* toWeightLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"To weight:"));
        toWeightLabel->SetFont(toWeightLabel->GetFont().Bold());
        varGrid->Add(toWeightLabel, wxSizerFlags{}.CenterVertical());
        m_toWeightVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_toWeightVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_toWeightVarLabel, wxSizerFlags{}.CenterVertical());

        auto* fromGroupLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"From group:"));
        fromGroupLabel->SetFont(fromGroupLabel->GetFont().Bold());
        varGrid->Add(fromGroupLabel, wxSizerFlags{}.CenterVertical());
        m_fromGroupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_fromGroupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_fromGroupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // flow shape
        auto* flowSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        flowSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Flow shape:")),
                       wxSizerFlags{}.CenterVertical());
        wxArrayString flowShapes;
        flowShapes.Add(_(L"Curvy"));
        flowShapes.Add(_(L"Jagged"));
        flowSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    flowShapes, 0, wxGenericValidator(&m_flowShapeIndex)),
                       wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(flowSizer, wxSizerFlags{}.Border());

        // group label display
        auto* glSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        glSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Group labels:")),
                     wxSizerFlags{}.CenterVertical());
        wxArrayString groupLabelDisplays;
        groupLabelDisplays.Add(_(L"Name"));
        groupLabelDisplays.Add(_(L"Value"));
        groupLabelDisplays.Add(_(L"Percentage"));
        groupLabelDisplays.Add(_(L"Name & Value"));
        groupLabelDisplays.Add(_(L"Name & Percentage"));
        groupLabelDisplays.Add(_(L"Value & Percentage"));
        groupLabelDisplays.Add(_(L"No Display"));
        glSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  groupLabelDisplays, 0,
                                  wxGenericValidator(&m_groupLabelDisplayIndex)),
                     wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(glSizer, wxSizerFlags{}.Border());

        // column header display
        auto* chSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        chSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Column headers:")),
                     wxSizerFlags{}.CenterVertical());
        wxArrayString columnHeaderDisplays;
        columnHeaderDisplays.Add(_(L"No Display"));
        columnHeaderDisplays.Add(_(L"As Header"));
        columnHeaderDisplays.Add(_(L"As Footer"));
        chSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  columnHeaderDisplays, 0,
                                  wxGenericValidator(&m_columnHeaderDisplayIndex)),
                     wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(chSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertSankeyDiagramDlg::OnDatasetChanged()
        {
        m_fromVariable.clear();
        m_toVariable.clear();
        m_fromWeightVariable.clear();
        m_toWeightVariable.clear();
        m_fromGroupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertSankeyDiagramDlg::OnSelectVariables()
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
                  .Label(_(L"From"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_fromVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_fromVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"To"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_toVariable.empty() ? std::vector<wxString>{} :
                                                           std::vector<wxString>{ m_toVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"From Weight"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_fromWeightVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_fromWeightVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"To Weight"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_toWeightVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_toWeightVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"From Group"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_fromGroupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_fromGroupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto fromVars = dlg.GetSelectedVariables(0);
        m_fromVariable = fromVars.empty() ? wxString{} : fromVars.front();

        const auto toVars = dlg.GetSelectedVariables(1);
        m_toVariable = toVars.empty() ? wxString{} : toVars.front();

        const auto fromWeightVars = dlg.GetSelectedVariables(2);
        m_fromWeightVariable = fromWeightVars.empty() ? wxString{} : fromWeightVars.front();

        const auto toWeightVars = dlg.GetSelectedVariables(3);
        m_toWeightVariable = toWeightVars.empty() ? wxString{} : toWeightVars.front();

        const auto fromGroupVars = dlg.GetSelectedVariables(4);
        m_fromGroupVariable = fromGroupVars.empty() ? wxString{} : fromGroupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertSankeyDiagramDlg::UpdateVariableLabels()
        {
        m_fromVarLabel->SetLabel(m_fromVariable);
        m_toVarLabel->SetLabel(m_toVariable);
        m_fromWeightVarLabel->SetLabel(m_fromWeightVariable);
        m_toWeightVarLabel->SetLabel(m_toWeightVariable);
        m_fromGroupVarLabel->SetLabel(m_fromGroupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertSankeyDiagramDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertSankeyDiagramDlg::GetSelectedDataset() const
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
    bool InsertSankeyDiagramDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_fromVariable.empty() || m_toVariable.empty())
            {
            wxMessageBox(_(L"Please select the From and To variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        // weight columns must be paired
        if ((!m_fromWeightVariable.empty() && m_toWeightVariable.empty()) ||
            (m_fromWeightVariable.empty() && !m_toWeightVariable.empty()))
            {
            wxMessageBox(_(L"Both From Weight and To Weight must be specified together."),
                         _(L"Weight Variables"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertSankeyDiagramDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* sankey = dynamic_cast<const Graphs::SankeyDiagram*>(&graph);
        if (sankey == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = sankey->GetPropertyTemplate(L"dataset");
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
        m_fromVariable = sankey->GetPropertyTemplate(L"variables.from");
        m_toVariable = sankey->GetPropertyTemplate(L"variables.to");
        m_fromWeightVariable = sankey->GetPropertyTemplate(L"variables.from-weight");
        m_toWeightVariable = sankey->GetPropertyTemplate(L"variables.to-weight");
        m_fromGroupVariable = sankey->GetPropertyTemplate(L"variables.from-group");
        UpdateVariableLabels();

        // flow shape
        m_flowShapeIndex = (sankey->GetFlowShape() == FlowShape::Jagged) ? 1 : 0;

        // group label display
        switch (sankey->GetGroupLabelDisplay())
            {
        case BinLabelDisplay::BinValue:
            m_groupLabelDisplayIndex = 1;
            break;
        case BinLabelDisplay::BinPercentage:
            m_groupLabelDisplayIndex = 2;
            break;
        case BinLabelDisplay::BinNameAndValue:
            m_groupLabelDisplayIndex = 3;
            break;
        case BinLabelDisplay::BinNameAndPercentage:
            m_groupLabelDisplayIndex = 4;
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            m_groupLabelDisplayIndex = 5;
            break;
        case BinLabelDisplay::NoDisplay:
            m_groupLabelDisplayIndex = 6;
            break;
        case BinLabelDisplay::BinName:
            [[fallthrough]];
        default:
            m_groupLabelDisplayIndex = 0;
            break;
            }

        // column header display
        switch (sankey->GetColumnHeaderDisplay())
            {
        case GraphColumnHeader::AsHeader:
            m_columnHeaderDisplayIndex = 1;
            break;
        case GraphColumnHeader::AsFooter:
            m_columnHeaderDisplayIndex = 2;
            break;
        case GraphColumnHeader::NoDisplay:
            [[fallthrough]];
        default:
            m_columnHeaderDisplayIndex = 0;
            break;
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
