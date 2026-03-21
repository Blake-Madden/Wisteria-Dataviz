///////////////////////////////////////////////////////////////////////////////
// Name:        insertpiechartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpiechartdlg.h"
#include "../../graphs/piechart.h"
#include "insertlabeldlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPieChartDlg::InsertPieChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertPieChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Pie Chart Options"), ID_OPTIONS_SECTION, true);

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

        auto* groupLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Group (outer):"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        auto* weightLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Weight:"));
        weightLabel->SetFont(weightLabel->GetFont().Bold());
        varGrid->Add(weightLabel, wxSizerFlags{}.CenterVertical());
        m_weightVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_weightVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_weightVarLabel, wxSizerFlags{}.CenterVertical());

        auto* group2Label = new wxStaticText(optionsPage, wxID_ANY, _(L"Subgroup (inner):"));
        group2Label->SetFont(group2Label->GetFont().Bold());
        varGrid->Add(group2Label, wxSizerFlags{}.CenterVertical());
        m_group2VarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_group2VarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_group2VarLabel, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(varGrid, wxSizerFlags{}.Border());

        // pie style
        auto* styleSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Style:")),
                        wxSizerFlags{}.CenterVertical());
            {
            auto* styleChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_pieStyle));
            styleChoice->Append(_(L"None"));
            styleChoice->Append(_(L"Clockface"));
            styleChoice->Append(_(L"Cheese pizza"));
            styleChoice->Append(_(L"Pepperoni pizza"));
            styleChoice->Append(_(L"Hawaiian pizza"));
            styleChoice->Append(_(L"Coffee ring"));
            styleChoice->Append(_(L"Venus"));
            styleChoice->Append(_(L"Mars"));
            styleChoice->Append(_(L"Chocolate chip cookie"));
            styleSizer->Add(styleChoice);
            }
        optionsSizer->Add(styleSizer, wxSizerFlags{}.Border());

        // labels area
        auto* labelsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Labels"));

        auto* labelGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        // outer midpoint labels
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Outer midpoint:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* midPointChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_outerMidPointDisplay));
            midPointChoice->Append(_(L"Value"));
            midPointChoice->Append(_(L"Percentage"));
            midPointChoice->Append(_(L"Value & percentage"));
            midPointChoice->Append(_(L"No labels"));
            midPointChoice->Append(_(L"Name"));
            midPointChoice->Append(_(L"Name & value"));
            midPointChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(midPointChoice);
            }

        // outer labels (gutter)
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Outer:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* outerLabelChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_outerLabelDisplay));
            outerLabelChoice->Append(_(L"Value"));
            outerLabelChoice->Append(_(L"Percentage"));
            outerLabelChoice->Append(_(L"Value & percentage"));
            outerLabelChoice->Append(_(L"No labels"));
            outerLabelChoice->Append(_(L"Name"));
            outerLabelChoice->Append(_(L"Name & value"));
            outerLabelChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(outerLabelChoice);
            }

        // inner midpoint labels
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Inner midpoint:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* innerMidChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_innerMidPointDisplay));
            innerMidChoice->Append(_(L"Value"));
            innerMidChoice->Append(_(L"Percentage"));
            innerMidChoice->Append(_(L"Value & percentage"));
            innerMidChoice->Append(_(L"No labels"));
            innerMidChoice->Append(_(L"Name"));
            innerMidChoice->Append(_(L"Name & value"));
            innerMidChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(innerMidChoice);
            }

        // label placement
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Placement:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* placementChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_labelPlacement));
            placementChoice->Append(_(L"Next to slice"));
            placementChoice->Append(_(L"Flush"));
            labelGrid->Add(placementChoice);
            }

        labelsBox->Add(labelGrid, wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Show outer pie labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_showOuterPieLabels)),
                       wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Show inner pie labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_showInnerPieLabels)),
                       wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Color-matched labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_useColorLabels)),
                       wxSizerFlags{}.Border());

        optionsSizer->Add(labelsBox, wxSizerFlags{}.Expand().Border());

        // donut hole area
        auto* donutBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Donut Hole"));

        auto* donutCheckBox = new wxCheckBox(
            donutBox->GetStaticBox(), wxID_ANY, _(L"Include donut hole"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator(&m_includeDonutHole));
        donutBox->Add(donutCheckBox, wxSizerFlags{}.Border());

        m_editDonutLabelButton =
            new wxButton(donutBox->GetStaticBox(), wxID_ANY, _(L"Edit Label..."));
        m_editDonutLabelButton->Enable(m_includeDonutHole);
        donutBox->Add(m_editDonutLabelButton, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));

        optionsSizer->Add(donutBox, wxSizerFlags{}.Expand().Border());

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        donutCheckBox->Bind(wxEVT_CHECKBOX,
                            [this]([[maybe_unused]] wxCommandEvent&)
                            {
                                TransferDataFromWindow();
                                m_editDonutLabelButton->Enable(m_includeDonutHole);
                            });

        m_editDonutLabelButton->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                     { OnEditDonutHoleLabel(); });
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnDatasetChanged()
        {
        m_groupVariable.clear();
        m_weightVariable.clear();
        m_group2Variable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnSelectVariables()
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
                  .Label(_(L"Group (outer)"))
                  .SingleSelection(true)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete,
                                   Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Weight"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_weightVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_weightVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Subgroup (inner)"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_group2Variable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_group2Variable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete,
                                   Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto groupVars = dlg.GetSelectedVariables(0);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        const auto weightVars = dlg.GetSelectedVariables(1);
        m_weightVariable = weightVars.empty() ? wxString{} : weightVars.front();

        const auto group2Vars = dlg.GetSelectedVariables(2);
        m_group2Variable = group2Vars.empty() ? wxString{} : group2Vars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertPieChartDlg::UpdateVariableLabels()
        {
        m_groupVarLabel->SetLabel(m_groupVariable);
        m_weightVarLabel->SetLabel(m_weightVariable);
        m_group2VarLabel->SetLabel(m_group2Variable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertPieChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertPieChartDlg::GetSelectedDataset() const
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
    bool InsertPieChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_groupVariable.empty())
            {
            wxMessageBox(_(L"Please select the grouping variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnEditDonutHoleLabel()
        {
        InsertLabelDlg dlg(GetCanvas(), nullptr, this, _(L"Edit Donut Hole Label"), wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           EditMode::Edit, false);
        dlg.LoadFromLabel(m_donutHoleLabel, GetCanvas());

        if (dlg.ShowModal() == wxID_OK)
            {
            dlg.ApplyToLabel(m_donutHoleLabel);
            }
        }

    //-------------------------------------------
    void InsertPieChartDlg::LoadFromGraph(const Graphs::Graph2D& graph, Canvas* canvas)
        {
        const auto* pieChart = dynamic_cast<const Graphs::PieChart*>(&graph);
        if (pieChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph, canvas);

        // select the dataset by name from the property template
        const auto dsName = pieChart->GetPropertyTemplate(L"dataset");
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
        m_groupVariable = pieChart->GetGroupColumn1Name();
        m_weightVariable = pieChart->GetWeightColumnName();
        m_group2Variable = pieChart->GetGroupColumn2Name();
        UpdateVariableLabels();

        m_includeDonutHole = pieChart->IsIncludingDonutHole();
        m_showOuterPieLabels = pieChart->IsShowingOuterPieLabels();
        m_showInnerPieLabels = pieChart->IsShowingInnerPieLabels();
        m_useColorLabels = pieChart->IsUsingColorLabels();
        m_outerMidPointDisplay = static_cast<int>(pieChart->GetOuterPieMidPointLabelDisplay());
        m_outerLabelDisplay = static_cast<int>(pieChart->GetOuterLabelDisplay());
        m_innerMidPointDisplay = static_cast<int>(pieChart->GetInnerPieMidPointLabelDisplay());
        m_labelPlacement = static_cast<int>(pieChart->GetLabelPlacement());
        m_pieStyle = static_cast<int>(pieChart->GetPieStyle());

        m_donutHoleLabel = pieChart->GetDonutHoleLabel();

        TransferDataToWindow();
        m_editDonutLabelButton->Enable(m_includeDonutHole);
        }
    } // namespace Wisteria::UI
