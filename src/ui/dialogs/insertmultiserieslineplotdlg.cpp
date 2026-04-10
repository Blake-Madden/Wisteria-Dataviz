///////////////////////////////////////////////////////////////////////////////
// Name:        insertmultiserieslineplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertmultiserieslineplotdlg.h"
#include "../../graphs/multi_series_lineplot.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertMultiSeriesLinePlotDlg::InsertMultiSeriesLinePlotDlg(
        Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
        const wxString& caption, const wxWindowID id, const wxPoint& pos, const wxSize& size,
        const long style, EditMode editMode)
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
    void InsertMultiSeriesLinePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAnnotationsPage();
        CreateAxisOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Multi-Series Line Plot"), ID_OPTIONS_SECTION,
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

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Y (series):"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarsLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarsLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_yVarsLabel, wxSizerFlags{}.CenterVertical());

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"X (axis):"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // line options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Auto spline"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_autoSpline)),
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
    void InsertMultiSeriesLinePlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariables.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertMultiSeriesLinePlotDlg::OnSelectVariables()
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
                  .Label(_(L"Y (series)"))
                  .SingleSelection(false)
                  .Required(true)
                  .DefaultVariables(m_yVariables)
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"X (axis)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_xVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_xVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_yVariables = dlg.GetSelectedVariables(0);

        const auto xVars = dlg.GetSelectedVariables(1);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertMultiSeriesLinePlotDlg::UpdateVariableLabels()
        {
        wxString yText;
        for (size_t i = 0; i < m_yVariables.size(); ++i)
            {
            if (i > 0)
                {
                yText += L", ";
                }
            yText += m_yVariables[i];
            }
        m_yVarsLabel->SetLabel(yText);
        m_xVarLabel->SetLabel(m_xVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertMultiSeriesLinePlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertMultiSeriesLinePlotDlg::GetSelectedDataset() const
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
    bool InsertMultiSeriesLinePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_yVariables.empty() || m_xVariable.empty())
            {
            wxMessageBox(_(L"Please select the Y and X variables."), _(L"Variable Not Specified"),
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
    void InsertMultiSeriesLinePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* linePlot = dynamic_cast<const Graphs::LinePlot*>(&graph);
        if (linePlot == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = linePlot->GetPropertyTemplate(L"dataset");
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

        // load Y column names from the lines' labels
        m_yVariables.clear();
        for (size_t i = 0; i < linePlot->GetLineCount(); ++i)
            {
            const auto& lineLabel = linePlot->GetLines()[i].GetText();
            if (!lineLabel.empty())
                {
                m_yVariables.push_back(lineLabel);
                }
            }

        // load X column name
        m_xVariable = linePlot->GetXColumnName();
        UpdateVariableLabels();

        // line-specific options
        m_autoSpline = linePlot->IsAutoSplining();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
