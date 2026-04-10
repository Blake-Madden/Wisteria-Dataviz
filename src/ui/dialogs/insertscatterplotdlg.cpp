///////////////////////////////////////////////////////////////////////////////
// Name:        insertscatterplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertscatterplotdlg.h"
#include "../../graphs/scatterplot.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertScatterPlotDlg::InsertScatterPlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(
              canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
              static_cast<GraphDlgOptions>(GraphDlgIncludeMost | GraphDlgIncludeShapeScheme))
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAxisOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Scatter Plot"), ID_OPTIONS_SECTION,
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

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"X (independent):"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Y (dependent):"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_yVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // regression options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show regression lines"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showRegressionLines)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show confidence bands"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showConfidenceBands)),
                          wxSizerFlags{}.Border());

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
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        // prefer the stored column preview info (preserves original file order)
        // over rebuilding it from the dataset's internal column grouping
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
                  .Label(_(L"X (independent)"))
                  .Required(true)
                  .DefaultVariables(m_xVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_xVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Y (dependent)"))
                  .Required(true)
                  .DefaultVariables(m_yVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_yVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Grouping"))
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

        const auto xVars = dlg.GetSelectedVariables(0);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        const auto yVars = dlg.GetSelectedVariables(1);
        m_yVariable = yVars.empty() ? wxString{} : yVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::UpdateVariableLabels()
        {
        m_xVarLabel->SetLabel(m_xVariable);
        m_yVarLabel->SetLabel(m_yVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertScatterPlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertScatterPlotDlg::GetSelectedDataset() const
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
    bool InsertScatterPlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_xVariable.empty() || m_yVariable.empty())
            {
            wxMessageBox(_(L"Please select the X and Y variables."), _(L"Variable Not Specified"),
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
    void InsertScatterPlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* scatter = dynamic_cast<const Graphs::ScatterPlot*>(&graph);
        if (scatter == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = scatter->GetPropertyTemplate(L"dataset");
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

        // load the actual column names used by the graph
        // (property templates may contain unexpanded {{placeholders}})
        m_xVariable = scatter->GetXColumnName();
        m_yVariable = scatter->GetYColumnName();
        const auto& series = scatter->GetSeriesList();
        if (!series.empty())
            {
            m_groupVariable = series.front().GetGroupColumnName().value_or(wxString{});
            }
        UpdateVariableLabels();

        // scatter-specific options
        m_showRegressionLines = scatter->IsShowingRegressionLines();
        m_showConfidenceBands = scatter->IsShowingConfidenceBands();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
