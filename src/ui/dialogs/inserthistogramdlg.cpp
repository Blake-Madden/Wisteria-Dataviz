///////////////////////////////////////////////////////////////////////////////
// Name:        inserthistogramdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inserthistogramdlg.h"
#include "../../graphs/histogram.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertHistogramDlg::InsertHistogramDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                           wxWindow* parent, const wxString& caption,
                                           const wxWindowID id, const wxPoint& pos,
                                           const wxSize& size, const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertHistogramDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAxisOptionsPage();
        CreateAnnotationsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Histogram"), ID_OPTIONS_SECTION, true);

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
        m_continuousVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_continuousVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // binning options
        auto* binSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        // binning method
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Binning method:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* binMethodChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_binningMethod));
            binMethodChoice->Append(_(L"Unique values"));
            binMethodChoice->Append(_(L"By range"));
            binMethodChoice->Append(_(L"By integer range"));
            binSizer->Add(binMethodChoice);
            }

        // rounding method
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Rounding:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* roundChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_roundingMethod));
            roundChoice->Append(_(L"Round"));
            roundChoice->Append(_(L"Round down"));
            roundChoice->Append(_(L"Round up"));
            roundChoice->Append(_(L"No rounding"));
            binSizer->Add(roundChoice);
            }

        // interval display
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Interval display:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* intervalChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_intervalDisplay));
            intervalChoice->Append(_(L"Cutpoints"));
            intervalChoice->Append(_(L"Midpoints"));
            binSizer->Add(intervalChoice);
            }

        // bin label display
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Bin labels:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* labelChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_binLabelDisplay));
            labelChoice->Append(_(L"Value"));
            labelChoice->Append(_(L"Percentage"));
            labelChoice->Append(_(L"Value & percentage"));
            labelChoice->Append(_(L"No labels"));
            labelChoice->Append(_(L"Name"));
            labelChoice->Append(_(L"Name & value"));
            labelChoice->Append(_(L"Name & percentage"));
            binSizer->Add(labelChoice);
            }

        optionsSizer->Add(binSizer, wxSizerFlags{}.Border());

        // checkboxes
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show full range of values"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showFullRange)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Use neat intervals"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_neatIntervals)),
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
    void InsertHistogramDlg::OnDatasetChanged()
        {
        m_continuousVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHistogramDlg::OnSelectVariables()
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
                  .Label(_(L"Continuous"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_continuousVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_continuousVariable })
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

        const auto contVars = dlg.GetSelectedVariables(0);
        m_continuousVariable = contVars.empty() ? wxString{} : contVars.front();

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHistogramDlg::UpdateVariableLabels()
        {
        m_continuousVarLabel->SetLabel(m_continuousVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertHistogramDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertHistogramDlg::GetSelectedDataset() const
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
    bool InsertHistogramDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_continuousVariable.empty())
            {
            wxMessageBox(_(L"Please select the continuous variable."), _(L"Variable Not Specified"),
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
    void InsertHistogramDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* histogram = dynamic_cast<const Graphs::Histogram*>(&graph);
        if (histogram == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = histogram->GetPropertyTemplate(L"dataset");
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
        m_continuousVariable = histogram->GetContinuousColumnName();
        m_groupVariable = histogram->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // binning options
        m_binningMethod = static_cast<int>(histogram->GetBinningMethod());
        m_roundingMethod = static_cast<int>(histogram->GetRoundingMethod());
        m_intervalDisplay = static_cast<int>(histogram->GetIntervalDisplay());
        m_binLabelDisplay = static_cast<int>(histogram->GetBinLabelDisplay());
        m_showFullRange = histogram->IsShowingFullRangeOfValues();
        m_neatIntervals = histogram->IsUsingNeatIntervals();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
